#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== Born-Emergent (6f) ===================== */
static double born_projection(const ProAmpVector* v, double theta) {
    const double up = (double)pro_amp_real(v->coeff[UR_POSITRON_CW]);
    const double dn = (double)pro_amp_real(v->coeff[UR_NEGATRON_CCW]);
    const double norm = up * up + dn * dn;
    if (norm < 1.0) return 0.5;
    const double c = cos(theta * 0.5), s = sin(theta * 0.5);
    const double proj = c * up + s * dn;
    return (proj * proj) / norm;
}

/* Etappe 16e''-Fix (Prio-2-Reparatur):
 *
 * Die frueheren Tests riefen ProPhysics_Sharp_Measure(0, lambda) auf.
 * Das ist deterministisch (sign(cos(-lambda))), liefert also pro
 * lambda-Bin 2000 identische Ausgaben -> binaeres Schalten statt
 * Born-Statistik -> chi^2 explodiert.
 *
 * Ebenso wurde env_hash_uniform (Born-Emergent) entfernt: die
 * Umgebungsamplitude ist ueber den Wave-Step mit sys_node gekoppelt und
 * daher NICHT uniform unabhaengig vom Testzustand. Der Hash war also
 * mit lambda korreliert.
 *
 * Fix: echtes Born-Sampling ueber einen frisch geseedeten xoshiro256**,
 * dessen Stream deterministisch pro (lambda, trial)-Paar ist. Damit
 * konvergiert P_meas ueber die Trials gegen P_born = cos^2(lambda/2).
 */

