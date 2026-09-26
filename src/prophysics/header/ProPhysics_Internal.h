/**
 * @file ProPhysics_Internal.h
 * @brief Interne Kernel-Header-Datei fuer alle .c-Module.
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * NICHT Teil des oeffentlichen API.
 *
 * Enthaelt:
 *   - Modul-uebergreifende Tabellen (U5-Gewichte, Spinor-Indizes,
 *     Nachbar-Kanaele)
 *   - static-inline Helfer (RNG, Rundungen, Saturation, Amplituden,
 *     Signed-Permutation, Pauli-Matrizen, Wilson-Pfad-Validierung,
 *     Aligned Allocation, SU(2)-Quaternion-Arithmetik,
 *     SU(2)-Exponential)
 *   - extern-Deklarationen der dateiuebergreifend genutzten Funktionen
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Detaillierte Konfiguration: siehe docs/project/CONFIG.md.
 */

#ifndef PROPHYSICS_INTERNAL_H
#define PROPHYSICS_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

#include "ProPhysics.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* ==========================================================================
     * Modul-Konstanten
     * ========================================================================== */

#define PRO_CORE_MAGIC       0x50525048u   /* 'P','R','P','H' */

     /* U5-Gewichte pro Basis-Index.
      * Die gewichtete Norm Sum_k Sum_b PRO_U5_W[b] * |c_b(k)|^2 ist der
      * Erhaltungssatz des Kernels (U5). */
    static const uint8_t PRO_U5_W[PRO_AMP_BASIS_SIZE] = {
        0u, 1u, 1u, 4u, 4u, 5u, 0u, 0u
    };

    /* Qubit-Basis fuer 2-Qubit-Gates (Tensor-Modul).
     * {UR_POSITRON_CW, UR_NEGATRON_CCW} bilden den 2-dim Unterraum. */
    static const int PRO_QUBIT_BASIS[2] = {
        (int)UR_POSITRON_CW, (int)UR_NEGATRON_CCW
    };

    /* Spinor-Index-Mapping fuer Spin-1/2 und Dirac.
     * UP/DN beziehen sich auf die zwei Basiszustaende pro Spinor-Sektor. */
    static const int PRO_SPINOR_UP[2] = {
        (int)UR_POSITRON_CW, (int)UR_NEGATRON_CW
    };
    static const int PRO_SPINOR_DN[2] = {
        (int)UR_POSITRON_CCW, (int)UR_NEGATRON_CCW
    };

    /* Reihenfolge der 6 physikalischen Nachbarkanaele (3D).
     * Wird von Transport-, Wave- und Diffusions-Schritten genutzt,
     * um konsistent ueber die raeumlichen Richtungen zu iterieren. */
    static const uint8_t PRO_NEIGHBOR_CHANNELS[6] = {
        PRO_NEIGHBOR_X_PLUS,
        PRO_NEIGHBOR_X_MINUS,
        PRO_NEIGHBOR_Y_PLUS,
        PRO_NEIGHBOR_Y_MINUS,
        PRO_NEIGHBOR_Z_PLUS,
        PRO_NEIGHBOR_Z_MINUS
    };

    /* ==========================================================================
     * Basis-Helfer (RNG, Rundung, Saturation)
     * ========================================================================== */

    static inline uint64_t pro_rotl64(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    /* xoshiro256** — Standard-RNG des Kernels. State ist uint64_t[4]. */
    static inline uint64_t pro_xoshiro_next(uint64_t s[4]) {
        const uint64_t result = pro_rotl64(s[1] * 5u, 7) * 9u;
        const uint64_t t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = pro_rotl64(s[3], 45);
        return result;
    }

    /* Gleichverteilung in [0, 1) aus 53 Mantissen-Bits. */
    static inline double pro_uniform01(uint64_t rng[4]) {
        return (double)(pro_xoshiro_next(rng) >> 11) / 9007199254740992.0;
    }

    /* splitmix64 — nicht-kryptographischer Hash fuer deterministische
     * Konstruktionen (u_field, Context-Perm). */
    static inline uint64_t pro_splitmix64(uint64_t z) {
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    /* Rundung mit Shift um n Stellen, vorzeichen-korrekt (Round-Half-Up
     * fuer positive, Round-Half-Down fuer negative Werte — symmetrisch). */
    static inline int64_t pro_round_shift_q30(int64_t x) {
        if (x >= 0) return (x + (1LL << 29)) >> 30;
        return -(((-x) + (1LL << 29)) >> 30);
    }

    static inline int64_t pro_round_shift_q31(int64_t x) {
        if (x >= 0) return (x + (1LL << 30)) >> 31;
        return -(((-x) + (1LL << 30)) >> 31);
    }

    /* Ganzzahlige Division mit Rundung (symmetrisch). */
    static inline int64_t pro_div_round(int64_t x, int64_t d) {
        if (x >= 0) return (x + d / 2) / d;
        return -((-x + d / 2) / d);
    }

    /* Saturation nach int32 (Clamping statt Overflow). */
    static inline int32_t pro_sat_i32(int64_t x) {
        if (x > INT32_MAX) return INT32_MAX;
        if (x < INT32_MIN) return INT32_MIN;
        return (int32_t)x;
    }

    /* Vorzeichenwechsel in Q31 ohne Overflow (0 - x in uint32). */
    static inline int32_t pro_neg_q31(int32_t x) {
        return (int32_t)(0u - (uint32_t)x);
    }

    /* Typ-Pruefungen fuer ProUrState. */
    static inline bool is_positron(uint8_t st) {
        return (st == UR_POSITRON_CW || st == UR_POSITRON_CCW);
    }

    static inline bool is_negatron(uint8_t st) {
        return (st == UR_NEGATRON_CW || st == UR_NEGATRON_CCW);
    }

    static inline bool is_valid_state(uint8_t st) {
        return (st <= (uint8_t)UR_PHOTON);
    }

    /* Overflow-Check fuer Allokationsgroessen. */
    static inline bool alloc_size_ok(uint64_t n, size_t elem) {
        if (n == 0 || elem == 0) return false;
        return (n <= (uint64_t)(SIZE_MAX / elem));
    }

    /* ==========================================================================
     * Amplituden-Helfer
     * ========================================================================== */

     /* Betragsquadrat |a|^2 eines komplexen Q31-Werts, overflow-sicher als
      * uint64. Vermeidet UB fuer re = im = INT32_MIN. */
    static inline uint64_t pro_amp_abs2(ProAmpQ31 a) {
        const int32_t re = pro_amp_real(a);
        const int32_t im = pro_amp_imag(a);
        const uint64_t re_u = (re < 0) ? (uint64_t)(-(int64_t)re) : (uint64_t)re;
        const uint64_t im_u = (im < 0) ? (uint64_t)(-(int64_t)im) : (uint64_t)im;
        return re_u * re_u + im_u * im_u;
    }

    /* Summe |c_b|^2 fuer b in [b_start, b_end). */
    static inline uint64_t pro_amp_vec_norm_sq_range(
        const ProAmpVector* v, uint8_t b_start, uint8_t b_end)
    {
        uint64_t sum = 0u;
        for (uint8_t b = b_start; b < b_end; ++b) {
            sum += pro_amp_abs2(v->coeff[b]);
        }
        return sum;
    }

    /* Wendet eine signierte Permutation auf einen Amplitudenvektor an:
     *   dst[i] = sign[i] * src[perm[i]]
     * Diese Operation ist die fundamentale unitaere Operation des Kernels. */
    static inline void pro_apply_signed_perm_vec(
        const ProAmpVector* src, ProAmpVector* dst,
        const uint8_t* perm, const int8_t* sign)
    {
        for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
            ProAmpQ31 v = src->coeff[perm[i]];
            if (sign[i] < 0) {
                v = pro_amp_pack(pro_neg_q31(pro_amp_real(v)),
                    pro_neg_q31(pro_amp_imag(v)));
            }
            dst->coeff[i] = v;
        }
    }

    /* ==========================================================================
     * Pauli-Matrizen (Generatoren i*sigma in Quaternion-Form)
     *
     * Rueckgabe in out[4] = (a_re, a_im, b_re, b_im) in Skala PRO_SU2_SCALE.
     * which: 0 = i*sigma_x, 1 = i*sigma_y, 2 = i*sigma_z.
     * ========================================================================== */

    static inline void pro_pauli_get(int which, int32_t out[4])
    {
        const int32_t S = (int32_t)PRO_SU2_SCALE;
        out[0] = 0; out[1] = 0; out[2] = 0; out[3] = 0;
        switch (which) {
        case 0: out[3] = S; break;   /* i*sigma_x */
        case 1: out[2] = S; break;   /* i*sigma_y */
        case 2: out[1] = S; break;   /* i*sigma_z */
        default: break;
        }
    }

    /* ==========================================================================
     * Wilson-Loop-Pfad-Helfer
     * ========================================================================== */

     /* Validiert Pfad-Argumente fuer Wilson_Loop (U(1) und SU(2)).
      * Prueft: pu != NULL, Arrays != NULL, path_len in (0, total_nodes],
      * alle Knoten-Indizes < total_nodes, alle Kanaele < CHANNELS_MAX. */
    static inline bool pro_wilson_validate_path(
        const ProUniverse* pu,
        const uint64_t* path_nodes,
        const uint8_t* path_channels,
        uint32_t        path_len)
    {
        if (!pu || !path_nodes || !path_channels) return false;
        if (path_len == 0u || path_len > pu->total_nodes) return false;
        for (uint32_t k = 0; k < path_len; ++k) {
            if (path_nodes[k] >= pu->total_nodes) return false;
            if (path_channels[k] >= CHANNELS_MAX) return false;
        }
        return true;
    }

    /* ==========================================================================
     * Aligned Allocation
     *
     * Cache-Line-aligned Allokation fuer Hot-Arrays. Plattform-Portabilitaet:
     *   MSVC      -> _aligned_malloc / _aligned_free
     *   C11       -> aligned_alloc (falls verfuegbar)
     *   POSIX     -> posix_memalign
     *   Fallback  -> malloc (kein Alignment)
     * ========================================================================== */

    static inline void* pro_aligned_calloc(size_t count, size_t size)
    {
        if (count == 0u || size == 0u) return NULL;
        if (count > SIZE_MAX / size) return NULL;
        const size_t total = count * size;

        if (total > SIZE_MAX - (PRO_CACHE_LINE - 1u)) return NULL;
        const size_t total_aligned =
            (total + (PRO_CACHE_LINE - 1u)) & ~((size_t)PRO_CACHE_LINE - 1u);

        void* p = NULL;
#if defined(_MSC_VER)
        p = _aligned_malloc(total_aligned, (size_t)PRO_CACHE_LINE);
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ALIGNED_ALLOC)
        p = aligned_alloc((size_t)PRO_CACHE_LINE, total_aligned);
#elif defined(_POSIX_VERSION)
        if (posix_memalign(&p, (size_t)PRO_CACHE_LINE, total_aligned) != 0) p = NULL;
#else
        p = malloc(total_aligned);
#endif
        if (!p) return NULL;
        memset(p, 0, total);
        return p;
    }

    static inline void pro_aligned_free(void* p)
    {
        if (!p) return;
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        free(p);
#endif
    }

    /* ==========================================================================
     * SU(2)-Quaternion-Arithmetik
     *
     * Quaternion-Repraesentation eines SU(2)-Links:
     *   U = [[a, b], [-b*, a*]]
     *
     * Skala PRO_SU2_SCALE = 2^30. Konvention:
     *   (a, b) * (c, d) = (a*c - b*d*, a*d + b*c*)
     *
     * Skala 2^30 (nicht 2^31), damit die Summe der vier Produktterme
     * in a_new = a1*a2 - b1* * b2 innerhalb int64 bleibt (max. 2^62).
     * ========================================================================== */

     /* Norm-Quadrat |a|^2 + |b|^2 als uint64, overflow-sicher. */
    static inline uint64_t pro_su2_norm_sq(
        int32_t a_re, int32_t a_im, int32_t b_re, int32_t b_im)
    {
        const uint64_t ar = (a_re < 0) ? (uint64_t)(-(int64_t)a_re) : (uint64_t)a_re;
        const uint64_t ai = (a_im < 0) ? (uint64_t)(-(int64_t)a_im) : (uint64_t)a_im;
        const uint64_t br = (b_re < 0) ? (uint64_t)(-(int64_t)b_re) : (uint64_t)b_re;
        const uint64_t bi = (b_im < 0) ? (uint64_t)(-(int64_t)b_im) : (uint64_t)b_im;
        return ar * ar + ai * ai + br * br + bi * bi;
    }

    /* Quaternion-Produkt (a1, b1) * (a2, b2) in Skala 2^30.
     * Die vier Ergebnis-Komponenten werden jeweils nach int32 saturiert. */
    static inline void pro_su2_mul(
        int32_t a1_re, int32_t a1_im, int32_t b1_re, int32_t b1_im,
        int32_t a2_re, int32_t a2_im, int32_t b2_re, int32_t b2_im,
        int32_t* out_a_re, int32_t* out_a_im,
        int32_t* out_b_re, int32_t* out_b_im)
    {
        /* a_out = a1*a2 - b1*b2* */
        const int64_t a_re_num =
            (int64_t)a1_re * a2_re - (int64_t)a1_im * a2_im
            - (int64_t)b1_re * b2_re - (int64_t)b1_im * b2_im;
        const int64_t a_im_num =
            (int64_t)a1_re * a2_im + (int64_t)a1_im * a2_re
            + (int64_t)b1_re * b2_im - (int64_t)b1_im * b2_re;

        /* b_out = a1*b2 + b1*a2* */
        const int64_t b_re_num =
            (int64_t)a1_re * b2_re - (int64_t)a1_im * b2_im
            + (int64_t)b1_re * a2_re + (int64_t)b1_im * a2_im;
        const int64_t b_im_num =
            (int64_t)a1_re * b2_im + (int64_t)a1_im * b2_re
            - (int64_t)b1_re * a2_im + (int64_t)b1_im * a2_re;

        *out_a_re = pro_sat_i32(pro_round_shift_q30(a_re_num));
        *out_a_im = pro_sat_i32(pro_round_shift_q30(a_im_num));
        *out_b_re = pro_sat_i32(pro_round_shift_q30(b_re_num));
        *out_b_im = pro_sat_i32(pro_round_shift_q30(b_im_num));
    }

    /* Adjungiertes: (a, b) -> (a*, -b) in Quaternion-Form. */
    static inline void pro_su2_conj(
        int32_t a_re, int32_t a_im, int32_t b_re, int32_t b_im,
        int32_t* out_a_re, int32_t* out_a_im,
        int32_t* out_b_re, int32_t* out_b_im)
    {
        *out_a_re = a_re;
        *out_a_im = pro_neg_q31(a_im);
        *out_b_re = pro_neg_q31(b_re);
        *out_b_im = pro_neg_q31(b_im);
    }

    /* ==========================================================================
     * SU(2)-Exponential
     *
     * Wendet exp(i * dt * E) auf einen Link an: U' = exp(i*dt*E) * U.
     *
     * E ist ein su(2)-Element in Quaternion-Form (E_a, E_b). E wird als
     * Achse-Winkel-Vektor in der ueblichen Pauli-Konvention interpretiert:
     *   E <-> -(E_b_im) * sigma_x - (E_b_re) * sigma_y - (E_a_im) * sigma_z
     * Der Realteil E_a_re wird ignoriert (su(2) ist rein imaginaer).
     *
     * Achse n = E/|E|, halber Rotationswinkel theta = dt*|E|.
     *   exp(i*dt*E) = (cos(theta) - i*sin(theta)*n_z,
     *                  -sin(theta)*(n_y + i*n_x))
     *
     * Skala PRO_SU2_SCALE, dt in Q15 (1.0 = 32768). Kein div/mod.
     * ========================================================================== */

    static inline void pro_su2_exp_apply(
        int32_t u_a_re, int32_t u_a_im, int32_t u_b_re, int32_t u_b_im,
        int32_t e_a_re, int32_t e_a_im, int32_t e_b_re, int32_t e_b_im,
        uint32_t dt_q15,
        int32_t* out_a_re, int32_t* out_a_im,
        int32_t* out_b_re, int32_t* out_b_im)
    {
        (void)e_a_re;   /* Realteil ignorieren -- su(2) ist rein imaginaer */

        /* Achse E in Skala 1.0 (normiert aus Q30). */
        const double Ex = -(double)e_b_im / (double)PRO_SU2_SCALE;
        const double Ey = -(double)e_b_re / (double)PRO_SU2_SCALE;
        const double Ez = -(double)e_a_im / (double)PRO_SU2_SCALE;
        const double E_mag = sqrt(Ex * Ex + Ey * Ey + Ez * Ez);

        /* dt in Skala 1.0 (aus Q15). */
        const double dt = (double)dt_q15 / 32768.0;

        /* Halber Rotationswinkel. */
        const double theta = dt * E_mag;

        const double c_theta = cos(theta);
        const double s_theta = sin(theta);

        /* exp(i*theta*n.sigma) in Quaternion-Form:
         *   a_e = cos(theta) - i*sin(theta)*n_z
         *   b_e = -sin(theta)*(n_y + i*n_x)
         * mit n = E/|E|. */
        const double inv_norm = (E_mag > 1e-20) ? (1.0 / E_mag) : 0.0;
        const double nx = Ex * inv_norm;
        const double ny = Ey * inv_norm;
        const double nz = Ez * inv_norm;

        const double e_a_re_f = c_theta;
        const double e_a_im_f = -s_theta * nz;
        const double e_b_re_f = -s_theta * ny;
        const double e_b_im_f = -s_theta * nx;

        const int32_t e_a_re_q = pro_sat_i32((int64_t)llround(e_a_re_f * (double)PRO_SU2_SCALE));
        const int32_t e_a_im_q = pro_sat_i32((int64_t)llround(e_a_im_f * (double)PRO_SU2_SCALE));
        const int32_t e_b_re_q = pro_sat_i32((int64_t)llround(e_b_re_f * (double)PRO_SU2_SCALE));
        const int32_t e_b_im_q = pro_sat_i32((int64_t)llround(e_b_im_f * (double)PRO_SU2_SCALE));

        pro_su2_mul(e_a_re_q, e_a_im_q, e_b_re_q, e_b_im_q,
            u_a_re, u_a_im, u_b_re, u_b_im,
            out_a_re, out_a_im, out_b_re, out_b_im);
    }

    /* ==========================================================================
     * Tensor-Index und Dichte-Umrechnung
     * ========================================================================== */

     /* Tensor-Index im 8x8-Kronecker-Produkt: idx = a*8 + b. */
    static inline int pro_tensor_idx(int a, int b) { return a * 8 + b; }

    /* Q31-Wert -> dimensionslose double in [-1, 1]. */
    static inline double pro_density_q31_to_unit(int32_t x) {
        return (double)x / 2147483647.0;
    }

    /* Dimensionslose double in [-1, 1] -> Q31, mit Clamping. */
    static inline int32_t pro_density_unit_to_q31(double x) {
        if (x > 1.0) x = 1.0;
        if (x < -1.0) x = -1.0;
        return (int32_t)llround(x * 2147483647.0);
    }

    /* ==========================================================================
     * Extern-Deklarationen
     *
     * Funktionen, die von mehreren Modulen genutzt werden, aber nicht
     * Teil der oeffentlichen API sind. Definition liegt jeweils im
     * genannten Modul.
     * ========================================================================== */

     /* --- in ProPhysics_Gauge.c --- */

     /* Projektionsachse lambda aus einem Amplitudenvektor (2-dim Unterraum
      * {UR_POSITRON_CW, UR_NEGATRON_CCW}). */
    double pro_amp_to_lambda(const ProAmpVector* v);

    /* Scharfe Messung: sign(cos(theta - lambda)). Deterministisch. */
    static inline int pro_measure_sharp(double theta, double lambda) {
        return (cos(theta - lambda) > 0.0) ? +1 : -1;
    }

    /* Komplexe Rotation c * exp(i*phi), phi in Q16. Q30-Tabelle. */
    ProAmpQ31 pro_amp_rotate_q16(ProAmpQ31 c, uint16_t phase_fx);

    /* --- in ProPhysics_Amp.c --- */

    /* Rationale Approximation der Transport-Koeffizienten.
     * Liefert c_num, s_num, denom mit cos ~ c_num/denom, sin ~ s_num/denom.
     * Gecached in pu->last_* (pu darf NULL sein). */
    void pro_transport_coeffs(ProUniverse* pu, uint32_t theta_q15,
        int64_t* c_num_out, int64_t* s_num_out, int64_t* denom_out);

    /* 2-Knoten-Unitare mit Koeffizienten (c_num, s_num, denom). */
    void pro_edge_transport_2node(
        ProAmpVector* va, ProAmpVector* vb,
        int64_t c_num, int64_t s_num, int64_t denom);

    /* --- in ProPhysics_Tensor.c --- */

    /* Jacobi-Eigenzerlegung fuer reelle symmetrische n x n-Matrix (n <= 16).
     * Wird von Tensor- und Density-Modul genutzt. */
    void pro_jacobi_sym_eigen(double A[16][16], double V[16][16], int n);

#ifdef __cplusplus
}
#endif

#endif /* PROPHYSICS_INTERNAL_H */