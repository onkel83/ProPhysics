/* ==========================================================================
 * ProPhysics - Density Modul
 * File: ProPhysics_Density.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Drei Dichte-Ebenen:
 *   - ProDensityMatrix  (8x8,    Knoten-Hilbertraum)
 *   - ProTensorDensity  (64x64,  Tensor-Paar-Hilbertraum)
 *   - ProFockDensity    (256x256, Fock-Hilbertraum)
 *
 * Konvention:
 *   Tr(rho_stored) = 2^31 entspricht physikalisch 1.0.
 *   Interne Rechnung in double, Speicherung in Q31.
 *
 * Interne Helfer:
 *   - pro_density_find_free_slot (Create-Slot-Suche, ersetzt 3x)
 *   - pro_lindblad_2x2 (2x2-Lindblad-Kern, ersetzt 2x)
 *   - pro_td_re / pro_td_im / pro_td_set (64x64-Q31-Zugriff)
 *   - pro_fd_re / pro_fd_im / pro_fd_set (256x256-Q31-Zugriff)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/Density.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer
  * ========================================================================== */

  /* Sucht den ersten freien Slot in einem Array, dessen erstes Element
   * ein uint32_t "active"-Flag ist.
   *
   * arr          : Zeiger auf das Array (z.B. pu->density_matrices).
   * stride_bytes : sizeof(struct) des Array-Elements.
   * cap          : Anzahl Elemente im Array.
   *
   * Rueckgabe: Slot-Index in [0, cap) oder -1 wenn kein Slot frei.
   *
   * Nutzt memcpy fuer den Lesezugriff, um strikte Aliasing-Verletzungen
   * zu vermeiden. Der Compiler optimiert memcpy zu einem einfachen Load.
   * R2-konform: kein malloc, laeuft nur beim Create. */
static int32_t pro_density_find_free_slot(
    const void* arr, size_t stride_bytes, uint32_t cap)
{
    if (!arr || cap == 0u) return -1;
    const uint8_t* base = (const uint8_t*)arr;
    for (uint32_t i = 0; i < cap; ++i) {
        uint32_t active = 0u;
        memcpy(&active, base + (size_t)i * stride_bytes, sizeof(active));
        if (active == 0u) return (int32_t)i;
    }
    return -1;
}

/* 2x2-Lindblad-Kern auf einem 2-Level-Block.
 *
 * decay_from, decay_to : Indizes 0 oder 1.
 *   Bei PRO_LINDBLAD_AMP_DAMP zerfaellt die Population von decay_from
 *   nach decay_to. Die Off-Diagonalen werden mit 0.5*gamma gedaempft.
 *
 * Konventionen der Aufrufer:
 *   TensorDensity: Konvention 0 = iu (angeregt), 1 = id (Grund).
 *                  AMP_DAMP: decay_from = 0, decay_to = 1.
 *   FockDensity:   Konvention 0 = Mode leer, 1 = Mode besetzt.
 *                  AMP_DAMP: decay_from = 1, decay_to = 0.
 *
 * PHASE_DAMP und DEPOLARIZE sind symmetrisch in den beiden Niveaus
 * und ignorieren decay_from / decay_to. */
static void pro_lindblad_2x2(
    const double M_re[2][2], const double M_im[2][2],
    double dM_re[2][2], double dM_im[2][2],
    uint32_t kind, double gamma,
    int decay_from, int decay_to)
{
    dM_re[0][0] = dM_im[0][0] = 0.0;
    dM_re[0][1] = dM_im[0][1] = 0.0;
    dM_re[1][0] = dM_im[1][0] = 0.0;
    dM_re[1][1] = dM_im[1][1] = 0.0;

    if (kind == PRO_LINDBLAD_AMP_DAMP) {
        dM_re[decay_to][decay_to] = +gamma * M_re[decay_from][decay_from];
        dM_im[decay_to][decay_to] = +gamma * M_im[decay_from][decay_from];
        dM_re[decay_from][decay_from] = -gamma * M_re[decay_from][decay_from];
        dM_im[decay_from][decay_from] = -gamma * M_im[decay_from][decay_from];
        dM_re[0][1] = -0.5 * gamma * M_re[0][1];
        dM_im[0][1] = -0.5 * gamma * M_im[0][1];
        dM_re[1][0] = -0.5 * gamma * M_re[1][0];
        dM_im[1][0] = -0.5 * gamma * M_im[1][0];
    }
    else if (kind == PRO_LINDBLAD_PHASE_DAMP) {
        dM_re[0][1] = -gamma * M_re[0][1];
        dM_im[0][1] = -gamma * M_im[0][1];
        dM_re[1][0] = -gamma * M_re[1][0];
        dM_im[1][0] = -gamma * M_im[1][0];
    }
    else if (kind == PRO_LINDBLAD_DEPOLARIZE) {
        const double tr = M_re[0][0] + M_re[1][1];
        dM_re[0][0] = (2.0 * gamma / 3.0) * tr - (4.0 * gamma / 3.0) * M_re[0][0];
        dM_im[0][0] = -(4.0 * gamma / 3.0) * M_im[0][0];
        dM_re[1][1] = (2.0 * gamma / 3.0) * tr - (4.0 * gamma / 3.0) * M_re[1][1];
        dM_im[1][1] = -(4.0 * gamma / 3.0) * M_im[1][1];
        dM_re[0][1] = -(4.0 * gamma / 3.0) * M_re[0][1];
        dM_im[0][1] = -(4.0 * gamma / 3.0) * M_im[0][1];
        dM_re[1][0] = -(4.0 * gamma / 3.0) * M_re[1][0];
        dM_im[1][0] = -(4.0 * gamma / 3.0) * M_im[1][0];
    }
}

/* --------------------------------------------------------------------------
 * Q31-Zugriff auf 64x64-Tensor-Dichte (Stride 64)
 * -------------------------------------------------------------------------- */

