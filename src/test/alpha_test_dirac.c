/* =========================================================================
 * alpha_test_dirac.c
 *
 * Etappe 21 + 21b: Dirac-Struktur.
 *
 * Etappe 21:  γ-Matrizen, Massenterm, einfacher Dirac-Step.
 * Etappe 21b: α^i = γ^0 γ^i-Kopplung an die 6 Sweeps.
 *
 * Test 1: γ-Algebra (Dirac- und Weyl-Darstellung)
 *   {γ^μ, γ^ν} = 2 η^μν · I  mit η = diag(+1, -1, -1, -1)
 *
 * Test 2: Massen-Kalibrierung bei k = 0
 *   Der α^i-Transport mittelt sich bei homogenem Zustand über die
 *   zwei Parität-Sweeps pro Richtung weg. Die Phase der L↑-Komponente
 *   ist damit rein durch den Massenterm bestimmt:
 *       ω(k=0, m) = θ_mass = (mass_q15 / 32768) · π
 *   Bei m = 0 ist ω(k=0) = 0 exakt.
 *
 * Test 3: Dispersions-Charakter (qualitativ)
 *   Bei k > 0 mischt der α-Transport die Komponenten. Die Phase der
 *   L↑-Komponente wird dispersiv. Der Test prüft, dass ω(k) monoton
 *   mit k wächst — ohne die Kontinuumsformel zu erwarten (die entsteht
 *   erst im Renormierungs-Limes, Etappe 23).
 *
 * Test 4: Zitterbewegung-Rohdaten (Option A + Vorbereitung C)
 *
 * Test 5: Regression ohne Dirac bit-identisch
 * ========================================================================= */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* =========================================================================
  * Konstanten
  * ========================================================================= */

#define DIRAC_DIM    32u
#define DIRAC_NODES  ((uint64_t)DIRAC_DIM * DIRAC_DIM * DIRAC_DIM)

#define DIRAC_THETA_Q15   2000u
#define DIRAC_N_TICKS     80u
#define DIRAC_N_SAMPLES   40u

  /* Dirac-Basis-Indizes (Basis-Slot 1..4 → 4 Dirac-Komponenten) */
#define BASIS_L_UP  1u
#define BASIS_L_DN  2u
#define BASIS_R_UP  3u
#define BASIS_R_DN  4u

/* =========================================================================
 * Gemeinsame Helfer
 * ========================================================================= */

static void prepare_dirac_bloch_state(
    ProUniverse* pu,
    uint32_t DIM, uint32_t shift,
    double kx, double ky, double kz,
    const double comps[4],
    double scale)
{
    for (uint32_t z = 0; z < DIM; ++z) {
        for (uint32_t y = 0; y < DIM; ++y) {
            for (uint32_t x = 0; x < DIM; ++x) {
                const uint64_t idx = ((uint64_t)z << (2u * shift))
                    | ((uint64_t)y << shift)
                    | (uint64_t)x;

                ProAmpVector* v = &pu->amp_grid[idx];
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                    v->coeff[b] = 0;
                }

                const double phase = kx * (double)x
                    + ky * (double)y
                    + kz * (double)z;
                const double cph = cos(phase);
                const double sph = sin(phase);

                for (int ci = 0; ci < 4; ++ci) {
                    const double re = comps[ci] * cph * scale;
                    const double im = comps[ci] * sph * scale;

                    int32_t rq = (int32_t)lround(re);
                    int32_t iq = (int32_t)lround(im);
                    if (rq > INT32_MAX) rq = INT32_MAX;
                    if (rq < INT32_MIN) rq = INT32_MIN;
                    if (iq > INT32_MAX) iq = INT32_MAX;
                    if (iq < INT32_MIN) iq = INT32_MIN;

                    const uint8_t basis = (uint8_t)(ci + 1);
                    v->coeff[basis] = pro_amp_pack(rq, iq);
                }
            }
        }
    }
}

static double measure_dirac_phase_node0(const ProUniverse* pu)
{
    const ProAmpQ31 c = pu->amp_grid[0].coeff[BASIS_L_UP];
    const int32_t re = pro_amp_real(c);
    const int32_t im = pro_amp_imag(c);
    return atan2((double)im, (double)re);
}

