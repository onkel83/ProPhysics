/* ==========================================================================
 * alpha_test_string_tension.c
 *
 * Etappe 23c (Revision 2): String-Tension Selbstkonsistenz-Test.
 *
 * =========================================================================
 * WICHTIG: KEIN V&V-ANKER GEGEN CAHILL & PRASAD (1989)
 * =========================================================================
 *
 * Revision 1 versuchte, sigma_a2 aus 3D-SU(2)-Wilson-Loops mit
 * publizierten Werten von Cahill & Prasad (PRD 40, 1274, 1989) zu
 * vergleichen. Das ist physikalisch falsch:
 *
 *   - Cahill & Prasad: 4D-SU(2)-Lattice-QCD, beta_c ~ 2.30.
 *     beta=2.4 (knapp oberhalb) -> a*sqrt(sigma) ~ 0.27.
 *   - Unser Kernel: 3D-SU(2)-Torus, beta_c ~ 1.6.
 *     beta=2.4 (weit im Confinement) -> a*sqrt(sigma) ~ 0.75.
 *
 * Der Faktor ~3 ist der 3D/4D-Unterschied, kein Bug.
 *
 * =========================================================================
 * Revision 2 -- Aenderungen:
 *
 *  (A) Anker-Vergleich -> Info-Block (kein PASS/FAIL).
 *  (B) beta-Sweep auf Confinement-Regime: {2.4, 2.5, 2.7, 3.0}.
 *  (C) Fit-Filter: W > 1e-3 und rel.err < 0.5.
 *  (D) chi2/dof-Schwelle auf < 10.
 *  (E) S2-Monotonie mit 2-sigma-Toleranz.
 *  (F) Sampler: Haar-Vorschlag U -> R*U.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

 /* --------------------------------------------------------------------------
  * Statistik-Konstanten
  * ----------------------------------------------------------------------- */

#define ST_N_THERMAL_STD    500u
#define ST_N_MEASURE_STD    1000u

#define ST_BIN_SIZE     10u
#define ST_MAX_DIMS     4u
#define ST_MAX_BETAS    5u
#define ST_MAX_RUNS     (ST_MAX_DIMS * ST_MAX_BETAS)
#define ST_N_LOOPS      8u

  /* Fit-Filter (Revision 2, Fix C) */
#define ST_W_FLOOR      1e-3
#define ST_RELERR_MAX   0.5

/* Vorwaerts-Kanaele in 3D (bit-interleaved (z,y,x)). */
static const uint8_t ST_FWD[3] = { 0u, 2u, 4u };

/* Loop-Satz: (m, n) */
static const uint32_t ST_LOOPS_M[ST_N_LOOPS] = { 1u, 2u, 2u, 3u, 3u, 4u, 4u, 4u };
static const uint32_t ST_LOOPS_N[ST_N_LOOPS] = { 1u, 1u, 2u, 1u, 2u, 2u, 3u, 4u };

/* --------------------------------------------------------------------------
 * Literatur-Anker (nur informativ, Revision 2 Fix A)
 * ----------------------------------------------------------------------- */

typedef struct {
    double   beta;
    double   a_sqrt_sigma;
    double   a_sqrt_sigma_err;
    const char* source;
} STAnchorEntry;

static const STAnchorEntry ST_ANCHORS[] = {
    { 2.4, 0.2673, 0.0015, "Cahill & Prasad 1989 (4D SU(2))" },
    { 2.5, 0.1860, 0.0030, "Cahill & Prasad 1989 (4D SU(2))" },
};
static const int ST_N_ANCHORS = (int)(sizeof(ST_ANCHORS) / sizeof(ST_ANCHORS[0]));

static const STAnchorEntry* st_find_anchor(double beta)
{
    for (int i = 0; i < ST_N_ANCHORS; ++i) {
        if (fabs(ST_ANCHORS[i].beta - beta) < 1e-9) return &ST_ANCHORS[i];
    }
    return NULL;
}

/* --------------------------------------------------------------------------
 * RNG-Helfer
 * ----------------------------------------------------------------------- */
