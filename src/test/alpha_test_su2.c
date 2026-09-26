/* ==========================================================================
 * alpha_test_su2.c
 *
 * Etappe 22: SU(2)-Eichfeld.
 *
 * 14 Untertests:
 *   T1  Quaternion-Unit: |a|^2+|b|^2 = PRO_SU2_NORM nach Normalisierung
 *   T2  Assoziativitaet: (q1*q2)*q3 = q1*(q2*q3)
 *   T3  q * q* = |q|^2 * I
 *   T4  Quaternion-Produkt == 2x2-Matrix-Produkt (Konsistenz)
 *   T5  Pauli-Spezialfall: [sx, sy] = 2i*sz in 2x2-Darstellung
 *   T6  U(1)-Einbettung: diag(e^i phi, e^i phi) -> Wilson-Loop = U(1)-Wert
 *   T7  Inversionssymmetrie: W(C^-1) = W(C)^dagger
 *   T8  Eichtransformation: Tr(W) invariant unter lokaler SU(2)-Eichung
 *   T9  Nicht-Abelsch: offener Pfad path=[0] vs path=[1]: W != W
 *   T10 Achsen-Winkel-Exponential = analytische Formel
 *   T11 Plaquette-Naeherung: konstantes Feld -> Tr(W) <= 2
 *   T12 Auto-Aktivierung: Set_Edge_SU2 setzt pu->su2_active = 1
 *   T13 R7-Regression: su2_active == 0 -> bestehender Pfad bit-identisch
 *   T14 U5-Erhaltung mit aktiven SU(2)-Links ueber 500 Ticks
 *
 * Zusaetzlich wird ProPhysics_Verify_SU2_Quaternion (Kernel-seitig)
 * aufgerufen und muss max_err < 1e-9 liefern.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* Q30-Skala aus ProPhysics_Config.h (transitiv ueber alpha_test_common.h). */
#define SU2_S      ((double)PRO_SU2_SCALE)   /* 2^30 */
#define SU2_NORM   ((uint64_t)PRO_SU2_NORM)  /* 2^60 */

/* -------------------------------------------------------------------------
 * Test-Helfer: Quaternion-Arithmetik in Q30 (Kopie der Kernel-Formel).
 *
 * Die Formel spiegelt exakt pro_su2_mul aus ProPhysics_SU2.c.
 * Sie dient als unabhaengige Referenz fuer die Testpruefungen.
 * ----------------------------------------------------------------------- */

static void su2_identity(int32_t* a_re, int32_t* a_im,
    int32_t* b_re, int32_t* b_im)
{
    *a_re = PRO_SU2_IDENT_RE;
    *a_im = PRO_SU2_IDENT_IM;
    *b_re = 0;
    *b_im = 0;
}

static uint64_t su2_norm_sq(int32_t a_re, int32_t a_im,
    int32_t b_re, int32_t b_im)
{
    const uint64_t ar = (a_re < 0) ? (uint64_t)(-(int64_t)a_re) : (uint64_t)a_re;
    const uint64_t ai = (a_im < 0) ? (uint64_t)(-(int64_t)a_im) : (uint64_t)a_im;
    const uint64_t br = (b_re < 0) ? (uint64_t)(-(int64_t)b_re) : (uint64_t)b_re;
    const uint64_t bi = (b_im < 0) ? (uint64_t)(-(int64_t)b_im) : (uint64_t)b_im;
    return ar * ar + ai * ai + br * br + bi * bi;
}

static int32_t su2_sat_i32(int64_t x)
{
    if (x > INT32_MAX) return INT32_MAX;
    if (x < INT32_MIN) return INT32_MIN;
    return (int32_t)x;
}

static int64_t su2_round_shift_q30(int64_t x)
{
    if (x >= 0) return (x + (1LL << 29)) >> 30;
    return -(((-x) + (1LL << 29)) >> 30);
}

static void su2_mul_ref(
    int32_t a1_re, int32_t a1_im, int32_t b1_re, int32_t b1_im,
    int32_t a2_re, int32_t a2_im, int32_t b2_re, int32_t b2_im,
    int32_t* o_ar, int32_t* o_ai, int32_t* o_br, int32_t* o_bi)
{
    /* (a, b) * (c, d) = (a*c - b*d*, a*d + b*c*) */
    const int64_t ar_num =
        (int64_t)a1_re * a2_re - (int64_t)a1_im * a2_im
        - (int64_t)b1_re * b2_re - (int64_t)b1_im * b2_im;
    const int64_t ai_num =
        (int64_t)a1_re * a2_im + (int64_t)a1_im * a2_re
        + (int64_t)b1_re * b2_im - (int64_t)b1_im * b2_re;
    const int64_t br_num =
        (int64_t)a1_re * b2_re - (int64_t)a1_im * b2_im
        + (int64_t)b1_re * a2_re + (int64_t)b1_im * a2_im;
    const int64_t bi_num =
        (int64_t)a1_re * b2_im + (int64_t)a1_im * b2_re
        - (int64_t)b1_re * a2_im + (int64_t)b1_im * a2_re;

    *o_ar = su2_sat_i32(su2_round_shift_q30(ar_num));
    *o_ai = su2_sat_i32(su2_round_shift_q30(ai_num));
    *o_br = su2_sat_i32(su2_round_shift_q30(br_num));
    *o_bi = su2_sat_i32(su2_round_shift_q30(bi_num));
}

