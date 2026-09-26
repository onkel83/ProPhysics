/* ==========================================================================
 * ProPhysics - Amplitude Modul
 * File: ProPhysics_Amp.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Unitare Dynamik auf amp_grid (U1-U5, Option B):
 *   - Signed-Permutation-Tick
 *   - Kontextabhaengige Permutationen (U2)
 *   - Sequenzieller und farbiger Kanten-Transport
 *   - Unitärer Wave-Step
 *   - GP-Selbstkopplung (Standard, Dilated, Spin)
 *   - U6-Fuehrungsgleichung, U5-Invariante
 *   - Apply_Amp_Step als Dispatch-Funktion des Ticks
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer
  * ========================================================================== */

  /* Wendet eine 8x8-Matrix U auf einen Amplitudenvektor an. */
static void pro_amp_apply_matrix(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE],
    const ProAmpVector* in,
    ProAmpVector* out)
{
    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        ProAmpQ31 sum = 0;
        for (uint8_t j = 0; j < PRO_AMP_BASIS_SIZE; ++j) {
            sum = pro_amp_add_q31(sum,
                pro_amp_mul_q31(U[i][j], in->coeff[j]));
        }
        out->coeff[i] = sum;
    }
}

/* Komplexe Rotation eines Q31-Werts um phase_rad mit Saturation.
 * Zentraler Baustein aller Phase-Steps und des Wave-Steps. */
static inline ProAmpQ31 pro_amp_rotate_by_phase(ProAmpQ31 a, double phase_rad)
{
    const int64_t re = (int64_t)pro_amp_real(a);
    const int64_t im = (int64_t)pro_amp_imag(a);
    const double c_rot = cos(phase_rad);
    const double s_rot = sin(phase_rad);
    const int64_t new_re = (int64_t)llround((double)re * c_rot - (double)im * s_rot);
    const int64_t new_im = (int64_t)llround((double)re * s_rot + (double)im * c_rot);
    return pro_amp_pack(pro_sat_i32(new_re), pro_sat_i32(new_im));
}

/* |a|^2 normiert auf Q62 (physikalisch 1.0 = 1.0). */
static inline double pro_amp_abs2_q62(ProAmpQ31 a, double inv_q62)
{
    return (double)pro_amp_abs2(a) * inv_q62;
}

/* Stellt grid_dim_shift und grid_dim_mask aus grid_dim ein (idempotent).
 * Voraussetzung: grid_dim ist Zweierpotenz. */
static void pro_ensure_grid_shift(ProUniverse* pu)
{
    if (!pu) return;
    if (pu->grid_dim_shift != 0 || pu->grid_dim_mask != 0) return;
    const uint32_t d = pu->grid_dim;
    if (d == 0u || (d & (d - 1u)) != 0u) return;
    uint32_t shift = 0;
    while ((1u << shift) < d) shift++;
    pu->grid_dim_shift = shift;
    pu->grid_dim_mask = d - 1u;
}

/* Waehlt die 4 (2D) oder 6 (3D) Nachbarkanaele aus. Liefert n_nb. */
static uint8_t pro_amp_select_neighbor_channels(
    const ProUniverse* pu, uint8_t ch_arr[6])
{
    if (pu->grid_ndim == 3u) {
        for (uint8_t i = 0; i < 6u; ++i)
            ch_arr[i] = PRO_NEIGHBOR_CHANNELS[i];
        return 6u;
    }
    ch_arr[0] = 0u; ch_arr[1] = 1u; ch_arr[2] = 2u; ch_arr[3] = 3u;
    return 4u;
}

/* Auto-Sync von Tensor-Paaren und Dichte-Trilogie nach dem Tick.
 * Wird von Apply_Amp_Step (Standard- und Dirac-Pfad) verwendet. */
static void pro_amp_auto_sync(ProUniverse* pu)
{
    if (pu->auto_sync_tensor && pu->tensor_pairs) {
        ProPhysics_Tensor_Sync_To_Amp(pu);
    }
    if (pu->auto_sync_density) {
        ProPhysics_TensorDensity_Sync_All_From_Pairs(pu);
        ProPhysics_FockDensity_Sync_All_From_Fock(pu);
    }
}

/* Sequenzieller Transport: jede Kante (k, nb) mit nb > k wird einmal
 * transformiert, in Knotenreihenfolge. Kein Schachbrett-Muster. */
static void pro_transport_sequential_core(
    ProUniverse* pu,
    int64_t c_num, int64_t s_num, int64_t denom)
{
    memcpy(pu->amp_scratch, pu->amp_grid,
        (size_t)pu->total_nodes * sizeof(ProAmpVector));
    ProAmpVector* src = pu->amp_grid;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const ProRegister* r = &pu->reg_source[k];
        for (uint8_t c = 0; c < 4u; ++c) {
            const uint64_t nb = r->channels[c];
            if (nb >= pu->total_nodes || nb <= k) continue;
            pro_edge_transport_2node(&src[k], &src[nb], c_num, s_num, denom);
        }
    }
}

