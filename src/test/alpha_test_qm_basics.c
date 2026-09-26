/* =========================================================================
 * alpha_test_qm_basics.c
 *
 * QM-Konsistenz-Tests fuer den ProPhysics-Kernel.
 *
 * Etappe 16e''-Zwischenschritt.
 *
 * Historie:
 *   v1: Wave_Step statt Edge_Transport -> keine Ausbreitung, alles 0.
 *   v2: Edge_Transport, aber Referenz = Einzelquelle -> falsche Baseline.
 *   v3 (diese): Drei-Lauf-Interferometrie: I_A, I_B, I_AB, dann
 *                Interferenz-Term I_int = I_AB - I_A - I_B.
 *                Schirm weiter weg (x=90), Ticks=700, Quellen enger.
 *
 * Test 1: Zwei kohaerente Quellen -> Interferenz-Term
 *         I_int = I_AB - (I_A + I_B) sollte deutlich von Null
 *         verschieden sein, wenn Interferenz emergiert.
 *
 * Test 2: Klassischer Doppelspalt. Bleibt unveraendert (PASS in v2).
 * ========================================================================= */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* -------------------------------------------------------------------------
  * Gemeinsame Helfer
  * ----------------------------------------------------------------------- */

static void wire_torus_generic(ProUniverse* pu, uint32_t dim) {
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    for (uint64_t i = 0; i < N; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t y_n = ((y == 0) ? dim - 1 : y - 1) * (uint64_t)dim;
        const uint64_t y_s = ((y == dim - 1) ? 0 : y + 1) * (uint64_t)dim;
        const uint64_t y_c = (uint64_t)y * dim;
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = y_c + x;
            pu->reg_source[idx].channels[0] = y_n + x;
            pu->reg_source[idx].channels[1] = y_s + x;
            pu->reg_source[idx].channels[2] = y_c + ((x == dim - 1) ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_c + ((x == 0) ? dim - 1 : x - 1);
        }
    }
}

/* Setzt 1 oder 2 Gauss-Quellen mit Phase 0.
 * n_sources=1: nur Quelle bei (x_src, y1).
 * n_sources=2: Quellen bei (x_src, y1) und (x_src, y2), beide mit Phase 0. */
static void prepare_sources(
    ProUniverse* pu, uint32_t dim,
    double x_src, double y1, double y2, double sigma,
    int n_sources)
{
    const uint64_t N = (uint64_t)dim * dim;
    for (uint64_t k = 0; k < N; ++k)
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu->amp_grid[k].coeff[b] = 0;

    /* Normierung auf Peak 0.4*Q31_MAX, unabhaengig von n_sources. */
    const double scale = 0.4 * Q31_MAXV;

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const double dx = (double)x - x_src;
            const double dy1 = (double)y - y1;
            double g = exp(-(dx * dx + dy1 * dy1) / (2.0 * sigma * sigma));
            if (n_sources == 2) {
                const double dy2 = (double)y - y2;
                g += exp(-(dx * dx + dy2 * dy2) / (2.0 * sigma * sigma));
            }
            const int32_t q = (int32_t)lround(g * scale);
            const uint64_t k = (uint64_t)y * dim + x;
            pu->amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(q, 0);
        }
    }
}

/* Zaehlt lokale Maxima eines Intensitaetsprofils. */
static int count_fringes(const double* intensity, uint32_t dim, double min_frac) {
    double max_val = 0.0;
    for (uint32_t y = 0; y < dim; ++y)
        if (intensity[y] > max_val) max_val = intensity[y];
    if (max_val <= 0.0) return 0;
    const double thr = min_frac * max_val;
    int n = 0;
    for (uint32_t y = 2; y + 2 < dim; ++y) {
        if (intensity[y] > intensity[y - 1] && intensity[y] > intensity[y + 1] &&
            intensity[y] > intensity[y - 2] && intensity[y] > intensity[y + 2] &&
            intensity[y] > thr) {
            n++;
        }
    }
    return n;
}