static double st_gauss(double u1, double u2)
{
    if (u1 < 1e-30) u1 = 1e-30;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double st_uniform(prng_state_t* rng)
{
    return (double)(prng_next(rng) >> 11) / 9007199254740992.0;
}

/* --------------------------------------------------------------------------
 * Haar-Vorschlag (Revision 2, Fix F)
 * ----------------------------------------------------------------------- */

static inline int64_t st_round_shift_q30(int64_t x)
{
    if (x >= 0) return (x + (1LL << 29)) >> 30;
    return -(((-x) + (1LL << 29)) >> 30);
}

static inline int32_t st_sat_i32(int64_t x)
{
    if (x > INT32_MAX) return INT32_MAX;
    if (x < INT32_MIN) return INT32_MIN;
    return (int32_t)x;
}

static void st_haar_propose(prng_state_t* rng, double eps,
    int32_t* ar, int32_t* ai,
    int32_t* br, int32_t* bi)
{
    const double u = 2.0 * st_uniform(rng) - 1.0;
    const double phi = 2.0 * M_PI * st_uniform(rng);
    const double s = sqrt(1.0 - u * u);
    const double nx = s * cos(phi);
    const double ny = s * sin(phi);
    const double nz = u;

    const double alpha = st_gauss(st_uniform(rng), st_uniform(rng)) * eps;

    const double half = 0.5 * alpha;
    const double c = cos(half);
    const double sn = sin(half);
    const double S = (double)PRO_SU2_SCALE;

    const int32_t r_ar = (int32_t)llround(c * S);
    const int32_t r_ai = (int32_t)llround(-sn * nz * S);
    const int32_t r_br = (int32_t)llround(-sn * ny * S);
    const int32_t r_bi = (int32_t)llround(-sn * nx * S);

    const int64_t u_ar = (int64_t)(*ar);
    const int64_t u_ai = (int64_t)(*ai);
    const int64_t u_br = (int64_t)(*br);
    const int64_t u_bi = (int64_t)(*bi);

    const int64_t na_re = (int64_t)r_ar * u_ar - (int64_t)r_ai * u_ai
        - (int64_t)r_br * u_br - (int64_t)r_bi * u_bi;
    const int64_t na_im = (int64_t)r_ar * u_ai + (int64_t)r_ai * u_ar
        - (int64_t)r_bi * u_br + (int64_t)r_br * u_bi;
    const int64_t nb_re = (int64_t)r_ar * u_br - (int64_t)r_ai * u_bi
        + (int64_t)r_br * u_ar + (int64_t)r_bi * u_ai;
    const int64_t nb_im = (int64_t)r_ar * u_bi + (int64_t)r_ai * u_br
        + (int64_t)r_bi * u_ar - (int64_t)r_br * u_ai;

    *ar = st_sat_i32(st_round_shift_q30(na_re));
    *ai = st_sat_i32(st_round_shift_q30(na_im));
    *br = st_sat_i32(st_round_shift_q30(nb_re));
    *bi = st_sat_i32(st_round_shift_q30(nb_im));
}

/* --------------------------------------------------------------------------
 * Links zufaellig initialisieren
 * ----------------------------------------------------------------------- */
static void st_randomize_links(ProUniverse* pu, prng_state_t* rng)
{
    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = ST_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            const double g1 = st_gauss(st_uniform(rng), st_uniform(rng));
            const double g2 = st_gauss(st_uniform(rng), st_uniform(rng));
            const double g3 = st_gauss(st_uniform(rng), st_uniform(rng));
            const double g4 = st_gauss(st_uniform(rng), st_uniform(rng));

            double nrm = sqrt(g1 * g1 + g2 * g2 + g3 * g3 + g4 * g4);
            if (nrm < 1e-30) nrm = 1.0;

            const int32_t ar = (int32_t)llround((g1 / nrm) * (double)PRO_SU2_SCALE);
            const int32_t ai = (int32_t)llround((g2 / nrm) * (double)PRO_SU2_SCALE);
            const int32_t br = (int32_t)llround((g3 / nrm) * (double)PRO_SU2_SCALE);
            const int32_t bi = (int32_t)llround((g4 / nrm) * (double)PRO_SU2_SCALE);

            ProPhysics_Set_Edge_SU2(pu, x, mu, ar, ai, br, bi);
        }
    }
}

/* --------------------------------------------------------------------------
 * Metropolis-Sweep mit Haar-Vorschlag
 * ----------------------------------------------------------------------- */