static inline double pro_td_re(const ProTensorDensity* td, uint32_t i, uint32_t j)
{
    return (double)pro_amp_real(td->rho[i * 64u + j]) / 2147483647.0;
}
static inline double pro_td_im(const ProTensorDensity* td, uint32_t i, uint32_t j)
{
    return (double)pro_amp_imag(td->rho[i * 64u + j]) / 2147483647.0;
}
static inline void pro_td_set(ProTensorDensity* td, uint32_t i, uint32_t j,
    double re, double im)
{
    if (re > 1.0) re = 1.0;
    if (re < -1.0) re = -1.0;
    if (im > 1.0) im = 1.0;
    if (im < -1.0) im = -1.0;
    td->rho[i * 64u + j] = pro_amp_pack(
        pro_sat_i32((int64_t)llround(re * 2147483647.0)),
        pro_sat_i32((int64_t)llround(im * 2147483647.0)));
}

/* --------------------------------------------------------------------------
 * Q31-Zugriff auf 256x256-Fock-Dichte (Stride 256)
 * -------------------------------------------------------------------------- */

static inline double pro_fd_re(const ProFockDensity* fd, uint32_t i, uint32_t j)
{
    return (double)pro_amp_real(fd->rho[i * 256u + j]) / 2147483647.0;
}
static inline double pro_fd_im(const ProFockDensity* fd, uint32_t i, uint32_t j)
{
    return (double)pro_amp_imag(fd->rho[i * 256u + j]) / 2147483647.0;
}
static inline void pro_fd_set(ProFockDensity* fd, uint32_t i, uint32_t j,
    double re, double im)
{
    if (re > 1.0) re = 1.0;
    if (re < -1.0) re = -1.0;
    if (im > 1.0) im = 1.0;
    if (im < -1.0) im = -1.0;
    fd->rho[i * 256u + j] = pro_amp_pack(
        pro_sat_i32((int64_t)llround(re * 2147483647.0)),
        pro_sat_i32((int64_t)llround(im * 2147483647.0)));
}

/* --------------------------------------------------------------------------
 * Datei-lokale Puffer fuer TensorDensity-Lindblad.
 *
 * Single-threaded. 64x64x8 B = 32 KB je Puffer, 4 Puffer = 128 KB BSS.
 * Vermeidet malloc im Lindblad-Pfad fuer die 64x64-Ebene.
 * -------------------------------------------------------------------------- */

static double g_td_r_re[64][64], g_td_r_im[64][64];
static double g_td_d_re[64][64], g_td_d_im[64][64];

