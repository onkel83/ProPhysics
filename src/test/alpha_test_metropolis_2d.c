/* ==========================================================================
 * alpha_test_metropolis_2d.c
 *
 * Etappe 23e: 2D SU(2) Metropolis Validierung.
 *
 * Referenz (KORRIGIERT):
 *
 *   <W_plaq> = <(1/2) Re Tr(U_plaq)> = I2(beta)/I1(beta)
 *
 *   Nicht I1(beta)/I0(beta). Letzteres gilt fuer U(1), nicht SU(2).
 *
 *   Herleitung:
 *     Z = int_SU(2) dU exp(-beta * (1 - (1/2) Re Tr U))
 *     Haar-Mass: int dU f(U) = (2/pi) int_0^pi sin^2(alpha) f(alpha) dalpha
 *     <cos alpha> = d ln Z / d beta = I2(beta)/I1(beta)
 *   mit cos(alpha) = (1/2) Re Tr U.
 *
 *   Der Test-Harness prueft jetzt gegen I2/I1. Numerische Verifikation:
 *     beta=0.5: I2/I1 = 0.1237  vs  gemessen 0.1252
 *     beta=1.0: I2/I1 = 0.2401  vs  gemessen 0.2409
 *     beta=2.0: I2/I1 = 0.4331  vs  gemessen 0.4345
 *     beta=4.0: I2/I1 = 0.6580  vs  gemessen 0.6558
 *   Alle innerhalb der statistischen Fehler.
 *
 * Sampler (beibehalten):
 *   Haar-Vorschlag U -> R*U mit R = exp(-i*(alpha/2)*n.sigma),
 *   alpha ~ N(0, eps^2), n uniform auf S^2. Exakt symmetrisch,
 *   kein Overflow-Risiko.
 *
 * Diagnose-Block (dim=4) unveraendert.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MP2D_DIM_FAST        16u
#define MP2D_DIM_FULL        32u
#define MP2D_N_THERMAL       500u
#define MP2D_N_MEASURE       1000u
#define MP2D_N_BINS          50u
#define MP2D_BIN_SIZE        (MP2D_N_MEASURE / MP2D_N_BINS)

 /* --------------------------------------------------------------------------
  * Helper: log2 fuer Zweierpotenzen
  * ----------------------------------------------------------------------- */

static uint32_t mp2d_log2_u32(uint32_t x)
{
    uint32_t r = 0;
    while ((1u << r) < x) r++;
    return r;
}

/* --------------------------------------------------------------------------
 * Eigener bit-interleaved 2D-Torus.
 * idx = x | (y << shift)
 * ----------------------------------------------------------------------- */

static void mp2d_wire_torus_2d(ProUniverse* pu, uint32_t dim)
{
    const uint32_t shift = mp2d_log2_u32(dim);
    const uint32_t mask = dim - 1u;

    pu->grid_dim = dim;
    pu->grid_dim_shift = shift;
    pu->grid_dim_mask = mask;
    pu->grid_ndim = 2;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        for (int c = 0; c < CHANNELS_MAX; ++c) {
            pu->reg_source[k].channels[c] = k;
        }
    }

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = (uint64_t)x | ((uint64_t)y << shift);

            const uint32_t xp = (x + 1u) & mask;
            const uint32_t xm = (x + mask) & mask;
            const uint32_t yp = (y + 1u) & mask;
            const uint32_t ym = (y + mask) & mask;

            pu->reg_source[idx].channels[PRO_NEIGHBOR_X_PLUS] =
                (uint64_t)xp | ((uint64_t)y << shift);
            pu->reg_source[idx].channels[PRO_NEIGHBOR_X_MINUS] =
                (uint64_t)xm | ((uint64_t)y << shift);
            pu->reg_source[idx].channels[PRO_NEIGHBOR_Y_PLUS] =
                (uint64_t)x | ((uint64_t)yp << shift);
            pu->reg_source[idx].channels[PRO_NEIGHBOR_Y_MINUS] =
                (uint64_t)x | ((uint64_t)ym << shift);
        }
    }
}

/* --------------------------------------------------------------------------
 * RNG-Helfer
 * ----------------------------------------------------------------------- */

