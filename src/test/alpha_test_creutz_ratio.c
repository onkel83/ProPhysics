/* ==========================================================================
 * alpha_test_creutz_ratio.c
 *
 * Etappe 23b: Creutz-Ratio als Konsistenz-Test des Metropolis-Samplers.
 *
 * Observable:
 *   chi(2,2) = -ln( W(2,2) * W(1,1) / W(2,1)^2 )
 *
 * mit W(m,n) = <Re Tr(W_C)/2> ueber alle m x n-Loops des 3D-Torus,
 * gemittelt ueber die drei Ebenen xy, xz, yz (Kernel-Funktion
 * ProPhysics_Wilson_Loop_Average).
 *
 * Ziel: kein neuer V&V-Anker. Stattdessen ein Konsistenz-Test:
 *   B1: chi > 0 fuer alle (dim, beta)
 *   B2: chi monoton fallend in beta (pro dim)
 *   B3: chi konsistent ueber dim innerhalb 3 sigma (pro beta)
 *
 * Sweep:
 *   Default:  dim in {16, 32}
 *   --creutz-full: dim in {16, 32, 64, 128}
 *   beta in {1.0, 2.0, 4.0}
 *
 * Thermalisierung 200 Sweeps, Messung 300 Sweeps, 30 Bins a 10 Sweeps.
 * Loop-Messung am Ende jedes Bins (reduziert O(dim^3)-Kosten).
 *
 * Deterministischer Seed pro (dim, beta):
 *   seed = 0xC0DE0000 + (dim<<16) + round(beta*1000)
 *
 * R-Konformitaet:
 *   - Kernel wird nicht veraendert. Nutzt nur
 *     ProPhysics_Set_Edge_SU2, ProPhysics_Get_Edge_SU2,
 *     ProPhysics_SU2_Link_Plaquette_Sum, ProPhysics_Wilson_Loop_Average.
 *   - su2_dynamics_active bleibt aus. Leapfrog laeuft nicht.
 *   - Jeder Lauf benutzt ein eigenes ProUniverse.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

 /* --------------------------------------------------------------------------
  * Konstanten
  * ----------------------------------------------------------------------- */

#define CR_N_THERMAL    200u
#define CR_N_MEASURE    300u
#define CR_BIN_SIZE     10u
#define CR_N_BINS       (CR_N_MEASURE / CR_BIN_SIZE)   /* = 30 */
#define CR_MAX_DIMS     4u
#define CR_MAX_BETAS    3u
#define CR_MAX_RUNS     (CR_MAX_DIMS * CR_MAX_BETAS)

  /* Vorwaerts-/Rueckwaerts-Kanaele in 3D (bit-interleaved (z,y,x)). */
static const uint8_t CR_FWD[3] = { 0u, 2u, 4u };

/* --------------------------------------------------------------------------
 * RNG-Helfer (Box-Muller aus zwei Uniformen in (0,1)).
 * ----------------------------------------------------------------------- */