/* ==========================================================================
 * 8x8-Knoten-Dichte
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Density_Create(
    ProUniverse* pu, uint32_t* out_rho_id)
{
    if (!pu || !pu->density_matrices) return false;
    if (pu->density_count >= pu->density_capacity) return false;

    const int32_t slot = pro_density_find_free_slot(
        pu->density_matrices,
        sizeof(ProDensityMatrix),
        (uint32_t)pu->density_capacity);
    if (slot < 0) return false;

    ProDensityMatrix* r = &pu->density_matrices[slot];
    r->active = 1u;
    r->dim = PRO_DENSITY_DIM;
    r->_pad = 0u;
    for (uint32_t k = 0; k < PRO_DENSITY_DIM * PRO_DENSITY_DIM; ++k) {
        r->rho[k] = 0;
    }
    r->rho[0] = pro_amp_pack(INT32_MAX, 0);

    pu->density_count++;
    if (out_rho_id) *out_rho_id = (uint32_t)slot;
    return true;
}

PROPHYSICS_API bool ProPhysics_Density_Destroy(
    ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->density_matrices) return false;
    if (rho_id >= pu->density_capacity) return false;
    if (pu->density_matrices[rho_id].active == 0u) return false;
    pu->density_matrices[rho_id].active = 0u;
    if (pu->density_count > 0u) pu->density_count--;
    return true;
}

PROPHYSICS_API uint64_t ProPhysics_Density_Count(const ProUniverse* pu)
{
    return pu ? pu->density_count : 0u;
}

PROPHYSICS_API bool ProPhysics_Density_From_Pure(
    ProUniverse* pu, uint32_t rho_id, const ProAmpVector* psi)
{
    if (!pu || !pu->density_matrices || !psi) return false;
    if (rho_id >= pu->density_capacity) return false;
    ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return false;

    ProU128 n2 = PRO_U128_ZERO;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        n2 = pro_u128_add(n2, pro_u128_from_u64(pro_amp_abs2(psi->coeff[b])));
    }
    if (pro_u128_is_zero(n2)) return false;

    const double inv_n2 = 2147483648.0 / pro_u128_to_double(n2);

    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        const int64_t ir = pro_amp_real(psi->coeff[i]);
        const int64_t ii = pro_amp_imag(psi->coeff[i]);
        for (uint8_t j = 0; j < PRO_AMP_BASIS_SIZE; ++j) {
            const int64_t jr = pro_amp_real(psi->coeff[j]);
            const int64_t ji = pro_amp_imag(psi->coeff[j]);
            const double dre = ((double)ir * (double)jr
                + (double)ii * (double)ji) * inv_n2;
            const double dim = ((double)ii * (double)jr
                - (double)ir * (double)ji) * inv_n2;
            r->rho[i * 8 + j] = pro_amp_pack(
                pro_density_unit_to_q31(dre / 2147483647.0),
                pro_density_unit_to_q31(dim / 2147483647.0));
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Density_From_Mixture(
    ProUniverse* pu, uint32_t rho_id,
    const ProAmpVector* states, const double* weights, uint32_t n)
{
    if (!pu || !pu->density_matrices) return false;
    if (rho_id >= pu->density_capacity) return false;
    if (!states || !weights || n == 0u) return false;

    double acc_re[8][8] = { {0} }, acc_im[8][8] = { {0} };

    double wsum = 0.0;
    for (uint32_t k = 0; k < n; ++k) wsum += weights[k];
    if (wsum <= 0.0) return false;

    for (uint32_t k = 0; k < n; ++k) {
        const ProAmpVector* psi = &states[k];
        const double w = weights[k] / wsum;

        double n2 = 0.0;
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            const double re = (double)pro_amp_real(psi->coeff[b]);
            const double im = (double)pro_amp_imag(psi->coeff[b]);
            n2 += re * re + im * im;
        }
        if (n2 < 1.0) continue;
        const double inv_n2 = 1.0 / n2;

        for (uint8_t i = 0; i < 8; ++i) {
            const double ir = (double)pro_amp_real(psi->coeff[i]);
            const double ii = (double)pro_amp_imag(psi->coeff[i]);
            for (uint8_t j = 0; j < 8; ++j) {
                const double jr = (double)pro_amp_real(psi->coeff[j]);
                const double ji = (double)pro_amp_imag(psi->coeff[j]);
                acc_re[i][j] += w * (ir * jr + ii * ji) * inv_n2;
                acc_im[i][j] += w * (ii * jr - ir * ji) * inv_n2;
            }
        }
    }

    ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return false;
    for (uint8_t i = 0; i < 8; ++i) {
        for (uint8_t j = 0; j < 8; ++j) {
            const int32_t re = pro_density_unit_to_q31(acc_re[i][j]);
            const int32_t im = pro_density_unit_to_q31(acc_im[i][j]);
            r->rho[i * 8 + j] = pro_amp_pack(re, im);
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Density_Get_Element(
    const ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t* out_re, int32_t* out_im)
{
    if (!pu || !pu->density_matrices) return false;
    if (rho_id >= pu->density_capacity) return false;
    const ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return false;
    if (i >= 8u || j >= 8u) return false;
    if (out_re) *out_re = pro_amp_real(r->rho[i * 8 + j]);
    if (out_im) *out_im = pro_amp_imag(r->rho[i * 8 + j]);
    return true;
}

PROPHYSICS_API bool ProPhysics_Density_Set_Element(
    ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t re, int32_t im)
{
    if (!pu || !pu->density_matrices) return false;
    if (rho_id >= pu->density_capacity) return false;
    ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return false;
    if (i >= 8u || j >= 8u) return false;
    r->rho[i * 8 + j] = pro_amp_pack(re, im);
    return true;
}

PROPHYSICS_API double ProPhysics_Density_Trace(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->density_matrices) return 0.0;
    if (rho_id >= pu->density_capacity) return 0.0;
    const ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return 0.0;

    double tr = 0.0;
    for (int i = 0; i < 8; ++i) {
        tr += pro_density_q31_to_unit(pro_amp_real(r->rho[i * 8 + i]));
    }
    return tr;
}

PROPHYSICS_API double ProPhysics_Density_Purity(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->density_matrices) return 0.0;
    if (rho_id >= pu->density_capacity) return 0.0;
    const ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return 0.0;

    uint64_t sum = 0;
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            sum += pro_amp_abs2(r->rho[a * 8 + b]);
        }
    }
    return (double)sum / 4611686018427387904.0;
}

PROPHYSICS_API double ProPhysics_Density_Offdiag_Magnitude(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->density_matrices) return 0.0;
    if (rho_id >= pu->density_capacity) return 0.0;
    const ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return 0.0;

    uint64_t sum = 0;
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            if (a == b) continue;
            sum += pro_amp_abs2(r->rho[a * 8 + b]);
        }
    }
    return (double)sum / 4611686018427387904.0;
}

PROPHYSICS_API double ProPhysics_Density_Von_Neumann_Entropy(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->density_matrices) return 0.0;
    if (rho_id >= pu->density_capacity) return 0.0;
    const ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return 0.0;

    /* Real-Embedding 8x8 komplex-hermitesch -> 16x16 reell-symmetrisch. */
    double M[16][16];
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            const double re = pro_density_q31_to_unit(
                pro_amp_real(r->rho[a * 8 + b]));
            const double im = pro_density_q31_to_unit(
                pro_amp_imag(r->rho[a * 8 + b]));
            M[2 * a][2 * b] = re;
            M[2 * a][2 * b + 1] = -im;
            M[2 * a + 1][2 * b] = im;
            M[2 * a + 1][2 * b + 1] = re;
        }
    }

    pro_jacobi_sym_eigen(M, NULL, 16);

    double total = 0.0;
    for (int k = 0; k < 16; ++k) total += M[k][k];
    if (total < 1e-12) return 0.0;

    double S = 0.0;
    for (int k = 0; k < 16; ++k) {
        const double mu = M[k][k];
        if (mu < 1e-12) continue;
        const double p = mu / total;
        S -= p * log(2.0 * p);
    }
    return S;
}

