/* ==========================================================================
 * ProPhysics - Dirac Modul
 * File: ProPhysics_Dirac.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * 4-Komponenten-Spinor {psi_L_up, psi_L_dn, psi_R_up, psi_R_dn}
 * in Basis 1..4.
 *
 * Prinzip (wie der ganze Kernel): signed permutations auf einer
 * 4-dim Basis, plus lokale 2-Knoten-Rotation fuer den Massenterm.
 * KEINE Matrixmultiplikation im Hotpath.
 *
 * Interne Helfer:
 *   - pro_gamma_op_init (Gamma-Init-Block, ersetzt 16x Wiederholung)
 *   - pro_dirac_one / neg_one / pos_i / neg_i (Q31-Konstanten)
 *   - Gamma- und alpha-Tabellen (Dirac + Weyl)
 *   - pro_dirac_apply_op, pro_dirac_transport_2node_with_alpha
 *   - pro_dirac_transport_3d
 *
 * Wiederverwendung:
 *   - pro_amp_mul_q31, pro_amp_add_q31 (ProPhysics_Types.h)
 *   - pro_transport_coeffs             (ProPhysics_Amp.c)
 *   - pro_round_shift_q31, pro_div_round, pro_sat_i32 (Internal.h)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/Dirac.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Gamma-Operationstabellen
  *
  * Jede gamma-Matrix wird als signed-complex permutation dargestellt:
  *   new[i] = coeff[i] * old[perm[i]]
  *
  * coeff[i] ist ein komplexer Q31-Wert aus {+1, -1, +i, -i}.
  * ========================================================================== */

typedef struct {
    uint8_t   perm[PRO_DIRAC_DIM];
    ProAmpQ31 coeff[PRO_DIRAC_DIM];
} ProGammaOp;

/* Q31-Konstanten fuer +-1 und +-i. */
static inline ProAmpQ31 pro_dirac_one(void) { return pro_amp_pack(INT32_MAX, 0); }
static inline ProAmpQ31 pro_dirac_neg_one(void) { return pro_amp_pack(-INT32_MAX, 0); }
static inline ProAmpQ31 pro_dirac_pos_i(void) { return pro_amp_pack(0, INT32_MAX); }
static inline ProAmpQ31 pro_dirac_neg_i(void) { return pro_amp_pack(0, -INT32_MAX); }

/* Initialisiert eine ProGammaOp mit Permutation und Koeffizienten.
 *
 * Ersetzt 16x wiederholte 8-Zeilen-Bloecke
 *   op->perm[0]=p0; op->perm[1]=p1; ...
 *   op->coeff[0]=c0; op->coeff[1]=c1; ... */
static void pro_gamma_op_init(ProGammaOp* op,
    uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3,
    ProAmpQ31 c0, ProAmpQ31 c1, ProAmpQ31 c2, ProAmpQ31 c3)
{
    op->perm[0] = p0; op->perm[1] = p1;
    op->perm[2] = p2; op->perm[3] = p3;
    op->coeff[0] = c0; op->coeff[1] = c1;
    op->coeff[2] = c2; op->coeff[3] = c3;
}

/* ==========================================================================
 * Dirac-Darstellung
 *
 *   gamma^0 = diag(+1, +1, -1, -1)
 *   gamma^1 = [[0, sx], [-sx, 0]]
 *   gamma^2 = [[0, sy], [-sy, 0]]
 *   gamma^3 = [[0, sz], [-sz, 0]]
 *   gamma^5 = i*gamma^0*gamma^1*gamma^2*gamma^3 = [[0, I], [I, 0]]
 *
 * Komponenten-Reihenfolge: (L_up, L_dn, R_up, R_dn)
 * Index 0 = gamma^0, 1 = gamma^1, 2 = gamma^2, 3 = gamma^3, 4 = gamma^5
 * ========================================================================== */

static ProGammaOp g_gamma_dirac[5];