/* Farbiger Transport mit Schachbrett-Muster.
 *
 * Parameter:
 *   sweep_channels : Kanal pro Sweep (Laenge n_sweeps)
 *   n_sweeps       : 4 (2D) oder 6 (3D)
 *   use_3d_parity  : 1 = (x+y+z) & 1, 0 = (x+y) & 1
 *   skip_marks     : NULL oder tensor_marks-Array (nur 2D erlaubt)
 *
 * Bei skip_marks != NULL werden markierte Knoten und markierte Nachbarn
 * uebersprungen. Wird vom Tensor-Marks-Pfad in Apply_Amp_Step genutzt. */
static void pro_transport_colored_core(
    ProUniverse* pu,
    int64_t c_num, int64_t s_num, int64_t denom,
    uint32_t grid_dim, uint32_t dim_shift, uint32_t dim_mask,
    const uint8_t* sweep_channels, uint8_t n_sweeps,
    int use_3d_parity,
    const uint8_t* skip_marks)
{
    memcpy(pu->amp_scratch, pu->amp_grid,
        (size_t)pu->total_nodes * sizeof(ProAmpVector));
    ProAmpVector* buf = pu->amp_scratch;

    for (uint8_t sweep = 0; sweep < n_sweeps; ++sweep) {
        const uint8_t  channel = sweep_channels[sweep];
        const uint32_t parity = (uint32_t)(sweep & 1u);

        for (uint64_t k = 0; k < pu->total_nodes; ++k) {
            if (skip_marks && skip_marks[k]) continue;

            uint32_t x, y, z = 0u;
            if (dim_mask != 0u) {
                x = (uint32_t)k & dim_mask;
                y = (uint32_t)k >> dim_shift;
                if (use_3d_parity) z = y >> dim_shift;
            }
            else {
                x = (uint32_t)(k % (uint64_t)grid_dim);
                y = (uint32_t)(k / (uint64_t)grid_dim);
            }

            const uint32_t sum_xyz = use_3d_parity ? (x + y + z) : (x + y);
            if ((sum_xyz & 1u) != parity) continue;

            const uint64_t nb = pu->reg_source[k].channels[channel];
            if (nb >= pu->total_nodes || nb == k) continue;
            if (skip_marks && skip_marks[nb]) continue;

            pro_edge_transport_2node(&buf[k], &buf[nb],
                c_num, s_num, denom);
        }
    }

    ProAmpVector* tmp = pu->amp_grid;
    pu->amp_grid = pu->amp_scratch;
    pu->amp_scratch = tmp;
}

/* Wave-Step-Kern: unitaere Rotation pro Basis-Koeffizient, Winkel
 * abhaengig von der lokalen Nachbar-Energie.
 *
 * Parameter:
 *   n_nb, ch_arr  : 4 oder 6 Nachbarkanaele
 *   skip_marks    : NULL oder tensor_marks-Array
 *
 * Bei skip_marks != NULL bleiben markierte Knoten unveraendert und
 * markierte Nachbarn tragen nicht zur Energie bei. */
static void pro_wave_step_core(
    ProUniverse* pu,
    uint32_t phase_step_q15,
    uint8_t n_nb, const uint8_t* ch_arr,
    const uint8_t* skip_marks)
{
    const double s = (double)phase_step_q15 / 32768.0;
    const uint64_t n = pu->total_nodes;
    const int64_t center_coeff = -(int64_t)n_nb;

    ProAmpVector* src = pu->amp_grid;
    ProAmpVector* dst = pu->amp_scratch;

    for (uint64_t k = 0; k < n; ++k) {
        if (skip_marks && skip_marks[k]) {
            dst[k] = src[k];
            continue;
        }
        const ProRegister* r = &pu->reg_source[k];
        const ProAmpVector* cur = &src[k];

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            const int64_t Re_c = (int64_t)pro_amp_real(cur->coeff[b]);
            const int64_t Im_c = (int64_t)pro_amp_imag(cur->coeff[b]);

            int64_t Re_L = center_coeff * Re_c;
            int64_t Im_L = center_coeff * Im_c;

            for (uint8_t i = 0; i < n_nb; ++i) {
                const uint64_t nb = r->channels[ch_arr[i]];
                if (nb >= n || nb == k) continue;
                if (skip_marks && skip_marks[nb]) continue;
                Re_L += (int64_t)pro_amp_real(src[nb].coeff[b]);
                Im_L += (int64_t)pro_amp_imag(src[nb].coeff[b]);
            }

            const double Re_c_d = (double)Re_c;
            const double Im_c_d = (double)Im_c;
            const double local_energy =
                Re_c_d * (double)Re_L + Im_c_d * (double)Im_L;
            const double norm_sq =
                Re_c_d * Re_c_d + Im_c_d * Im_c_d;

            if (norm_sq < 1.0) {
                dst[k].coeff[b] = cur->coeff[b];
                continue;
            }

            const double d_theta = s * local_energy / norm_sq;
            dst[k].coeff[b] = pro_amp_rotate_by_phase(cur->coeff[b], d_theta);
        }
    }

    ProAmpVector* tmp = pu->amp_grid;
    pu->amp_grid = pu->amp_scratch;
    pu->amp_scratch = tmp;
}