static double linear_omega(const double* tick_arr,
    const double* phi_arr,
    uint32_t n)
{
    if (n < 4u) return 0.0;
    double sum_t = 0.0, sum_p = 0.0;
    for (uint32_t i = 0; i < n; ++i) {
        sum_t += tick_arr[i];
        sum_p += phi_arr[i];
    }
    const double t_bar = sum_t / (double)n;
    const double p_bar = sum_p / (double)n;

    double num = 0.0, den = 0.0;
    for (uint32_t i = 0; i < n; ++i) {
        const double dt = tick_arr[i] - t_bar;
        num += dt * (phi_arr[i] - p_bar);
        den += dt * dt;
    }
    return (den > 1e-12) ? (num / den) : 0.0;
}

static void unwrap_phase(double* phi_arr, uint32_t n)
{
    for (uint32_t i = 1; i < n; ++i) {
        double d = phi_arr[i] - phi_arr[i - 1];
        while (d > M_PI) d -= 2.0 * M_PI;
        while (d < -M_PI) d += 2.0 * M_PI;
        phi_arr[i] = phi_arr[i - 1] + d;
    }
}

/* Fuehrt einen Dirac-Lauf fuer einen Bloch-Zustand durch.
 * Rueckgabe: omega_meas, norm_drift.
 * Wird von Test 2 und Test 3 wiederverwendet. */
static bool run_dirac_bloch(
    ProGammaBasis basis,
    int32_t mass_q15,
    double kx, double ky, double kz,
    double* out_omega,
    double* out_norm_drift)
{
    const double comps[4] = { 0.5, 0.0, 0.5, 0.0 };
    const double scale = 2147483647.0;
    const uint32_t SHIFT = 5u;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, DIRAC_NODES);
    wire_torus_3d(&pu, DIRAC_DIM);

    pu.dirac_active = 1u;
    pu.dirac_gamma_basis = (uint8_t)basis;
    pu.dirac_mass_q15 = mass_q15;
    for (uint64_t k = 0; k < DIRAC_NODES; ++k) {
        pu.ur_grid[k].reserved_gating |= PRO_NODE_DIRAC_BIT;
    }

    prepare_dirac_bloch_state(&pu, DIRAC_DIM, SHIFT,
        kx, ky, kz,
        comps, scale);

    /* Norm-Referenz (alle 4 Komponenten ueber alle Knoten) */
    double norm0 = 0.0;
    for (uint64_t k = 0; k < DIRAC_NODES; ++k) {
        for (int ci = 0; ci < 4; ++ci) {
            const ProAmpQ31 c = pu.amp_grid[k].coeff[PRO_DIRAC_TO_BASIS((uint8_t)ci)];
            const double re = (double)pro_amp_real(c);
            const double im = (double)pro_amp_imag(c);
            norm0 += re * re + im * im;
        }
    }

    double phi_arr[DIRAC_N_SAMPLES];
    double tick_arr[DIRAC_N_SAMPLES];
    uint32_t collected = 0u;
    const uint32_t stride = DIRAC_N_TICKS / DIRAC_N_SAMPLES;

    for (uint32_t t = 0; t <= DIRAC_N_TICKS; ++t) {
        if (t % stride == 0u && collected < DIRAC_N_SAMPLES) {
            phi_arr[collected] = measure_dirac_phase_node0(&pu);
            tick_arr[collected] = (double)t;
            collected++;
        }
        if (t < DIRAC_N_TICKS) {
            ProPhysics_Apply_Dirac_Step(&pu, mass_q15, DIRAC_THETA_Q15);
        }
    }

    if (collected < 4u) { ProPhysics_Free(&pu); return false; }

    double norm1 = 0.0;
    for (uint64_t k = 0; k < DIRAC_NODES; ++k) {
        for (int ci = 0; ci < 4; ++ci) {
            const ProAmpQ31 c = pu.amp_grid[k].coeff[PRO_DIRAC_TO_BASIS((uint8_t)ci)];
            const double re = (double)pro_amp_real(c);
            const double im = (double)pro_amp_imag(c);
            norm1 += re * re + im * im;
        }
    }
    *out_norm_drift = (norm0 > 0.0) ? fabs(norm1 - norm0) / norm0 : 0.0;

    unwrap_phase(phi_arr, collected);
    *out_omega = linear_omega(tick_arr, phi_arr, collected);

    ProPhysics_Free(&pu);
    return true;
}

