/* ==========================================================================
 * alpha_test_hydrogen.c
 *
 * Etappe 18  : Coulomb/Hydrogen bei dim=32 (Original, Gauss-Praep., moderate Kopplung).
 * Etappe 18b : Coulomb/Hydrogen bei dim=32 mit Imaginaerzeit-Praeparation.
 * Etappe 18d : Coulomb/Hydrogen bei dim=64 mit Imaginaerzeit-Praeparation.
 *
 * Methode: Autokorrelations-Spektroskopie.
 *
 * Imaginaerzeit-Evolution:
 *   Gauss als Startpunkt. Dann abwechselnd:
 *     1. Diffusion:  psi <- psi + dt_kin * (sum_nb - 6*psi)
 *     2. Amplitude:  psi <- psi * exp(dt_pot * strength / r_eff)
 *     3. Normierung
 *
 *   Zwei Parametersaetze:
 *
 *   Etappe 18b (dim=32):
 *     dt_kin = 1e-2, dt_pot = 1e-6, n_iter = 1000.
 *     L_diff = sqrt(2*dt_kin*n_iter) = 4.47.
 *     Wasserstoff rel_dev(1/n^2) = 0.139 bei strength=16000 (bester Wert).
 *
 *   Etappe 18d (dim=64):
 *     dt_kin = 1e-2, dt_pot = 2e-7, n_iter = 1000.
 *     L_diff/a(32000) = 4.47/3.125 = 1.43.
 *     Ziel: rel_dev(1/n^2) < 0.05.
 *
 *   Erwartete Grundzustandsradien mit 18d:
 *     strength=16000 -> a=6.25, <r^2> ~ 100-200
 *     strength=32000 -> a=3.13, <r^2> ~ 30-60
 *     strength=48000 -> a=2.08, <r^2> ~ 15-30
 *
 * F6-Architektur (Etappe 18d):
 *   Die Imaginaerzeit-Parameter (dt_kin, dt_pot, n_iter) sind jetzt ueber
 *   `run_hydrogen_single_ex` und `test_hydrogen_spectrum_impl_ex`
 *   konfigurierbar. Die alte API (`run_hydrogen_single`,
 *   `test_hydrogen_spectrum_impl`) bleibt erhalten und ruft die neuen
 *   Funktionen mit den 18b-Defaults auf. Damit ist R5 (keine stillen
 *   API-Brueche) gewahrt, und mehrere Etappen koennen parallel mit
 *   unterschiedlichen Parametern laufen.
 *
 * WICHTIG: dim MUSS eine Zweierpotenz sein. 48 ist verboten.
 *          Grund: wire_torus_3d (R1-Bit-Interleaved-Index) und der
 *          Shift/Mask-Hotpath setzen Zweierpotenzen voraus.
 *
 * Kein <complex.h> (MSVC). Eigener Cx-Struct.
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* Lokale Kopie der 3D-Nachbarschafts-Reihenfolge. */
static const uint8_t HYDRO_NB_CHANNELS[6] = {
    0u, 1u, 2u, 3u, 4u, 6u
};

/* --------------------------------------------------------------------------
 * Kleine Complex-Arithmetik.
 * -------------------------------------------------------------------------- */
typedef struct { double re, im; } Cx;

static Cx cx(double re, double im) { Cx c; c.re = re; c.im = im; return c; }
static Cx cx_add(Cx a, Cx b) { return cx(a.re + b.re, a.im + b.im); }
static Cx cx_mul(Cx a, Cx b) {
    return cx(a.re * b.re - a.im * b.im,
        a.re * b.im + a.im * b.re);
}
static Cx cx_conj(Cx a) { return cx(a.re, -a.im); }
static double cx_abs2(Cx a) { return a.re * a.re + a.im * a.im; }

/* --------------------------------------------------------------------------
 * Radix-2-Cooley-Tukey FFT in-place (DIT).
 * -------------------------------------------------------------------------- */
static void fft_radix2(double* re, double* im, uint32_t N, int inverse)
{
    uint32_t j = 0;
    for (uint32_t i = 1; i < N; ++i) {
        uint32_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            double tr = re[i]; re[i] = re[j]; re[j] = tr;
            double ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }

    for (uint32_t len = 2; len <= N; len <<= 1) {
        const double ang = (inverse ? +1.0 : -1.0) * 2.0 * M_PI / (double)len;
        const double wlen_re = cos(ang);
        const double wlen_im = sin(ang);
        for (uint32_t i = 0; i < N; i += len) {
            double w_re = 1.0, w_im = 0.0;
            const uint32_t half = len >> 1;
            for (uint32_t k = 0; k < half; ++k) {
                const uint32_t u = i + k;
                const uint32_t v = i + k + half;
                const double vr = re[v] * w_re - im[v] * w_im;
                const double vi = re[v] * w_im + im[v] * w_re;
                re[v] = re[u] - vr;
                im[v] = im[u] - vi;
                re[u] += vr;
                im[u] += vi;
                const double nw_re = w_re * wlen_re - w_im * wlen_im;
                const double nw_im = w_re * wlen_im + w_im * wlen_re;
                w_re = nw_re;
                w_im = nw_im;
            }
        }
    }

    if (inverse) {
        const double inv_n = 1.0 / (double)N;
        for (uint32_t i = 0; i < N; ++i) {
            re[i] *= inv_n;
            im[i] *= inv_n;
        }
    }
}

