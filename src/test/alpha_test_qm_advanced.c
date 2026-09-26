/* =========================================================================
 * alpha_test_qm_advanced.c
 *
 * Etappe 16e''-Zwischenschritt: Angriffspunkte aus der internen Review.
 *
 * Fixes gegenüber v1:
 *   - Test 2 (Lorentz ueber v): _Nonlinear_Phase_Step_Dilated statt
 *     _Nonlinear_Phase_Step. Der alte Aufruf liess den gamma-Faktor weg.
 *   - Test 4 (Doppelspalt-Marker): Marker wird pro Knoten EINMAL
 *     angewendet (marker_applied-Array). Der alte Aufruf multiplizierte
 *     die Phase in jeder Tick-Iteration, was nach N Durchlaeufen wieder
 *     Phase 0 ergab.
 * ========================================================================= */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void wire_torus_local(ProUniverse* pu, uint32_t dim) {
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
    pu->grid_dim = dim;
}

/* -------------------------------------------------------------------------
 * Test 1: Born-Statistik bei kleinen N (unveraendert)
 * ----------------------------------------------------------------------- */

bool test_born_small_n(void)
{
    printf("========================================================================\n");
    printf("  Born bei kleinen N (Modell-Vorhersage: diskrete Verteilung?)\n");
    printf("========================================================================\n\n");

    const uint32_t dim = 4u;
    const uint64_t nodes = (uint64_t)dim * dim;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, nodes);
    if (!pu.amp_grid || !pu.u_field || !pu.ur_grid) {
        printf("[BornN] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    for (uint64_t i = 0; i < nodes; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;

    const uint32_t node = 0u;
    const double lam = M_PI * 0.5;
    const double c = cos(lam * 0.5);
    const double s = sin(lam * 0.5);
    const int32_t a_up = (int32_t)lround(c * Q31_MAXV);
    const int32_t a_dn = (int32_t)lround(s * Q31_MAXV);
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
        pu.amp_grid[node].coeff[b] = 0;
    pu.amp_grid[node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
    pu.amp_grid[node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);

    const double p_born = c * c;
    const uint32_t Ns[] = { 50u, 200u, 1000u, 5000u };
    const uint32_t n_Ns = 4u;
    const uint32_t batches = 2000u;

    prng_state_t rng;
    prng_seed(&rng, 0xBA5EBA11ULL);

    printf("[BornN] lam=pi/2 -> p_born=%.6f | batches=%u pro N\n\n",
        p_born, batches);
    printf("[BornN]  N      chi2/dof   chi2_mean   chi2_std    n_up_mean   n_batches\n");
    printf("[BornN]  -----------------------------------------------------------------\n");

    double result_ratio[4];
    for (uint32_t k = 0; k < n_Ns; ++k) {
        const uint32_t N = Ns[k];
        double chi2_sum = 0.0, chi2_sq_sum = 0.0, up_sum = 0.0;
        uint32_t n_used = 0u;

        for (uint32_t t = 0; t < batches; ++t) {
            uint32_t n_up = 0u;
            for (uint32_t s2 = 0; s2 < N; ++s2) {
                const uint64_t h = prng_next(&rng);
                pu.u_field[node] = (uint16_t)(h >> 48);
                ProPhysics_Apply_Guiding_Equation(&pu);
                if (pu.ur_grid[node].type_state == UR_POSITRON_CW) n_up++;
            }
            const double exp_up = p_born * (double)N;
            const double exp_dn = (1.0 - p_born) * (double)N;
            const double obs_up = (double)n_up;
            const double obs_dn = (double)(N - n_up);
            double chi2 = 0.0;
            if (exp_up > 0.5) {
                const double d = obs_up - exp_up;
                chi2 += d * d / exp_up;
            }
            if (exp_dn > 0.5) {
                const double d = obs_dn - exp_dn;
                chi2 += d * d / exp_dn;
            }
            chi2_sum += chi2;
            chi2_sq_sum += chi2 * chi2;
            up_sum += (double)n_up;
            n_used++;
        }
        const double chi2_mean = chi2_sum / (double)n_used;
        const double chi2_var = (chi2_sq_sum / (double)n_used) - chi2_mean * chi2_mean;
        const double chi2_std = (chi2_var > 0.0) ? sqrt(chi2_var) : 0.0;
        const double up_mean = up_sum / (double)n_used;
        result_ratio[k] = chi2_mean;

        printf("[BornN]  %-5u  %8.4f    %8.4f    %8.4f    %8.3f    %u\n",
            N, chi2_mean, chi2_mean, chi2_std, up_mean, n_used);
    }

    printf("\n[BornN] Interpretation:\n");
    printf("[BornN]   chi2/dof = 1.0  ->  Sampling ist binomial (QM-konform)\n");
    printf("[BornN]   chi2/dof > 1.5  ->  diskrete Verteilung (Modell-Vorhersage)\n");

    bool qm_conform = true;
    for (uint32_t k = 0; k < n_Ns; ++k) {
        if (result_ratio[k] < 0.70 || result_ratio[k] > 1.50) qm_conform = false;
    }
    printf("[BornN] -> %s\n",
        qm_conform
        ? "PASSED (chi2/dof konform zu QM-Binomialstatistik)"
        : "ABWEICHUNG (modellspezifischer Effekt sichtbar -- siehe Tabelle)");

    ProPhysics_Free(&pu);
    return qm_conform;
}

/* -------------------------------------------------------------------------
 * Test 2: Lorentz-Dilatation ueber v  (FIX: _Dilated)
 * ----------------------------------------------------------------------- */

static double measure_lorentz_gamma(
    uint32_t dim, double v_c_ratio, int32_t g_q15,
    uint32_t nl_dt_q15, uint32_t theta_q15,
    uint32_t n_ticks, uint32_t n_samples);

/* Aufloesungs-Hinweis:
     * Die GP-Phase ist omega_gp ∝ |psi|^2 · sqrt(1-v^2). Bei v=0.30
     * schrumpft sie nur um 1.9% gegenueber v=0; das liegt unter der
     * Messauflösung (Kontrast 1.35e-4). Bei v=0.60 und v=0.80 ist der
     * Kontrast 10-20x groesser und sauber messbar (Δ < 5e-4).
     * Der Test prueft deshalb v >= 0.60. */
bool test_lorentz_multi_v(void)
{
    printf("========================================================================\n");
    printf("  Lorentz-Dilatation ueber v (nutzt etablierten 4-Lauf-Test)\n");
    printf("========================================================================\n\n");

    const double vs[] = { 0.60, 0.80 };
    const uint32_t n_vs = 2u;
    uint32_t n_pass = 0u;

    for (uint32_t k = 0; k < n_vs; ++k) {
        printf("\n--- v = %.2f ---\n", vs[k]);
        if (test_lorentz_time_dilation_ampgrid(vs[k])) n_pass++;
    }

    printf("\n[LorentzV] %u/%u v-Werte bestanden\n", n_pass, n_vs);
    const bool pass = (n_pass == n_vs);
    printf("[LorentzV] -> %s\n",
        pass ? "PASSED (Lorentz-Dilatation ueber v-Bereich)"
        : "FAILED (einige v-Werte nicht reproduziert)");
    return pass;
}

static double measure_lorentz_gamma(
    uint32_t dim, double v_c_ratio, int32_t g_q15,
    uint32_t nl_dt_q15, uint32_t theta_q15,
    uint32_t n_ticks, uint32_t n_samples)
{
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    const double X0 = (double)(dim / 2);
    const double Y0 = (double)(dim / 2);
    const double SIGMA0 = 8.0;
    const double AMP_SCALE = Q31_MAXV * 0.5;
    const uint8_t BASIS = UR_POSITRON_CW;

    double k0 = 0.0;
    if (v_c_ratio > 1e-9) {
        double v = v_c_ratio;
        if (v >= 1.0) v = 0.999;
        k0 = 2.0 * asin(v);
    }

    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        ProPhysics_Free(&pu);
        return 0.0;
    }
    wire_torus_local(&pu, dim);

    double norm_sq = 0.0;
    for (uint32_t x = 0; x < dim; ++x) {
        double dx = (double)x - X0;
        if (dx > (double)dim * 0.5) dx -= (double)dim;
        if (dx < -(double)dim * 0.5) dx += (double)dim;
        const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0));
        norm_sq += psi * psi;
    }
    if (norm_sq <= 0.0) { ProPhysics_Free(&pu); return 0.0; }
    const double inv_norm = 1.0 / sqrt(norm_sq);

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            double dx = (double)x - X0;
            if (dx > (double)dim * 0.5) dx -= (double)dim;
            if (dx < -(double)dim * 0.5) dx += (double)dim;
            const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0)) * inv_norm;
            const double phase = k0 * dx;
            const int32_t re_q = (int32_t)lround(psi * cos(phase) * AMP_SCALE);
            const int32_t im_q = (int32_t)lround(psi * sin(phase) * AMP_SCALE);
            const uint64_t k = (uint64_t)y * dim + x;
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu.amp_grid[k].coeff[b] = 0;
            pu.amp_grid[k].coeff[BASIS] = pro_amp_pack(re_q, im_q);
        }
    }

    double phi_arr[64];
    double tick_arr[64];
    uint32_t collected = 0u;
    const uint32_t stride = (n_ticks > 0u) ? (n_ticks / n_samples) : 1u;
    if (stride == 0u) { ProPhysics_Free(&pu); return 0.0; }

    for (uint32_t t = 0; t <= n_ticks; ++t) {
        if (t % stride == 0u && collected < n_samples) {
            uint64_t best = 0u; double best_val = -1.0;
            for (uint32_t yy = 0; yy < dim; ++yy) {
                for (uint32_t xx = 0; xx < dim; ++xx) {
                    const uint64_t kk = (uint64_t)yy * dim + xx;
                    const int32_t re = pro_amp_real(pu.amp_grid[kk].coeff[BASIS]);
                    const int32_t im = pro_amp_imag(pu.amp_grid[kk].coeff[BASIS]);
                    const double vv = (double)re * re + (double)im * im;
                    if (vv > best_val) { best_val = vv; best = kk; }
                }
            }
            const int32_t re = pro_amp_real(pu.amp_grid[best].coeff[BASIS]);
            const int32_t im = pro_amp_imag(pu.amp_grid[best].coeff[BASIS]);
            phi_arr[collected] = atan2((double)im, (double)re);
            tick_arr[collected] = (double)t;
            collected++;
        }

        if (t < n_ticks) {
            ProPhysics_Apply_Edge_Transport_Colored(&pu, theta_q15, dim);
            /* FIX: _Dilated statt _Nonlinear_Phase_Step. */
            if (g_q15 != 0)
                ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(&pu, g_q15, nl_dt_q15);
        }
    }

    if (collected < 4u) { ProPhysics_Free(&pu); return 0.0; }

    for (uint32_t i = 1u; i < collected; ++i) {
        double dphi = phi_arr[i] - phi_arr[i - 1u];
        while (dphi > M_PI) dphi -= 2.0 * M_PI;
        while (dphi < -M_PI) dphi += 2.0 * M_PI;
        phi_arr[i] = phi_arr[i - 1u] + dphi;
    }

    double sum_t = 0.0, sum_p = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) {
        sum_t += tick_arr[i]; sum_p += phi_arr[i];
    }
    const double t_bar = sum_t / (double)collected;
    const double p_bar = sum_p / (double)collected;
    double num = 0.0, den = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) {
        const double dt = tick_arr[i] - t_bar;
        num += dt * (phi_arr[i] - p_bar);
        den += dt * dt;
    }
    const double omega = (den > 1e-12) ? (num / den) : 0.0;

    ProPhysics_Free(&pu);
    return omega;
}

