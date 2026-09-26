/* ==========================================================================
 * ProPhysics - SU(2)-Eichfeld (Etappe 22)
 * File: ProPhysics_SU2.c
 *
 * Kernel: 1.23.0
 * Etappe: 22
 * 
 * Quaternion-Repraesentation eines SU(2)-Links:
 *
 *   U = [ a   b  ]       mit |a|^2 + |b|^2 = 1
 *       [-b*  a* ]
 *
 * Speicherung in int32 pro Komponente, Skala PRO_SU2_SCALE = 2^30:
 *   a = a_re + i*a_im      (a_re, a_im in [-2^30, 2^30])
 *   b = b_re + i*b_im
 *   Normierung |a|^2 + |b|^2 = PRO_SU2_NORM = 2^60.
 *
 * Warum Skala 2^30 statt 2^31?
 *   Bei 2^31 kann die Summe der vier Produktterme in a_new = a1*a2 - b1* * b2
 *   bis |2^62| + |2^62| = 2^63 gehen -- int64-Grenze. Bei 2^30 bleibt der
 *   Zwischenwert <= 4*2^60 = 2^62 (sicher).
 *
 * Wilson-Loop-Konvention (rueckwaerts / Path-Ordered):
 *   W(C) = U_{n-1} * U_{n-2} * ... * U_0
 *   mit U_k = Link(path_nodes[k] -> path_nodes[k+1]).
 *   Inversionssymmetrie W(C^-1) = W(C)^dagger ist damit erfuellt.
 *
 * R-Konformitaet:
 *   R1: keine div/mod in Hotpath (Bit-Shift statt /).
 *   R2: kein malloc (lokale Puffer, Stack).
 *   R3: U5 bleibt erhalten (nur Link-Metadaten, amp_grid unberuehrt).
 *   R7: bei su2_active == 0 liest niemand die su2-Felder.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer: SU(2)-Arithmetik in Skala 2^30.
  *
  * pro_su2_mul, pro_su2_conj, pro_su2_norm_sq sind ab Etappe 22b zentral
  * in ProPhysics_Internal.h definiert (static inline), damit auch
  * ProPhysics_SU2_Dynamics.c und ProPhysics_Amp.c sie nutzen koennen.
  * Hier bleibt nur die lokale Normalisierung pro_su2_normalize.
  * ========================================================================== */
static bool pro_su2_normalize(
    int32_t* a_re, int32_t* a_im, int32_t* b_re, int32_t* b_im)
{
    const uint64_t norm = pro_su2_norm_sq(*a_re, *a_im, *b_re, *b_im);
    if (norm == 0u) {
        /* Degeneriert: auf Identitaet zuruecksetzen. */
        *a_re = PRO_SU2_IDENT_RE;
        *a_im = PRO_SU2_IDENT_IM;
        *b_re = 0;
        *b_im = 0;
        return false;
    }

    const uint64_t target = (uint64_t)PRO_SU2_NORM;
    /* Wenn Norm schon 2^60 (Toleranz +-4), nichts tun. */
    if (norm >= target - 4u && norm <= target + 4u) return true;

    /* Skalierung per double. Nicht im Hotpath (1x pro Set_Edge_SU2). */
    const double scale = sqrt((double)target / (double)norm);
    double ar = (double)*a_re * scale;
    double ai = (double)*a_im * scale;
    double br = (double)*b_re * scale;
    double bi = (double)*b_im * scale;

    *a_re = pro_sat_i32((int64_t)llround(ar));
    *a_im = pro_sat_i32((int64_t)llround(ai));
    *b_re = pro_sat_i32((int64_t)llround(br));
    *b_im = pro_sat_i32((int64_t)llround(bi));
    return false;
}

/* ==========================================================================
 * Interne Helfer: Zugriff auf den Link-Slot.
 * ========================================================================== */

