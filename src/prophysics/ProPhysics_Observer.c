/* ==========================================================================
 * ProPhysics - Observer Modul
 * File: ProPhysics_Observer.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Test-spezifische Zusatzdynamik auf amp_grid.
 *
 * Verantwortlich fuer:
 *   - ProPhysics_Init_Observer
 *   - ProPhysics_Observer_Read_Local
 *   - ProPhysics_Get_Environment_Trace
 *   - ProPhysics_Apply_Local_Amplitude_Diffusion
 *   - ProPhysics_Observer_Measure_CHSH
 *   - ProPhysics_Init_Chaotic_Source
 *   - ProPhysics_Apply_Nonlinear_Diffusion_Tick
 *   - ProPhysics_Observer_Measure_CHSH_Projected
 *   - ProPhysics_Apply_Local_Dephasing_Tick
 *
 * Interpretation (bindend):
 *   Alle Funktionen in dieser Datei sind TEST-SPEZIFISCH. Sie sind:
 *     - NICHT Teil von pro_urregeln_apply (in v3.0 entfernt)
 *     - NICHT Teil des SDK-Ticks (ProPhysics_SDK_Execute_Plastizitaet_Tick)
 *     - NICHT im U5-Invarianten-Pfad
 *   Sie modulieren amp_grid direkt, um bestimmte Hypothesen zu pruefen.
 *   S > 2 in einem nachfolgenden CHSH-Test ist KEIN Bell-Bruch -- es waere
 *   Superdeterminismus-Loop oder Testdesign-Fehler.
 *
 * Interne Helfer:
 *   - pro_amp_vector_abs2_sum (ProU128-Summe |c_b|^2, Overflow-sicher)
 *   - pro_observer_diffusion_pass (gemeinsamer Diffusions-Kern)
 *   - pro_observer_ping_pong (amp_grid <-> amp_scratch)
 *   - pro_observer_measure_axis (sign(cos(theta - lambda(v))))
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/Observer.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer
  * ========================================================================== */

  /* Summe der Quadrate |c_b|^2 ueber alle Basis-Zustaende eines Vektors,
   * als ProU128 (Overflow-Schutz fuer Summen ueber viele Knoten).
   *
   * Verwendung:
   *   - ProPhysics_Get_Environment_Trace (Summe ueber Nicht-Observer)
   *   - ProPhysics_Apply_Nonlinear_Diffusion_Tick (Sättigungs-Check)
   *
   * Bei einem spaeteren Bedarf in anderen Modulen kann dieser Helfer nach
   * ProPhysics_Internal.h wandern; derzeit modul-lokal (static). */
static inline ProU128 pro_amp_vector_abs2_sum(const ProAmpVector* v)
{
    ProU128 sum = PRO_U128_ZERO;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        sum = pro_u128_add(sum, pro_u128_from_u64(pro_amp_abs2(v->coeff[b])));
    }
    return sum;
}

/* Gemeinsamer Diffusions-Kern fuer lineare und nichtlineare Diffusion.
 *
 *   psi_k <- (1-alpha)*psi_k + (alpha/N) * Sum_{i<N} psi_{nb_i(k)}
 *
 * Ergebnis wird in pu->amp_scratch geschrieben; der Aufrufer ruft
 * anschliessend pro_observer_ping_pong, um die Puffer zu tauschen. So
 * kann die Sättigung (Nonlinear) noch auf dem frisch berechneten
 * Ergebnis arbeiten, bevor es zum neuen amp_grid wird.
 *
 * alpha_q31 ist der Kopplungsparameter in Q31 (0 = keine Diffusion).
 * n_nb ist 4 (2D) oder 6 (3D); ch_arr zeigt auf die n_nb Kanaele. */
static void pro_observer_diffusion_pass(
    ProUniverse* pu,
    int64_t alpha_q31,
    uint8_t n_nb,
    const uint8_t* ch_arr)
{
    const int64_t one_minus_q31 = INT32_MAX - alpha_q31;
    const int64_t inv_nb_alpha_q31 = alpha_q31 / (int64_t)n_nb;

    ProAmpVector* src = pu->amp_grid;
    ProAmpVector* dst = pu->amp_scratch;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const ProRegister* r = &pu->reg_source[k];
        const ProAmpVector* cur = &src[k];
        ProAmpVector* out = &dst[k];

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            out->coeff[b] = pro_amp_scale_q31(cur->coeff[b], one_minus_q31);
        }

        for (uint8_t i = 0; i < n_nb; ++i) {
            const uint64_t nb = r->channels[ch_arr[i]];
            if (nb >= pu->total_nodes || nb == k) continue;
            const ProAmpVector* vn = &src[nb];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                out->coeff[b] = pro_amp_add_q31(
                    out->coeff[b],
                    pro_amp_scale_q31(vn->coeff[b], inv_nb_alpha_q31));
            }
        }
    }
}