/* --------------------------------------------------------------------------
 * Praeparation: Gauss-Paket.
 * -------------------------------------------------------------------------- */
static void prepare_gaussian_packet_3d(ProUniverse* pu,
    uint32_t dim, uint32_t shift,
    uint32_t cx, uint32_t cy, uint32_t cz,
    double sigma, uint8_t basis)
{
    const uint64_t N = (uint64_t)dim * dim * dim;
    const double inv_2s2 = 1.0 / (2.0 * sigma * sigma);
    const uint32_t mask = dim - 1u;

    double norm_sq = 0.0;
    for (uint64_t k = 0; k < N; ++k) {
        const uint32_t x = (uint32_t)k & mask;
        const uint32_t y = ((uint32_t)k >> shift) & mask;
        const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;
        int32_t dx = (int32_t)x - (int32_t)cx;
        if (dx > (int32_t)dim / 2) dx -= (int32_t)dim;
        if (dx < -(int32_t)dim / 2) dx += (int32_t)dim;
        int32_t dy = (int32_t)y - (int32_t)cy;
        if (dy > (int32_t)dim / 2) dy -= (int32_t)dim;
        if (dy < -(int32_t)dim / 2) dy += (int32_t)dim;
        int32_t dz = (int32_t)z - (int32_t)cz;
        if (dz > (int32_t)dim / 2) dz -= (int32_t)dim;
        if (dz < -(int32_t)dim / 2) dz += (int32_t)dim;
        const double r2 = (double)(dx * dx + dy * dy + dz * dz);
        const double g = exp(-r2 * inv_2s2);
        norm_sq += g * g;
    }
    const double inv_norm = (norm_sq > 0.0) ? (1.0 / sqrt(norm_sq)) : 0.0;

    for (uint64_t k = 0; k < N; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu->amp_grid[k].coeff[b] = 0;
        const uint32_t x = (uint32_t)k & mask;
        const uint32_t y = ((uint32_t)k >> shift) & mask;
        const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;
        int32_t dx = (int32_t)x - (int32_t)cx;
        if (dx > (int32_t)dim / 2) dx -= (int32_t)dim;
        if (dx < -(int32_t)dim / 2) dx += (int32_t)dim;
        int32_t dy = (int32_t)y - (int32_t)cy;
        if (dy > (int32_t)dim / 2) dy -= (int32_t)dim;
        if (dy < -(int32_t)dim / 2) dy += (int32_t)dim;
        int32_t dz = (int32_t)z - (int32_t)cz;
        if (dz > (int32_t)dim / 2) dz -= (int32_t)dim;
        if (dz < -(int32_t)dim / 2) dz += (int32_t)dim;
        const double r2 = (double)(dx * dx + dy * dy + dz * dz);
        const double g = exp(-r2 * inv_2s2) * inv_norm;
        const int32_t re_q = (int32_t)lround(g * Q31_MAXV);
        pu->amp_grid[k].coeff[basis] = pro_amp_pack(re_q, 0);
    }
}

/* --------------------------------------------------------------------------
 * Praeparation: wasserstoff-artiger Grundzustand (analytisch).
 * -------------------------------------------------------------------------- */
static void prepare_exponential_packet_3d(ProUniverse* pu,
    uint32_t dim, uint32_t shift,
    uint32_t cx, uint32_t cy, uint32_t cz,
    double a0, uint8_t basis)
{
    const uint64_t N = (uint64_t)dim * dim * dim;
    const uint32_t mask = dim - 1u;

    double norm_sq = 0.0;
    for (uint64_t k = 0; k < N; ++k) {
        const uint32_t x = (uint32_t)k & mask;
        const uint32_t y = ((uint32_t)k >> shift) & mask;
        const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;
        int32_t dx = (int32_t)x - (int32_t)cx;
        if (dx > (int32_t)dim / 2) dx -= (int32_t)dim;
        if (dx < -(int32_t)dim / 2) dx += (int32_t)dim;
        int32_t dy = (int32_t)y - (int32_t)cy;
        if (dy > (int32_t)dim / 2) dy -= (int32_t)dim;
        if (dy < -(int32_t)dim / 2) dy += (int32_t)dim;
        int32_t dz = (int32_t)z - (int32_t)cz;
        if (dz > (int32_t)dim / 2) dz -= (int32_t)dim;
        if (dz < -(int32_t)dim / 2) dz += (int32_t)dim;
        const double r = sqrt((double)(dx * dx + dy * dy + dz * dz));
        const double g = exp(-r / a0);
        norm_sq += g * g;
    }
    const double inv_norm = (norm_sq > 0.0) ? (1.0 / sqrt(norm_sq)) : 0.0;

    for (uint64_t k = 0; k < N; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu->amp_grid[k].coeff[b] = 0;
        const uint32_t x = (uint32_t)k & mask;
        const uint32_t y = ((uint32_t)k >> shift) & mask;
        const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;
        int32_t dx = (int32_t)x - (int32_t)cx;
        if (dx > (int32_t)dim / 2) dx -= (int32_t)dim;
        if (dx < -(int32_t)dim / 2) dx += (int32_t)dim;
        int32_t dy = (int32_t)y - (int32_t)cy;
        if (dy > (int32_t)dim / 2) dy -= (int32_t)dim;
        if (dy < -(int32_t)dim / 2) dy += (int32_t)dim;
        int32_t dz = (int32_t)z - (int32_t)cz;
        if (dz > (int32_t)dim / 2) dz -= (int32_t)dim;
        if (dz < -(int32_t)dim / 2) dz += (int32_t)dim;
        const double r = sqrt((double)(dx * dx + dy * dy + dz * dz));
        const double g = exp(-r / a0) * inv_norm;
        const int32_t re_q = (int32_t)lround(g * Q31_MAXV);
        pu->amp_grid[k].coeff[basis] = pro_amp_pack(re_q, 0);
    }
}

