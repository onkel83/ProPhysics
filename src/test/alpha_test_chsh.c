#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== CHSH-Metriken ===================== */
double evaluate_correlation(const ProUniverse* pu,
    double theta_a, double theta_b,
    uint64_t rng[4])
{
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->edge_phases) return 0.0;
    double sum = 0.0;
    uint64_t count = 0;
    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        if (pu->ur_grid[i].type_state == UR_NEUTRAL) continue;
        const uint64_t partner = pu->reg_source[i].channels[PRO_EPR_CHANNEL];
        if (partner >= pu->total_nodes || partner == i) continue;
        if (partner < i) continue;
        if (pu->ur_grid[partner].type_state == UR_NEUTRAL) continue;
        const uint8_t edge_type =
            pu->edge_phases[i * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL].type;
        if (edge_type != PRO_EDGE_SINGLET && edge_type != PRO_EDGE_TRIPLET) continue;
        int a = 0, b = 0;
        if (ProPhysics_Measure_EPR_Pair((ProUniverse*)pu, i,
            theta_a, theta_b, rng, &a, &b)) {
            sum += (double)(a * b);
            count++;
        }
    }
    return count ? (sum / (double)count) : 0.0;
}

CHSH_Result compute_chsh_metrics(ProUniverse* pu, uint64_t rng[4]) {
    CHSH_Result res = { 0 };
    const double thA = 0.0;
    const double thAp = M_PI / 2.0;
    const double thB = M_PI / 4.0;
    const double thBp = 3.0 * M_PI / 4.0;
    res.E_ab = evaluate_correlation(pu, thA, thB, rng);
    res.E_ab_prime = evaluate_correlation(pu, thA, thBp, rng);
    res.E_a_prime_b = evaluate_correlation(pu, thAp, thB, rng);
    res.E_a_prime_b_prime = evaluate_correlation(pu, thAp, thBp, rng);
    res.S_CHSH = fabs(res.E_ab - res.E_ab_prime)
        + fabs(res.E_a_prime_b + res.E_a_prime_b_prime);
    return res;
}

/* ===================== CHSH Edge-Type ===================== */
bool test_chsh_edge_type(ProEdgeType edge_type, const char* label,
    double expected_S, double tolerance)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source) {
        printf("   [CHSH %s] Init failed\n", label);
        return false;
    }
    init_torus(&pu);
    prng_state_t rng;
    prng_seed(&rng, 0xC0FFEE1234567ULL);
    const uint32_t pair_attempts = 1500;
    for (uint32_t i = 0; i < pair_attempts; ++i)
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)edge_type);

    uint32_t pair_count = 0;
    for (uint64_t i = 0; i < pu.total_nodes; ++i) {
        const uint64_t p = pu.reg_source[i].channels[PRO_EPR_CHANNEL];
        if (p < pu.total_nodes && p != i && p > i) pair_count++;
    }
    const CHSH_Result res = compute_chsh_metrics(&pu, rng.s);
    const double dev = fabs(res.S_CHSH - expected_S);
    printf("   [CHSH %-7s] pairs=%u | E=(%+.4f, %+.4f, %+.4f, %+.4f)\n",
        label, pair_count,
        res.E_ab, res.E_ab_prime, res.E_a_prime_b, res.E_a_prime_b_prime);
    printf("                  S = %.6f | expected = %.6f | |dev| = %.2e\n",
        res.S_CHSH, expected_S, dev);

    const double d = M_PI / 4.0;
    const double sign_s = (edge_type == PRO_EDGE_SINGLET) ? -1.0 : +1.0;
    const double exp_E_ab = sign_s * (1.0 - (2.0 / M_PI) * d);
    const double exp_E_abp = sign_s * (1.0 - (2.0 / M_PI) * (M_PI - d));
    const double exp_E_apb = sign_s * (1.0 - (2.0 / M_PI) * d);
    const double exp_E_apbp = sign_s * (1.0 - (2.0 / M_PI) * d);
    const double tol_tri = 0.10;
    const bool triangle_ok =
        fabs(res.E_ab - exp_E_ab) < tol_tri &&
        fabs(res.E_ab_prime - exp_E_abp) < tol_tri &&
        fabs(res.E_a_prime_b - exp_E_apb) < tol_tri &&
        fabs(res.E_a_prime_b_prime - exp_E_apbp) < tol_tri;
    printf("                  Dreiecksform: %s (tol %.2f)\n",
        triangle_ok ? "OK" : "ABWEICHUNG", tol_tri);
    ProPhysics_Free(&pu);
    return (dev < tolerance) && triangle_ok;
}

