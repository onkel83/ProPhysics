/* ==========================================================================
 * ProPhysics - Gauge Modul (Etappe 4-18, Refactoring 22)
 * File: ProPhysics_Gauge.c
 * Architecture: U(1)-Eichstruktur, lambda-Feld, Amplitudenrotation,
 *               Mess-Helfer fuer den Test-Harness.
 * Kernel: 1.23.0
 * Etappe: 22
 *
 * Etappe 22-Refactoring:
 *   - pro_sat_i32 in pro_amp_rotate_q16.
 *   - pro_wilson_validate_path Helper fuer Pfad-Validierung
 *     (shared mit ProPhysics_SU2.c, definiert in ProPhysics_Internal.h).
 *   - Header-Kommentare auf v3.1 aktualisiert.
 *
 * Etappe 17b: ProPhysics_Apply_Local_Phase_Plate.
 * Etappe 18:  ProPhysics_Apply_Coulomb_Phase_Field_3D.
 *
 * Hinweis zur U(1)- vs. SU(2)-Wilson-Loop:
 *   Der U(1)-Loop ist der abelsche Spezialfall des SU(2)-Loops mit
 *   diagonalen Links (a = e^(i*phi), b = 0). Er wird bewusst separat
 *   implementiert:
 *     - exakte Ganzzahl-Arithmetik (uint16, keine Q30-Rundung),
 *     - O(N) statt O(N * 30) Kosten pro Kante,
 *     - die U(1)-Tests pruefen exakte Werte (W4 = 32768 exakt).
 *   Der SU(2)-Loop ist rueckwaerts-iterierend (non-abelisch,
 *   Path-Ordered), der U(1)-Loop vorwaerts (abelsch, Reihenfolge
 *   irrelevant).
 *
 * Verantwortlich fuer:
 *   - pro_amp_to_lambda, pro_measure_sharp (Wrapper)
 *   - pro_trig_init, pro_amp_rotate_q16 (Q30-Tabelle, lazy init)
 *   - ProPhysics_Wilson_Loop
 *   - ProPhysics_Global_Phase
 *   - ProPhysics_Get_Born_Probability
 *   - ProPhysics_Make_Lambda_Field
 *   - ProPhysics_Apply_Local_Gauge
 *   - ProPhysics_Apply_Local_Phase_Plate   (Etappe 17b)
 *   - ProPhysics_Apply_Coulomb_Phase_Field_3D (Etappe 18)
 *   - ProPhysics_Compute_Lambda (oeffentlicher Wrapper)
 *   - ProPhysics_Sharp_Measure  (oeffentlicher Wrapper)
 *   - ProPhysics_Type_State_Measure
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Amplituden-Vektor -> Projektionsachse lambda
  *
  * lambda ist der Winkel im 2-dim Unterraum {UR_POSITRON_CW (=1),
  * UR_NEGATRON_CCW (=4)}, den der Amplitudenvektor aufspannt.
  * Annahme: beide Koeffizienten sind reell.
  * ========================================================================== */

double pro_amp_to_lambda(const ProAmpVector* v)
{
    const double c1 = (double)pro_amp_real(v->coeff[UR_POSITRON_CW]);
    const double c4 = (double)pro_amp_real(v->coeff[UR_NEGATRON_CCW]);
    double lam = 2.0 * atan2(c4, c1);
    if (lam < 0.0) lam += PRO_2PI;
    if (lam >= PRO_2PI) lam -= PRO_2PI;
    return lam;
}

/* ==========================================================================
 * Q30-Trig-Tabelle fuer komplexe Rotation
 *
 * Modul-lokal, lazy initialisiert. Q30 statt Q15, weil bei Q15 der
 * Faktor cos^2 + sin^2 ~ (32767/32768)^2 systematisch < 1 ist und ueber
 * viele Rotationen zu sichtbarem Normverlust akkumuliert (~6% nach
 * 1000 Ticks).
 *
 * Thread-Safety: single-threaded (Konsistenz mit Kernel-Design).
 * ========================================================================== */

static int32_t PRO_COS_Q30[256];
static int32_t PRO_SIN_Q30[256];
static bool    PRO_TRIG_READY = false;

static void pro_trig_init(void)
{
    if (PRO_TRIG_READY) return;
    for (int k = 0; k < 256; ++k) {
        const double phi = (double)k * (2.0 * PRO_2PI / 256.0);
        const double c = cos(phi);
        const double s = sin(phi);
        int32_t ci = (int32_t)(c * 1073741824.0 + (c >= 0.0 ? 0.5 : -0.5));
        int32_t si = (int32_t)(s * 1073741824.0 + (s >= 0.0 ? 0.5 : -0.5));
        PRO_COS_Q30[k] = ci;
        PRO_SIN_Q30[k] = si;
    }
    PRO_TRIG_READY = true;
}