/* --------------------------------------------------------------------------
 * Imaginaerzeit-Evolution.
 *
 * Der Fix (Etappe 18b/18d) stellt sicher, dass die Diffusionslaenge
 *     L_diff = sqrt(2 * dt_kin * n_iter)
 * unter der erwarteten Lokalisierungslaenge
 *     a = 2*dt_kin / (dt_pot * strength)
 * bleibt. Sonst wird der gebundene Zustand durch Diffusion ausgewaschen.
 *
 * 18b (dim=32): dt_kin=1e-2, dt_pot=1e-6, n_iter=1000.
 * 18d (dim=64): dt_kin=1e-2, dt_pot=2e-7, n_iter=1000.
 * -------------------------------------------------------------------------- */
static void imaginary_time_evolve_3d(ProUniverse* pu,
    uint32_t dim, uint32_t shift,
    uint32_t cx, uint32_t cy, uint32_t cz,
    int32_t strength_q15, uint32_t softening_q8,
    uint32_t n_iter,
    double dt_kin, double dt_pot_base)
{
    const uint64_t N = pu->total_nodes;
    const uint32_t mask = dim - 1u;

    const double dt_pot = dt_pot_base;

    double* psi = (double*)malloc(N * sizeof(double));
    double* psi_new = (double*)malloc(N * sizeof(double));
    if (!psi || !psi_new) { free(psi); free(psi_new); return; }

    for (uint64_t k = 0; k < N; ++k) {
        const int32_t re = pro_amp_real(pu->amp_grid[k].coeff[UR_POSITRON_CW]);
        psi[k] = (double)re / (double)Q31_MAXV;
    }

    const double soft_r = (double)softening_q8 / 256.0;

    for (uint32_t iter = 0; iter < n_iter; ++iter) {
        /* 1. Diffusion (Kinetik). */
        for (uint64_t k = 0; k < N; ++k) {
            const ProRegister* r = &pu->reg_source[k];
            double sum_nb = 0.0;
            for (int i = 0; i < 6; ++i) {
                const uint64_t nb = r->channels[HYDRO_NB_CHANNELS[i]];
                if (nb >= N || nb == k) continue;
                sum_nb += psi[nb];
            }
            psi_new[k] = psi[k] + dt_kin * (sum_nb - 6.0 * psi[k]);
        }
        for (uint64_t k = 0; k < N; ++k) psi[k] = psi_new[k];

        /* 2. Potential (Amplifikation). */
        if (strength_q15 != 0) {
            for (uint64_t k = 0; k < N; ++k) {
                const uint32_t x = (uint32_t)k & mask;
                const uint32_t y = ((uint32_t)k >> shift) & mask;
                const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;
                int32_t dx = (int32_t)x - (int32_t)cx;
                if (dx > (int32_t)dim / 2) dx -= (int32_t)dim;
                if (dx < -(int32_t)dim / 2) dx += (int32_t)dim;
                int32_t dy = (int32_t)y - (int32_t)cy;
                if (dy > (int32_t)dim / 2) dy -= (int32_t)dim;
                if (dy < -(int32_t)dim / 2) dy += (int32_t)dim;
                int32_t dz = (int32_t)z - (int32_t)cz;
                if (dz > (int32_t)dim / 2) dz -= (int32_t)dim;
                if (dz < -(int32_t)dim / 2) dz += (int32_t)dim;
                const double r = sqrt((double)(dx * dx + dy * dy + dz * dz));
                const double r_eff = r + soft_r;
                const double exponent = dt_pot * (double)strength_q15 / r_eff;
                const double e_capped = (exponent > 20.0) ? 20.0 : exponent;
                psi[k] *= exp(e_capped);
            }
        }

        /* 3. Normierung. */
        double norm = 0.0;
        for (uint64_t k = 0; k < N; ++k) norm += psi[k] * psi[k];
        if (norm < 1e-30) break;
        const double inv = 1.0 / sqrt(norm);
        for (uint64_t k = 0; k < N; ++k) psi[k] *= inv;
    }

    for (uint64_t k = 0; k < N; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu->amp_grid[k].coeff[b] = 0;
        int64_t re_q = (int64_t)llround(psi[k] * Q31_MAXV);
        if (re_q > INT32_MAX) re_q = INT32_MAX;
        if (re_q < INT32_MIN) re_q = INT32_MIN;
        pu->amp_grid[k].coeff[UR_POSITRON_CW] =
            pro_amp_pack((int32_t)re_q, 0);
    }

    free(psi);
    free(psi_new);
}

/* --------------------------------------------------------------------------
 * Praeparations-Arten.
 * -------------------------------------------------------------------------- */
typedef enum {
    HYDRO_PREP_GAUSS = 0,
    HYDRO_PREP_EXP = 1,
    HYDRO_PREP_GROUND = 2
} HydrogenPrepKind;

