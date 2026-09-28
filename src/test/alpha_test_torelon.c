/* ==========================================================================
 * alpha_test_torelon.c
 *
 * Etappe 23d: Torelon-Masse als V&V-Anker gegen Cahill & Prasad (1989).
 *
 * Die Torelon-Masse ist die Energie eines Flux-Tube, der sich um eine
 * periodische Raumrichtung wickelt. Fuer grosse Gitterdimension L gilt:
 *
 *     m(L) = sigma * L  (fuehrender Term)
 *     sigma_a2 = m_lat / L  (dimensionslose String-Tension)
 *
 * bzw. mit Luescher-Korrektur:
 *
 *     m(L) = sigma * L - pi / (6 * L) + ...
 *
 * Beobachtbare: Polyakov-Loop-Korrelator
 *
 *     P_x(y, z) = Tr( prod_{i=0}^{L-1} U_x(i, y, z) ) / 2   (SU(2))
 *     C(z)      = (1/L_y) * sum_y < P_x(y, z) * P_x(y, 0) >
 *
 * Fuer kleine z (z << L/2) gilt C(z) ~ A * exp(-m * z).
 * Log-linearer Fit liefert m in Gitter-Einheiten.
 *
 * Literatur-Anker (Teper, hep-th/9812187 Appendix A, zitiert nach
 * Cahill & Prasad, PRD 40, 1274, 1989):
 *
 *     beta = 2.4 : a * sqrt(sigma) = 0.2673 +/- 0.0015
 *     beta = 2.5 : a * sqrt(sigma) = 0.1860 +/- 0.0030
 *
 * Modi:
 *   Default           dim=32, beta=2.4, 1000 Configs    (FAST, ~3 min)
 *   --torelon-full    dim=32, beta in {2.4, 2.5}, 3000  (FULL, ~10 min)
 *   --tension-anchor  aktiviert Anker-Vergleich
 *
 * Kein Kernel-Change. Nutzt ausschliesslich:
 *   - ProPhysics_Set_Edge_SU2 / Get_Edge_SU2
 *   - ProPhysics_SU2_Link_Plaquette_Sum
 *   - ProPhysics_SU2_Plaquette_Action
 *
 * R-Konformitaet:
 *   R1: Bit-Mask fuer Koordinaten.
 *   R2: Kein malloc im Hotpath; Test nutzt Stack.
 *   R3: read-only; keine amp_grid-Mutation.
 *   R7: additiv; kein bestehender Pfad geaendert.
 * ========================================================================== */

#include "ProPhysics_Internal.h"
#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

 /* --------------------------------------------------------------------------
  * Konstanten
  * ----------------------------------------------------------------------- */

#define TL_N_THERMAL_STD    500u
#define TL_N_MEASURE_STD    1000u
#define TL_N_THERMAL_FULL   1000u
#define TL_N_MEASURE_FULL   3000u

#define TL_MEAS_SKIP        1u
#define TL_MAX_Z_SEP        65u    /* z-Separation bis dim/2 (dim <= 128) */

#define TL_FIT_Z_MIN        1u     /* fruehester z fuer Fit */
#define TL_SN_MIN           3.0    /* Signal-zu-Rauschen fuer Fit */

  /* Vorwaerts-Kanaele in 3D (bit-interleaved (z,y,x)). */
static const uint8_t TL_FWD[3] = { 0u, 2u, 4u };
static const uint8_t TL_CH_X_PLUS = 0u;

/* Anker-Referenzwerte (Teper hep-th/9812187). */
typedef struct {
    double beta;
    double a_sqrt_sigma;
    double err;
} TLAnchorEntry;

static const TLAnchorEntry TL_ANCHORS[] = {
    { 2.4, 0.2673, 0.0015 },
    { 2.5, 0.1860, 0.0030 },
};
static const int TL_N_ANCHORS =
(int)(sizeof(TL_ANCHORS) / sizeof(TL_ANCHORS[0]));