static double mp2d_gauss(double u1, double u2)
{
    if (u1 < 1e-30) u1 = 1e-30;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double mp2d_uniform(prng_state_t* rng)
{
    return (double)(prng_next(rng) >> 11) / 9007199254740992.0;
}

/* --------------------------------------------------------------------------
 * Bessel-Funktionen
 *
 * I_n(x) = sum_{k=0}^inf (x/2)^{2k+n} / (k! (k+n)!)
 *
 * Alle drei werden in der Form
 *   I_n(x) = (x/2)^n * sum_{k=0}^inf (x/2)^{2k} / (k! (k+n)!)
 * berechnet.
 *
 * I0(x) = sum_{k} (x^2/4)^k / (k!)^2
 * I1(x) = (x/2) sum_{k} (x^2/4)^k / (k! (k+1)!)
 * I2(x) = (x^2/4) sum_{k} (x^2/4)^k / (k! (k+2)!)
 * ----------------------------------------------------------------------- */

static double mp2d_bessel_i0(double x)
{
    double term = 1.0, sum = 1.0;
    const double xx4 = 0.25 * x * x;
    for (int k = 1; k < 200; ++k) {
        term *= xx4 / ((double)k * (double)k);
        sum += term;
        if (term < 1e-15 * sum) break;
    }
    return sum;
}

static double mp2d_bessel_i1(double x)
{
    double term = 1.0, sum = 1.0;
    const double xx4 = 0.25 * x * x;
    for (int k = 1; k < 200; ++k) {
        term *= xx4 / ((double)k * (double)(k + 1));
        sum += term;
        if (term < 1e-15 * sum) break;
    }
    return 0.5 * x * sum;
}

static double mp2d_bessel_i2(double x)
{
    /* k=0: (x/2)^{2} / (0! * 2!) = x^2/8;   Faktor (x/2)^2 = x^2/4 ausklammern
     *       -> Restterm k=0 ist 1/(0! * 2!) = 1/2. */
    const double xx4 = 0.25 * x * x;
    double term = 0.5;
    double sum = 0.5;
    for (int k = 1; k < 200; ++k) {
        term *= xx4 / ((double)k * (double)(k + 2));
        sum += term;
        if (term < 1e-15 * sum) break;
    }
    return xx4 * sum;
}

/* --------------------------------------------------------------------------
 * Quaternion-Arithmetik (fuer Plaquette-Berechnung)
 * ----------------------------------------------------------------------- */

static int64_t mp2d_rs_q30(int64_t x)
{
    if (x >= 0) return (x + (1LL << 29)) >> 30;
    return -(((-x) + (1LL << 29)) >> 30);
}

static void mp2d_quat_mul(
    int32_t a1r, int32_t a1i, int32_t b1r, int32_t b1i,
    int32_t a2r, int32_t a2i, int32_t b2r, int32_t b2i,
    int32_t* aor, int32_t* aoi, int32_t* bor, int32_t* boi)
{
    const int64_t ar =
        (int64_t)a1r * a2r - (int64_t)a1i * a2i
        - (int64_t)b1r * b2r - (int64_t)b1i * b2i;
    const int64_t ai =
        (int64_t)a1r * a2i + (int64_t)a1i * a2r
        + (int64_t)b1r * b2i - (int64_t)b1i * b2r;
    const int64_t br =
        (int64_t)a1r * b2r - (int64_t)a1i * b2i
        + (int64_t)b1r * a2r + (int64_t)b1i * a2i;
    const int64_t bi =
        (int64_t)a1r * b2i + (int64_t)a1i * b2r
        - (int64_t)b1r * a2i + (int64_t)b1i * a2r;

    *aor = (int32_t)mp2d_rs_q30(ar);
    *aoi = (int32_t)mp2d_rs_q30(ai);
    *bor = (int32_t)mp2d_rs_q30(br);
    *boi = (int32_t)mp2d_rs_q30(bi);
}

static void mp2d_quat_conj(
    int32_t ar, int32_t ai, int32_t br, int32_t bi,
    int32_t* cor, int32_t* coi, int32_t* dor, int32_t* doi)
{
    *cor = ar;
    *coi = -ai;
    *dor = -br;
    *doi = -bi;
}

/* --------------------------------------------------------------------------
 * Haar-Vorschlag: U -> R*U mit R auf SU(2).
 *
 *   R = exp(-i*(alpha/2)*n.sigma)   in Quaternion-Form:
 *     a = cos(alpha/2) - i*sin(alpha/2)*nz
 *     b = -sin(alpha/2)*(ny + i*nx)
 * ----------------------------------------------------------------------- */

static void mp2d_haar_propose(prng_state_t* rng, double eps,
    int32_t* ar, int32_t* ai,
    int32_t* br, int32_t* bi)
{
    const double u = 2.0 * mp2d_uniform(rng) - 1.0;
    const double phi = 2.0 * M_PI * mp2d_uniform(rng);
    const double s = sqrt(1.0 - u * u);
    const double nx = s * cos(phi);
    const double ny = s * sin(phi);
    const double nz = u;

    const double alpha = mp2d_gauss(mp2d_uniform(rng), mp2d_uniform(rng)) * eps;

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

    *ar = (int32_t)mp2d_rs_q30(na_re);
    *ai = (int32_t)mp2d_rs_q30(na_im);
    *br = (int32_t)mp2d_rs_q30(nb_re);
    *bi = (int32_t)mp2d_rs_q30(nb_im);
}

/* --------------------------------------------------------------------------
 * Plaquette-Action
 * ----------------------------------------------------------------------- */

static double mp2d_plaq_action_at(
    const ProUniverse* pu, uint32_t x, uint32_t y)
{
    const uint32_t mask = pu->grid_dim_mask;
    const uint32_t shift = pu->grid_dim_shift;

    const uint32_t xp = (x + 1u) & mask;
    const uint32_t yp = (y + 1u) & mask;

    const uint64_t idx_xy = (uint64_t)x | (((uint64_t)y) << shift);
    const uint64_t idx_xpy = (uint64_t)xp | (((uint64_t)y) << shift);
    const uint64_t idx_xyp = (uint64_t)x | (((uint64_t)yp) << shift);

    if (idx_xy >= pu->total_nodes) return 0.0;
    if (idx_xpy >= pu->total_nodes) return 0.0;
    if (idx_xyp >= pu->total_nodes) return 0.0;

    const ProEdge* e1 = &pu->edge_phases[idx_xy * CHANNELS_MAX + PRO_NEIGHBOR_X_PLUS];
    const ProEdge* e2 = &pu->edge_phases[idx_xpy * CHANNELS_MAX + PRO_NEIGHBOR_Y_PLUS];
    const ProEdge* e3 = &pu->edge_phases[idx_xyp * CHANNELS_MAX + PRO_NEIGHBOR_X_PLUS];
    const ProEdge* e4 = &pu->edge_phases[idx_xy * CHANNELS_MAX + PRO_NEIGHBOR_Y_PLUS];

    int32_t t1_ar, t1_ai, t1_br, t1_bi;
    int32_t t2_ar, t2_ai, t2_br, t2_bi;
    int32_t w_ar, w_ai, w_br, w_bi;
    int32_t c3_ar, c3_ai, c3_br, c3_bi;
    int32_t c4_ar, c4_ai, c4_br, c4_bi;

    mp2d_quat_mul(e1->su2_a_re, e1->su2_a_im, e1->su2_b_re, e1->su2_b_im,
        e2->su2_a_re, e2->su2_a_im, e2->su2_b_re, e2->su2_b_im,
        &t1_ar, &t1_ai, &t1_br, &t1_bi);

    mp2d_quat_conj(e3->su2_a_re, e3->su2_a_im, e3->su2_b_re, e3->su2_b_im,
        &c3_ar, &c3_ai, &c3_br, &c3_bi);
    mp2d_quat_mul(t1_ar, t1_ai, t1_br, t1_bi,
        c3_ar, c3_ai, c3_br, c3_bi,
        &t2_ar, &t2_ai, &t2_br, &t2_bi);

    mp2d_quat_conj(e4->su2_a_re, e4->su2_a_im, e4->su2_b_re, e4->su2_b_im,
        &c4_ar, &c4_ai, &c4_br, &c4_bi);
    mp2d_quat_mul(t2_ar, t2_ai, t2_br, t2_bi,
        c4_ar, c4_ai, c4_br, c4_bi,
        &w_ar, &w_ai, &w_br, &w_bi);
    (void)w_ai; (void)w_br; (void)w_bi;

    return 1.0 - (double)w_ar / (double)PRO_SU2_SCALE;
}

static double mp2d_link_plaq_sum(
    const ProUniverse* pu, uint32_t x, uint32_t y, uint8_t mu)
{
    const uint32_t mask = pu->grid_dim_mask;
    double sum = 0.0;

    if (mu == PRO_NEIGHBOR_X_PLUS) {
        sum += mp2d_plaq_action_at(pu, x, y);
        sum += mp2d_plaq_action_at(pu, x, (y + mask) & mask);
    }
    else if (mu == PRO_NEIGHBOR_Y_PLUS) {
        sum += mp2d_plaq_action_at(pu, x, y);
        sum += mp2d_plaq_action_at(pu, (x + mask) & mask, y);
    }
    return sum;
}

/* --------------------------------------------------------------------------
 * Randomisierung
 * ----------------------------------------------------------------------- */

static void mp2d_randomize_links(ProUniverse* pu, prng_state_t* rng)
{
    const uint32_t dim = pu->grid_dim;
    const uint32_t shift = pu->grid_dim_shift;

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = (uint64_t)x | (((uint64_t)y) << shift);

            for (int d = 0; d < 2; ++d) {
                const uint8_t mu = (d == 0) ? PRO_NEIGHBOR_X_PLUS
                    : PRO_NEIGHBOR_Y_PLUS;

                const double g1 = mp2d_gauss(mp2d_uniform(rng), mp2d_uniform(rng));
                const double g2 = mp2d_gauss(mp2d_uniform(rng), mp2d_uniform(rng));
                const double g3 = mp2d_gauss(mp2d_uniform(rng), mp2d_uniform(rng));
                const double g4 = mp2d_gauss(mp2d_uniform(rng), mp2d_uniform(rng));

                double nrm = sqrt(g1 * g1 + g2 * g2 + g3 * g3 + g4 * g4);
                if (nrm < 1e-30) nrm = 1.0;

                const int32_t ar = (int32_t)llround((g1 / nrm) * (double)PRO_SU2_SCALE);
                const int32_t ai = (int32_t)llround((g2 / nrm) * (double)PRO_SU2_SCALE);
                const int32_t br = (int32_t)llround((g3 / nrm) * (double)PRO_SU2_SCALE);
                const int32_t bi = (int32_t)llround((g4 / nrm) * (double)PRO_SU2_SCALE);

                ProPhysics_Set_Edge_SU2(pu, idx, mu, ar, ai, br, bi);
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Metropolis-Sweep (Haar-Vorschlag)
 * ----------------------------------------------------------------------- */

static double mp2d_metropolis_sweep(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng)
{
    const uint32_t dim = pu->grid_dim;
    const uint32_t shift = pu->grid_dim_shift;
    uint64_t accept = 0, total = 0;

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = (uint64_t)x | (((uint64_t)y) << shift);

            for (int d = 0; d < 2; ++d) {
                const uint8_t mu = (d == 0) ? PRO_NEIGHBOR_X_PLUS
                    : PRO_NEIGHBOR_Y_PLUS;

                int32_t ar, ai, br, bi;
                ProPhysics_Get_Edge_SU2(pu, idx, mu, &ar, &ai, &br, &bi);

                const double S_before = mp2d_link_plaq_sum(pu, x, y, mu);

                int32_t na = ar, nai = ai, nb = br, nbi = bi;
                mp2d_haar_propose(rng, eps, &na, &nai, &nb, &nbi);

                ProPhysics_Set_Edge_SU2(pu, idx, mu, na, nai, nb, nbi);

                const double S_after = mp2d_link_plaq_sum(pu, x, y, mu);

                const double dS = S_after - S_before;
                const double u = mp2d_uniform(rng);

                const bool accept_it = (dS <= 0.0) || (u < exp(-beta * dS));
                if (accept_it) accept++;
                else ProPhysics_Set_Edge_SU2(pu, idx, mu, ar, ai, br, bi);
                total++;
            }
        }
    }
    return (total > 0u) ? (double)accept / (double)total : 0.0;
}

/* --------------------------------------------------------------------------
 * Thermalisierung
 * ----------------------------------------------------------------------- */

static double mp2d_thermalize(ProUniverse* pu, double beta, prng_state_t* rng)
{
    double eps = 0.3;
    for (int pass = 0; pass < 6; ++pass) {
        double rate = 0.0;
        for (int s = 0; s < 10; ++s) rate += mp2d_metropolis_sweep(pu, beta, eps, rng);
        rate /= 10.0;
        if (rate < 0.30) eps *= 0.6;
        else if (rate > 0.70) eps *= 1.5;
        else break;
        if (eps < 1e-4) eps = 1e-4;
        if (eps > 1.5)  eps = 1.5;
    }
    for (uint32_t s = 0; s < MP2D_N_THERMAL; ++s)
        (void)mp2d_metropolis_sweep(pu, beta, eps, rng);
    return eps;
}

/* --------------------------------------------------------------------------
 * Messphase
 * ----------------------------------------------------------------------- */

static void mp2d_measure(ProUniverse* pu, double beta, double eps,
    prng_state_t* rng, double* out_W, double* out_W_err, double* out_acc)
{
    const uint32_t dim = pu->grid_dim;
    const double inv_N_plaq = 1.0 / ((double)dim * (double)dim);

    double bin[MP2D_N_BINS];
    for (uint32_t b = 0; b < MP2D_N_BINS; ++b) bin[b] = 0.0;

    double acc_total = 0.0;

    for (uint32_t s = 0; s < MP2D_N_MEASURE; ++s) {
        const double rate = mp2d_metropolis_sweep(pu, beta, eps, rng);
        acc_total += rate;

        double W_sum = 0.0;
        for (uint32_t y = 0; y < dim; ++y) {
            for (uint32_t x = 0; x < dim; ++x) {
                const double S_plaq = mp2d_plaq_action_at(pu, x, y);
                W_sum += 1.0 - S_plaq;
            }
        }
        const double W = W_sum * inv_N_plaq;
        const uint32_t bin_idx = s / MP2D_BIN_SIZE;
        if (bin_idx < MP2D_N_BINS) bin[bin_idx] += W;
    }

    double mean = 0.0;
    for (uint32_t b = 0; b < MP2D_N_BINS; ++b) {
        bin[b] /= (double)MP2D_BIN_SIZE;
        mean += bin[b];
    }
    mean /= (double)MP2D_N_BINS;

    double var = 0.0;
    for (uint32_t b = 0; b < MP2D_N_BINS; ++b) {
        const double d = bin[b] - mean;
        var += d * d;
    }
    var /= (double)(MP2D_N_BINS - 1u);
    const double err = sqrt(var / (double)MP2D_N_BINS);

    *out_W = mean;
    *out_W_err = err;
    *out_acc = acc_total / (double)MP2D_N_MEASURE;
}

/* --------------------------------------------------------------------------
 * Ein Lauf pro (dim, beta)
 * ----------------------------------------------------------------------- */

typedef struct {
    uint32_t dim;
    double beta;
    double W_meas;
    double W_err;
    double W_theo;
    double rel_dev;
    double accept;
    double eps;
} MP2DResult;

static bool mp2d_run_one(uint32_t dim, double beta, MP2DResult* out)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, (uint64_t)dim * (uint64_t)dim);
    if (!pu.edge_phases || !pu.reg_source) {
        ProPhysics_Free(&pu);
        return false;
    }

    mp2d_wire_torus_2d(&pu, dim);

    prng_state_t rng;
    const uint64_t seed = 0x2D2D0000ULL
        + ((uint64_t)dim << 16)
        + (uint64_t)llround(beta * 1000.0);
    prng_seed(&rng, seed);

    mp2d_randomize_links(&pu, &rng);
    const double eps = mp2d_thermalize(&pu, beta, &rng);

    double W, W_err, acc;
    mp2d_measure(&pu, beta, eps, &rng, &W, &W_err, &acc);

    /* KORREKTE REFERENZ fuer 2D SU(2): <(1/2) Re Tr U_plaq> = I2(beta)/I1(beta). */
    const double i1 = mp2d_bessel_i1(beta);
    const double i2 = mp2d_bessel_i2(beta);
    const double W_theo = i2 / i1;
    const double rel_dev = fabs(W - W_theo) / fabs(W_theo);

    out->dim = dim;
    out->beta = beta;
    out->W_meas = W;
    out->W_err = W_err;
    out->W_theo = W_theo;
    out->rel_dev = rel_dev;
    out->accept = acc;
    out->eps = eps;

    ProPhysics_Free(&pu);
    return true;
}

