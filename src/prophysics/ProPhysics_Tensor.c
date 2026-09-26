/* ==========================================================================
 * ProPhysics - Tensor Modul
 * File: ProPhysics_Tensor.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * 2-Knoten-Verschaenkung in Q31. Tensorpaare sind isolierte Objekte,
 * optional an amp_grid gekoppelt.
 *
 * Verantwortlich fuer:
 *   - Jacobi-Eigenzerlegung (shared mit ProPhysics_Density.c)
 *   - Tensor-Paar-Lifecycle (Create, Destroy, Set/Get_State, Local_Op)
 *   - Partial_Trace_A / _B, VN-Entropie, Tensor_Entanglement_Entropy
 *   - Tensor_Measure_Projective (ECHTE Born-Regel, kein sharp-Modell)
 *   - Sync_To_Amp (Tensor -> amp_grid, dominanter Eigenvektor)
 *   - Sync_From_Amp (amp_grid -> Tensor, aeusseres Produkt)
 *   - Gatter: Hadamard, X, Y, Z, CNOT, CZ, SWAP, SqrtSwap
 *   - Concurrence
 *   - Mark / Unmark / Auto_Sync / XX-Kopplung / Hopping
 *   - SU(2)-Rotation, Spin-Messung, SU(2)-Algebra-Check
 *   - Fermionen: Is_Antisymmetric, Pauli_Violation, Antisymmetrize,
 *     Fermionize, Set_Slater, Apply_Hopping
 *   - Tensor<->Fock-Adapter (2-Teilchen-Sektor)
 *
 * Konventionen:
 *   - Basis-Layout: coeff[a*8 + b], a = Knoten A, b = Knoten B.
 *   - Qubit-Basis: {1, 4} = {UR_POSITRON_CW, UR_NEGATRON_CCW}.
 *   - Antisymmetrie: psi[a,b] = -psi[b,a] (Fermionen-Konvention).
 *   - Skala: Q31 (2^31-1 = 1.0).
 *   - Q62-Normierung: 2^62 = physikalisch 1.0 (in Antisymmetrize).
 *
 * Interne Helfer:
 *   - pro_rho_to_real16 (Q31 8x8 -> reelles 16x16)
 *   - pro_write_dominant_eigvec (Jacobi-Eigenvektor nach amp_grid)
 *   - pro_tensor_cmul_acc (acc += a*b in Q31)
 *   - pro_tensor_cmul_conj_acc (acc += a*conj(b) in Q31)
 *   - pro_tensor_get / pro_tensor_set (Zugriff mit Index-Layout)
 *   - pro_pair_to_bits (Tensor-Paar -> Fock-Bits)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/Tensor.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer
  * ========================================================================== */

  /* acc += a * b (komplexe Multiplikation in Q31, ein Term).
   *
   *   (a_re + i·a_im) · (b_re + i·b_im)
   *     = (a_re·b_re - a_im·b_im) + i·(a_re·b_im + a_im·b_re)
   *
   * Akkumulation erfolgt NACH dem pro_round_shift_q31, damit die
   * Reihenfolge der Shift-Operationen bit-identisch zu den vormals
   * inline-geschriebenen Summen bleibt. */
static inline void pro_tensor_cmul_acc(
    int64_t ar, int64_t ai, int64_t br, int64_t bi,
    int64_t* acc_re, int64_t* acc_im)
{
    *acc_re += pro_round_shift_q31(ar * br - ai * bi);
    *acc_im += pro_round_shift_q31(ar * bi + ai * br);
}

/* acc += a * conj(b) (komplexe Multiplikation mit Konjugation).
 *
 *   (a_re + i·a_im) · (b_re - i·b_im)
 *     = (a_re·b_re + a_im·b_im) + i·(a_im·b_re - a_re·b_im) */
static inline void pro_tensor_cmul_conj_acc(
    int64_t ar, int64_t ai, int64_t br, int64_t bi,
    int64_t* acc_re, int64_t* acc_im)
{
    *acc_re += pro_round_shift_q31(ar * br + ai * bi);
    *acc_im += pro_round_shift_q31(ai * br - ar * bi);
}

/* Q31-rho (8x8 komplex) -> reelles 16x16-Embedding.
 *
 *   M[2i,   2j  ] =  Re(rho[i][j]) / 2^31
 *   M[2i,   2j+1] = -Im(rho[i][j]) / 2^31
 *   M[2i+1, 2j  ] =  Im(rho[i][j]) / 2^31
 *   M[2i+1, 2j+1] =  Re(rho[i][j]) / 2^31
 *
 * Verwendung in ProPhysics_Von_Neumann_Entropy und in
 * ProPhysics_Tensor_Sync_To_Amp (beide mit anschliessender
 * Jacobi-Eigenzerlegung). */
static void pro_rho_to_real16(
    const ProAmpQ31 rho[8][8], double M[16][16])
{
    for (int a = 0; a < 8; ++a) {
        for (int ap = 0; ap < 8; ++ap) {
            const double re = (double)pro_amp_real(rho[a][ap]) / 2147483647.0;
            const double im = (double)pro_amp_imag(rho[a][ap]) / 2147483647.0;
            M[2 * a][2 * ap] = re;
            M[2 * a][2 * ap + 1] = -im;
            M[2 * a + 1][2 * ap] = im;
            M[2 * a + 1][2 * ap + 1] = re;
        }
    }
}

/* ==========================================================================
 * Jacobi-Eigenzerlegung fuer reelle symmetrische n x n-Matrix (in-place).
 *
 * A: Matrix, Diagonale = Eigenwerte nach Konvergenz.
 * V: optional (NULL erlaubt). Wenn != NULL, akkumuliert V die
 *    Rotationsmatrix. Spalte k von V = Eigenvektor zu A[k][k].
 * n <= 16.
 *
 * Shared mit ProPhysics_Density.c (dort via ProPhysics_Internal.h).
 * ========================================================================== */

void pro_jacobi_sym_eigen(double A[16][16], double V[16][16], int n)
{
    if (V) {
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                V[i][j] = (i == j) ? 1.0 : 0.0;
    }

    const int MAX_SWEEPS = 100;
    for (int sweep = 0; sweep < MAX_SWEEPS; ++sweep) {
        double off = 0.0;
        for (int p = 0; p < n - 1; ++p)
            for (int q = p + 1; q < n; ++q)
                off += A[p][q] * A[p][q];
        if (off < 1e-24) break;

        for (int p = 0; p < n - 1; ++p) {
            for (int q = p + 1; q < n; ++q) {
                if (fabs(A[p][q]) < 1e-18) continue;
                const double theta = (A[q][q] - A[p][p]) / (2.0 * A[p][q]);
                const double t = (theta >= 0.0)
                    ? 1.0 / (theta + sqrt(theta * theta + 1.0))
                    : 1.0 / (theta - sqrt(theta * theta + 1.0));
                const double c = 1.0 / sqrt(t * t + 1.0);
                const double s = t * c;

                const double App = A[p][p];
                const double Aqq = A[q][q];
                const double Apq = A[p][q];
                A[p][p] = App - t * Apq;
                A[q][q] = Aqq + t * Apq;
                A[p][q] = 0.0;
                A[q][p] = 0.0;

                for (int k = 0; k < n; ++k) {
                    if (k == p || k == q) continue;
                    const double Akp = A[k][p];
                    const double Akq = A[k][q];
                    A[k][p] = A[p][k] = c * Akp - s * Akq;
                    A[k][q] = A[q][k] = s * Akp + c * Akq;
                }

                if (V) {
                    for (int k = 0; k < n; ++k) {
                        const double Vkp = V[k][p];
                        const double Vkq = V[k][q];
                        V[k][p] = c * Vkp - s * Vkq;
                        V[k][q] = s * Vkp + c * Vkq;
                    }
                }
            }
        }
    }
}