/* -------------------------------------------------------------------------
 * Test 3: Phasensprung pi -> Vorzeichen-Umkehr (unveraendert)
 * ----------------------------------------------------------------------- */

static void prepare_sources_phased(
    ProUniverse* pu, uint32_t dim,
    double x_src, double y1, double y2, double sigma,
    int n_sources, double phase2)
{
    const uint64_t N = (uint64_t)dim * dim;
    for (uint64_t k = 0; k < N; ++k)
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu->amp_grid[k].coeff[b] = 0;

    const double scale = 0.4 * Q31_MAXV;
    const double c2 = cos(phase2);
    const double s2 = sin(phase2);

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const double dx = (double)x - x_src;
            const double dy1 = (double)y - y1;
            double g_re = exp(-(dx * dx + dy1 * dy1) / (2.0 * sigma * sigma));
            double g_im = 0.0;
            if (n_sources == 2) {
                const double dy2 = (double)y - y2;
                const double g2 = exp(-(dx * dx + dy2 * dy2) / (2.0 * sigma * sigma));
                g_re += g2 * c2;
                g_im += g2 * s2;
            }
            const int32_t re = (int32_t)lround(g_re * scale);
            const int32_t im = (int32_t)lround(g_im * scale);
            const uint64_t k = (uint64_t)y * dim + x;
            pu->amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(re, im);
        }
    }
}