static void pro_dirac_init_dirac_basis(void)
{
    const ProAmpQ31 one = pro_dirac_one();
    const ProAmpQ31 m1 = pro_dirac_neg_one();
    const ProAmpQ31 ppi = pro_dirac_pos_i();
    const ProAmpQ31 pni = pro_dirac_neg_i();

    /* gamma^0: Identitaet auf Index, Vorzeichen auf R. */
    pro_gamma_op_init(&g_gamma_dirac[0], 0, 1, 2, 3,
        one, one, m1, m1);
    /* gamma^1: Swap L<->R mit Spin-Swap, Vorzeichen auf neuem R. */
    pro_gamma_op_init(&g_gamma_dirac[1], 3, 2, 1, 0,
        one, one, m1, m1);
    /* gamma^2: wie gamma^1, aber mit +-i. */
    pro_gamma_op_init(&g_gamma_dirac[2], 3, 2, 1, 0,
        pni, ppi, ppi, pni);
    /* gamma^3: Swap L<->R, Vorzeichen-Wechsel auf L_dn, R_up. */
    pro_gamma_op_init(&g_gamma_dirac[3], 2, 3, 0, 1,
        one, m1, m1, one);
    /* gamma^5: Chirality-Flip, keine Vorzeichen. */
    pro_gamma_op_init(&g_gamma_dirac[4], 2, 3, 0, 1,
        one, one, one, one);
}

/* ==========================================================================
 * Weyl-Darstellung (chiral)
 *
 *   gamma^0 = [[0, I], [I, 0]]      (Chirality-Flip)
 *   gamma^1 = [[0, sx], [-sx, 0]]   (identisch zu Dirac)
 *   gamma^2 = [[0, sy], [-sy, 0]]   (identisch zu Dirac)
 *   gamma^3 = [[0, sz], [-sz, 0]]   (identisch zu Dirac)
 *   gamma^5 = diag(-I, -I, +I, +I)  (Chirality-Vorzeichen)
 * ========================================================================== */

static ProGammaOp g_gamma_weyl[5];

static void pro_dirac_init_weyl_basis(void)
{
    const ProAmpQ31 one = pro_dirac_one();
    const ProAmpQ31 m1 = pro_dirac_neg_one();

    /* gamma^0_Weyl = gamma^5_Dirac. */
    pro_gamma_op_init(&g_gamma_weyl[0], 2, 3, 0, 1,
        one, one, one, one);

    /* gamma^1..3_Weyl = identisch zu Dirac. */
    g_gamma_weyl[1] = g_gamma_dirac[1];
    g_gamma_weyl[2] = g_gamma_dirac[2];
    g_gamma_weyl[3] = g_gamma_dirac[3];

    /* gamma^5_Weyl = diag(-1, -1, +1, +1). */
    pro_gamma_op_init(&g_gamma_weyl[4], 0, 1, 2, 3,
        m1, m1, one, one);
}

/* ==========================================================================
 * Dirac-Matrizen alpha^i = gamma^0 gamma^i (hermitesch, selbstinvers)
 *
 * Sie sind die "richtigen" Transport-Generatoren fuer die Dirac-Gleichung:
 *   d_t psi = -(alpha^1 d_x + alpha^2 d_y + alpha^3 d_z) psi
 *
 * Index 0 = alpha^1 (x-Richtung), 1 = alpha^2 (y-Richtung),
 * 2 = alpha^3 (z-Richtung).
 * ========================================================================== */

static ProGammaOp g_alpha_dirac[3];
static ProGammaOp g_alpha_weyl[3];