static void prepare_hydrogen_packet_3d(ProUniverse* pu,
    uint32_t dim, uint32_t shift,
    uint32_t cx, uint32_t cy, uint32_t cz,
    double shape_param, HydrogenPrepKind prep_kind, uint8_t basis)
{
    if (prep_kind == HYDRO_PREP_EXP) {
        prepare_exponential_packet_3d(pu, dim, shift, cx, cy, cz,
            shape_param, basis);
    }
    else {
        prepare_gaussian_packet_3d(pu, dim, shift, cx, cy, cz,
            shape_param, basis);
    }
}

/* --------------------------------------------------------------------------
 * Autokorrelation C(t) = <phi(0)|phi(t)>.
 * -------------------------------------------------------------------------- */
static Cx autocorrelation_at(const ProUniverse* pu,
    const ProAmpVector* psi0, uint64_t N)
{
    Cx c = cx(0.0, 0.0);
    for (uint64_t k = 0; k < N; ++k) {
        const double a_re = (double)pro_amp_real(psi0[k].coeff[UR_POSITRON_CW]);
        const double a_im = (double)pro_amp_imag(psi0[k].coeff[UR_POSITRON_CW]);
        const double b_re = (double)pro_amp_real(pu->amp_grid[k].coeff[UR_POSITRON_CW]);
        const double b_im = (double)pro_amp_imag(pu->amp_grid[k].coeff[UR_POSITRON_CW]);
        const double dre = a_re * b_re + a_im * b_im;
        const double dim = a_re * b_im - a_im * b_re;
        c.re += dre;
        c.im += dim;
    }
    return c;
}

/* --------------------------------------------------------------------------
 * Mittleres <r^2>.
 * -------------------------------------------------------------------------- */
static double compute_r2_moment(const ProUniverse* pu,
    uint32_t dim, uint32_t shift,
    uint32_t cx, uint32_t cy, uint32_t cz)
{
    const uint64_t N = pu->total_nodes;
    const uint32_t mask = dim - 1u;

    double sum_a2 = 0.0;
    double sum_a2_r2 = 0.0;
    for (uint64_t k = 0; k < N; ++k) {
        const uint32_t x = (uint32_t)k & mask;
        const uint32_t y = ((uint32_t)k >> shift) & mask;
        const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;

        int32_t dx = (int32_t)x - (int32_t)cx;
        if (dx > (int32_t)dim / 2) dx -= (int32_t)dim;
        if (dx < -(int32_t)dim / 2) dx += (int32_t)dim;
        int32_t dy = (int32_t)y - (int32_t)cy;
        if (dy > (int32_t)dim / 2) dy -= (int32_t)dim;
        if (dy < -(int32_t)dim / 2) dy += (int32_t)dim;
        int32_t dz = (int32_t)z - (int32_t)cz;
        if (dz > (int32_t)dim / 2) dz -= (int32_t)dim;
        if (dz < -(int32_t)dim / 2) dz += (int32_t)dim;

        const double r2 = (double)(dx * dx + dy * dy + dz * dz);
        const int32_t re = pro_amp_real(pu->amp_grid[k].coeff[UR_POSITRON_CW]);
        const int32_t im = pro_amp_imag(pu->amp_grid[k].coeff[UR_POSITRON_CW]);
        const double a2 = (double)re * (double)re + (double)im * (double)im;

        sum_a2 += a2;
        sum_a2_r2 += a2 * r2;
    }
    return (sum_a2 > 0.0) ? (sum_a2_r2 / sum_a2) : 0.0;
}

/* --------------------------------------------------------------------------
 * Spektrum.
 * -------------------------------------------------------------------------- */
static uint32_t compute_spectrum(const Cx* C, uint32_t T, uint32_t pad_factor,
    double* out_spec_re, double* out_spec_im)
{
    uint32_t N_pad = 1u;
    while (N_pad < T * pad_factor) N_pad <<= 1;

    for (uint32_t i = 0; i < N_pad; ++i) {
        out_spec_re[i] = 0.0;
        out_spec_im[i] = 0.0;
    }
    for (uint32_t i = 0; i < T; ++i) {
        const double w = 0.5 * (1.0 - cos(2.0 * M_PI * (double)i / (double)(T - 1)));
        out_spec_re[i] = C[i].re * w;
        out_spec_im[i] = C[i].im * w;
    }

    fft_radix2(out_spec_re, out_spec_im, N_pad, 0);
    return N_pad;
}

/* --------------------------------------------------------------------------
 * Peak-Suche.
 * -------------------------------------------------------------------------- */
#define MAX_PEAKS 8

typedef struct {
    uint32_t bin;
    double   omega;
    double   amp;
} Peak;