static ProEdge* pro_su2_edge_mut(ProUniverse* pu, uint64_t src, uint8_t ch)
{
    if (!pu || !pu->edge_phases) return NULL;
    if (src >= pu->total_nodes) return NULL;
    if (ch >= CHANNELS_MAX) return NULL;
    return &pu->edge_phases[src * (uint64_t)CHANNELS_MAX + ch];
}

static const ProEdge* pro_su2_edge(const ProUniverse* pu,
    uint64_t src, uint8_t ch)
{
    if (!pu || !pu->edge_phases) return NULL;
    if (src >= pu->total_nodes) return NULL;
    if (ch >= CHANNELS_MAX) return NULL;
    return &pu->edge_phases[src * (uint64_t)CHANNELS_MAX + ch];
}

/* ==========================================================================
 * Oeffentliche API
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Set_Edge_SU2(
    ProUniverse* pu, uint64_t src, uint8_t ch,
    int32_t a_re_q30, int32_t a_im_q30,
    int32_t b_re_q30, int32_t b_im_q30)
{
    ProEdge* e = pro_su2_edge_mut(pu, src, ch);
    if (!e) return false;

    (void)pro_su2_normalize(&a_re_q30, &a_im_q30, &b_re_q30, &b_im_q30);

    e->su2_a_re = a_re_q30;
    e->su2_a_im = a_im_q30;
    e->su2_b_re = b_re_q30;
    e->su2_b_im = b_im_q30;

    /* Auto-Aktivierung: erster Set-Aufruf schaltet SU(2)-Pfad global an. */
    pu->su2_active = 1u;
    return true;
}

PROPHYSICS_API bool ProPhysics_Set_Edge_SU2_AxisAngle(
    ProUniverse* pu, uint64_t src, uint8_t ch,
    double nx, double ny, double nz, double alpha)
{
    const double nrm = sqrt(nx * nx + ny * ny + nz * nz);
    if (nrm < 1e-12) return false;
    nx /= nrm; ny /= nrm; nz /= nrm;

    /* U = cos(alpha/2) * I - i * sin(alpha/2) * (n . sigma)
     *
     * In Quaternion-Form (a, b):
     *   a = cos(alpha/2) - i * sin(alpha/2) * nz
     *   b = -sin(alpha/2) * (ny + i*nx)  */
    const double c = cos(alpha * 0.5);
    const double s = sin(alpha * 0.5);

    const double S = (double)PRO_SU2_SCALE;
    const int32_t a_re = pro_sat_i32((int64_t)llround(c * S));
    const int32_t a_im = pro_sat_i32((int64_t)llround(-s * nz * S));
    const int32_t b_re = pro_sat_i32((int64_t)llround(-s * ny * S));
    const int32_t b_im = pro_sat_i32((int64_t)llround(-s * nx * S));

    return ProPhysics_Set_Edge_SU2(pu, src, ch, a_re, a_im, b_re, b_im);
}

PROPHYSICS_API bool ProPhysics_Get_Edge_SU2(
    const ProUniverse* pu, uint64_t src, uint8_t ch,
    int32_t* out_a_re_q30, int32_t* out_a_im_q30,
    int32_t* out_b_re_q30, int32_t* out_b_im_q30)
{
    const ProEdge* e = pro_su2_edge(pu, src, ch);
    if (!e) return false;
    if (out_a_re_q30) *out_a_re_q30 = e->su2_a_re;
    if (out_a_im_q30) *out_a_im_q30 = e->su2_a_im;
    if (out_b_re_q30) *out_b_re_q30 = e->su2_b_re;
    if (out_b_im_q30) *out_b_im_q30 = e->su2_b_im;
    return true;
}