static void pro_dirac_init_alpha_tables(void)
{
    const ProAmpQ31 one = pro_dirac_one();
    const ProAmpQ31 m1 = pro_dirac_neg_one();
    const ProAmpQ31 ppi = pro_dirac_pos_i();
    const ProAmpQ31 pni = pro_dirac_neg_i();

    /* --- Dirac-Darstellung --- */
    pro_gamma_op_init(&g_alpha_dirac[0], 3, 2, 1, 0,
        one, one, one, one);
    pro_gamma_op_init(&g_alpha_dirac[1], 3, 2, 1, 0,
        pni, ppi, pni, ppi);
    pro_gamma_op_init(&g_alpha_dirac[2], 2, 3, 0, 1,
        one, m1, one, m1);

    /* --- Weyl-Darstellung --- */
    pro_gamma_op_init(&g_alpha_weyl[0], 1, 0, 3, 2,
        m1, m1, one, one);
    pro_gamma_op_init(&g_alpha_weyl[1], 1, 0, 3, 2,
        pni, ppi, ppi, pni);
    pro_gamma_op_init(&g_alpha_weyl[2], 0, 1, 2, 3,
        m1, one, one, m1);
}

/* Lazy-Init aller Tabellen. Idempotent. */
static void pro_dirac_init_once(void)
{
    static bool s_ready = false;
    if (s_ready) return;
    pro_dirac_init_dirac_basis();
    pro_dirac_init_weyl_basis();
    pro_dirac_init_alpha_tables();
    s_ready = true;
}

/* Auswahl der aktuellen gamma-Tabelle. */
static const ProGammaOp* pro_dirac_table(uint8_t basis, int index)
{
    pro_dirac_init_once();
    if (basis == PRO_GAMMA_BASIS_WEYL) return &g_gamma_weyl[index];
    return &g_gamma_dirac[index];
}

/* Auswahl der aktuellen alpha^i-Tabelle.
 * direction: 0 = x, 1 = y, 2 = z. */
static const ProGammaOp* pro_dirac_alpha(uint8_t basis, int direction)
{
    pro_dirac_init_once();
    if (basis == PRO_GAMMA_BASIS_WEYL) return &g_alpha_weyl[direction];
    return &g_alpha_dirac[direction];
}

/* ==========================================================================
 * Gamma-Algebra-Verifikation
 *
 * Prueft {gamma^mu, gamma^nu} = 2 eta^mu_nu * I mit
 * eta = diag(+1, -1, -1, -1) fuer mu, nu in 0..3.
 * Zusaetzlich {gamma^5, gamma^mu} = 0 und (gamma^5)^2 = I.
 * ========================================================================== */

static void pro_dirac_expand_matrix(const ProGammaOp* op,
    ProAmpQ31 dst[4][4])
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            dst[i][j] = 0;
        }
    }
    for (int i = 0; i < 4; ++i) {
        dst[i][op->perm[i]] = op->coeff[i];
    }
}

PROPHYSICS_API double ProPhysics_Verify_Gamma_Algebra(void)
{
    pro_dirac_init_once();

    /* Zielmetrik eta^mu_nu = diag(+1, -1, -1, -1, +1) fuer Indizes 0..4. */
    const double eta_diag[5] = { +1.0, -1.0, -1.0, -1.0, +1.0 };

    ProAmpQ31 gamma[5][4][4];
    for (int i = 0; i < 5; ++i) {
        pro_dirac_expand_matrix(&g_gamma_dirac[i], gamma[i]);
    }

    double max_err = 0.0;

    for (int mu = 0; mu < 5; ++mu) {
        for (int nu = 0; nu < 5; ++nu) {
            const double target_re = 2.0 * eta_diag[mu] *
                ((mu == nu) ? 1.0 : 0.0);

            for (int a = 0; a < 4; ++a) {
                for (int b = 0; b < 4; ++b) {
                    int64_t sum_re = 0;
                    int64_t sum_im = 0;

                    for (int c = 0; c < 4; ++c) {
                        const int64_t ar = pro_amp_real(gamma[mu][a][c]);
                        const int64_t ai = pro_amp_imag(gamma[mu][a][c]);
                        const int64_t br = pro_amp_real(gamma[nu][c][b]);
                        const int64_t bi = pro_amp_imag(gamma[nu][c][b]);

                        sum_re += pro_round_shift_q31(ar * br - ai * bi);
                        sum_im += pro_round_shift_q31(ar * bi + ai * br);
                    }
                    for (int c = 0; c < 4; ++c) {
                        const int64_t ar = pro_amp_real(gamma[nu][a][c]);
                        const int64_t ai = pro_amp_imag(gamma[nu][a][c]);
                        const int64_t br = pro_amp_real(gamma[mu][c][b]);
                        const int64_t bi = pro_amp_imag(gamma[mu][c][b]);

                        sum_re += pro_round_shift_q31(ar * br - ai * bi);
                        sum_im += pro_round_shift_q31(ar * bi + ai * br);
                    }

                    const double got_re = (double)sum_re / 2147483647.0;
                    const double got_im = (double)sum_im / 2147483647.0;

                    const double exp_re = (a == b) ? target_re : 0.0;
                    const double exp_im = 0.0;

                    const double dr = got_re - exp_re;
                    const double di = got_im - exp_im;
                    const double err = sqrt(dr * dr + di * di);

                    if (err > max_err) max_err = err;
                }
            }
        }
    }

    return max_err;
}