static int find_peaks(const double* spec_re, const double* spec_im,
    uint32_t N_pad, double omega_max_bin,
    Peak* out_peaks, uint32_t max_peaks)
{
    const uint32_t search_limit = (uint32_t)omega_max_bin;
    if (search_limit < 4u) return 0;

    double* amp = (double*)malloc(search_limit * sizeof(double));
    if (!amp) return 0;
    for (uint32_t i = 0; i < search_limit; ++i) {
        amp[i] = spec_re[i] * spec_re[i] + spec_im[i] * spec_im[i];
    }

    Peak candidates[MAX_PEAKS];
    uint32_t n_cand = 0u;

    for (uint32_t i = 2u; i + 2u < search_limit; ++i) {
        const double a = amp[i];
        if (a <= amp[i - 2] || a <= amp[i - 1]) continue;
        if (a <= amp[i + 1] || a <= amp[i + 2]) continue;

        if (n_cand < MAX_PEAKS) {
            candidates[n_cand].bin = i;
            candidates[n_cand].omega = (double)i * 2.0 * M_PI / (double)N_pad;
            candidates[n_cand].amp = a;
            n_cand++;
        }
        else {
            uint32_t min_idx = 0;
            double min_amp = candidates[0].amp;
            for (uint32_t k = 1; k < MAX_PEAKS; ++k) {
                if (candidates[k].amp < min_amp) {
                    min_amp = candidates[k].amp;
                    min_idx = k;
                }
            }
            if (a > min_amp) {
                candidates[min_idx].bin = i;
                candidates[min_idx].omega = (double)i * 2.0 * M_PI / (double)N_pad;
                candidates[min_idx].amp = a;
            }
        }
    }

    for (uint32_t i = 0; i < n_cand; ++i) {
        for (uint32_t j = i + 1; j < n_cand; ++j) {
            if (candidates[j].amp > candidates[i].amp) {
                Peak tmp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = tmp;
            }
        }
    }

    const uint32_t n_out = (n_cand < max_peaks) ? n_cand : max_peaks;
    for (uint32_t k = 0; k < n_out; ++k) out_peaks[k] = candidates[k];

    free(amp);
    return (int)n_out;
}

/* ==========================================================================
 * Ein Lauf — zwei Varianten.
 *
 * run_hydrogen_single_ex ist die neue, parametrisierte Version.
 * run_hydrogen_single bleibt als API-erhaltender Wrapper mit den
 * 18b-Defaults (dt_kin=1e-2, dt_pot=1e-6, n_iter=1000) erhalten.
 * ========================================================================== */

static int run_hydrogen_single_ex(uint32_t dim,
    int32_t strength_q15, uint32_t softening_q8,
    double shape_param, HydrogenPrepKind prep_kind,
    uint32_t n_ticks, Cx* out_C,
    double imag_dt_kin, double imag_dt_pot, uint32_t imag_n_iter)
{
    const uint64_t N = (uint64_t)dim * dim * dim;
    uint32_t shift = 0u;
    while ((1u << shift) < dim) shift++;
    const uint32_t cx = dim / 2u, cy = dim / 2u, cz = dim / 2u;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        ProPhysics_Free(&pu);
        return 0;
    }
    wire_torus_3d(&pu, dim);

    if (prep_kind == HYDRO_PREP_GROUND && strength_q15 != 0) {
        prepare_gaussian_packet_3d(&pu, dim, shift, cx, cy, cz,
            shape_param, UR_POSITRON_CW);
        imaginary_time_evolve_3d(&pu, dim, shift, cx, cy, cz,
            strength_q15, softening_q8,
            imag_n_iter, imag_dt_kin, imag_dt_pot);
    }
    else {
        prepare_hydrogen_packet_3d(&pu, dim, shift, cx, cy, cz,
            shape_param,
            (prep_kind == HYDRO_PREP_GROUND) ? HYDRO_PREP_GAUSS : prep_kind,
            UR_POSITRON_CW);
    }

    ProAmpVector* psi0 = (ProAmpVector*)malloc(N * sizeof(ProAmpVector));
    if (!psi0) { ProPhysics_Free(&pu); return 0; }
    memcpy(psi0, pu.amp_grid, N * sizeof(ProAmpVector));

    out_C[0] = autocorrelation_at(&pu, psi0, N);

    const uint32_t THETA_Q15 = 2000u;

    for (uint32_t t = 1; t < n_ticks; ++t) {
        ProPhysics_Apply_Edge_Transport_Colored_3D(&pu, THETA_Q15, dim);
        if (strength_q15 != 0) {
            ProPhysics_Apply_Coulomb_Phase_Field_3D(&pu,
                cx, cy, cz, strength_q15, softening_q8);
        }
        out_C[t] = autocorrelation_at(&pu, psi0, N);
    }

    free(psi0);
    ProPhysics_Free(&pu);
    return 1;
}

/* Wrapper mit 18b-Defaults. API-erhaltend. */
static int run_hydrogen_single(uint32_t dim,
    int32_t strength_q15, uint32_t softening_q8,
    double shape_param, HydrogenPrepKind prep_kind,
    uint32_t n_ticks, Cx* out_C)
{
    return run_hydrogen_single_ex(dim, strength_q15, softening_q8,
        shape_param, prep_kind, n_ticks, out_C,
        0.01,      /* imag_dt_kin (18b) */
        0.000001,  /* imag_dt_pot (18b) */
        1000u);    /* imag_n_iter (18b) */
}

/* --------------------------------------------------------------------------
 * 1/n^2-Check.
 * -------------------------------------------------------------------------- */
static double check_1_over_n2(const Peak* peaks, uint32_t n_peaks,
    double* out_ratio_21, double* out_ratio_31)
{
    if (n_peaks < 3u) return 1e300;

    Peak sorted[MAX_PEAKS];
    for (uint32_t i = 0; i < n_peaks; ++i) sorted[i] = peaks[i];
    for (uint32_t i = 0; i < n_peaks; ++i)
        for (uint32_t j = i + 1; j < n_peaks; ++j)
            if (sorted[j].omega < sorted[i].omega) {
                Peak tmp = sorted[i]; sorted[i] = sorted[j]; sorted[j] = tmp;
            }

    const double w1 = sorted[0].omega;
    const double w2 = sorted[1].omega;
    const double w3 = sorted[2].omega;

    const double d21 = w2 - w1;
    const double d31 = w3 - w1;

    if (fabs(d21) < 1e-9) return 1e300;

    *out_ratio_21 = d21 / d21;
    *out_ratio_31 = d31 / d21;

    const double expected = 32.0 / 27.0;
    return fabs(*out_ratio_31 - expected) / expected;
}