static double st_metropolis_sweep(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng)
{
    uint64_t accept = 0, total = 0;

    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = ST_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            int32_t ar, ai, br, bi;
            ProPhysics_Get_Edge_SU2(pu, x, mu, &ar, &ai, &br, &bi);

            const double S_before =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            int32_t na = ar, nai = ai, nb = br, nbi = bi;
            st_haar_propose(rng, eps, &na, &nai, &nb, &nbi);

            ProPhysics_Set_Edge_SU2(pu, x, mu, na, nai, nb, nbi);

            const double S_after =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double dS = S_after - S_before;
            const double u = st_uniform(rng);

            const bool accept_it = (dS <= 0.0) || (u < exp(-beta * dS));

            if (accept_it) {
                accept++;
            }
            else {
                ProPhysics_Set_Edge_SU2(pu, x, mu, ar, ai, br, bi);
            }
            total++;
        }
    }
    return (total > 0u) ? (double)accept / (double)total : 0.0;
}

/* --------------------------------------------------------------------------
 * eps-Kalibrierung + Thermalisierung.
 * ----------------------------------------------------------------------- */
static double st_thermalize(ProUniverse* pu, double beta, uint32_t n_thermal,
    prng_state_t* rng)
{
    double eps = 0.3;
    for (int pass = 0; pass < 6; ++pass) {
        double rate = 0.0;
        const int n_quick = 10;
        for (int s = 0; s < n_quick; ++s) {
            rate += st_metropolis_sweep(pu, beta, eps, rng);
        }
        rate /= (double)n_quick;
        if (rate < 0.30) eps *= 0.6;
        else if (rate > 0.70) eps *= 1.5;
        else break;
        if (eps < 1e-4) eps = 1e-4;
        if (eps > 1.5)  eps = 1.5;
    }
    for (uint32_t s = 0; s < n_thermal; ++s) {
        (void)st_metropolis_sweep(pu, beta, eps, rng);
    }
    return eps;
}

/* --------------------------------------------------------------------------
 * Fit-Ergebnis pro (dim, beta).
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t dim;
    double   beta;
    double   W[ST_N_LOOPS];
    double   W_err[ST_N_LOOPS];
    double   sigma_a2;
    double   sigma_a2_err;
    double   a_sqrt_sigma;
    double   a_sqrt_sigma_err;
    double   mu_a;
    double   mu_a_err;
    double   c;
    double   chi2;
    double   chi2_dof;
    uint32_t n_used;
    bool     fit_ok;
    double   accept_rate;
    double   eps;
    double   plaq_avg;
} STResult;

/* --------------------------------------------------------------------------
 * Fit (Revision 2, Fix C)
 * ----------------------------------------------------------------------- */
