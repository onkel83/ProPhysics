/* ==========================================================================
 * alpha_test_shared.c
 *
 * Etappe 18c/18e: Shared Reference (U4) Test + Formula Tournament.
 *
 * Enthaelt:
 *   test_shared_reference        — U4-Mechanik + 18e Tick-Integration (T1-T11)
 *   test_shared_formula_tournament — Formelvergleich (F1-F7), Etappe 18e-Probe
 *
 * Etappe-18e-Fixes in test_shared_reference:
 *   T6: Q31-grosse Testzustaende (10^9 statt 10^3)
 *   T9: prueft |psi_B|^2 (re^2 + im^2) statt nur den Realteil
 *   T10: Klassen nicht-ueberlappend (Abstand 6 statt 4)
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SR_DIM 32u
#define SR_NODES ((uint64_t)SR_DIM * SR_DIM * SR_DIM)

 /* Q31-Skala fuer U5-Stabilitaetstests. */
#define SR_MARKER_Q31 (1 << 20)

/* ==========================================================================
 * Helfer
 * ========================================================================== */

static void sr_set_test_state(ProUniverse* pu, uint64_t k, int32_t marker)
{
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu->amp_grid[k].coeff[b] = 0;
    }
    pu->amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(marker, 0);
    pu->amp_grid[k].coeff[UR_NEGATRON_CCW] = pro_amp_pack(marker / 2, 0);
}

static void sr_set_test_state_q31(ProUniverse* pu, uint64_t k,
    int32_t base_marker)
{
    const int32_t pc = base_marker * SR_MARKER_Q31;
    const int32_t nc = (base_marker / 2) * SR_MARKER_Q31;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu->amp_grid[k].coeff[b] = 0;
    }
    pu->amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(pc, 0);
    pu->amp_grid[k].coeff[UR_NEGATRON_CCW] = pro_amp_pack(nc, 0);
}

static bool sr_vectors_equal(const ProAmpVector* a, const ProAmpVector* b)
{
    return memcmp(a, b, sizeof(ProAmpVector)) == 0;
}

static bool sr_vector_is_state(const ProAmpVector* v,
    int32_t expected_re_pc, int32_t expected_re_nc)
{
    if (pro_amp_real(v->coeff[UR_POSITRON_CW]) != expected_re_pc) return false;
    if (pro_amp_real(v->coeff[UR_NEGATRON_CCW]) != expected_re_nc) return false;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        if (b == UR_POSITRON_CW || b == UR_NEGATRON_CCW) continue;
        if (v->coeff[b] != 0) return false;
    }
    return true;
}

/* ==========================================================================
 * T1-T11: Shared Reference + 18e Tick-Integration
 * ========================================================================== */