PROPHYSICS_API bool ProPhysics_Wilson_Loop_SU2(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t* path_channels,
    uint32_t        path_len,
    int32_t* out_a_re_q30, int32_t* out_a_im_q30,
    int32_t* out_b_re_q30, int32_t* out_b_im_q30)
{
    if (!pro_wilson_validate_path(pu, path_nodes, path_channels, path_len))
        return false;

    /* Start: Identitaet. */
    int32_t a_re = PRO_SU2_IDENT_RE, a_im = PRO_SU2_IDENT_IM;
    int32_t b_re = 0, b_im = 0;

    /* Vorwaerts-Iteration (Standard-Lattice-QCD-Konvention):
     *   W(C) = U(p[0]->p[1]) * U(p[1]->p[2]) * ... * U(p[n-1]->p[0])
     * mit U_k = Link(path_nodes[k] -> path_nodes[k+1 mod n]).
     *
     * Diese Reihenfolge ist entscheidend fuer die Eichinvarianz:
     *   W' = g(p[0]) * W * g(p[0])^dagger
     *   Tr(W') = Tr(W).
     *
     * Die vorige Version (rueckwaerts) ist NICHT eichinvariant, weil die
     * inneren g-Faktoren dann nicht paarweise invers sind. */
    for (uint32_t k = 0; k < path_len; ++k) {
        const uint64_t src = path_nodes[k];
        const uint8_t  ch = path_channels[k];
        const ProEdge* e = pro_su2_edge(pu, src, ch);
        if (!e) return false;

        int32_t new_a_re, new_a_im, new_b_re, new_b_im;
        pro_su2_mul(
            a_re, a_im, b_re, b_im,                          /* akkumuliert links */
            e->su2_a_re, e->su2_a_im, e->su2_b_re, e->su2_b_im, /* U_k rechts */
            &new_a_re, &new_a_im, &new_b_re, &new_b_im);
        a_re = new_a_re; a_im = new_a_im;
        b_re = new_b_re; b_im = new_b_im;
    }

    if (out_a_re_q30) *out_a_re_q30 = a_re;
    if (out_a_im_q30) *out_a_im_q30 = a_im;
    if (out_b_re_q30) *out_b_re_q30 = b_re;
    if (out_b_im_q30) *out_b_im_q30 = b_im;
    return true;
}

PROPHYSICS_API double ProPhysics_Wilson_Loop_SU2_Trace(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t* path_channels,
    uint32_t        path_len)
{
    int32_t a_re = 0, a_im = 0, b_re = 0, b_im = 0;
    if (!ProPhysics_Wilson_Loop_SU2(pu, path_nodes, path_channels, path_len,
        &a_re, &a_im, &b_re, &b_im)) {
        return 0.0;
    }
    (void)a_im; (void)b_re; (void)b_im;
    /* Tr(U) = 2 * Re(a). Normiert auf 1.0 = Identitaet. */
    return (double)a_re / (double)PRO_SU2_SCALE;
}

PROPHYSICS_API void ProPhysics_Apply_Local_SU2_Gauge(
    ProUniverse* pu,
    const int32_t* lambda_q30)
{
    if (!pu || !pu->edge_phases || !pu->reg_source || !lambda_q30) return;
    if (pu->total_nodes == 0u) return;

    const uint64_t n = pu->total_nodes;

    for (uint64_t x = 0; x < n; ++x) {
        const ProRegister* r = &pu->reg_source[x];

        /* g(x) laden. */
        const int32_t gx_a_re = lambda_q30[x * 4u + 0u];
        const int32_t gx_a_im = lambda_q30[x * 4u + 1u];
        const int32_t gx_b_re = lambda_q30[x * 4u + 2u];
        const int32_t gx_b_im = lambda_q30[x * 4u + 3u];

        for (uint8_t ch = 0; ch < (uint8_t)CHANNELS_MAX; ++ch) {
            const uint64_t y = r->channels[ch];
            if (y >= n || y == x) continue;

            ProEdge* e = &pu->edge_phases[x * (uint64_t)CHANNELS_MAX + ch];

            /* g(y)-adjungiert. */
            int32_t gy_a_re, gy_a_im, gy_b_re, gy_b_im;
            pro_su2_conj(
                lambda_q30[y * 4u + 0u],
                lambda_q30[y * 4u + 1u],
                lambda_q30[y * 4u + 2u],
                lambda_q30[y * 4u + 3u],
                &gy_a_re, &gy_a_im, &gy_b_re, &gy_b_im);

            /* tmp = U * g(y)^dagger. */
            int32_t t_a_re, t_a_im, t_b_re, t_b_im;
            pro_su2_mul(
                e->su2_a_re, e->su2_a_im, e->su2_b_re, e->su2_b_im,
                gy_a_re, gy_a_im, gy_b_re, gy_b_im,
                &t_a_re, &t_a_im, &t_b_re, &t_b_im);

            /* U' = g(x) * tmp. */
            int32_t n_a_re, n_a_im, n_b_re, n_b_im;
            pro_su2_mul(
                gx_a_re, gx_a_im, gx_b_re, gx_b_im,
                t_a_re, t_a_im, t_b_re, t_b_im,
                &n_a_re, &n_a_im, &n_b_re, &n_b_im);

            e->su2_a_re = n_a_re;
            e->su2_a_im = n_a_im;
            e->su2_b_re = n_b_re;
            e->su2_b_im = n_b_im;
        }
    }
}