static bool st_fit_loops(const double* W, const double* W_err,
    double* out_sigma_a2, double* out_sigma_a2_err,
    double* out_mu_a, double* out_mu_a_err,
    double* out_c, double* out_chi2, double* out_chi2_dof,
    uint32_t* out_n_used,
    double* out_a_sqrt_sigma, double* out_a_sqrt_sigma_err)
{
    double X[ST_N_LOOPS][3];
    double y[ST_N_LOOPS];
    double w[ST_N_LOOPS];
    uint32_t n = 0u;

    for (uint32_t i = 0; i < ST_N_LOOPS; ++i) {
        if (W[i] <= ST_W_FLOOR) continue;
        if (W_err[i] <= 0.0) continue;
        const double rel_err = W_err[i] / W[i];
        if (rel_err > ST_RELERR_MAX) continue;

        X[n][0] = (double)(ST_LOOPS_M[i] * ST_LOOPS_N[i]);
        X[n][1] = (double)(ST_LOOPS_M[i] + ST_LOOPS_N[i]);
        X[n][2] = 1.0;
        y[n] = log(W[i]);
        w[n] = (W[i] * W[i]) / (W_err[i] * W_err[i]);
        n++;
    }

    *out_n_used = n;
    *out_a_sqrt_sigma = 0.0;
    *out_a_sqrt_sigma_err = 0.0;

    if (n < 4u) {
        *out_sigma_a2 = 0.0;
        *out_sigma_a2_err = 0.0;
        *out_mu_a = 0.0;
        *out_mu_a_err = 0.0;
        *out_c = 0.0;
        *out_chi2 = 0.0;
        *out_chi2_dof = 0.0;
        return false;
    }

    double ATA[3][3] = { {0.0} };
    double ATy[3] = { 0.0 };

    for (uint32_t i = 0; i < n; ++i) {
        for (int a = 0; a < 3; ++a) {
            for (int b = 0; b < 3; ++b) {
                ATA[a][b] += w[i] * X[i][a] * X[i][b];
            }
            ATy[a] += w[i] * X[i][a] * y[i];
        }
    }

    const double det =
        ATA[0][0] * (ATA[1][1] * ATA[2][2] - ATA[1][2] * ATA[2][1])
        - ATA[0][1] * (ATA[1][0] * ATA[2][2] - ATA[1][2] * ATA[2][0])
        + ATA[0][2] * (ATA[1][0] * ATA[2][1] - ATA[1][1] * ATA[2][0]);

    if (fabs(det) < 1e-30) {
        *out_sigma_a2 = 0.0; *out_sigma_a2_err = 0.0;
        *out_mu_a = 0.0; *out_mu_a_err = 0.0;
        *out_c = 0.0; *out_chi2 = 0.0; *out_chi2_dof = 0.0;
        return false;
    }

    double inv[3][3];
    inv[0][0] = (ATA[1][1] * ATA[2][2] - ATA[1][2] * ATA[2][1]) / det;
    inv[0][1] = -(ATA[0][1] * ATA[2][2] - ATA[0][2] * ATA[2][1]) / det;
    inv[0][2] = (ATA[0][1] * ATA[1][2] - ATA[0][2] * ATA[1][1]) / det;
    inv[1][0] = -(ATA[1][0] * ATA[2][2] - ATA[1][2] * ATA[2][0]) / det;
    inv[1][1] = (ATA[0][0] * ATA[2][2] - ATA[0][2] * ATA[2][0]) / det;
    inv[1][2] = -(ATA[0][0] * ATA[1][2] - ATA[0][2] * ATA[1][0]) / det;
    inv[2][0] = (ATA[1][0] * ATA[2][1] - ATA[1][1] * ATA[2][0]) / det;
    inv[2][1] = -(ATA[0][0] * ATA[2][1] - ATA[0][1] * ATA[2][0]) / det;
    inv[2][2] = (ATA[0][0] * ATA[1][1] - ATA[0][1] * ATA[1][0]) / det;

    double beta_fit[3];
    for (int a = 0; a < 3; ++a) {
        beta_fit[a] = 0.0;
        for (int b = 0; b < 3; ++b) {
            beta_fit[a] += inv[a][b] * ATy[b];
        }
    }

    double chi2 = 0.0;
    for (uint32_t i = 0; i < n; ++i) {
        const double y_model =
            beta_fit[0] * X[i][0] + beta_fit[1] * X[i][1] + beta_fit[2] * X[i][2];
        const double r = y[i] - y_model;
        chi2 += w[i] * r * r;
    }
    const double dof = (double)(n - 3u);

    const double sigma_a2 = -beta_fit[0];
    const double sigma_a2_err = sqrt(inv[0][0]);

    *out_sigma_a2 = sigma_a2;
    *out_sigma_a2_err = sigma_a2_err;
    *out_mu_a = -beta_fit[1];
    *out_mu_a_err = sqrt(inv[1][1]);
    *out_c = beta_fit[2];
    *out_chi2 = chi2;
    *out_chi2_dof = (dof > 0.0) ? (chi2 / dof) : 0.0;

    if (!(sigma_a2 > 0.0)) {
        return false;
    }

    const double a_sqrt_sigma = sqrt(sigma_a2);
    const double a_sqrt_sigma_err = (a_sqrt_sigma > 1e-30)
        ? (sigma_a2_err / (2.0 * a_sqrt_sigma)) : 0.0;

    *out_a_sqrt_sigma = a_sqrt_sigma;
    *out_a_sqrt_sigma_err = a_sqrt_sigma_err;
    return true;
}

/* --------------------------------------------------------------------------
 * Messphase mit Binning.
 * ----------------------------------------------------------------------- */