/* ===================== Superdeterminismus ===================== */
static void set_apparatus_phase_amp(ProUniverse* pu, uint64_t node, double theta) {
    if (!pu || !pu->amp_grid || node >= pu->total_nodes) return;
    ProAmpVector* v = &pu->amp_grid[node];
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) v->coeff[b] = 0;
    const int32_t re = (int32_t)lround(cos(theta) * Q31_MAXV);
    const int32_t im = (int32_t)lround(sin(theta) * Q31_MAXV);
    v->coeff[UR_POSITRON_CW] = pro_amp_pack(re, im);
}
static double read_apparatus_phase_amp(const ProUniverse* pu, uint64_t node) {
    if (!pu || !pu->amp_grid || node >= pu->total_nodes) return 0.0;
    const int32_t re = pro_amp_real(pu->amp_grid[node].coeff[UR_POSITRON_CW]);
    const int32_t im = pro_amp_imag(pu->amp_grid[node].coeff[UR_POSITRON_CW]);
    if (re == 0 && im == 0) return 0.0;
    double phi = atan2((double)im, (double)re);
    if (phi < 0.0) phi += 2.0 * M_PI;
    return phi;
}
static void set_pair_lambda(ProUniverse* pu, uint64_t idx_a, uint64_t idx_b,
    double lambda, uint8_t edge_type)
{
    if (!pu || !pu->amp_grid) return;
    if (idx_a >= pu->total_nodes || idx_b >= pu->total_nodes) return;
    if (edge_type != PRO_EDGE_SINGLET && edge_type != PRO_EDGE_TRIPLET) return;
    ProAmpVector* v1 = &pu->amp_grid[idx_a];
    ProAmpVector* v2 = &pu->amp_grid[idx_b];
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) { v1->coeff[b] = 0; v2->coeff[b] = 0; }
    const double half = lambda * 0.5;
    const double c = cos(half), s = sin(half);
    const int32_t a_up = (int32_t)lround(c * Q31_MAXV);
    const int32_t a_dn = (int32_t)lround(s * Q31_MAXV);
    v1->coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
    v1->coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
    if (edge_type == PRO_EDGE_SINGLET) {
        const int32_t b_up = (int32_t)lround(-s * Q31_MAXV);
        const int32_t b_dn = (int32_t)lround(c * Q31_MAXV);
        v2->coeff[UR_POSITRON_CW] = pro_amp_pack(b_up, 0);
        v2->coeff[UR_NEGATRON_CCW] = pro_amp_pack(b_dn, 0);
    }
    else {
        v2->coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
        v2->coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
    }
}

