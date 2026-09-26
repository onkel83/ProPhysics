#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== Lorentz-Helfer ===================== */
static uint64_t find_peak_node(const ProUniverse* pu, uint32_t dim,
    uint8_t basis, uint32_t* out_x, uint32_t* out_y)
{
    if (!pu || !pu->amp_grid || dim == 0u) return 0u;
    uint64_t best = 0u;
    double best_val = -1.0;
    for (uint32_t y = 0; y < dim; ++y)
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t k = (uint64_t)y * dim + x;
            if (k >= pu->total_nodes) continue;
            const int32_t re = pro_amp_real(pu->amp_grid[k].coeff[basis]);
            const int32_t im = pro_amp_imag(pu->amp_grid[k].coeff[basis]);
            const double val = (double)re * (double)re + (double)im * (double)im;
            if (val > best_val) { best_val = val; best = k; if (out_x) *out_x = x; if (out_y) *out_y = y; }
        }
    return best;
}

static double measure_phase_at(const ProUniverse* pu, uint64_t node, uint8_t basis) {
    if (!pu || !pu->amp_grid || node >= pu->total_nodes) return 0.0;
    const int32_t re = pro_amp_real(pu->amp_grid[node].coeff[basis]);
    const int32_t im = pro_amp_imag(pu->amp_grid[node].coeff[basis]);
    if (re == 0 && im == 0) return 0.0;
    return atan2((double)im, (double)re);
}

static void prepare_soliton_ampgrid(ProUniverse* pu, uint32_t dim,
    double x0, double y0, double sigma, double amp_scale,
    double k0, uint8_t basis)
{
    if (!pu || !pu->amp_grid || dim == 0u) return;
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    if (N > pu->total_nodes) return;
    double norm_sq = 0.0;
    for (uint32_t x = 0; x < dim; ++x) {
        double dx = (double)x - x0;
        if (dx > (double)dim * 0.5) dx -= (double)dim;
        if (dx < -(double)dim * 0.5) dx += (double)dim;
        const double psi = exp(-dx * dx / (2.0 * sigma * sigma));
        norm_sq += psi * psi;
    }
    if (norm_sq <= 0.0) return;
    const double inv_norm = 1.0 / sqrt(norm_sq);
    for (uint32_t y = 0; y < dim; ++y)
        for (uint32_t x = 0; x < dim; ++x) {
            double dx = (double)x - x0;
            if (dx > (double)dim * 0.5) dx -= (double)dim;
            if (dx < -(double)dim * 0.5) dx += (double)dim;
            const double psi = exp(-dx * dx / (2.0 * sigma * sigma)) * inv_norm;
            const double phase = k0 * dx;
            const double re = psi * cos(phase);
            const double im = psi * sin(phase);
            int32_t re_q31 = (int32_t)lround(re * amp_scale);
            int32_t im_q31 = (int32_t)lround(im * amp_scale);
            const uint64_t k = (uint64_t)y * dim + x;
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu->amp_grid[k].coeff[b] = 0;
            pu->amp_grid[k].coeff[basis] = pro_amp_pack(re_q31, im_q31);
        }
}

static void lorentz_propagate_tick(ProUniverse* pu, uint32_t dim,
    uint32_t theta_q15, int32_t g_q15, uint32_t nl_dt_q15)
{
    ProPhysics_Apply_Edge_Transport_Colored(pu, theta_q15, dim);
    if (g_q15 != 0)
        ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(pu, g_q15, nl_dt_q15);
}

#define LORENTZ_MAX_SAMPLES 128u
typedef struct {
    double   v_c_ratio;
    double   k0;
    double   omega_intern;
    double   v_g_measured;
    double   peak_amp_sq_t0;
    double   peak_amp_sq_tend;
    double   peak_amp_sq_mean;
    uint32_t n_samples;
} LorentzRunResult;