/* ==========================================================================
 * Algebra-Verifikation.
 *
 * Prueft zwei Aussagen:
 *   (1) Quaternion-Produkt = 2x2-Matrix-Multiplikation.
 *       Referenz: baut die 2x2-Matrix explizit und multipliziert in double.
 *   (2) Pauli-Kommutator [sx, sy] = 2i * sz (ueber pro_pauli_get).
 *
 * Rueckgabe: max_err ueber beide Pruefungen.
 * ========================================================================== */

static double pro_su2_verify_product(void)
{
    /* Achsen-Winkel-Paare (normiert). */
    const double angles[3] = { 0.7, 1.7, 2.9 };
    const double axes[3][3] = {
        { 1.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0 },
        { 0.0, 0.0, 1.0 },
    };

    const double S = (double)PRO_SU2_SCALE;
    double max_err = 0.0;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const double ai = angles[i] * 0.5;
            const double aj = angles[j] * 0.5;

            /* U_i, U_j in Quaternion-Form (Q30). */
            const int32_t i_ar = (int32_t)llround(cos(ai) * S);
            const int32_t i_ai = (int32_t)llround(-sin(ai) * axes[i][2] * S);
            const int32_t i_br = (int32_t)llround(-sin(ai) * axes[i][1] * S);
            const int32_t i_bi = (int32_t)llround(-sin(ai) * axes[i][0] * S);

            const int32_t j_ar = (int32_t)llround(cos(aj) * S);
            const int32_t j_ai = (int32_t)llround(-sin(aj) * axes[j][2] * S);
            const int32_t j_br = (int32_t)llround(-sin(aj) * axes[j][1] * S);
            const int32_t j_bi = (int32_t)llround(-sin(aj) * axes[j][0] * S);

            /* Ist: pro_su2_mul. */
            int32_t p_ar, p_ai, p_br, p_bi;
            pro_su2_mul(i_ar, i_ai, i_br, i_bi,
                j_ar, j_ai, j_br, j_bi,
                &p_ar, &p_ai, &p_br, &p_bi);

            /* Soll: 2x2-Matrix-Multiplikation in double.
             * U = [[a, b], [-b*, a*]], alles in komplexer Arithmetik. */
            const double ia_re = (double)i_ar / S, ia_im = (double)i_ai / S;
            const double ib_re = (double)i_br / S, ib_im = (double)i_bi / S;
            const double ja_re = (double)j_ar / S, ja_im = (double)j_ai / S;
            const double jb_re = (double)j_br / S, jb_im = (double)j_bi / S;

            /* Matrix-Eintraege von U_i und U_j (2x2, komplex). */
            const double Ui00_re = ia_re, Ui00_im = ia_im;
            const double Ui01_re = ib_re, Ui01_im = ib_im;
            const double Ui10_re = -ib_re, Ui10_im = ib_im;
            const double Ui11_re = ia_re, Ui11_im = -ia_im;
            const double Uj00_re = ja_re, Uj00_im = ja_im;
            const double Uj01_re = jb_re, Uj01_im = jb_im;
            const double Uj10_re = -jb_re, Uj10_im = jb_im;
            const double Uj11_re = ja_re, Uj11_im = -ja_im;

            /* M[0][0] = Ui00*Uj00 + Ui01*Uj10. */
            const double M00_re =
                Ui00_re * Uj00_re - Ui00_im * Uj00_im
                + Ui01_re * Uj10_re - Ui01_im * Uj10_im;
            const double M00_im =
                Ui00_re * Uj00_im + Ui00_im * Uj00_re
                + Ui01_re * Uj10_im + Ui01_im * Uj10_re;
            /* M[0][1] = Ui00*Uj01 + Ui01*Uj11. */
            const double M01_re =
                Ui00_re * Uj01_re - Ui00_im * Uj01_im
                + Ui01_re * Uj11_re - Ui01_im * Uj11_im;
            const double M01_im =
                Ui00_re * Uj01_im + Ui00_im * Uj01_re
                + Ui01_re * Uj11_im + Ui01_im * Uj11_re;

            /* Ist in double. */
            const double p_ar_d = (double)p_ar / S;
            const double p_ai_d = (double)p_ai / S;
            const double p_br_d = (double)p_br / S;
            const double p_bi_d = (double)p_bi / S;

            const double e_ar = fabs(p_ar_d - M00_re);
            const double e_ai = fabs(p_ai_d - M00_im);
            const double e_br = fabs(p_br_d - M01_re);
            const double e_bi = fabs(p_bi_d - M01_im);

            if (e_ar > max_err) max_err = e_ar;
            if (e_ai > max_err) max_err = e_ai;
            if (e_br > max_err) max_err = e_br;
            if (e_bi > max_err) max_err = e_bi;
        }
    }
    return max_err;
}