/* ==========================================================================
 * Unitare Tick-Helfer (Verify)
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Unitary_Tick(ProUniverse* pu,
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE])
{
    if (!pu || !pu->amp_grid || !U) return;

    ProAmpVector tmp;
    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        pro_amp_apply_matrix(U, &pu->amp_grid[k], &tmp);
        pu->amp_grid[k] = tmp;
    }
}

PROPHYSICS_API double ProPhysics_Verify_Unitarity(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE])
{
    if (!U) return 1e300;

    double max_err = 0.0;

    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        for (uint8_t j = 0; j < PRO_AMP_BASIS_SIZE; ++j) {
            double re_sum = 0.0;
            double im_sum = 0.0;

            for (uint8_t k = 0; k < PRO_AMP_BASIS_SIZE; ++k) {
                const ProAmpQ31 a = U[i][k];
                const ProAmpQ31 b = U[j][k];

                const double a_re = (double)pro_amp_real(a) / 2147483647.0;
                const double a_im = (double)pro_amp_imag(a) / 2147483647.0;
                const double b_re = (double)pro_amp_real(b) / 2147483647.0;
                const double b_im = (double)pro_amp_imag(b) / 2147483647.0;

                re_sum += a_re * b_re + a_im * b_im;
                im_sum += a_im * b_re - a_re * b_im;
            }

            const double expected_re = (i == j) ? 1.0 : 0.0;
            const double dre = re_sum - expected_re;
            const double dim = im_sum;

            const double err = sqrt(dre * dre + dim * dim);
            if (err > max_err) max_err = err;
        }
    }
    return max_err;
}

PROPHYSICS_API double ProPhysics_Verify_U5_Commutator(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE])
{
    if (!U) return 1e300;

    double max_err = 0.0;

    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        for (uint8_t j = 0; j < PRO_AMP_BASIS_SIZE; ++j) {
            if (PRO_U5_W[i] == PRO_U5_W[j]) continue;

            const ProAmpQ31 a = U[i][j];
            const double re = (double)pro_amp_real(a) / 2147483647.0;
            const double im = (double)pro_amp_imag(a) / 2147483647.0;
            const double mag = sqrt(re * re + im * im);

            if (mag > max_err) max_err = mag;
        }
    }
    return max_err;
}

PROPHYSICS_API ProU128 ProPhysics_Weighted_Norm(
    const ProUniverse* pu, uint64_t node_idx)
{
    if (!pu || !pu->amp_grid) return PRO_U128_ZERO;
    if (node_idx >= pu->total_nodes) return PRO_U128_ZERO;

    ProU128 norm = PRO_U128_ZERO;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        const uint64_t sq = pro_amp_abs2(pu->amp_grid[node_idx].coeff[b]);
        norm = pro_u128_add(norm,
            pro_u128_mul_small(pro_u128_from_u64(sq),
                (uint64_t)PRO_U5_W[b]));
    }
    return norm;
}

/* ==========================================================================
 * Signed-Permutation-Tick
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Signed_Permutation(ProUniverse* pu,
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE])
{
    if (!pu || !pu->amp_grid || !perm || !sign) return;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const ProAmpVector src = pu->amp_grid[k];
        ProAmpVector dst = { { 0 } };
        pro_apply_signed_perm_vec(&src, &dst, perm, sign);
        pu->amp_grid[k] = dst;
    }
}

PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_Unitarity(
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE])
{
    if (!perm || !sign) return false;

    uint8_t seen[PRO_AMP_BASIS_SIZE] = { 0 };
    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        if (perm[i] >= PRO_AMP_BASIS_SIZE) return false;
        if (seen[perm[i]] != 0) return false;
        seen[perm[i]] = 1;
        if (sign[i] != +1 && sign[i] != -1) return false;
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_U5(
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE])
{
    if (!perm || !sign) return false;
    (void)sign;

    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        if (perm[i] >= PRO_AMP_BASIS_SIZE) return false;
        if (PRO_U5_W[perm[i]] != PRO_U5_W[i]) return false;
    }
    return true;
}

/* ==========================================================================
 * Kontextabhaengige signed permutations
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Compute_Context_Perm(
    const ProUniverse* pu,
    uint64_t node_idx,
    uint8_t  perm_out[PRO_AMP_BASIS_SIZE],
    int8_t   sign_out[PRO_AMP_BASIS_SIZE])
{
    if (!perm_out || !sign_out) return;

    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        perm_out[i] = i;
        sign_out[i] = +1;
    }
    if (!pu || !pu->ur_grid || !pu->reg_source) return;
    if (node_idx >= pu->total_nodes) return;

    uint64_t h = node_idx * 0x9e3779b97f4a7c15ULL;

    const ProRegister* r = &pu->reg_source[node_idx];
    for (uint8_t c = 0; c < (uint8_t)PRO_EPR_CHANNEL; ++c) {
        const uint64_t nb = r->channels[c];
        if (nb >= pu->total_nodes || nb == node_idx) continue;

        uint64_t amp_hash = 0;
        if (pu->amp_grid) {
            const ProAmpVector* vn = &pu->amp_grid[nb];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                const uint32_t re = (uint32_t)pro_amp_real(vn->coeff[b]);
                const uint32_t im = (uint32_t)pro_amp_imag(vn->coeff[b]);
                amp_hash = amp_hash * 0x9e3779b97f4a7c15ULL
                    + ((uint64_t)re << 32 | (uint64_t)im);
            }
        }

        h ^= amp_hash + 0x9e3779b97f4a7c15ULL * (uint64_t)(c + 1u);
    }

    h = pro_splitmix64(h);

    if ((h >> 0) & 1u) { perm_out[1] = 2; perm_out[2] = 1; }
    sign_out[1] = ((h >> 1) & 1u) ? +1 : -1;
    sign_out[2] = ((h >> 2) & 1u) ? +1 : -1;

    if ((h >> 3) & 1u) { perm_out[3] = 4; perm_out[4] = 3; }
    sign_out[3] = ((h >> 4) & 1u) ? +1 : -1;
    sign_out[4] = ((h >> 5) & 1u) ? +1 : -1;

    sign_out[5] = ((h >> 6) & 1u) ? +1 : -1;
    sign_out[6] = ((h >> 7) & 1u) ? +1 : -1;
    sign_out[7] = ((h >> 8) & 1u) ? +1 : -1;
}

PROPHYSICS_API void ProPhysics_Apply_Context_Tick(ProUniverse* pu)
{
    if (!pu || !pu->amp_grid || !pu->ur_grid || !pu->reg_source) return;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        uint8_t perm[PRO_AMP_BASIS_SIZE];
        int8_t  sign[PRO_AMP_BASIS_SIZE];
        ProPhysics_Compute_Context_Perm(pu, k, perm, sign);

        const ProAmpVector src = pu->amp_grid[k];
        ProAmpVector dst = { { 0 } };
        pro_apply_signed_perm_vec(&src, &dst, perm, sign);
        pu->amp_grid[k] = dst;
    }
}

/* ==========================================================================
 * Transport-Koeffizienten
 *
 * Rationale Approximation der Koeffizienten fuer eine 2-Knoten-Rotation
 * mit Winkel theta. Ergebnis als (c_num, s_num, denom) mit
 * cos(theta) ~ c_num/denom und sin(theta) ~ s_num/denom.
 * Gecached in pu->last_* (pu darf NULL sein).
 * ========================================================================== */