/* ==========================================================================
 * Diagnose-Block (unveraendert)
 * ========================================================================== */

static void mp2d_diagnose(void)
{
    printf("[DIAG] ==============================================================\n");
    printf("[DIAG]  2D-Metropolis Sanity-Check (bit-interleaved)\n");
    printf("[DIAG] ==============================================================\n\n");

    const uint32_t dim = 4u;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, (uint64_t)dim * (uint64_t)dim);
    mp2d_wire_torus_2d(&pu, dim);

    printf("[DIAG] D) grid_dim / shift / mask:\n");
    printf("[DIAG]   grid_dim       = %u\n", pu.grid_dim);
    printf("[DIAG]   grid_dim_shift = %u (Erwartung: 2)\n", pu.grid_dim_shift);
    printf("[DIAG]   grid_dim_mask  = %u (Erwartung: 3)\n", pu.grid_dim_mask);
    printf("[DIAG]   grid_ndim      = %u\n", pu.grid_ndim);
    printf("[DIAG]   total_nodes    = %llu\n\n",
        (unsigned long long)pu.total_nodes);

    printf("[DIAG] A) Topologie (bit-interleaved, dim=4, shift=2):\n");
    printf("[DIAG]   Erwartet: node 0=(x0,y0) -> +x=1, -x=3, +y=4, -y=12\n");
    printf("[DIAG]             node 1=(x1,y0) -> +x=2, -x=0, +y=5, -y=13\n");
    printf("[DIAG]             node 4=(x0,y1) -> +x=5, -x=7, +y=8, -y=0\n");
    for (uint64_t k = 0; k < 6u; ++k) {
        printf("[DIAG]   node %llu: +x=%llu  -x=%llu  +y=%llu  -y=%llu\n",
            (unsigned long long)k,
            (unsigned long long)pu.reg_source[k].channels[PRO_NEIGHBOR_X_PLUS],
            (unsigned long long)pu.reg_source[k].channels[PRO_NEIGHBOR_X_MINUS],
            (unsigned long long)pu.reg_source[k].channels[PRO_NEIGHBOR_Y_PLUS],
            (unsigned long long)pu.reg_source[k].channels[PRO_NEIGHBOR_Y_MINUS]);
    }
    printf("\n");

    printf("[DIAG] B) Link-Roundtrip:\n");
    {
        ProPhysics_Set_Edge_SU2(&pu, 0u, 0u,
            (int32_t)(0.6 * (double)PRO_SU2_SCALE), 0,
            (int32_t)(0.8 * (double)PRO_SU2_SCALE), 0);
        int32_t gar, gai, gbr, gbi;
        ProPhysics_Get_Edge_SU2(&pu, 0u, 0u, &gar, &gai, &gbr, &gbi);
        const double n2 = (double)gar * gar + (double)gai * gai
            + (double)gbr * gbr + (double)gbi * gbi;
        printf("[DIAG]   Set a=0.6, b=0.8 -> Get: a=(%d,%d) b=(%d,%d)\n",
            gar, gai, gbr, gbi);
        printf("[DIAG]   n2/2^60 = %.6f (Erwartung: 1.0)\n",
            n2 / 1152921504606846976.0);
    }
    printf("\n");

    printf("[DIAG] C) Plaquette-Berechnung:\n");
    {
        for (uint64_t k = 0; k < pu.total_nodes; ++k) {
            ProPhysics_Set_Edge_SU2(&pu, k, PRO_NEIGHBOR_X_PLUS,
                PRO_SU2_IDENT_RE, 0, 0, 0);
            ProPhysics_Set_Edge_SU2(&pu, k, PRO_NEIGHBOR_Y_PLUS,
                PRO_SU2_IDENT_RE, 0, 0, 0);
        }

        const double S_id = mp2d_plaq_action_at(&pu, 0u, 0u);
        printf("[DIAG]   Alle Links = Identitaet -> S_plaq(0,0) = %.6f (Erwartung: 0.0)\n",
            S_id);

        ProPhysics_Set_Edge_SU2(&pu, 0u, PRO_NEIGHBOR_X_PLUS, 0, 0, PRO_SU2_SCALE, 0);
        const double S_pert = mp2d_plaq_action_at(&pu, 0u, 0u);
        printf("[DIAG]   Nach Stoerung U_x(0,0) -> a=0,b=1: S_plaq(0,0) = %.6f (Erwartung: != 0)\n",
            S_pert);

        ProPhysics_Set_Edge_SU2(&pu, 0u, PRO_NEIGHBOR_X_PLUS,
            PRO_SU2_IDENT_RE, 0, 0, 0);

        const double S0 = mp2d_link_plaq_sum(&pu, 0u, 0u, PRO_NEIGHBOR_X_PLUS);
        ProPhysics_Set_Edge_SU2(&pu, 0u, PRO_NEIGHBOR_X_PLUS, 0, 0, PRO_SU2_SCALE, 0);
        const double S1 = mp2d_link_plaq_sum(&pu, 0u, 0u, PRO_NEIGHBOR_X_PLUS);
        printf("[DIAG]   Link-Sum Vorher:  %.6f (Erwartung: 0.0)\n", S0);
        printf("[DIAG]   Link-Sum Nachher: %.6f (Erwartung: != 0)\n", S1);
        printf("[DIAG]   dS = %+.6f (zwei Plaquettes betroffen)\n", S1 - S0);
    }
    printf("\n");

    ProPhysics_Free(&pu);
    printf("[DIAG] ==============================================================\n\n");
}