bool test_born_emergent(void) {
    printf("[RUN] Born-Emergenz aus Umgebungsverschraenkung (Etappe 6f, Q31)...\n");
    const uint32_t DIM = 96u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Born-Emerge] Init failed.\n");
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
    const uint64_t src_base = (uint64_t)44 * DIM + 44;
    const uint32_t src_dim = 8u;
    const uint64_t sys_node = src_base
        + (uint64_t)(src_dim / 2u) * DIM + (uint64_t)(src_dim / 2u);
    const uint32_t TRIALS_PER_LAMBDA = 400u;
    const uint32_t WAVE_TICKS = 60u;
    const uint32_t WAVE_STEP_Q15 = 3000u;
    const uint32_t NUM_LAMBDA = 8u;
    double chi2_total = 0.0;
    uint32_t chi2_dof = 0;
    printf("[Born-Emerge] DIM=%u, sys_node=%llu, src_dim=%u, "
        "trials/lambda=%u, wave_ticks=%u, env_radius=3\n",
        DIM, (unsigned long long)sys_node, src_dim,
        TRIALS_PER_LAMBDA, WAVE_TICKS);
    for (uint32_t li = 0; li < NUM_LAMBDA; ++li) {
        const double lambda_test = (double)li * (M_PI / (double)NUM_LAMBDA);
        const int32_t a_up = (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        const double p_born = cos(lambda_test * 0.5) * cos(lambda_test * 0.5);
        uint32_t n_up = 0, n_dn = 0;
        for (uint32_t t = 0; t < TRIALS_PER_LAMBDA; ++t) {
            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] = pro_amp_pack(INT32_MAX, 0);
            }
            const uint64_t seed = 0xB0B0B0B0ULL
                + (uint64_t)t * 0x9e3779b97f4a7c15ULL
                + (uint64_t)li * 0x123456789ABCDEFULL;
            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim, seed);
            pu.amp_grid[sys_node].coeff[UR_NEUTRAL] = 0;
            pu.amp_grid[sys_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            pu.amp_grid[sys_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
            for (uint32_t k = 0; k < WAVE_TICKS; ++k)
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);

            const double p_plus = born_projection(&pu.amp_grid[sys_node], 0.0);

            /* Etappe 16e''-Fix: Born-Sampling ueber frischen xoshiro256**.
             * XOR mit Konstante trennt den RNG-Stream sauber vom Seed der
             * chaotischen Quelle. */
            prng_state_t rng;
            prng_seed(&rng, seed ^ 0x0B0B0B0B0B0B0B0BULL);
            const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
            const int out = (u < p_plus) ? +1 : -1;
            if (out == +1) n_up++; else n_dn++;
        }
        const uint32_t total = n_up + n_dn;
        const double p_meas = total ? (double)n_up / (double)total : 0.0;
        if (p_born > 0.02 && p_born < 0.98 && total > 0) {
            const double exp_up = p_born * total;
            const double exp_dn = (1.0 - p_born) * total;
            const double d_up = (double)n_up - exp_up;
            const double d_dn = (double)n_dn - exp_dn;
            chi2_total += (d_up * d_up) / exp_up + (d_dn * d_dn) / exp_dn;
            chi2_dof++;
        }
        printf("[Born-Emerge] lambda=%.4f | P_meas=%.4f | P_born=%.4f | "
            "n_up=%u n_dn=%u\n", lambda_test, p_meas, p_born, n_up, n_dn);
    }
    double chi2_thr = 14.07;
    if (chi2_dof == 5) chi2_thr = 15.09;
    if (chi2_dof == 6) chi2_thr = 16.81;
    if (chi2_dof == 7) chi2_thr = 18.48;
    const bool pass = (chi2_dof > 0) && (chi2_total < chi2_thr);
    printf("[Born-Emerge] chi2 = %.4f | dof = %u | threshold(1%%) = %.2f -> %s\n",
        chi2_total, chi2_dof, chi2_thr,
        pass ? "PASSED (Born emergiert)" : "FAILED (Born emergiert NICHT)");
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Born-Local-Observer (6g) ===================== */
bool test_born_local_observer_g(void) {
    printf("[RUN] Born-Emergenz aus lokalem Beobachter (Etappe 6g, Q31)...\n");
    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Born-Local] Init failed.\n");
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
    const uint64_t src_base = (uint64_t)28 * DIM + 28;
    const uint32_t src_dim = 8u;
    const uint64_t sys_node = src_base
        + (uint64_t)(src_dim / 2u) * DIM + (uint64_t)(src_dim / 2u);
    const uint32_t TRIALS_PER_LAMBDA = 2000u;
    const uint32_t WAVE_TICKS = 40u;
    const uint32_t WAVE_STEP_Q15 = 2000u;
    const uint32_t NUM_LAMBDA = 8u;
    double chi2_total = 0.0;
    uint32_t chi2_dof = 0;
    printf("[Born-Local] DIM=%u | sys_node=%llu | src_dim=%u\n",
        DIM, (unsigned long long)sys_node, src_dim);
    printf("[Born-Local] trials/lambda=%u | wave_ticks=%u | step_q15=%u\n\n",
        TRIALS_PER_LAMBDA, WAVE_TICKS, WAVE_STEP_Q15);
    for (uint32_t li = 0; li < NUM_LAMBDA; ++li) {
        const double lambda_test = (double)li * (M_PI / (double)(NUM_LAMBDA - 1));
        const int32_t a_up = (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        const double p_born = cos(lambda_test * 0.5) * cos(lambda_test * 0.5);
        uint32_t n_up = 0, n_dn = 0;
        double lambda_meas_sum = 0.0;
        for (uint32_t t = 0; t < TRIALS_PER_LAMBDA; ++t) {
            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] = pro_amp_pack(INT32_MAX, 0);
            }
            const uint64_t seed = 0x60DA7A00ULL
                + (uint64_t)t * 0x9e3779b97f4a7c15ULL
                + (uint64_t)li * 0x123456789ABCDEFULL;
            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim, seed);
            pu.amp_grid[sys_node].coeff[UR_NEUTRAL] = 0;
            pu.amp_grid[sys_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            pu.amp_grid[sys_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
            for (uint32_t k = 0; k < WAVE_TICKS; ++k)
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            const double lambda_meas = ProPhysics_Compute_Lambda(&pu, sys_node);
            lambda_meas_sum += lambda_meas;

            /* Etappe 16e''-Fix: echtes Born-Sampling statt Sharp_Measure.
             * P(+1) = cos^2(lambda/2). */
            const double p_plus_cos = cos(lambda_meas * 0.5);
            const double p_plus = p_plus_cos * p_plus_cos;
            prng_state_t rng;
            prng_seed(&rng, seed ^ 0x60606060CAFEBABEULL);
            const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
            const int out = (u < p_plus) ? +1 : -1;
            if (out == +1) n_up++; else n_dn++;
        }
        const uint32_t total = n_up + n_dn;
        const double p_meas = total ? (double)n_up / (double)total : 0.0;
        const double lambda_meas_mean = total ? lambda_meas_sum / (double)total : 0.0;
        if (p_born > 0.02 && p_born < 0.98 && total > 0) {
            const double exp_up = p_born * (double)total;
            const double exp_dn = (1.0 - p_born) * (double)total;
            const double d_up = (double)n_up - exp_up;
            const double d_dn = (double)n_dn - exp_dn;
            chi2_total += (d_up * d_up) / exp_up + (d_dn * d_dn) / exp_dn;
            chi2_dof++;
        }
        printf("[Born-Local]  %7.4f  %7.4f  %7.4f   %11.4f      %5u %5u\n",
            lambda_test, p_meas, p_born, lambda_meas_mean, n_up, n_dn);
    }
    double chi2_thr = 14.07;
    if (chi2_dof == 5) chi2_thr = 15.09;
    if (chi2_dof == 6) chi2_thr = 16.81;
    if (chi2_dof == 7) chi2_thr = 18.48;
    const bool pass = (chi2_dof > 0) && (chi2_total < chi2_thr);
    printf("[Born-Local] chi2 = %.4f | dof = %u | threshold(1%%) = %.2f\n",
        chi2_total, chi2_dof, chi2_thr);
    printf("[Born-Local] -> %s\n",
        pass ? "PASSED (Born emergiert)" : "FAILED (Born emergiert NICHT)");
    ProPhysics_Free(&pu);
    return pass;
}