void pro_transport_coeffs(ProUniverse* pu, uint32_t theta_q15,
    int64_t* c_num_out,
    int64_t* s_num_out,
    int64_t* denom_out)
{
    if (pu && pu->transport_cache_valid
        && pu->last_theta_q15 == (int64_t)theta_q15) {
        *c_num_out = pu->last_c_num;
        *s_num_out = pu->last_s_num;
        *denom_out = pu->last_denom;
        return;
    }

    const double theta = ((double)theta_q15 / 32768.0) * PRO_2PI * 0.5;
    const double t_target = tan(theta * 0.5);

    const int64_t Q_MAX = 2048;
    int64_t best_p = 0, best_q = 1;
    double  best_err = fabs(t_target);

    for (int64_t q = 1; q <= Q_MAX; ++q) {
        const int64_t p = (int64_t)llround(t_target * (double)q);
        const double err = fabs((double)p / (double)q - t_target);
        if (err < best_err) {
            best_err = err;
            best_p = p;
            best_q = q;
        }
    }

    const int64_t p = best_p;
    const int64_t q = best_q;

    *c_num_out = q * q - p * p;
    *s_num_out = 2 * p * q;
    *denom_out = q * q + p * p;

    if (pu) {
        pu->last_theta_q15 = (int64_t)theta_q15;
        pu->last_c_num = *c_num_out;
        pu->last_s_num = *s_num_out;
        pu->last_denom = *denom_out;
        pu->transport_cache_valid = 1u;
    }
}