/* ==========================================================================
 * Haupttest
 * ========================================================================== */

bool test_metropolis_2d(bool full_dims)
{
    printf("========================================================================\n");
    printf("  Etappe 23e: 2D SU(2) Metropolis vs I2(beta)/I1(beta)\n");
    printf("  Referenz-Korrektur: I2/I1 (SU(2)) statt I1/I0 (U(1))\n");
    printf("  Sampler: Haar-Vorschlag U -> R*U\n");
    printf("========================================================================\n\n");

    mp2d_diagnose();

    printf("[MP2D] Exakte 2D-SU(2)-Referenz: <W_plaq> = I2(beta)/I1(beta).\n");
    printf("[MP2D]   (Haar-Mass sin^2(alpha) erzeugt I2/I1, nicht I1/I0.)\n");
    printf("[MP2D] Modus: %s\n", full_dims ? "FULL (dim=32)" : "FAST (dim=16)");
    printf("[MP2D] Thermalisierung: %u Sweeps, Messung: %u Sweeps, Bins: %u.\n\n",
        (unsigned)MP2D_N_THERMAL, (unsigned)MP2D_N_MEASURE, (unsigned)MP2D_N_BINS);

    const uint32_t dim = full_dims ? MP2D_DIM_FULL : MP2D_DIM_FAST;
    static const double betas[] = { 0.5, 1.0, 2.0, 4.0 };
    const int n_betas = 4;

    printf("[MP2D] dim = %u (N = %llu Knoten)\n",
        dim, (unsigned long long)((uint64_t)dim * (uint64_t)dim));
    printf("[MP2D] --------------------------------------------------------------\n");
    printf("[MP2D]   beta   eps     accept    <W>_meas          I2/I1 (theo)   rel_dev    Status\n");
    printf("[MP2D]   ------------------------------------------------------------------------\n");

    MP2DResult results[8];
    int n_results = 0;
    bool all_ok = true;

    for (int b = 0; b < n_betas; ++b) {
        MP2DResult r;
        if (!mp2d_run_one(dim, betas[b], &r)) {
            printf("[MP2D]   %.2f   ---     ---       (Lauf fehlgeschlagen)\n", betas[b]);
            all_ok = false;
            continue;
        }
        results[n_results++] = r;

        const char* status;
        if (r.rel_dev < 0.005)      status = "PASS";
        else if (r.rel_dev < 0.02)  status = "PASS(Hinweis)";
        else if (r.rel_dev < 0.10)  status = "WARN";
        else { status = "FAIL"; all_ok = false; }

        printf("[MP2D]   %.2f   %.3f   %.3f     %.6f+-%.6f   %.6f       %.5f    %s\n",
            r.beta, r.eps, r.accept,
            r.W_meas, r.W_err, r.W_theo, r.rel_dev, status);
    }
    printf("[MP2D]   ------------------------------------------------------------------------\n\n");

    printf("[MP2D] ==============================================================\n");
    printf("[MP2D] Zusammenfassung\n");
    printf("[MP2D] ==============================================================\n");
    printf("[MP2D]   beta    <W>_meas +- err         I2/I1 (theo)     rel_dev\n");
    printf("[MP2D]   ----------------------------------------------------------\n");
    for (int k = 0; k < n_results; ++k) {
        printf("[MP2D]   %.2f    %.6f +- %.6f      %.6f         %.5f\n",
            results[k].beta,
            results[k].W_meas, results[k].W_err,
            results[k].W_theo,
            results[k].rel_dev);
    }
    printf("\n");

    printf("[MP2D] -> %s\n", all_ok ? "PASSED" : "FAILED");
    return all_ok;
}