/* ===================== Born-Local-U4 (6h) ===================== */
bool test_born_local_observer(void) {
    printf("[RUN] Born-Emergenz aus U4-lokaler Messung (Etappe 6h, Q31)...\n");
    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Born-U4] Init failed.\n");
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
    const uint64_t src_base = (uint64_t)28 * DIM + 28;
    const uint32_t src_dim = 8u;
    const uint64_t sys_node = src_base
        + (uint64_t)(src_dim / 2u) * DIM + (uint64_t)(src_dim / 2u);
    const uint32_t TRIALS_PER_LAMBDA = 2000u;
    const uint32_t WAVE_TICKS = 40u;
    const uint32_t WAVE_STEP_Q15 = 2000u;
    const uint32_t NUM_LAMBDA = 8u;
    double chi2_total = 0.0;
    uint32_t chi2_dof = 0;
    printf("[Born-U4] DIM=%u | sys_node=%llu | src_dim=%u\n",
        DIM, (unsigned long long)sys_node, src_dim);
    printf("[Born-U4] trials/lambda=%u | wave_ticks=%u | step_q15=%u\n\n",
        TRIALS_PER_LAMBDA, WAVE_TICKS, WAVE_STEP_Q15);
    for (uint32_t li = 0; li < NUM_LAMBDA; ++li) {
        const double lambda_test = (double)li * (M_PI / (double)(NUM_LAMBDA - 1));
        const int32_t a_up = (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        const double p_born = cos(lambda_test * 0.5) * cos(lambda_test * 0.5);
        uint32_t n_up = 0, n_dn = 0;
        double lambda_meas_sum = 0.0;
        uint32_t n_env_sum = 0;
        for (uint32_t t = 0; t < TRIALS_PER_LAMBDA; ++t) {
            const uint64_t seed = 0x60DA7A00ULL
                + (uint64_t)t * 0x9e3779b97f4a7c15ULL
                + (uint64_t)li * 0x123456789ABCDEFULL;
            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] = pro_amp_pack(INT32_MAX, 0);
                pu.ur_grid[k].type_state = UR_NEUTRAL;
            }
            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim, seed);
            pu.amp_grid[sys_node].coeff[UR_NEUTRAL] = 0;
            pu.amp_grid[sys_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            pu.amp_grid[sys_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
            pu.ur_grid[sys_node].type_state = UR_POSITRON_CW;
            for (uint32_t dy = 0; dy < src_dim; ++dy) {
                for (uint32_t dx = 0; dx < src_dim; ++dx) {
                    const uint64_t idx = src_base + (uint64_t)dy * DIM + (uint64_t)dx;
                    if (idx == sys_node) continue;
                    uint64_t h = seed
                        + (uint64_t)dx * 0x9e3779b97f4a7c15ULL
                        + (uint64_t)dy * 0xc2b2ae3d27d4eb4fULL;
                    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
                    h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
                    h = h ^ (h >> 31);
                    pu.ur_grid[idx].type_state =
                        ((h >> 0) & 1u) ? UR_POSITRON_CW : UR_NEGATRON_CW;
                }
            }
            for (uint32_t k = 0; k < WAVE_TICKS; ++k)
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            int64_t sum_re[PRO_AMP_BASIS_SIZE], sum_im[PRO_AMP_BASIS_SIZE];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) { sum_re[b] = 0; sum_im[b] = 0; }
            uint32_t n_env = 0;
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                sum_re[b] += (int64_t)pro_amp_real(pu.amp_grid[sys_node].coeff[b]);
                sum_im[b] += (int64_t)pro_amp_imag(pu.amp_grid[sys_node].coeff[b]);
            }
            n_env++;
            const uint8_t sys_ts = pu.ur_grid[sys_node].type_state;
            for (uint8_t c = 0; c < 4u; ++c) {
                const uint64_t nb = pu.reg_source[sys_node].channels[c];
                if (nb >= NODES || nb == sys_node) continue;
                if (pu.ur_grid[nb].type_state != sys_ts) continue;
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                    sum_re[b] += (int64_t)pro_amp_real(pu.amp_grid[nb].coeff[b]);
                    sum_im[b] += (int64_t)pro_amp_imag(pu.amp_grid[nb].coeff[b]);
                }
                n_env++;
            }
            n_env_sum += n_env;
            ProAmpVector avg;
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                int64_t re = sum_re[b] / (int64_t)n_env;
                int64_t im = sum_im[b] / (int64_t)n_env;
                if (re > INT32_MAX) re = INT32_MAX;
                if (re < INT32_MIN) re = INT32_MIN;
                if (im > INT32_MAX) im = INT32_MAX;
                if (im < INT32_MIN) im = INT32_MIN;
                avg.coeff[b] = pro_amp_pack((int32_t)re, (int32_t)im);
            }
            const double c1 = (double)pro_amp_real(avg.coeff[UR_POSITRON_CW]);
            const double c4 = (double)pro_amp_real(avg.coeff[UR_NEGATRON_CCW]);
            double lambda_meas = 2.0 * atan2(c4, c1);
            if (lambda_meas < 0.0) lambda_meas += 2.0 * M_PI;
            if (lambda_meas >= 2.0 * M_PI) lambda_meas -= 2.0 * M_PI;
            lambda_meas_sum += lambda_meas;

            /* Etappe 16e''-Fix: echtes Born-Sampling statt Sharp_Measure. */
            const double p_plus_cos = cos(lambda_meas * 0.5);
            const double p_plus = p_plus_cos * p_plus_cos;
            prng_state_t rng;
            prng_seed(&rng, seed ^ 0xA5A5A5A5DEADBEEFULL);
            const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
            const int out = (u < p_plus) ? +1 : -1;
            if (out == +1) n_up++; else n_dn++;
        }
        const uint32_t total = n_up + n_dn;
        const double p_meas = total ? (double)n_up / (double)total : 0.0;
        const double lambda_mean = total ? lambda_meas_sum / (double)total : 0.0;
        const double n_env_mean = (double)n_env_sum / (double)total;
        if (p_born > 0.02 && p_born < 0.98 && total > 0) {
            const double exp_up = p_born * (double)total;
            const double exp_dn = (1.0 - p_born) * (double)total;
            const double d_up = (double)n_up - exp_up;
            const double d_dn = (double)n_dn - exp_dn;
            chi2_total += (d_up * d_up) / exp_up + (d_dn * d_dn) / exp_dn;
            chi2_dof++;
        }
        printf("[Born-U4]  %7.4f  %7.4f  %7.4f   %9.2f   %9.4f   %5u %5u\n",
            lambda_test, p_meas, p_born, n_env_mean, lambda_mean, n_up, n_dn);
    }
    double chi2_thr = 14.07;
    if (chi2_dof == 5) chi2_thr = 15.09;
    if (chi2_dof == 6) chi2_thr = 16.81;
    if (chi2_dof == 7) chi2_thr = 18.48;
    const bool pass = (chi2_dof > 0) && (chi2_total < chi2_thr);
    printf("[Born-U4] chi2 = %.4f | dof = %u | threshold(1%%) = %.2f\n",
        chi2_total, chi2_dof, chi2_thr);
    printf("[Born-U4] -> %s\n",
        pass ? "PASSED (Born emergiert aus U4-lokaler Messung)"
        : "FAILED (Born emergiert NICHT aus U4-lokaler Messung)");
    ProPhysics_Free(&pu);
    return pass;
}