static double pro_su2_verify_pauli(void)
{
    /* Die drei Generatoren i*sigma_x, i*sigma_y, i*sigma_z.
     * In der SU(2)-Quaternion-Form gilt:
     *   [i*sigma_x, i*sigma_y] = -2 * i*sigma_z. */
    int32_t sx[4], sy[4], sz[4];
    pro_pauli_get(0, sx);
    pro_pauli_get(1, sy);
    pro_pauli_get(2, sz);

    int32_t p_ar, p_ai, p_br, p_bi;
    int32_t q_ar, q_ai, q_br, q_bi;
    pro_su2_mul(sx[0], sx[1], sx[2], sx[3],
        sy[0], sy[1], sy[2], sy[3],
        &p_ar, &p_ai, &p_br, &p_bi);
    pro_su2_mul(sy[0], sy[1], sy[2], sy[3],
        sx[0], sx[1], sx[2], sx[3],
        &q_ar, &q_ai, &q_br, &q_bi);

    const int32_t d_ar = p_ar - q_ar;
    const int32_t d_ai = p_ai - q_ai;
    const int32_t d_br = p_br - q_br;
    const int32_t d_bi = p_bi - q_bi;

    const double S = (double)PRO_SU2_SCALE;
    /* Erwartet: -2 * i*sigma_z = -2 * (0, S, 0, 0) = (0, -2S, 0, 0).
     * Normiert: (0, -2, 0, 0). */
    const double e_ar = fabs((double)d_ar / S - 0.0);
    const double e_ai = fabs((double)d_ai / S - (-2.0));
    const double e_br = fabs((double)d_br / S - 0.0);
    const double e_bi = fabs((double)d_bi / S - 0.0);

    double m = e_ar;
    if (e_ai > m) m = e_ai;
    if (e_br > m) m = e_br;
    if (e_bi > m) m = e_bi;
    return m;
}

PROPHYSICS_API double ProPhysics_Verify_SU2_Quaternion(void)
{
    const double e_prod = pro_su2_verify_product();
    const double e_pauli = pro_su2_verify_pauli();
    return (e_prod > e_pauli) ? e_prod : e_pauli;
}


/* ==========================================================================
 * End of ProPhysics_SU2.c
 * ========================================================================== */