bool test_chsh_superdet(bool shared_source, double* out_S) {
    printf("[RUN] CHSH-Superdeterminismus (Quelle=%s, amp_grid-Apparat)...\n",
        shared_source ? "SHARED" : "SEPARATE");
    const uint64_t NODES = 2048;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.edge_phases) {
        printf("[Superdet] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    const uint64_t app_a = 0, app_b = 1, part_a = 1024, part_b = 1025;
    const uint8_t edge_type = PRO_EDGE_SINGLET;
    pu.reg_source[part_a].channels[PRO_EPR_CHANNEL] = part_b;
    pu.reg_source[part_b].channels[PRO_EPR_CHANNEL] = part_a;
    ProPhysics_Set_Edge_Phase(&pu, part_a, PRO_EPR_CHANNEL, 0.0, edge_type);
    ProPhysics_Set_Edge_Phase(&pu, part_b, PRO_EPR_CHANNEL, 0.0, edge_type);
    prng_state_t rng;
    prng_seed(&rng, shared_source ? 0x5D5D5D5D01ULL : 0x5D5D5D5D02ULL);
    const uint32_t trials = 20000;
    double E_sum[2][2] = { {0,0}, {0,0} };
    uint32_t E_cnt[2][2] = { {0,0}, {0,0} };
    for (uint32_t t = 0; t < trials; ++t) {
        const double S1 = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
        int A, B; double lambda;
        if (shared_source) {
            if (S1 < 0.25) { A = 0; B = 0; lambda = 0.0; }
            else if (S1 < 0.50) { A = 0; B = 1; lambda = M_PI; }
            else if (S1 < 0.75) { A = 1; B = 0; lambda = M_PI / 2.0; }
            else { A = 1; B = 1; lambda = 3.0 * M_PI / 2.0; }
        }
        else {
            if (S1 < 0.25) { A = 0; B = 0; }
            else if (S1 < 0.50) { A = 0; B = 1; }
            else if (S1 < 0.75) { A = 1; B = 0; }
            else { A = 1; B = 1; }
            const double S2 = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
            lambda = S2 * 2.0 * M_PI;
        }
        const double theta_a = (A == 0) ? 0.0 : (M_PI / 2.0);
        const double theta_b = (B == 0) ? (M_PI / 4.0) : (3.0 * M_PI / 4.0);
        set_apparatus_phase_amp(&pu, app_a, theta_a);
        set_apparatus_phase_amp(&pu, app_b, theta_b);
        set_pair_lambda(&pu, part_a, part_b, lambda, edge_type);
        const double th_a_read = read_apparatus_phase_amp(&pu, app_a);
        const double th_b_read = read_apparatus_phase_amp(&pu, app_b);
        int out_a = 0, out_b = 0;
        if (!ProPhysics_Measure_EPR_Pair_Amp(&pu, part_a, th_a_read, th_b_read,
            &out_a, &out_b)) continue;
        E_sum[A][B] += (double)(out_a * out_b);
        E_cnt[A][B]++;
    }
    double E[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            E[i][j] = E_cnt[i][j] ? (E_sum[i][j] / (double)E_cnt[i][j]) : 0.0;
    const double S_val = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    printf("[Superdet] trials=%u | counts: (00=%u, 01=%u, 10=%u, 11=%u)\n",
        trials, E_cnt[0][0], E_cnt[0][1], E_cnt[1][0], E_cnt[1][1]);
    printf("[Superdet] E(0,0)=%+.4f  E(0,1)=%+.4f  E(1,0)=%+.4f  E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Superdet] S = %.4f\n", S_val);
    if (out_S) *out_S = S_val;
    ProPhysics_Free(&pu);
    return true;
}

/* ===================== Observer-CHSH ===================== */
bool test_observer_chsh(void) {
    printf("[RUN] Observer-CHSH (Etappe 6a, lokale Amplitudendiffusion, Q31)...\n");
    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Observer-CHSH] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
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
    ProObserver obs_A, obs_B;
    const uint64_t source_node = 2048u;
    ProPhysics_Init_Observer(&pu, &obs_A, 2028u, 64u);
    ProPhysics_Init_Observer(&pu, &obs_B, 2068u, 64u);
    const uint32_t DIFF_TICKS = 30u;
    const uint32_t DIFF_RATE = 100u;
    const uint32_t TRIALS = 200u;
    const double thA0 = 0.0, thA1 = M_PI / 2.0, thB0 = M_PI / 4.0, thB1 = 3.0 * M_PI / 4.0;
    prng_state_t rng;
    prng_seed(&rng, 0x0606A06AULL);
    double sum_E[2][2] = { {0,0}, {0,0} };
    uint32_t cnt_E[2][2] = { {0,0}, {0,0} };
    for (uint32_t t = 0; t < TRIALS; ++t) {
        for (uint64_t k = 0; k < NODES; ++k) {
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu.amp_grid[k].coeff[b] = 0;
            pu.amp_grid[k].coeff[UR_NEUTRAL] = pro_amp_pack(INT32_MAX, 0);
        }
        const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
        const double lambda = u * 2.0 * M_PI;
        const double c = cos(lambda * 0.5), s = sin(lambda * 0.5);
        const int32_t a_up = (int32_t)lround(c * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(s * Q31_MAXV);
        pu.amp_grid[source_node].coeff[UR_NEUTRAL] = 0;
        pu.amp_grid[source_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
        pu.amp_grid[source_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
        for (uint32_t k = 0; k < DIFF_TICKS; ++k)
            ProPhysics_Apply_Local_Amplitude_Diffusion(&pu, DIFF_RATE);
        const double thA[2] = { thA0, thA1 };
        const double thB[2] = { thB0, thB1 };
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 2; ++j) {
                int oa = 0, ob = 0;
                if (!ProPhysics_Observer_Measure_CHSH(&pu, &obs_A, &obs_B,
                    thA[i], thB[j], &oa, &ob)) continue;
                sum_E[i][j] += (double)(oa * ob);
                cnt_E[i][j] += 1u;
            }
    }
    double E[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            E[i][j] = cnt_E[i][j] ? sum_E[i][j] / (double)cnt_E[i][j] : 0.0;
    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double se_S = 2.0 / sqrt((double)TRIALS);
    const double thr = 2.0 + 3.0 * se_S;
    printf("[Observer-CHSH] trials=%u | ticks=%u | rate=%u%% | source=%llu\n",
        TRIALS, DIFF_TICKS, DIFF_RATE, (unsigned long long)source_node);
    printf("[Observer-CHSH] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Observer-CHSH] S = %.4f ± %.4f (1σ) | 3σ-Schwelle = %.4f\n", S, se_S, thr);
    const bool violate = (S > thr);
    printf("[Observer-CHSH] -> %s\n",
        violate ? "FAILED (S > 2 + 3σ)" : "PASSED (S <= 2 + 3σ)");
    ProPhysics_Free(&pu);
    return !violate;
}

/* ===================== Chaotische Quelle ===================== */
static bool test_chsh_chaotic_impl(bool use_wave) {
    printf("[RUN] CHSH chaotische Quelle (Modus: %s, Q31)...\n",
        use_wave ? "WAVE (6d, unitär)" : "DIFFUSION (6a-6c, dissipativ)");
    const uint32_t DIM = 96u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Chaotic] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
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
    const uint64_t src_base = (uint64_t)40 * DIM + 40;
    const uint32_t src_dim = 8u;
    const uint64_t obsA_base = (uint64_t)35 * DIM + 35;
    const uint64_t obsB_base = (uint64_t)50 * DIM + 50;
    const uint32_t obs_dim = 4u;
    ProObserver obs_A, obs_B;
    ProPhysics_Init_Observer(&pu, &obs_A, obsA_base, obs_dim * obs_dim);
    ProPhysics_Init_Observer(&pu, &obs_B, obsB_base, obs_dim * obs_dim);
    const uint32_t TRIALS = 100u;
    const uint32_t TICKS = use_wave ? 200u : 60u;
    const uint32_t RATE = 100u;
    const uint64_t THRESH = UINT64_MAX;
    const uint32_t DEPHASE_STRENGTH = 50u;
    const uint32_t WAVE_STEP_Q15 = 1000u;
    prng_state_t rng;
    prng_seed(&rng, 0x06B06B06BULL);
    const double thA[2] = { 0.0, M_PI / 2.0 };
    const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };
    double sum_E[2][2] = { {0,0}, {0,0} };
    uint32_t cnt_E[2][2] = { {0,0}, {0,0} };
    uint32_t n_up = 0, n_dn = 0;
    for (uint32_t t = 0; t < TRIALS; ++t) {
        ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim,
            (uint64_t)0xC0FFEE6BULL + (uint64_t)t * 0x9e3779b97f4a7c15ULL);
        if (use_wave) {
            for (uint32_t k = 0; k < TICKS; ++k)
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
        }
        else {
            for (uint32_t k = 0; k < TICKS; ++k) {
                ProPhysics_Apply_Nonlinear_Diffusion_Tick(&pu, RATE, THRESH);
                ProPhysics_Apply_Local_Dephasing_Tick(&pu, DEPHASE_STRENGTH);
            }
        }
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 2; ++j) {
                int oa = 0, ob = 0;
                if (!ProPhysics_Observer_Measure_CHSH_Projected(
                    &pu, &obs_A, &obs_B, thA[i], thB[j], rng.s,
                    &oa, &ob, NULL, NULL)) continue;
                sum_E[i][j] += (double)(oa * ob);
                cnt_E[i][j] += 1u;
                if (i == 0) { if (oa == +1) n_up++; else n_dn++; }
            }
    }
    double E[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            E[i][j] = cnt_E[i][j] ? sum_E[i][j] / (double)cnt_E[i][j] : 0.0;
    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double se_S = 2.0 / sqrt((double)(TRIALS > 0 ? TRIALS : 1));
    const double thr_hi = 2.0 + 3.0 * se_S;
    const double thr_lo = 2.0 - 3.0 * se_S;
    const uint32_t total_born = n_up + n_dn;
    const double p_up = total_born ? (double)n_up / (double)total_born : 0.0;
    printf("[Chaotic] modus=%s | trials=%u | ticks=%u | src_dim=%u\n",
        use_wave ? "wave" : "diffusion", TRIALS, TICKS, src_dim);
    printf("[Chaotic] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Chaotic] S = %.4f | 3σ-Band um 2.0: [%.4f, %.4f]\n", S, thr_lo, thr_hi);
    printf("[Chaotic] P(out_A=+1 | θ_a=0) = %.4f (n=%u)\n", p_up, total_born);
    if (S > thr_hi)
        printf("[Chaotic] -> S > 2 + 3σ. KEIN Bell-Bruch.\n");
    else if (S < thr_lo)
        printf("[Chaotic] -> S < 2 - 3σ. Signalverlust.\n");
    else
        printf("[Chaotic] -> S ≈ 2.0 im 3σ-Band. Klassische Schranke respektiert.\n");
    ProPhysics_Free(&pu);
    return true;
}

bool test_chsh_diffusion(void) { return test_chsh_chaotic_impl(false); }
bool test_chsh_wave(void) { return test_chsh_chaotic_impl(true); }

/* ===================== Collapse ===================== */
bool test_chsh_collapse(void) {
    printf("[RUN] CHSH mit Kollaps-Messung (Etappe 6e, U4)...\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.edge_phases) {
        printf("[Collapse] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    init_torus(&pu);
    prng_state_t rng;
    prng_seed(&rng, 0xC011A95EULL);
    for (uint32_t i = 0; i < 1500; ++i)
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)PRO_EDGE_SINGLET);
    const double thA[2] = { 0.0, M_PI / 2.0 };
    const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };
    double E[2][2] = { {0,0}, {0,0} };
    uint32_t cnt[2][2] = { {0,0}, {0,0} };
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;
            for (uint64_t k = 0; k < pu.total_nodes; ++k) {
                const uint64_t partner = pu.reg_source[k].channels[PRO_EPR_CHANNEL];
                if (partner >= pu.total_nodes || partner == k) continue;
                if (partner < k) continue;
                const uint8_t et = pu.edge_phases[k * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL].type;
                if (et != PRO_EDGE_SINGLET && et != PRO_EDGE_TRIPLET) continue;
                int a = 0, b = 0;
                if (!ProPhysics_Measure_EPR_Pair_Collapse(&pu, k, thA[i], thB[j],
                    rng.s, &a, &b)) continue;
                sum += (double)(a * b);
                count++;
            }
            E[i][j] = count ? (sum / (double)count) : 0.0;
            cnt[i][j] = count;
        }
    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double S_tsirelson = 2.0 * sqrt(2.0);
    const uint32_t total = cnt[0][0] + cnt[0][1] + cnt[1][0] + cnt[1][1];
    const double se_S = (total > 0) ? (2.0 / sqrt((double)total)) : 0.0;
    const double band = 5.0 * se_S + 0.05;
    printf("[Collapse] pairs: (00=%u, 01=%u, 10=%u, 11=%u)\n",
        cnt[0][0], cnt[0][1], cnt[1][0], cnt[1][1]);
    printf("[Collapse] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Collapse] S = %.4f | erwartet %.4f | 5σ-Band = ±%.4f\n",
        S, S_tsirelson, band);
    const bool tsirelson_ok = fabs(S - S_tsirelson) < band;
    printf("[Collapse] -> %s\n", tsirelson_ok ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return tsirelson_ok;
}