/* ==========================================================================
 * Gemeinsame Test-Implementierung — zwei Varianten.
 *
 * test_hydrogen_spectrum_impl_ex ist die neue, parametrisierte Version
 * (F6). test_hydrogen_spectrum_impl bleibt als Wrapper mit den 18b-Defaults
 * erhalten (API-Stabilitaet, R5).
 * ========================================================================== */
static bool test_hydrogen_spectrum_impl_ex(
    const char* label,
    uint32_t DIM, uint32_t N_TICKS, double shape_param,
    HydrogenPrepKind prep_kind,
    uint32_t softening_q8,
    double omega_cutoff,
    const int32_t* loc_strengths, uint32_t n_loc,
    const int32_t* strengths, uint32_t n_str,
    uint32_t pad_factor, uint32_t spec_buffer_size,
    double imag_dt_kin, double imag_dt_pot, uint32_t imag_n_iter)
{
    const uint64_t NODES = (uint64_t)DIM * DIM * DIM;

    const char* prep_name =
        (prep_kind == HYDRO_PREP_GROUND) ? "ground" :
        (prep_kind == HYDRO_PREP_EXP) ? "exp" : "gauss";

    printf("========================================================================\n");
    printf("  %s\n", label);
    printf("  Methode: Autokorrelations-Spektroskopie\n");
    printf("========================================================================\n\n");

    printf("[H] DIM=%u | NODES=%llu | N_TICKS=%u | shape=%.2f | prep=%s | SOF=%u | pad=%u | cutoff=%.2f\n",
        DIM, (unsigned long long)NODES, N_TICKS, shape_param, prep_name,
        softening_q8, pad_factor, omega_cutoff);
    printf("[H] Imaginaerzeit: dt_kin=%.3e | dt_pot=%.3e | n_iter=%u\n",
        imag_dt_kin, imag_dt_pot, imag_n_iter);
    printf("[H] L_diff = sqrt(2*dt_kin*n_iter) = %.3f\n",
        sqrt(2.0 * imag_dt_kin * (double)imag_n_iter));

    printf("\n[H] Lokalisierungs-Nachweis (<r^2>(t)):\n");
    printf("[H]   strength   <r^2>(0)    <r^2>(%u)  <r^2>(%u) <r^2>(%u)  Saettigung?\n",
        N_TICKS / 20u, N_TICKS / 4u, N_TICKS);
    printf("[H]   ------------------------------------------------------------------------\n");

    int best_loc_strength = 0;
    double best_loc_ratio = 1e300;

    for (uint32_t s = 0; s < n_loc; ++s) {
        const int32_t strength = loc_strengths[s];

        ProUniverse pu_loc;
        ProPhysics_Initialize(&pu_loc, NODES);
        if (!pu_loc.amp_grid || !pu_loc.amp_scratch || !pu_loc.reg_source) {
            ProPhysics_Free(&pu_loc);
            continue;
        }
        wire_torus_3d(&pu_loc, DIM);

        uint32_t shift_loc = 0u;
        while ((1u << shift_loc) < DIM) shift_loc++;
        const uint32_t cx = DIM / 2u, cy = DIM / 2u, cz = DIM / 2u;

        if (prep_kind == HYDRO_PREP_GROUND && strength != 0) {
            prepare_gaussian_packet_3d(&pu_loc, DIM, shift_loc,
                cx, cy, cz, shape_param, UR_POSITRON_CW);
            imaginary_time_evolve_3d(&pu_loc, DIM, shift_loc, cx, cy, cz,
                strength, softening_q8,
                imag_n_iter, imag_dt_kin, imag_dt_pot);
        }
        else {
            prepare_hydrogen_packet_3d(&pu_loc, DIM, shift_loc,
                cx, cy, cz, shape_param,
                (prep_kind == HYDRO_PREP_GROUND) ? HYDRO_PREP_GAUSS : prep_kind,
                UR_POSITRON_CW);
        }

        double r2_vals[4] = { 0.0, 0.0, 0.0, 0.0 };
        r2_vals[0] = compute_r2_moment(&pu_loc, DIM, shift_loc, cx, cy, cz);

        const uint32_t t_a = N_TICKS / 20u;
        const uint32_t t_b = N_TICKS / 4u;
        const uint32_t t_c = N_TICKS;

        for (uint32_t t = 1; t <= N_TICKS; ++t) {
            ProPhysics_Apply_Edge_Transport_Colored_3D(&pu_loc, 2000u, DIM);
            if (strength != 0) {
                ProPhysics_Apply_Coulomb_Phase_Field_3D(&pu_loc,
                    cx, cy, cz, strength, softening_q8);
            }
            if (t == t_a) r2_vals[1] = compute_r2_moment(&pu_loc, DIM, shift_loc, cx, cy, cz);
            if (t == t_b) r2_vals[2] = compute_r2_moment(&pu_loc, DIM, shift_loc, cx, cy, cz);
            if (t == t_c) r2_vals[3] = compute_r2_moment(&pu_loc, DIM, shift_loc, cx, cy, cz);
        }

        const double ratio = (r2_vals[0] > 1e-9) ? (r2_vals[3] / r2_vals[0]) : 1e300;
        const bool saturated = (ratio < 2.0);
        printf("[H]   %6d     %.2f        %.2f        %.2f        %.2f        %s\n",
            (int)strength, r2_vals[0], r2_vals[1], r2_vals[2], r2_vals[3],
            saturated ? "JA (gebunden)" : "nein (frei)");

        if (strength != 0 && ratio < best_loc_ratio) {
            best_loc_ratio = ratio;
            best_loc_strength = strength;
        }

        ProPhysics_Free(&pu_loc);
    }

    printf("\n[H] Beste Lokalisierung: strength=%d, <r^2>(%u)/<r^2>(0) = %.3f\n",
        best_loc_strength, N_TICKS, best_loc_ratio);

    Cx* C = (Cx*)malloc(N_TICKS * sizeof(Cx));
    if (!C) {
        printf("[H] malloc C failed.\n");
        return false;
    }

    printf("\n[H] Referenzlauf ohne Coulomb-Field...\n");
    if (!run_hydrogen_single_ex(DIM, 0, softening_q8,
        shape_param,
        (prep_kind == HYDRO_PREP_GROUND) ? HYDRO_PREP_GAUSS : prep_kind,
        N_TICKS, C, imag_dt_kin, imag_dt_pot, imag_n_iter)) {
        printf("[H] Ref-Lauf fehlgeschlagen.\n");
        free(C);
        return false;
    }

    double* spec_re = (double*)malloc(spec_buffer_size * sizeof(double));
    double* spec_im = (double*)malloc(spec_buffer_size * sizeof(double));
    if (!spec_re || !spec_im) {
        printf("[H] malloc spec failed.\n");
        free(C); free(spec_re); free(spec_im);
        return false;
    }

    const uint32_t N_pad_ref = compute_spectrum(C, N_TICKS, pad_factor,
        spec_re, spec_im);
    Peak ref_peaks[MAX_PEAKS];
    const double omega_cutoff_bin =
        omega_cutoff * (double)N_pad_ref / (2.0 * M_PI);
    const int ref_n = find_peaks(spec_re, spec_im, N_pad_ref,
        omega_cutoff_bin, ref_peaks, 3u);
    printf("[H] Referenz: %d Peaks gefunden.\n", ref_n);

    int best_strength = 0;
    double best_rel_dev = 1e300;
    Peak best_peaks[3];
    memset(best_peaks, 0, sizeof(best_peaks));
    int best_n_peaks = 0;

    printf("\n[H] Strength-Sweep:\n");
    printf("[H]   strength  n_peaks  omega_1   omega_2   omega_3   rel_dev(1/n^2)\n");
    printf("[H]   -----------------------------------------------------------------\n");

    for (uint32_t s = 0; s < n_str; ++s) {
        if (!run_hydrogen_single_ex(DIM, strengths[s], softening_q8,
            shape_param, prep_kind, N_TICKS, C,
            imag_dt_kin, imag_dt_pot, imag_n_iter))
        {
            printf("[H]   %6d   --- Lauf fehlgeschlagen ---\n", (int)strengths[s]);
            continue;
        }

        const uint32_t N_pad = compute_spectrum(C, N_TICKS, pad_factor,
            spec_re, spec_im);
        Peak peaks[MAX_PEAKS];
        memset(peaks, 0, sizeof(peaks));
        const double cutoff_bin = omega_cutoff * (double)N_pad / (2.0 * M_PI);
        const int n_peaks = find_peaks(spec_re, spec_im, N_pad,
            cutoff_bin, peaks, 3u);

        double ratio_21 = 0.0, ratio_31 = 0.0;
        const double rel_dev = check_1_over_n2(peaks, (uint32_t)n_peaks,
            &ratio_21, &ratio_31);

        printf("[H]   %6d   %d        ", (int)strengths[s], n_peaks);
        if (n_peaks >= 3) {
            Peak sorted[3];
            for (uint32_t i = 0; i < 3; ++i) sorted[i] = peaks[i];
            for (uint32_t i = 0; i < 3; ++i)
                for (uint32_t j = i + 1; j < 3; ++j)
                    if (sorted[j].omega < sorted[i].omega) {
                        Peak tmp = sorted[i]; sorted[i] = sorted[j]; sorted[j] = tmp;
                    }
            printf("%.4f    %.4f    %.4f    %.4e\n",
                sorted[0].omega, sorted[1].omega, sorted[2].omega, rel_dev);

            if (rel_dev < best_rel_dev) {
                best_rel_dev = rel_dev;
                best_strength = strengths[s];
                for (uint32_t i = 0; i < 3; ++i) best_peaks[i] = sorted[i];
                best_n_peaks = 3;
            }
        }
        else {
            printf("---          (n_peaks < 3)\n");
        }
    }

    printf("\n[H] Beste strength: %d, rel_dev(1/n^2) = %.4e\n",
        best_strength, best_rel_dev);

    if (best_n_peaks >= 3) {
        if (run_hydrogen_single_ex(DIM, best_strength, softening_q8,
            shape_param, prep_kind, N_TICKS, C,
            imag_dt_kin, imag_dt_pot, imag_n_iter))
        {
            const uint32_t N_pad = compute_spectrum(C, N_TICKS, pad_factor,
                spec_re, spec_im);
            double peak_amp = best_peaks[0].amp;
            double peak_amp_2 = best_peaks[1].amp;
            double peak_amp_3 = best_peaks[2].amp;

            const uint32_t cutoff_bin = (uint32_t)(omega_cutoff
                * (double)N_pad / (2.0 * M_PI));
            double* amps = (double*)malloc(cutoff_bin * sizeof(double));
            if (amps) {
                for (uint32_t i = 0; i < cutoff_bin; ++i)
                    amps[i] = spec_re[i] * spec_re[i] + spec_im[i] * spec_im[i];
                for (uint32_t i = 0; i < cutoff_bin; ++i)
                    for (uint32_t j = i + 1; j < cutoff_bin; ++j)
                        if (amps[j] < amps[i]) {
                            double tmp = amps[i]; amps[i] = amps[j]; amps[j] = tmp;
                        }
                const double median = amps[cutoff_bin / 2];

                printf("[H] Peak/Median-Verhaeltnis: ");
                printf("%.1f, %.1f, %.1f\n",
                    peak_amp / (median + 1e-30),
                    peak_amp_2 / (median + 1e-30),
                    peak_amp_3 / (median + 1e-30));
                free(amps);
            }
        }
    }

    const bool bound_by_spectrum = (best_n_peaks >= 1
        && best_peaks[0].omega < 0.3);
    const bool bound_by_localization = (best_loc_ratio < 2.0);
    const bool pass = bound_by_spectrum || bound_by_localization;

    printf("\n[H] Bindungs-Nachweis:\n");
    printf("[H]   Spektral (omega_1 < 0.3)       : %s (omega_1=%.4f)\n",
        bound_by_spectrum ? "OK" : "FAILED", best_peaks[0].omega);
    printf("[H]   Lokalisierung (<r^2> gesaettigt): %s (ratio=%.3f)\n",
        bound_by_localization ? "OK" : "FAILED", best_loc_ratio);
    printf("[H]   Kopplungskonstante              : strength=%d\n",
        best_loc_strength);

    printf("[H] -> %s\n",
        pass
        ? "PASSED (gebundener Zustand beobachtet)"
        : "FAILED (kein gebundener Zustand beobachtet)");

    free(C);
    free(spec_re);
    free(spec_im);
    return pass;
}