/* Rotation c * exp(i*phi), phi in Q16.
 * Q30-Tabelle + Shift um 30. Round-to-Nearest eliminiert Negativ-Bias.
 *
 * Wertebereiche:
 *   re, im in [-2^31, 2^31)
 *   cs, sn in [-2^30, 2^30)
 *   Produkt in [-2^61, 2^61]  ->  int64 reicht.
 *
 * Etappe 22-Refactoring: pro_sat_i32 statt manueller if-Ketten. */
ProAmpQ31 pro_amp_rotate_q16(ProAmpQ31 c, uint16_t phase_fx)
{
    pro_trig_init();
    const uint8_t idx = (uint8_t)(phase_fx >> 8);
    const int32_t cs = PRO_COS_Q30[idx];
    const int32_t sn = PRO_SIN_Q30[idx];

    const int64_t re = (int64_t)pro_amp_real(c);
    const int64_t im = (int64_t)pro_amp_imag(c);

    const int64_t new_re = pro_round_shift_q30(re * cs - im * sn);
    const int64_t new_im = pro_round_shift_q30(re * sn + im * cs);

    return pro_amp_pack(pro_sat_i32(new_re), pro_sat_i32(new_im));
}

/* ==========================================================================
 * Wilson-Loop (U(1))
 *
 * Summe der Kantenphasen mod 65536 ueber einen geschlossenen Pfad.
 * Spezialfall path_len == 1: Self-Loop, gibt die eine Phase zurueck.
 *
 * Etappe 22-Refactoring: Pfad-Validierung zentralisiert ueber
 * pro_wilson_validate_path (shared mit ProPhysics_SU2.c).
 *
 * Reihenfolge: vorwaerts. Fuer die abelsche U(1)-Gruppe ist die
 * Reihenfolge irrelevant. Die SU(2)-Version iteriert rueckwaerts
 * (Path-Ordered, W(C) = U_{n-1} * ... * U_0).
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Wilson_Loop(const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t* path_channels,
    uint32_t        path_len,
    uint16_t* out_phase_fx)
{
    if (!out_phase_fx) return;
    *out_phase_fx = 0u;

    if (!pro_wilson_validate_path(pu, path_nodes, path_channels, path_len))
        return;

    if (path_len == 1u) {
        const uint64_t x = path_nodes[0];
        const uint8_t  c = path_channels[0];
        const ProEdge* e = &pu->edge_phases[x * (uint64_t)CHANNELS_MAX + c];
        *out_phase_fx = e->phase;
        return;
    }

    uint32_t sum = 0u;
    for (uint32_t k = 0; k < path_len; ++k) {
        const uint64_t x = path_nodes[k];
        const uint8_t  c = path_channels[k];
        const ProEdge* e = &pu->edge_phases[x * (uint64_t)CHANNELS_MAX + c];
        sum += (uint32_t)e->phase;
    }

    *out_phase_fx = (uint16_t)(sum & 0xFFFFu);
}

/* ==========================================================================
 * Globale Phase
 *
 * Multipliziert jeden Amplitudenvektor mit exp(i*phase_fx).
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Global_Phase(ProUniverse* pu, uint16_t phase_fx)
{
    if (!pu || !pu->amp_grid) return;
    if (phase_fx == 0u) return;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        ProAmpVector* v = &pu->amp_grid[k];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            v->coeff[b] = pro_amp_rotate_q16(v->coeff[b], phase_fx);
        }
    }
}

/* ==========================================================================
 * Born-Wahrscheinlichkeit eines Basisindex
 *
 * P(b | k) = |c_b(k)|^2 / Sum_j |c_j(k)|^2. Reine Leseoperation.
 *
 * Etappe 22-Refactoring: pro_amp_abs2 fuer uint64-sichere Berechnung. */

PROPHYSICS_API double ProPhysics_Get_Born_Probability(const ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx)
{
    if (!pu || !pu->amp_grid) return 0.0;
    if (node_idx >= pu->total_nodes) return 0.0;
    if (basis_idx >= PRO_AMP_BASIS_SIZE) return 0.0;

    const ProAmpVector* v = &pu->amp_grid[node_idx];

    uint64_t sq_basis = 0u;
    uint64_t sum = 0u;
    for (uint8_t j = 0; j < PRO_AMP_BASIS_SIZE; ++j) {
        const uint64_t sq = pro_amp_abs2(v->coeff[j]);
        sum += sq;
        if (j == basis_idx) sq_basis = sq;
    }
    if (sum == 0u) return 0.0;
    return (double)sq_basis / (double)sum;
}