/* ==========================================================================
 * 2-Knoten-Unitare (Baustein aller Transport-Varianten)
 * ========================================================================== */

void pro_edge_transport_2node(
    ProAmpVector* va, ProAmpVector* vb,
    int64_t c_num, int64_t s_num, int64_t denom)
{
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        const int64_t a_re = (int64_t)pro_amp_real(va->coeff[b]);
        const int64_t a_im = (int64_t)pro_amp_imag(va->coeff[b]);
        const int64_t b_re = (int64_t)pro_amp_real(vb->coeff[b]);
        const int64_t b_im = (int64_t)pro_amp_imag(vb->coeff[b]);

        const int64_t new_a_re = pro_div_round(c_num * a_re - s_num * b_im, denom);
        const int64_t new_a_im = pro_div_round(c_num * a_im + s_num * b_re, denom);
        const int64_t new_b_re = pro_div_round(c_num * b_re - s_num * a_im, denom);
        const int64_t new_b_im = pro_div_round(c_num * b_im + s_num * a_re, denom);

        va->coeff[b] = pro_amp_pack(pro_sat_i32(new_a_re), pro_sat_i32(new_a_im));
        vb->coeff[b] = pro_amp_pack(pro_sat_i32(new_b_re), pro_sat_i32(new_b_im));
    }
}

/* ==========================================================================
 * Oeffentliche Transport-Varianten
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Edge_Transport(ProUniverse* pu,
    uint32_t theta_q15)
{
    if (!pu || !pu->amp_grid || !pu->amp_scratch || !pu->reg_source) return;
    if (theta_q15 == 0u) return;
    if (theta_q15 > 32767u) theta_q15 = 32767u;

    int64_t c_num = 0, s_num = 0, denom = 1;
    pro_transport_coeffs(pu, theta_q15, &c_num, &s_num, &denom);

    pro_transport_sequential_core(pu, c_num, s_num, denom);
}

PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim)
{
    if (!pu || !pu->amp_grid || !pu->amp_scratch || !pu->reg_source) return;
    if (theta_q15 == 0u || grid_dim == 0u) return;
    if (theta_q15 > 32767u) theta_q15 = 32767u;

    int64_t c_num = 0, s_num = 0, denom = 1;
    pro_transport_coeffs(pu, theta_q15, &c_num, &s_num, &denom);

    uint32_t dim_shift = 0u, dim_mask = 0u;
    if ((grid_dim & (grid_dim - 1u)) == 0u) {
        uint32_t s = 0;
        while ((1u << s) < grid_dim) s++;
        dim_shift = s;
        dim_mask = grid_dim - 1u;
    }

    static const uint8_t sweep_channel[4] = { 2u, 2u, 0u, 0u };
    pro_transport_colored_core(pu, c_num, s_num, denom,
        grid_dim, dim_shift, dim_mask,
        sweep_channel, 4u, 0, NULL);
}

PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored_3D(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim)
{
    if (!pu || !pu->amp_grid || !pu->amp_scratch || !pu->reg_source) return;
    if (theta_q15 == 0u || grid_dim == 0u) return;
    if (theta_q15 > 32767u) theta_q15 = 32767u;
    if ((grid_dim & (grid_dim - 1u)) != 0u) return;

    int64_t c_num = 0, s_num = 0, denom = 1;
    pro_transport_coeffs(pu, theta_q15, &c_num, &s_num, &denom);

    uint32_t dim_shift = 0u;
    while ((1u << dim_shift) < grid_dim) dim_shift++;
    const uint32_t dim_mask = grid_dim - 1u;

    static const uint8_t sweep_channel[6] = { 0u, 0u, 2u, 2u, 4u, 4u };
    pro_transport_colored_core(pu, c_num, s_num, denom,
        grid_dim, dim_shift, dim_mask,
        sweep_channel, 6u, 1, NULL);
}

/* ==========================================================================
 * Unitärer Wave-Step (oeffentliche Fassung)
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Wave_Step(ProUniverse* pu,
    uint32_t phase_step_q15)
{
    if (!pu || !pu->amp_grid || !pu->reg_source) return;
    if (!pu->amp_scratch) return;
    if (phase_step_q15 == 0u) return;
    if (phase_step_q15 > 32767u) phase_step_q15 = 32767u;

    uint8_t ch_arr[6];
    const uint8_t n_nb = pro_amp_select_neighbor_channels(pu, ch_arr);

    pro_wave_step_core(pu, phase_step_q15, n_nb, ch_arr, NULL);
}

/* ==========================================================================
 * U6 -- Fuehrungsgleichung
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Guiding_Equation(ProUniverse* pu)
{
    if (!pu || !pu->amp_grid || !pu->ur_grid || !pu->u_field) return;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const ProAmpVector* v = &pu->amp_grid[k];

        const uint64_t pos_sq = pro_amp_vec_norm_sq_range(v, 1u, 3u);
        const uint64_t neg_sq = pro_amp_vec_norm_sq_range(v, 3u, 5u);

        ProU128 p_pos = pro_u128_from_u64(pos_sq);
        ProU128 p_neg = pro_u128_from_u64(neg_sq);

        const ProU128 total = pro_u128_add(p_pos, p_neg);
        if (pro_u128_is_zero(total)) continue;

        const uint64_t u = pu->u_field[k];
        const ProU128 lhs = pro_u128_mul_small(total, u);
        const ProU128 rhs = pro_u128_shl(p_pos, 16);

        pu->ur_grid[k].type_state =
            (pro_u128_cmp(lhs, rhs) < 0) ? UR_POSITRON_CW : UR_NEGATRON_CCW;
    }
}

/* ==========================================================================
 * U5-Invariante
 * ========================================================================== */