/* ==========================================================================
 * Tensor-Paar Lifecycle
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Create_Pair(
    ProUniverse* pu, uint64_t node_a, uint64_t node_b, uint32_t* out_pair_id)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (node_a >= pu->total_nodes) return false;
    if (node_b >= pu->total_nodes) return false;
    if (node_a == node_b) return false;

    int32_t slot = -1;
    for (uint32_t i = 0; i < pu->tensor_pair_capacity; ++i) {
        if (pu->tensor_pairs[i].active == 0u) { slot = (int32_t)i; break; }
    }
    if (slot < 0) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[slot];
    p->node_a = node_a;
    p->node_b = node_b;
    p->pair_id = (uint32_t)slot;
    p->active = 1u;
    p->coupling_q15 = 0;
    p->_coupling_pad = 0u;
    p->hopping_theta_q15 = 0;
    p->hopping_i = 0u;
    p->hopping_j = 0u;
    p->_hopping_pad = 0u;

    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) p->coeff[i] = 0;
    p->coeff[0] = pro_amp_pack(INT32_MAX, 0);   /* |0> (x) |0> */

    pu->tensor_pair_count++;
    if (out_pair_id) *out_pair_id = (uint32_t)slot;
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Destroy_Pair(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    pu->tensor_pairs[pair_id].active = 0u;
    if (pu->tensor_pair_count > 0u) pu->tensor_pair_count--;
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Set_State(
    ProUniverse* pu, uint32_t pair_id, const ProAmpQ31 coeff[PRO_TENSOR_DIM])
{
    if (!pu || !pu->tensor_pairs || !coeff) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    memcpy(pu->tensor_pairs[pair_id].coeff, coeff,
        PRO_TENSOR_DIM * sizeof(ProAmpQ31));
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Get_State(
    const ProUniverse* pu, uint32_t pair_id, ProAmpQ31 coeff_out[PRO_TENSOR_DIM])
{
    if (!pu || !pu->tensor_pairs || !coeff_out) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    memcpy(coeff_out, pu->tensor_pairs[pair_id].coeff,
        PRO_TENSOR_DIM * sizeof(ProAmpQ31));
    return true;
}

/* ==========================================================================
 * Local_Op (U (x) I bzw. I (x) U, 8x8-Matrix)
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Local_Op(
    ProUniverse* pu, uint32_t pair_id, int side,
    const ProAmpQ31 U[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM])
{
    if (!pu || !pu->tensor_pairs || !U) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    if (side != 0 && side != 1) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    ProAmpQ31 tmp[PRO_TENSOR_DIM];

    if (side == 0) {
        /* U (x) I: new[a, b] = Sum_{a'} U[a][a'] * old[a', b] */
        for (int b = 0; b < 8; ++b) {
            for (int a = 0; a < 8; ++a) {
                int64_t sum_re = 0, sum_im = 0;
                for (int ap = 0; ap < 8; ++ap) {
                    const ProAmpQ31 u = U[a][ap];
                    const ProAmpQ31 psi = p->coeff[ap * 8 + b];
                    pro_tensor_cmul_acc(
                        pro_amp_real(u), pro_amp_imag(u),
                        pro_amp_real(psi), pro_amp_imag(psi),
                        &sum_re, &sum_im);
                }
                tmp[a * 8 + b] = pro_amp_pack(pro_sat_i32(sum_re),
                    pro_sat_i32(sum_im));
            }
        }
    }
    else {
        /* I (x) U: new[a, b] = Sum_{b'} U[b][b'] * old[a, b'] */
        for (int a = 0; a < 8; ++a) {
            for (int b = 0; b < 8; ++b) {
                int64_t sum_re = 0, sum_im = 0;
                for (int bp = 0; bp < 8; ++bp) {
                    const ProAmpQ31 u = U[b][bp];
                    const ProAmpQ31 psi = p->coeff[a * 8 + bp];
                    pro_tensor_cmul_acc(
                        pro_amp_real(u), pro_amp_imag(u),
                        pro_amp_real(psi), pro_amp_imag(psi),
                        &sum_re, &sum_im);
                }
                tmp[a * 8 + b] = pro_amp_pack(pro_sat_i32(sum_re),
                    pro_sat_i32(sum_im));
            }
        }
    }

    memcpy(p->coeff, tmp, PRO_TENSOR_DIM * sizeof(ProAmpQ31));
    return true;
}

/* ==========================================================================
 * Partial Traces
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_A(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM])
{
    if (!pu || !pu->tensor_pairs || !out_rho) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;

    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];

    for (int a = 0; a < 8; ++a) {
        for (int ap = 0; ap < 8; ++ap) {
            int64_t sum_re = 0, sum_im = 0;
            for (int b = 0; b < 8; ++b) {
                const ProAmpQ31 psi_ab = p->coeff[a * 8 + b];
                const ProAmpQ31 psi_apb = p->coeff[ap * 8 + b];
                pro_tensor_cmul_conj_acc(
                    pro_amp_real(psi_ab), pro_amp_imag(psi_ab),
                    pro_amp_real(psi_apb), pro_amp_imag(psi_apb),
                    &sum_re, &sum_im);
            }
            out_rho[a][ap] = pro_amp_pack(pro_sat_i32(sum_re),
                pro_sat_i32(sum_im));
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_B(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM])
{
    if (!pu || !pu->tensor_pairs || !out_rho) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;

    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];

    for (int b = 0; b < 8; ++b) {
        for (int bp = 0; bp < 8; ++bp) {
            int64_t sum_re = 0, sum_im = 0;
            for (int a = 0; a < 8; ++a) {
                const ProAmpQ31 psi_ab = p->coeff[a * 8 + b];
                const ProAmpQ31 psi_abp = p->coeff[a * 8 + bp];
                pro_tensor_cmul_conj_acc(
                    pro_amp_real(psi_ab), pro_amp_imag(psi_ab),
                    pro_amp_real(psi_abp), pro_amp_imag(psi_abp),
                    &sum_re, &sum_im);
            }
            out_rho[b][bp] = pro_amp_pack(pro_sat_i32(sum_re),
                pro_sat_i32(sum_im));
        }
    }
    return true;
}

/* ==========================================================================
 * Von-Neumann-Entropie (Real-Embedding 8x8 -> 16x16)
 * ========================================================================== */