/* ==========================================================================
 * Anwendung einer ProGammaOp auf einen Amplitudenvektor
 *
 * out[i] = coeff[i] * in[perm[i]]  fuer i in 0..3
 *
 * Nur die 4 Dirac-Komponenten (Basis 1..4) werden transformiert.
 * ========================================================================== */

static void pro_dirac_apply_op(const ProGammaOp* op,
    const ProAmpVector* in,
    ProAmpQ31 out[PRO_DIRAC_DIM])
{
    for (int i = 0; i < PRO_DIRAC_DIM; ++i) {
        const uint8_t basis_src = PRO_DIRAC_TO_BASIS(op->perm[i]);
        out[i] = pro_amp_mul_q31(op->coeff[i], in->coeff[basis_src]);
    }
}

/* ==========================================================================
 * 2-Knoten-Rotation mit alpha-Kopplung
 *
 *   a' = c*a + s*(alpha*b)
 *   b' = c*b - s*(alpha*a)
 *
 * Nur die 4 Dirac-Komponenten (Basis 1..4) werden transformiert.
 * Basis-Zustaende 0, 5, 6, 7 bleiben unveraendert.
 * ========================================================================== */

static void pro_dirac_transport_2node_with_alpha(
    ProAmpVector* va, ProAmpVector* vb,
    const ProGammaOp* alpha,
    int64_t c_num, int64_t s_num, int64_t denom)
{
    ProAmpQ31 alpha_va[PRO_DIRAC_DIM];
    ProAmpQ31 alpha_vb[PRO_DIRAC_DIM];

    pro_dirac_apply_op(alpha, va, alpha_va);
    pro_dirac_apply_op(alpha, vb, alpha_vb);

    for (int i = 0; i < PRO_DIRAC_DIM; ++i) {
        const uint8_t basis = PRO_DIRAC_TO_BASIS((uint8_t)i);

        const int64_t a_re = (int64_t)pro_amp_real(va->coeff[basis]);
        const int64_t a_im = (int64_t)pro_amp_imag(va->coeff[basis]);
        const int64_t b_re = (int64_t)pro_amp_real(vb->coeff[basis]);
        const int64_t b_im = (int64_t)pro_amp_imag(vb->coeff[basis]);

        const int64_t av_re = (int64_t)pro_amp_real(alpha_va[i]);
        const int64_t av_im = (int64_t)pro_amp_imag(alpha_va[i]);
        const int64_t bv_re = (int64_t)pro_amp_real(alpha_vb[i]);
        const int64_t bv_im = (int64_t)pro_amp_imag(alpha_vb[i]);

        const int64_t new_a_re = pro_div_round(c_num * a_re + s_num * bv_re, denom);
        const int64_t new_a_im = pro_div_round(c_num * a_im + s_num * bv_im, denom);
        const int64_t new_b_re = pro_div_round(c_num * b_re - s_num * av_re, denom);
        const int64_t new_b_im = pro_div_round(c_num * b_im - s_num * av_im, denom);

        va->coeff[basis] = pro_amp_pack(pro_sat_i32(new_a_re),
            pro_sat_i32(new_a_im));
        vb->coeff[basis] = pro_amp_pack(pro_sat_i32(new_b_re),
            pro_sat_i32(new_b_im));
    }
}