PROPHYSICS_API ProU128 ProPhysics_Measure_Amp_Invariant(
    const ProUniverse* pu)
{
    if (!pu || !pu->amp_grid) return PRO_U128_ZERO;
    ProU128 sum = PRO_U128_ZERO;
    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        if (pu->shared.parent && pu->shared.parent[k] != k) continue;

        const ProAmpVector* v = &pu->amp_grid[k];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            if (b == 0u || b == 6u || b == 7u) continue;
            const uint64_t sq = pro_amp_abs2(v->coeff[b]);
            sum = pro_u128_add(sum,
                pro_u128_mul_small(pro_u128_from_u64(sq),
                    (uint64_t)PRO_U5_W[b]));
        }
    }
    return sum;
}

PROPHYSICS_API const uint16_t* ProPhysics_Get_U_Field(const ProUniverse* pu)
{
    return pu ? pu->u_field : NULL;
}

/* ==========================================================================
 * Apply_Amp_Step -- Hauptfunktion des SDK-Ticks.
 *
 * Dispatch-Reihenfolge:
 *   1. su2_dynamics_active  -> Leapfrog (kein return)
 *   2. shared.active        -> Shared_Tick_Reps (return)
 *   3. dirac_active         -> Apply_Dirac_Step (return)
 *   4. Standard-Pfad:
 *      Context -> Transport -> Wave -> Tensor-Kopplungen -> Auto-Sync
 *
 * Prioritaet 1 laeuft parallel, weil SU(2)-Dynamik nur ProEdge
 * modifiziert. Prioritaeten 2 und 3 sind vollstaendige Alternativen.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Amp_Step(ProUniverse* pu,
    uint32_t phase_step_q15)
{
    if (!pu || !pu->amp_grid) return;

    /* SU(2)-Link-Dynamik (Leapfrog). Nur aktiv bei expliziter
     * Aktivierung durch ProPhysics_Enable_SU2_Dynamics. */
    if (pu->su2_dynamics_active) {
        ProPhysics_Apply_SU2_Tick(pu, PRO_SU2_LEAPFROG_DT_Q15);
    }

    /* Klassen-Tick bei aktivem shared reference. */
    if (pu->shared.active) {
        ProPhysics_Shared_Tick_Reps(pu, phase_step_q15);
        return;
    }

    /* Dirac-Pfad. */
    if (pu->dirac_active) {
        ProPhysics_Apply_Dirac_Step(pu,
            pu->dirac_mass_q15, phase_step_q15);
        pro_amp_auto_sync(pu);
        return;
    }

    pro_ensure_grid_shift(pu);

    /* Schritt 1: Kontext-Permutation. */
    if (pu->tensor_marks) {
        for (uint64_t k = 0; k < pu->total_nodes; ++k) {
            if (pu->tensor_marks[k]) continue;
            uint8_t perm[PRO_AMP_BASIS_SIZE];
            int8_t  sign[PRO_AMP_BASIS_SIZE];
            ProPhysics_Compute_Context_Perm(pu, k, perm, sign);
            const ProAmpVector src = pu->amp_grid[k];
            ProAmpVector dst = { { 0 } };
            pro_apply_signed_perm_vec(&src, &dst, perm, sign);
            pu->amp_grid[k] = dst;
        }
    }
    else {
        ProPhysics_Apply_Context_Tick(pu);
    }

    /* Schritt 2: Kanten-Transport. */
    if (pu->grid_dim > 0u) {
        int64_t c_num = 0, s_num = 0, denom = 1;
        pro_transport_coeffs(pu, PRO_DEFAULT_TRANSPORT_THETA_Q15,
            &c_num, &s_num, &denom);

        if (pu->grid_ndim == 3u) {
            uint32_t dim_shift = 0u;
            while ((1u << dim_shift) < pu->grid_dim) dim_shift++;
            const uint32_t dim_mask = pu->grid_dim - 1u;
            static const uint8_t sweep_channel[6] = { 0u, 0u, 2u, 2u, 4u, 4u };
            pro_transport_colored_core(pu, c_num, s_num, denom,
                pu->grid_dim, dim_shift, dim_mask,
                sweep_channel, 6u, 1, NULL);
        }
        else if (pu->tensor_marks) {
            static const uint8_t sweep_channel[4] = { 2u, 2u, 0u, 0u };
            pro_transport_colored_core(pu, c_num, s_num, denom,
                pu->grid_dim, pu->grid_dim_shift, pu->grid_dim_mask,
                sweep_channel, 4u, 0, pu->tensor_marks);
        }
        else {
            uint32_t dim_shift = 0u, dim_mask = 0u;
            if ((pu->grid_dim & (pu->grid_dim - 1u)) == 0u) {
                uint32_t s = 0;
                while ((1u << s) < pu->grid_dim) s++;
                dim_shift = s;
                dim_mask = pu->grid_dim - 1u;
            }
            static const uint8_t sweep_channel[4] = { 2u, 2u, 0u, 0u };
            pro_transport_colored_core(pu, c_num, s_num, denom,
                pu->grid_dim, dim_shift, dim_mask,
                sweep_channel, 4u, 0, NULL);
        }
    }
    else {
        int64_t c_num = 0, s_num = 0, denom = 1;
        pro_transport_coeffs(pu, PRO_DEFAULT_TRANSPORT_THETA_Q15,
            &c_num, &s_num, &denom);
        pro_transport_sequential_core(pu, c_num, s_num, denom);
    }

    /* Schritt 3: Wave-Step. */
    if (phase_step_q15 > 0u) {
        uint8_t ch_arr[6];
        const uint8_t n_nb = pro_amp_select_neighbor_channels(pu, ch_arr);
        pro_wave_step_core(pu, phase_step_q15, n_nb, ch_arr,
            pu->tensor_marks);
    }

    /* Schritt 4: Tensor-Paar-Wechselwirkungen. */
    if (pu->tensor_pairs) {
        for (uint32_t i = 0; i < pu->tensor_pair_capacity; ++i) {
            const ProAmpTensorPair* p = &pu->tensor_pairs[i];
            if (p->active == 0u) continue;
            if (p->coupling_q15 != 0) {
                ProPhysics_Tensor_Apply_XX_Step(pu, i);
            }
            if (p->hopping_theta_q15 != 0) {
                ProPhysics_Tensor_Apply_Hopping(pu, i,
                    p->hopping_i, p->hopping_j,
                    p->hopping_theta_q15);
            }
        }
    }

    /* Schritt 5+6: Auto-Sync Tensor und Dichte. */
    pro_amp_auto_sync(pu);
}