/* Tauscht amp_grid und amp_scratch. Nach einem Diffusions-Pass liegt
 * das Ergebnis in amp_scratch; dieser Tausch macht es zum neuen
 * amp_grid, und der alte amp_grid wird zum neuen Scratch. */
static inline void pro_observer_ping_pong(ProUniverse* pu)
{
    ProAmpVector* tmp = pu->amp_grid;
    pu->amp_grid = pu->amp_scratch;
    pu->amp_scratch = tmp;
}

/* Misst einen Amplitudenvektor entlang der Achse theta:
 *   out = sign(cos(theta - lambda(v)))
 *
 * Verwendung in beiden CHSH-Varianten (Mittelwert und projiziert). */
static inline int pro_observer_measure_axis(const ProAmpVector* v, double theta)
{
    return pro_measure_sharp(theta, pro_amp_to_lambda(v));
}

/* ==========================================================================
 * Observer-Deskriptor
 *
 * Reine Zuweisung, keine Allokation. size >= 1.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Init_Observer(ProUniverse* pu,
    ProObserver* obs, uint64_t base_node, uint32_t size)
{
    if (!obs) return;
    obs->base_node = 0;
    obs->size = 0;
    obs->_pad = 0;

    if (!pu) return;
    if (size == 0) return;
    if (base_node >= pu->total_nodes) return;
    if (base_node + (uint64_t)size > pu->total_nodes) return;

    obs->base_node = base_node;
    obs->size = size;
}

/* ==========================================================================
 * Observer_Read_Local
 *
 * Mittelwert der amp_grid-Vektoren ueber [base, base+size).
 * Rein lesend. Q31-Clamping nach Summation.
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Observer_Read_Local(const ProUniverse* pu,
    const ProObserver* obs, ProAmpVector* out_amp)
{
    if (!pu || !pu->amp_grid || !obs || !out_amp) return false;
    if (obs->size == 0) return false;
    if (obs->base_node + (uint64_t)obs->size > pu->total_nodes) return false;

    int64_t sum_re[PRO_AMP_BASIS_SIZE];
    int64_t sum_im[PRO_AMP_BASIS_SIZE];
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        sum_re[b] = 0;
        sum_im[b] = 0;
    }

    const uint64_t start = obs->base_node;
    const uint64_t end = obs->base_node + (uint64_t)obs->size;

    for (uint64_t k = start; k < end; ++k) {
        const ProAmpVector* v = &pu->amp_grid[k];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            sum_re[b] += (int64_t)pro_amp_real(v->coeff[b]);
            sum_im[b] += (int64_t)pro_amp_imag(v->coeff[b]);
        }
    }

    const int64_t inv = (int64_t)obs->size;
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        const int64_t re = sum_re[b] / inv;
        const int64_t im = sum_im[b] / inv;
        out_amp->coeff[b] = pro_amp_pack(pro_sat_i32(re), pro_sat_i32(im));
    }
    return true;
}

/* ==========================================================================
 * Get_Environment_Trace
 *
 * Sum_{x nicht in observer} Sum_b |c_b(x)|^2, als ProU128
 * (Overflow-Schutz). Rein lesend.
 * ========================================================================== */

PROPHYSICS_API ProU128 ProPhysics_Get_Environment_Trace(
    const ProUniverse* pu, const ProObserver* obs)
{
    if (!pu || !pu->amp_grid || !obs) return PRO_U128_ZERO;

    const uint64_t start = obs->base_node;
    const uint64_t end = obs->base_node + (uint64_t)obs->size;

    ProU128 trace = PRO_U128_ZERO;
    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        if (k >= start && k < end) continue;
        trace = pro_u128_add(trace,
            pro_amp_vector_abs2_sum(&pu->amp_grid[k]));
    }
    return trace;
}