PROPHYSICS_API bool ProPhysics_Density_Apply_Unitary(
    ProUniverse* pu, uint32_t rho_id,
    const ProAmpQ31 U[PRO_DENSITY_DIM][PRO_DENSITY_DIM])
{
    if (!pu || !pu->density_matrices || !U) return false;
    if (rho_id >= pu->density_capacity) return false;
    ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return false;

    /* tmp = U * rho. */
    ProAmpQ31 tmp[8][8];
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            int64_t sr = 0, si = 0;
            for (int k = 0; k < 8; ++k) {
                const int64_t ur = pro_amp_real(U[a][k]);
                const int64_t ui = pro_amp_imag(U[a][k]);
                const int64_t pr = pro_amp_real(r->rho[k * 8 + b]);
                const int64_t pi = pro_amp_imag(r->rho[k * 8 + b]);
                sr += pro_round_shift_q31(ur * pr - ui * pi);
                si += pro_round_shift_q31(ur * pi + ui * pr);
            }
            tmp[a][b] = pro_amp_pack(pro_sat_i32(sr), pro_sat_i32(si));
        }
    }
    /* rho = tmp * U-dagger. */
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            int64_t sr = 0, si = 0;
            for (int k = 0; k < 8; ++k) {
                const int64_t tr_ = pro_amp_real(tmp[a][k]);
                const int64_t ti_ = pro_amp_imag(tmp[a][k]);
                const int64_t ur = pro_amp_real(U[b][k]);
                const int64_t ui = pro_amp_imag(U[b][k]);
                sr += pro_round_shift_q31(tr_ * ur + ti_ * ui);
                si += pro_round_shift_q31(ti_ * ur - tr_ * ui);
            }
            r->rho[a * 8 + b] = pro_amp_pack(pro_sat_i32(sr), pro_sat_i32(si));
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Density_Apply_Lindblad_Step(
    ProUniverse* pu, uint32_t rho_id,
    uint32_t lindblad_kind, double strength)
{
    if (!pu || !pu->density_matrices) return false;
    if (rho_id >= pu->density_capacity) return false;
    ProDensityMatrix* r = &pu->density_matrices[rho_id];
    if (r->active == 0u) return false;
    if (lindblad_kind == PRO_LINDBLAD_NONE) return true;
    if (strength <= 0.0) return true;

    const int iu = UR_POSITRON_CW;
    const int id = UR_NEGATRON_CCW;

    double rr[8][8], ri[8][8];
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            rr[a][b] = pro_density_q31_to_unit(pro_amp_real(r->rho[a * 8 + b]));
            ri[a][b] = pro_density_q31_to_unit(pro_amp_imag(r->rho[a * 8 + b]));
        }
    }

    double dr[8][8] = { {0} }, di[8][8] = { {0} };

    if (lindblad_kind == PRO_LINDBLAD_AMP_DAMP) {
        const double g = strength;
        const double r_ii_re = rr[iu][iu];
        const double r_ii_im = ri[iu][iu];

        dr[id][id] += g * r_ii_re;
        di[id][id] += g * r_ii_im;

        for (int b = 0; b < 8; ++b) {
            dr[iu][b] -= 0.5 * g * rr[iu][b];
            di[iu][b] -= 0.5 * g * ri[iu][b];
        }
        for (int a = 0; a < 8; ++a) {
            dr[a][iu] -= 0.5 * g * rr[a][iu];
            di[a][iu] -= 0.5 * g * ri[a][iu];
        }
    }
    else if (lindblad_kind == PRO_LINDBLAD_PHASE_DAMP) {
        const double g = strength;
        int ssign[8] = { 0 };
        ssign[iu] = +1;
        ssign[id] = -1;
        for (int a = 0; a < 8; ++a) {
            if (a != iu && a != id) continue;
            for (int b = 0; b < 8; ++b) {
                if (b != iu && b != id) continue;
                const double factor = 0.5 * g * (ssign[a] * ssign[b] - 1);
                dr[a][b] += factor * rr[a][b];
                di[a][b] += factor * ri[a][b];
            }
        }
    }
    else if (lindblad_kind == PRO_LINDBLAD_DEPOLARIZE) {
        const double g = strength;
        const int s_i = iu, s_d = id;
        const double tr2 = rr[s_i][s_i] + rr[s_d][s_d];
        const double cI = 2.0 * g / 3.0 * tr2;
        const double cR = 4.0 * g / 3.0;

        for (int a = 0; a < 8; ++a) {
            if (a != s_i && a != s_d) continue;
            for (int b = 0; b < 8; ++b) {
                if (b != s_i && b != s_d) continue;
                const double boost = (a == b) ? cI : 0.0;
                dr[a][b] += boost - cR * rr[a][b];
                di[a][b] -= cR * ri[a][b];
            }
        }
    }
    else return false;

    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            const double nre = rr[a][b] + dr[a][b];
            const double nim = ri[a][b] + di[a][b];
            r->rho[a * 8 + b] = pro_amp_pack(
                pro_density_unit_to_q31(nre),
                pro_density_unit_to_q31(nim));
        }
    }
    return true;
}