bool test_shared_reference(void)
{
    printf("========================================================================\n");
    printf("  Etappe 18c/18e: Shared Reference (U4) Test\n");
    printf("========================================================================\n\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, SR_NODES);
    if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch
        || !pu.shared.parent) {
        printf("[SR] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_3d(&pu, SR_DIM);

    int n_pass = 0;
    int n_total = 0;

    /* --- T1 --- */
    printf("[SR] T1: Entangle_Nodes(0, 1) -- byte-identisch\n");
    sr_set_test_state(&pu, 0, 1000);
    sr_set_test_state(&pu, 1, 2000);

    const bool t1_diff_before = !sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[1]);
    printf("[SR]   vorher: Knoten 0 != 1 : %s\n", t1_diff_before ? "OK" : "FAILED");
    n_total++; if (t1_diff_before) n_pass++;

    const bool t1_ok = ProPhysics_Entangle_Nodes(&pu, 0, 1);
    printf("[SR]   Entangle_Nodes : %s\n", t1_ok ? "OK" : "FAILED");
    n_total++; if (t1_ok) n_pass++;

    const bool t1_same = sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[1]);
    printf("[SR]   byte-identisch : %s\n", t1_same ? "OK" : "FAILED");
    n_total++; if (t1_same) n_pass++;

    const bool t1_a_wins = sr_vector_is_state(&pu.amp_grid[1], 1000, 500);
    printf("[SR]   a gewinnt      : %s\n", t1_a_wins ? "OK" : "FAILED");
    n_total++; if (t1_a_wins) n_pass++;

    const bool t1_is_ent = ProPhysics_Is_Entangled(&pu, 0, 1);
    const bool t1_not_ent = !ProPhysics_Is_Entangled(&pu, 0, 2);
    printf("[SR]   Is_Entangled(0,1)=T,(0,2)=F : %s / %s\n",
        t1_is_ent ? "OK" : "FAILED", t1_not_ent ? "OK" : "FAILED");
    n_total++; if (t1_is_ent) n_pass++;
    n_total++; if (t1_not_ent) n_pass++;

    /* --- T2 --- */
    printf("\n[SR] T2: Chain 0-1-2-3\n");
    sr_set_test_state(&pu, 2, 3000);
    sr_set_test_state(&pu, 3, 4000);
    ProPhysics_Entangle_Nodes(&pu, 1, 2);
    ProPhysics_Entangle_Nodes(&pu, 2, 3);

    const bool t2_all = sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[2])
        && sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[3]);
    printf("[SR]   alle 0..3 identisch : %s\n", t2_all ? "OK" : "FAILED");
    n_total++; if (t2_all) n_pass++;

    const uint64_t t2_count = ProPhysics_Shared_Class_Count(&pu);
    const uint64_t t2_expected = SR_NODES - 3u;
    printf("[SR]   Klassen-Anzahl %llu/%llu : %s\n",
        (unsigned long long)t2_count, (unsigned long long)t2_expected,
        t2_count == t2_expected ? "OK" : "FAILED");
    n_total++; if (t2_count == t2_expected) n_pass++;

    /* --- T3 --- */
    printf("\n[SR] T3: Union {4,5} und {6,7}\n");
    sr_set_test_state(&pu, 4, 5000);
    sr_set_test_state(&pu, 5, 6000);
    sr_set_test_state(&pu, 6, 7000);
    sr_set_test_state(&pu, 7, 8000);

    ProPhysics_Entangle_Nodes(&pu, 4, 5);
    ProPhysics_Entangle_Nodes(&pu, 6, 7);

    const bool t3_sep = !sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[6]);
    printf("[SR]   {4,5} != {6,7} : %s\n", t3_sep ? "OK" : "FAILED");
    n_total++; if (t3_sep) n_pass++;

    ProPhysics_Entangle_Nodes(&pu, 5, 6);
    const bool t3_all_same =
        sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[6])
        && sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[7])
        && sr_vectors_equal(&pu.amp_grid[5], &pu.amp_grid[4]);
    printf("[SR]   alle 4..7 identisch : %s\n", t3_all_same ? "OK" : "FAILED");
    n_total++; if (t3_all_same) n_pass++;

    /* --- T4 --- */
    printf("\n[SR] T4: 100 Ticks -- Klassen synchron\n");

    for (uint32_t t = 0; t < 100; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const bool t4_0_3 = sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[1])
        && sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[2])
        && sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[3]);
    const bool t4_4_7 = sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[5])
        && sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[6])
        && sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[7]);
    const bool t4_sep = !sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[4]);

    printf("[SR]   {0,1,2,3} synchron : %s\n", t4_0_3 ? "OK" : "FAILED");
    n_total++; if (t4_0_3) n_pass++;
    printf("[SR]   {4,5,6,7} synchron : %s\n", t4_4_7 ? "OK" : "FAILED");
    n_total++; if (t4_4_7) n_pass++;
    printf("[SR]   Klassen untersch.  : %s\n", t4_sep ? "OK" : "FAILED");
    n_total++; if (t4_sep) n_pass++;

    /* --- T5 --- */
    printf("\n[SR] T5: Dissociate_Node(2)\n");
    const uint64_t t5_rep_before = ProPhysics_Get_Representative(&pu, 0);
    printf("[SR]   Rep von 0 vorher : %llu\n",
        (unsigned long long)t5_rep_before);

    const bool t5_ok = ProPhysics_Dissociate_Node(&pu, 2);
    printf("[SR]   Dissociate       : %s\n", t5_ok ? "OK" : "FAILED");
    n_total++; if (t5_ok) n_pass++;

    const bool t5_2_indep = (ProPhysics_Get_Representative(&pu, 2) == 2u);
    printf("[SR]   2 eigene Wurzel  : %s\n", t5_2_indep ? "OK" : "FAILED");
    n_total++; if (t5_2_indep) n_pass++;

    const bool t5_others = sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[1])
        && sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[3]);
    printf("[SR]   {0,1,3} synchron : %s\n", t5_others ? "OK" : "FAILED");
    n_total++; if (t5_others) n_pass++;

    const bool t5_not_ent = !ProPhysics_Is_Entangled(&pu, 2, 0);
    printf("[SR]   Is_Entangled(2,0)=F : %s\n", t5_not_ent ? "OK" : "FAILED");
    n_total++; if (t5_not_ent) n_pass++;

    /* --- T6: 1000 Ticks, Q31-grosse Zustände --- */
    printf("\n[SR] T6: 1000 Ticks -- U5-Erhaltung (Q31-Zustand)\n");

    sr_set_test_state_q31(&pu, 0, 1000);
    ProPhysics_Shared_Sync(&pu);
    sr_set_test_state_q31(&pu, 2, 1000);
    sr_set_test_state_q31(&pu, 4, 5000);
    ProPhysics_Shared_Sync(&pu);

    const ProU128 t6_before = ProPhysics_Measure_Amp_Invariant(&pu);
    for (uint32_t t = 0; t < 1000; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }
    const ProU128 t6_after = ProPhysics_Measure_Amp_Invariant(&pu);

    const double t6_b = pro_u128_to_double(t6_before);
    const double t6_a = pro_u128_to_double(t6_after);
    const double t6_drift = (t6_b > 0.0) ? (fabs(t6_a - t6_b) / t6_b) : 0.0;

    printf("[SR]   Shared_Tick_Reps Aufrufe : %llu\n",
        (unsigned long long)pu._shared_tick_count);
    printf("[SR]   U5-Drift ueber 1000 Ticks : %.4e\n", t6_drift);
    const bool t6_u5_ok = (t6_drift < 1e-3);
    printf("[SR]   U5 erhalten : %s\n", t6_u5_ok ? "OK" : "FAILED");
    n_total++; if (t6_u5_ok) n_pass++;

    const bool t6_class_0_1_3 =
        sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[1])
        && sr_vectors_equal(&pu.amp_grid[0], &pu.amp_grid[3]);
    const bool t6_class_4_7 =
        sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[5])
        && sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[6])
        && sr_vectors_equal(&pu.amp_grid[4], &pu.amp_grid[7]);

    printf("[SR]   {0,1,3} konsistent : %s\n", t6_class_0_1_3 ? "OK" : "FAILED");
    n_total++; if (t6_class_0_1_3) n_pass++;
    printf("[SR]   {4,5,6,7} konsistent : %s\n", t6_class_4_7 ? "OK" : "FAILED");
    n_total++; if (t6_class_4_7) n_pass++;

    /* --- T7 --- */
    printf("\n[SR] T7: Regression -- nichts verschraenkt\n");

    ProUniverse pu2;
    ProPhysics_Initialize(&pu2, SR_NODES);
    wire_torus_3d(&pu2, SR_DIM);

    const bool t7_init_inactive =
        (!pu2.shared.active) && (pu2.shared.parent != NULL);
    printf("[SR]   active=0, parent!=NULL : %s\n", t7_init_inactive ? "OK" : "FAILED");
    n_total++; if (t7_init_inactive) n_pass++;

    uint64_t t7_singletons = 0;
    for (uint64_t k = 0; k < SR_NODES; ++k) {
        if (pu2.shared.parent[k] == k) t7_singletons++;
    }
    const bool t7_all_self = (t7_singletons == SR_NODES);
    printf("[SR]   alle Singleton : %s\n", t7_all_self ? "OK" : "FAILED");
    n_total++; if (t7_all_self) n_pass++;

    const uint64_t t7_classes = ProPhysics_Shared_Class_Count(&pu2);
    const bool t7_count_ok = (t7_classes == SR_NODES);
    printf("[SR]   Class_Count == total : %s\n", t7_count_ok ? "OK" : "FAILED");
    n_total++; if (t7_count_ok) n_pass++;

    ProPhysics_Free(&pu2);

    /* --- T9: Klassen-Tick tauscht Amplitude --- */
    printf("\n[SR] T9: Klassen-Tick tauscht Amplitude (Etappe 18e)\n");

    ProUniverse pu9;
    ProPhysics_Initialize(&pu9, SR_NODES);
    wire_torus_3d(&pu9, SR_DIM);

    const uint32_t shift9 = 5u;
    const uint64_t a0_9 = ((uint64_t)16u << (2u * shift9)) |
        ((uint64_t)16u << shift9) | (uint64_t)10u;
    const uint64_t a1_9 = ((uint64_t)16u << (2u * shift9)) |
        ((uint64_t)16u << shift9) | (uint64_t)11u;
    const uint64_t b0_9 = ((uint64_t)16u << (2u * shift9)) |
        ((uint64_t)16u << shift9) | (uint64_t)12u;
    const uint64_t b1_9 = ((uint64_t)16u << (2u * shift9)) |
        ((uint64_t)16u << shift9) | (uint64_t)13u;

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu9.amp_grid[a0_9].coeff[b] = 0;
        pu9.amp_grid[b0_9].coeff[b] = 0;
    }
    pu9.amp_grid[a0_9].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(16384), 0);

    ProPhysics_Entangle_Nodes(&pu9, a0_9, a1_9);
    ProPhysics_Entangle_Nodes(&pu9, b0_9, b1_9);

    const ProU128 u5_pre = ProPhysics_Measure_Amp_Invariant(&pu9);

    ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu9,
        ResearchPlugin_DynamicPlasticTopology);

    const ProU128 u5_post = ProPhysics_Measure_Amp_Invariant(&pu9);
    const double u5_pre_d = pro_u128_to_double(u5_pre);
    const double u5_post_d = pro_u128_to_double(u5_post);
    const double u5_drift_t9 =
        (u5_pre_d > 0.0) ? fabs(u5_post_d - u5_pre_d) / u5_pre_d : 0.0;

    printf("[SR]   U5-Drift nach 1 Tick : %.4e\n", u5_drift_t9);
    const bool t9_u5_ok = (u5_drift_t9 < 1e-6);
    printf("[SR]   U5 erhalten : %s\n", t9_u5_ok ? "OK" : "FAILED");
    n_total++; if (t9_u5_ok) n_pass++;

    const int64_t b_re_post = (int64_t)pro_amp_real(
        pu9.amp_grid[b0_9].coeff[UR_POSITRON_CW]);
    const int64_t b_im_post = (int64_t)pro_amp_imag(
        pu9.amp_grid[b0_9].coeff[UR_POSITRON_CW]);
    const int64_t b_norm_sq = b_re_post * b_re_post + b_im_post * b_im_post;

    printf("[SR]   |psi_B|^2 nach Tick : %lld (re=%lld im=%lld)\n",
        (long long)b_norm_sq, (long long)b_re_post, (long long)b_im_post);
    const bool t9_transfer = (b_norm_sq > 0);
    printf("[SR]   Amplitude zwischen Klassen getauscht : %s\n",
        t9_transfer ? "OK" : "FAILED");
    n_total++; if (t9_transfer) n_pass++;

    const bool t9_class_a = sr_vectors_equal(&pu9.amp_grid[a0_9],
        &pu9.amp_grid[a1_9]);
    const bool t9_class_b = sr_vectors_equal(&pu9.amp_grid[b0_9],
        &pu9.amp_grid[b1_9]);
    printf("[SR]   Klasse A konsistent : %s\n", t9_class_a ? "OK" : "FAILED");
    n_total++; if (t9_class_a) n_pass++;
    printf("[SR]   Klasse B konsistent : %s\n", t9_class_b ? "OK" : "FAILED");
    n_total++; if (t9_class_b) n_pass++;

    ProPhysics_Free(&pu9);

    /* --- T10: U5-Erhaltung ueber 500 Ticks, nicht-ueberlappende Klassen --- */
    printf("\n[SR] T10: U5-Erhaltung ueber 500 Ticks (nicht-ueberlappend)\n");

    ProUniverse pu10;
    ProPhysics_Initialize(&pu10, SR_NODES);
    wire_torus_3d(&pu10, SR_DIM);

    const uint32_t class_x[5] = { 8u, 14u, 20u, 26u, 2u };
    for (uint32_t c = 0; c < 5; ++c) {
        const uint64_t base = ((uint64_t)16u << (2u * shift9)) |
            ((uint64_t)16u << shift9) | (uint64_t)class_x[c];
        for (uint64_t k = 1; k < 4; ++k) {
            ProPhysics_Entangle_Nodes(&pu10, base, base + k);
        }
    }

    {
        const uint64_t base0 = ((uint64_t)16u << (2u * shift9)) |
            ((uint64_t)16u << shift9) | (uint64_t)class_x[0];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            pu10.amp_grid[base0].coeff[b] = 0;
        }
        pu10.amp_grid[base0].coeff[UR_POSITRON_CW] =
            pro_amp_pack((int32_t)(1000LL * SR_MARKER_Q31), 0);
    }
    ProPhysics_Shared_Sync(&pu10);

    const ProU128 t10_pre = ProPhysics_Measure_Amp_Invariant(&pu10);
    for (uint32_t t = 0; t < 500; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu10,
            ResearchPlugin_DynamicPlasticTopology);
    }
    const ProU128 t10_post = ProPhysics_Measure_Amp_Invariant(&pu10);

    const double t10_pre_d = pro_u128_to_double(t10_pre);
    const double t10_post_d = pro_u128_to_double(t10_post);
    const double t10_drift =
        (t10_pre_d > 0.0) ? fabs(t10_post_d - t10_pre_d) / t10_pre_d : 0.0;

    printf("[SR]   U5-Drift ueber 500 Ticks : %.4e\n", t10_drift);
    const bool t10_ok = (t10_drift < 1e-3);
    printf("[SR]   U5 erhalten : %s\n", t10_ok ? "OK" : "FAILED");
    n_total++; if (t10_ok) n_pass++;

    ProPhysics_Free(&pu10);

    /* --- T11: Regression ohne shared --- */
    printf("\n[SR] T11: Regression -- U5 ohne shared\n");

    ProUniverse pu11;
    ProPhysics_Initialize(&pu11, SR_NODES);
    wire_torus_3d(&pu11, SR_DIM);

    const ProU128 t11_pre = ProPhysics_Measure_Amp_Invariant(&pu11);
    for (uint32_t t = 0; t < 200; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu11,
            ResearchPlugin_DynamicPlasticTopology);
    }
    const ProU128 t11_post = ProPhysics_Measure_Amp_Invariant(&pu11);

    const double t11_pre_d = pro_u128_to_double(t11_pre);
    const double t11_post_d = pro_u128_to_double(t11_post);
    const double t11_drift =
        (t11_pre_d > 0.0) ? fabs(t11_post_d - t11_pre_d) / t11_pre_d : 0.0;

    printf("[SR]   U5-Drift ueber 200 Ticks : %.4e\n", t11_drift);
    const bool t11_ok = (t11_drift < 1e-6);
    printf("[SR]   U5 erhalten : %s\n", t11_ok ? "OK" : "FAILED");
    n_total++; if (t11_ok) n_pass++;

    ProPhysics_Free(&pu11);

    ProPhysics_Free(&pu);

    printf("\n[SR] Ergebnis: %d / %d\n", n_pass, n_total);
    const bool pass = (n_pass == n_total);
    printf("[SR] -> %s\n",
        pass ? "PASSED (U4 als shared reference realisiert + 18e Tick-Integration)"
        : "FAILED (siehe oben)");
    return pass;
}