/* ==========================================================================
 * Apply_Local_Amplitude_Diffusion
 *
 *   psi_k <- (1-alpha) * psi_k + (alpha/N) * Sum_{i<N} psi_{nb_i(k)}
 *
 * N = 4 (2D) oder 6 (3D). alpha = rate_percent/100.
 * Ping-Pong ueber pu->amp_grid / pu->amp_scratch.
 * Linear, deterministisch, erhaelt die Richtung im 2-dim Unterraum.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Local_Amplitude_Diffusion(ProUniverse* pu,
    uint32_t rate_percent)
{
    if (!pu || !pu->amp_grid || !pu->reg_source) return;
    if (!pu->amp_scratch) return;
    if (rate_percent == 0u) return;
    if (rate_percent > 100u) rate_percent = 100u;

    const int64_t alpha_q31 = ((int64_t)rate_percent * INT32_MAX) / 100;

    uint8_t n_nb;
    uint8_t ch_arr[6];
    if (pu->grid_ndim == 3u) {
        n_nb = 6u;
        for (uint8_t i = 0; i < 6u; ++i)
            ch_arr[i] = PRO_NEIGHBOR_CHANNELS[i];
    }
    else {
        n_nb = 4u;
        ch_arr[0] = 0u; ch_arr[1] = 1u; ch_arr[2] = 2u; ch_arr[3] = 3u;
    }

    pro_observer_diffusion_pass(pu, alpha_q31, n_nb, ch_arr);
    pro_observer_ping_pong(pu);
}

/* ==========================================================================
 * Observer_Measure_CHSH
 *
 * Einzelmessung an zwei Beobachtern ueber den Mittelwert ihrer Regionen.
 * lambda_A = pro_amp_to_lambda(Read_Local(obs_A)).
 * out = sign(cos(theta - lambda)). Rein lesend.
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH(const ProUniverse* pu,
    const ProObserver* obs_A, const ProObserver* obs_B,
    double theta_a, double theta_b, int* out_a, int* out_b)
{
    if (!pu || !pu->amp_grid || !obs_A || !obs_B) return false;
    if (!out_a || !out_b) return false;

    ProAmpVector amp_A, amp_B;
    if (!ProPhysics_Observer_Read_Local(pu, obs_A, &amp_A)) return false;
    if (!ProPhysics_Observer_Read_Local(pu, obs_B, &amp_B)) return false;

    *out_a = pro_observer_measure_axis(&amp_A, theta_a);
    *out_b = pro_observer_measure_axis(&amp_B, theta_b);
    return true;
}

/* ==========================================================================
 * Init_Chaotic_Source
 *
 * dim x dim-Region ab base_node mit deterministischer lambda-Verteilung:
 *   lambda(x,y) = 2*pi * fract( splitmix64(x*A + y*B + seed) / 2^64 )
 *   psi(x,y) = cos(lambda/2)|up> + sin(lambda/2)|down>
 * Kein PRNG, keine externe Entropie.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Init_Chaotic_Source(ProUniverse* pu,
    uint64_t base_node, uint32_t dim, uint64_t seed)
{
    if (!pu || !pu->amp_grid) return;
    if (dim < 2) return;

    const uint64_t nodes = (uint64_t)dim * (uint64_t)dim;
    if (base_node + nodes > pu->total_nodes) return;

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = base_node + (uint64_t)y * dim + x;

            uint64_t h = (uint64_t)x * 0x9e3779b97f4a7c15ULL
                + (uint64_t)y * 0xc2b2ae3d27d4eb4fULL
                + seed;
            h = pro_splitmix64(h);

            const double u = (double)(h >> 11) / 9007199254740992.0;
            const double lambda = u * PRO_2PI;

            const double half = lambda * 0.5;
            const int32_t a_up = (int32_t)lround(cos(half) * 2147483647.0);
            const int32_t a_dn = (int32_t)lround(sin(half) * 2147483647.0);

            ProAmpVector* v = &pu->amp_grid[idx];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) v->coeff[b] = 0;
            v->coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            v->coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
        }
    }
}

/* ==========================================================================
 * Apply_Nonlinear_Diffusion_Tick
 *
 *   1. Lineare Diffusion (4 Nachbarn, fest).
 *   2. Gross-Pitaevskii-artige Saettigung: falls Sum_b |c_b|^2 > threshold,
 *      skaliere alle c_b mit threshold/dichte.
 *
 * saturation_threshold_q62 == 0 deaktiviert Saettigung.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Diffusion_Tick(ProUniverse* pu,
    uint32_t rate_percent, uint64_t saturation_threshold_q62)
{
    if (!pu || !pu->amp_grid || !pu->reg_source) return;
    if (!pu->amp_scratch) return;
    if (rate_percent == 0u) return;
    if (rate_percent > 100u) rate_percent = 100u;

    const int64_t alpha_q31 = ((int64_t)rate_percent * INT32_MAX) / 100;

    /* Schritt 1: lineare Diffusion mit 4 Nachbarn (2D-Reihenfolge,
     * bit-identisch zum Vorzustand). */
    const uint8_t ch_arr[4] = { 0u, 1u, 2u, 3u };
    pro_observer_diffusion_pass(pu, alpha_q31, 4u, ch_arr);

    /* Schritt 2: Saettigung auf dem frisch berechneten Ergebnis
     * (liegt in pu->amp_scratch, vor dem Ping-Pong). */
    if (saturation_threshold_q62 > 0u) {
        const double thresh = (double)saturation_threshold_q62;
        for (uint64_t k = 0; k < pu->total_nodes; ++k) {
            ProAmpVector* v = &pu->amp_scratch[k];

            const double dichte = pro_u128_to_double(
                pro_amp_vector_abs2_sum(v));
            if (dichte <= thresh) continue;

            const double scale_d = (thresh / dichte) * (double)INT32_MAX;
            int64_t s = (int64_t)(scale_d + 0.5);
            if (s < 0) s = 0;

            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                v->coeff[b] = pro_amp_scale_q31(v->coeff[b], s);
            }
        }
    }

    /* Ping-Pong: Ergebnis wird zum neuen amp_grid. */
    pro_observer_ping_pong(pu);
}