/* ==========================================================================
 * 64x64-Tensor-Dichte
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_TensorDensity_Create(
    ProUniverse* pu, uint32_t pair_id, uint32_t* out_rho_id)
{
    if (!pu || !pu->tensor_densities) return false;
    if (pu->tensor_density_count >= pu->tensor_density_capacity) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;

    const int32_t slot = pro_density_find_free_slot(
        pu->tensor_densities,
        sizeof(ProTensorDensity),
        (uint32_t)pu->tensor_density_capacity);
    if (slot < 0) return false;

    ProTensorDensity* td = &pu->tensor_densities[slot];
    td->active = 1u;
    td->pair_id = pair_id;
    td->_pad = 0u;
    for (uint32_t k = 0; k < PRO_TENSOR_DENSITY_DIM * PRO_TENSOR_DENSITY_DIM; ++k) {
        td->rho[k] = 0;
    }
    td->rho[0] = pro_amp_pack(INT32_MAX, 0);

    pu->tensor_density_count++;
    if (out_rho_id) *out_rho_id = (uint32_t)slot;
    return true;
}

PROPHYSICS_API bool ProPhysics_TensorDensity_Destroy(
    ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->tensor_densities) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    if (pu->tensor_densities[rho_id].active == 0u) return false;
    pu->tensor_densities[rho_id].active = 0u;
    if (pu->tensor_density_count > 0u) pu->tensor_density_count--;
    return true;
}

PROPHYSICS_API uint64_t ProPhysics_TensorDensity_Count(const ProUniverse* pu)
{
    return pu ? pu->tensor_density_count : 0u;
}

PROPHYSICS_API bool ProPhysics_TensorDensity_From_Pair(
    ProUniverse* pu, uint32_t rho_id, uint32_t pair_id)
{
    if (!pu || !pu->tensor_densities || !pu->tensor_pairs) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProTensorDensity* td = &pu->tensor_densities[rho_id];
    if (td->active == 0u) return false;
    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;

    for (uint32_t i = 0; i < PRO_TENSOR_DENSITY_DIM; ++i) {
        const double ir = (double)pro_amp_real(p->coeff[i]);
        const double ii = (double)pro_amp_imag(p->coeff[i]);
        for (uint32_t j = 0; j < PRO_TENSOR_DENSITY_DIM; ++j) {
            const double jr = (double)pro_amp_real(p->coeff[j]);
            const double ji = (double)pro_amp_imag(p->coeff[j]);
            const double re = (ir * jr + ii * ji) / 4611686018427387904.0;
            const double im = (ii * jr - ir * ji) / 4611686018427387904.0;
            pro_td_set(td, i, j, re, im);
        }
    }
    td->pair_id = pair_id;
    return true;
}

PROPHYSICS_API bool ProPhysics_TensorDensity_Get_Element(
    const ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t* out_re, int32_t* out_im)
{
    if (!pu || !pu->tensor_densities) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    if (i >= PRO_TENSOR_DENSITY_DIM || j >= PRO_TENSOR_DENSITY_DIM) return false;
    const ProTensorDensity* td = &pu->tensor_densities[rho_id];
    if (td->active == 0u) return false;
    if (out_re) *out_re = pro_amp_real(td->rho[i * 64u + j]);
    if (out_im) *out_im = pro_amp_imag(td->rho[i * 64u + j]);
    return true;
}

PROPHYSICS_API double ProPhysics_TensorDensity_Trace(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->tensor_densities) return 0.0;
    if (rho_id >= pu->tensor_density_capacity) return 0.0;
    const ProTensorDensity* td = &pu->tensor_densities[rho_id];
    if (td->active == 0u) return 0.0;

    double tr = 0.0;
    for (uint32_t i = 0; i < PRO_TENSOR_DENSITY_DIM; ++i) {
        tr += pro_td_re(td, i, i);
    }
    return tr;
}

PROPHYSICS_API double ProPhysics_TensorDensity_Purity(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->tensor_densities) return 0.0;
    if (rho_id >= pu->tensor_density_capacity) return 0.0;
    const ProTensorDensity* td = &pu->tensor_densities[rho_id];
    if (td->active == 0u) return 0.0;

    double sum = 0.0;
    for (uint32_t i = 0; i < PRO_TENSOR_DENSITY_DIM; ++i) {
        for (uint32_t j = 0; j < PRO_TENSOR_DENSITY_DIM; ++j) {
            const double re = pro_td_re(td, i, j);
            const double im = pro_td_im(td, i, j);
            sum += re * re + im * im;
        }
    }
    return sum;
}

PROPHYSICS_API bool ProPhysics_TensorDensity_Partial_Trace_A(
    const ProUniverse* pu, uint32_t rho_id, uint32_t out_rho_id)
{
    if (!pu || !pu->tensor_densities || !pu->density_matrices) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    if (out_rho_id >= pu->density_capacity) return false;
    const ProTensorDensity* td = &pu->tensor_densities[rho_id];
    ProDensityMatrix* out = &pu->density_matrices[out_rho_id];
    if (td->active == 0u || out->active == 0u) return false;

    for (uint32_t a = 0; a < 8; ++a) {
        for (uint32_t ap = 0; ap < 8; ++ap) {
            double sr = 0.0, si = 0.0;
            for (uint32_t b = 0; b < 8; ++b) {
                const uint32_t i = a * 8u + b;
                const uint32_t j = ap * 8u + b;
                sr += pro_td_re(td, i, j);
                si += pro_td_im(td, i, j);
            }
            out->rho[a * 8u + ap] = pro_amp_pack(
                pro_sat_i32((int64_t)llround(sr * 2147483647.0)),
                pro_sat_i32((int64_t)llround(si * 2147483647.0)));
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_TensorDensity_Partial_Trace_B(
    const ProUniverse* pu, uint32_t rho_id, uint32_t out_rho_id)
{
    if (!pu || !pu->tensor_densities || !pu->density_matrices) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    if (out_rho_id >= pu->density_capacity) return false;
    const ProTensorDensity* td = &pu->tensor_densities[rho_id];
    ProDensityMatrix* out = &pu->density_matrices[out_rho_id];
    if (td->active == 0u || out->active == 0u) return false;

    for (uint32_t b = 0; b < 8; ++b) {
        for (uint32_t bp = 0; bp < 8; ++bp) {
            double sr = 0.0, si = 0.0;
            for (uint32_t a = 0; a < 8; ++a) {
                const uint32_t i = a * 8u + b;
                const uint32_t j = a * 8u + bp;
                sr += pro_td_re(td, i, j);
                si += pro_td_im(td, i, j);
            }
            out->rho[b * 8u + bp] = pro_amp_pack(
                pro_sat_i32((int64_t)llround(sr * 2147483647.0)),
                pro_sat_i32((int64_t)llround(si * 2147483647.0)));
        }
    }
    return true;
}

/* Apply_Local_Lindblad: liest rho in Datei-Puffer, akkumuliert Delta,
 * wendet es nach vollstaendiger Block-Iteration an. Aliasing strukturell
 * ausgeschlossen durch die getrennten Puffer g_td_r_* und g_td_d_*.
 *
 * Konvention der 2-Level-Bloecke:
 *   Index 0 = iu (UR_POSITRON_CW, angeregt)
 *   Index 1 = id (UR_NEGATRON_CCW, Grund)
 *   AMP_DAMP zerfaellt von 0 nach 1. */