/* ==========================================================================
 * Dirac-Transport 3D mit alpha^i-Kopplung
 *
 * Ersetzt ProPhysics_Apply_Edge_Transport_Colored_3D im Dirac-Pfad.
 *
 * Sweep-Reihenfolge identisch zum Standard-3D-Transport:
 *   0,1: x-Richtung (Kanal 0)
 *   2,3: y-Richtung (Kanal 2)
 *   4,5: z-Richtung (Kanal 4)
 *
 * Paritaetsmuster: (x + y + z) & 1.
 * ========================================================================== */

static void pro_dirac_transport_3d(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim)
{
    if (!pu || !pu->amp_grid || !pu->amp_scratch) return;
    if (theta_q15 == 0u || grid_dim == 0u) return;
    if ((grid_dim & (grid_dim - 1u)) != 0u) return;

    int64_t c_num = 0, s_num = 0, denom = 1;
    pro_transport_coeffs(pu, theta_q15, &c_num, &s_num, &denom);

    uint32_t dim_shift = 0u;
    while ((1u << dim_shift) < grid_dim) dim_shift++;
    const uint32_t dim_mask = grid_dim - 1u;

    memcpy(pu->amp_scratch, pu->amp_grid,
        (size_t)pu->total_nodes * sizeof(ProAmpVector));
    ProAmpVector* buf = pu->amp_scratch;

    static const uint8_t sweep_channel[6] = { 0u, 0u, 2u, 2u, 4u, 4u };
    static const int     sweep_direction[6] = { 0, 0, 1, 1, 2, 2 };

    for (uint8_t sweep = 0; sweep < 6u; ++sweep) {
        const uint8_t  channel = sweep_channel[sweep];
        const uint32_t parity = (uint32_t)(sweep & 1u);
        const ProGammaOp* alpha = pro_dirac_alpha(
            pu->dirac_gamma_basis, sweep_direction[sweep]);

        for (uint64_t k = 0; k < pu->total_nodes; ++k) {
            const uint32_t x = (uint32_t)k & dim_mask;
            const uint32_t y = ((uint32_t)k >> dim_shift) & dim_mask;
            const uint32_t z = ((uint32_t)k >> (2u * dim_shift)) & dim_mask;
            if (((x + y + z) & 1u) != parity) continue;

            const uint64_t nb = pu->reg_source[k].channels[channel];
            if (nb >= pu->total_nodes || nb == k) continue;

            pro_dirac_transport_2node_with_alpha(&buf[k], &buf[nb],
                alpha, c_num, s_num, denom);
        }
    }

    ProAmpVector* tmp = pu->amp_grid;
    pu->amp_grid = pu->amp_scratch;
    pu->amp_scratch = tmp;
}

/* ==========================================================================
 * Massenterm (lokale L<->R-Rotation)
 *
 *   psi_L' = c*psi_L + s*psi_R
 *   psi_R' = c*psi_R - s*psi_L
 *
 * mit c = cos(m*dt), s = sin(m*dt), rationale Koeffizienten via
 * pro_transport_coeffs (geteilter Cache mit dem Transport).
 *
 * Nur Knoten mit PRO_NODE_DIRAC_BIT werden transformiert.
 *
 * Wichtig: Das Vorzeichen von mass_q15 wird auf s_num angewendet.
 * c_num und denom bleiben positiv (Symmetrie der Rotation). */