/* ==========================================================================
 * T8: Formula Tournament
 *
 * dim=32, 3D-Torus. Zwei Klassen A={a0,a1}, B={b0,b1} mit N_AB=1.
 * Start: A-Rep hat Q31-grossen Zustand. 200 Ticks Klassen-Rotation mit
 * theta_eff(formel_id). Messung: Tick bei frac_B > 0.5.
 * Referenz: unshared 2-Knoten mit theta.
 *
 * Sieger F7: theta_eff = theta (N_AB=1, |A|=|B|=2).
 * ========================================================================== */

#define TOURN_DIM 32u
#define TOURN_NODES ((uint64_t)TOURN_DIM * TOURN_DIM * TOURN_DIM)
#define TOURN_N_TICKS 200u
#define TOURN_THETA_Q15 2000u

static uint64_t tourn_idx3d(uint32_t x, uint32_t y, uint32_t z, uint32_t shift)
{
    return ((uint64_t)z << (2u * shift)) | ((uint64_t)y << shift) | (uint64_t)x;
}

static void tourn_rotate_class_pair(ProUniverse* pu,
    uint64_t rep_A, uint64_t rep_B,
    double theta_eff)
{
    const double c = cos(theta_eff);
    const double s = sin(theta_eff);
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        const int64_t ar = (int64_t)pro_amp_real(pu->amp_grid[rep_A].coeff[b]);
        const int64_t ai = (int64_t)pro_amp_imag(pu->amp_grid[rep_A].coeff[b]);
        const int64_t br = (int64_t)pro_amp_real(pu->amp_grid[rep_B].coeff[b]);
        const int64_t bi = (int64_t)pro_amp_imag(pu->amp_grid[rep_B].coeff[b]);

        int64_t na_re = (int64_t)llround(c * (double)ar - s * (double)bi);
        int64_t na_im = (int64_t)llround(c * (double)ai + s * (double)br);
        int64_t nb_re = (int64_t)llround(c * (double)br - s * (double)ai);
        int64_t nb_im = (int64_t)llround(c * (double)bi + s * (double)ar);

        if (na_re > INT32_MAX) na_re = INT32_MAX;
        if (na_re < INT32_MIN) na_re = INT32_MIN;
        if (na_im > INT32_MAX) na_im = INT32_MAX;
        if (na_im < INT32_MIN) na_im = INT32_MIN;
        if (nb_re > INT32_MAX) nb_re = INT32_MAX;
        if (nb_re < INT32_MIN) nb_re = INT32_MIN;
        if (nb_im > INT32_MAX) nb_im = INT32_MAX;
        if (nb_im < INT32_MIN) nb_im = INT32_MIN;

        pu->amp_grid[rep_A].coeff[b] =
            pro_amp_pack((int32_t)na_re, (int32_t)na_im);
        pu->amp_grid[rep_B].coeff[b] =
            pro_amp_pack((int32_t)nb_re, (int32_t)nb_im);
    }
}

