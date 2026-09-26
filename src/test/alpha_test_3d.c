/* ==========================================================================
 * alpha_test_3d.c
 *
 * Etappe 17: 3D-Torus-Tests.
 *
 * Hinweis: KEIN <complex.h>. MSVC-C unterstuetzt es nicht zuverlaessig.
 * Stattdessen ein kleines Cx-Struct mit den noetigen Operationen.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* --------------------------------------------------------------------------
  * Kleine Complex-Arithmetik.
  * -------------------------------------------------------------------------- */
typedef struct { double re, im; } Cx;

static Cx cx(double re, double im) { Cx c; c.re = re; c.im = im; return c; }
static Cx cx_add(Cx a, Cx b) { return cx(a.re + b.re, a.im + b.im); }
static Cx cx_sub(Cx a, Cx b) { return cx(a.re - b.re, a.im - b.im); }
static Cx cx_mul(Cx a, Cx b) {
    return cx(a.re * b.re - a.im * b.im,
        a.re * b.im + a.im * b.re);
}
static Cx cx_scale(Cx a, double s) { return cx(a.re * s, a.im * s); }
static Cx cx_conj(Cx a) { return cx(a.re, -a.im); }
static double cx_abs2(Cx a) { return a.re * a.re + a.im * a.im; }
static double cx_abs(Cx a) { return sqrt(cx_abs2(a)); }
static double cx_arg(Cx a) { return atan2(a.im, a.re); }
static Cx cx_exp_i(double phase) { return cx(cos(phase), sin(phase)); }

static Cx cx_div(Cx a, Cx b) {
    const double d = b.re * b.re + b.im * b.im;
    if (d < 1e-300) return cx(0.0, 0.0);
    return cx((a.re * b.re + a.im * b.im) / d,
        (a.im * b.re - a.re * b.im) / d);
}

static Cx cx_sqrt(Cx a) {
    const double r = sqrt(a.re * a.re + a.im * a.im);
    double re = sqrt((r + a.re) * 0.5);
    double im = sqrt((r - a.re) * 0.5);
    if (a.im < 0.0) im = -im;
    return cx(re, im);
}

/* --------------------------------------------------------------------------
 * 2x2-Matrix ueber Cx.
 * -------------------------------------------------------------------------- */
typedef struct { Cx m[2][2]; } Cx2;

static Cx2 cx2_mul(Cx2 A, Cx2 B) {
    Cx2 C;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            C.m[i][j] = cx_add(cx_mul(A.m[i][0], B.m[0][j]),
                cx_mul(A.m[i][1], B.m[1][j]));
        }
    }
    return C;
}

/* --------------------------------------------------------------------------
 * Gemeinsamer Index-Helfer: bit-interleaved (z, y, x).
 * -------------------------------------------------------------------------- */
static inline uint64_t idx3d(uint32_t x, uint32_t y, uint32_t z, uint32_t shift)
{
    return ((uint64_t)z << (2u * shift)) | ((uint64_t)y << shift) | (uint64_t)x;
}

/* ==========================================================================
 * test_3d_smoke_ballistic
 * ========================================================================== */