static double cr_gauss(double u1, double u2)
{
    if (u1 < 1e-30) u1 = 1e-30;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double cr_uniform(prng_state_t* rng)
{
    return (double)(prng_next(rng) >> 11) / 9007199254740992.0;
}

/* --------------------------------------------------------------------------
 * Links zufaellig initialisieren (uniform auf S^3 via 4D-Gauss-Normierung).
 * ----------------------------------------------------------------------- */
static void cr_randomize_links(ProUniverse* pu, prng_state_t* rng)
{
    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = CR_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            const double g1 = cr_gauss(cr_uniform(rng), cr_uniform(rng));
            const double g2 = cr_gauss(cr_uniform(rng), cr_uniform(rng));
            const double g3 = cr_gauss(cr_uniform(rng), cr_uniform(rng));
            const double g4 = cr_gauss(cr_uniform(rng), cr_uniform(rng));

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
 * Ein Metropolis-Sweep ueber alle Links.
 * Rueckgabe: Akzeptanzrate.
 * ----------------------------------------------------------------------- */
static double cr_metropolis_sweep(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng)
{
    uint64_t accept = 0, total = 0;

    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = CR_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            int32_t ar, ai, br, bi;
            ProPhysics_Get_Edge_SU2(pu, x, mu, &ar, &ai, &br, &bi);

            const double S_before =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double eps_q30 = eps * (double)PRO_SU2_SCALE;
            const int32_t d_ar = (int32_t)llround(cr_gauss(cr_uniform(rng), cr_uniform(rng)) * eps_q30);
            const int32_t d_ai = (int32_t)llround(cr_gauss(cr_uniform(rng), cr_uniform(rng)) * eps_q30);
            const int32_t d_br = (int32_t)llround(cr_gauss(cr_uniform(rng), cr_uniform(rng)) * eps_q30);
            const int32_t d_bi = (int32_t)llround(cr_gauss(cr_uniform(rng), cr_uniform(rng)) * eps_q30);

            ProPhysics_Set_Edge_SU2(pu, x, mu,
                ar + d_ar, ai + d_ai, br + d_br, bi + d_bi);

            const double S_after =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double dS = S_after - S_before;
            const double u = cr_uniform(rng);

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
static double cr_thermalize(ProUniverse* pu, double beta, prng_state_t* rng)
{
    double eps = 0.3;
    for (int pass = 0; pass < 4; ++pass) {
        double rate = 0.0;
        const int n_quick = 10;
        for (int s = 0; s < n_quick; ++s) {
            rate += cr_metropolis_sweep(pu, beta, eps, rng);
        }
        rate /= (double)n_quick;
        if (rate < 0.30) eps *= 0.6;
        else if (rate > 0.70) eps *= 1.5;
        else break;
        if (eps < 1e-4) eps = 1e-4;
        if (eps > 2.0)  eps = 2.0;
    }
    for (uint32_t s = 0; s < CR_N_THERMAL; ++s) {
        (void)cr_metropolis_sweep(pu, beta, eps, rng);
    }
    return eps;
}

/* --------------------------------------------------------------------------
 * Messphase mit Binning.
 *
 * Pro Bin:
 *   - 10 Metropolis-Sweeps,
 *   - danach Loop-Messung: W(1,1), W(2,1), W(2,2) via Kernel-Average,
 *   - Bin-Wert chi_b = -ln( W22_b * W11_b / W21_b^2 ).
 *
 * Verworfen werden Bins, in denen
 *   - W21_b < 1e-6 oder
 *   - W22_b * W11_b <= 0.
 * Das sollte selten passieren (nur bei pathologischen Konfigurationen).
 * ----------------------------------------------------------------------- */
typedef struct {
    double w11_mean, w11_err;
    double w21_mean, w21_err;
    double w22_mean, w22_err;
    double chi_mean, chi_err;
    uint32_t n_chi_valid;
    double   accept_rate;
} CRMetrics;

static void cr_measure(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng, CRMetrics* out)
{
    double w11_b[CR_N_BINS];
    double w21_b[CR_N_BINS];
    double w22_b[CR_N_BINS];
    double chi_b[CR_N_BINS];
    uint32_t n_chi_valid = 0u;

    double acc_total = 0.0;

    for (uint32_t b = 0; b < CR_N_BINS; ++b) {
        for (uint32_t s = 0; s < CR_BIN_SIZE; ++s) {
            const double rate = cr_metropolis_sweep(pu, beta, eps, rng);
            acc_total += rate;
        }

        /* Loop-Messung (teuer: O(dim^3 * (m+n))). */
        const double W11 = ProPhysics_Wilson_Loop_Average(pu, 1u, 1u);
        const double W21 = ProPhysics_Wilson_Loop_Average(pu, 2u, 1u);
        const double W22 = ProPhysics_Wilson_Loop_Average(pu, 2u, 2u);

        w11_b[b] = W11;
        w21_b[b] = W21;
        w22_b[b] = W22;

        const double prod = W22 * W11;
        const double denom = W21 * W21;
        if (denom > 1e-12 && prod > 1e-12) {
            chi_b[b] = -log(prod / denom);
            n_chi_valid++;
        }
        else {
            chi_b[b] = 0.0;
        }
    }

    /* Mittelwerte und Standardfehler (Binning-Methode). */
    const double inv_n = 1.0 / (double)CR_N_BINS;

    double m11 = 0.0, m21 = 0.0, m22 = 0.0;
    for (uint32_t b = 0; b < CR_N_BINS; ++b) {
        m11 += w11_b[b];
        m21 += w21_b[b];
        m22 += w22_b[b];
    }
    m11 *= inv_n; m21 *= inv_n; m22 *= inv_n;

    double v11 = 0.0, v21 = 0.0, v22 = 0.0;
    for (uint32_t b = 0; b < CR_N_BINS; ++b) {
        v11 += (w11_b[b] - m11) * (w11_b[b] - m11);
        v21 += (w21_b[b] - m21) * (w21_b[b] - m21);
        v22 += (w22_b[b] - m22) * (w22_b[b] - m22);
    }
    v11 /= (double)(CR_N_BINS - 1u);
    v21 /= (double)(CR_N_BINS - 1u);
    v22 /= (double)(CR_N_BINS - 1u);

    double mchi = 0.0;
    for (uint32_t b = 0; b < CR_N_BINS; ++b) {
        mchi += chi_b[b];
    }
    mchi /= (double)CR_N_BINS;

    double vchi = 0.0;
    for (uint32_t b = 0; b < CR_N_BINS; ++b) {
        vchi += (chi_b[b] - mchi) * (chi_b[b] - mchi);
    }
    vchi /= (double)(CR_N_BINS - 1u);

    out->w11_mean = m11;
    out->w11_err = sqrt(v11 / (double)CR_N_BINS);
    out->w21_mean = m21;
    out->w21_err = sqrt(v21 / (double)CR_N_BINS);
    out->w22_mean = m22;
    out->w22_err = sqrt(v22 / (double)CR_N_BINS);
    out->chi_mean = mchi;
    out->chi_err = sqrt(vchi / (double)CR_N_BINS);
    out->n_chi_valid = n_chi_valid;
    out->accept_rate = acc_total / (double)CR_N_MEASURE;
}

/* --------------------------------------------------------------------------
 * Ein Lauf fuer festes (dim, beta).
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t dim;
    double   beta;
    double   W11, W11_err;
    double   W21, W21_err;
    double   W22, W22_err;
    double   chi, chi_err;
    uint32_t n_chi_valid;
    double   accept_rate;
    double   eps;
} CRResult;

static bool cr_run_one(uint32_t dim, double beta, CRResult* out)
{
    const uint64_t N = (uint64_t)dim * (uint64_t)dim * (uint64_t)dim;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.edge_phases || !pu.reg_source || !pu.amp_grid) {
        ProPhysics_Free(&pu);
        return false;
    }

    wire_torus_3d(&pu, dim);

    /* Deterministischer Seed pro (dim, beta). */
    prng_state_t rng;
    const uint64_t seed = 0xC0DE0000ULL
        + ((uint64_t)dim << 16)
        + (uint64_t)llround(beta * 1000.0);
    prng_seed(&rng, seed);

    cr_randomize_links(&pu, &rng);
    const double eps = cr_thermalize(&pu, beta, &rng);

    CRMetrics m;
    cr_measure(&pu, beta, eps, &rng, &m);

    out->dim = dim;
    out->beta = beta;
    out->W11 = m.w11_mean; out->W11_err = m.w11_err;
    out->W21 = m.w21_mean; out->W21_err = m.w21_err;
    out->W22 = m.w22_mean; out->W22_err = m.w22_err;
    out->chi = m.chi_mean; out->chi_err = m.chi_err;
    out->n_chi_valid = m.n_chi_valid;
    out->accept_rate = m.accept_rate;
    out->eps = eps;

    ProPhysics_Free(&pu);
    return true;
}

/* --------------------------------------------------------------------------
 * Konsistenz-Pruefungen B1, B2, B3.
 * ----------------------------------------------------------------------- */
static bool cr_check_B1(const CRResult* r, int n_runs)
{
    bool ok = true;
    for (int k = 0; k < n_runs; ++k) {
        if (r[k].chi <= 0.0) {
            printf("[CRx]   B1 FAIL: chi(dim=%u, beta=%.2f) = %.6f <= 0\n",
                r[k].dim, r[k].beta, r[k].chi);
            ok = false;
        }
    }
    return ok;
}

static bool cr_check_B2(const CRResult* r, int n_runs,
    const uint32_t* dims, int n_dims,
    const double* betas, int n_betas)
{
    bool ok = true;
    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];
        /* Betas sind aufsteigend sortiert; chi muss streng fallen. */
        double prev = 0.0;
        bool have_prev = false;
        for (int bi = 0; bi < n_betas; ++bi) {
            const double beta = betas[bi];
            const CRResult* hit = NULL;
            for (int k = 0; k < n_runs; ++k) {
                if (r[k].dim == dim &&
                    fabs(r[k].beta - beta) < 1e-9) {
                    hit = &r[k];
                    break;
                }
            }
            if (!hit) continue;
            if (have_prev) {
                if (!(hit->chi < prev)) {
                    printf("[CRx]   B2 FAIL: dim=%u beta=%.2f "
                        "chi=%.6f >= chi(prev)=%.6f\n",
                        dim, beta, hit->chi, prev);
                    ok = false;
                }
            }
            prev = hit->chi;
            have_prev = true;
        }
    }
    return ok;
}

