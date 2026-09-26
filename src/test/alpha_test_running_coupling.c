/* ==========================================================================
 * alpha_test_running_coupling.c
 *
 * Etappe 23 (II): Running Coupling / Beta-Funktion.
 *
 * Metropolis-Sampling auf den SU(2)-Links. Observable:
 *   u_plaq(beta, dim) = <S_plaq> / (2 * N_plaq)
 *
 * mit N_plaq = 3 * dim^3 in 3D.
 *
 * Sweep ueber dim in {16, 32, 64} und beta in {0.5, 1, 2, 4}.
 * Ausgabe als Tabelle; Beta-Funktion wird extern gefittet.
 *
 * Wichtig:
 *   - Kernel wird nicht veraendert. Nutzt nur
 *     ProPhysics_Set_Edge_SU2, ProPhysics_Get_Edge_SU2,
 *     ProPhysics_SU2_Link_Plaquette_Sum, ProPhysics_SU2_Plaquette_Action.
 *   - su2_dynamics_active bleibt aus. Leapfrog laeuft nicht.
 *   - Jeder Test benutzt ein eigenes ProUniverse.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RC_N_THERMAL   200u
#define RC_N_MEASURE   300u
#define RC_BIN_SIZE    10u
#define RC_MAX_DIMS    8u
#define RC_MAX_BETAS   8u

 /* Vorwaerts-/Rueckwaerts-Kanaele in 3D. */
static const uint8_t RC_FWD[3] = { 0u, 2u, 4u };
static const uint8_t RC_REV[3] = { 1u, 3u, 6u };

/* --------------------------------------------------------------------------
 * Box-Muller aus zwei Uniformen in (0,1). Liefert eine Standardnormalzahl.
 * ----------------------------------------------------------------------- */