static double tourn_formula_theta_eff(int formel_id,
    double theta_rad, int N_AB, int size_A, int size_B)
{
    double te;
    switch (formel_id) {
    case 1:  te = theta_rad * N_AB / sqrt((double)size_A * size_B); break;
    case 2:  te = theta_rad * N_AB / (double)(size_A * size_B); break;
    case 3:  te = theta_rad * N_AB /
        (double)(size_A > size_B ? size_A : size_B); break;
    case 4:  te = theta_rad * sqrt((double)N_AB) /
        sqrt((double)size_A * size_B); break;
    case 5:  te = theta_rad * N_AB / (double)(size_A + size_B); break;
    case 6:  te = theta_rad * N_AB /
        (double)(size_A + size_B - N_AB); break;
    case 7:  te = theta_rad; break;
    default: te = theta_rad; break;
    }
    if (te > M_PI * 0.5) te = M_PI * 0.5;
    return te;
}

static uint32_t tourn_time_to_50(ProUniverse* pu,
    uint64_t rep_A, uint64_t rep_B,
    double theta_eff, uint32_t max_ticks)
{
    for (uint32_t t = 0; t < max_ticks; t++) {
        tourn_rotate_class_pair(pu, rep_A, rep_B, theta_eff);
        ProPhysics_Shared_Sync(pu);

        const double a_re = (double)pro_amp_real(
            pu->amp_grid[rep_A].coeff[UR_POSITRON_CW]);
        const double a_im = (double)pro_amp_imag(
            pu->amp_grid[rep_A].coeff[UR_POSITRON_CW]);
        const double b_re = (double)pro_amp_real(
            pu->amp_grid[rep_B].coeff[UR_POSITRON_CW]);
        const double b_im = (double)pro_amp_imag(
            pu->amp_grid[rep_B].coeff[UR_POSITRON_CW]);

        const double a_sq = a_re * a_re + a_im * a_im;
        const double b_sq = b_re * b_re + b_im * b_im;
        const double denom = a_sq + b_sq + 1e-30;
        const double fb = b_sq / denom;
        if (fb > 0.5) return t;
    }
    return UINT32_MAX;
}