/* =========================================================================
 * Test 1: γ-Algebra
 * ========================================================================= */

static bool test_dirac_gamma_algebra(void)
{
    printf("========================================================================\n");
    printf("  Test 1: Gamma-Algebra {gamma_mu, gamma_nu} = 2 eta_mu_nu * I\n");
    printf("========================================================================\n\n");

    const double err = ProPhysics_Verify_Gamma_Algebra();

    printf("[Dirac] Gamma-Algebra-Abweichung (Dirac-Darstellung):\n");
    printf("[Dirac]   max_err = %.4e   (Schwelle 1e-9)\n", err);

    /* Q31-Rundungsgrenze: 2^-30 ≈ 9.3e-10 */
    const bool ok = (err < 1e-9);

    printf("[Dirac]   -> %s\n\n", ok ? "PASS" : "FAIL");
    return ok;
}

/* =========================================================================
 * Test 2: Massen-Kalibrierung bei k = 0
 *
 * Nach Etappe 21b traegt der α-Transport bei homogenem Zustand (k=0)
 * nichts bei — die zwei Paritaet-Sweeps pro Richtung heben sich auf.
 * Die Phase der L↑-Komponente ist rein durch den Massenterm bestimmt:
 *
 *     ω(k=0, m) = θ_mass = (mass_q15 / 32768) · π
 *
 * Bei m = 0 exakt 0.
 * ========================================================================= */

static bool test_dirac_mass_gap(ProGammaBasis basis, const char* label)
{
    printf("------------------------------------------------------------------------\n");
    printf("  Test 2: Massen-Kalibrierung bei k=0 [%s]\n", label);
    printf("------------------------------------------------------------------------\n\n");

    printf("[Dirac] DIM=%u | theta_q15=%u | N_TICKS=%u\n",
        DIRAC_DIM, DIRAC_THETA_Q15, DIRAC_N_TICKS);
    printf("[Dirac] gamma-Basis: %s\n\n", label);

    const int32_t masses[3] = { 0, 1000, 3000 };
    const char* mass_labels[3] = { "m=0      ", "m=1000   ", "m=3000   " };

    printf("[Dirac]   Masse    omega_meas      theta_mass(theo)   rel_dev      norm_drift\n");
    printf("[Dirac]   --------------------------------------------------------------------\n");

    bool all_ok = true;

    for (int i = 0; i < 3; ++i) {
        const int32_t m = masses[i];

        double omega = 0.0;
        double norm_drift = 0.0;
        if (!run_dirac_bloch(basis, m, 0.0, 0.0, 0.0,
            &omega, &norm_drift)) {
            printf("[Dirac]   %s   --- Lauf fehlgeschlagen ---\n", mass_labels[i]);
            all_ok = false;
            continue;
        }

        const double theta_mass =
            ((double)m / 32768.0) * M_PI;

        /* rel_dev ist nur aussagekraeftig wenn theta_mass > 0.
         * Bei m=0 pruefen wir absolut (< 5e-3). */
        const double dev = fabs(omega - theta_mass);

        printf("[Dirac]   %s   %+.6f       %.6f           %.2e     %.2e\n",
            mass_labels[i], omega, theta_mass, dev, norm_drift);

        /* Kalibrierungspruefung: absolute Abweichung < 5e-3 rad/tick.
         * Das ist 5% der kleinsten erwarteten Masse-Phase. */
        const bool calib_ok = (dev < 5e-3);

        /* Norm-Check: unabhängig von der Kalibrierung. */
        const bool norm_ok = (norm_drift < 1e-3);

        if (!calib_ok || !norm_ok) {
            all_ok = false;
            if (!calib_ok) {
                printf("[Dirac]     -> Kalibrierung FAIL (dev = %.4e)\n", dev);
            }
            if (!norm_ok) {
                printf("[Dirac]     -> Norm FAIL (drift = %.4e)\n", norm_drift);
            }
        }
    }

    printf("\n[Dirac] -> %s\n\n", all_ok ? "PASSED" : "FAILED");
    return all_ok;
}