static bool cr_check_B3(const CRResult* r, int n_runs,
    const uint32_t* dims, int n_dims,
    const double* betas, int n_betas)
{
    /* Fuer jedes beta: paarweise Vergleiche der dims.
     * |chi_i - chi_j| < 3 * sqrt(sigma_i^2 + sigma_j^2).
     * Nur wenn beide sigma > 0 sind. */
    bool ok = true;
    for (int bi = 0; bi < n_betas; ++bi) {
        const double beta = betas[bi];
        for (int i = 0; i < n_dims; ++i) {
            for (int j = i + 1; j < n_dims; ++j) {
                const CRResult* ri = NULL;
                const CRResult* rj = NULL;
                for (int k = 0; k < n_runs; ++k) {
                    if (r[k].dim == dims[i] &&
                        fabs(r[k].beta - beta) < 1e-9) ri = &r[k];
                    if (r[k].dim == dims[j] &&
                        fabs(r[k].beta - beta) < 1e-9) rj = &r[k];
                }
                if (!ri || !rj) continue;
                const double diff = fabs(ri->chi - rj->chi);
                const double tol = 3.0 * sqrt(ri->chi_err * ri->chi_err
                    + rj->chi_err * rj->chi_err);
                const char* mark = (diff <= tol) ? "OK" : "FAIL";
                printf("[CRx]   beta %.2f: |chi(%u)-chi(%u)| = "
                    "%.6f vs 3sigma = %.6f  %s\n",
                    beta, dims[i], dims[j], diff, tol, mark);
                if (diff > tol) ok = false;
            }
        }
    }
    return ok;
}