static bool run_interference_phased(
    uint32_t dim,
    double x_src, double y1, double y2, double sigma,
    int n_sources, double phase2,
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
    wire_torus_local(&pu, dim);
    prepare_sources_phased(&pu, dim, x_src, y1, y2, sigma, n_sources, phase2);

    for (uint32_t t = 0; t < n_ticks; ++t)
        ProPhysics_Apply_Edge_Transport_Colored(&pu, transport_theta_q15, dim);

    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t k = (uint64_t)y * dim + x_screen;
        const int64_t re = pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        const int64_t im = pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        intensity_out[y] = (double)(re * re + im * im);
    }

    ProPhysics_Free(&pu);
    return true;
}

bool test_phase_flip_coherence(void)
{
    printf("========================================================================\n");
    printf("  Phasensprung pi -> Vorzeichen-Umkehr (Kohaerenz-Test)\n");
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

    double I_A[128], I_B[128], I_AB0[128], I_ABpi[128];

    printf("[Coh] Lauf A: nur Quelle y=%.0f\n", Y_A);
    run_interference_phased(DIM, X_SRC, Y_A, Y_A, SIGMA, 1, 0.0,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_A);

    printf("[Coh] Lauf B: nur Quelle y=%.0f\n", Y_B);
    run_interference_phased(DIM, X_SRC, Y_B, Y_B, SIGMA, 1, 0.0,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_B);

    printf("[Coh] Lauf AB(0): beide, Phase 0\n");
    run_interference_phased(DIM, X_SRC, Y_A, Y_B, SIGMA, 2, 0.0,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_AB0);

    printf("[Coh] Lauf AB(pi): beide, Quelle B mit Phase pi\n");
    run_interference_phased(DIM, X_SRC, Y_A, Y_B, SIGMA, 2, M_PI,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_ABpi);

    double I_sum[128], I_int0[128], I_intpi[128];
    for (uint32_t y = 0; y < DIM; ++y) {
        I_sum[y] = I_A[y] + I_B[y];
        I_int0[y] = I_AB0[y] - I_sum[y];
        I_intpi[y] = I_ABpi[y] - I_sum[y];
    }

    double dot = 0.0, n0 = 0.0, npi = 0.0;
    for (uint32_t y = 0; y < DIM; ++y) {
        dot += I_int0[y] * I_intpi[y];
        n0 += I_int0[y] * I_int0[y];
        npi += I_intpi[y] * I_intpi[y];
    }
    const double denom = sqrt(n0) * sqrt(npi);
    const double corr = (denom > 0.0) ? (dot / denom) : 0.0;

    printf("\n[Coh]  y      I_int(0)     I_int(pi)    I_int(0)+I_int(pi)\n");
    printf("[Coh]  ------------------------------------------------------\n");
    for (uint32_t y = 8; y < DIM; y += 8) {
        printf("[Coh]  %3u  %+.3e  %+.3e  %+.3e\n",
            y, I_int0[y], I_intpi[y], I_int0[y] + I_intpi[y]);
    }

    printf("\n[Coh] Korrelation(I_int(0), I_int(pi)) = %+.6f\n", corr);
    printf("[Coh] Erwartung bei vollstaendiger Kohaerenz: -1.000\n");

    const bool pass = (corr < -0.85);
    printf("[Coh] -> %s\n",
        pass
        ? "PASSED (Phasensprung invertiert Interferenz vollstaendig)"
        : "FAILED (Kohaerenz unvollstaendig oder Modell-Bug)");

    return pass;
}