/* ==========================================================================
 * Observer_Measure_CHSH_Projected
 *
 * Zieht pro Seite EINEN Knoten aus der Beobachterregion via xoshiro256**,
 * liest dort die Amplitude und wertet sign(cos(theta - lambda)) aus.
 * out_node_a/out_node_b sind optional (Diagnose).
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH_Projected(
    const ProUniverse* pu,
    const ProObserver* obs_A,
    const ProObserver* obs_B,
    double theta_a,
    double theta_b,
    uint64_t rng[4],
    int* out_a,
    int* out_b,
    uint64_t* out_node_a,
    uint64_t* out_node_b)
{
    if (!pu || !pu->amp_grid || !obs_A || !obs_B) return false;
    if (!out_a || !out_b || !rng) return false;
    if (obs_A->size == 0 || obs_B->size == 0) return false;
    if (obs_A->base_node + (uint64_t)obs_A->size > pu->total_nodes) return false;
    if (obs_B->base_node + (uint64_t)obs_B->size > pu->total_nodes) return false;

    const uint64_t pick_a = pro_xoshiro_next(rng) % (uint64_t)obs_A->size;
    const uint64_t pick_b = pro_xoshiro_next(rng) % (uint64_t)obs_B->size;
    const uint64_t node_a = obs_A->base_node + pick_a;
    const uint64_t node_b = obs_B->base_node + pick_b;

    *out_a = pro_observer_measure_axis(&pu->amp_grid[node_a], theta_a);
    *out_b = pro_observer_measure_axis(&pu->amp_grid[node_b], theta_b);
    if (out_node_a) *out_node_a = node_a;
    if (out_node_b) *out_node_b = node_b;
    return true;
}

/* ==========================================================================
 * Apply_Local_Dephasing_Tick
 *
 * Rotiert coeff[UR_NEGATRON_CCW] um einen deterministischen Winkel:
 *   h = splitmix64(k*A + channels[5][k]*B + type_state*C)
 *   phi = U(0,1)(h) * 2*pi * (strength_percent/100)
 *   c4 <- c4 * exp(i*phi)
 *
 * Komplexe Rotation ist unitaer, |c4|^2 bleibt erhalten.
 * strength_percent == 0: no-op. 100: volle 2*pi-Rotation.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Local_Dephasing_Tick(ProUniverse* pu,
    uint32_t strength_percent)
{
    if (!pu || !pu->amp_grid) return;
    if (strength_percent == 0u) return;
    if (strength_percent > 100u) strength_percent = 100u;

    const uint64_t n = pu->total_nodes;

    for (uint64_t k = 0; k < n; ++k) {
        const uint8_t ts = pu->ur_grid ? pu->ur_grid[k].type_state : 0u;
        const uint64_t partner = pu->reg_source
            ? pu->reg_source[k].channels[PRO_DEPHASE_CHANNEL]
            : k;
        const uint64_t partner_safe = (partner < n) ? partner : k;

        uint64_t h = k * 0x9e3779b97f4a7c15ULL
            + partner_safe * 0xc2b2ae3d27d4eb4fULL
            + (uint64_t)ts * 0xbf58476d1ce4e5b9ULL;
        h = pro_splitmix64(h);

        const double u = (double)(h >> 11) / 9007199254740992.0;
        const double strength = (double)strength_percent / 100.0;
        const double phi = u * PRO_2PI * strength;

        uint32_t fx = (uint32_t)(phi * PRO_INV_2PI * 65536.0 + 0.5);
        if (fx >= 65536u) fx = 0u;

        ProAmpQ31* c4 = &pu->amp_grid[k].coeff[UR_NEGATRON_CCW];
        *c4 = pro_amp_rotate_q16(*c4, (uint16_t)fx);
    }
}

/* ==========================================================================
 * End of ProPhysics_Observer.c
 * ========================================================================== */