static bool lorentz_run_single(uint32_t dim, double v_c_ratio,
    int32_t g_q15, uint32_t nl_dt_q15, uint32_t theta_q15,
    uint32_t n_ticks, uint32_t n_samples, LorentzRunResult* out)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    out->v_c_ratio = v_c_ratio;
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    const double X0 = (double)(dim / 2), Y0 = (double)(dim / 2);
    const double SIGMA0 = 12.0;
    const double AMP_SCALE = Q31_MAXV * 0.5;
    const uint8_t BASIS = UR_POSITRON_CW;
    double k0 = 0.0;
    if (v_c_ratio > 1e-9) {
        double v_clamped = v_c_ratio;
        if (v_clamped >= 1.0) v_clamped = 0.999;
        k0 = 2.0 * asin(v_clamped);
    }
    out->k0 = k0;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus(&pu, dim);
    prepare_soliton_ampgrid(&pu, dim, X0, Y0, SIGMA0, AMP_SCALE, k0, BASIS);
    double   x_peak_arr[LORENTZ_MAX_SAMPLES];
    double   phi_arr[LORENTZ_MAX_SAMPLES];
    double   amp_sq_arr[LORENTZ_MAX_SAMPLES];
    uint32_t tick_arr[LORENTZ_MAX_SAMPLES];
    uint32_t collected = 0u;
    if (n_samples > LORENTZ_MAX_SAMPLES) n_samples = LORENTZ_MAX_SAMPLES;
    const uint32_t sample_stride = (n_ticks > 0u) ? (n_ticks / n_samples) : 1u;
    if (sample_stride == 0u) { ProPhysics_Free(&pu); return false; }
    const double amp_scale_sq = AMP_SCALE * AMP_SCALE;
    for (uint32_t t = 0; t <= n_ticks; ++t) {
        if (t % sample_stride == 0u && collected < n_samples) {
            uint32_t px = 0u, py = 0u;
            const uint64_t peak = find_peak_node(&pu, dim, BASIS, &px, &py);
            (void)py;
            const double phi = measure_phase_at(&pu, peak, BASIS);
            x_peak_arr[collected] = (double)px;
            phi_arr[collected] = phi;
            tick_arr[collected] = t;
            const int32_t pre = pro_amp_real(pu.amp_grid[peak].coeff[BASIS]);
            const int32_t pim = pro_amp_imag(pu.amp_grid[peak].coeff[BASIS]);
            const double peak_sq = ((double)pre * (double)pre + (double)pim * (double)pim) / amp_scale_sq;
            amp_sq_arr[collected] = peak_sq;
            if (collected == 0u) out->peak_amp_sq_t0 = peak_sq;
            out->peak_amp_sq_tend = peak_sq;
            collected++;
        }
        if (t < n_ticks) lorentz_propagate_tick(&pu, dim, theta_q15, g_q15, nl_dt_q15);
    }
    if (collected < 4u) { ProPhysics_Free(&pu); return false; }
    double amp_sum = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) amp_sum += amp_sq_arr[i];
    out->peak_amp_sq_mean = amp_sum / (double)collected;
    for (uint32_t i = 1u; i < collected; ++i) {
        double dx = x_peak_arr[i] - x_peak_arr[i - 1u];
        if (dx > (double)dim * 0.5) dx -= (double)dim;
        if (dx < -(double)dim * 0.5) dx += (double)dim;
        x_peak_arr[i] = x_peak_arr[i - 1u] + dx;
    }
    for (uint32_t i = 1u; i < collected; ++i) {
        double dphi = phi_arr[i] - phi_arr[i - 1u];
        while (dphi > M_PI) dphi -= 2.0 * M_PI;
        while (dphi < -M_PI) dphi += 2.0 * M_PI;
        phi_arr[i] = phi_arr[i - 1u] + dphi;
    }
    double phi_intern[LORENTZ_MAX_SAMPLES];
    for (uint32_t i = 0u; i < collected; ++i)
        phi_intern[i] = phi_arr[i] - k0 * x_peak_arr[i];
    double sum_t = 0.0, sum_phi = 0.0, sum_x = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) {
        sum_t += (double)tick_arr[i];
        sum_phi += phi_intern[i];
        sum_x += x_peak_arr[i];
    }
    const double t_bar = sum_t / (double)collected;
    const double phi_bar = sum_phi / (double)collected;
    const double x_bar = sum_x / (double)collected;
    double num_phi = 0.0, num_x = 0.0, den = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) {
        const double dt = (double)tick_arr[i] - t_bar;
        num_phi += dt * (phi_intern[i] - phi_bar);
        num_x += dt * (x_peak_arr[i] - x_bar);
        den += dt * dt;
    }
    out->omega_intern = (den > 1e-12) ? (num_phi / den) : 0.0;
    out->v_g_measured = (den > 1e-12) ? (num_x / den) : 0.0;
    out->n_samples = collected;
    ProPhysics_Free(&pu);
    return true;
}