PROPHYSICS_API void ProPhysics_Apply_Dirac_Mass_Term(
    ProUniverse* pu,
    int32_t mass_q15)
{
    if (!pu || !pu->amp_grid) return;
    if (mass_q15 == 0) return;

    const uint32_t theta_q15 =
        (uint32_t)((mass_q15 < 0) ? -mass_q15 : mass_q15);

    int64_t c_num = 0, s_num = 0, denom = 1;
    pro_transport_coeffs(pu, theta_q15, &c_num, &s_num, &denom);

    if (mass_q15 < 0) {
        s_num = -s_num;
    }

    const uint64_t n = pu->total_nodes;

    for (uint64_t k = 0; k < n; ++k) {
        if (pu->ur_grid &&
            (pu->ur_grid[k].reserved_gating & PRO_NODE_DIRAC_BIT) == 0u) {
            continue;
        }

        ProAmpVector* v = &pu->amp_grid[k];

        for (int spin = 0; spin < 2; ++spin) {
            const uint8_t l_idx = (spin == 0) ? 1u : 2u;   /* L_up, L_dn */
            const uint8_t r_idx = (spin == 0) ? 3u : 4u;   /* R_up, R_dn */

            const int64_t l_re = (int64_t)pro_amp_real(v->coeff[l_idx]);
            const int64_t l_im = (int64_t)pro_amp_imag(v->coeff[l_idx]);
            const int64_t r_re = (int64_t)pro_amp_real(v->coeff[r_idx]);
            const int64_t r_im = (int64_t)pro_amp_imag(v->coeff[r_idx]);

            const int64_t new_l_re = pro_div_round(c_num * l_re - s_num * r_im, denom);
            const int64_t new_l_im = pro_div_round(c_num * l_im + s_num * r_re, denom);
            const int64_t new_r_re = pro_div_round(c_num * r_re - s_num * l_im, denom);
            const int64_t new_r_im = pro_div_round(c_num * r_im + s_num * l_re, denom);

            v->coeff[l_idx] = pro_amp_pack(pro_sat_i32(new_l_re),
                pro_sat_i32(new_l_im));
            v->coeff[r_idx] = pro_amp_pack(pro_sat_i32(new_r_re),
                pro_sat_i32(new_r_im));
        }
    }
}

/* ==========================================================================
 * Dirac-Step (oeffentlicher Tick)
 *
 * Ein Tick Dirac-Evolution:
 *   1. Raeumlicher Anteil mit alpha^i-Kopplung.
 *      - 3D: pro_dirac_transport_3d (alpha^i pro Sweep-Richtung)
 *      - 2D/flach: Fallback auf Standard-Transport
 *   2. Massenterm: lokale L<->R-Rotation.
 *
 * Dispatch: wird von Apply_Amp_Step aufgerufen, wenn dirac_active == 1.
 * Bei dirac_active == 0 laeuft der Standardpfad bit-identisch (R7).
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Dirac_Step(
    ProUniverse* pu,
    int32_t  mass_q15,
    uint32_t theta_q15)
{
    if (!pu || !pu->amp_grid) return;

    /* --- 1. Raeumlicher Transport mit alpha^i-Kopplung --- */
    if (pu->grid_dim > 0u && pu->grid_ndim == 3u) {
        pro_dirac_transport_3d(pu, theta_q15, pu->grid_dim);
    }
    else if (pu->grid_dim > 0u) {
        ProPhysics_Apply_Edge_Transport_Colored(pu, theta_q15, pu->grid_dim);
    }
    else {
        ProPhysics_Apply_Edge_Transport(pu, theta_q15);
    }

    /* --- 2. Massenterm --- */
    if (mass_q15 != 0) {
        ProPhysics_Apply_Dirac_Mass_Term(pu, mass_q15);
    }
}

/* ==========================================================================
 * End of ProPhysics_Dirac.c
 * ========================================================================== */