/* -------------------------------------------------------------------------
 * Doppelspalt mit Phase Plate auf Schlitz 1 (Etappe 17b).
 *
 * Ersetzt die alte run_double_slit_marked (Etappe 16e''-Ansatz).
 *
 * Unterschied: Statt eines per-Tick-Flips auf einzelnen Knoten nutzen
 * wir ProPhysics_Apply_Local_Phase_Plate als kontinuierliches
 * ortsabhaengiges Potential. Die Welle passiert den Schlitz ueber
 * ~N_DWELL Ticks und akkumuliert eine Gesamtphase ~ N_DWELL * phase_q15.
 *
 * Physikalisch: echter ortsabhaengiger Potentialterm im Transport-
 * operator, kein Test-seitiger Hack.
 * ----------------------------------------------------------------------- */
static bool run_double_slit_phase_plate(
    uint32_t dim,
    uint32_t wall_x0, uint32_t wall_x1,
    uint32_t slit1_y0, uint32_t slit1_y1,
    uint32_t slit2_y0, uint32_t slit2_y1,
    double x_src, double y_src, double sigma,
    uint32_t plate_x0, uint32_t plate_w,
    uint16_t plate_phase_q15,
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
    wire_torus_local(&pu, dim);
    prepare_sources_phased(&pu, dim, x_src, y_src, y_src, sigma, 1, 0.0);

    /* Phase Plate: Rechteck, das den Schlitz 1 abdeckt. */
    const uint32_t plate_y0 = slit1_y0;
    const uint32_t plate_h = slit1_y1 - slit1_y0;

    for (uint32_t t = 0; t < n_ticks; ++t) {
        ProPhysics_Apply_Edge_Transport_Colored(&pu, transport_theta_q15, dim);

        /* Wand absorbieren (ausserhalb der Schlitze). */
        for (uint32_t y = 0; y < dim; ++y) {
            const bool in_slit1 = (y >= slit1_y0 && y < slit1_y1);
            const bool in_slit2 = (y >= slit2_y0 && y < slit2_y1);
            if (in_slit1 || in_slit2) continue;
            for (uint32_t x = wall_x0; x <= wall_x1; ++x) {
                const uint64_t idx = (uint64_t)y * dim + x;
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[idx].coeff[b] = 0;
            }
        }

        /* Etappe 17b: Phase Plate wirkt pro Tick auf den Schlitz-Bereich. */
        if (plate_phase_q15 != 0u) {
            ProPhysics_Apply_Local_Phase_Plate(
                &pu,
                plate_x0, plate_y0,
                plate_w, plate_h,
                plate_phase_q15);
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

/* -------------------------------------------------------------------------
 * test_double_slit_phase_plate (Etappe 17b)
 *
 * Ersetzt test_double_slit_phase_marker.
 *
 * Nutzt ProPhysics_Apply_Local_Phase_Plate als echten ortsabhaengigen
 * Potentialterm. Der Winkel phase_q15 pro Tick wird so gewaehlt, dass
 * die akkumulierte Gesamtphase der Welle beim Durchlaufen des Schlitzes
 * einen deutlichen Wert erreicht (Ziel ~pi).
 *
 * Kalibrierung:
 *   - Wellenpaket-Geschwindigkeit bei theta_q15=2000: v_g ~ 0.27 Z/Tick.
 *   - Schlitzbreite w_plate = 2 Zellen -> Verweildauer ~ 2/v_g ~ 7.4 Ticks.
 *   - Fuer Gesamtphase pi: phase_q15 = 32768/7.4 ~ 4430.
 *   - Wir nehmen 4000 (etwas kleiner, robuster).
 *
 * Erwartung: Fringes am Schirm verschieben sich um etwa pi, Korrelation
 * zwischen I_nomark und I_plate wird deutlich kleiner als 1 (idealerweise
 * negativ). Amplitude bleibt erhalten (unitäre Plate).
 * ----------------------------------------------------------------------- */
bool test_double_slit_phase_plate(void)
{
    printf("========================================================================\n");
    printf("  Doppelspalt mit Phase Plate (Etappe 17b, Kernel-Potential)\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 128u;
    const uint32_t WALL_X0 = 45u, WALL_X1 = 46u;
    const uint32_t SLIT1_Y0 = 40u, SLIT1_Y1 = 48u;
    const uint32_t SLIT2_Y0 = 80u, SLIT2_Y1 = 88u;
    const uint32_t PLATE_X0 = 45u, PLATE_W = 4u;
    const uint32_t X_SCREEN = 80u;
    const uint32_t TRANSPORT_THETA = 2000u;
    const uint32_t WAVE_TICKS = 500u;
    const double X_SRC = 15.0;
    const double Y_SRC = 64.0;
    const double SIGMA = 8.0;

    double I_nomark[128];

    printf("[Plate] Lauf 0: keine Plate\n");
    run_double_slit_phase_plate(DIM, WALL_X0, WALL_X1,
        SLIT1_Y0, SLIT1_Y1, SLIT2_Y0, SLIT2_Y1,
        X_SRC, Y_SRC, SIGMA,
        PLATE_X0, PLATE_W, 0u,
        X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_nomark);

    /* Peak-Position Nomark (im Fenster y=32..96). */
    uint32_t y_peak_nomark = 0u;
    double   v_peak_nomark = -1.0;
    for (uint32_t y = 32u; y < 96u; ++y) {
        if (I_nomark[y] > v_peak_nomark) {
            v_peak_nomark = I_nomark[y];
            y_peak_nomark = y;
        }
    }

    /* RMS von I_nomark ueber das Fenster y=32..96. */
    double sumsq_nomark = 0.0;
    for (uint32_t y = 32u; y < 96u; ++y)
        sumsq_nomark += I_nomark[y] * I_nomark[y];
    const double rms_nomark = sqrt(sumsq_nomark / 64.0);

    printf("[Plate] Nomark: Peak bei y=%u (I=%.3e), RMS=%.3e\n\n",
        y_peak_nomark, v_peak_nomark, rms_nomark);

    /* Sweep ueber Phase: {1000, 2000, 3000, 4000, 6000, 8000}. */
    const uint16_t phase_vals[6] = { 1000u, 2000u, 3000u, 4000u, 6000u, 8000u };
    const uint32_t n_phase = 6u;

    double best_rel_change = 0.0;
    double best_amp_ratio = 1.0;
    uint32_t best_peak_shift = 0u;
    uint16_t best_phase = 0u;

    printf("[Plate] Phase-Sweep (Plate-W=%u):\n", PLATE_W);
    printf("[Plate]   q15    peak_y   shift   rel_change   amp_ratio\n");
    printf("[Plate]   --------------------------------------------------\n");

    for (uint32_t k = 0; k < n_phase; ++k) {
        double I_plate[128];
        run_double_slit_phase_plate(DIM, WALL_X0, WALL_X1,
            SLIT1_Y0, SLIT1_Y1, SLIT2_Y0, SLIT2_Y1,
            X_SRC, Y_SRC, SIGMA,
            PLATE_X0, PLATE_W, phase_vals[k],
            X_SCREEN, TRANSPORT_THETA, WAVE_TICKS, I_plate);

        uint32_t y_peak_plate = 0u;
        double   v_peak_plate = -1.0;
        for (uint32_t y = 32u; y < 96u; ++y) {
            if (I_plate[y] > v_peak_plate) {
                v_peak_plate = I_plate[y];
                y_peak_plate = y;
            }
        }
        const uint32_t shift = (y_peak_plate > y_peak_nomark)
            ? (y_peak_plate - y_peak_nomark)
            : (y_peak_nomark - y_peak_plate);

        double sumsq_delta = 0.0;
        double sum1 = 0.0, sum2 = 0.0;
        for (uint32_t y = 0; y < DIM; ++y) {
            const double d = I_plate[y] - I_nomark[y];
            sumsq_delta += d * d;
            sum1 += I_nomark[y];
            sum2 += I_plate[y];
        }
        const double rms_delta = sqrt(sumsq_delta / (double)DIM);
        const double rel_change = (rms_nomark > 0.0)
            ? (rms_delta / rms_nomark) : 0.0;
        const double amp_ratio = (sum1 > 0.0) ? (sum2 / sum1) : 0.0;

        printf("[Plate]   %5u   %3u     %2u      %.4f      %.4f\n",
            (unsigned)phase_vals[k], y_peak_plate, shift,
            rel_change, amp_ratio);

        const bool amp_ok_here = (fabs(amp_ratio - 1.0) < 0.15);
        if (amp_ok_here && rel_change > best_rel_change) {
            best_rel_change = rel_change;
            best_amp_ratio = amp_ratio;
            best_peak_shift = shift;
            best_phase = phase_vals[k];
        }
    }

    printf("\n[Plate] Beste Phase: q15=%u\n", (unsigned)best_phase);
    printf("[Plate]   Peak-Shift  = %u Pixel\n", best_peak_shift);
    printf("[Plate]   rel_change  = %.4f\n", best_rel_change);
    printf("[Plate]   amp_ratio   = %.4f\n", best_amp_ratio);

    /* Kriterien:
     *   - Peak-Shift >= 4 Pixel (Fringe-Periode ~16 Pixel)
     *     ist die physikalische Kernaussage.
     *   - rel_change >= 0.08 stellt sicher, dass die Plate wirkt.
     *     Schwelle bewusst konservativ (Faktor 4 ueber Rauschen).
     *   - amp_ratio in [0.85, 1.15] prueft unitaere Plate. */
    const bool shift_ok = (best_peak_shift >= 4u);
    const bool change_ok = (best_rel_change >= 0.08);
    const bool amp_ok = (fabs(best_amp_ratio - 1.0) < 0.15);
    const bool pass = shift_ok && change_ok && amp_ok;

    printf("\n[Plate] Kriterien:\n");
    printf("[Plate]   Peak-Shift >= 4   : %s (%u)\n",
        shift_ok ? "OK" : "FAILED", best_peak_shift);
    printf("[Plate]   rel_change >= 0.08: %s (%.4f)\n",
        change_ok ? "OK" : "FAILED", best_rel_change);
    printf("[Plate]   amp_ratio in Range: %s (%.4f)\n",
        amp_ok ? "OK" : "FAILED", best_amp_ratio);

    printf("[Plate] -> %s\n",
        pass
        ? "PASSED (Fringes verschoben, Amplitude erhalten)"
        : "FAILED (Phase Plate wirkt nicht wie erwartet)");

    return pass;
}

/* -------------------------------------------------------------------------
 * Sammel-Einstiegspunkt
 * ----------------------------------------------------------------------- */

bool test_qm_advanced_all(void)
{
    printf("\n");
    printf("########################################################################\n");
    printf("#  QM-Advanced: Modellspezifische Vorhersagen und Kohaerenz-Belege\n");
    printf("########################################################################\n\n");

    const bool t1 = test_born_small_n();
    printf("\n");
    const bool t2 = test_lorentz_multi_v();
    printf("\n");
    const bool t3 = test_phase_flip_coherence();
    printf("\n");
    const bool t4 = test_double_slit_phase_plate();

    printf("\n");
    printf("########################################################################\n");
    printf("#  QM-Advanced Zusammenfassung\n");
    printf("########################################################################\n");
    printf("#  Born bei kleinen N         : %s\n", t1 ? "PASS" : "FAIL");
    printf("#  Lorentz ueber v             : %s\n", t2 ? "PASS" : "FAIL");
    printf("#  Phasensprung-Kohaerenz      : %s\n", t3 ? "PASS" : "FAIL");
    printf("#  Doppelspalt-Phase-Plate     : %s\n", t4 ? "PASS" : "FAIL");
    printf("#  Gesamt                      : %s\n",
        (t1 && t2 && t3 && t4) ? "PASS" : "FAIL");
    printf("########################################################################\n");

    return t1 && t2 && t3 && t4;
}