bool test_lorentz_time_dilation_ampgrid(double v_c_ratio) {
    printf("\n========================================================================\n");
    printf("  Etappe 12: Lorentz-Zeitdilatation auf amp_grid (Option B)\n");
    printf("  Vier-Lauf-Kalibrierung: GP-Anteil isoliert durch Subtraktion\n");
    printf("  der reinen Transport-Phase (g=0).\n");
    printf("========================================================================\n");
    const uint32_t DIM = 256u;
    const uint32_t THETA_Q15 = 2000u;
    const int32_t  G_Q15 = 20000;
    const uint32_t NL_DT_Q15 = 32768u;
    const uint32_t N_TICKS = 50u;
    const uint32_t N_SAMPLES = 30u;
    printf("  Parameter: DIM=%u | theta_q15=%u | g_q15=%d | NL_dt=%u\n",
        DIM, THETA_Q15, G_Q15, NL_DT_Q15);
    printf("             Ticks=%u | Samples=%u | sigma0=12.0\n\n", N_TICKS, N_SAMPLES);
    LorentzRunResult A, B, C, D;
    printf("  --- Lauf A: v/c=0.00 | g>0 (Transport + GP) ---\n");
    if (!lorentz_run_single(DIM, 0.0, G_Q15, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &A)) {
        printf("  [Lorentz] Lauf A fehlgeschlagen.\n");
        return false;
    }
    printf("  omega_intern     = %+.6e rad/Tick\n", A.omega_intern);
    printf("  v_g              = %+.6f\n", A.v_g_measured);
    printf("  peak_amp_sq_mean = %.6f\n\n", A.peak_amp_sq_mean);

    printf("  --- Lauf B: v/c=%.2f | g>0 (Transport + GP, geboostet) ---\n", v_c_ratio);
    if (!lorentz_run_single(DIM, v_c_ratio, G_Q15, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &B)) {
        printf("  [Lorentz] Lauf B fehlgeschlagen.\n");
        return false;
    }
    printf("  k0               = %.6f rad/Gitterzelle (2*asin(v))\n", B.k0);
    printf("  omega_intern     = %+.6e rad/Tick\n", B.omega_intern);
    printf("  v_g              = %+.6f\n", B.v_g_measured);
    printf("  peak_amp_sq_mean = %.6f\n\n", B.peak_amp_sq_mean);

    printf("  --- Lauf C: v/c=0.00 | g=0 (nur Transport, Kalibrierung) ---\n");
    if (!lorentz_run_single(DIM, 0.0, 0, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &C)) {
        printf("  [Lorentz] Lauf C fehlgeschlagen.\n");
        return false;
    }
    printf("  omega_intern     = %+.6e rad/Tick (reine Transport-Phase)\n\n", C.omega_intern);

    printf("  --- Lauf D: v/c=%.2f | g=0 (nur Transport, Kalibrierung geboostet) ---\n", v_c_ratio);
    if (!lorentz_run_single(DIM, v_c_ratio, 0, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &D)) {
        printf("  [Lorentz] Lauf D fehlgeschlagen.\n");
        return false;
    }
    printf("  omega_intern     = %+.6e rad/Tick (reine Transport-Phase)\n\n", D.omega_intern);

    const double omega_gp_0 = A.omega_intern - C.omega_intern;
    const double omega_gp_v = B.omega_intern - D.omega_intern;
    const double expected_ratio = sqrt(1.0 - v_c_ratio * v_c_ratio);
    const double measured_ratio = (fabs(omega_gp_0) > 1e-12)
        ? (omega_gp_v / omega_gp_0) : 0.0;
    const double amp_ratio_mean = (A.peak_amp_sq_mean > 1e-12)
        ? (B.peak_amp_sq_mean / A.peak_amp_sq_mean) : 1.0;
    const double measured_ratio_corrected = (fabs(amp_ratio_mean) > 1e-12)
        ? (measured_ratio / amp_ratio_mean) : 0.0;
    const double dev_raw = fabs(measured_ratio - expected_ratio);
    const double dev_cor = fabs(measured_ratio_corrected - expected_ratio);
    printf("========================================================================\n");
    printf("  [Lorentz-Ergebnis, amp_grid-basiert, 4-Lauf-kalibriert]\n");
    printf("    Transport-Phase v=0     = %+.6e\n", C.omega_intern);
    printf("    Transport-Phase v=%.2f  = %+.6e\n", v_c_ratio, D.omega_intern);
    printf("    Gesamt-Phase v=0        = %+.6e\n", A.omega_intern);
    printf("    Gesamt-Phase v=%.2f     = %+.6e\n", v_c_ratio, B.omega_intern);
    printf("    -------------------------------------------\n");
    printf("    GP-Phase v=0            = %+.6e rad/Tick\n", omega_gp_0);
    printf("    GP-Phase v=%.2f         = %+.6e rad/Tick\n", v_c_ratio, omega_gp_v);
    printf("    Erwartet sqrt(1-v^2)    = %.6f\n", expected_ratio);
    printf("    Gemessen  GP_v/GP_0     = %.6f\n", measured_ratio);
    printf("    Amplitude-Verhaeltnis   = %.6f\n", amp_ratio_mean);
    printf("    Korrigiert (GP/GP)/amp  = %.6f\n", measured_ratio_corrected);
    printf("    Abweichung (unkorrigiert) = %.2e\n", dev_raw);
    printf("    Abweichung (korrigiert)   = %.2e\n", dev_cor);
    printf("========================================================================\n");
    const double tolerance = 5e-3;
    const bool signal_ok = (fabs(omega_gp_0) > 1e-10) && (fabs(omega_gp_v) > 1e-10);
    const bool pass_raw = (dev_raw < tolerance) && signal_ok;
    const bool pass_cor = (dev_cor < tolerance) && signal_ok;
    if (pass_raw)
        printf("  [Lorentz] -> PASSED (Zeitdilatation auf amp_grid nachgewiesen)\n");
    else if (pass_cor)
        printf("  [Lorentz] -> PASSED (nach Amplituden-Korrektur)\n");
    else
        printf("  [Lorentz] -> FAILED\n");
    return pass_raw || pass_cor;
}