/* ASCII-Darstellung. pos- und neg-Werte werden getrennt behandelt. */
static void print_profile(const char* label, const double* intensity,
    uint32_t dim, int show_signed) {
    double max_abs = 0.0;
    for (uint32_t y = 0; y < dim; ++y) {
        const double a = show_signed ? fabs(intensity[y]) : intensity[y];
        if (a > max_abs) max_abs = a;
    }
    if (max_abs <= 0.0) max_abs = 1.0;

    printf("[DS] %s (max_abs=%.3e):\n", label, max_abs);
    for (uint32_t y = 0; y < dim; y += 2) {
        const double val = intensity[y];
        const double norm = val / max_abs;
        const int bar = (int)(50.0 * fabs(norm));
        char sign_char = ' ';
        if (show_signed) sign_char = (val >= 0.0) ? '+' : '-';
        printf("[DS]   y=%3u  %+.4f %c ", y, norm, sign_char);
        for (int b = 0; b < bar && b < 50; ++b) printf("#");
        printf("\n");
    }
}

/* -------------------------------------------------------------------------
 * Zwei-Quellen-Interferometrie
 *
 * Fuehrt einen Lauf mit gegebener Quellenkonfiguration aus.
 * intensity_out[y] = |psi(x_screen, y)|^2  nach n_ticks Transport-Schritten.
 * ----------------------------------------------------------------------- */

static bool run_source_run(
    uint32_t dim, int n_sources,
    double x_src, double y1, double y2, double sigma,
    uint32_t x_screen, uint32_t transport_theta_q15, uint32_t n_ticks,
    double* intensity_out)
{
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch) {
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_generic(&pu, dim);
    prepare_sources(&pu, dim, x_src, y1, y2, sigma, n_sources);

    for (uint32_t t = 0; t < n_ticks; ++t) {
        ProPhysics_Apply_Edge_Transport_Colored(&pu, transport_theta_q15, dim);
    }

    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t k = (uint64_t)y * dim + x_screen;
        const int64_t re = pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        const int64_t im = pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        intensity_out[y] = (double)(re * re + im * im);
    }

    ProPhysics_Free(&pu);
    return true;
}