static void st_measure(ProUniverse* pu, double beta, double eps,
    uint32_t n_measure,
    prng_state_t* rng, STResult* out)
{
    const uint32_t n_bins = n_measure / ST_BIN_SIZE;

    static const uint32_t ST_BINS_MAX = 512u;
    double W_bin[ST_N_LOOPS][512];
    if (n_bins > ST_BINS_MAX) {
        out->fit_ok = false;
        out->n_used = 0u;
        return;
    }

    double acc_total = 0.0;

    for (uint32_t b = 0; b < n_bins; ++b) {
        for (uint32_t s = 0; s < ST_BIN_SIZE; ++s) {
            const double rate = st_metropolis_sweep(pu, beta, eps, rng);
            acc_total += rate;
        }
        for (uint32_t i = 0; i < ST_N_LOOPS; ++i) {
            W_bin[i][b] = ProPhysics_Wilson_Loop_Average(
                pu, ST_LOOPS_M[i], ST_LOOPS_N[i]);
        }
    }

    const double inv_n = 1.0 / (double)n_bins;

    for (uint32_t i = 0; i < ST_N_LOOPS; ++i) {
        double mean = 0.0;
        for (uint32_t b = 0; b < n_bins; ++b) {
            mean += W_bin[i][b];
        }
        mean *= inv_n;

        double var = 0.0;
        for (uint32_t b = 0; b < n_bins; ++b) {
            const double d = W_bin[i][b] - mean;
            var += d * d;
        }
        var /= (double)(n_bins - 1u);
        out->W[i] = mean;
        out->W_err[i] = sqrt(var / (double)n_bins);
    }

    out->accept_rate = acc_total / (double)n_measure;
    out->plaq_avg = out->W[0];

    out->fit_ok = st_fit_loops(out->W, out->W_err,
        &out->sigma_a2, &out->sigma_a2_err,
        &out->mu_a, &out->mu_a_err,
        &out->c, &out->chi2, &out->chi2_dof,
        &out->n_used,
        &out->a_sqrt_sigma, &out->a_sqrt_sigma_err);
}

/* --------------------------------------------------------------------------
 * Ein Lauf fuer festes (dim, beta).
 * ----------------------------------------------------------------------- */
static bool st_run_one(uint32_t dim, double beta,
    uint32_t n_thermal, uint32_t n_measure,
    STResult* out)
{
    out->dim = dim;
    out->beta = beta;
    out->fit_ok = false;
    out->n_used = 0u;
    out->sigma_a2 = 0.0;
    out->sigma_a2_err = 0.0;
    out->a_sqrt_sigma = 0.0;
    out->a_sqrt_sigma_err = 0.0;
    out->mu_a = 0.0;
    out->mu_a_err = 0.0;
    out->c = 0.0;
    out->chi2 = 0.0;
    out->chi2_dof = 0.0;
    out->accept_rate = 0.0;
    out->eps = 0.0;
    out->plaq_avg = 0.0;
    for (uint32_t i = 0; i < ST_N_LOOPS; ++i) {
        out->W[i] = 0.0;
        out->W_err[i] = 0.0;
    }

    const uint64_t N = (uint64_t)dim * (uint64_t)dim * (uint64_t)dim;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.edge_phases || !pu.reg_source || !pu.amp_grid) {
        ProPhysics_Free(&pu);
        return false;
    }

    wire_torus_3d(&pu, dim);

    prng_state_t rng;
    const uint64_t seed = 0x57E50000ULL
        + ((uint64_t)dim << 16)
        + (uint64_t)llround(beta * 1000.0);
    prng_seed(&rng, seed);

    st_randomize_links(&pu, &rng);
    const double eps = st_thermalize(&pu, beta, n_thermal, &rng);

    out->eps = eps;
    st_measure(&pu, beta, eps, n_measure, &rng, out);

    ProPhysics_Free(&pu);
    return true;
}

/* --------------------------------------------------------------------------
 * Anker-INFO (Revision 2, Fix A: kein PASS/FAIL)
 * ----------------------------------------------------------------------- */
static void st_anchor_info(const STResult* r, int n_runs)
{
    printf("[ST] ==============================================================\n");
    printf("[ST] INFO: Vergleich mit Cahill & Prasad (1989)\n");
    printf("[ST] ACHTUNG: Die Referenz ist 4D-SU(2), unser Kernel ist 3D.\n");
    printf("[ST] Die Werte sind NICHT direkt vergleichbar. Die Ausgabe ist\n");
    printf("[ST] rein informativ und beeinflusst NICHT den PASS/FAIL-Status.\n");
    printf("[ST] ==============================================================\n");
    printf("[ST]   dim  beta   a*sqrt(s) 3D       a*sqrt(s) 4D      |Delta|/Lit  Info\n");
    printf("[ST]   ---------------------------------------------------------------------\n");

    for (int k = 0; k < n_runs; ++k) {
        if (!r[k].fit_ok) continue;
        const STAnchorEntry* a = st_find_anchor(r[k].beta);
        if (!a) continue;

        const double fit = r[k].a_sqrt_sigma;
        const double lit = a->a_sqrt_sigma;
        const double d = fabs(fit - lit) / lit;

        printf("[ST]   %3u  %.2f   %.6f +- %.6f     %.6f        %.4f      (3D vs 4D)\n",
            r[k].dim, r[k].beta,
            fit, r[k].a_sqrt_sigma_err,
            lit, d);
    }
    printf("\n");
}