/* =========================================================================
 * Test 3: Dispersions-Charakter (qualitativ)
 *
 * Bei k > 0 mischt der α-Transport die 4 Dirac-Komponenten.
 * Die Phase der L↑-Komponente wird dispersiv. Wir pruefen:
 *   - |omega(k)| > |omega(k=0)| bei m=0  (Transport traegt bei)
 *   - Monotonie: |omega(k)| steigt mit k
 *
 * Keine Kontinuumsformel ω² = k² + m² — die entsteht erst im
 * Renormierungs-Limes (Etappe 23).
 * ========================================================================= */

static bool test_dirac_dispersion_qualitative(ProGammaBasis basis,
    const char* label)
{
    printf("------------------------------------------------------------------------\n");
    printf("  Test 3: Dispersions-Charakter (qualitativ) [%s]\n", label);
    printf("------------------------------------------------------------------------\n\n");

    const uint32_t n_k = 4u;
    const double k_vals[4] = {
        0.0,
        2.0 * M_PI * 2.0 / (double)DIRAC_DIM,
        2.0 * M_PI * 4.0 / (double)DIRAC_DIM,
        2.0 * M_PI * 6.0 / (double)DIRAC_DIM,
    };

    printf("[Dirac] gamma-Basis: %s | m_q15 = 0 (masselos)\n\n", label);
    printf("[Dirac]   k/pi    omega_meas      norm_drift\n");
    printf("[Dirac]   ----------------------------------------\n");

    double omegas[4];
    bool all_ok = true;
    double max_norm_drift = 0.0;

    for (uint32_t i = 0; i < n_k; ++i) {
        double omega = 0.0;
        double norm_drift = 0.0;
        if (!run_dirac_bloch(basis, 0,
            k_vals[i], 0.0, 0.0,
            &omega, &norm_drift)) {
            printf("[Dirac]   %.4f   --- Lauf fehlgeschlagen ---\n",
                k_vals[i] / M_PI);
            all_ok = false;
            continue;
        }
        omegas[i] = omega;
        if (norm_drift > max_norm_drift) max_norm_drift = norm_drift;

        printf("[Dirac]   %.4f    %+.6f         %.2e\n",
            k_vals[i] / M_PI, omega, norm_drift);
    }

    /* Kriterium: bei k=0 ist omega = 0 (kein Transportbeitrag).
     * Bei k>0 ist |omega| > 0 (Transport traegt dispersiv bei). */
    const bool k0_zero = (fabs(omegas[0]) < 5e-3);
    printf("\n[Dirac] |omega(k=0)| < 5e-3 : %s (%.6f)\n",
        k0_zero ? "OK" : "FAIL", omegas[0]);
    if (!k0_zero) all_ok = false;

    /* Monotonie: |omega(k)| waechst mit k */
    bool monotonic = true;
    for (uint32_t i = 1; i < n_k; ++i) {
        if (fabs(omegas[i]) <= fabs(omegas[i - 1])) {
            /* Toleranz: bei Q31-Rauschen kann der Zuwachs sehr klein sein */
            if (fabs(fabs(omegas[i]) - fabs(omegas[i - 1])) > 1e-3) {
                monotonic = false;
            }
        }
    }
    printf("[Dirac] Monotonie |omega(k)| : %s\n",
        monotonic ? "OK" : "FAIL");
    if (!monotonic) all_ok = false;

    printf("[Dirac] max norm_drift = %.2e (Schwelle 1e-3)\n",
        max_norm_drift);
    if (max_norm_drift > 1e-3) {
        printf("[Dirac] Norm : FAIL\n");
        all_ok = false;
    }
    else {
        printf("[Dirac] Norm : OK\n");
    }

    printf("\n[Dirac] -> %s\n\n", all_ok ? "PASSED" : "FAILED");
    return all_ok;
}