/* ==========================================================================
 * lambda-Feld (lokale Eichparameter)
 *
 * splitmix64-Stream -> deterministische Q16-Phasen pro Knoten.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Make_Lambda_Field(uint64_t seed,
    uint16_t* lambda_fx, uint64_t total_nodes)
{
    if (!lambda_fx) return;

    uint64_t h = seed;
    for (uint64_t k = 0; k < total_nodes; ++k) {
        h += 0x9e3779b97f4a7c15ULL;
        uint64_t z = h;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        z = z ^ (z >> 31);
        lambda_fx[k] = (uint16_t)(z & 0xFFFFu);
    }
}

/* ==========================================================================
 * Lokale Eichtransformation
 *
 *   1. Kantenphase:  phi(x->y) <- phi(x->y) + lambda(y) - lambda(x) mod 2^16.
 *      Exakt ganzzahlig, keine Rundung.
 *   2. Amplitude:    psi(x)   <- exp(i*lambda(x)) * psi(x).
 *      Mit Q30-Rotation aus pro_amp_rotate_q16.
 *
 * Der Wilson-Loop ist invariant, weil sich die lambda-Terme auf einem
 * geschlossenen Pfad paarweise wegheben.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_Local_Gauge(ProUniverse* pu,
    const uint16_t* lambda_fx)
{
    if (!pu || !pu->amp_grid || !pu->edge_phases) return;
    if (!lambda_fx) return;

    const uint64_t n = pu->total_nodes;

    /* --- 1. Kantenphasen anpassen. --- */
    for (uint64_t x = 0; x < n; ++x) {
        const ProRegister* r = &pu->reg_source[x];
        ProEdge* edges_x = &pu->edge_phases[x * (uint64_t)CHANNELS_MAX];

        for (uint8_t ch = 0; ch < (uint8_t)CHANNELS_MAX; ++ch) {
            const uint64_t y = r->channels[ch];
            if (y >= n || y == x) continue;

            const int32_t d = (int32_t)lambda_fx[y] - (int32_t)lambda_fx[x];
            const uint32_t new_phase =
                ((uint32_t)edges_x[ch].phase + (uint32_t)d) & 0xFFFFu;
            edges_x[ch].phase = (uint16_t)new_phase;
        }
    }

    /* --- 2. Amplituden rotieren. --- */
    for (uint64_t k = 0; k < n; ++k) {
        const uint16_t lam = lambda_fx[k];
        if (lam == 0u) continue;

        ProAmpVector* v = &pu->amp_grid[k];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            v->coeff[b] = pro_amp_rotate_q16(v->coeff[b], lam);
        }
    }
}

/* ==========================================================================
 * Etappe 17b: Ortsabhaengige Phase Plate.
 *
 * Multipliziert die Amplitude an allen Knoten im 2D-Rechteck
 *   [x0, x0+w) x [y0, y0+h)
 * mit exp(i * phase_q15 / 65536 * 2*pi).
 *
 * Pro Aufruf wird der Winkel EINMAL angewendet. Fuer eine dauerhafte
 * Phase Plate (die auf eine durchlaufende Welle als Potential wirkt)
 * muss der Aufrufer die Funktion pro Tick aufrufen; die totale
 * akkumulierte Phase ist dann N_ticks_in_region * phase_q15.
 *
 * Winkel-Normierung: phase_q15 = 65536 entspricht 2*pi, konsistent
 * mit ProEdge.phase.
 *
 * Rein unitaer. Kein Div/Mod (R1), kein malloc (R2).
 *
 * 3D-Support: aktuell nicht implementiert (return bei grid_ndim == 3).
 * Der Doppelspalt-Test arbeitet in 2D.
 * ========================================================================== */
PROPHYSICS_API void ProPhysics_Apply_Local_Phase_Plate(
    ProUniverse* pu,
    uint32_t x0, uint32_t y0,
    uint32_t w, uint32_t h,
    uint16_t phase_q15)
{
    if (!pu || !pu->amp_grid) return;
    if (phase_q15 == 0u) return;
    if (w == 0u || h == 0u) return;
    if (pu->grid_ndim == 3u) return;   /* nicht implementiert */

    const uint32_t dim = pu->grid_dim;
    if (dim == 0u) return;

    if (x0 >= dim || y0 >= dim) return;
    const uint32_t x1 = (x0 + w < dim) ? (x0 + w) : dim;
    const uint32_t y1 = (y0 + h < dim) ? (y0 + h) : dim;

    for (uint32_t y = y0; y < y1; ++y) {
        const uint64_t row_base = (uint64_t)y * dim;
        for (uint32_t x = x0; x < x1; ++x) {
            const uint64_t idx = row_base + x;
            if (idx >= pu->total_nodes) continue;
            ProAmpVector* v = &pu->amp_grid[idx];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                v->coeff[b] = pro_amp_rotate_q16(v->coeff[b], phase_q15);
            }
        }
    }
}