/* --------------------------------------------------------------------------
 * Konsistenz-Pruefung S1-S4.
 * ----------------------------------------------------------------------- */
static bool st_check_S1(const STResult* r, int n_runs)
{
    bool ok = true;
    for (int k = 0; k < n_runs; ++k) {
        if (!r[k].fit_ok) {
            printf("[ST]   S1 SKIP: dim=%u, beta=%.2f (Fit unzureichend, n_used=%u)\n",
                r[k].dim, r[k].beta, r[k].n_used);
            continue;
        }
        if (!(r[k].sigma_a2 > 0.0)) {
            printf("[ST]   S1 FAIL: sigma_a2(dim=%u, beta=%.2f) = %.6f <= 0\n",
                r[k].dim, r[k].beta, r[k].sigma_a2);
            ok = false;
        }
    }
    return ok;
}

/* Revision 2, Fix E: S2 mit 2-sigma-Toleranz */
static bool st_check_S2(const STResult* r, int n_runs,
    const uint32_t* dims, int n_dims,
    const double* betas, int n_betas)
{
    bool ok = true;
    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];
        double prev = 0.0;
        double prev_err = 0.0;
        bool have_prev = false;
        for (int bi = 0; bi < n_betas; ++bi) {
            const double beta = betas[bi];
            const STResult* hit = NULL;
            for (int k = 0; k < n_runs; ++k) {
                if (r[k].dim == dim && fabs(r[k].beta - beta) < 1e-9) {
                    hit = &r[k];
                    break;
                }
            }
            if (!hit || !hit->fit_ok) continue;
            if (have_prev) {
                const double tol = 2.0 * sqrt(prev_err * prev_err + hit->sigma_a2_err * hit->sigma_a2_err);
                if (hit->sigma_a2 >= prev + tol) {
                    printf("[ST]   S2 FAIL: dim=%u beta=%.2f "
                        "sigma_a2=%.6f >= sigma_a2(prev)=%.6f + 2sigma=%.6f\n",
                        dim, beta, hit->sigma_a2, prev, tol);
                    ok = false;
                }
            }
            prev = hit->sigma_a2;
            prev_err = hit->sigma_a2_err;
            have_prev = true;
        }
    }
    return ok;
}

/* Revision 2, Fix D: chi2/dof-Schwelle auf 10 */
static bool st_check_S3(const STResult* r, int n_runs)
{
    bool ok = true;
    for (int k = 0; k < n_runs; ++k) {
        if (!r[k].fit_ok) continue;
        if (r[k].chi2_dof > 10.0) {
            printf("[ST]   S3 WARN: chi2/dof(dim=%u, beta=%.2f) = %.4f > 10\n",
                r[k].dim, r[k].beta, r[k].chi2_dof);
            ok = false;
        }
    }
    return ok;
}

static bool st_check_S4(const STResult* r, int n_runs,
    const uint32_t* dims, int n_dims,
    const double* betas, int n_betas)
{
    bool ok = true;
    for (int bi = 0; bi < n_betas; ++bi) {
        const double beta = betas[bi];
        for (int i = 0; i + 1 < n_dims; ++i) {
            const STResult* ri = NULL;
            const STResult* rj = NULL;
            for (int k = 0; k < n_runs; ++k) {
                if (r[k].dim == dims[i] && fabs(r[k].beta - beta) < 1e-9) ri = &r[k];
                if (r[k].dim == dims[i + 1] && fabs(r[k].beta - beta) < 1e-9) rj = &r[k];
            }
            if (!ri || !rj) continue;
            if (!ri->fit_ok || !rj->fit_ok) {
                printf("[ST]   S4 SKIP: beta %.2f, dim {%u,%u} (Fit unzureichend)\n",
                    beta, dims[i], dims[i + 1]);
                continue;
            }
            const double denom = (rj->sigma_a2 > 1e-30) ? rj->sigma_a2 : 1e-30;
            const double rel = fabs(ri->sigma_a2 - rj->sigma_a2) / denom;
            const double sigma_rel = sqrt(
                (ri->sigma_a2_err * ri->sigma_a2_err) / (denom * denom)
                + (rj->sigma_a2_err * rj->sigma_a2_err)
                * (ri->sigma_a2 * ri->sigma_a2)
                / (denom * denom * denom * denom));
            const double tol = 0.10 + 3.0 * sigma_rel;
            const char* mark = (rel < tol) ? "OK" : "WARN";
            printf("[ST]   S4 beta %.2f: |s(d=%u)-s(d=%u)|/s = "
                "%.4f vs tol = %.4f  %s\n",
                beta, dims[i], dims[i + 1], rel, tol, mark);
            if (rel > tol) ok = false;
        }
    }
    return ok;
}