static double rc_gauss(double u1, double u2)
{
    if (u1 < 1e-30) u1 = 1e-30;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double rc_uniform(prng_state_t* rng)
{
    return (double)(prng_next(rng) >> 11) / 9007199254740992.0;
}

/* --------------------------------------------------------------------------
 * Links zufaellig initialisieren (uniform auf S^3 via 4D-Gauss-Normierung).
 * ----------------------------------------------------------------------- */
static void rc_randomize_links(ProUniverse* pu, prng_state_t* rng)
{
    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = RC_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            const double g1 = rc_gauss(rc_uniform(rng), rc_uniform(rng));
            const double g2 = rc_gauss(rc_uniform(rng), rc_uniform(rng));
            const double g3 = rc_gauss(rc_uniform(rng), rc_uniform(rng));
            const double g4 = rc_gauss(rc_uniform(rng), rc_uniform(rng));

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
 * Ein Metropolis-Sweep: einmal ueber alle Links, in fester Reihenfolge.
 * Rueckgabe: Akzeptanzrate.
 * ----------------------------------------------------------------------- */
static double rc_metropolis_sweep(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng)
{
    uint64_t accept = 0, total = 0;

    for (uint64_t x = 0; x < pu->total_nodes; ++x) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = RC_FWD[d];
            if (pu->reg_source[x].channels[mu] >= pu->total_nodes) continue;
            if (pu->reg_source[x].channels[mu] == x) continue;

            int32_t ar, ai, br, bi;
            ProPhysics_Get_Edge_SU2(pu, x, mu, &ar, &ai, &br, &bi);

            const double S_before =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double eps_q30 = eps * (double)PRO_SU2_SCALE;
            const int32_t d_ar = (int32_t)llround(rc_gauss(rc_uniform(rng), rc_uniform(rng)) * eps_q30);
            const int32_t d_ai = (int32_t)llround(rc_gauss(rc_uniform(rng), rc_uniform(rng)) * eps_q30);
            const int32_t d_br = (int32_t)llround(rc_gauss(rc_uniform(rng), rc_uniform(rng)) * eps_q30);
            const int32_t d_bi = (int32_t)llround(rc_gauss(rc_uniform(rng), rc_uniform(rng)) * eps_q30);

            ProPhysics_Set_Edge_SU2(pu, x, mu,
                ar + d_ar, ai + d_ai, br + d_br, bi + d_bi);

            const double S_after =
                ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu);

            const double dS = S_after - S_before;
            const double u = rc_uniform(rng);

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
static double rc_thermalize(ProUniverse* pu, double beta, prng_state_t* rng)
{
    double eps = 0.3;
    for (int pass = 0; pass < 4; ++pass) {
        double rate = 0.0;
        const int n_quick = 10;
        for (int s = 0; s < n_quick; ++s) {
            rate += rc_metropolis_sweep(pu, beta, eps, rng);
        }
        rate /= (double)n_quick;
        if (rate < 0.30) eps *= 0.6;
        else if (rate > 0.70) eps *= 1.5;
        else break;
        if (eps < 1e-4) eps = 1e-4;
        if (eps > 2.0)  eps = 2.0;
    }
    for (uint32_t s = 0; s < RC_N_THERMAL; ++s) {
        (void)rc_metropolis_sweep(pu, beta, eps, rng);
    }
    return eps;
}

/* --------------------------------------------------------------------------
 * Messphase mit Binning.
 * ----------------------------------------------------------------------- */
static void rc_measure(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng,
    double* out_u, double* out_err, double* out_acc)
{
    const double S_scale = 1.0 / (2.0
        * 3.0 * (double)pu->grid_dim * (double)pu->grid_dim * (double)pu->grid_dim);

    const uint32_t n_bins = RC_N_MEASURE / RC_BIN_SIZE;
    double bin_sum[64];
    if (n_bins > 64u) { *out_u = 0.0; *out_err = 0.0; *out_acc = 0.0; return; }
    for (uint32_t i = 0; i < n_bins; ++i) bin_sum[i] = 0.0;

    double acc_total = 0.0;

    for (uint32_t s = 0; s < RC_N_MEASURE; ++s) {
        const double rate = rc_metropolis_sweep(pu, beta, eps, rng);
        acc_total += rate;
        const double u = ProPhysics_SU2_Plaquette_Action(pu) * S_scale;
        bin_sum[s / RC_BIN_SIZE] += u;
    }

    double mean = 0.0;
    for (uint32_t i = 0; i < n_bins; ++i) {
        mean += bin_sum[i] / (double)RC_BIN_SIZE;
    }
    mean /= (double)n_bins;

    double var = 0.0;
    for (uint32_t i = 0; i < n_bins; ++i) {
        const double bm = bin_sum[i] / (double)RC_BIN_SIZE;
        var += (bm - mean) * (bm - mean);
    }
    var /= (double)(n_bins - 1u);
    const double err = sqrt(var / (double)n_bins);

    *out_u = mean;
    *out_err = err;
    *out_acc = acc_total / (double)RC_N_MEASURE;
}

/* --------------------------------------------------------------------------
 * Ein Lauf fuer festes (dim, beta).
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t dim;
    double   beta;
    double   u_plaq;
    double   u_err;
    double   accept_rate;
    double   eps;
} RCResult;

static bool rc_run_one(uint32_t dim, double beta, RCResult* out)
{
    const uint64_t N = (uint64_t)dim * (uint64_t)dim * (uint64_t)dim;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.edge_phases || !pu.reg_source || !pu.amp_grid) {
        ProPhysics_Free(&pu);
        return false;
    }

    wire_torus_3d(&pu, dim);

    /* Seed deterministisch pro (dim, beta). */
    prng_state_t rng;
    const uint64_t seed = 0xC0DE0000ULL
        + ((uint64_t)dim << 16)
        + (uint64_t)llround(beta * 1000.0);
    prng_seed(&rng, seed);

    rc_randomize_links(&pu, &rng);
    const double eps = rc_thermalize(&pu, beta, &rng);

    double u, err, acc;
    rc_measure(&pu, beta, eps, &rng, &u, &err, &acc);

    out->dim = dim;
    out->beta = beta;
    out->u_plaq = u;
    out->u_err = err;
    out->accept_rate = acc;
    out->eps = eps;

    ProPhysics_Free(&pu);
    return true;
}

/* ==========================================================================
 * Haupttest.
 * ========================================================================== */

bool test_running_coupling(void)
{
    printf("========================================================================\n");
    printf("  Etappe 23 (II): Running Coupling / Beta-Funktion\n");
    printf("========================================================================\n\n");
    printf("[RC] Metropolis-Sampling auf SU(2)-Links.\n");
    printf("[RC] Observable: u_plaq = <S_plaq> / (2 * N_plaq).\n");
    printf("[RC] Sweep: dim in {16, 32, 64} x beta in {0.5, 1, 2, 4}.\n");
    printf("[RC] Thermalisierung: %u Sweeps, Messung: %u Sweeps, Bins: %u.\n\n",
        (unsigned)RC_N_THERMAL, (unsigned)RC_N_MEASURE,
        (unsigned)(RC_N_MEASURE / RC_BIN_SIZE));

    static const uint32_t dims[] = { 16u, 32u, 64u };
    static const double   betas[] = { 0.5, 1.0, 2.0, 4.0 };
    const int n_dims = 3;
    const int n_betas = 4;

    RCResult results[RC_MAX_DIMS * RC_MAX_BETAS];
    int n_results = 0;

    for (int di = 0; di < n_dims; ++di) {
        const uint32_t dim = dims[di];

        printf("[RC] --------------------------------------------------------------\n");
        printf("[RC] dim = %u  (N = %llu Knoten)\n", dim,
            (unsigned long long)((uint64_t)dim * dim * dim));
        printf("[RC] --------------------------------------------------------------\n");
        printf("[RC]   beta     eps     accept    u_plaq       u_err\n");
        printf("[RC]   -------------------------------------------------------\n");

        for (int bi = 0; bi < n_betas; ++bi) {
            RCResult r;
            if (!rc_run_one(dim, betas[bi], &r)) {
                printf("[RC]   %.2f    ---     ---       (Lauf fehlgeschlagen)\n",
                    betas[bi]);
                continue;
            }
            if (n_results < RC_MAX_DIMS * RC_MAX_BETAS) {
                results[n_results++] = r;
            }
            printf("[RC]   %.2f    %.3f   %.3f     %.6f   %.6f\n",
                r.beta, r.eps, r.accept_rate, r.u_plaq, r.u_err);
        }
        printf("\n");
    }

    /* Zusammenfassung: u_plaq(beta, dim) als Tabelle. */
    printf("[RC] ==============================================================\n");
    printf("[RC] Zusammenfassung: u_plaq(beta, dim)\n");
    printf("[RC] ==============================================================\n");
    printf("[RC]   beta    ");
    for (int di = 0; di < n_dims; ++di) printf("  u(d=%2u)     ", dims[di]);
    printf("\n");
    printf("[RC]   -------------------------------------------------------\n");

    for (int bi = 0; bi < n_betas; ++bi) {
        printf("[RC]   %.2f    ", betas[bi]);
        for (int di = 0; di < n_dims; ++di) {
            double u = 0.0, e = 0.0;
            for (int k = 0; k < n_results; ++k) {
                if (results[k].dim == dims[di] &&
                    fabs(results[k].beta - betas[bi]) < 1e-9) {
                    u = results[k].u_plaq;
                    e = results[k].u_err;
                    break;
                }
            }
            printf("  %.4f+-%.4f", u, e);
        }
        printf("\n");
    }

    printf("\n[RC] Interpretation:\n");
    printf("[RC]   u_plaq -> 1.0 fuer beta -> inf (schwache Kopplung)\n");
    printf("[RC]   u_plaq -> ~0.5 fuer beta -> 0  (starke Kopplung)\n");
    printf("[RC]   Verschiebung mit dim zeigt laufende Kopplung.\n");
    printf("[RC]   Beta-Funktion wird extern gefittet.\n");

    printf("\n[RC] -> PASSED (Rohdaten, weitere Analyse extern)\n");
    return true;
}