/* ==========================================================================
 * Haupttest.
 * ========================================================================== */

bool test_creutz_ratio(bool full_dims)
{
    printf("========================================================================\n");
    printf("  Etappe 23b: Creutz-Ratio Konsistenz-Test\n");
    printf("========================================================================\n\n");
    printf("[CRx] Ziel: Konsistenz des Metropolis-Samplers auf SU(2)-Links.\n");
    printf("[CRx] Observable: chi(2,2) = -ln( W(2,2)*W(1,1) / W(2,1)^2 ).\n");

    static const uint32_t dims_full[] = { 16u, 32u, 64u, 128u };
    static const uint32_t dims_fast[] = { 16u, 32u };
    const uint32_t* dims = full_dims ? dims_full : dims_fast;
    const int n_dims = full_dims ? 4 : 2;

    static const double betas[] = { 1.0, 2.0, 4.0 };
    const int n_betas = 3;

    printf("[CRx] Sweep: dim in {");
    for (int di = 0; di < n_dims; ++di) {
        printf("%u%s", dims[di], (di + 1 < n_dims) ? ", " : "");
    }
    printf("} x beta in {1.0, 2.0, 4.0}.\n");
    printf("[CRx] Modus: %s\n", full_dims ? "FULL (--creutz-full)" : "FAST (default)");
    printf("[CRx] Thermalisierung: %u Sweeps, Messung: %u Sweeps, Bins: %u.\n",
        (unsigned)CR_N_THERMAL, (unsigned)CR_N_MEASURE, (unsigned)CR_N_BINS);
    printf("[CRx] Loop-Messung am Ende jedes Bins.\n\n");

    CRResult results[CR_MAX_RUNS];
    int n_results = 0;

    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];

        printf("[CRx] --------------------------------------------------------------\n");
        printf("[CRx] dim = %u  (N = %llu Knoten)\n", dim,
            (unsigned long long)((uint64_t)dim * dim * dim));
        printf("[CRx] --------------------------------------------------------------\n");
        printf("[CRx]   beta    eps    accept    W(1,1)         W(2,1)         "
            "W(2,2)         chi(2,2)\n");
        printf("[CRx]   --------------------------------------------------------"
            "---------------------------------------------\n");

        for (int bi = 0; bi < n_betas; ++bi) {
            CRResult r;
            if (!cr_run_one(dim, betas[bi], &r)) {
                printf("[CRx]   %.2f    ---    ---       "
                    "(Lauf fehlgeschlagen)\n", betas[bi]);
                continue;
            }
            if (n_results < CR_MAX_RUNS) {
                results[n_results++] = r;
            }
            printf("[CRx]   %.2f    %.3f  %.3f     "
                "%.5f+-%.5f  %.5f+-%.5f  %.5f+-%.5f  %.5f+-%.5f\n",
                r.beta, r.eps, r.accept_rate,
                r.W11, r.W11_err,
                r.W21, r.W21_err,
                r.W22, r.W22_err,
                r.chi, r.chi_err);
        }
        printf("\n");
    }

    /* Zusammenfassung: chi(beta, dim) als Matrix. */
    printf("[CRx] ==============================================================\n");
    printf("[CRx] Zusammenfassung: chi(2,2)(beta, dim)\n");
    printf("[CRx] ==============================================================\n");
    printf("[CRx]   beta    ");
    for (int di = 0; di < n_dims; ++di) {
        printf("  chi(d=%3u)     ", dims[di]);
    }
    printf("\n");
    printf("[CRx]   ----------------------------------------------------------\n");

    for (int bi = 0; bi < n_betas; ++bi) {
        printf("[CRx]   %.2f    ", betas[bi]);
        for (int di = 0; di < n_dims; ++di) {
            double chi = 0.0, err = 0.0;
            for (int k = 0; k < n_results; ++k) {
                if (results[k].dim == dims[di] &&
                    fabs(results[k].beta - betas[bi]) < 1e-9) {
                    chi = results[k].chi;
                    err = results[k].chi_err;
                    break;
                }
            }
            printf("  %.5f+-%.5f", chi, err);
        }
        printf("\n");
    }
    printf("\n");

    /* Konsistenz-Pruefung. */
    printf("[CRx] ==============================================================\n");
    printf("[CRx] Konsistenz-Pruefung\n");
    printf("[CRx] ==============================================================\n");

    printf("[CRx] B1 (chi > 0 fuer alle dim,beta):\n");
    const bool b1 = cr_check_B1(results, n_results);
    printf("[CRx]   B1: %s\n\n", b1 ? "PASS" : "FAIL");

    printf("[CRx] B2 (chi monoton fallend in beta pro dim):\n");
    const bool b2 = cr_check_B2(results, n_results,
        dims, n_dims, betas, n_betas);
    printf("[CRx]   B2: %s\n\n", b2 ? "PASS" : "FAIL");

    printf("[CRx] B3 (chi konsistent ueber dim, 3 sigma):\n");
    const bool b3 = cr_check_B3(results, n_results,
        dims, n_dims, betas, n_betas);
    printf("[CRx]   B3: %s\n\n", b3 ? "PASS" : "FAIL");

    const bool all_ok = b1 && b2 && b3;

    printf("[CRx] --------------------------------------------------------------\n");
    printf("[CRx] Interpretation:\n");
    printf("[CRx]   chi > 0: Loops groesserer Ausdehnung sind staerker\n");
    printf("[CRx]            unterdrueckt als kleinere (Confinement-Signal).\n");
    printf("[CRx]   chi faellt in beta: Kopplung wird schwaecher.\n");
    printf("[CRx]   chi ~ dim-unabhaengig: UV-Konsistenz im getesteten Bereich.\n");
    printf("[CRx]   Kein V&V-Anker -- reiner Konsistenz-Test.\n\n");

    printf("[CRx] -> %s\n", all_ok ? "PASSED" : "FAILED");
    return all_ok;
}