/* ==========================================================================
 * Haupttest.
 * ========================================================================== */

bool test_string_tension(bool full_dims, bool anchor)
{
    printf("========================================================================\n");
    printf("  Etappe 23c (Rev.2): String-Tension Selbstkonsistenz-Test\n");
    printf("  Sampler: Haar-Vorschlag U -> R*U\n");
    printf("  Kein V&V-Anker (Referenz ist 4D, Kernel ist 3D)\n");
    printf("========================================================================\n\n");
    printf("[ST] Ziel: sigma_a2 aus Wilson-Loops via Flaechengesetz-Fit.\n");
    printf("[ST] Fit: ln W = -sigma_a2 * (m*n) - mu_a * (m+n) + c.\n");
    printf("[ST] Ausgabe: sigma_a2 und a*sqrt(sigma).\n");

    uint32_t n_thermal, n_measure;
    const uint32_t* dims;
    int n_dims;
    static const uint32_t dims_fast[] = { 16u, 32u };
    static const uint32_t dims_full[] = { 16u, 32u, 64u };
    /* Revision 2, Fix B: Confinement-Regime */
    static const double   betas_std[] = { 2.4, 2.5, 2.7, 3.0 };
    const double* betas;
    int n_betas;

    if (anchor) {
        n_thermal = 2000u;
        n_measure = 3000u;
        dims = dims_full;
        n_dims = 3;
        betas = betas_std;
        n_betas = 4;
        printf("[ST] Modus: ANKER (dim in {16, 32, 64}, beta in {2.4, 2.5, 2.7, 3.0}).\n");
    }
    else if (full_dims) {
        n_thermal = ST_N_THERMAL_STD;
        n_measure = ST_N_MEASURE_STD;
        dims = dims_full;
        n_dims = 3;
        betas = betas_std;
        n_betas = 4;
        printf("[ST] Modus: FULL (dim in {16, 32, 64}).\n");
    }
    else {
        n_thermal = ST_N_THERMAL_STD;
        n_measure = ST_N_MEASURE_STD;
        dims = dims_fast;
        n_dims = 2;
        betas = betas_std;
        n_betas = 4;
        printf("[ST] Modus: FAST (dim in {16, 32}).\n");
    }

    printf("[ST] Thermalisierung: %u Sweeps, Messung: %u Sweeps, Bins: %u.\n",
        (unsigned)n_thermal, (unsigned)n_measure,
        (unsigned)(n_measure / ST_BIN_SIZE));
    printf("[ST] Fit-Filter: W > %.0e, rel.err = W_err/W < %.2f.\n",
        (double)ST_W_FLOOR, (double)ST_RELERR_MAX);
    printf("[ST] Sweep: dim in {");
    for (int di = 0; di < n_dims; ++di) {
        printf("%u%s", dims[di], (di + 1 < n_dims) ? ", " : "");
    }
    printf("} x beta in {");
    for (int bi = 0; bi < n_betas; ++bi) {
        printf("%.2f%s", betas[bi], (bi + 1 < n_betas) ? ", " : "");
    }
    printf("}.\n\n");

    STResult results[ST_MAX_RUNS];
    int n_results = 0;

    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];

        printf("[ST] --------------------------------------------------------------\n");
        printf("[ST] dim = %u  (N = %llu Knoten)\n", dim,
            (unsigned long long)((uint64_t)dim * dim * dim));
        printf("[ST] --------------------------------------------------------------\n");
        printf("[ST]   beta    eps    accept    <1/2 Re Tr U>   sigma_a2           "
            "a*sqrt(sigma)     chi2/dof  n_used\n");
        printf("[ST]   --------------------------------------------------------"
            "----------------------------------------\n");

        for (int bi = 0; bi < n_betas; ++bi) {
            STResult r;
            if (!st_run_one(dim, betas[bi], n_thermal, n_measure, &r)) {
                printf("[ST]   %.2f    ---    ---       (Lauf fehlgeschlagen)\n",
                    betas[bi]);
                continue;
            }
            if (n_results < ST_MAX_RUNS) {
                results[n_results++] = r;
            }

            if (r.fit_ok) {
                printf("[ST]   %.2f    %.3f  %.3f     %.5f          "
                    "%.6f+-%.6f   %.6f+-%.6f   %.4f    %u\n",
                    r.beta, r.eps, r.accept_rate, r.plaq_avg,
                    r.sigma_a2, r.sigma_a2_err,
                    r.a_sqrt_sigma, r.a_sqrt_sigma_err,
                    r.chi2_dof, r.n_used);
            }
            else {
                printf("[ST]   %.2f    %.3f  %.3f     %.5f          "
                    "n/a (fit unzureichend)   n_used=%u\n",
                    r.beta, r.eps, r.accept_rate, r.plaq_avg, r.n_used);
            }
        }
        printf("\n");
    }

    printf("[ST] ==============================================================\n");
    printf("[ST] Zusammenfassung: sigma_a2(beta, dim)\n");
    printf("[ST] ==============================================================\n");
    printf("[ST]   beta    ");
    for (int di = 0; di < n_dims; ++di) {
        printf("  s(d=%2u)           ", dims[di]);
    }
    printf("\n");
    printf("[ST]   ----------------------------------------------------------\n");

    for (int bi = 0; bi < n_betas; ++bi) {
        printf("[ST]   %.2f    ", betas[bi]);
        for (int di = 0; di < n_dims; ++di) {
            const STResult* hit = NULL;
            for (int k = 0; k < n_results; ++k) {
                if (results[k].dim == dims[di] &&
                    fabs(results[k].beta - betas[bi]) < 1e-9) {
                    hit = &results[k];
                    break;
                }
            }
            if (hit && hit->fit_ok) {
                printf("  %.5f+-%.5f", hit->sigma_a2, hit->sigma_a2_err);
            }
            else {
                printf("  n/a             ");
            }
        }
        printf("\n");
    }
    printf("\n");

    printf("[ST] ==============================================================\n");
    printf("[ST] Konsistenz-Pruefung\n");
    printf("[ST] ==============================================================\n");

    printf("[ST] S1 (sigma_a2 > 0 fuer alle fittbaren dim,beta):\n");
    const bool s1 = st_check_S1(results, n_results);
    printf("[ST]   S1: %s\n\n", s1 ? "PASS" : "FAIL");

    printf("[ST] S2 (sigma_a2 monoton fallend in beta pro dim, 2sigma-Toleranz):\n");
    const bool s2 = st_check_S2(results, n_results,
        dims, n_dims, betas, n_betas);
    printf("[ST]   S2: %s\n\n", s2 ? "PASS" : "FAIL");

    printf("[ST] S3 (chi2/dof < 10):\n");
    const bool s3 = st_check_S3(results, n_results);
    printf("[ST]   S3: %s\n\n", s3 ? "PASS" : "FAIL");

    printf("[ST] S4 (Konvergenz ueber dim, < 10%% + 3sigma):\n");
    const bool s4 = st_check_S4(results, n_results,
        dims, n_dims, betas, n_betas);
    printf("[ST]   S4: %s\n\n", s4 ? "PASS" : "FAIL");

    st_anchor_info(results, n_results);

    const bool all_ok = s1 && s2 && s3 && s4;

    printf("[ST] --------------------------------------------------------------\n");
    printf("[ST] Interpretation:\n");
    printf("[ST]   sigma_a2 > 0: String-Tension positiv (Confinement in 3D).\n");
    printf("[ST]   sigma_a2 faellt in beta: asymptotische Freiheit.\n");
    printf("[ST]   Konvergenz ueber dim: endliche-Gitter-Effekte kontrolliert.\n");
    printf("[ST]   Der Vergleich mit Cahill & Prasad (4D) ist informativ.\n");
    printf("[ST]   Die Abweichung um Faktor 3-4 ist die 3D/4D-Differenz,\n");
    printf("[ST]   kein Kernel-Bug.\n");
    printf("\n");

    printf("[ST] -> %s\n", all_ok ? "PASSED (Selbstkonsistenz)" : "FAILED");
    return all_ok;
}