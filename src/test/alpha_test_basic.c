#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== Born-Regel-Test ===================== */
bool test_born_rule(void) {
    printf("[RUN] Born-Regel-Test: 10000 Messungen gegen |alpha|^2:|beta|^2...\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid) {
        printf("[Born] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    pu.ur_grid[0].type_state = UR_PHOTON;
    pu.ur_grid[1].type_state = UR_PHOTON;
    pu.reg_source[0].channels[PRO_EPR_CHANNEL] = 1;
    pu.reg_source[1].channels[PRO_EPR_CHANNEL] = 0;
    ProPhysics_Set_Edge_Phase(&pu, 0, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_SINGLET);
    ProPhysics_Set_Edge_Phase(&pu, 1, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_SINGLET);

    const int32_t alpha_q31 = Q15TO31(16384);
    const int32_t beta_q31 = Q15TO31(28377);
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
        pu.amp_grid[1].coeff[b] = 0;
    }
    const ProAmpQ31 ca = pro_amp_pack(alpha_q31, 0);
    const ProAmpQ31 cb = pro_amp_pack(beta_q31, 0);
    pu.amp_grid[0].coeff[0] = ca;
    pu.amp_grid[0].coeff[1] = cb;
    pu.amp_grid[1].coeff[0] = ca;
    pu.amp_grid[1].coeff[1] = cb;

    prng_state_t rng;
    prng_seed(&rng, 0xB0B0B0B0ULL);
    uint32_t n_up = 0, n_down = 0;
    const uint32_t trials = 10000;
    const double p0 = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    for (uint32_t t = 0; t < trials; ++t) {
        const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
        if (u < p0) n_up++; else n_down++;
    }
    const double expected_up = 0.25 * (double)trials;
    const double expected_dn = 0.75 * (double)trials;
    const double d_up = (double)n_up - expected_up;
    const double d_dn = (double)n_down - expected_dn;
    const double chi2 = (d_up * d_up) / expected_up + (d_dn * d_dn) / expected_dn;
    const bool pass = (chi2 < 6.635);
    printf("[Born] trials=%u | up=%u (erw %.0f) | down=%u (erw %.0f) | "
        "chi2=%.4f (Schwelle 6.635) -> %s\n",
        trials, n_up, expected_up, n_down, expected_dn, chi2,
        pass ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Unitärer Tick ===================== */
bool test_unitary_tick(void) {
    printf("[RUN] Unitärer-Tick-Test (Etappe 3a, signed permutation)...\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.amp_grid) {
        printf("[Unitary] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    const uint8_t perm[PRO_AMP_BASIS_SIZE] = { 0, 2, 1, 4, 3, 5, 6, 7 };
    const int8_t  sign[PRO_AMP_BASIS_SIZE] = { +1, -1, +1, -1, +1, +1, +1, +1 };
    const bool unit_ok = ProPhysics_Verify_Signed_Perm_Unitarity(perm, sign);
    const bool u5_ok = ProPhysics_Verify_Signed_Perm_U5(perm, sign);
    printf("[Unitary] perm Bijektion + sign ∈ {±1}: %s\n", unit_ok ? "OK" : "FAILED");
    printf("[Unitary] w[perm[i]] = w[i] (U5): %s\n", u5_ok ? "OK" : "FAILED");

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[3] = pro_amp_pack(Q15TO31(23170), 0);
    const ProU128 norm_before = ProPhysics_Weighted_Norm(&pu, 0);
    for (int t = 0; t < 10000; ++t)
        ProPhysics_Apply_Signed_Permutation(&pu, perm, sign);
    const ProU128 norm_after = ProPhysics_Weighted_Norm(&pu, 0);
    const double drift = !pro_u128_is_zero(norm_before)
        ? fabs(pro_u128_to_double(norm_after) - pro_u128_to_double(norm_before))
        / pro_u128_to_double(norm_before)
        : 0.0;
    printf("[Unitary] U5-Norm: before=%.6e after=%.6e drift=%.4e (Schwelle 1e-12)\n",
        pro_u128_to_double(norm_before), pro_u128_to_double(norm_after), drift);
    const bool pass = unit_ok && u5_ok && (drift < 1e-12);
    printf("[Unitary] -> %s\n", pass ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Kontext-Perm ===================== */
bool test_context_perm(void) {
    printf("[RUN] Kontextabhängige signed permutations (Etappe 3b)...\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.amp_grid || !pu.ur_grid || !pu.reg_source) {
        printf("[Context] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    for (uint8_t c = 0; c < (uint8_t)PRO_EPR_CHANNEL; ++c) {
        pu.reg_source[0].channels[c] = (uint64_t)(1 + c);
        pu.reg_source[1].channels[c] = (uint64_t)(30 + c);
    }
#define SET_AMP_UNIT(k, basis_idx) do { \
        for (uint8_t _b = 0; _b < PRO_AMP_BASIS_SIZE; ++_b) \
            pu.amp_grid[(k)].coeff[_b] = 0; \
        pu.amp_grid[(k)].coeff[(basis_idx)] = pro_amp_pack(INT32_MAX, 0); \
    } while (0)
    SET_AMP_UNIT(1, UR_POSITRON_CW);
    SET_AMP_UNIT(2, UR_POSITRON_CCW);
    SET_AMP_UNIT(3, UR_NEGATRON_CCW);

    uint8_t perm0[PRO_AMP_BASIS_SIZE], perm1[PRO_AMP_BASIS_SIZE];
    int8_t  sign0[PRO_AMP_BASIS_SIZE], sign1[PRO_AMP_BASIS_SIZE];
    ProPhysics_Compute_Context_Perm(&pu, 0, perm0, sign0);
    ProPhysics_Compute_Context_Perm(&pu, 1, perm1, sign1);
    bool differ = false;
    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        if (perm0[i] != perm1[i] || sign0[i] != sign1[i]) { differ = true; break; }
    }
    printf("[Context] Knoten 0 vs 1 unterschiedliche perm: %s\n", differ ? "OK" : "FAILED");
    const bool u5_ok =
        ProPhysics_Verify_Signed_Perm_U5(perm0, sign0) &&
        ProPhysics_Verify_Signed_Perm_U5(perm1, sign1);
    printf("[Context] U5-Kompatibilität: %s\n", u5_ok ? "OK" : "FAILED");

    uint8_t perm0_after[PRO_AMP_BASIS_SIZE];
    int8_t  sign0_after[PRO_AMP_BASIS_SIZE];
    SET_AMP_UNIT(1, UR_NEGATRON_CW);
    ProPhysics_Compute_Context_Perm(&pu, 0, perm0_after, sign0_after);
    bool changed = false;
    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        if (perm0[i] != perm0_after[i] || sign0[i] != sign0_after[i]) { changed = true; break; }
    }
    printf("[Context] Nachbar-amp_grid-Änderung ändert perm: %s\n", changed ? "OK" : "FAILED");
    SET_AMP_UNIT(1, UR_POSITRON_CW);

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[3] = pro_amp_pack(Q15TO31(23170), 0);
    const ProU128 norm_before = ProPhysics_Weighted_Norm(&pu, 0);
    for (int t = 0; t < 10000; ++t) ProPhysics_Apply_Context_Tick(&pu);
    const ProU128 norm_after = ProPhysics_Weighted_Norm(&pu, 0);
    const double drift = !pro_u128_is_zero(norm_before)
        ? fabs(pro_u128_to_double(norm_after) - pro_u128_to_double(norm_before))
        / pro_u128_to_double(norm_before)
        : 0.0;
    printf("[Unitary] U5-Norm: before=%.6e after=%.6e drift=%.4e (Schwelle 1e-12)\n",
        pro_u128_to_double(norm_before), pro_u128_to_double(norm_after), drift);
    const bool pass = differ && u5_ok && changed && (drift < 1e-12);
    printf("[Context] -> %s\n", pass ? "PASSED" : "FAILED");
#undef SET_AMP_UNIT
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Wilson-Loop ===================== */
bool test_wilson_loop(void) {
    printf("[RUN] U(1)-Wilson-Loop-Test (Etappe 4a)...\n");
    const uint32_t DIM = 4u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.edge_phases || !pu.reg_source || !pu.amp_grid) {
        printf("[Wilson] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    ProPhysics_Set_Edge_Phase(&pu, 0, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 1, 0, -0.25 * M_PI, PRO_EDGE_NONE);
    const uint64_t path2[2] = { 0u, 1u };
    const uint8_t  chans2[2] = { 0u, 0u };
    uint16_t W2 = 0u;
    ProPhysics_Wilson_Loop(&pu, path2, chans2, 2u, &W2);
    printf("[Wilson] 2-Kanten-Loop W_fx = %u (erwartet 0)\n", (unsigned)W2);

    const uint64_t path4[4] = { 0u, 1u, 5u, 4u };
    const uint8_t  chans4[4] = { 0u, 0u, 0u, 0u };
    ProPhysics_Set_Edge_Phase(&pu, 0, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 1, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 5, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 4, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    uint16_t W4 = 0u;
    ProPhysics_Wilson_Loop(&pu, path4, chans4, 4u, &W4);
    printf("[Wilson] 4-Kanten-Plaquette W_fx = %u (erwartet 32768)\n", (unsigned)W4);

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[0] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(16384), 0);
    const double p0_before = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_before = ProPhysics_Get_Born_Probability(&pu, 0, 1);
    ProPhysics_Global_Phase(&pu, 21845u);
    const double p0_after = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_after = ProPhysics_Get_Born_Probability(&pu, 0, 1);
    const double dp0 = fabs(p0_after - p0_before);
    const double dp1 = fabs(p1_after - p1_before);
    printf("[Wilson] Eichinvarianz: P(0) %.8f → %.8f (|Δ|=%.2e)\n", p0_before, p0_after, dp0);
    printf("[Wilson] Eichinvarianz: P(1) %.8f → %.8f (|Δ|=%.2e)\n", p1_before, p1_after, dp1);
    const bool pass = (W2 == 0u) && (W4 == 32768u) && (dp0 < 1e-3) && (dp1 < 1e-3);
    printf("[Wilson] -> %s\n", pass ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Lokale Eichinvarianz ===================== */
bool test_local_gauge(void) {
    printf("[RUN] Lokale Eichinvarianz (Etappe 4b)...\n");
    const uint32_t DIM = 4u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.amp_grid || !pu.edge_phases || !pu.reg_source) {
        printf("[Gauge] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
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
        for (uint8_t ch = 0; ch < 4; ++ch) {
            const uint16_t ph = (uint16_t)(((k * 4099u) + ch * 12345u) & 0xFFFFu);
            pu.edge_phases[k * (uint64_t)CHANNELS_MAX + ch].phase = ph;
            pu.edge_phases[k * (uint64_t)CHANNELS_MAX + ch].type = PRO_EDGE_NONE;
        }
    }
    const uint64_t plq_nodes[4] = { 0u, 1u, 5u, 4u };
    const uint8_t  plq_ch[4] = { 2u, 1u, 3u, 0u };
    uint16_t W_before = 0u;
    ProPhysics_Wilson_Loop(&pu, plq_nodes, plq_ch, 4u, &W_before);
    uint16_t phi_before[4];
    for (int i = 0; i < 4; ++i)
        phi_before[i] = pu.edge_phases[plq_nodes[i] * (uint64_t)CHANNELS_MAX + plq_ch[i]].phase;

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[0] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(16384), 0);
    const double p0_before = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_before = ProPhysics_Get_Born_Probability(&pu, 0, 1);

    uint16_t* lambda = (uint16_t*)malloc((size_t)NODES * sizeof(uint16_t));
    if (!lambda) { ProPhysics_Free(&pu); return false; }
    ProPhysics_Make_Lambda_Field(0xDEADBEEFu, lambda, NODES);
    ProPhysics_Apply_Local_Gauge(&pu, lambda);

    bool edges_ok = true;
    for (int i = 0; i < 4 && edges_ok; ++i) {
        const uint64_t x = plq_nodes[i];
        const uint8_t  ch = plq_ch[i];
        const uint64_t y = pu.reg_source[x].channels[ch];
        const uint16_t phi_after = pu.edge_phases[x * (uint64_t)CHANNELS_MAX + ch].phase;
        const int32_t d = (int32_t)lambda[y] - (int32_t)lambda[x];
        const uint16_t expected = (uint16_t)(((uint32_t)phi_before[i] + (uint32_t)d) & 0xFFFFu);
        if (phi_after != expected) {
            printf("[Gauge] Kante %d: phi_after=%u expected=%u (MISMATCH)\n",
                i, (unsigned)phi_after, (unsigned)expected);
            edges_ok = false;
        }
    }
    printf("[Gauge] Kantenphasen exakt transformiert: %s\n", edges_ok ? "OK" : "FAILED");
    uint16_t W_after = 0u;
    ProPhysics_Wilson_Loop(&pu, plq_nodes, plq_ch, 4u, &W_after);
    const bool wilson_ok = (W_before == W_after);
    printf("[Gauge] Wilson-Loop: before=%u after=%u -> %s\n",
        (unsigned)W_before, (unsigned)W_after, wilson_ok ? "INVARIANT" : "VIOLATED");
    const double p0_after = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_after = ProPhysics_Get_Born_Probability(&pu, 0, 1);
    const double dp0 = fabs(p0_after - p0_before);
    const double dp1 = fabs(p1_after - p1_before);
    const bool born_ok = (dp0 < 1e-3) && (dp1 < 1e-3);
    printf("[Gauge] Born P(0): %.8f -> %.8f (|Δ|=%.2e)\n", p0_before, p0_after, dp0);
    printf("[Gauge] Born P(1): %.8f -> %.8f (|Δ|=%.2e)\n", p1_before, p1_after, dp1);
    const bool pass = edges_ok && wilson_ok && born_ok;
    printf("[Gauge] -> %s\n", pass ? "PASSED" : "FAILED");
    free(lambda);
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Dreieckskorrelation ===================== */
bool test_triangle_correlation(void) {
    printf("[RUN] Dreieckskorrelation (Etappe 4c)...\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source) {
        printf("[Triangle] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    init_torus(&pu);
    prng_state_t rng;
    prng_seed(&rng, 0xFACEFEEDULL);
    for (uint32_t i = 0; i < 2000; ++i)
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)PRO_EDGE_SINGLET);
    const uint32_t N = 9;
    double max_dev = 0.0;
    printf("[Triangle]   Δ/π      E_measured   E_triangle   |dev|\n");
    for (uint32_t k = 0; k < N; ++k) {
        const double delta = (double)k * (M_PI / (double)(N - 1));
        const double E = evaluate_correlation(&pu, 0.0, delta, rng.s);
        const double E_tri = -1.0 + (2.0 / M_PI) * delta;
        const double dev = fabs(E - E_tri);
        if (dev > max_dev) max_dev = dev;
        printf("[Triangle]   %.4f   %+.6f    %+.6f    %.4f\n",
            delta / M_PI, E, E_tri, dev);
    }
    const double tol = 0.10;
    const bool pass = (max_dev < tol);
    printf("[Triangle] max |dev| = %.4f (tol %.2f) -> %s\n",
        max_dev, tol, pass ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Amp-Smoke ===================== */
void amplitude_layer_smoke_test(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.amp_grid) {
        printf("[Amp-Test] FAILED: amp_grid ist NULL — Etappe 1 nicht eingespielt.\n");
        ProPhysics_Free(&pu);
        return;
    }
    int32_t re = 0, im = 0;
    if (!ProPhysics_Get_Node_Amplitude(&pu, 0, 0, &re, &im)) {
        printf("[Amp-Test] FAILED: Get_Node_Amplitude fehlgeschlagen.\n");
        ProPhysics_Free(&pu);
        return;
    }
    printf("[Amp-Test] OK: amp_grid allokiert, coeff[0] = (%d, %d) "
        "(erwartet ~2147483647, 0)\n", (int)re, (int)im);
    const int32_t test_re = Q15TO31(16384);
    const int32_t test_im = -Q15TO31(8192);
    ProPhysics_Set_Node_Amplitude(&pu, 1, 5, test_re, test_im);
    int32_t re2 = 0, im2 = 0;
    ProPhysics_Get_Node_Amplitude(&pu, 1, 5, &re2, &im2);
    printf("[Amp-Test] Set/Get roundtrip: (%d, %d) (erwartet %d, %d)\n",
        (int)re2, (int)im2, (int)test_re, (int)test_im);
    ProPhysics_Free(&pu);
}