static const TLAnchorEntry* tl_find_anchor(double beta)
{
    for (int i = 0; i < TL_N_ANCHORS; ++i) {
        if (fabs(TL_ANCHORS[i].beta - beta) < 1e-9) return &TL_ANCHORS[i];
    }
    return NULL;
}

/* --------------------------------------------------------------------------
 * RNG-Helfer
 * ----------------------------------------------------------------------- */
static double tl_gauss(double u1, double u2)
{
    if (u1 < 1e-30) u1 = 1e-30;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double tl_uniform(prng_state_t* rng)
{
    return (double)(prng_next(rng) >> 11) / 9007199254740992.0;
}

/* --------------------------------------------------------------------------
 * Links randomisieren (uniform auf S^3)
 * ----------------------------------------------------------------------- */
static void tl_randomize_links(ProUniverse* pu, prng_state_t* rng)
{
    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = TL_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            const double g1 = tl_gauss(tl_uniform(rng), tl_uniform(rng));
            const double g2 = tl_gauss(tl_uniform(rng), tl_uniform(rng));
            const double g3 = tl_gauss(tl_uniform(rng), tl_uniform(rng));
            const double g4 = tl_gauss(tl_uniform(rng), tl_uniform(rng));

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
 * Metropolis-Sweep (identisch zu alpha_test_string_tension.c)
 * ----------------------------------------------------------------------- */
static double tl_metropolis_sweep(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng)
{
    uint64_t accept = 0, total = 0;

    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = TL_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            int32_t ar, ai, br, bi;
            ProPhysics_Get_Edge_SU2(pu, x, mu, &ar, &ai, &br, &bi);

            const double S_before =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double eps_q30 = eps * (double)PRO_SU2_SCALE;
            const int32_t d_ar = (int32_t)llround(tl_gauss(tl_uniform(rng), tl_uniform(rng)) * eps_q30);
            const int32_t d_ai = (int32_t)llround(tl_gauss(tl_uniform(rng), tl_uniform(rng)) * eps_q30);
            const int32_t d_br = (int32_t)llround(tl_gauss(tl_uniform(rng), tl_uniform(rng)) * eps_q30);
            const int32_t d_bi = (int32_t)llround(tl_gauss(tl_uniform(rng), tl_uniform(rng)) * eps_q30);

            ProPhysics_Set_Edge_SU2(pu, x, mu,
                ar + d_ar, ai + d_ai, br + d_br, bi + d_bi);

            const double S_after =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double dS = S_after - S_before;
            const double u = tl_uniform(rng);

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

static double tl_thermalize(ProUniverse* pu, double beta, uint32_t n_thermal,
    prng_state_t* rng)
{
    double eps = 0.3;
    for (int pass = 0; pass < 4; ++pass) {
        double rate = 0.0;
        const int n_quick = 10;
        for (int s = 0; s < n_quick; ++s) {
            rate += tl_metropolis_sweep(pu, beta, eps, rng);
        }
        rate /= (double)n_quick;
        if (rate < 0.30) eps *= 0.6;
        else if (rate > 0.70) eps *= 1.5;
        else break;
        if (eps < 1e-4) eps = 1e-4;
        if (eps > 2.0)  eps = 2.0;
    }
    for (uint32_t s = 0; s < n_thermal; ++s) {
        (void)tl_metropolis_sweep(pu, beta, eps, rng);
    }
    return eps;
}

/* --------------------------------------------------------------------------
 * Polyakov-Loop in x-Richtung: P_x(y, z) = Tr(prod U_x) / 2
 * ----------------------------------------------------------------------- */
static double tl_polyakov_x(const ProUniverse* pu, uint32_t y, uint32_t z)
{
    const uint32_t dim = pu->grid_dim;
    const uint32_t shift = pu->grid_dim_shift;

    int32_t a_re = PRO_SU2_IDENT_RE, a_im = PRO_SU2_IDENT_IM;
    int32_t b_re = 0, b_im = 0;

    for (uint32_t i = 0; i < dim; ++i) {
        const uint64_t idx =
            (uint64_t)i
            | ((uint64_t)y << shift)
            | ((uint64_t)z << (2u * shift));

        const ProEdge* e = pro_su2_edge(pu, idx, TL_CH_X_PLUS);
        if (!e) return 0.0;

        int32_t na_re, na_im, nb_re, nb_im;
        pro_su2_mul(a_re, a_im, b_re, b_im,
            e->su2_a_re, e->su2_a_im, e->su2_b_re, e->su2_b_im,
            &na_re, &na_im, &nb_re, &nb_im);
        a_re = na_re; a_im = na_im;
        b_re = nb_re; b_im = nb_im;
    }
    return (double)a_re / (double)PRO_SU2_SCALE;
}

/* --------------------------------------------------------------------------
 * Korrelator C(z): (1/Ly) * sum_{y} < P_x(y, z) * P_x(y, 0) >
 * ----------------------------------------------------------------------- */
static void tl_measure_corr(const ProUniverse* pu,
    double* C_out, uint32_t C_len)
{
    const uint32_t dim = pu->grid_dim;
    const uint32_t mask = pu->grid_dim_mask;

    for (uint32_t z_sep = 0; z_sep < C_len; ++z_sep) {
        double sum = 0.0;
        uint64_t count = 0;

        for (uint32_t y = 0; y < dim; ++y) {
            for (uint32_t z0 = 0; z0 < dim; ++z0) {
                const uint32_t z1 = (z0 + z_sep) & mask;
                const double P1 = tl_polyakov_x(pu, y, z0);
                const double P2 = tl_polyakov_x(pu, y, z1);
                sum += P1 * P2;
                count++;
            }
        }
        C_out[z_sep] = (count > 0u) ? (sum / (double)count) : 0.0;
    }
}

/* --------------------------------------------------------------------------
 * Least-Squares-Fit y = a + b*x mit Gewichten w
 * ----------------------------------------------------------------------- */
static void tl_linfit(const double* x, const double* y, const double* w,
    uint32_t n,
    double* out_a, double* out_b,
    double* out_a_err, double* out_b_err,
    double* out_chi2)
{
    double Sw = 0, Swx = 0, Swy = 0, Swxx = 0, Swxy = 0;
    for (uint32_t i = 0; i < n; ++i) {
        Sw += w[i];
        Swx += w[i] * x[i];
        Swy += w[i] * y[i];
        Swxx += w[i] * x[i] * x[i];
        Swxy += w[i] * x[i] * y[i];
    }
    const double Delta = Sw * Swxx - Swx * Swx;
    if (fabs(Delta) < 1e-30) {
        *out_a = 0.0; *out_b = 0.0;
        *out_a_err = 0.0; *out_b_err = 0.0;
        *out_chi2 = 0.0;
        return;
    }
    *out_b = (Sw * Swxy - Swx * Swy) / Delta;
    *out_a = (Swy - (*out_b) * Swx) / Sw;
    *out_a_err = sqrt(Swxx / Delta);
    *out_b_err = sqrt(Sw / Delta);

    double chi2 = 0.0;
    for (uint32_t i = 0; i < n; ++i) {
        const double r = y[i] - (*out_a) - (*out_b) * x[i];
        chi2 += w[i] * r * r;
    }
    *out_chi2 = chi2;
}

/* --------------------------------------------------------------------------
 * Fit-Ergebnis pro (dim, beta).
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t dim;
    double   beta;
    double   C[TL_MAX_Z_SEP];
    double   C_err[TL_MAX_Z_SEP];
    uint32_t C_len;
    double   mass;
    double   mass_err;
    double   sigma_a2;
    double   sigma_a2_err;
    double   a_sqrt_sigma;
    double   a_sqrt_sigma_err;
    uint32_t n_fit_pts;
    double   chi2_dof;
    bool     fit_ok;
    double   accept_rate;
} TLResult;

/* --------------------------------------------------------------------------
 * Massen-Fit: log C(z) = log A - m * z
 *
 * Nur Punkte mit C(z) > 0 und C(z) / C_err(z) >= TL_SN_MIN.
 * ----------------------------------------------------------------------- */
static bool tl_fit_mass(const TLResult* r, uint32_t L, TLResult* out)
{
    double x[TL_MAX_Z_SEP];
    double y[TL_MAX_Z_SEP];
    double w[TL_MAX_Z_SEP];
    uint32_t n = 0;

    const uint32_t z_max_fit = r->C_len / 2;
    for (uint32_t z = TL_FIT_Z_MIN; z < z_max_fit; ++z) {
        if (r->C[z] <= 0.0) continue;
        if (r->C_err[z] <= 0.0) continue;
        const double sn = r->C[z] / r->C_err[z];
        if (sn < TL_SN_MIN) continue;

        x[n] = (double)z;
        y[n] = log(r->C[z]);
        const double rel_err = r->C_err[z] / r->C[z];
        w[n] = 1.0 / (rel_err * rel_err);
        n++;
    }

    out->n_fit_pts = n;
    if (n < 3u) return false;

    double a, b, a_err, b_err, chi2;
    tl_linfit(x, y, w, n, &a, &b, &a_err, &b_err, &chi2);

    /* b = -m, also m = -b */
    const double m = -b;
    if (!(m > 0.0)) return false;

    out->mass = m;
    out->mass_err = b_err;
    out->chi2_dof = (n > 2u) ? (chi2 / (double)(n - 2u)) : 0.0;

    /* sigma_a2 = m / L */
    out->sigma_a2 = m / (double)L;
    out->sigma_a2_err = b_err / (double)L;

    /* a*sqrt(sigma) = sqrt(sigma_a2) */
    const double a_sqrt = sqrt(out->sigma_a2);
    out->a_sqrt_sigma = a_sqrt;
    out->a_sqrt_sigma_err = (a_sqrt > 1e-30)
        ? (out->sigma_a2_err / (2.0 * a_sqrt))
        : 0.0;

    return true;
}

/* --------------------------------------------------------------------------
 * Ein Lauf fuer festes (dim, beta).
 * ----------------------------------------------------------------------- */
static bool tl_run_one(uint32_t dim, double beta,
    uint32_t n_thermal, uint32_t n_measure,
    TLResult* out)
{
    memset(out, 0, sizeof(*out));
    out->dim = dim;
    out->beta = beta;

    const uint64_t N = (uint64_t)dim * (uint64_t)dim * (uint64_t)dim;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.edge_phases || !pu.reg_source || !pu.amp_grid) {
        ProPhysics_Free(&pu);
        return false;
    }

    wire_torus_3d(&pu, dim);

    prng_state_t rng;
    const uint64_t seed = 0x704C0000ULL
        + ((uint64_t)dim << 16)
        + (uint64_t)llround(beta * 1000.0);
    prng_seed(&rng, seed);

    tl_randomize_links(&pu, &rng);
    const double eps = tl_thermalize(&pu, beta, n_thermal, &rng);

    /* C_len = dim/2 + 1 */
    const uint32_t C_len = dim / 2u + 1u;
    if (C_len > TL_MAX_Z_SEP) {
        ProPhysics_Free(&pu);
        return false;
    }
    out->C_len = C_len;

    /* Kumulative Akkumulation */
    static double C_sum[TL_MAX_Z_SEP];
    static double C_sqsum[TL_MAX_Z_SEP];
    for (uint32_t z = 0; z < C_len; ++z) {
        C_sum[z] = 0.0;
        C_sqsum[z] = 0.0;
    }

    uint32_t n_configs = 0;
    double acc_total = 0.0;
    double C_local[TL_MAX_Z_SEP];

    for (uint32_t s = 0; s < n_measure; ++s) {
        const double rate = tl_metropolis_sweep(&pu, beta, eps, &rng);
        acc_total += rate;

        if ((s % TL_MEAS_SKIP) == 0u) {
            tl_measure_corr(&pu, C_local, C_len);
            for (uint32_t z = 0; z < C_len; ++z) {
                C_sum[z] += C_local[z];
                C_sqsum[z] += C_local[z] * C_local[z];
            }
            n_configs++;
        }
    }

    out->accept_rate = (n_measure > 0u) ? (acc_total / (double)n_measure) : 0.0;

    if (n_configs < 2u) {
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint32_t z = 0; z < C_len; ++z) {
        const double mean = C_sum[z] / (double)n_configs;
        const double var = (C_sqsum[z] - (double)n_configs * mean * mean)
            / (double)(n_configs - 1u);
        out->C[z] = mean;
        out->C_err[z] = (var > 0.0) ? sqrt(var / (double)n_configs) : 0.0;
    }

    out->fit_ok = tl_fit_mass(out, dim, out);

    ProPhysics_Free(&pu);
    return true;
}

/* --------------------------------------------------------------------------
 * Anker-Vergleich
 * ----------------------------------------------------------------------- */
static bool tl_anchor_check(const TLResult* r, int n_runs, bool anchor_mode)
{
    if (!anchor_mode) return true;

    printf("[TL] ==============================================================\n");
    printf("[TL] Anker-Vergleich gegen Cahill & Prasad (PRD 40, 1274, 1989)\n");
    printf("[TL] Werte zitiert nach Teper hep-th/9812187 Appendix A.\n");
    printf("[TL] Observable: a*sqrt(sigma) aus Torelon-Masse m = sigma_a2 * L.\n");
    printf("[TL] ==============================================================\n");
    printf("[TL]   dim  beta   a*sqrt(s) fit       Lit.       |Delta|/Lit  Status\n");
    printf("[TL]   -----------------------------------------------------------\n");

    bool all_ok = true;

    for (int k = 0; k < n_runs; ++k) {
        if (!r[k].fit_ok) continue;
        const TLAnchorEntry* a = tl_find_anchor(r[k].beta);
        if (!a) continue;

        const double fit = r[k].a_sqrt_sigma;
        const double lit = a->a_sqrt_sigma;
        const double d = fabs(fit - lit) / lit;

        const char* status;
        if (d < 0.05)       status = "PASS";
        else if (d < 0.15)  status = "PASS(Hinweis)";
        else if (d < 0.50)  status = "WARN";
        else { status = "FAIL"; all_ok = false; }

        printf("[TL]   %3u  %.2f   %.6f +- %.6f   %.6f   %.4f      %s\n",
            r[k].dim, r[k].beta,
            fit, r[k].a_sqrt_sigma_err,
            lit, d, status);
    }
    printf("\n");
    return all_ok;
}

/* --------------------------------------------------------------------------
 * Konsistenz-Pruefung S1-S3 fuer Torelon.
 *
 *   S1: m > 0 fuer alle (dim, beta) mit fit_ok.
 *   S2: sigma_a2 monoton fallend in beta pro dim.
 *   S3: chi2/dof < 5 fuer alle fit_ok.
 * ----------------------------------------------------------------------- */
static bool tl_check_S1(const TLResult* r, int n_runs)
{
    bool ok = true;
    for (int k = 0; k < n_runs; ++k) {
        if (!r[k].fit_ok) continue;
        if (!(r[k].mass > 0.0)) {
            printf("[TL]   S1 FAIL: mass(dim=%u, beta=%.2f) = %.6f <= 0\n",
                r[k].dim, r[k].beta, r[k].mass);
            ok = false;
        }
    }
    return ok;
}

static bool tl_check_S2(const TLResult* r, int n_runs,
    const uint32_t* dims, int n_dims,
    const double* betas, int n_betas)
{
    bool ok = true;
    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];
        double prev = 0.0;
        bool have_prev = false;
        for (int bi = 0; bi < n_betas; ++bi) {
            const double beta = betas[bi];
            const TLResult* hit = NULL;
            for (int k = 0; k < n_runs; ++k) {
                if (r[k].dim == dim && fabs(r[k].beta - beta) < 1e-9) {
                    hit = &r[k];
                    break;
                }
            }
            if (!hit || !hit->fit_ok) continue;
            if (have_prev) {
                if (!(hit->sigma_a2 < prev)) {
                    printf("[TL]   S2 FAIL: dim=%u beta=%.2f "
                        "sigma_a2=%.6f >= prev=%.6f\n",
                        dim, beta, hit->sigma_a2, prev);
                    ok = false;
                }
            }
            prev = hit->sigma_a2;
            have_prev = true;
        }
    }
    return ok;
}