/* ==========================================================================
 * Nichtlineare GP-Selbstkopplung (Standard)
 *
 * phase_b = g * |c_b|^2 * dt, angewendet auf alle Basis-Komponenten.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step(
    ProUniverse* pu,
    int32_t  g_q15_signed,
    uint32_t phase_step_q15)
{
    if (!pu || !pu->amp_grid) return;
    if (g_q15_signed == 0) return;
    if (phase_step_q15 == 0u) return;

    const uint64_t n = pu->total_nodes;
    const double g = (double)g_q15_signed / 32768.0;
    const double dt = (double)phase_step_q15 / 32768.0;
    const double inv_q62 = 1.0 / 4611686018427387904.0;

    for (uint64_t k = 0; k < n; ++k) {
        ProAmpVector* v = &pu->amp_grid[k];

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            const ProAmpQ31 a = v->coeff[b];
            if (a == 0) continue;

            const double norm_sq = pro_amp_abs2_q62(a, inv_q62);
            const double phase_rad = g * norm_sq * dt;
            if (phase_rad == 0.0) continue;

            v->coeff[b] = pro_amp_rotate_by_phase(a, phase_rad);
        }
    }
}

/* ==========================================================================
 * GP-Selbstkopplung mit Lorentz-Dilatation
 *
 * phase_b = gamma_inv(k, b) * g * |c_b|^2 * dt,
 * mit gamma_inv aus dem lokalen Phasengradienten (+x, +y).
 * Nur 2D.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(
    ProUniverse* pu,
    int32_t  g_q15_signed,
    uint32_t phase_step_q15)
{
    if (!pu || !pu->amp_grid) return;
    if (g_q15_signed == 0) return;
    if (phase_step_q15 == 0u) return;
    if (pu->grid_dim < 2u) return;

    pro_ensure_grid_shift(pu);

    const uint32_t dim = pu->grid_dim;
    const uint32_t dim_shift = pu->grid_dim_shift;
    const uint32_t dim_mask = pu->grid_dim_mask;

    const uint64_t n = pu->total_nodes;
    const double g = (double)g_q15_signed / 32768.0;
    const double dt = (double)phase_step_q15 / 32768.0;
    const double inv_q62 = 1.0 / 4611686018427387904.0;

    const uint64_t AMP_EPS_Q62 = 4611686018ull;

    for (uint64_t k = 0; k < n; ++k) {
        ProAmpVector* v = &pu->amp_grid[k];

        uint32_t x, y;
        if (dim_mask != 0u) {
            x = (uint32_t)k & dim_mask;
            y = (uint32_t)k >> dim_shift;
        }
        else {
            x = (uint32_t)(k % (uint64_t)dim);
            y = (uint32_t)(k / (uint64_t)dim);
        }

        const uint32_t xp = (x + 1u == dim) ? 0u : (x + 1u);
        const uint32_t yp = (y + 1u == dim) ? 0u : (y + 1u);
        const uint64_t k_xp = (uint64_t)y * dim + xp;
        const uint64_t k_yp = (uint64_t)yp * dim + x;

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            const ProAmpQ31 a = v->coeff[b];
            if (a == 0) continue;

            const uint64_t sq = pro_amp_abs2(a);
            if (sq < AMP_EPS_Q62) continue;

            double kx = 0.0, ky = 0.0;

            {
                const ProAmpQ31 an = pu->amp_grid[k_xp].coeff[b];
                if (an != 0) {
                    const int64_t re = (int64_t)pro_amp_real(a);
                    const int64_t im = (int64_t)pro_amp_imag(a);
                    const int64_t re_n = (int64_t)pro_amp_real(an);
                    const int64_t im_n = (int64_t)pro_amp_imag(an);
                    const double d_re = (double)re_n * (double)re
                        + (double)im_n * (double)im;
                    const double d_im = (double)im_n * (double)re
                        - (double)re_n * (double)im;
                    kx = atan2(d_im, d_re);
                }
            }
            {
                const ProAmpQ31 an = pu->amp_grid[k_yp].coeff[b];
                if (an != 0) {
                    const int64_t re = (int64_t)pro_amp_real(a);
                    const int64_t im = (int64_t)pro_amp_imag(a);
                    const int64_t re_n = (int64_t)pro_amp_real(an);
                    const int64_t im_n = (int64_t)pro_amp_imag(an);
                    const double d_re = (double)re_n * (double)re
                        + (double)im_n * (double)im;
                    const double d_im = (double)im_n * (double)re
                        - (double)re_n * (double)im;
                    ky = atan2(d_im, d_re);
                }
            }

            const double sx = sin(kx * 0.5);
            const double sy = sin(ky * 0.5);
            const double beta2 = sx * sx + sy * sy;

            double gamma_inv;
            if (beta2 <= 0.0) gamma_inv = 1.0;
            else if (beta2 >= 1.0) gamma_inv = 0.0;
            else gamma_inv = sqrt(1.0 - beta2);
            if (gamma_inv == 0.0) continue;

            const double norm_sq = (double)sq * inv_q62;
            const double phase_rad = gamma_inv * g * norm_sq * dt;
            if (phase_rad == 0.0) continue;

            v->coeff[b] = pro_amp_rotate_by_phase(a, phase_rad);
        }
    }
}

/* ==========================================================================
 * Spin-1/2-Phase-Step
 *
 * phase_b = sign_b * g_spin * |c_b|^2 * dt,
 * mit sign_b = +1 fuer UR_POSITRON_CW (up), -1 fuer UR_POSITRON_CCW (down).
 * Nur die beiden Spin-Komponenten werden rotiert.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Spin(
    ProUniverse* pu,
    int32_t  g_spin_q15_signed,
    uint32_t phase_step_q15)
{
    if (!pu || !pu->amp_grid) return;
    if (g_spin_q15_signed == 0) return;
    if (phase_step_q15 == 0u) return;

    const uint64_t n = pu->total_nodes;
    const double g = (double)g_spin_q15_signed / 32768.0;
    const double dt = (double)phase_step_q15 / 32768.0;
    const double inv_q62 = 1.0 / 4611686018427387904.0;

    for (uint64_t k = 0; k < n; ++k) {
        ProAmpVector* v = &pu->amp_grid[k];

        for (uint8_t b = UR_POSITRON_CW; b <= UR_POSITRON_CCW; ++b) {
            const ProAmpQ31 a = v->coeff[b];
            if (a == 0) continue;

            const double norm_sq = pro_amp_abs2_q62(a, inv_q62);
            const double sign = (b == UR_POSITRON_CW) ? +1.0 : -1.0;
            const double phase_rad = sign * g * norm_sq * dt;
            if (phase_rad == 0.0) continue;

            v->coeff[b] = pro_amp_rotate_by_phase(a, phase_rad);
        }
    }
}

/* ==========================================================================
 * End of ProPhysics_Amp.c
 * ========================================================================== */