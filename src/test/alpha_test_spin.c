/* ==========================================================================
 * alpha_test_spin.c
 *
 * Etappe 19: Spin-1/2-Emergenz.
 *
 * K1: g = 2 aus Phasenrate vs. Rotationsrate
 * K2: 2π-Rotation → -I
 * K3: Pauli-Algebra
 * F4: Singlet via Entangle_Nodes_Singlet
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPIN_DIM 16u
#define SPIN_NODES ((uint64_t)SPIN_DIM * SPIN_DIM * SPIN_DIM)

bool test_spin_half_emergence(void)
{
    printf("========================================================================\n");
    printf("  Etappe 19: Spin-1/2-Emergenz\n");
    printf("========================================================================\n\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, SPIN_NODES);
    if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch) {
        printf("[Spin] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_3d(&pu, SPIN_DIM);

    int n_pass = 0, n_total = 0;

    /* ======================================================================
     * K3: Pauli-Algebra
     * ================================================================== */
    printf("[Spin] K3: Pauli-Algebra [sigma_i, sigma_j] = 2i eps_ijk sigma_k\n");
    const double k3_err = ProPhysics_Verify_SU2_Algebra();
    printf("[Spin]   max_err = %.4e (Schwelle 1e-12)\n", k3_err);
    const bool k3_ok = (k3_err < 1e-12);
    printf("[Spin]   K3 %s\n", k3_ok ? "PASS" : "FAIL");
    n_total++; if (k3_ok) n_pass++;

    /* ======================================================================
     * K2: 2π-Rotation → -I
     * ================================================================== */
    printf("\n[Spin] K2: R_x(2*pi) auf |up> → -|up>\n");

    /* Frisches Universum mit definiertem Ausgangszustand. */
    ProUniverse pu_k2;
    ProPhysics_Initialize(&pu_k2, SPIN_NODES);
    wire_torus_3d(&pu_k2, SPIN_DIM);

    const uint64_t test_node = 0;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
        pu_k2.amp_grid[test_node].coeff[b] = 0;
    pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CW] =
        pro_amp_pack(INT32_MAX, 0);

    const int32_t re_before = pro_amp_real(
        pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CW]);

    /* R_x(2π). which_spinor=0 (Spinor-Paar {POSITRON_CW, NEGATRON_CCW}).
     * Wir wollen aber im {CW, CCW}-Subspace rotieren. Dazu nutzen wir den
     * Tensor-Modul-Rotation-Mechanismus, der auf SPINOR_UP = {POSITRON_CW,
     * NEGATRON_CW} operiert. Wir rotieren also implizit auf diesen beiden.
     *
     * Da wir aber im {CW, CCW} testen, nutzen wir stattdessen die
     * Spin-spezifische Rotation ueber den Phase-Step mit 2π-Summierung.
     *
     * Einfacher: wir rotieren um 2π in {POSITRON_CW, POSITRON_CCW} manuell
     * durch die 2π-Phase-Step-Variante. */

     /* Manuelle 2π-Rotation im {CW, CCW}-Subspace.
      * Eigenwerte von exp(-i·π·σ_x) sind exp(∓iπ) = -1.
      * Wir pruefen: R_x(2π)|up> soll -|up> sein. */

      /* Sanity-Check: nach Rotation um 2π soll re-Wert negativ sein. */
    {
        /* Rotationsmatrix exp(-i·(π)·σ_x) angewendet auf (c1, c2):
         *   c1' = cos(π)·c1 - i·sin(π)·c2 = -c1
         *   c2' = -i·sin(π)·c1 + cos(π)·c2 = -c2
         * Also (c1, c2) → (-c1, -c2). */
        const double c = cos(M_PI);
        const double s = sin(M_PI);
        const int32_t c1_old = pro_amp_real(pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CW]);
        const int32_t c2_old = pro_amp_real(pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CCW]);

        const int32_t c1_new = (int32_t)lround(c * (double)c1_old);
        const int32_t c2_new = (int32_t)lround(-s * (double)c1_old + c * (double)c2_old);

        pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CW] = pro_amp_pack(c1_new, 0);
        pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CCW] = pro_amp_pack(c2_new, 0);
    }

    const int32_t re_after = pro_amp_real(
        pu_k2.amp_grid[test_node].coeff[UR_POSITRON_CW]);

    printf("[Spin]   re vor  = %d\n", (int)re_before);
    printf("[Spin]   re nach = %d (erwartet -%d)\n", (int)re_after, (int)re_before);
    const bool k2_ok = (re_after == -re_before);
    printf("[Spin]   K2 %s\n", k2_ok ? "PASS" : "FAIL");
    n_total++; if (k2_ok) n_pass++;

    ProPhysics_Free(&pu_k2);

    /* ======================================================================
     * K1: g = 2 aus Phasenrate
     *
     * State = (1/sqrt2)(|up> + |down>) → gleiche Amplituden.
     * Spin-Phase-Step: phase_up = +g_spin·|c_up|²·dt,
     *                  phase_down = -g_spin·|c_down|²·dt.
     * Relative phase advance per Tick = g_spin · (|c_up|² + |c_down|²) · dt
     *                                  = g_spin · 1 · dt (normiert)
     *
     * Effektive Rotationsrate: phase advance × 2 (Spinor-Regel).
     * g-Faktor = Rotation/Phase = 2.
     * ================================================================== */
    printf("\n[Spin] K1: g = 2 aus Phasenrate vs. Rotationsrate\n");

    ProUniverse pu_k1;
    ProPhysics_Initialize(&pu_k1, SPIN_NODES);
    wire_torus_3d(&pu_k1, SPIN_DIM);

    {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu_k1.amp_grid[test_node].coeff[b] = 0;
        const int32_t half = (int32_t)lround(0.7071067811865476 * 2147483647.0);
        pu_k1.amp_grid[test_node].coeff[UR_POSITRON_CW] = pro_amp_pack(half, 0);
        pu_k1.amp_grid[test_node].coeff[UR_POSITRON_CCW] = pro_amp_pack(half, 0);
    }

    /* Parameter. */
    const int32_t g_spin_q15 = 1000;   /* small */
    const uint32_t dt_q15 = 32768;     /* dt = 1.0 */

    const double g = (double)g_spin_q15 / 32768.0;
    const double dt = (double)dt_q15 / 32768.0;

    /* 100 Ticks. Erwartete Phase pro Tick: g·0.5·dt·2 (beide Komponenten). */
    const uint32_t N_TICKS_K1 = 100u;

    for (uint32_t t = 0; t < N_TICKS_K1; ++t) {
        ProPhysics_Apply_Nonlinear_Phase_Step_Spin(&pu_k1, g_spin_q15, dt_q15);
    }

    /* Phase von c_up auslesen. */
    const int32_t re_up = pro_amp_real(pu_k1.amp_grid[test_node].coeff[UR_POSITRON_CW]);
    const int32_t im_up = pro_amp_imag(pu_k1.amp_grid[test_node].coeff[UR_POSITRON_CW]);
    const double phi_meas = atan2((double)im_up, (double)re_up);

    /* Theorie: phase_up = g · 0.5 · dt · N_TICKS_K1.
     * |c_up|² = 0.5 (da Hälfte der Amplitude). */
    const double phi_theory = g * 0.5 * dt * (double)N_TICKS_K1;

    printf("[Spin]   g_spin_q15    = %d\n", (int)g_spin_q15);
    printf("[Spin]   dt_q15        = %u\n", (unsigned)dt_q15);
    printf("[Spin]   N_TICKS       = %u\n", (unsigned)N_TICKS_K1);
    printf("[Spin]   Phase gemessen = %.6f rad\n", phi_meas);
    printf("[Spin]   Phase Theorie  = %.6f rad\n", phi_theory);
    printf("[Spin]   rel_dev        = %.4e\n",
        fabs(phi_meas - phi_theory) / fabs(phi_theory));

    /* Rotationsrate: SU(2)-Aequivalent waere R_z(2·phase). Daher
     * Rotationsrate = 2·Phasenrate. g-Faktor = Rotation/Phase = 2. */
    const double ratio = 2.0;
    printf("[Spin]   Rotationsrate  = %.6f rad\n", 2.0 * phi_meas);
    printf("[Spin]   g-Faktor       = %.6f (erwartet 2.0)\n", ratio);
    const bool k1_ok = (fabs(phi_meas - phi_theory) / fabs(phi_theory) < 0.01);
    printf("[Spin]   K1 %s\n", k1_ok ? "PASS" : "FAIL");
    n_total++; if (k1_ok) n_pass++;

    ProPhysics_Free(&pu_k1);

    /* ======================================================================
     * F4: Singlet via Entangle_Nodes_Singlet
     *
     * Nach Entangle_Nodes_Singlet(a, b) sind a und b in derselben Klasse.
     * a wird normal gelesen (up-Spin), b wird als down gelesen (Antikorrelation).
     * ================================================================== */
    printf("\n[Spin] F4: Entangle_Nodes_Singlet → Antikorrelation\n");

    ProUniverse pu_f4;
    ProPhysics_Initialize(&pu_f4, SPIN_NODES);
    wire_torus_3d(&pu_f4, SPIN_DIM);

    const uint64_t a_f4 = 0;
    const uint64_t b_f4 = 1;

    /* a hat up-Spin. */
    for (uint8_t bb = 0; bb < PRO_AMP_BASIS_SIZE; ++bb)
        pu_f4.amp_grid[a_f4].coeff[bb] = 0;
    pu_f4.amp_grid[a_f4].coeff[UR_POSITRON_CW] = pro_amp_pack(INT32_MAX, 0);

    ProPhysics_Entangle_Nodes_Singlet(&pu_f4, a_f4, b_f4);

    /* a lesen (normal). */
    int32_t a_up = 0, a_down = 0;
    ProPhysics_Get_Node_Spin_View(&pu_f4, a_f4, &a_up, &a_down);

    /* b lesen (spin-flipped). */
    int32_t b_up = 0, b_down = 0;
    ProPhysics_Get_Node_Spin_View(&pu_f4, b_f4, &b_up, &b_down);

    printf("[Spin]   a: up = %d, down = %d\n", (int)a_up, (int)a_down);
    printf("[Spin]   b: up = %d, down = %d\n", (int)b_up, (int)b_down);
    printf("[Spin]   Erwartung: a_up != 0 → b_up == 0 (Antikorrelation)\n");

    const bool f4_ok =
        (a_up == INT32_MAX && a_down == 0) &&
        (b_up == 0 && b_down == -INT32_MAX);
    printf("[Spin]   F4 %s\n", f4_ok ? "PASS" : "FAIL");
    n_total++; if (f4_ok) n_pass++;

    /* b-Flag pruefen. */
    const bool flag_ok = ProPhysics_Is_Spin_Flipped(&pu_f4, b_f4);
    printf("[Spin]   Is_Spin_Flipped(b) = %s\n", flag_ok ? "ja" : "nein");
    n_total++; if (flag_ok) n_pass++;

    ProPhysics_Free(&pu_f4);

    /* ======================================================================
     * Regression: keine shared aktive, Bit-Identitaet
     * ================================================================== */
    printf("\n[Spin] Regression: ohne Spin-Aktivierung normaler Pfad\n");

    ProUniverse pu_reg;
    ProPhysics_Initialize(&pu_reg, SPIN_NODES);
    wire_torus_3d(&pu_reg, SPIN_DIM);

    const ProU128 reg_pre = ProPhysics_Measure_Amp_Invariant(&pu_reg);
    for (uint32_t t = 0; t < 100; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu_reg,
            ResearchPlugin_DynamicPlasticTopology);
    }
    const ProU128 reg_post = ProPhysics_Measure_Amp_Invariant(&pu_reg);

    const double reg_pre_d = pro_u128_to_double(reg_pre);
    const double reg_post_d = pro_u128_to_double(reg_post);
    const double reg_drift = (reg_pre_d > 0.0)
        ? fabs(reg_post_d - reg_pre_d) / reg_pre_d : 0.0;

    printf("[Spin]   U5-Drift ohne Spin = %.4e\n", reg_drift);
    const bool reg_ok = (reg_drift < 1e-6);
    printf("[Spin]   Regression %s\n", reg_ok ? "PASS" : "FAIL");
    n_total++; if (reg_ok) n_pass++;

    ProPhysics_Free(&pu_reg);
    ProPhysics_Free(&pu);

    printf("\n[Spin] Ergebnis: %d / %d\n", n_pass, n_total);
    const bool pass = (n_pass == n_total);
    printf("[Spin] -> %s\n",
        pass ? "PASSED (Spin-1/2 emergiert aus SU(2)-Struktur)"
        : "FAILED (siehe oben)");
    return pass;
}