PROPHYSICS_API bool ProPhysics_TensorDensity_Apply_Local_Lindblad(
    ProUniverse* pu, uint32_t rho_id, int side,
    uint32_t lindblad_kind, double strength)
{
    if (!pu || !pu->tensor_densities) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    if (side != 0 && side != 1) return false;
    ProTensorDensity* td = &pu->tensor_densities[rho_id];
    if (td->active == 0u) return false;
    if (strength <= 0.0) return true;
    if (lindblad_kind == PRO_LINDBLAD_NONE) return true;

    const uint32_t iu = UR_POSITRON_CW;
    const uint32_t id = UR_NEGATRON_CCW;

    /* 1. Vollkopie + Delta-Puffer nullen. */
    for (uint32_t i = 0; i < 64u; ++i) {
        for (uint32_t j = 0; j < 64u; ++j) {
            g_td_r_re[i][j] = pro_td_re(td, i, j);
            g_td_r_im[i][j] = pro_td_im(td, i, j);
            g_td_d_re[i][j] = 0.0;
            g_td_d_im[i][j] = 0.0;
        }
    }

    /* 2. Kanaele blockweise. */
    if (side == 0) {
        for (uint32_t b = 0; b < 8u; ++b) {
            for (uint32_t h = 0; h < 8u; ++h) {
                const uint32_t i00 = iu * 8u + b;
                const uint32_t i01 = iu * 8u + h;
                const uint32_t i10 = id * 8u + b;
                const uint32_t i11 = id * 8u + h;

                double M_re[2][2], M_im[2][2], dM_re[2][2], dM_im[2][2];

                M_re[0][0] = g_td_r_re[i00][i01];
                M_im[0][0] = g_td_r_im[i00][i01];
                M_re[0][1] = g_td_r_re[i00][i11];
                M_im[0][1] = g_td_r_im[i00][i11];
                M_re[1][0] = g_td_r_re[i10][i01];
                M_im[1][0] = g_td_r_im[i10][i01];
                M_re[1][1] = g_td_r_re[i10][i11];
                M_im[1][1] = g_td_r_im[i10][i11];

                pro_lindblad_2x2(M_re, M_im, dM_re, dM_im,
                    lindblad_kind, strength, 0, 1);

                g_td_d_re[i00][i01] = dM_re[0][0];
                g_td_d_im[i00][i01] = dM_im[0][0];
                g_td_d_re[i00][i11] = dM_re[0][1];
                g_td_d_im[i00][i11] = dM_im[0][1];
                g_td_d_re[i10][i01] = dM_re[1][0];
                g_td_d_im[i10][i01] = dM_im[1][0];
                g_td_d_re[i10][i11] = dM_re[1][1];
                g_td_d_im[i10][i11] = dM_im[1][1];
            }
        }
    }
    else {
        for (uint32_t a = 0; a < 8u; ++a) {
            for (uint32_t g = 0; g < 8u; ++g) {
                const uint32_t i00 = a * 8u + iu;
                const uint32_t i01 = a * 8u + id;
                const uint32_t i10 = g * 8u + iu;
                const uint32_t i11 = g * 8u + id;

                double M_re[2][2], M_im[2][2], dM_re[2][2], dM_im[2][2];

                M_re[0][0] = g_td_r_re[i00][i10];
                M_im[0][0] = g_td_r_im[i00][i10];
                M_re[0][1] = g_td_r_re[i00][i11];
                M_im[0][1] = g_td_r_im[i00][i11];
                M_re[1][0] = g_td_r_re[i01][i10];
                M_im[1][0] = g_td_r_im[i01][i10];
                M_re[1][1] = g_td_r_re[i01][i11];
                M_im[1][1] = g_td_r_im[i01][i11];

                pro_lindblad_2x2(M_re, M_im, dM_re, dM_im,
                    lindblad_kind, strength, 0, 1);

                g_td_d_re[i00][i10] = dM_re[0][0];
                g_td_d_im[i00][i10] = dM_im[0][0];
                g_td_d_re[i00][i11] = dM_re[0][1];
                g_td_d_im[i00][i11] = dM_im[0][1];
                g_td_d_re[i01][i10] = dM_re[1][0];
                g_td_d_im[i01][i10] = dM_im[1][0];
                g_td_d_re[i01][i11] = dM_re[1][1];
                g_td_d_im[i01][i11] = dM_im[1][1];
            }
        }
    }

    /* 3. Alles anwenden. */
    for (uint32_t i = 0; i < 64u; ++i) {
        for (uint32_t j = 0; j < 64u; ++j) {
            pro_td_set(td, i, j,
                g_td_r_re[i][j] + g_td_d_re[i][j],
                g_td_r_im[i][j] + g_td_d_im[i][j]);
        }
    }
    return true;
}