bool test_shared_formula_tournament(void)
{
    printf("========================================================================\n");
    printf("  Etappe 18e-Probe: Formel-Tournament fuer Klassen-Transport\n");
    printf("  dim=%u/3D, 2 Klassen |A|=|B|=2, N_AB=1\n", TOURN_DIM);
    printf("========================================================================\n\n");

    const uint32_t shift = 5u;

    const uint64_t a0 = tourn_idx3d(10, 16, 16, shift);
    const uint64_t a1 = tourn_idx3d(11, 16, 16, shift);
    const uint64_t b0 = tourn_idx3d(12, 16, 16, shift);
    const uint64_t b1 = tourn_idx3d(13, 16, 16, shift);

    const int N_AB = 1;
    const int size_A = 2;
    const int size_B = 2;

    const double theta_rad =
        ((double)TOURN_THETA_Q15 / 32768.0) * M_PI * 0.5;

    printf("[SR-T8] theta_q15=%u, theta_rad=%.6f\n",
        TOURN_THETA_Q15, theta_rad);

    double ref_T50_u32 = 0;
    uint32_t ref_T50 = UINT32_MAX;
    {
        ProUniverse pu;
        ProPhysics_Initialize(&pu, TOURN_NODES);
        wire_torus_3d(&pu, TOURN_DIM);

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; b++) {
            pu.amp_grid[a0].coeff[b] = 0;
            pu.amp_grid[b0].coeff[b] = 0;
        }
        pu.amp_grid[a0].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(16384), 0);

        for (uint32_t t = 0; t < TOURN_N_TICKS; t++) {
            tourn_rotate_class_pair(&pu, a0, b0, theta_rad);
            const double a_re = (double)pro_amp_real(
                pu.amp_grid[a0].coeff[UR_POSITRON_CW]);
            const double a_im = (double)pro_amp_imag(
                pu.amp_grid[a0].coeff[UR_POSITRON_CW]);
            const double b_re = (double)pro_amp_real(
                pu.amp_grid[b0].coeff[UR_POSITRON_CW]);
            const double b_im = (double)pro_amp_imag(
                pu.amp_grid[b0].coeff[UR_POSITRON_CW]);
            const double a_sq = a_re * a_re + a_im * a_im;
            const double b_sq = b_re * b_re + b_im * b_im;
            const double fb = b_sq / (a_sq + b_sq + 1e-30);
            if (ref_T50 == UINT32_MAX && fb > 0.5) {
                ref_T50 = t;
                ref_T50_u32 = (double)t;
                break;
            }
        }

        printf("[SR-T8] Referenz (unshared): T_50 = %u Ticks\n", ref_T50);
        printf("[SR-T8] Theorie T_50 = pi/4 / theta_rad = %.1f Ticks\n\n",
            (M_PI / 4.0) / theta_rad);

        ProPhysics_Free(&pu);
    }

    const char* formel_desc[] = {
        "(unused)",
        "N_AB / sqrt(|A|*|B|)",
        "N_AB / (|A|*|B|)",
        "N_AB / max(|A|,|B|)",
        "sqrt(N_AB) / sqrt(|A|*|B|)",
        "N_AB / (|A|+|B|)",
        "N_AB / |A union B|",
        "N_AB / 1 (konstant)"
    };

    printf("[SR-T8]   F   Formel                       te/theta   T_50   |dT|    U5-Drift   Konsistenz\n");
    printf("[SR-T8]   ---------------------------------------------------------------------------\n");

    int best_formel = 0;
    double best_dist = 1e300;

    for (int formel_id = 1; formel_id <= 7; formel_id++) {
        ProUniverse pu;
        ProPhysics_Initialize(&pu, TOURN_NODES);
        wire_torus_3d(&pu, TOURN_DIM);

        pu.shared.parent[a0] = a0;
        pu.shared.parent[a1] = a0;
        pu.shared.parent[b0] = b0;
        pu.shared.parent[b1] = b0;
        pu.shared.active = 1u;

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; b++) {
            pu.amp_grid[a0].coeff[b] = 0;
        }
        pu.amp_grid[a0].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(16384), 0);

        ProPhysics_Shared_Sync(&pu);

        const ProU128 u5_before = ProPhysics_Measure_Amp_Invariant(&pu);

        const double theta_eff = tourn_formula_theta_eff(
            formel_id, theta_rad, N_AB, size_A, size_B);

        const uint32_t t50 = tourn_time_to_50(
            &pu, a0, b0, theta_eff, TOURN_N_TICKS);

        const ProU128 u5_after = ProPhysics_Measure_Amp_Invariant(&pu);
        const double u5_b = pro_u128_to_double(u5_before);
        const double u5_a = pro_u128_to_double(u5_after);
        const double u5_drift =
            (u5_b > 0.0) ? fabs(u5_a - u5_b) / u5_b : 0.0;

        const bool konsistenz =
            (memcmp(&pu.amp_grid[a0], &pu.amp_grid[a1],
                sizeof(ProAmpVector)) == 0) &&
            (memcmp(&pu.amp_grid[b0], &pu.amp_grid[b1],
                sizeof(ProAmpVector)) == 0);

        const double dist = (t50 == UINT32_MAX)
            ? 1e300
            : fabs((double)t50 - ref_T50_u32);

        if (dist < best_dist) {
            best_dist = dist;
            best_formel = formel_id;
        }

        printf("[SR-T8]   F%d  %-28s  %.4f     %-6s %.2f    %.2e    %s\n",
            formel_id, formel_desc[formel_id],
            theta_eff / theta_rad,
            (t50 == UINT32_MAX) ? "---" : "      ",
            (t50 == UINT32_MAX) ? -1.0 : (double)t50,
            u5_drift,
            konsistenz ? "OK" : "FAIL");

        ProPhysics_Free(&pu);
    }

    printf("\n[SR-T8] Beste Formel: F%d (%s)\n",
        best_formel, formel_desc[best_formel]);
    printf("[SR-T8] Abweichung von unshared Referenz: dT = %.2f Ticks\n",
        best_dist);

    printf("\n[SR-T8] -> PASSED (Tournament abgeschlossen, siehe Tabelle)\n");
    return true;
}