/* ===================== Soliton-Stabilität ===================== */
bool test_soliton_stability(void) {
    printf("[RUN] Soliton-Stabilitaet, Langzeit (Etappe 11 v4, Q31)...\n");
    const uint32_t DIM = 128u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    const uint32_t THETA = 2000u;
    const double SIGMA0 = 3.0;
    const double X0 = (double)(DIM / 2);
    const double AMP_SCALE = Q31_MAXV * 0.5;
    const uint32_t NL_DT_Q15 = 32768u;
    const uint32_t TICKS = 3000u;
    const int32_t G_VALUES[3] = { 0, 100000, 200000 };
    const char* G_LABELS[3] = { "LINEAR  ", "MODERAT ", "STARK   " };
    const uint32_t N_SAMPLES = 21u;
    const uint32_t SAMPLE_TICKS[21] = {
        0u, 100u, 200u, 300u, 400u, 500u, 600u, 700u, 800u, 900u, 1000u,
        1200u, 1400u, 1600u, 1800u, 2000u, 2200u, 2400u, 2600u, 2800u, 3000u
    };
    double sigma[3][21], maxamp[3][21];
    uint32_t clip_count[3] = { 0, 0, 0 };
    for (int mode = 0; mode < 3; ++mode) {
        for (uint32_t i = 0; i < N_SAMPLES; ++i) { sigma[mode][i] = 0.0; maxamp[mode][i] = 0.0; }
        ProUniverse pu;
        ProPhysics_Initialize(&pu, NODES);
        if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
            printf("[Soliton] Init failed (mode=%d).\n", mode);
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
        for (uint64_t k = 0; k < NODES; ++k)
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu.amp_grid[k].coeff[b] = 0;
        double norm_sq = 0.0;
        for (uint64_t x = 0; x < DIM; ++x) {
            const double dx = (double)x - X0;
            const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0));
            norm_sq += psi * psi;
        }
        const double inv_norm = 1.0 / sqrt(norm_sq);
        for (uint64_t y = 0; y < DIM; ++y)
            for (uint64_t x = 0; x < DIM; ++x) {
                const double dx = (double)x - X0;
                const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0)) * inv_norm;
                const int32_t q31 = (int32_t)lround(psi * AMP_SCALE);
                const uint64_t k = y * DIM + x;
                pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(q31, 0);
            }
        printf("[Soliton] Modus: %s | G_q15=%6d | NL_dt_q15=%u | Ticks=%u\n",
            G_LABELS[mode], G_VALUES[mode], NL_DT_Q15, TICKS);
        uint32_t sample_idx = 0u;
        for (uint32_t step = 0; step <= TICKS; ++step) {
            if (step > 0u && step % 1000u == 0u)
                printf("[Soliton]   ... t=%u/%u\n", step, TICKS);
            if (sample_idx < N_SAMPLES && step == SAMPLE_TICKS[sample_idx]) {
                double rho_x[128];
                for (uint32_t x = 0; x < DIM; ++x) rho_x[x] = 0.0;
                double sum_sq = 0.0, sum_x = 0.0, max_sq = 0.0;
                for (uint64_t y = 0; y < DIM; ++y)
                    for (uint64_t x = 0; x < DIM; ++x) {
                        const uint64_t k = y * DIM + x;
                        const int32_t re_s = pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
                        const int32_t im_s = pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
                        const int64_t re64 = (int64_t)re_s, im64 = (int64_t)im_s;
                        const double a2 = (double)(re64 * re64 + im64 * im64);
                        rho_x[x] += a2;
                        sum_sq += a2;
                        sum_x += a2 * (double)x;
                        if (a2 > max_sq) max_sq = a2;
                        if (re_s == INT32_MAX || re_s == INT32_MIN ||
                            im_s == INT32_MAX || im_s == INT32_MIN)
                            clip_count[mode]++;
                    }
                const double x_bar = (sum_sq > 0.0) ? (sum_x / sum_sq) : 0.0;
                double var_x = 0.0;
                for (uint32_t x = 0; x < DIM; ++x) {
                    const double dx = (double)x - x_bar;
                    var_x += rho_x[x] * dx * dx;
                }
                const double sigma_x = (sum_sq > 0.0) ? sqrt(var_x / sum_sq) : 0.0;
                const double max_a = sqrt(max_sq);
                sigma[mode][sample_idx] = sigma_x;
                maxamp[mode][sample_idx] = max_a;
                sample_idx++;
            }
            if (step < TICKS) {
                ProPhysics_Apply_Edge_Transport_Colored(&pu, THETA, DIM);
                if (G_VALUES[mode] != 0)
                    ProPhysics_Apply_Nonlinear_Phase_Step(&pu, G_VALUES[mode], NL_DT_Q15);
            }
        }
        ProPhysics_Free(&pu);
    }
    printf("\n[Soliton]  step   sig_lin   sig_mod   sig_str   mx_lin    mx_str\n");
    printf("[Soliton]  ----------------------------------------------------------\n");
    for (uint32_t i = 0; i < N_SAMPLES; ++i)
        printf("[Soliton]  %4u   %8.4f  %8.4f  %8.4f  %8.4e  %8.4e\n",
            SAMPLE_TICKS[i], sigma[0][i], sigma[1][i], sigma[2][i],
            maxamp[0][i], maxamp[2][i]);
    const uint32_t TAIL_START = 12u;
    const uint32_t TAIL_END = N_SAMPLES - 1u;
    double slope_tail[3], span_tail[3], mean_tail[3];
    for (int mode = 0; mode < 3; ++mode) {
        const uint32_t n = TAIL_END - TAIL_START + 1u;
        double sum_x = 0.0, sum_y = 0.0, mx = 0.0, mn = 1e30;
        for (uint32_t i = TAIL_START; i <= TAIL_END; ++i) {
            const double x = (double)SAMPLE_TICKS[i];
            const double y = sigma[mode][i];
            sum_x += x; sum_y += y;
            if (y > mx) mx = y;
            if (y < mn) mn = y;
        }
        const double x_bar = sum_x / (double)n;
        const double y_bar = sum_y / (double)n;
        double num = 0.0, den = 0.0;
        for (uint32_t i = TAIL_START; i <= TAIL_END; ++i) {
            const double dx = (double)SAMPLE_TICKS[i] - x_bar;
            num += dx * (sigma[mode][i] - y_bar);
            den += dx * dx;
        }
        slope_tail[mode] = (den > 1e-9) ? (num / den) : 0.0;
        span_tail[mode] = mx - mn;
        mean_tail[mode] = y_bar;
    }
    printf("\n[Soliton] Tail-Analyse ueber t=[%u..%u] (lineare Regression):\n",
        SAMPLE_TICKS[TAIL_START], SAMPLE_TICKS[TAIL_END]);
    printf("[Soliton]   Modus     mean_tail   span_tail   slope_tail   Klasse\n");
    printf("[Soliton]   -----------------------------------------------------------\n");
    const char* class_str[3];
    for (int mode = 0; mode < 3; ++mode) {
        const double sl = slope_tail[mode], sp = span_tail[mode];
        if (fabs(sl) < 0.005 && sp < 0.20) class_str[mode] = "STATIONAER ";
        else if (fabs(sl) < 0.005 && sp > 0.50) class_str[mode] = "BREATHER   ";
        else if (sl < -0.01) class_str[mode] = "RELAXIEREND";
        else if (sl > 0.01) class_str[mode] = "WACHSEND   ";
        else class_str[mode] = "UNKLAR     ";
        printf("[Soliton]   %s  %8.4f    %8.4f   %+8.5f   %s\n",
            G_LABELS[mode], mean_tail[mode], sp, sl, class_str[mode]);
    }
    printf("\n[Soliton] Clipping-Check:\n");
    printf("[Soliton]   LINEAR : %u | MODERAT: %u | STARK: %u\n",
        clip_count[0], clip_count[1], clip_count[2]);
    const bool clipping_ok = (clip_count[0] == 0u) && (clip_count[1] == 0u) && (clip_count[2] == 0u);
    if (!clipping_ok)
        printf("[Soliton] WARNUNG: Clipping erkannt!\n");
    const double sig_ratio_mod = (sigma[0][N_SAMPLES - 1u] > 1e-9)
        ? (sigma[1][N_SAMPLES - 1u] / sigma[0][N_SAMPLES - 1u]) : 0.0;
    const double sig_ratio_str = (sigma[0][N_SAMPLES - 1u] > 1e-9)
        ? (sigma[2][N_SAMPLES - 1u] / sigma[0][N_SAMPLES - 1u]) : 0.0;
    const double mx_ratio_mod = (maxamp[0][N_SAMPLES - 1u] > 1e-9)
        ? (maxamp[1][N_SAMPLES - 1u] / maxamp[0][N_SAMPLES - 1u]) : 0.0;
    const double mx_ratio_str = (maxamp[0][N_SAMPLES - 1u] > 1e-9)
        ? (maxamp[2][N_SAMPLES - 1u] / maxamp[0][N_SAMPLES - 1u]) : 0.0;
    printf("\n[Soliton] Endpunkt-Verhaeltnisse (t=%u):\n", SAMPLE_TICKS[N_SAMPLES - 1u]);
    printf("[Soliton]   sigma_mod/sigma_lin = %.4f\n", sig_ratio_mod);
    printf("[Soliton]   sigma_str/sigma_lin = %.4f\n", sig_ratio_str);
    printf("[Soliton]   maxamp_mod/maxamp_lin = %.4f\n", mx_ratio_mod);
    printf("[Soliton]   maxamp_str/maxamp_lin = %.4f\n", mx_ratio_str);
    const double box_limit = sqrt(((double)DIM * (double)DIM - 1.0) / 12.0);
    const bool linear_saturated = (fabs(sigma[0][N_SAMPLES - 1u] - box_limit) < 2.0);
    printf("\n[Soliton] Linear-Check: sigma_lin(T)=%.4f, Box-Limit=%.4f -> %s\n",
        sigma[0][N_SAMPLES - 1u], box_limit,
        linear_saturated ? "SATURIERT (erwartet)" : "NICHT saturiert");
    const bool focus_mod = (sig_ratio_mod < 0.75) || (mx_ratio_mod > 1.5);
    const bool focus_str = (sig_ratio_str < 0.75) || (mx_ratio_str > 1.5);
    const bool primary_pass = focus_mod || focus_str;
    const bool full_pass = primary_pass && clipping_ok;
    printf("\n[Soliton] Bewertung:\n");
    printf("[Soliton]   Selbstfokussierung MODERAT : %s\n", focus_mod ? "JA" : "nein");
    printf("[Soliton]   Selbstfokussierung STARK   : %s\n", focus_str ? "JA" : "nein");
    printf("[Soliton]   Clipping-frei              : %s\n", clipping_ok ? "JA" : "NEIN");
    printf("[Soliton]   MODERAT-Klasse            : %s\n", class_str[1]);
    printf("[Soliton]   STARK-Klasse              : %s\n", class_str[2]);
    printf("\n[Soliton] -> %s\n",
        full_pass ? "PASSED" : "FAILED");
    return full_pass;
}