/* ==========================================================================
 * 256x256-Fock-Dichte
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_FockDensity_Create(
    ProUniverse* pu, uint64_t fock_id, uint32_t* out_rho_id)
{
    if (!pu || !pu->fock_densities) return false;
    if (pu->fock_density_count >= pu->fock_density_capacity) return false;

    const int32_t slot = pro_density_find_free_slot(
        pu->fock_densities,
        sizeof(ProFockDensity),
        (uint32_t)pu->fock_density_capacity);
    if (slot < 0) return false;

    ProFockDensity* fd = &pu->fock_densities[slot];
    fd->active = 1u;
    fd->_pad = 0u;
    fd->fock_id = fock_id;
    for (uint32_t k = 0; k < PRO_FOCK_DENSITY_DIM * PRO_FOCK_DENSITY_DIM; ++k) {
        fd->rho[k] = 0;
    }
    fd->rho[0] = pro_amp_pack(INT32_MAX, 0);

    pu->fock_density_count++;
    if (out_rho_id) *out_rho_id = (uint32_t)slot;
    return true;
}

PROPHYSICS_API bool ProPhysics_FockDensity_Destroy(
    ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->fock_densities) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    if (pu->fock_densities[rho_id].active == 0u) return false;
    pu->fock_densities[rho_id].active = 0u;
    if (pu->fock_density_count > 0u) pu->fock_density_count--;
    return true;
}

PROPHYSICS_API uint64_t ProPhysics_FockDensity_Count(const ProUniverse* pu)
{
    return pu ? pu->fock_density_count : 0u;
}

PROPHYSICS_API bool ProPhysics_FockDensity_From_Fock(
    ProUniverse* pu, uint32_t rho_id, uint64_t fock_id)
{
    if (!pu || !pu->fock_densities || !pu->fock_states) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return false;
    const ProFockState* fs = &pu->fock_states[fock_id];
    if (fs->active == 0u) return false;

    for (uint32_t i = 0; i < 256u; ++i) {
        const double ir = (double)pro_amp_real(fs->coeff[i]);
        const double ii = (double)pro_amp_imag(fs->coeff[i]);
        for (uint32_t j = 0; j < 256u; ++j) {
            const double jr = (double)pro_amp_real(fs->coeff[j]);
            const double ji = (double)pro_amp_imag(fs->coeff[j]);
            const double re = (ir * jr + ii * ji) / 4611686018427387904.0;
            const double im = (ii * jr - ir * ji) / 4611686018427387904.0;
            pro_fd_set(fd, i, j, re, im);
        }
    }
    fd->fock_id = fock_id;
    return true;
}

PROPHYSICS_API bool ProPhysics_FockDensity_Get_Element(
    const ProUniverse* pu, uint32_t rho_id,
    uint32_t i, uint32_t j, int32_t* out_re, int32_t* out_im)
{
    if (!pu || !pu->fock_densities) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    if (i >= 256u || j >= 256u) return false;
    const ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return false;
    if (out_re) *out_re = pro_amp_real(fd->rho[i * 256u + j]);
    if (out_im) *out_im = pro_amp_imag(fd->rho[i * 256u + j]);
    return true;
}

PROPHYSICS_API bool ProPhysics_FockDensity_Set_Element(
    ProUniverse* pu, uint32_t rho_id,
    uint32_t i, uint32_t j, int32_t re, int32_t im)
{
    if (!pu || !pu->fock_densities) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    if (i >= 256u || j >= 256u) return false;
    ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return false;
    fd->rho[i * 256u + j] = pro_amp_pack(re, im);
    return true;
}

PROPHYSICS_API double ProPhysics_FockDensity_Trace(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->fock_densities) return 0.0;
    if (rho_id >= pu->fock_density_capacity) return 0.0;
    const ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return 0.0;

    double tr = 0.0;
    for (uint32_t i = 0; i < 256u; ++i) tr += pro_fd_re(fd, i, i);
    return tr;
}

PROPHYSICS_API double ProPhysics_FockDensity_Purity(
    const ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->fock_densities) return 0.0;
    if (rho_id >= pu->fock_density_capacity) return 0.0;
    const ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return 0.0;

    double sum = 0.0;
    for (uint32_t i = 0; i < 256u; ++i) {
        for (uint32_t j = 0; j < 256u; ++j) {
            const double re = pro_fd_re(fd, i, j);
            const double im = pro_fd_im(fd, i, j);
            sum += re * re + im * im;
        }
    }
    return sum;
}

PROPHYSICS_API double ProPhysics_FockDensity_Mode_Occupation(
    const ProUniverse* pu, uint32_t rho_id, uint32_t mode)
{
    if (!pu || !pu->fock_densities) return 0.0;
    if (rho_id >= pu->fock_density_capacity) return 0.0;
    if (mode >= 8u) return 0.0;
    const ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return 0.0;

    const uint32_t mask = 1u << mode;
    double sum = 0.0;
    for (uint32_t s = 0; s < 256u; ++s) {
        if ((s & mask) == 0u) continue;
        sum += pro_fd_re(fd, s, s);
    }
    return sum;
}

/* Mode-Lindblad auf der 256x256-Fock-Dichte.
 *
 * Pro Modus wird jeder 2x2-Block {Mode leer, Mode besetzt} mit dem
 * Lindblad-Kern behandelt. KONVENTION:
 *   Index 0 = Mode leer  (Grundzustand)
 *   Index 1 = Mode besetzt (angeregter Zustand)
 *   AMP_DAMP zerfaellt von 1 nach 0.
 *
 * Arbeitet mit malloc von 4 Puffern (2 MB). Kein Hotpath. */