PROPHYSICS_API double ProPhysics_Von_Neumann_Entropy(
    const ProAmpQ31 rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM])
{
    if (!rho) return 0.0;

    double M[16][16];
    pro_rho_to_real16(rho, M);
    pro_jacobi_sym_eigen(M, NULL, 16);

    double S = 0.0;
    for (int k = 0; k < 16; ++k) {
        const double mu = M[k][k];
        if (mu > 1e-12) S -= 0.5 * mu * log(mu);
    }
    return S;
}

PROPHYSICS_API double ProPhysics_Tensor_Entanglement_Entropy(
    const ProUniverse* pu, uint32_t pair_id)
{
    ProAmpQ31 rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM];
    if (!ProPhysics_Tensor_Partial_Trace_A(pu, pair_id, rho)) return 0.0;
    return ProPhysics_Von_Neumann_Entropy(rho);
}

/* ==========================================================================
 * Tensor_Measure_Projective
 *
 * ECHTE Born-Regel: p(sigma_a, sigma_b | theta_a, theta_b)
 *   = |<theta_a, sigma_a; theta_b, sigma_b | psi>|^2.
 * Projektor-Unterraum {|1,1>, |1,4>, |4,1>, |4,4>}.
 * Sampling via xoshiro256**. Rein lesend.
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Measure_Projective(
    const ProUniverse* pu, uint32_t pair_id,
    double theta_a, double theta_b,
    uint64_t rng[4], int* out_a, int* out_b)
{
    if (!pu || !pu->tensor_pairs || !rng) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    if (!out_a || !out_b) return false;

    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];

    const int BAS_UP = (int)UR_POSITRON_CW;    /* = 1 */
    const int BAS_DN = (int)UR_NEGATRON_CCW;   /* = 4 */

    const double c_a = cos(theta_a * 0.5);
    const double s_a = sin(theta_a * 0.5);
    const double c_b = cos(theta_b * 0.5);
    const double s_b = sin(theta_b * 0.5);

    const double phi_a[2][2] = { { c_a, s_a }, { -s_a, c_a } };
    const double phi_b[2][2] = { { c_b, s_b }, { -s_b, c_b } };

    const int    basis[2] = { BAS_UP, BAS_DN };
    const double norm_inv = 1.0 / 2147483647.0;

    double amp_re[2][2] = { { 0, 0 }, { 0, 0 } };
    double amp_im[2][2] = { { 0, 0 }, { 0, 0 } };

    for (int sa = 0; sa < 2; ++sa) {
        for (int sb = 0; sb < 2; ++sb) {
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 2; ++j) {
                    const ProAmpQ31 psi = p->coeff[basis[i] * 8 + basis[j]];
                    const double pr = (double)pro_amp_real(psi) * norm_inv;
                    const double pi_ = (double)pro_amp_imag(psi) * norm_inv;
                    const double w = phi_a[sa][i] * phi_b[sb][j];
                    amp_re[sa][sb] += w * pr;
                    amp_im[sa][sb] += w * pi_;
                }
            }
        }
    }

    double prob[2][2];
    double p_total = 0.0;
    for (int sa = 0; sa < 2; ++sa) {
        for (int sb = 0; sb < 2; ++sb) {
            prob[sa][sb] = amp_re[sa][sb] * amp_re[sa][sb]
                + amp_im[sa][sb] * amp_im[sa][sb];
            p_total += prob[sa][sb];
        }
    }

    if (p_total < 0.5) return false;

    const double inv_total = 1.0 / p_total;
    double p_norm[2][2];
    for (int sa = 0; sa < 2; ++sa) {
        for (int sb = 0; sb < 2; ++sb) {
            p_norm[sa][sb] = prob[sa][sb] * inv_total;
        }
    }

    const double u = (double)(pro_xoshiro_next(rng) >> 11)
        / 9007199254740992.0;

    double cum = 0.0;
    int out_sa = 1, out_sb = 1;
    int found = 0;

    for (int sa = 0; sa < 2 && !found; ++sa) {
        for (int sb = 0; sb < 2 && !found; ++sb) {
            cum += p_norm[sa][sb];
            if (u < cum) {
                out_sa = sa;
                out_sb = sb;
                found = 1;
            }
        }
    }

    *out_a = (out_sa == 0) ? +1 : -1;
    *out_b = (out_sb == 0) ? +1 : -1;
    return true;
}

/* ==========================================================================
 * Find_Pair_By_Node
 * ========================================================================== */

PROPHYSICS_API uint32_t ProPhysics_Tensor_Find_Pair_By_Node(
    const ProUniverse* pu, uint64_t node)
{
    if (!pu || !pu->tensor_pairs) return UINT32_MAX;
    for (uint32_t i = 0; i < pu->tensor_pair_capacity; ++i) {
        const ProAmpTensorPair* p = &pu->tensor_pairs[i];
        if (p->active == 0u) continue;
        if (p->node_a == node || p->node_b == node) return i;
    }
    return UINT32_MAX;
}

/* ==========================================================================
 * Sync Tensor -> amp_grid
 *
 * amp_grid[k] ist die Marginale des fundamentalen Tensor-Zustands.
 * Fuer Paare: dominanter Eigenvektor von rho_k.
 *
 * Interner Helfer: rho (8x8 Q31 komplex) -> 16x16 reell-symmetrisch, dann
 * Jacobi mit Eigenvektoren, dominanter zurueck nach Q31.
 * ========================================================================== */

static void pro_write_dominant_eigvec(
    ProUniverse* pu, uint64_t node, double M[16][16])
{
    double V[16][16];
    pro_jacobi_sym_eigen(M, V, 16);

    int best = 0;
    double best_val = M[0][0];
    for (int k = 1; k < 16; ++k) {
        if (M[k][k] > best_val) { best_val = M[k][k]; best = k; }
    }

    for (int b = 0; b < 8; ++b) {
        const double re = V[2 * b][best] * 2147483647.0;
        const double im = V[2 * b + 1][best] * 2147483647.0;
        pu->amp_grid[node].coeff[b] =
            pro_amp_pack(pro_sat_i32((int64_t)llround(re)),
                pro_sat_i32((int64_t)llround(im)));
    }
}

PROPHYSICS_API bool ProPhysics_Tensor_Sync_To_Amp(ProUniverse* pu)
{
    if (!pu || !pu->tensor_pairs || !pu->amp_grid) return false;

    for (uint32_t i = 0; i < pu->tensor_pair_capacity; ++i) {
        ProAmpTensorPair* p = &pu->tensor_pairs[i];
        if (p->active == 0u) continue;
        if (p->node_a >= pu->total_nodes) continue;
        if (p->node_b >= pu->total_nodes) continue;

        ProAmpQ31 rho[8][8];
        double M[16][16];

        if (ProPhysics_Tensor_Partial_Trace_A(pu, i, rho)) {
            pro_rho_to_real16(rho, M);
            pro_write_dominant_eigvec(pu, p->node_a, M);
        }

        if (ProPhysics_Tensor_Partial_Trace_B(pu, i, rho)) {
            pro_rho_to_real16(rho, M);
            pro_write_dominant_eigvec(pu, p->node_b, M);
        }
    }
    return true;
}