bool test_two_source_interference(void) {
    printf("========================================================================\n");
    printf("  Zwei-Quellen-Interferometrie (QM-Konsistenz-Test)\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 128u;
    const uint32_t X_SCREEN = 90u;
    const uint32_t TRANSPORT_THETA = 2000u;
    const uint32_t WAVE_TICKS = 700u;

    const double X_SRC = 20.0;
    const double Y_CENTER = 64.0;
    const double Y_SEP = 32.0;
    const double SIGMA = 4.0;

    const double Y_A = Y_CENTER - Y_SEP / 2.0;
    const double Y_B = Y_CENTER + Y_SEP / 2.0;

    printf("[DS] DIM=%u | Quellen x=%.0f y=%.0f und %.0f (sep=%.0f) sigma=%.1f\n",
        DIM, X_SRC, Y_A, Y_B, Y_SEP, SIGMA);
    printf("[DS] Schirm x=%u | Ticks=%u | theta_q15=%u\n\n",
        X_SCREEN, WAVE_TICKS, TRANSPORT_THETA);

    double I_A[128];
    double I_B[128];
    double I_AB[128];

    printf("[DS] Lauf A: Quelle bei y=%.0f allein...\n", Y_A);
    run_source_run(DIM, 1, X_SRC, Y_A, Y_A, SIGMA,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_A);

    printf("[DS] Lauf B: Quelle bei y=%.0f allein...\n", Y_B);
    run_source_run(DIM, 1, X_SRC, Y_B, Y_B, SIGMA,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_B);

    printf("[DS] Lauf AB: beide Quellen kohaerent...\n");
    run_source_run(DIM, 2, X_SRC, Y_A, Y_B, SIGMA,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_AB);

    /* Interferenz-Term: I_int = I_AB - (I_A + I_B) */
    double I_sum[128];   /* I_A + I_B */
    double I_int[128];   /* Interferenz */
    double max_sum = 0.0;
    double max_abs_int = 0.0;
    for (uint32_t y = 0; y < DIM; ++y) {
        I_sum[y] = I_A[y] + I_B[y];
        I_int[y] = I_AB[y] - I_sum[y];
        if (I_sum[y] > max_sum) max_sum = I_sum[y];
        if (fabs(I_int[y]) > max_abs_int) max_abs_int = fabs(I_int[y]);
    }

    printf("\n");
    print_profile("I_A (Quelle A allein)", I_A, DIM, 0);
    printf("\n");
    print_profile("I_B (Quelle B allein)", I_B, DIM, 0);
    printf("\n");
    print_profile("I_AB (beide kohaerent)", I_AB, DIM, 0);
    printf("\n");
    print_profile("I_int = I_AB - (I_A + I_B)", I_int, DIM, 1);
    printf("\n");

    /* Interferenz-Kontrast: max|I_int| / max(I_A + I_B).
     * Wenn Quellen kohaerent -> >0.05 typischerweise. */
    const double contrast = (max_sum > 0.0)
        ? (max_abs_int / max_sum) : 0.0;

    const int n_fringes_AB = count_fringes(I_AB, DIM, 0.3);
    const int n_fringes_sum = count_fringes(I_sum, DIM, 0.3);

    printf("[DS] max(I_A+I_B) = %.3e\n", max_sum);
    printf("[DS] max|I_int|   = %.3e\n", max_abs_int);
    printf("[DS] Kontrast max|I_int|/max(I_A+I_B) = %.4f\n", contrast);
    printf("[DS] Fringes in I_AB           : %d\n", n_fringes_AB);
    printf("[DS] Fringes in I_A+I_B        : %d\n", n_fringes_sum);

    /* Erfolgskriterium: signifikanter Interferenz-Term
     * (Kontrast > 5%) ODER die kohaerente Summe zeigt mehr Fringes
     * als die Inkohaerenz-Summe. */
    const bool contrast_ok = (contrast > 0.05);
    const bool fringe_ok = (n_fringes_AB > n_fringes_sum);

    const bool pass = contrast_ok || fringe_ok;
    printf("\n[DS] -> %s\n",
        pass ? "PASSED (Interferenz-Term nachgewiesen)"
        : "FAILED (kein Interferenz-Term ueber Rauschen)");

    return pass;
}

/* -------------------------------------------------------------------------
 * Klassischer Doppelspalt (unveraendert, war PASS)
 * ----------------------------------------------------------------------- */

static bool run_double_slit_experiment(
    uint32_t dim,
    uint32_t wall_x0, uint32_t wall_x1,
    uint32_t slit1_y0, uint32_t slit1_y1,
    uint32_t slit2_y0, uint32_t slit2_y1,
    int n_slits_open,
    double x_src, double y_src, double sigma,
    uint32_t x_screen, uint32_t transport_theta_q15, uint32_t n_ticks,
    double* intensity_out)
{
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch) {
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_generic(&pu, dim);
    prepare_sources(&pu, dim, x_src, y_src, y_src, sigma, 1);

    for (uint32_t t = 0; t < n_ticks; ++t) {
        ProPhysics_Apply_Edge_Transport_Colored(&pu, transport_theta_q15, dim);

        for (uint32_t y = 0; y < dim; ++y) {
            const bool in_slit1 = (n_slits_open >= 1) &&
                (y >= slit1_y0 && y < slit1_y1);
            const bool in_slit2 = (n_slits_open >= 2) &&
                (y >= slit2_y0 && y < slit2_y1);
            if (in_slit1 || in_slit2) continue;
            for (uint32_t x = wall_x0; x <= wall_x1; ++x) {
                const uint64_t idx = (uint64_t)y * dim + x;
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[idx].coeff[b] = 0;
            }
        }
    }

    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t k = (uint64_t)y * dim + x_screen;
        const int64_t re = pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        const int64_t im = pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        intensity_out[y] = (double)(re * re + im * im);
    }

    ProPhysics_Free(&pu);
    return true;
}