/* ==========================================================================
 * Etappe 18: Coulomb-Phase-Field 3D
 *
 * Radialsymmetrisches 1/r-Phase-Feld um ein Zentrum (cx, cy, cz) im
 * 3D-Torus. Pro Tick wird jeder Knoten (x, y, z) um
 *
 *   phase_q15(r) = strength_q15 / r_eff
 *
 * rotiert, mit
 *
 *   r = sqrt(dx^2 + dy^2 + dz^2)
 *   r_eff = r + softening_q8 / 256
 *
 * und Torus-Abstaenden
 *
 *   dx = min(|x-cx|, dim-|x-cx|)   analog dy, dz
 *
 * Das entspricht einer Schroedinger-Zeitentwicklung mit V(r) = -K/r
 * (Coulomb-Potential) fuer K proportional zur strength_q15.
 *
 * Vorab-Cache:
 *   phase_q15(r) als Funktion von r_q8 (r in Q8, ganzzahlig,
 *   0..COULOMB_CACHE_SIZE-1) in einem static-Array. Nur bei
 *   Aenderung der strength_q15 wird der Cache neu berechnet.
 *   Damit ist der Hotpath Div-frei (R1-konform).
 *
 * 2D-Support: return bei grid_ndim != 3.
 * 3D-Layout: bit-interleaved (z, y, x) mit shift = log2(dim).
 *
 * Rein unitaer (pro_amp_rotate_q16), keine Allokation (R2).
 * ========================================================================== */

 /* Cache-Groesse: r_q8 in [0, 255] -> r in [0, 1) in Q8-Schritten.
  * Fuer r >= 1 wird die Phase direkt berechnet (selten, ~1/256 aller
  * Knoten bei dim=32). Damit ist der Cache-Bereich ausreichend fuer
  * den Div-freien Pfad bei kleinen r. */
#define COULOMB_CACHE_SIZE 256u

static int32_t g_coulomb_phase_cache[COULOMB_CACHE_SIZE];
static int32_t g_coulomb_last_strength_q15 = 0;
static uint32_t g_coulomb_last_softening_q8 = 0;
static bool    g_coulomb_cache_valid = false;

/* Cache mit strength_q15/r_eff fuer r_q8 = 0..255 vorberechnen.
 * softening_q8 ist die Regularisierung in Q8. */
static void pro_coulomb_cache_fill(int32_t strength_q15,
    uint32_t softening_q8)
{
    for (uint32_t r_q8 = 0u; r_q8 < COULOMB_CACHE_SIZE; ++r_q8) {
        /* r_eff_q8 = r_q8 + softening_q8, in Q8 */
        const uint32_t r_eff_q8 = r_q8 + softening_q8;
        if (r_eff_q8 == 0u) {
            /* r = 0 und softening = 0: Phase 0 setzen (Grenzfall). */
            g_coulomb_phase_cache[r_q8] = 0;
            continue;
        }
        /* phase_q15 = strength_q15 * 256 / r_eff_q8
         * = strength_q15 / (r_eff_q8 / 256)  -- exakt dasselbe.
         * Integer-Division mit Q8-Qualitaet. */
        const int32_t phase = (int32_t)(((int64_t)strength_q15 * 256)
            / (int64_t)r_eff_q8);
        g_coulomb_phase_cache[r_q8] = phase;
    }
    g_coulomb_last_strength_q15 = strength_q15;
    g_coulomb_last_softening_q8 = softening_q8;
    g_coulomb_cache_valid = true;
}