PROPHYSICS_API bool ProPhysics_FockDensity_Apply_Mode_Lindblad(
    ProUniverse* pu, uint32_t rho_id, uint32_t mode,
    uint32_t lindblad_kind, double strength)
{
    if (!pu || !pu->fock_densities) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    if (mode >= 8u) return false;
    ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return false;
    if (strength <= 0.0) return true;
    if (lindblad_kind == PRO_LINDBLAD_NONE) return true;

    /* 4 Puffer x 65536 x 8 B = 2 MB pro Aufruf. */
    double* r_re = (double*)malloc(256u * 256u * sizeof(double));
    double* r_im = (double*)malloc(256u * 256u * sizeof(double));
    double* d_re = (double*)calloc(256u * 256u, sizeof(double));
    double* d_im = (double*)calloc(256u * 256u, sizeof(double));
    if (!r_re || !r_im || !d_re || !d_im) {
        free(r_re); free(r_im); free(d_re); free(d_im);
        return false;
    }

    for (uint32_t i = 0; i < 256u; ++i) {
        for (uint32_t j = 0; j < 256u; ++j) {
            r_re[i * 256u + j] = pro_fd_re(fd, i, j);
            r_im[i * 256u + j] = pro_fd_im(fd, i, j);
        }
    }

    const uint32_t mask = 1u << mode;
    for (uint32_t s = 0; s < 256u; ++s) {
        if (s & mask) continue;
        const uint32_t s0 = s;
        const uint32_t s1 = s | mask;

        for (uint32_t t = 0; t < 256u; ++t) {
            if (t & mask) continue;
            const uint32_t t0 = t;
            const uint32_t t1 = t | mask;

            double M_re[2][2], M_im[2][2], dM_re[2][2], dM_im[2][2];

            M_re[0][0] = r_re[s0 * 256u + t0];
            M_im[0][0] = r_im[s0 * 256u + t0];
            M_re[0][1] = r_re[s0 * 256u + t1];
            M_im[0][1] = r_im[s0 * 256u + t1];
            M_re[1][0] = r_re[s1 * 256u + t0];
            M_im[1][0] = r_im[s1 * 256u + t0];
            M_re[1][1] = r_re[s1 * 256u + t1];
            M_im[1][1] = r_im[s1 * 256u + t1];

            pro_lindblad_2x2(M_re, M_im, dM_re, dM_im,
                lindblad_kind, strength, 1, 0);

            d_re[s0 * 256u + t0] += dM_re[0][0];
            d_im[s0 * 256u + t0] += dM_im[0][0];
            d_re[s0 * 256u + t1] += dM_re[0][1];
            d_im[s0 * 256u + t1] += dM_im[0][1];
            d_re[s1 * 256u + t0] += dM_re[1][0];
            d_im[s1 * 256u + t0] += dM_im[1][0];
            d_re[s1 * 256u + t1] += dM_re[1][1];
            d_im[s1 * 256u + t1] += dM_im[1][1];
        }
    }

    for (uint32_t i = 0; i < 256u; ++i) {
        for (uint32_t j = 0; j < 256u; ++j) {
            pro_fd_set(fd, i, j,
                r_re[i * 256u + j] + d_re[i * 256u + j],
                r_im[i * 256u + j] + d_im[i * 256u + j]);
        }
    }

    free(r_re); free(r_im); free(d_re); free(d_im);
    return true;
}

PROPHYSICS_API bool ProPhysics_FockDensity_Partial_Trace_Modes(
    const ProUniverse* pu, uint32_t rho_id,
    uint32_t n_keep, ProAmpQ31* out_rho, uint32_t* out_dim)
{
    if (!pu || !pu->fock_densities || !out_rho) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    if (n_keep == 0u || n_keep > 7u) return false;
    const ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return false;

    const uint32_t dim_A = 1u << n_keep;
    const uint32_t dim_B = 256u >> n_keep;
    if (out_dim) *out_dim = dim_A;

    for (uint32_t a = 0; a < dim_A; ++a) {
        for (uint32_t ap = 0; ap < dim_A; ++ap) {
            double sr = 0.0, si = 0.0;
            for (uint32_t b = 0; b < dim_B; ++b) {
                const uint32_t i = (b << n_keep) | a;
                const uint32_t j = (b << n_keep) | ap;
                sr += pro_fd_re(fd, i, j);
                si += pro_fd_im(fd, i, j);
            }
            out_rho[a * dim_A + ap] = pro_amp_pack(
                pro_sat_i32((int64_t)llround(sr * 2147483647.0)),
                pro_sat_i32((int64_t)llround(si * 2147483647.0)));
        }
    }
    return true;
}

/* ==========================================================================
 * Auto-Sync-API
 *
 * Nach jedem SDK-Tick werden alle aktiven Dichte-Objekte mit ihren
 * Grundzustaenden synchronisiert -- analog zu auto_sync_tensor.
 *
 * Grundzustand pro Ebene:
 *   ProTensorDensity  <-  ProAmpTensorPair (reiner Tensor)
 *   ProFockDensity    <-  ProFockState      (reiner Fock)
 *
 * ProDensityMatrix (8x8) hat kein Binding an einen Knoten und ist
 * NICHT Teil von auto_sync_density (bewusste Einschraenkung).
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Density_Set_Auto_Sync(ProUniverse* pu, int enable)
{
    if (!pu) return;
    pu->auto_sync_density = (enable != 0) ? 1u : 0u;
}

PROPHYSICS_API bool ProPhysics_TensorDensity_Sync_From_Pair(
    ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->tensor_densities) return false;
    if (rho_id >= pu->tensor_density_capacity) return false;
    ProTensorDensity* td = &pu->tensor_densities[rho_id];
    if (td->active == 0u) return false;
    if (td->pair_id >= pu->tensor_pair_capacity) return false;

    return ProPhysics_TensorDensity_From_Pair(pu, rho_id, td->pair_id);
}

PROPHYSICS_API bool ProPhysics_FockDensity_Sync_From_Fock(
    ProUniverse* pu, uint32_t rho_id)
{
    if (!pu || !pu->fock_densities) return false;
    if (rho_id >= pu->fock_density_capacity) return false;
    ProFockDensity* fd = &pu->fock_densities[rho_id];
    if (fd->active == 0u) return false;
    if (fd->fock_id == UINT64_MAX) return false;
    if (fd->fock_id >= pu->fock_state_capacity) return false;

    return ProPhysics_FockDensity_From_Fock(pu, rho_id, fd->fock_id);
}

PROPHYSICS_API bool ProPhysics_TensorDensity_Sync_All_From_Pairs(ProUniverse* pu)
{
    if (!pu || !pu->tensor_densities) return false;
    for (uint32_t i = 0; i < pu->tensor_density_capacity; ++i) {
        if (pu->tensor_densities[i].active == 0u) continue;
        ProPhysics_TensorDensity_Sync_From_Pair(pu, i);
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_FockDensity_Sync_All_From_Fock(ProUniverse* pu)
{
    if (!pu || !pu->fock_densities) return false;
    for (uint32_t i = 0; i < pu->fock_density_capacity; ++i) {
        if (pu->fock_densities[i].active == 0u) continue;
        ProPhysics_FockDensity_Sync_From_Fock(pu, i);
    }
    return true;
}

/* ==========================================================================
 * End of ProPhysics_Density.c
 * ========================================================================== */