/* ==========================================================================
 * Sync amp_grid -> Tensor
 *
 * Aeusseres Produkt der beiden Knoten-Amplitudenvektoren:
 *   p->coeff[a*8+b] = amp_grid[node_a].coeff[a] * amp_grid[node_b].coeff[b]
 * (mit Q31-Rundung, Clamping).
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Sync_From_Amp(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs || !pu->amp_grid) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;
    if (p->node_a >= pu->total_nodes) return false;
    if (p->node_b >= pu->total_nodes) return false;

    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            const ProAmpQ31 ca = pu->amp_grid[p->node_a].coeff[a];
            const ProAmpQ31 cb = pu->amp_grid[p->node_b].coeff[b];

            int64_t re = 0, im = 0;
            pro_tensor_cmul_acc(
                pro_amp_real(ca), pro_amp_imag(ca),
                pro_amp_real(cb), pro_amp_imag(cb),
                &re, &im);

            p->coeff[a * 8 + b] = pro_amp_pack(pro_sat_i32(re),
                pro_sat_i32(im));
        }
    }
    return true;
}

/* ==========================================================================
 * 1-Qubit-Gatter
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Single_Qubit_Gate(
    ProUniverse* pu, uint32_t pair_id, int side, const ProAmpQ31 U[2][2])
{
    if (!pu || !pu->tensor_pairs || !U) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    if (side != 0 && side != 1) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    ProAmpQ31 tmp[PRO_TENSOR_DIM];
    memcpy(tmp, p->coeff, PRO_TENSOR_DIM * sizeof(ProAmpQ31));

    const int64_t u00r = pro_amp_real(U[0][0]), u00i = pro_amp_imag(U[0][0]);
    const int64_t u01r = pro_amp_real(U[0][1]), u01i = pro_amp_imag(U[0][1]);
    const int64_t u10r = pro_amp_real(U[1][0]), u10i = pro_amp_imag(U[1][0]);
    const int64_t u11r = pro_amp_real(U[1][1]), u11i = pro_amp_imag(U[1][1]);

    const int b0 = PRO_QUBIT_BASIS[0];
    const int b1 = PRO_QUBIT_BASIS[1];

    if (side == 0) {
        for (int j = 0; j < 8; ++j) {
            const ProAmpQ31 o0 = tmp[b0 * 8 + j];
            const ProAmpQ31 o1 = tmp[b1 * 8 + j];
            const int64_t o0r = pro_amp_real(o0), o0i = pro_amp_imag(o0);
            const int64_t o1r = pro_amp_real(o1), o1i = pro_amp_imag(o1);

            int64_t nr0 = 0, ni0 = 0, nr1 = 0, ni1 = 0;
            pro_tensor_cmul_acc(u00r, u00i, o0r, o0i, &nr0, &ni0);
            pro_tensor_cmul_acc(u01r, u01i, o1r, o1i, &nr0, &ni0);
            pro_tensor_cmul_acc(u10r, u10i, o0r, o0i, &nr1, &ni1);
            pro_tensor_cmul_acc(u11r, u11i, o1r, o1i, &nr1, &ni1);

            p->coeff[b0 * 8 + j] = pro_amp_pack(pro_sat_i32(nr0),
                pro_sat_i32(ni0));
            p->coeff[b1 * 8 + j] = pro_amp_pack(pro_sat_i32(nr1),
                pro_sat_i32(ni1));
        }
    }
    else {
        for (int i = 0; i < 8; ++i) {
            const ProAmpQ31 o0 = tmp[i * 8 + b0];
            const ProAmpQ31 o1 = tmp[i * 8 + b1];
            const int64_t o0r = pro_amp_real(o0), o0i = pro_amp_imag(o0);
            const int64_t o1r = pro_amp_real(o1), o1i = pro_amp_imag(o1);

            int64_t nr0 = 0, ni0 = 0, nr1 = 0, ni1 = 0;
            pro_tensor_cmul_acc(u00r, u00i, o0r, o0i, &nr0, &ni0);
            pro_tensor_cmul_acc(u01r, u01i, o1r, o1i, &nr0, &ni0);
            pro_tensor_cmul_acc(u10r, u10i, o0r, o0i, &nr1, &ni1);
            pro_tensor_cmul_acc(u11r, u11i, o1r, o1i, &nr1, &ni1);

            p->coeff[i * 8 + b0] = pro_amp_pack(pro_sat_i32(nr0),
                pro_sat_i32(ni0));
            p->coeff[i * 8 + b1] = pro_amp_pack(pro_sat_i32(nr1),
                pro_sat_i32(ni1));
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Two_Qubit_Gate(
    ProUniverse* pu, uint32_t pair_id, const ProAmpQ31 G[4][4])
{
    if (!pu || !pu->tensor_pairs || !G) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    const int b0 = PRO_QUBIT_BASIS[0];
    const int b1 = PRO_QUBIT_BASIS[1];

    int64_t gr[4][4], gi[4][4];
    for (int a = 0; a < 4; ++a) {
        for (int b = 0; b < 4; ++b) {
            gr[a][b] = pro_amp_real(G[a][b]);
            gi[a][b] = pro_amp_imag(G[a][b]);
        }
    }

    ProAmpQ31 old[4];
    old[0] = p->coeff[b0 * 8 + b0];
    old[1] = p->coeff[b0 * 8 + b1];
    old[2] = p->coeff[b1 * 8 + b0];
    old[3] = p->coeff[b1 * 8 + b1];

    ProAmpQ31 out[4] = { 0, 0, 0, 0 };
    for (int a = 0; a < 4; ++a) {
        int64_t sr = 0, si = 0;
        for (int b = 0; b < 4; ++b) {
            pro_tensor_cmul_acc(gr[a][b], gi[a][b],
                pro_amp_real(old[b]), pro_amp_imag(old[b]),
                &sr, &si);
        }
        out[a] = pro_amp_pack(pro_sat_i32(sr), pro_sat_i32(si));
    }

    p->coeff[b0 * 8 + b0] = out[0];
    p->coeff[b0 * 8 + b1] = out[1];
    p->coeff[b1 * 8 + b0] = out[2];
    p->coeff[b1 * 8 + b1] = out[3];
    return true;
}

/* --- Convenience-Wrapper --- */

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Hadamard(
    ProUniverse* pu, uint32_t pair_id, int side)
{
    const ProAmpQ31 h = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);
    const ProAmpQ31 m = pro_amp_pack(-PRO_Q31_HALF_SQRT2, 0);
    const ProAmpQ31 U[2][2] = { { h, h }, { h, m } };
    return ProPhysics_Tensor_Apply_Single_Qubit_Gate(pu, pair_id, side, U);
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_X(
    ProUniverse* pu, uint32_t pair_id, int side)
{
    const ProAmpQ31 one = pro_amp_pack(INT32_MAX, 0);
    const ProAmpQ31 U[2][2] = { { 0, one }, { one, 0 } };
    return ProPhysics_Tensor_Apply_Single_Qubit_Gate(pu, pair_id, side, U);
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Y(
    ProUniverse* pu, uint32_t pair_id, int side)
{
    const ProAmpQ31 neg_i = pro_amp_pack(0, -INT32_MAX);
    const ProAmpQ31 pos_i = pro_amp_pack(0, INT32_MAX);
    const ProAmpQ31 U[2][2] = { { 0, neg_i }, { pos_i, 0 } };
    return ProPhysics_Tensor_Apply_Single_Qubit_Gate(pu, pair_id, side, U);
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Z(
    ProUniverse* pu, uint32_t pair_id, int side)
{
    const ProAmpQ31 one = pro_amp_pack(INT32_MAX, 0);
    const ProAmpQ31 m = pro_amp_pack(-INT32_MAX, 0);
    const ProAmpQ31 U[2][2] = { { one, 0 }, { 0, m } };
    return ProPhysics_Tensor_Apply_Single_Qubit_Gate(pu, pair_id, side, U);
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_CNOT(
    ProUniverse* pu, uint32_t pair_id, int side_control)
{
    if (side_control != 0 && side_control != 1) return false;

    const ProAmpQ31 one = pro_amp_pack(INT32_MAX, 0);
    ProAmpQ31 G[4][4] = { {0} };
    G[0][0] = one;
    G[1][1] = one;
    G[2][3] = one;
    G[3][2] = one;

    if (side_control == 0) {
        return ProPhysics_Tensor_Apply_Two_Qubit_Gate(pu, pair_id, G);
    }
    else {
        ProAmpQ31 G2[4][4] = { {0} };
        G2[0][0] = one;
        G2[3][1] = one;
        G2[2][2] = one;
        G2[1][3] = one;
        return ProPhysics_Tensor_Apply_Two_Qubit_Gate(pu, pair_id, G2);
    }
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_CZ(
    ProUniverse* pu, uint32_t pair_id)
{
    const ProAmpQ31 one = pro_amp_pack(INT32_MAX, 0);
    const ProAmpQ31 m = pro_amp_pack(-INT32_MAX, 0);
    ProAmpQ31 G[4][4] = { {0} };
    G[0][0] = one;
    G[1][1] = one;
    G[2][2] = one;
    G[3][3] = m;
    return ProPhysics_Tensor_Apply_Two_Qubit_Gate(pu, pair_id, G);
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_SWAP(
    ProUniverse* pu, uint32_t pair_id)
{
    const ProAmpQ31 one = pro_amp_pack(INT32_MAX, 0);
    ProAmpQ31 G[4][4] = { {0} };
    G[0][0] = one;
    G[1][2] = one;
    G[2][1] = one;
    G[3][3] = one;
    return ProPhysics_Tensor_Apply_Two_Qubit_Gate(pu, pair_id, G);
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_SqrtSwap(
    ProUniverse* pu, uint32_t pair_id)
{
    const ProAmpQ31 one = pro_amp_pack(INT32_MAX, 0);
    const ProAmpQ31 half = pro_amp_pack(INT32_MAX / 2, INT32_MAX / 2);
    const ProAmpQ31 half_c = pro_amp_pack(INT32_MAX / 2, -INT32_MAX / 2);
    ProAmpQ31 G[4][4] = { {0} };
    G[0][0] = one;
    G[1][1] = half;
    G[1][2] = half_c;
    G[2][1] = half_c;
    G[2][2] = half;
    G[3][3] = one;
    return ProPhysics_Tensor_Apply_Two_Qubit_Gate(pu, pair_id, G);
}

/* ==========================================================================
 * Concurrence (2-Qubit, rein, in {1,4}x{1,4}-Unterebene)
 *   C = 2*|c_00*c_11 - c_01*c_10| / Sum|c|^2
 * ========================================================================== */

PROPHYSICS_API double ProPhysics_Tensor_Concurrence(
    const ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return 0.0;
    if (pair_id >= pu->tensor_pair_capacity) return 0.0;
    if (pu->tensor_pairs[pair_id].active == 0u) return 0.0;

    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    const int b0 = PRO_QUBIT_BASIS[0];
    const int b1 = PRO_QUBIT_BASIS[1];

    const ProAmpQ31 c00 = p->coeff[b0 * 8 + b0];
    const ProAmpQ31 c01 = p->coeff[b0 * 8 + b1];
    const ProAmpQ31 c10 = p->coeff[b1 * 8 + b0];
    const ProAmpQ31 c11 = p->coeff[b1 * 8 + b1];

    const double inv = 1.0 / 2147483647.0;
    const double c00r = pro_amp_real(c00) * inv, c00i = pro_amp_imag(c00) * inv;
    const double c01r = pro_amp_real(c01) * inv, c01i = pro_amp_imag(c01) * inv;
    const double c10r = pro_amp_real(c10) * inv, c10i = pro_amp_imag(c10) * inv;
    const double c11r = pro_amp_real(c11) * inv, c11i = pro_amp_imag(c11) * inv;

    const double dr = c00r * c11r - c00i * c11i - (c01r * c10r - c01i * c10i);
    const double di = c00r * c11i + c00i * c11r - (c01r * c10i + c01i * c10r);

    const double norm = c00r * c00r + c00i * c00i
        + c01r * c01r + c01i * c01i
        + c10r * c10r + c10i * c10i
        + c11r * c11r + c11i * c11i;
    if (norm < 1e-20) return 0.0;

    return 2.0 * sqrt(dr * dr + di * di) / norm;
}

/* ==========================================================================
 * Mark / Unmark / Auto-Sync
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Mark_Nodes(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs || !pu->tensor_marks) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;
    if (p->node_a >= pu->total_nodes) return false;
    if (p->node_b >= pu->total_nodes) return false;

    pu->tensor_marks[p->node_a] = 1u;
    pu->tensor_marks[p->node_b] = 1u;
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Unmark_Nodes(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs || !pu->tensor_marks) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;

    if (p->node_a < pu->total_nodes) pu->tensor_marks[p->node_a] = 0u;
    if (p->node_b < pu->total_nodes) pu->tensor_marks[p->node_b] = 0u;
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Is_Node_Marked(
    const ProUniverse* pu, uint64_t node)
{
    if (!pu || !pu->tensor_marks) return false;
    if (node >= pu->total_nodes) return false;
    return pu->tensor_marks[node] != 0u;
}

PROPHYSICS_API void ProPhysics_Tensor_Set_Auto_Sync(
    ProUniverse* pu, int enable)
{
    if (!pu) return;
    pu->auto_sync_tensor = (enable != 0) ? 1u : 0u;
}

PROPHYSICS_API bool ProPhysics_Tensor_Sync_Marked_To_Amp(ProUniverse* pu)
{
    if (!pu || !pu->tensor_pairs || !pu->tensor_marks) return false;
    return ProPhysics_Tensor_Sync_To_Amp(pu);
}

/* ==========================================================================
 * XX-Kopplung
 *
 * U(theta) = cos(theta) * I - i sin(theta) * sigma_x (x) sigma_x
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Set_Coupling(
    ProUniverse* pu, uint32_t pair_id, int32_t theta_q15)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;
    pu->tensor_pairs[pair_id].coupling_q15 = theta_q15;
    return true;
}

PROPHYSICS_API int32_t ProPhysics_Tensor_Get_Coupling(
    const ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return 0;
    if (pair_id >= pu->tensor_pair_capacity) return 0;
    if (pu->tensor_pairs[pair_id].active == 0u) return 0;
    return pu->tensor_pairs[pair_id].coupling_q15;
}

PROPHYSICS_API bool ProPhysics_Tensor_Apply_XX_Step(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;
    if (p->coupling_q15 == 0) return false;

    const double theta = (double)p->coupling_q15 / 32768.0;
    const double c = cos(theta);
    const double s = sin(theta);

    const int32_t cr = pro_sat_i32((int64_t)llround(c * 2147483647.0));
    const int32_t sr = pro_sat_i32((int64_t)llround(s * 2147483647.0));

    const ProAmpQ31 cos_q = pro_amp_pack(cr, 0);
    const ProAmpQ31 neg_i_s = pro_amp_pack(0, -sr);

    ProAmpQ31 G[4][4] = { {0} };
    G[0][0] = cos_q;    G[0][3] = neg_i_s;
    G[1][1] = cos_q;    G[1][2] = neg_i_s;
    G[2][1] = neg_i_s;  G[2][2] = cos_q;
    G[3][0] = neg_i_s;  G[3][3] = cos_q;

    return ProPhysics_Tensor_Apply_Two_Qubit_Gate(pu, pair_id, G);
}

/* ==========================================================================
 * SU(2) auf Spinor-Paaren
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Apply_SU2_Rotation(
    ProUniverse* pu, uint64_t node, int which_spinor,
    double nx, double ny, double nz, double alpha)
{
    if (!pu || !pu->amp_grid) return false;
    if (node >= pu->total_nodes) return false;
    if (which_spinor < 0 || which_spinor > 1) return false;

    const double nrm = sqrt(nx * nx + ny * ny + nz * nz);
    if (nrm < 1e-12) return false;
    nx /= nrm; ny /= nrm; nz /= nrm;

    const double c = cos(alpha * 0.5);
    const double s = sin(alpha * 0.5);

    const int64_t Q = 2147483647;
    const int64_t u00r = (int64_t)llround(c * (double)Q);
    const int64_t u00i = (int64_t)llround(-s * nz * (double)Q);
    const int64_t u01r = (int64_t)llround(-s * ny * (double)Q);
    const int64_t u01i = (int64_t)llround(-s * nx * (double)Q);
    const int64_t u10r = (int64_t)llround(s * ny * (double)Q);
    const int64_t u10i = (int64_t)llround(-s * nx * (double)Q);
    const int64_t u11r = (int64_t)llround(c * (double)Q);
    const int64_t u11i = (int64_t)llround(s * nz * (double)Q);

    const int iu = PRO_SPINOR_UP[which_spinor];
    const int id = PRO_SPINOR_DN[which_spinor];

    const int64_t ar = pro_amp_real(pu->amp_grid[node].coeff[iu]);
    const int64_t ai = pro_amp_imag(pu->amp_grid[node].coeff[iu]);
    const int64_t br = pro_amp_real(pu->amp_grid[node].coeff[id]);
    const int64_t bi = pro_amp_imag(pu->amp_grid[node].coeff[id]);

    int64_t nr0 = 0, ni0 = 0, nr1 = 0, ni1 = 0;
    pro_tensor_cmul_acc(u00r, u00i, ar, ai, &nr0, &ni0);
    pro_tensor_cmul_acc(u01r, u01i, br, bi, &nr0, &ni0);
    pro_tensor_cmul_acc(u10r, u10i, ar, ai, &nr1, &ni1);
    pro_tensor_cmul_acc(u11r, u11i, br, bi, &nr1, &ni1);

    pu->amp_grid[node].coeff[iu] = pro_amp_pack(pro_sat_i32(nr0),
        pro_sat_i32(ni0));
    pu->amp_grid[node].coeff[id] = pro_amp_pack(pro_sat_i32(nr1),
        pro_sat_i32(ni1));
    return true;
}

PROPHYSICS_API bool ProPhysics_Measure_Spin(
    const ProUniverse* pu, uint64_t node, int which_spinor,
    double* out_sx, double* out_sy, double* out_sz)
{
    if (!pu || !pu->amp_grid) return false;
    if (node >= pu->total_nodes) return false;
    if (which_spinor < 0 || which_spinor > 1) return false;

    const int iu = PRO_SPINOR_UP[which_spinor];
    const int id = PRO_SPINOR_DN[which_spinor];

    const double inv = 1.0 / 2147483647.0;
    const double ar = (double)pro_amp_real(pu->amp_grid[node].coeff[iu]) * inv;
    const double ai = (double)pro_amp_imag(pu->amp_grid[node].coeff[iu]) * inv;
    const double br = (double)pro_amp_real(pu->amp_grid[node].coeff[id]) * inv;
    const double bi = (double)pro_amp_imag(pu->amp_grid[node].coeff[id]) * inv;

    const double norm = ar * ar + ai * ai + br * br + bi * bi;
    if (norm < 1e-20) {
        if (out_sx) *out_sx = 0.0;
        if (out_sy) *out_sy = 0.0;
        if (out_sz) *out_sz = 0.0;
        return true;
    }

    if (out_sx) *out_sx = (ar * br + ai * bi) / norm;
    if (out_sy) *out_sy = (ar * bi - ai * br) / norm;
    if (out_sz) *out_sz = 0.5 * (ar * ar + ai * ai - br * br - bi * bi) / norm;
    return true;
}

PROPHYSICS_API double ProPhysics_Verify_SU2_Algebra(void)
{
    /* sigma_i in 2x2 komplexer Darstellung, doppelt (Re, Im). */
    const double sx[2][2][2] = { {{0,0},{1,0}}, {{1,0},{0,0}} };
    const double sy[2][2][2] = { {{0,0},{0,-1}}, {{0,1},{0,0}} };
    const double sz[2][2][2] = { {{1,0},{0,0}}, {{0,0},{-1,0}} };
    const double (*s[3])[2][2] = { sx, sy, sz };

    double eps[3][3][3] = { {{0}} };
    eps[0][1][2] = eps[1][2][0] = eps[2][0][1] = 1.0;
    eps[0][2][1] = eps[2][1][0] = eps[1][0][2] = -1.0;

    double max_err = 0.0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int a = 0; a < 2; ++a)
                for (int b = 0; b < 2; ++b) {
                    double lhs_re = 0, lhs_im = 0;
                    for (int c2 = 0; c2 < 2; ++c2) {
                        lhs_re += s[i][a][c2][0] * s[j][c2][b][0]
                            - s[i][a][c2][1] * s[j][c2][b][1];
                        lhs_im += s[i][a][c2][0] * s[j][c2][b][1]
                            + s[i][a][c2][1] * s[j][c2][b][0];
                        lhs_re -= s[j][a][c2][0] * s[i][c2][b][0]
                            - s[j][a][c2][1] * s[i][c2][b][1];
                        lhs_im -= s[j][a][c2][0] * s[i][c2][b][1]
                            + s[j][a][c2][1] * s[i][c2][b][0];
                    }
                    double rhs_re = 0, rhs_im = 0;
                    for (int k = 0; k < 3; ++k) {
                        const double coef = 2.0 * eps[i][j][k];
                        rhs_re += coef * (-s[k][a][b][1]);
                        rhs_im += coef * (s[k][a][b][0]);
                    }
                    const double dr = lhs_re - rhs_re;
                    const double di = lhs_im - rhs_im;
                    const double err = sqrt(dr * dr + di * di);
                    if (err > max_err) max_err = err;
                }
    return max_err;
}

/* ==========================================================================
 * Fermionen / Antisymmetrie
 * ========================================================================== */

static inline void pro_tensor_get(const ProAmpTensorPair* p,
    int a, int b, double* out_re, double* out_im)
{
    const ProAmpQ31 c = p->coeff[pro_tensor_idx(a, b)];
    *out_re = (double)pro_amp_real(c);
    *out_im = (double)pro_amp_imag(c);
}

static inline void pro_tensor_set(ProAmpTensorPair* p,
    int a, int b, double re, double im)
{
    p->coeff[pro_tensor_idx(a, b)] =
        pro_amp_pack(pro_sat_i32((int64_t)llround(re)),
            pro_sat_i32((int64_t)llround(im)));
}

PROPHYSICS_API bool ProPhysics_Tensor_Is_Antisymmetric(
    const ProUniverse* pu, uint32_t pair_id, int32_t tol_q31)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    if (pu->tensor_pairs[pair_id].active == 0u) return false;

    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    const double tol = (double)tol_q31;

    double norm_sq = 0.0;
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            const ProAmpQ31 c = p->coeff[pro_tensor_idx(a, b)];
            const double re = (double)pro_amp_real(c);
            const double im = (double)pro_amp_imag(c);
            norm_sq += re * re + im * im;
        }
    }
    if (norm_sq < 1.0) return true;

    const double tol_sq = tol * tol * norm_sq;

    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            const ProAmpQ31 cab = p->coeff[pro_tensor_idx(a, b)];
            const ProAmpQ31 cba = p->coeff[pro_tensor_idx(b, a)];
            const double dre = (double)pro_amp_real(cab)
                + (double)pro_amp_real(cba);
            const double dim = (double)pro_amp_imag(cab)
                + (double)pro_amp_imag(cba);
            if (dre * dre + dim * dim > tol_sq) return false;
        }
    }
    return true;
}