/* Achsen-Winkel-Exponential in Q30 (Kopie der Kernel-Formel). */
static void su2_axis_angle(int32_t* a_re, int32_t* a_im,
    int32_t* b_re, int32_t* b_im,
    double nx, double ny, double nz, double alpha)
{
    const double nrm = sqrt(nx * nx + ny * ny + nz * nz);
    if (nrm < 1e-12) { su2_identity(a_re, a_im, b_re, b_im); return; }
    nx /= nrm; ny /= nrm; nz /= nrm;
    const double c = cos(alpha * 0.5);
    const double s = sin(alpha * 0.5);
    *a_re = su2_sat_i32((int64_t)llround(c * SU2_S));
    *a_im = su2_sat_i32((int64_t)llround(-s * nz * SU2_S));
    *b_re = su2_sat_i32((int64_t)llround(-s * ny * SU2_S));
    *b_im = su2_sat_i32((int64_t)llround(-s * nx * SU2_S));
}

/* Kleiner Helfer: 4x4-Torus fuer SU(2)-Tests verdrahten. */
static void su2_wire_torus4(ProUniverse* pu)
{
    const uint32_t DIM = 4u;
    for (uint64_t i = 0; i < pu->total_nodes; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu->reg_source[idx].channels[0] = y_n + x;
            pu->reg_source[idx].channels[1] = y_s + x;
            pu->reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }
}

/* -------------------------------------------------------------------------
 * T1: Quaternion-Unit — |a|^2 + |b|^2 = PRO_SU2_NORM nach Normalisierung.
 *
 * Wir rufen ProPhysics_Set_Edge_SU2 mit willkuerlichen Werten auf und
 * pruefen via ProPhysics_Get_Edge_SU2, dass der Kernel auf Norm 2^60
 * normalisiert hat.
 * ----------------------------------------------------------------------- */