PROPHYSICS_API void ProPhysics_Apply_Coulomb_Phase_Field_3D(
    ProUniverse* pu,
    uint32_t cx, uint32_t cy, uint32_t cz,
    int32_t  strength_q15,
    uint32_t softening_q8)
{
    if (!pu || !pu->amp_grid) return;
    if (strength_q15 == 0) return;
    if (pu->grid_ndim != 3u) return;

    const uint32_t dim = pu->grid_dim;
    if (dim == 0u || (dim & (dim - 1u)) != 0u) return;   /* Zweierpotenz */

    if (cx >= dim || cy >= dim || cz >= dim) return;

    /* Cache pruefen / aktualisieren. */
    if (!g_coulomb_cache_valid
        || g_coulomb_last_strength_q15 != strength_q15
        || g_coulomb_last_softening_q8 != softening_q8)
    {
        pro_coulomb_cache_fill(strength_q15, softening_q8);
    }

    uint32_t shift = 0u;
    while ((1u << shift) < dim) shift++;
    const uint32_t mask = dim - 1u;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const uint32_t x = (uint32_t)k & mask;
        const uint32_t y = ((uint32_t)k >> shift) & mask;
        const uint32_t z = ((uint32_t)k >> (2u * shift)) & mask;

        /* Torus-Abstand zum Zentrum. */
        uint32_t dx = (x >= cx) ? (x - cx) : (cx - x);
        if (dx > dim / 2u) dx = dim - dx;
        uint32_t dy = (y >= cy) ? (y - cy) : (cy - y);
        if (dy > dim / 2u) dy = dim - dy;
        uint32_t dz = (z >= cz) ? (z - cz) : (cz - z);
        if (dz > dim / 2u) dz = dim - dz;

        /* r_q8 = sqrt(dx^2 + dy^2 + dz^2) in Q8.
         * Wir nutzen double sqrt hier einmal pro Knoten. Das ist
         * akzeptabel: 32768 sqrt-Aufrufe pro Tick ~ 300 us bei dim=32.
         * Fuer hoehere Leistung koennte man eine Q8-Lookup-Tabelle
         * aufbauen; das ist aber erst bei dim >= 64 noetig. */
        const double r = sqrt((double)(dx * dx + dy * dy + dz * dz));
        const uint32_t r_q8 = (uint32_t)(r * 256.0 + 0.5);

        int32_t phase_q15;
        if (r_q8 < COULOMB_CACHE_SIZE) {
            phase_q15 = g_coulomb_phase_cache[r_q8];
        }
        else {
            const uint32_t r_eff_q8 = r_q8 + softening_q8;
            if (r_eff_q8 == 0u) continue;
            phase_q15 = (int32_t)(((int64_t)strength_q15 * 256)
                / (int64_t)r_eff_q8);
        }

        if (phase_q15 == 0) continue;

        /* phase_q15 in Q16-Phase umrechnen:
         * strength_q15 ist in Q15-Einheiten (2*pi/32768 pro Einheit).
         * Um daraus einen Q16-Phasenwert zu machen:
         *   phase_q16 = phase_q15 * 2. */
        int64_t phase_q16_i64 = (int64_t)phase_q15 * 2;
        phase_q16_i64 = phase_q16_i64 % 65536;
        if (phase_q16_i64 < 0) phase_q16_i64 += 65536;

        const uint16_t phase_q16 = (uint16_t)phase_q16_i64;

        ProAmpVector* v = &pu->amp_grid[k];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            v->coeff[b] = pro_amp_rotate_q16(v->coeff[b], phase_q16);
        }
    }
}

/* ==========================================================================
 * Oeffentliche Wrapper fuer den Test-Harness (Etappe 6f)
 *
 * pro_amp_to_lambda und pro_measure_sharp sind im Kernel static (interne
 * Linkage). Diese Wrapper machen sie ueber die oeffentliche API
 * zugaenglich, ohne ihre Semantik zu aendern.
 * ========================================================================== */

PROPHYSICS_API double ProPhysics_Compute_Lambda(const ProUniverse* pu,
    uint64_t node_idx)
{
    if (!pu || !pu->amp_grid) return 0.0;
    if (node_idx >= pu->total_nodes) return 0.0;
    return pro_amp_to_lambda(&pu->amp_grid[node_idx]);
}

PROPHYSICS_API int ProPhysics_Sharp_Measure(double theta, double lambda)
{
    return pro_measure_sharp(theta, lambda);
}

/* ==========================================================================
 * type_state-basierte Messung (U6c)
 *
 * out = sign(cos(theta - lambda_state)), mit lambda_state = 0 fuer
 * UR_POSITRON_CW und pi fuer UR_NEGATRON_CCW. Deterministisch, kein RNG.
 * ========================================================================== */

PROPHYSICS_API int ProPhysics_Type_State_Measure(double theta, uint8_t type_state)
{
    const double lambda_state = (type_state == UR_POSITRON_CW)
        ? 0.0
        : PRO_2PI * 0.5;

    return (cos(theta - lambda_state) > 0.0) ? +1 : -1;
}

/* ==========================================================================
 * End of ProPhysics_Gauge.c
 * ========================================================================== */