PROPHYSICS_API double ProPhysics_Tensor_Pauli_Violation(
    const ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return 0.0;
    if (pair_id >= pu->tensor_pair_capacity) return 0.0;
    if (pu->tensor_pairs[pair_id].active == 0u) return 0.0;

    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];

    double diag_sq = 0.0;
    double total_sq = 0.0;
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            const ProAmpQ31 c = p->coeff[pro_tensor_idx(a, b)];
            const double re = (double)pro_amp_real(c);
            const double im = (double)pro_amp_imag(c);
            const double sq = re * re + im * im;
            total_sq += sq;
            if (a == b) diag_sq += sq;
        }
    }
    if (total_sq < 1.0) return 0.0;
    return diag_sq / total_sq;
}

PROPHYSICS_API double ProPhysics_Tensor_Antisymmetrize(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return 0.0;
    if (pair_id >= pu->tensor_pair_capacity) return 0.0;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return 0.0;

    double norm_sq_out = 0.0;

    double orig_re[8][8], orig_im[8][8];
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            pro_tensor_get(p, a, b, &orig_re[a][b], &orig_im[a][b]);
        }
    }

    for (int a = 0; a < 8; ++a) {
        pro_tensor_set(p, a, a, 0.0, 0.0);

        for (int b = a + 1; b < 8; ++b) {
            const double re_ab = (orig_re[a][b] - orig_re[b][a]) * 0.5;
            const double im_ab = (orig_im[a][b] - orig_im[b][a]) * 0.5;
            pro_tensor_set(p, a, b, re_ab, im_ab);
            pro_tensor_set(p, b, a, -re_ab, -im_ab);
            norm_sq_out += 2.0 * (re_ab * re_ab + im_ab * im_ab);
        }
    }
    return norm_sq_out / 4611686018427387904.0;
}