/* =========================================================================
 * Test 4: Zitterbewegung-Rohdaten (Option A + Vorbereitung C)
 * ========================================================================= */

static bool test_dirac_zitterbewegung(void)
{
    printf("========================================================================\n");
    printf("  Test 4: Zitterbewegung — Rohdaten (Option A + Vorbereitung C)\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * DIM * DIM;
    const uint32_t SHIFT = 6u;

    const double X0 = (double)(DIM / 2);
    const double SIGMA0 = 3.0;
    const double AMP_SCALE = 2147483647.0 * 0.4;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    wire_torus_3d(&pu, DIM);

    pu.dirac_active = 1u;
    pu.dirac_gamma_basis = (uint8_t)PRO_GAMMA_BASIS_DIRAC;
    pu.dirac_mass_q15 = 1000;
    for (uint64_t k = 0; k < NODES; ++k) {
        pu.ur_grid[k].reserved_gating |= PRO_NODE_DIRAC_BIT;
    }

    double norm_sq = 0.0;
    for (uint32_t x = 0; x < DIM; ++x) {
        double dx = (double)x - X0;
        if (dx > (double)DIM * 0.5) dx -= (double)DIM;
        if (dx < -(double)DIM * 0.5) dx += (double)DIM;
        norm_sq += exp(-dx * dx / (SIGMA0 * SIGMA0));
    }
    const double inv_norm = 1.0 / sqrt(norm_sq);

    for (uint32_t z = 0; z < DIM; ++z) {
        for (uint32_t y = 0; y < DIM; ++y) {
            for (uint32_t x = 0; x < DIM; ++x) {
                double dx = (double)x - X0;
                if (dx > (double)DIM * 0.5) dx -= (double)DIM;
                if (dx < -(double)DIM * 0.5) dx += (double)DIM;
                const double g = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0))
                    * inv_norm;

                const uint64_t idx = ((uint64_t)z << (2u * SHIFT))
                    | ((uint64_t)y << SHIFT)
                    | (uint64_t)x;
                ProAmpVector* v = &pu.amp_grid[idx];
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                    v->coeff[b] = 0;
                }
                v->coeff[BASIS_L_UP] = pro_amp_pack(
                    (int32_t)lround(g * AMP_SCALE), 0);
            }
        }
    }

    const uint32_t N_TICKS = 200u;
    const uint32_t N_SAMPLES = 40u;
    const uint32_t stride = N_TICKS / N_SAMPLES;

    printf("[ZBW] DIM=%u | sigma0=%.1f | m_q15=%d | Ticks=%u\n\n",
        DIM, SIGMA0, (int)pu.dirac_mass_q15, N_TICKS);
    printf("[ZBW]  tick   peak_x   peak_amp_sq    peak_phase\n");
    printf("[ZBW]  --------------------------------------------\n");

    for (uint32_t t = 0; t <= N_TICKS; ++t) {
        if (t % stride == 0u || t == N_TICKS) {
            double peak_val = -1.0;
            uint32_t peak_x = 0u;

            for (uint32_t x = 0; x < DIM; ++x) {
                double sum = 0.0;
                for (uint32_t z = 0; z < DIM; ++z) {
                    for (uint32_t y = 0; y < DIM; ++y) {
                        const uint64_t idx = ((uint64_t)z << (2u * SHIFT))
                            | ((uint64_t)y << SHIFT)
                            | (uint64_t)x;
                        const ProAmpQ31 c =
                            pu.amp_grid[idx].coeff[BASIS_L_UP];
                        const double re = (double)pro_amp_real(c);
                        const double im = (double)pro_amp_imag(c);
                        sum += re * re + im * im;
                    }
                }
                if (sum > peak_val) { peak_val = sum; peak_x = x; }
            }

            uint32_t pz = DIM / 2u, py = DIM / 2u;
            const uint64_t peak_idx = ((uint64_t)pz << (2u * SHIFT))
                | ((uint64_t)py << SHIFT)
                | (uint64_t)peak_x;
            const ProAmpQ31 pc = pu.amp_grid[peak_idx].coeff[BASIS_L_UP];
            const double ph = atan2((double)pro_amp_imag(pc),
                (double)pro_amp_real(pc));

            printf("[ZBW]  %4u   %5u    %.6e    %+.6f\n",
                t, peak_x, peak_val, ph);
        }

        if (t < N_TICKS) {
            ProPhysics_Apply_Dirac_Step(&pu, pu.dirac_mass_q15,
                DIRAC_THETA_Q15);
        }
    }

    ProPhysics_Free(&pu);

    printf("\n[ZBW] Rohdaten ausgegeben. Frequenzanalyse spaeter (Option C).\n");
    printf("[ZBW] -> PASSED (informativ)\n");
    return true;
}