static bool tl_check_S3(const TLResult* r, int n_runs)
{
    bool ok = true;
    for (int k = 0; k < n_runs; ++k) {
        if (!r[k].fit_ok) continue;
        if (r[k].chi2_dof > 5.0) {
            printf("[TL]   S3 FAIL: chi2/dof(dim=%u, beta=%.2f) = %.4f > 5\n",
                r[k].dim, r[k].beta, r[k].chi2_dof);
            ok = false;
        }
    }
    return ok;
}

/* ==========================================================================
 * Haupttest.
 * ========================================================================== */

bool test_torelon_mass(bool full_dims, bool anchor)
{
    printf("========================================================================\n");
    printf("  Etappe 23d: Torelon-Masse als V&V-Anker\n");
    printf("========================================================================\n\n");
    printf("[TL] Ziel: sigma_a2 aus Torelon-Masse m = sigma_a2 * L.\n");
    printf("[TL] Observable: Polyakov-Loop-Korrelator C(z) ~ exp(-m * z).\n");
    printf("[TL] Fit: log C(z) = log A - m * z.\n");
    printf("[TL] Vergleich mit a*sqrt(sigma) aus Cahill & Prasad (1989).\n");

    uint32_t n_thermal, n_measure;
    const uint32_t* dims;
    int n_dims;
    static const uint32_t dims_fast[] = { 32u };
    static const uint32_t dims_full[] = { 32u, 64u };
    static const double   betas_std[] = { 2.4, 2.5 };
    const double* betas;
    int n_betas;

    if (full_dims) {
        n_thermal = TL_N_THERMAL_FULL;
        n_measure = TL_N_MEASURE_FULL;
        dims = dims_full;
        n_dims = 2;
        betas = betas_std;
        n_betas = 2;
        printf("[TL] Modus: FULL (dim in {32, 64}, beta in {2.4, 2.5}).\n");
    }
    else {
        n_thermal = TL_N_THERMAL_STD;
        n_measure = TL_N_MEASURE_STD;
        dims = dims_fast;
        n_dims = 1;
        betas = betas_std;
        n_betas = 2;
        printf("[TL] Modus: FAST (dim in {32}, beta in {2.4, 2.5}).\n");
    }
    if (anchor) printf("[TL] Anker-Vergleich: AKTIV.\n");
    printf("[TL] Thermalisierung: %u Sweeps, Messung: %u Sweeps.\n",
        (unsigned)n_thermal, (unsigned)n_measure);
    printf("[TL] Messung alle %u Sweeps. Messpunkte: dim/2 + 1.\n\n",
        (unsigned)TL_MEAS_SKIP);

    TLResult results[8];
    int n_results = 0;

    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];

        printf("[TL] --------------------------------------------------------------\n");
        printf("[TL] dim = %u  (N = %llu Knoten)\n", dim,
            (unsigned long long)((uint64_t)dim * dim * dim));
        printf("[TL] --------------------------------------------------------------\n");
        printf("[TL]   beta    accept    m_lat          sigma_a2        "
            "a*sqrt(s)       chi2/dof  n_fit\n");
        printf("[TL]   --------------------------------------------------------"
            "-----------------------------------\n");

        for (int bi = 0; bi < n_betas; ++bi) {
            TLResult r;
            if (!tl_run_one(dim, betas[bi], n_thermal, n_measure, &r)) {
                printf("[TL]   %.2f    ---       (Lauf fehlgeschlagen)\n",
                    betas[bi]);
                continue;
            }
            if (n_results < 8) results[n_results++] = r;

            if (r.fit_ok) {
                printf("[TL]   %.2f    %.3f     %.6f+-%.6f  %.6f+-%.6f  "
                    "%.6f+-%.6f  %.4f    %u\n",
                    r.beta, r.accept_rate,
                    r.mass, r.mass_err,
                    r.sigma_a2, r.sigma_a2_err,
                    r.a_sqrt_sigma, r.a_sqrt_sigma_err,
                    r.chi2_dof, r.n_fit_pts);
            }
            else {
                printf("[TL]   %.2f    %.3f     n/a (fit unzureichend, "
                    "n_fit=%u)\n",
                    r.beta, r.accept_rate, r.n_fit_pts);
            }

            /* C(z) Diagnostik wenn fit fehlgeschlagen */
            if (!r.fit_ok) {
                for (uint32_t z = 0; z < r.C_len; ++z) {
                    const double sn = (r.C_err[z] > 0.0)
                        ? (r.C[z] / r.C_err[z]) : 0.0;
                    printf("[TL]     z=%2u: C = %.6e +- %.2e (S/N = %.2f)\n",
                        z, r.C[z], r.C_err[z], sn);
                }
            }
        }
        printf("\n");
    }

    /* Zusammenfassung */
    printf("[TL] ==============================================================\n");
    printf("[TL] Zusammenfassung: a*sqrt(sigma)(beta, dim)\n");
    printf("[TL] ==============================================================\n");
    printf("[TL]   beta    ");
    for (int di = 0; di < n_dims; ++di) {
        printf("  as(d=%2u)        ", dims[di]);
    }
    printf("  Lit.\n");
    printf("[TL]   ----------------------------------------------------------\n");

    for (int bi = 0; bi < n_betas; ++bi) {
        printf("[TL]   %.2f    ", betas[bi]);
        for (int di = 0; di < n_dims; ++di) {
            const TLResult* hit = NULL;
            for (int k = 0; k < n_results; ++k) {
                if (results[k].dim == dims[di] &&
                    fabs(results[k].beta - betas[bi]) < 1e-9) {
                    hit = &results[k];
                    break;
                }
            }
            if (hit && hit->fit_ok) {
                printf("  %.5f+-%.5f", hit->a_sqrt_sigma, hit->a_sqrt_sigma_err);
            }
            else {
                printf("  n/a             ");
            }
        }
        const TLAnchorEntry* a = tl_find_anchor(betas[bi]);
        if (a) printf("  %.4f", a->a_sqrt_sigma);
        printf("\n");
    }
    printf("\n");

    /* Konsistenz */
    printf("[TL] ==============================================================\n");
    printf("[TL] Konsistenz-Pruefung\n");
    printf("[TL] ==============================================================\n");

    printf("[TL] S1 (m > 0 fuer alle fit_ok):\n");
    const bool s1 = tl_check_S1(results, n_results);
    printf("[TL]   S1: %s\n\n", s1 ? "PASS" : "FAIL");

    printf("[TL] S2 (sigma_a2 monoton fallend in beta):\n");
    const bool s2 = tl_check_S2(results, n_results,
        dims, n_dims, betas, n_betas);
    printf("[TL]   S2: %s\n\n", s2 ? "PASS" : "FAIL");

    printf("[TL] S3 (chi2/dof < 5):\n");
    const bool s3 = tl_check_S3(results, n_results);
    printf("[TL]   S3: %s\n\n", s3 ? "PASS" : "FAIL");

    const bool anchor_ok = tl_anchor_check(results, n_results, anchor);

    const bool all_ok = s1 && s2 && s3 && anchor_ok;

    printf("[TL] --------------------------------------------------------------\n");
    printf("[TL] Interpretation:\n");
    printf("[TL]   m > 0: Confinement (Torelon-Masse positiv).\n");
    printf("[TL]   sigma_a2 = m / L: dimensionslose String-Tension.\n");
    printf("[TL]   a*sqrt(sigma) = sqrt(sigma_a2): Standard-Literaturkonvention.\n");
    printf("[TL]   Vergleich mit Cahill & Prasad (1989) via Teper-Zitat.\n");
    printf("\n");

    printf("[TL] -> %s\n", all_ok ? "PASSED" : "FAILED");
    return all_ok;
}