/* Wrapper mit 18b-Defaults (dt_kin=1e-2, dt_pot=1e-6, n_iter=1000).
 * API-erhaltend. */
static bool test_hydrogen_spectrum_impl(
    const char* label,
    uint32_t DIM, uint32_t N_TICKS, double shape_param,
    HydrogenPrepKind prep_kind,
    uint32_t softening_q8,
    double omega_cutoff,
    const int32_t* loc_strengths, uint32_t n_loc,
    const int32_t* strengths, uint32_t n_str,
    uint32_t pad_factor, uint32_t spec_buffer_size)
{
    return test_hydrogen_spectrum_impl_ex(
        label, DIM, N_TICKS, shape_param, prep_kind, softening_q8,
        omega_cutoff, loc_strengths, n_loc, strengths, n_str,
        pad_factor, spec_buffer_size,
        0.01,       /* imag_dt_kin (18b) */
        0.000001,   /* imag_dt_pot (18b) */
        1000u);     /* imag_n_iter (18b) */
}

/* ==========================================================================
 * test_hydrogen_spectrum (Etappe 18).
 *
 * Unveraendert: dim=32, Gauss-Prep, moderate Kopplung, keine Imaginaerzeit.
 * ========================================================================== */
bool test_hydrogen_spectrum(void)
{
    static const int32_t loc[] = { 0, 2000, 4000, 8000 };
    static const int32_t str[] = { 500, 1000, 2000, 4000 };
    return test_hydrogen_spectrum_impl(
        "Etappe 18: Coulomb / Wasserstoff-Spektrum (dim=32, Gauss)",
        32u, 2000u, 3.0,
        HYDRO_PREP_GAUSS,
        64u, 0.6,
        loc, 4u, str, 4u,
        4u, 8192u
    );
}