bool test_3d_smoke_ballistic(void)
{
    printf("[RUN] 3D-Smoke: ballistische Ausbreitung, 64^3-Torus, Punktimpuls\n");

    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * DIM * DIM;
    const uint32_t THETA_Q15 = 2000u;
    const uint32_t c = DIM / 2u;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        printf("[3D] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_3d(&pu, DIM);

    uint32_t shift = 0u;
    while ((1u << shift) < DIM) shift++;

    for (uint64_t k = 0; k < NODES; ++k)
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
    const uint64_t src = idx3d(c, c, c, shift);
    pu.amp_grid[src].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(8192), 0);

    const ProU128 inv0 = ProPhysics_Measure_Amp_Invariant(&pu);
    const double inv0_d = pro_u128_to_double(inv0);

    const uint32_t T_SAMPLES[5] = { 0u, 5u, 10u, 15u, 20u };
    double sigma[5] = { 0, 0, 0, 0, 0 };
    double sum_amp[5] = { 0, 0, 0, 0, 0 };

    for (int s = 0; s < 5; ++s) {
        if (s > 0) {
            for (uint32_t t = T_SAMPLES[s - 1]; t < T_SAMPLES[s]; ++t)
                ProPhysics_Apply_Edge_Transport_Colored_3D(
                    &pu, THETA_Q15, DIM);
        }

        double sum_sq = 0.0, sum_x = 0.0;
        for (uint64_t k = 0; k < NODES; ++k) {
            const int32_t re = pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
            const int32_t im = pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
            const double a2 = (double)re * (double)re + (double)im * (double)im;
            const uint32_t x = (uint32_t)k & (DIM - 1u);
            sum_sq += a2;
            sum_x += a2 * (double)x;
        }
        sum_amp[s] = sum_sq;
        const double x_bar = (sum_sq > 0.0) ? (sum_x / sum_sq) : 0.0;

        double var = 0.0;
        for (uint64_t k = 0; k < NODES; ++k) {
            const int32_t re = pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
            const int32_t im = pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
            const double a2 = (double)re * (double)re + (double)im * (double)im;
            const uint32_t x = (uint32_t)k & (DIM - 1u);
            const double dx = (double)x - x_bar;
            var += a2 * dx * dx;
        }
        sigma[s] = (sum_sq > 0.0) ? sqrt(var / sum_sq) : 0.0;
    }

    const ProU128 inv1 = ProPhysics_Measure_Amp_Invariant(&pu);
    const double inv1_d = pro_u128_to_double(inv1);
    const double drift = (inv0_d > 0.0) ? fabs(inv1_d - inv0_d) / inv0_d : 1.0;

    printf("[3D]  step   sigma_x    sum_amp\n");
    for (int s = 0; s < 5; ++s)
        printf("[3D]  %4u   %.4f     %.4e\n", T_SAMPLES[s], sigma[s], sum_amp[s]);

    const double r_20_10 = (sigma[2] > 1e-9) ? sigma[4] / sigma[2] : 0.0;
    const double r_20_05 = (sigma[1] > 1e-9) ? sigma[4] / sigma[1] : 0.0;

    printf("[3D] sigma(20)/sigma(10) = %.4f (ballistisch ~2.0)\n", r_20_10);
    printf("[3D] sigma(20)/sigma( 5) = %.4f (ballistisch ~4.0)\n", r_20_05);
    printf("[3D] U5-Drift = %.4e (Schwelle 5e-2)\n", drift);

    const bool ballistic_ok = (r_20_10 > 1.7) && (r_20_05 > 3.2);
    const bool u5_ok = (drift < 5e-2);
    const bool pass = ballistic_ok && u5_ok;

    printf("[3D] Ballistik: %s | U5: %s -> %s\n",
        ballistic_ok ? "OK" : "FAILED",
        u5_ok ? "OK" : "FAILED",
        pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}

/* ==========================================================================
 * test_3d_invariance_under_tick
 * ========================================================================== */
bool test_3d_invariance_under_tick(uint32_t dim, uint32_t ticks, uint32_t seed)
{
    printf("[RUN] 3D-U5-Invariante ueber %u Ticks, dim=%u\n", ticks, dim);

    const uint64_t NODES = (uint64_t)dim * dim * dim;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid) {
        printf("[3D-Inv] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_3d(&pu, dim);

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
        pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(8192), 0);
        pu.amp_grid[k].coeff[UR_NEGATRON_CCW] = pro_amp_pack(Q15TO31(8192), 0);
    }
    ProPhysics_Apply_Guiding_Equation(&pu);

    const ProU128 base = ProPhysics_Measure_Amp_Invariant(&pu);
    const double base_d = pro_u128_to_double(base);

    double max_rel_drift = 0.0;
    uint32_t first_violation = 0u;
    uint64_t violation_count = 0u;
    const double threshold = 5e-2;

    (void)seed;

    for (uint32_t t = 1; t <= ticks; ++t) {
        ProPhysics_Apply_Edge_Transport_Colored_3D(
            &pu, PRO_DEFAULT_TRANSPORT_THETA_Q15, dim);
        ProPhysics_Apply_Wave_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
        ProPhysics_Apply_Context_Tick(&pu);
        ProPhysics_Apply_Guiding_Equation(&pu);

        const ProU128 cur = ProPhysics_Measure_Amp_Invariant(&pu);
        const double cur_d = pro_u128_to_double(cur);
        const double rel = (base_d > 0.0) ? fabs(cur_d - base_d) / base_d : 1.0;
        if (rel > max_rel_drift) max_rel_drift = rel;
        if (rel > threshold) {
            violation_count++;
            if (first_violation == 0u) first_violation = t;
        }
    }

    printf("[3D-Inv] Base = %.6e | max rel.drift = %.4e\n",
        base_d, max_rel_drift);
    printf("[3D-Inv] Verletzungen = %llu",
        (unsigned long long)violation_count);
    if (first_violation) printf(" | erste bei Tick %u", first_violation);
    printf("\n");

    const bool pass = (violation_count == 0u);
    printf("[3D-Inv] -> %s\n", pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}

/* ==========================================================================
 * test_3d_dispersion
 *
 * Numerische Bloch-Vorhersage. Pro Achse a in {x,y,z} mit Wellenzahl qa:
 *   ua = e^{i qa}
 *   M_a_even = [[c,        i*s*ua ]]
 *              [[i*s/ua,   c      ]]
 *   M_a_odd  = [[c,        i*s/ua ]]
 *              [[i*s*ua,   c      ]]
 *   M_a = M_a_odd * M_a_even.
 *   U_3D = M_z * M_y * M_x.
 *   omega = arg(lambda+), lambda+ = Eigenwert von U_3D mit Im > 0.
 * ========================================================================== */

static Cx2 axis_matrix(double q, double c, double s, int odd)
{
    const Cx u = cx_exp_i(q);
    const Cx u_inv = cx_conj(u);   /* 1/u, da |u| = 1 */

    /* i*s*u  und  i*s/u */
    const Cx isu = cx_mul(cx(0.0, s), u);
    const Cx isu_inv = cx_mul(cx(0.0, s), u_inv);

    Cx2 M;
    if (!odd) {
        M.m[0][0] = cx(c, 0.0);   M.m[0][1] = isu;
        M.m[1][0] = isu_inv;      M.m[1][1] = cx(c, 0.0);
    }
    else {
        M.m[0][0] = cx(c, 0.0);   M.m[0][1] = isu_inv;
        M.m[1][0] = isu;          M.m[1][1] = cx(c, 0.0);
    }
    return M;
}

static Cx2 u_3d(double kx, double ky, double kz, double theta)
{
    const double c = cos(theta);
    const double s = sin(theta);
    Cx2 Mx = cx2_mul(axis_matrix(kx, c, s, 1), axis_matrix(kx, c, s, 0));
    Cx2 My = cx2_mul(axis_matrix(ky, c, s, 1), axis_matrix(ky, c, s, 0));
    Cx2 Mz = cx2_mul(axis_matrix(kz, c, s, 1), axis_matrix(kz, c, s, 0));
    return cx2_mul(cx2_mul(Mz, My), Mx);
}

static Cx eigenvector_beta(Cx2 U, Cx lam)
{
    /* beta = (lam - U00)/U01  oder  beta = U10/(lam - U11); nimm stabilere */
    Cx b1 = cx(0.0, 0.0), b2 = cx(0.0, 0.0);
    if (cx_abs(U.m[0][1]) > 1e-12)
        b1 = cx_div(cx_sub(lam, U.m[0][0]), U.m[0][1]);
    if (cx_abs(cx_sub(lam, U.m[1][1])) > 1e-12)
        b2 = cx_div(U.m[1][0], cx_sub(lam, U.m[1][1]));
    return (cx_abs(b1) > cx_abs(b2)) ? b1 : b2;
}

bool test_3d_dispersion(void)
{
    printf("[RUN] 3D-Bloch-Dispersion omega(kx,ky,kz), DIM=64\n");

    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * DIM * DIM;
    const uint32_t THETA_Q15 = 2000u;
    const uint32_t N_TICKS = 80u;
    const uint32_t N_SAMPLES = 40u;

    const double theta_rad = ((double)THETA_Q15 / 32768.0) * M_PI;

    const uint32_t m_triples[4][3] = {
        {  4u,  0u,  0u },
        {  4u,  4u,  0u },
        {  4u,  4u,  4u },
        {  8u,  4u,  2u }
    };

    printf("[3D-Disp] theta_q15=%u | DIM=%u | N_TICKS=%u\n",
        THETA_Q15, DIM, N_TICKS);
    printf("[3D-Disp]  mx my mz   omega_theory   omega_meas    rel_dev    amp_drift\n");
    printf("[3D-Disp]  ---------------------------------------------------------------\n");

    double max_rel_dev = 0.0;
    uint32_t n_valid = 0u, n_pass = 0u;

    for (int t = 0; t < 4; ++t) {
        const double kx = 2.0 * M_PI * (double)m_triples[t][0] / (double)DIM;
        const double ky = 2.0 * M_PI * (double)m_triples[t][1] / (double)DIM;
        const double kz = 2.0 * M_PI * (double)m_triples[t][2] / (double)DIM;

        Cx2 U = u_3d(kx, ky, kz, theta_rad);

        const Cx tr = cx_add(U.m[0][0], U.m[1][1]);
        const Cx det = cx_sub(cx_mul(U.m[0][0], U.m[1][1]),
            cx_mul(U.m[0][1], U.m[1][0]));
        const Cx disc = cx_sqrt(cx_sub(cx_mul(tr, tr), cx_scale(det, 4.0)));
        const Cx lam1 = cx_scale(cx_add(tr, disc), 0.5);
        const Cx lam2 = cx_scale(cx_sub(tr, disc), 0.5);

        const Cx lam_p = (cx_arg(lam1) > 0.0) ? lam1 : lam2;
        const double omega_theory = cx_arg(lam_p);

        const Cx beta = eigenvector_beta(U, lam_p);
        const Cx alpha = cx(1.0, 0.0);

        const double norm2 = cx_abs2(alpha) + cx_abs2(beta);
        const double scale = sqrt(2.0 / ((double)NODES * norm2));

        ProUniverse pu;
        ProPhysics_Initialize(&pu, NODES);
        if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch) {
            ProPhysics_Free(&pu);
            continue;
        }
        wire_torus_3d(&pu, DIM);

        uint32_t shift = 0u;
        while ((1u << shift) < DIM) shift++;

        for (uint32_t z = 0; z < DIM; ++z) {
            for (uint32_t y = 0; y < DIM; ++y) {
                for (uint32_t x = 0; x < DIM; ++x) {
                    const uint64_t idx = idx3d(x, y, z, shift);
                    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                        pu.amp_grid[idx].coeff[b] = 0;

                    const double phase = kx * (double)x
                        + ky * (double)y
                        + kz * (double)z;
                    const double cph = cos(phase);
                    const double sph = sin(phase);
                    const bool even = (((x + y + z) & 1u) == 0u);
                    const double are = even ? alpha.re : beta.re;
                    const double aim = even ? alpha.im : beta.im;

                    const double re = (are * cph - aim * sph) * scale;
                    const double im = (are * sph + aim * cph) * scale;

                    const int32_t re_q = (int32_t)lround(re * Q31_MAXV);
                    const int32_t im_q = (int32_t)lround(im * Q31_MAXV);
                    pu.amp_grid[idx].coeff[UR_POSITRON_CW] =
                        pro_amp_pack(re_q, im_q);
                }
            }
        }

        double phi_arr[64];
        double tick_arr[64];
        uint32_t collected = 0u;
        double amp_start = 0.0;
        const uint32_t stride = N_TICKS / N_SAMPLES;
        if (stride == 0u) { ProPhysics_Free(&pu); continue; }

        for (uint32_t tt = 0; tt <= N_TICKS; ++tt) {
            if (tt % stride == 0u && collected < N_SAMPLES) {
                const int32_t re0 = pro_amp_real(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
                const int32_t im0 = pro_amp_imag(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
                phi_arr[collected] = atan2((double)im0, (double)re0);
                tick_arr[collected] = (double)tt;
                const double a2 = (double)re0 * (double)re0
                    + (double)im0 * (double)im0;
                if (tt == 0u) amp_start = a2;
                collected++;
            }
            if (tt < N_TICKS)
                ProPhysics_Apply_Edge_Transport_Colored_3D(
                    &pu, THETA_Q15, DIM);
        }

        if (collected < 4u) { ProPhysics_Free(&pu); continue; }

        const int32_t re_end = pro_amp_real(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
        const int32_t im_end = pro_amp_imag(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
        const double a2_end = (double)re_end * (double)re_end
            + (double)im_end * (double)im_end;
        const double amp_drift = (amp_start > 1.0)
            ? fabs(a2_end - amp_start) / amp_start : 0.0;

        for (uint32_t i = 1; i < collected; ++i) {
            double dphi = phi_arr[i] - phi_arr[i - 1];
            while (dphi > M_PI) dphi -= 2.0 * M_PI;
            while (dphi < -M_PI) dphi += 2.0 * M_PI;
            phi_arr[i] = phi_arr[i - 1] + dphi;
        }
        double sum_t = 0.0, sum_p = 0.0;
        for (uint32_t i = 0; i < collected; ++i) {
            sum_t += tick_arr[i];
            sum_p += phi_arr[i];
        }
        const double t_bar = sum_t / (double)collected;
        const double p_bar = sum_p / (double)collected;
        double num = 0.0, den = 0.0;
        for (uint32_t i = 0; i < collected; ++i) {
            const double dt = tick_arr[i] - t_bar;
            num += dt * (phi_arr[i] - p_bar);
            den += dt * dt;
        }
        const double omega_meas = (den > 1e-12) ? (num / den) : 0.0;

        const double rel_dev = (fabs(omega_theory) > 1e-6)
            ? fabs(omega_meas - omega_theory) / fabs(omega_theory)
            : fabs(omega_meas);
        if (rel_dev > max_rel_dev) max_rel_dev = rel_dev;
        n_valid++;

        const bool local_pass = (rel_dev < 5e-3) && (amp_drift < 1e-3);
        if (local_pass) n_pass++;

        printf("[3D-Disp]  %2u %2u %2u    %+.6f     %+.6f     %.2e     %.2e  %s\n",
            m_triples[t][0], m_triples[t][1], m_triples[t][2],
            omega_theory, omega_meas, rel_dev, amp_drift,
            local_pass ? "" : "<--");

        ProPhysics_Free(&pu);
    }

    printf("[3D-Disp] %u/%u bestanden | max rel_dev = %.4e\n",
        n_pass, n_valid, max_rel_dev);

    const bool pass = (n_valid > 0u) && (n_pass == n_valid);
    printf("[3D-Disp] -> %s\n",
        pass ? "PASSED (3D-Bloch-Vorhersage bestaetigt)"
        : "FAILED (Modell weicht von Bloch-Vorhersage ab)");
    return pass;
}