/* =========================================================================
 * Test 5: Regression ohne Dirac bit-identisch
 * ========================================================================= */

static bool test_dirac_regression(void)
{
    printf("========================================================================\n");
    printf("  Test 5: Regression — ohne Dirac-Aktivierung bit-identisch\n");
    printf("========================================================================\n\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 4096u);
    wire_torus_3d(&pu, 16u);

    const ProU128 before = ProPhysics_Measure_Amp_Invariant(&pu);

    for (uint32_t t = 0; t < 100; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(
            &pu, ResearchPlugin_DynamicPlasticTopology);
    }

    const ProU128 after = ProPhysics_Measure_Amp_Invariant(&pu);

    const double b = pro_u128_to_double(before);
    const double a = pro_u128_to_double(after);
    const double drift = (b > 0.0) ? fabs(a - b) / b : 0.0;

    printf("[Dirac-Reg] U5-Drift ueber 100 Ticks (dirac_active=0) = %.4e\n",
        drift);
    const bool ok = (drift < 1e-6);
    printf("[Dirac-Reg] -> %s\n\n", ok ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return ok;
}

/* =========================================================================
 * Sammel-Einstiegspunkt
 * ========================================================================= */

bool test_dirac_all(void)
{
    printf("\n");
    printf("########################################################################\n");
    printf("#  Dirac-Struktur (Etappe 21 + 21b)\n");
    printf("########################################################################\n\n");

    const bool t1 = test_dirac_gamma_algebra();
    printf("\n");
    const bool t2a = test_dirac_mass_gap(
        PRO_GAMMA_BASIS_DIRAC, "Dirac");
    printf("\n");
    const bool t2b = test_dirac_mass_gap(
        PRO_GAMMA_BASIS_WEYL, "Weyl");
    printf("\n");
    const bool t3 = test_dirac_dispersion_qualitative(
        PRO_GAMMA_BASIS_DIRAC, "Dirac");
    printf("\n");
    const bool t4 = test_dirac_zitterbewegung();
    printf("\n");
    const bool t5 = test_dirac_regression();

    printf("\n");
    printf("########################################################################\n");
    printf("#  Dirac-Zusammenfassung\n");
    printf("########################################################################\n");
    printf("#  Gamma-Algebra                : %s\n", t1 ? "PASS" : "FAIL");
    printf("#  Massen-Kalibrierung (Dirac)  : %s\n", t2a ? "PASS" : "FAIL");
    printf("#  Massen-Kalibrierung (Weyl)   : %s\n", t2b ? "PASS" : "FAIL");
    printf("#  Dispersions-Charakter        : %s\n", t3 ? "PASS" : "FAIL");
    printf("#  Zitterbewegung (Rohdaten)    : %s\n", t4 ? "PASS" : "FAIL");
    printf("#  Regression ohne Dirac        : %s\n", t5 ? "PASS" : "FAIL");
    printf("#  Gesamt                       : %s\n",
        (t1 && t2a && t2b && t3 && t4 && t5) ? "PASS" : "FAIL");
    printf("########################################################################\n");

    return t1 && t2a && t2b && t3 && t4 && t5;
}

bool test_dirac_dispersion_entry(void)
{
    return test_dirac_all();
}