PROPHYSICS_API bool ProPhysics_Tensor_Fermionize(
    ProUniverse* pu, uint32_t pair_id)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;

    const double norm_sq = ProPhysics_Tensor_Antisymmetrize(pu, pair_id);
    if (norm_sq < 1e-20) return false;

    const double scale = 1.0 / sqrt(norm_sq);
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            double re, im;
            pro_tensor_get(p, a, b, &re, &im);
            pro_tensor_set(p, a, b, re * scale, im * scale);
        }
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Set_Slater(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_a, uint8_t orbital_b)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;
    if (orbital_a >= 8u || orbital_b >= 8u) return false;
    if (orbital_a == orbital_b) return false;

    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) p->coeff[i] = 0;

    const int32_t amp = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    p->coeff[pro_tensor_idx(orbital_a, orbital_b)] = pro_amp_pack(amp, 0);
    p->coeff[pro_tensor_idx(orbital_b, orbital_a)] = pro_amp_pack(-amp, 0);
    return true;
}

/* ==========================================================================
 * Fermionisches Hopping
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Tensor_Apply_Hopping(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;
    if (orbital_i >= 8u || orbital_j >= 8u) return false;
    if (orbital_i == orbital_j) return false;
    if (theta_q15 == 0) return true;

    const double theta = (double)theta_q15 / 32768.0;
    const double c = cos(theta);
    const double s = sin(theta);

    const int i = (int)orbital_i;
    const int j = (int)orbital_j;

    for (int b = 0; b < 8; ++b) {
        if (b == i || b == j) continue;

        const int ib_min = (i < b) ? i : b;
        const int ib_max = (i < b) ? b : i;
        const int jb_min = (j < b) ? j : b;
        const int jb_max = (j < b) ? b : j;

        const int p_ib = ib_min * 8 + ib_max;
        const int p_jb = jb_min * 8 + jb_max;

        const int sgn = ((b - i) * (b - j) > 0) ? +1 : -1;
        const double s_signed = s * (double)sgn;

        const int64_t a_ib_re = (int64_t)pro_amp_real(p->coeff[p_ib]);
        const int64_t a_ib_im = (int64_t)pro_amp_imag(p->coeff[p_ib]);
        const int64_t a_jb_re = (int64_t)pro_amp_real(p->coeff[p_jb]);
        const int64_t a_jb_im = (int64_t)pro_amp_imag(p->coeff[p_jb]);

        const double new_ib_re = c * (double)a_ib_re + s_signed * (double)a_jb_im;
        const double new_ib_im = c * (double)a_ib_im - s_signed * (double)a_jb_re;
        const double new_jb_re = c * (double)a_jb_re + s_signed * (double)a_ib_im;
        const double new_jb_im = c * (double)a_jb_im - s_signed * (double)a_ib_re;

        const ProAmpQ31 new_ib = pro_amp_pack(
            pro_sat_i32((int64_t)llround(new_ib_re)),
            pro_sat_i32((int64_t)llround(new_ib_im)));
        const ProAmpQ31 new_jb = pro_amp_pack(
            pro_sat_i32((int64_t)llround(new_jb_re)),
            pro_sat_i32((int64_t)llround(new_jb_im)));
        p->coeff[p_ib] = new_ib;
        p->coeff[p_jb] = new_jb;

        const int m_ib = ib_max * 8 + ib_min;
        const int m_jb = jb_max * 8 + jb_min;
        p->coeff[m_ib] = pro_amp_pack(-pro_amp_real(new_ib),
            -pro_amp_imag(new_ib));
        p->coeff[m_jb] = pro_amp_pack(-pro_amp_real(new_jb),
            -pro_amp_imag(new_jb));
    }
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Set_Hopping(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;
    if (orbital_i >= 8u || orbital_j >= 8u) return false;
    if (orbital_i == orbital_j) return false;

    p->hopping_i = orbital_i;
    p->hopping_j = orbital_j;
    p->hopping_theta_q15 = theta_q15;
    return true;
}

PROPHYSICS_API bool ProPhysics_Tensor_Get_Hopping(
    const ProUniverse* pu, uint32_t pair_id,
    uint8_t* out_i, uint8_t* out_j, int32_t* out_theta_q15)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;

    if (out_i) *out_i = p->hopping_i;
    if (out_j) *out_j = p->hopping_j;
    if (out_theta_q15) *out_theta_q15 = p->hopping_theta_q15;
    return true;
}

/* ==========================================================================
 * Tensor <-> Fock Adapter
 *
 * Konvention (norm-erhaltend):
 *   Tensor Slater(i,j)  <->  Fock |i,j>  mit bits = (1<<i)|(1<<j)
 *   fock[bits(i,j)] = sqrt(2) * tensor[i*8 + j]
 *   tensor[i*8+j] = +fock[bits]/sqrt(2)
 *   tensor[j*8+i] = -fock[bits]/sqrt(2)
 *
 * Randbedingungen:
 *   Tensor_To_Fock: Tensor muss antisymmetrisch sein.
 *   Fock_To_Tensor: Fock-State darf NUR Popcounts = 2 haben.
 * ========================================================================== */