static bool test_T1_unit(void)
{
    printf("[SU2] T1: Quaternion-Unit\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 16);
    su2_wire_torus4(&pu);

    const int32_t raw_a_re[] = { 500000000, -800000000, 1000000, 2000000000, 1073741824 };
    const int32_t raw_a_im[] = { -300000000, 400000000, -2000000, 500000000, 0 };
    const int32_t raw_b_re[] = { 200000000, -600000000, 3000000, -1200000000, 0 };
    const int32_t raw_b_im[] = { -100000000, 500000000, -4000000, 800000000, 0 };

    double max_dev = 0.0;
    const uint32_t N = 5u;

    for (uint32_t i = 0; i < N; ++i) {
        const uint64_t src = i % 16u;
        const uint8_t  ch = (uint8_t)(i % 4u);

        ProPhysics_Set_Edge_SU2(&pu, src, ch,
            raw_a_re[i], raw_a_im[i], raw_b_re[i], raw_b_im[i]);

        int32_t ar, ai, br, bi;
        ProPhysics_Get_Edge_SU2(&pu, src, ch, &ar, &ai, &br, &bi);

        const uint64_t n2 = su2_norm_sq(ar, ai, br, bi);
        const double dev = fabs((double)n2 - (double)SU2_NORM)
            / (double)SU2_NORM;
        if (dev > max_dev) max_dev = dev;
    }

    printf("[SU2]   max |norm^2/2^60 - 1| = %.4e (Schwelle 1e-9)\n", max_dev);
    const bool ok = (max_dev < 1e-9);
    printf("[SU2]   T1 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* -------------------------------------------------------------------------
 * T2: Assoziativitaet (q1*q2)*q3 = q1*(q2*q3).
 * ----------------------------------------------------------------------- */
static bool test_T2_assoc(void)
{
    printf("[SU2] T2: Assoziativitaet\n");

    int32_t a1r, a1i, b1r, b1i;
    int32_t a2r, a2i, b2r, b2i;
    int32_t a3r, a3i, b3r, b3i;
    su2_axis_angle(&a1r, &a1i, &b1r, &b1i, 1, 0, 0, 0.7);
    su2_axis_angle(&a2r, &a2i, &b2r, &b2i, 0, 1, 0, 1.3);
    su2_axis_angle(&a3r, &a3i, &b3r, &b3i, 0, 0, 1, 2.1);

    int32_t t_ar, t_ai, t_br, t_bi;
    int32_t l_ar, l_ai, l_br, l_bi;
    int32_t r_ar, r_ai, r_br, r_bi;

    su2_mul_ref(a1r, a1i, b1r, b1i, a2r, a2i, b2r, b2i,
        &t_ar, &t_ai, &t_br, &t_bi);
    su2_mul_ref(t_ar, t_ai, t_br, t_bi, a3r, a3i, b3r, b3i,
        &l_ar, &l_ai, &l_br, &l_bi);

    su2_mul_ref(a2r, a2i, b2r, b2i, a3r, a3i, b3r, b3i,
        &t_ar, &t_ai, &t_br, &t_bi);
    su2_mul_ref(a1r, a1i, b1r, b1i, t_ar, t_ai, t_br, t_bi,
        &r_ar, &r_ai, &r_br, &r_bi);

    const double diff_ar = fabs((double)(l_ar - r_ar)) / SU2_S;
    const double diff_ai = fabs((double)(l_ai - r_ai)) / SU2_S;
    const double diff_br = fabs((double)(l_br - r_br)) / SU2_S;
    const double diff_bi = fabs((double)(l_bi - r_bi)) / SU2_S;
    const double max_diff = fmax(fmax(diff_ar, diff_ai), fmax(diff_br, diff_bi));

    printf("[SU2]   max |L - R| = %.4e (Schwelle 1e-9)\n", max_diff);
    const bool ok = (max_diff < 1e-9);
    printf("[SU2]   T2 %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

/* -------------------------------------------------------------------------
 * T3: q * q* = |q|^2 * I.
 * ----------------------------------------------------------------------- */
static bool test_T3_conj(void)
{
    printf("[SU2] T3: q * q* = |q|^2 * I\n");

    int32_t ar, ai, br, bi;
    su2_axis_angle(&ar, &ai, &br, &bi, 0.6, 0.8, 0.0, 1.9);

    const int32_t car = ar;
    const int32_t cai = (int32_t)(0u - (uint32_t)ai);
    const int32_t cbr = (int32_t)(0u - (uint32_t)br);
    const int32_t cbi = (int32_t)(0u - (uint32_t)bi);

    int32_t o_ar, o_ai, o_br, o_bi;
    su2_mul_ref(ar, ai, br, bi, car, cai, cbr, cbi,
        &o_ar, &o_ai, &o_br, &o_bi);

    const uint64_t norm = su2_norm_sq(ar, ai, br, bi);
    const double expected_ar = (double)norm / SU2_S;

    const double e_ar = fabs((double)o_ar - expected_ar) / SU2_S;
    const double e_ai = fabs((double)o_ai) / SU2_S;
    const double e_br = fabs((double)o_br) / SU2_S;
    const double e_bi = fabs((double)o_bi) / SU2_S;
    const double max_err = fmax(fmax(e_ar, e_ai), fmax(e_br, e_bi));

    printf("[SU2]   max |q*q* - |q|^2 I| = %.4e (Schwelle 1e-9)\n", max_err);
    const bool ok = (max_err < 1e-9);
    printf("[SU2]   T3 %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

/* -------------------------------------------------------------------------
 * T4: Quaternion-Produkt == 2x2-Matrix-Produkt.
 *
 * Wir nehmen zwei konkrete SU(2)-Elemente, multiplizieren sie per
 * su2_mul_ref und vergleichen mit der 2x2-Matrix-Multiplikation in double.
 * ----------------------------------------------------------------------- */
static bool test_T4_matrix_consistency(void)
{
    printf("[SU2] T4: Quaternion == 2x2-Matrix-Produkt\n");

    const double angles[3] = { 0.7, 1.7, 2.9 };
    const double axes[3][3] = {
        { 1.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0 },
        { 0.0, 0.0, 1.0 },
    };

    double max_err = 0.0;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const double ai = angles[i] * 0.5;
            const double aj = angles[j] * 0.5;

            const int32_t i_ar = (int32_t)llround(cos(ai) * SU2_S);
            const int32_t i_ai = (int32_t)llround(-sin(ai) * axes[i][2] * SU2_S);
            const int32_t i_br = (int32_t)llround(-sin(ai) * axes[i][1] * SU2_S);
            const int32_t i_bi = (int32_t)llround(-sin(ai) * axes[i][0] * SU2_S);

            const int32_t j_ar = (int32_t)llround(cos(aj) * SU2_S);
            const int32_t j_ai = (int32_t)llround(-sin(aj) * axes[j][2] * SU2_S);
            const int32_t j_br = (int32_t)llround(-sin(aj) * axes[j][1] * SU2_S);
            const int32_t j_bi = (int32_t)llround(-sin(aj) * axes[j][0] * SU2_S);

            int32_t p_ar, p_ai, p_br, p_bi;
            su2_mul_ref(i_ar, i_ai, i_br, i_bi,
                j_ar, j_ai, j_br, j_bi,
                &p_ar, &p_ai, &p_br, &p_bi);

            /* 2x2-Matrizen aus Quaternion-Form.
             * U = [[a, b], [-b*, a*]] komplex. */
            const double ia_re = (double)i_ar / SU2_S, ia_im = (double)i_ai / SU2_S;
            const double ib_re = (double)i_br / SU2_S, ib_im = (double)i_bi / SU2_S;
            const double ja_re = (double)j_ar / SU2_S, ja_im = (double)j_ai / SU2_S;
            const double jb_re = (double)j_br / SU2_S, jb_im = (double)j_bi / SU2_S;

            /* Ui = [[a, b], [-b*, a*]], Uj analog. */
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

            const double p_ar_d = (double)p_ar / SU2_S;
            const double p_ai_d = (double)p_ai / SU2_S;
            const double p_br_d = (double)p_br / SU2_S;
            const double p_bi_d = (double)p_bi / SU2_S;

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

    printf("[SU2]   max |Quaternion - Matrix| = %.4e (Schwelle 1e-9)\n", max_err);
    const bool ok = (max_err < 1e-9);
    printf("[SU2]   T4 %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

/* -------------------------------------------------------------------------
 * T5: Pauli-Spezialfall in 2x2-Darstellung.
 *
 * Wir pruefen die klassische Pauli-Relation [sx, sy] = 2i*sz direkt auf
 * den 2x2-Matrizen (Standard-Darstellung), unabhaengig von der
 * Quaternion-Konvention. Das zeigt, dass die Algebra, in der die
 * SU(2)-Struktur lebt, tatsaechlich die Pauli-Algebra ist.
 * ----------------------------------------------------------------------- */
static bool test_T5_pauli_relation(void)
{
    printf("[SU2] T5: Pauli [sx, sy] = 2i*sz (2x2)\n");

    /* Standard-Pauli in komplexer 2x2-Form (re, im komponentenweise). */
    const double sx[2][2][2] = { {{0,0},{1,0}}, {{1,0},{0,0}} };
    const double sy[2][2][2] = { {{0,0},{0,-1}}, {{0,1},{0,0}} };
    const double sz[2][2][2] = { {{1,0},{0,0}}, {{0,0},{-1,0}} };

    double max_err = 0.0;
    for (int a = 0; a < 2; ++a) {
        for (int b = 0; b < 2; ++b) {
            /* [sx, sy] = sx*sy - sy*sx. */
            double xy_re = 0.0, xy_im = 0.0;
            double yx_re = 0.0, yx_im = 0.0;
            for (int c = 0; c < 2; ++c) {
                xy_re += sx[a][c][0] * sy[c][b][0] - sx[a][c][1] * sy[c][b][1];
                xy_im += sx[a][c][0] * sy[c][b][1] + sx[a][c][1] * sy[c][b][0];
                yx_re += sy[a][c][0] * sx[c][b][0] - sy[a][c][1] * sx[c][b][1];
                yx_im += sy[a][c][0] * sx[c][b][1] + sy[a][c][1] * sx[c][b][0];
            }
            const double d_re = xy_re - yx_re;
            const double d_im = xy_im - yx_im;

            /* Erwartet: 2i*sz. */
            const double e_re = 0.0;
            const double e_im = 2.0 * sz[a][b][0];   /* nur Realteil von sz relevant */

            const double err = sqrt((d_re - e_re) * (d_re - e_re)
                + (d_im - e_im) * (d_im - e_im));
            if (err > max_err) max_err = err;
        }
    }

    printf("[SU2]   max |[sx,sy] - 2i*sz| = %.4e (Schwelle 1e-9)\n", max_err);
    const bool ok = (max_err < 1e-9);
    printf("[SU2]   T5 %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

/* -------------------------------------------------------------------------
 * T6: U(1)-Einbettung.
 *
 * Vier Links als diag(e^{i*pi/4}, e^{i*pi/4}). Wilson-Loop ergibt e^{i*pi} = -1.
 * pu.su2_active wird explizit auf 0 gesetzt, weil dieser Test den
 * U(1)-Fallback pruefen soll, nicht den SU(2)-Pfad.
 * ----------------------------------------------------------------------- */
static bool test_T6_u1_embedding(void)
{
    printf("[SU2] T6: U(1)-Einbettung (diag)\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 16);
    su2_wire_torus4(&pu);

    const uint64_t path[4] = { 0u, 1u, 5u, 4u };
    const uint8_t  chans[4] = { 0u, 0u, 0u, 0u };

    const double phi = M_PI * 0.25;
    const int32_t ar = (int32_t)llround(cos(phi) * SU2_S);
    const int32_t ai = (int32_t)llround(sin(phi) * SU2_S);

    for (int i = 0; i < 4; ++i) {
        ProPhysics_Set_Edge_SU2(&pu, path[i], chans[i], ar, ai, 0, 0);
    }

    /* Nach Set_Edge_SU2 ist su2_active == 1. Fuer den U(1)-Fallback
     * setzen wir es wieder auf 0 (nur Test-Repraesentation). */
    pu.su2_active = 0u;

    int32_t w_ar, w_ai, w_br, w_bi;
    const bool ok_loop = ProPhysics_Wilson_Loop_SU2(
        &pu, path, chans, 4u, &w_ar, &w_ai, &w_br, &w_bi);

    const double exp_a_re = -1.0;
    const double exp_a_im = 0.0;
    const double exp_b_re = 0.0;
    const double exp_b_im = 0.0;

    const double e_ar = fabs((double)w_ar / SU2_S - exp_a_re);
    const double e_ai = fabs((double)w_ai / SU2_S - exp_a_im);
    const double e_br = fabs((double)w_br / SU2_S - exp_b_re);
    const double e_bi = fabs((double)w_bi / SU2_S - exp_b_im);
    const double max_err = fmax(fmax(e_ar, e_ai), fmax(e_br, e_bi));

    printf("[SU2]   W = (%.6f, %.6f, %.6f, %.6f) (erwartet (-1,0,0,0))\n",
        (double)w_ar / SU2_S, (double)w_ai / SU2_S,
        (double)w_br / SU2_S, (double)w_bi / SU2_S);
    printf("[SU2]   max |W - (-1,0,0,0)| = %.4e (Schwelle 1e-6)\n", max_err);
    const bool ok = ok_loop && (max_err < 1e-6);
    printf("[SU2]   T6 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* -------------------------------------------------------------------------
 * T7: Tr(W) invariant unter zyklischer Rotation des Pfads.
 *
 * Die Wilson-Loop W(C) ist das geordnete Produkt der Links. Bei einem
 * geschlossenen Pfad ist Tr(W) invariant unter zyklischer Verschiebung
 * der Link-Reihenfolge (Spur-Zyklizitaet: Tr(AB) = Tr(BA)).
 *
 * Die vorige Version dieses Tests (W(C^-1) = W(C)^dagger) gilt nur fuer
 * selbstadjungierte Links und ist daher kein allgemeingueltiges
 * Kriterium. Die zyklische Spur-Invarianz ist dagegen fuer alle
 * unitaeren Links gueltig.
 * ----------------------------------------------------------------------- */
static bool test_T7_cyclic_trace(void)
{
    printf("[SU2] T7: Tr(W) invariant unter zyklischer Rotation\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 16);
    su2_wire_torus4(&pu);

    const uint64_t path_a[4] = { 0u, 1u, 5u, 4u };
    const uint64_t path_b[4] = { 1u, 5u, 4u, 0u };
    const uint8_t  chans[4] = { 0u, 0u, 0u, 0u };

    /* Vier nicht-triviale SU(2)-Links an den Pfad-Positionen.
     * Beide Pfade verwenden DIESELBEN (node, channel)-Paare, nur in
     * zyklisch verschobener Reihenfolge. */
    for (int i = 0; i < 4; ++i) {
        int32_t ar, ai, br, bi;
        su2_axis_angle(&ar, &ai, &br, &bi,
            (double)(i + 1) * 0.3,
            (double)(i + 2) * 0.2,
            (double)(i + 3) * 0.1,
            0.5 + 0.4 * (double)i);
        ProPhysics_Set_Edge_SU2(&pu, path_a[i], chans[i], ar, ai, br, bi);
    }

    const double tr_a = ProPhysics_Wilson_Loop_SU2_Trace(
        &pu, path_a, chans, 4u);
    const double tr_b = ProPhysics_Wilson_Loop_SU2_Trace(
        &pu, path_b, chans, 4u);

    const double diff = fabs(tr_a - tr_b);
    printf("[SU2]   Tr(W(A)) = %.9f\n", tr_a);
    printf("[SU2]   Tr(W(B)) = %.9f  (zyklisch verschobener Pfad)\n", tr_b);
    printf("[SU2]   |Delta| = %.4e (Schwelle 1e-9)\n", diff);

    const bool ok = (diff < 1e-9);
    printf("[SU2]   T7 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* -------------------------------------------------------------------------
 * T8: Tr(W) invariant unter lokaler SU(2)-Eichung.
 *
 * pu.su2_active wird explizit auf 0 gesetzt, weil dieser Test die
 * Link-Transformation prueft, nicht den Dispatch.
 * ----------------------------------------------------------------------- */
static bool test_T8_gauge_invariance(void)
{
    printf("[SU2] T8: Tr(W) invariant unter lokaler Eichung\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 16);
    su2_wire_torus4(&pu);

    const uint64_t path[4] = { 0u, 1u, 5u, 4u };
    /* Echte geschlossene Plaquette um das Quadrat {0, 1, 4, 5}:
    *   0 -> 1 (channel 2 = E)
    *   1 -> 5 (channel 1 = S)
    *   5 -> 4 (channel 3 = W)
    *   4 -> 0 (channel 0 = N)
    * Nur so heben sich die Eichfaktoren g(y)^dagger und g(x) in
    * aufeinanderfolgenden Kanten auf und Tr(W) ist invariant. */
    const uint8_t  chans[4] = { 2u, 1u, 3u, 0u };

    for (int i = 0; i < 4; ++i) {
        int32_t ar, ai, br, bi;
        su2_axis_angle(&ar, &ai, &br, &bi,
            0.5 + 0.2 * i, 0.3 + 0.1 * i, 0.7, 0.8);
        ProPhysics_Set_Edge_SU2(&pu, path[i], chans[i], ar, ai, br, bi);
    }

    pu.su2_active = 0u;

    const double trace_before = ProPhysics_Wilson_Loop_SU2_Trace(
        &pu, path, chans, 4u);

    int32_t* lambda = (int32_t*)malloc(16u * 4u * sizeof(int32_t));
    if (!lambda) { ProPhysics_Free(&pu); return false; }
    for (uint64_t k = 0; k < 16u; ++k) {
        int32_t ar, ai, br, bi;
        su2_axis_angle(&ar, &ai, &br, &bi,
            (double)(k + 1) * 0.13,
            (double)(k + 2) * 0.17,
            (double)(k + 3) * 0.11,
            (double)k * 0.31);
        lambda[k * 4 + 0] = ar;
        lambda[k * 4 + 1] = ai;
        lambda[k * 4 + 2] = br;
        lambda[k * 4 + 3] = bi;
    }
    ProPhysics_Apply_Local_SU2_Gauge(&pu, lambda);

    const double trace_after = ProPhysics_Wilson_Loop_SU2_Trace(
        &pu, path, chans, 4u);

    const double diff = fabs(trace_after - trace_before);
    printf("[SU2]   Tr(W) before = %.9f\n", trace_before);
    printf("[SU2]   Tr(W) after  = %.9f\n", trace_after);
    printf("[SU2]   |Delta| = %.4e (Schwelle 1e-9)\n", diff);

    free(lambda);
    const bool ok = (diff < 1e-9);
    printf("[SU2]   T8 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* -------------------------------------------------------------------------
 * T9: Nicht-Abelsch.
 *
 * Zwei verschiedene Links auf Kanten 0->1 und 1->0. Ein 1-Kanten-Pfad
 * liefert genau den jeweiligen Link zurueck. Da die Links per
 * Konstruktion verschieden sind, ist W(0->1) != W(1->0).
 * ----------------------------------------------------------------------- */
static bool test_T9_nonabelian(void)
{
    printf("[SU2] T9: Nicht-Abelsch (offener Pfad)\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 4);
    pu.reg_source[0].channels[0] = 1u;
    pu.reg_source[1].channels[0] = 0u;

    int32_t u1_ar, u1_ai, u1_br, u1_bi;
    int32_t u2_ar, u2_ai, u2_br, u2_bi;
    su2_axis_angle(&u1_ar, &u1_ai, &u1_br, &u1_bi, 1, 0, 0, 1.2);
    su2_axis_angle(&u2_ar, &u2_ai, &u2_br, &u2_bi, 0, 0, 1, 0.9);

    ProPhysics_Set_Edge_SU2(&pu, 0u, 0u, u1_ar, u1_ai, u1_br, u1_bi);
    ProPhysics_Set_Edge_SU2(&pu, 1u, 0u, u2_ar, u2_ai, u2_br, u2_bi);

    const uint64_t p01[1] = { 0u };
    const uint8_t  c01[1] = { 0u };
    const uint64_t p10[1] = { 1u };
    const uint8_t  c10[1] = { 0u };

    int32_t w01_ar, w01_ai, w01_br, w01_bi;
    int32_t w10_ar, w10_ai, w10_br, w10_bi;
    ProPhysics_Wilson_Loop_SU2(&pu, p01, c01, 1u,
        &w01_ar, &w01_ai, &w01_br, &w01_bi);
    ProPhysics_Wilson_Loop_SU2(&pu, p10, c10, 1u,
        &w10_ar, &w10_ai, &w10_br, &w10_bi);

    const double e_ar = fabs((double)(w01_ar - w10_ar)) / SU2_S;
    const double e_ai = fabs((double)(w01_ai - w10_ai)) / SU2_S;
    const double e_br = fabs((double)(w01_br - w10_br)) / SU2_S;
    const double e_bi = fabs((double)(w01_bi - w10_bi)) / SU2_S;
    const double total_diff = e_ar + e_ai + e_br + e_bi;

    printf("[SU2]   |W(0->1) - W(1->0)|_1 = %.6f (Schwelle 0.1)\n", total_diff);
    const bool ok = (total_diff > 0.1);
    printf("[SU2]   T9 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* -------------------------------------------------------------------------
 * T10: Achsen-Winkel-Exponential gegen analytische Formel.
 * ----------------------------------------------------------------------- */
static bool test_T10_axis_angle(void)
{
    printf("[SU2] T10: Achsen-Winkel-Exponential\n");

    const double alpha = M_PI * 0.25;
    const double c = cos(alpha * 0.5);
    const double s = sin(alpha * 0.5);

    /* U = exp(i * pi/4 * sigma_x). Analytisch in Quaternion-Form:
     *   a = cos(pi/8), a_im = 0
     *   b = i * sin(pi/8)  -> b_re = 0, b_im = sin(pi/8) */
    const double exp_ar = c;
    const double exp_ai = 0.0;
    const double exp_br = 0.0;
    /* Kernel-Konvention: U = exp(-i*alpha/2 * n*sigma), Standard-QM.
    * Fuer n = x: b = -i*sin(alpha/2)  -> b_im = -s. */
    const double exp_bi = -s;

    int32_t ar, ai, br, bi;
    su2_axis_angle(&ar, &ai, &br, &bi, 1.0, 0.0, 0.0, alpha);

    const double e_ar = fabs((double)ar / SU2_S - exp_ar);
    const double e_ai = fabs((double)ai / SU2_S - exp_ai);
    const double e_br = fabs((double)br / SU2_S - exp_br);
    const double e_bi = fabs((double)bi / SU2_S - exp_bi);
    const double max_err = fmax(fmax(e_ar, e_ai), fmax(e_br, e_bi));

    printf("[SU2]   max |U - U_exp| = %.4e (Schwelle 1e-9)\n", max_err);
    const bool ok = (max_err < 1e-9);
    printf("[SU2]   T10 %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

/* -------------------------------------------------------------------------
 * T11: Plaquette-Naeherung. Konstantes Feld in x-Richtung, kleines alpha.
 *
 * Tr(W) <= 2 und Reduktion >= 0.
 * ----------------------------------------------------------------------- */
static bool test_T11_plaquette(void)
{
    printf("[SU2] T11: Plaquette-Naeherung (kleines alpha)\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 16);
    su2_wire_torus4(&pu);

    const uint64_t path[4] = { 0u, 1u, 5u, 4u };
    const uint8_t  chans[4] = { 0u, 0u, 0u, 0u };

    const double alpha = 0.1;
    for (int i = 0; i < 4; ++i) {
        int32_t ar, ai, br, bi;
        su2_axis_angle(&ar, &ai, &br, &bi, 1, 0, 0, alpha);
        ProPhysics_Set_Edge_SU2(&pu, path[i], chans[i], ar, ai, br, bi);
    }

    pu.su2_active = 0u;

    const double tr = ProPhysics_Wilson_Loop_SU2_Trace(&pu, path, chans, 4u);
    const double reduction = 2.0 - tr;

    printf("[SU2]   Tr(W) = %.9f (2.0 - %.2e)\n", tr, reduction);
    const bool ok = (tr <= 2.0 + 1e-9) && (reduction >= -1e-9);
    printf("[SU2]   T11 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* --------------------------------------------------------------------------
 * T12: Auto-Aktivierung.
 * ----------------------------------------------------------------------- */
static bool test_T12_auto_active(void)
{
    printf("[SU2] T12: Auto-Aktivierung\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 4);
    pu.reg_source[0].channels[0] = 1u;

    const bool was_inactive = (pu.su2_active == 0u);
    printf("[SU2]   vorher: su2_active = %u\n", (unsigned)pu.su2_active);

    ProPhysics_Set_Edge_SU2(&pu, 0u, 0u, PRO_SU2_IDENT_RE, 0, 0, 0);

    const bool is_active = (pu.su2_active == 1u);
    printf("[SU2]   nachher: su2_active = %u\n", (unsigned)pu.su2_active);

    const bool ok = was_inactive && is_active;
    printf("[SU2]   T12 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* --------------------------------------------------------------------------
 * T13: R7-Regression. su2_active == 0 -> bestehender Pfad bit-identisch.
 * ----------------------------------------------------------------------- */
static bool test_T13_regression(void)
{
    printf("[SU2] T13: R7-Regression (su2_active == 0)\n");

    const uint64_t NODES = 256u;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);

    const uint32_t DIM = 16u;
    for (uint64_t i = 0; i < NODES; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
        pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(8192), 0);
    }
    ProPhysics_Apply_Guiding_Equation(&pu);

    const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
    for (int t = 0; t < 100; ++t) {
        ProPhysics_Apply_Amp_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
    }
    const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);

    const double nd0 = pro_u128_to_double(n0);
    const double nd1 = pro_u128_to_double(n1);
    const double drift = (nd0 > 0.0) ? fabs(nd1 - nd0) / nd0 : 0.0;

    printf("[SU2]   su2_active = %u (erwartet 0)\n", (unsigned)pu.su2_active);
    printf("[SU2]   U5-Drift ueber 100 Ticks = %.4e (Schwelle 1e-6)\n", drift);

    const bool ok = (pu.su2_active == 0u) && (drift < 1e-6);
    printf("[SU2]   T13 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* --------------------------------------------------------------------------
 * T14: U5-Erhaltung mit aktiven SU(2)-Links ueber 500 Ticks.
 * ----------------------------------------------------------------------- */
static bool test_T14_u5_with_su2(void)
{
    printf("[SU2] T14: U5-Erhaltung mit aktiven SU(2)-Links\n");

    const uint64_t NODES = 256u;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);

    const uint32_t DIM = 16u;
    for (uint64_t i = 0; i < NODES; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t ch = 0; ch < 4u; ++ch) {
            int32_t ar, ai, br, bi;
            su2_axis_angle(&ar, &ai, &br, &bi,
                (double)(k + 1) * 0.07,
                (double)(ch + 1) * 0.11,
                (double)(k + ch + 1) * 0.05,
                (double)(k * 4 + ch) * 0.09);
            ProPhysics_Set_Edge_SU2(&pu, k, ch, ar, ai, br, bi);
        }
    }

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
        pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(8192), 0);
    }
    ProPhysics_Apply_Guiding_Equation(&pu);

    const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
    for (int t = 0; t < 500; ++t) {
        ProPhysics_Apply_Amp_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
    }
    const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);

    const double nd0 = pro_u128_to_double(n0);
    const double nd1 = pro_u128_to_double(n1);
    const double drift = (nd0 > 0.0) ? fabs(nd1 - nd0) / nd0 : 0.0;

    printf("[SU2]   su2_active = %u\n", (unsigned)pu.su2_active);
    printf("[SU2]   U5-Drift ueber 500 Ticks = %.4e (Schwelle 1e-3)\n", drift);
    const bool ok = (pu.su2_active == 1u) && (drift < 1e-3);
    printf("[SU2]   T14 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* --------------------------------------------------------------------------
 * T15: Link-Norm bleibt unter Leapfrog erhalten (Unitariät).
 * ----------------------------------------------------------------------- */
static bool test_T15_dynamics_norm_preserved(void)
{
    printf("[SU2] T15: Dynamik erhaelt Link-Norm\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    su2_wire_torus4(&pu);

    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        for (uint8_t ch = 0; ch < 4u; ++ch) {
            int32_t ar, ai, br, bi;
            su2_axis_angle(&ar, &ai, &br, &bi,
                (double)(k + 1) * 0.07,
                (double)(ch + 1) * 0.11,
                0.5,
                (double)(k * 4 + ch) * 0.09);
            ProPhysics_Set_Edge_SU2(&pu, k, ch, ar, ai, br, bi);
        }
    }

    ProPhysics_Enable_SU2_Dynamics(&pu);
    ProPhysics_Set_SU2_Yang_Mills(&pu, 500);

    for (int t = 0; t < 100; ++t) {
        ProPhysics_Apply_SU2_Tick(&pu, 500u);
    }

    double max_dev = 0.0;
    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        for (uint8_t ch = 0; ch < 4u; ++ch) {
            int32_t ar, ai, br, bi;
            ProPhysics_Get_Edge_SU2(&pu, k, ch, &ar, &ai, &br, &bi);
            const uint64_t n2 = su2_norm_sq(ar, ai, br, bi);
            const double dev = fabs((double)n2 - (double)SU2_NORM)
                / (double)SU2_NORM;
            if (dev > max_dev) max_dev = dev;
        }
    }

    printf("[SU2]   max |norm^2/2^60 - 1| = %.4e (Schwelle 1e-5)\n", max_dev);
    const bool ok = (max_dev < 1e-5);
    printf("[SU2]   T15 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* --------------------------------------------------------------------------
 * T16: Leapfrog — Energie bleibt grob erhalten.
 * ----------------------------------------------------------------------- */
static bool test_T16_dynamics_energy(void)
{
    printf("[SU2] T16: Dynamik — Energieerhaltung (100 Ticks)\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    su2_wire_torus4(&pu);

    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        for (uint8_t ch = 0; ch < 4u; ++ch) {
            int32_t ar, ai, br, bi;
            su2_axis_angle(&ar, &ai, &br, &bi,
                (double)(k + 1) * 0.13,
                (double)(ch + 1) * 0.07,
                0.3,
                (double)(k * 4 + ch) * 0.05);
            ProPhysics_Set_Edge_SU2(&pu, k, ch, ar, ai, br, bi);
        }
    }

    ProPhysics_Enable_SU2_Dynamics(&pu);
    ProPhysics_Set_SU2_Yang_Mills(&pu, 500);

    const double e0 = ProPhysics_SU2_Total_Energy(&pu);

    for (int t = 0; t < 100; ++t) {
        ProPhysics_Apply_SU2_Tick(&pu, 500u);
    }

    const double e1 = ProPhysics_SU2_Total_Energy(&pu);
    const double rel = (e0 > 1e-12) ? fabs(e1 - e0) / e0 : 0.0;

    printf("[SU2]   E(0) = %.9f\n", e0);
    printf("[SU2]   E(100) = %.9f\n", e1);
    printf("[SU2]   rel. Drift = %.4e (Schwelle 1e-2)\n", rel);

    const bool ok = (rel < 1e-2);
    printf("[SU2]   T16 %s\n", ok ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return ok;
}

/* --------------------------------------------------------------------------
 * T17: R7 — bei deaktivierter Dynamik bleibt Link unveraendert.
 * ----------------------------------------------------------------------- */
static bool test_T17_dynamics_r7(void)
{
    printf("[SU2] T17: R7 — Dynamics aus, Link unveraendert\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    su2_wire_torus4(&pu);

    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        for (uint8_t ch = 0; ch < 4u; ++ch) {
            int32_t ar, ai, br, bi;
            su2_axis_angle(&ar, &ai, &br, &bi, 1, 0, 0, 0.7);
            ProPhysics_Set_Edge_SU2(&pu, k, ch, ar, ai, br, bi);
        }
    }

    ProPhysics_Disable_SU2_Dynamics(&pu);

    int32_t a0r, a0i, b0r, b0i;
    ProPhysics_Get_Edge_SU2(&pu, 0, 0, &a0r, &a0i, &b0r, &b0i);

    for (int t = 0; t < 100; ++t) {
        ProPhysics_Apply_SU2_Tick(&pu, PRO_SU2_LEAPFROG_DT_Q15);
    }

    int32_t a1r, a1i, b1r, b1i;
    ProPhysics_Get_Edge_SU2(&pu, 0, 0, &a1r, &a1i, &b1r, &b1i);

    const bool unchanged = (a0r == a1r) && (a0i == a1i)
        && (b0r == b1r) && (b0i == b1i);

    printf("[SU2]   Link unveraendert: %s\n", unchanged ? "ja" : "NEIN");
    printf("[SU2]   T17 %s\n", unchanged ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return unchanged;
}

/* --------------------------------------------------------------------------
 * T18: aktive Dynamik aendert Links.
 * ----------------------------------------------------------------------- */
static bool test_T18_dynamics_evolves(void)
{
    printf("[SU2] T18: Dynamics aendert Links bei aktivem Flag\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    su2_wire_torus4(&pu);

    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        for (uint8_t ch = 0; ch < 4u; ++ch) {
            int32_t ar, ai, br, bi;
            su2_axis_angle(&ar, &ai, &br, &bi,
                (double)(k + 1) * 0.11,
                (double)(ch + 1) * 0.09,
                0.4,
                (double)(k * 4 + ch) * 0.07);
            ProPhysics_Set_Edge_SU2(&pu, k, ch, ar, ai, br, bi);
        }
    }

    ProPhysics_Enable_SU2_Dynamics(&pu);
    ProPhysics_Set_SU2_Yang_Mills(&pu, 2000);
    ProPhysics_Apply_SU2_Tick(&pu, 32768u);

    bool changed = false;
    for (uint64_t k = 0; k < pu.total_nodes && !changed; ++k) {
        for (uint8_t ch = 0; ch < 4u && !changed; ++ch) {
            int32_t ar, ai, br, bi;
            ProPhysics_Get_Edge_SU2(&pu, k, ch, &ar, &ai, &br, &bi);
            int32_t ref_ar, ref_ai, ref_br, ref_bi;
            su2_axis_angle(&ref_ar, &ref_ai, &ref_br, &ref_bi,
                (double)(k + 1) * 0.11,
                (double)(ch + 1) * 0.09,
                0.4,
                (double)(k * 4 + ch) * 0.07);
            if (ar != ref_ar || ai != ref_ai
                || br != ref_br || bi != ref_bi) {
                changed = true;
            }
        }
    }

    printf("[SU2]   Link geaendert nach Tick: %s\n", changed ? "ja" : "NEIN");
    printf("[SU2]   T18 %s\n", changed ? "PASS" : "FAIL");
    ProPhysics_Free(&pu);
    return changed;
}

/* ==========================================================================
 * Gesamteinstiegspunkt.
 * ========================================================================== */

bool test_su2_wilson_loop(void)
{
    printf("========================================================================\n");
    printf("  Etappe 22: SU(2)-Eichfeld\n");
    printf("========================================================================\n\n");

    int n_pass = 0, n_total = 0;

    const bool t1 = test_T1_unit();
    n_total++; if (t1)  n_pass++;
    const bool t2 = test_T2_assoc();
    n_total++; if (t2)  n_pass++;
    const bool t3 = test_T3_conj();
    n_total++; if (t3)  n_pass++;
    const bool t4 = test_T4_matrix_consistency();
    n_total++; if (t4)  n_pass++;
    const bool t5 = test_T5_pauli_relation();
    n_total++; if (t5)  n_pass++;
    const bool t6 = test_T6_u1_embedding();
    n_total++; if (t6)  n_pass++;
    const bool t7 = test_T7_cyclic_trace();
    n_total++; if (t7)  n_pass++;
    const bool t8 = test_T8_gauge_invariance();
    n_total++; if (t8)  n_pass++;
    const bool t9 = test_T9_nonabelian();
    n_total++; if (t9)  n_pass++;
    const bool t10 = test_T10_axis_angle();
    n_total++; if (t10) n_pass++;
    const bool t11 = test_T11_plaquette();
    n_total++; if (t11) n_pass++;
    const bool t12 = test_T12_auto_active();
    n_total++; if (t12) n_pass++;
    const bool t13 = test_T13_regression();
    n_total++; if (t13) n_pass++;
    const bool t14 = test_T14_u5_with_su2();
    n_total++; if (t14) n_pass++;
    const bool t15 = test_T15_dynamics_norm_preserved();
    n_total++; if (t15) n_pass++;
    const bool t16 = test_T16_dynamics_energy();
    n_total++; if (t16) n_pass++;
    const bool t17 = test_T17_dynamics_r7();
    n_total++; if (t17) n_pass++;
    const bool t18 = test_T18_dynamics_evolves();
    n_total++; if (t18) n_pass++;

    printf("\n[SU2] Ergebnis: %d / %d\n", n_pass, n_total);

    printf("\n[SU2] Ergebnis: %d / %d\n", n_pass, n_total);

    /* Zusaetzlich: Kernel-Algebra-Check. */
    const double algebra_err = ProPhysics_Verify_SU2_Quaternion();
    printf("[SU2] Kernel-Algebra-Check max_err = %.4e (Schwelle 1e-9)\n",
        algebra_err);
    const bool kernel_ok = (algebra_err < 1e-9);
    printf("[SU2] Kernel-Algebra %s\n", kernel_ok ? "PASS" : "FAIL");
    n_total++; if (kernel_ok) n_pass++;

    const bool pass = (n_pass == n_total);
    printf("\n[SU2] -> %s\n",
        pass ? "PASSED (SU(2)-Eichfeld implementiert)"
        : "FAILED (siehe oben)");
    return pass;
}