bool test_double_slit(void) {
    printf("========================================================================\n");
    printf("  Klassischer Doppelspalt (Wand + zwei Schlitze)\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 128u;
    const uint32_t WALL_X0 = 45u, WALL_X1 = 46u;
    const uint32_t SLIT1_Y0 = 40u, SLIT1_Y1 = 48u;
    const uint32_t SLIT2_Y0 = 80u, SLIT2_Y1 = 88u;
    const uint32_t X_SCREEN = 80u;
    const uint32_t TRANSPORT_THETA = 2000u;
    const uint32_t WAVE_TICKS = 500u;

    const double X_SRC = 15.0;
    const double Y_SRC = 64.0;
    const double SIGMA = 8.0;

    printf("[DS] DIM=%u | Wand x=%u..%u | Schlitze y=%u..%u und %u..%u\n",
        DIM, WALL_X0, WALL_X1, SLIT1_Y0, SLIT1_Y1, SLIT2_Y0, SLIT2_Y1);
    printf("[DS] Schirm x=%u | Ticks=%u | Quelle (%.0f, %.0f) sigma=%.1f\n\n",
        X_SCREEN, WAVE_TICKS, X_SRC, Y_SRC, SIGMA);

    double I_1slit[128];
    double I_2slits[128];

    printf("[DS] Lauf 1: nur unterer Schlitz offen...\n");
    run_double_slit_experiment(DIM, WALL_X0, WALL_X1,
        SLIT1_Y0, SLIT1_Y1, SLIT2_Y0, SLIT2_Y1,
        1, X_SRC, Y_SRC, SIGMA, X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_1slit);

    printf("[DS] Lauf 2: beide Schlitze offen...\n");
    run_double_slit_experiment(DIM, WALL_X0, WALL_X1,
        SLIT1_Y0, SLIT1_Y1, SLIT2_Y0, SLIT2_Y1,
        2, X_SRC, Y_SRC, SIGMA, X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_2slits);

    printf("\n");
    print_profile("Ein Schlitz offen", I_1slit, DIM, 0);
    printf("\n");
    print_profile("Beide Schlitze offen", I_2slits, DIM, 0);
    printf("\n");

    const int n_1 = count_fringes(I_1slit, DIM, 0.4);
    const int n_2 = count_fringes(I_2slits, DIM, 0.4);

    printf("[DS] Lokale Maxima am Schirm:\n");
    printf("[DS]   Ein Schlitz:  %d\n", n_1);
    printf("[DS]   Zwei Schlitze: %d\n", n_2);

    const bool pass = (n_2 >= 3) && (n_2 > n_1);
    printf("\n[DS] -> %s\n",
        pass ? "PASSED (Doppelspalt-Interferenz beobachtet)"
        : "FAILED (keine zusaetzlichen Fringes)");

    return pass;
}

/* -------------------------------------------------------------------------
 * Haupt-Einstiegspunkt
 * ----------------------------------------------------------------------- */

bool test_qm_basics_all(void) {
    printf("\n");
    printf("########################################################################\n");
    printf("#  QM-Basics: Emergenz von Wellenmechanik aus reiner Topologie\n");
    printf("########################################################################\n\n");

    const bool t1 = test_two_source_interference();
    printf("\n");
    const bool t2 = test_double_slit();

    printf("\n");
    printf("########################################################################\n");
    printf("#  QM-Basics Zusammenfassung\n");
    printf("########################################################################\n");
    printf("#  Zwei-Quellen-Interferenz : %s\n", t1 ? "PASS" : "FAIL");
    printf("#  Doppelspalt              : %s\n", t2 ? "PASS" : "FAIL");
    printf("#  Gesamt                   : %s\n", (t1 && t2) ? "PASS" : "FAIL");
    printf("########################################################################\n");

    return t1 && t2;
}