static inline uint8_t pro_pair_to_bits(int i, int j)
{
    return (uint8_t)((1u << i) | (1u << j));
}

PROPHYSICS_API bool ProPhysics_Tensor_To_Fock(
    ProUniverse* pu, uint32_t pair_id, uint64_t* out_fock_id)
{
    if (!pu || !pu->tensor_pairs) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;
    const ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;

    if (!ProPhysics_Tensor_Is_Antisymmetric(pu, pair_id, 1000)) return false;

    uint64_t fid = 0;
    if (!ProPhysics_Fock_Create(pu, &fid)) return false;
    ProFockState* f = &pu->fock_states[fid];

    for (uint32_t k = 0; k < PRO_FOCK_DIM; ++k) f->coeff[k] = 0;

    const double inv_sqrt2 = 0.7071067811865476;

    for (int i = 0; i < 8; ++i) {
        for (int j = i + 1; j < 8; ++j) {
            const ProAmpQ31 c = p->coeff[i * 8 + j];
            const int32_t re = pro_amp_real(c);
            const int32_t im = pro_amp_imag(c);
            if (re == 0 && im == 0) continue;

            const double new_re = (double)re * inv_sqrt2 * 2.0;
            const double new_im = (double)im * inv_sqrt2 * 2.0;

            const uint8_t bits = pro_pair_to_bits(i, j);
            f->coeff[bits] = pro_amp_pack(
                pro_sat_i32((int64_t)llround(new_re)),
                pro_sat_i32((int64_t)llround(new_im)));
        }
    }

    if (out_fock_id) *out_fock_id = fid;
    return true;
}