/* ===================== Graph ===================== */
bool test_chsh_graph(void) {
    printf("[RUN] CHSH Graph-Messung via Kantenphase (Etappe 6e')...\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.edge_phases) {
        printf("[Graph] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    init_torus(&pu);
    prng_state_t rng;
    prng_seed(&rng, 0x6A4F6A4FULL);
    for (uint32_t i = 0; i < 1500; ++i)
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)PRO_EDGE_SINGLET);
    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        ProEdge* e = &pu.edge_phases[k * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL];
        if (e->type == PRO_EDGE_SINGLET || e->type == PRO_EDGE_TRIPLET)
            e->phase = 32768u;
    }
    const double thA[2] = { 0.0, M_PI / 2.0 };
    const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };
    double E[2][2] = { {0,0}, {0,0} };
    uint32_t cnt[2][2] = { {0,0}, {0,0} };
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;
            for (uint64_t k = 0; k < pu.total_nodes; ++k) {
                const uint64_t partner = pu.reg_source[k].channels[PRO_EPR_CHANNEL];
                if (partner >= pu.total_nodes || partner == k) continue;
                if (partner < k) continue;
                int a = 0, b = 0;
                if (!ProPhysics_Measure_EPR_Pair_Graph(&pu, k, thA[i], thB[j],
                    rng.s, &a, &b)) continue;
                sum += (double)(a * b);
                count++;
            }
            E[i][j] = count ? (sum / (double)count) : 0.0;
            cnt[i][j] = count;
        }
    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double S_tsirelson = 2.0 * sqrt(2.0);
    const uint32_t total = cnt[0][0] + cnt[0][1] + cnt[1][0] + cnt[1][1];
    const double se_S = (total > 0) ? (2.0 / sqrt((double)total)) : 0.0;
    const double band = 5.0 * se_S + 0.05;
    printf("[Graph] pairs: (00=%u, 01=%u, 10=%u, 11=%u)\n",
        cnt[0][0], cnt[0][1], cnt[1][0], cnt[1][1]);
    printf("[Graph] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Graph] S = %.4f | erwartet %.4f | 5sigma-Band = +-%.4f\n",
        S, S_tsirelson, band);
    const bool tsirelson_ok = fabs(S - S_tsirelson) < band;
    printf("[Graph] -> %s\n", tsirelson_ok ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return tsirelson_ok;
}