/* ==========================================================================
 * test_hydrogen_spectrum_48 (jetzt Etappe 18d, dim=64).
 *
 * ACHTUNG: Historischer Name (siehe F1 in der Roadmap-Notiz).
 *          Tatsaechlich laeuft dieser Test jetzt bei dim=64.
 *
 * Etappe 18b (dim=32) ist in der Roadmap dokumentiert, wird aber nicht
 * mehr direkt aufgerufen. Der Parametersatz ist ueber die _ex-Variante
 * weiterhin verfuegbar, falls 18b reproduziert werden soll.
 *
 * Etappe 18d: dim=64, N_TICKS=4000, SOF=32, dt_pot=2e-7.
 * ========================================================================== */
bool test_hydrogen_spectrum_48(void)
{
    static const int32_t loc[] = { 0, 16000, 32000, 48000 };
    static const int32_t str[] = { 16000, 32000, 48000 };
    return test_hydrogen_spectrum_impl_ex(
        "Etappe 18d: Coulomb / Wasserstoff-Spektrum (dim=64, Imaginaerzeit)",
        64u, 4000u, 6.0,
        HYDRO_PREP_GROUND,
        32u, 0.2,
        loc, 4u, str, 3u,
        16u, 131072u,
        0.01,       /* imag_dt_kin */
        0.0000002,  /* imag_dt_pot = 2e-7 (18d) */
        1000u);     /* imag_n_iter */
}