PROPHYSICS_API bool ProPhysics_Fock_To_Tensor(
    const ProUniverse* pu, uint64_t fock_id, uint32_t pair_id)
{
    if (!pu || !pu->fock_states || !pu->tensor_pairs) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    if (pair_id >= pu->tensor_pair_capacity) return false;

    const ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;

    ProAmpTensorPair* p = &pu->tensor_pairs[pair_id];
    if (p->active == 0u) return false;

    for (uint32_t bits = 0; bits < PRO_FOCK_DIM; ++bits) {
        if (f->coeff[bits] == 0) continue;
        if (ProPhysics_Fock_Popcount((uint8_t)bits) != 2u) return false;
    }

    for (uint32_t k = 0; k < PRO_TENSOR_DIM; ++k) p->coeff[k] = 0;

    const double inv_sqrt2 = 0.7071067811865476;

    for (uint32_t bits = 0; bits < PRO_FOCK_DIM; ++bits) {
        const ProAmpQ31 c = f->coeff[bits];
        if (c == 0) continue;

        int i = -1, j = -1;
        for (int b = 0; b < 8; ++b) {
            if (bits & (1u << b)) {
                if (i < 0) i = b;
                else        j = b;
            }
        }
        if (i < 0 || j < 0 || i == j) return false;

        const double re = (double)pro_amp_real(c) * inv_sqrt2;
        const double im = (double)pro_amp_imag(c) * inv_sqrt2;

        const ProAmpQ31 v_pos = pro_amp_pack(
            pro_sat_i32((int64_t)llround(re)),
            pro_sat_i32((int64_t)llround(im)));
        const ProAmpQ31 v_neg = pro_amp_pack(-pro_amp_real(v_pos),
            -pro_amp_imag(v_pos));

        p->coeff[i * 8 + j] = v_pos;
        p->coeff[j * 8 + i] = v_neg;
    }
    return true;
}

/* ==========================================================================
 * End of ProPhysics_Tensor.c
 * ========================================================================== */