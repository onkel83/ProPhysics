/* ==========================================================================
 * ProPhysics - EPR Modul
 * File: ProPhysics_EPR.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Verantwortlich fuer:
 *   - EPR-Paarmessungen (4 Varianten: sharp + type_state-Guard,
 *     sharp + amp_grid-Guard, Born-Kollaps + Typ-Relation,
 *     Born-Kollaps + Kantenphase)
 *   - EPR-Propagation Runtime-API (Delay, Debug)
 *   - Apparat-Subgraph fuer den Superdeterminismus-Test
 *
 * Design-Entscheidungen:
 *   - Alle Messfunktionen sind REIN LESEND. Sie modifizieren amp_grid NICHT.
 *   - Option B: type_state ist Anzeige, nicht Fundament. _Amp und _Graph
 *     nutzen keinen type_state-Guard.
 *   - _Collapse und _Graph nutzen pro_uniform01(rng); der Aufrufer
 *     liefert den xoshiro256**-State.
 *
 * Interne Helfer:
 *   - pro_epr_validate_pair (Guard, ersetzt 3x)
 *   - pro_epr_sharp_measure_pair (Sharp-Sampling, ersetzt 2x)
 *   - pro_epr_measure_born_collapse (Born-Collapse, ersetzt 2x)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/EPR.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer
  * ========================================================================== */

  /* Vollstaendiger Pre-Flight-Check fuer eine EPR-Paarmessung.
   *
   * Prueft:
   *   - pu und alle benoetigten Arrays nicht NULL
   *   - out_a und out_b nicht NULL
   *   - node_a in Range
   *   - partner in Range und != node_a
   *   - bei require_type_state: beide Knoten != UR_NEUTRAL
   *   - bei require_edge_type: edge->type in {SINGLET, TRIPLET}
   *   - bei require_amp_nonzero: mindestens ein Koeffizient != 0 auf beiden
   *
   * Rueckgabe:
   *   partner-Index bei Erfolg, oder UINT64_MAX bei Fehler.
   *
   * Ersetzt 3x wiederholte Guard-Bloecke (_Pair, _Collapse, _Graph). */
static uint64_t pro_epr_validate_pair(
    const ProUniverse* pu,
    uint64_t node_a,
    int require_type_state,
    int require_edge_type,
    int require_amp_nonzero)
{
    if (!pu) return UINT64_MAX;
    if (!pu->ur_grid || !pu->reg_source || !pu->edge_phases) return UINT64_MAX;
    if (!pu->amp_grid) return UINT64_MAX;
    if (node_a >= pu->total_nodes) return UINT64_MAX;

    const uint64_t partner = pu->reg_source[node_a].channels[PRO_EPR_CHANNEL];
    if (partner >= pu->total_nodes || partner == node_a) return UINT64_MAX;

    if (require_type_state) {
        if (pu->ur_grid[node_a].type_state == UR_NEUTRAL) return UINT64_MAX;
        if (pu->ur_grid[partner].type_state == UR_NEUTRAL) return UINT64_MAX;
    }

    if (require_edge_type) {
        const ProEdge* e = &pu->edge_phases[node_a * (uint64_t)CHANNELS_MAX
            + PRO_EPR_CHANNEL];
        if (e->type != PRO_EDGE_SINGLET && e->type != PRO_EDGE_TRIPLET) {
            return UINT64_MAX;
        }
    }

    if (require_amp_nonzero) {
        const ProAmpVector* va = &pu->amp_grid[node_a];
        const ProAmpVector* vb = &pu->amp_grid[partner];
        int va_nonzero = 0, vb_nonzero = 0;
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            if (va->coeff[b] != 0) va_nonzero = 1;
            if (vb->coeff[b] != 0) vb_nonzero = 1;
        }
        if (!va_nonzero || !vb_nonzero) return UINT64_MAX;
    }

    return partner;
}

/* Sharp-Measure-Paar: wendet pro_measure_sharp auf beide Seiten an.
 * Deterministisch. Kein RNG.
 *
 * Ersetzt 2x wiederholte Sequenz:
 *   lambda_a = pro_amp_to_lambda(...)
 *   lambda_b = pro_amp_to_lambda(...)
 *   *out_a = pro_measure_sharp(theta_a, lambda_a)
 *   *out_b = pro_measure_sharp(theta_b, lambda_b) */
static void pro_epr_sharp_measure_pair(
    const ProAmpVector* va,
    const ProAmpVector* vb,
    double theta_a, double theta_b,
    int* out_a, int* out_b)
{
    const double lambda_a = pro_amp_to_lambda(va);
    const double lambda_b = pro_amp_to_lambda(vb);
    *out_a = pro_measure_sharp(theta_a, lambda_a);
    *out_b = pro_measure_sharp(theta_b, lambda_b);
}

/* Born-Collapse-Sequenz fuer ein EPR-Paar.
 *
 *   1. A wird bei theta_a via Born-Regel gemessen.
 *   2. v_A kollabiert auf {theta_a, theta_a + pi}.
 *   3. v_B kollabiert mit: lambda_b = lambda_a_collapsed + phase_offset_rad.
 *   4. B wird bei theta_b auf dem kollabierten Zustand gemessen.
 *
 * Der Parameter phase_offset_rad kodiert die Korrelationsrelation:
 *   _Collapse: phase_offset = (SINGLET ? pi : 0)
 *   _Graph:    phase_offset = 2*pi * e->phase / 65536
 *
 * Ersetzt 2x wiederholte Born-Collapse-Sequenz. */
static void pro_epr_measure_born_collapse(
    const ProAmpVector* va,
    double theta_a, double theta_b,
    double phase_offset_rad,
    uint64_t rng[4],
    int* out_a, int* out_b)
{
    /* 1. A's Ausgang via Born-Regel. */
    const double lambda_a = pro_amp_to_lambda(va);
    const double p_a_plus_cos = cos((theta_a - lambda_a) * 0.5);
    const double p_a_plus = p_a_plus_cos * p_a_plus_cos;
    const int a = (pro_uniform01(rng) < p_a_plus) ? +1 : -1;

    /* 2. Kollaps von A. */
    const double lambda_a_collapsed = (a == +1)
        ? theta_a
        : (theta_a + PRO_2PI * 0.5);

    /* 3. Kollaps von B aus Relation. */
    const double lambda_b_collapsed = lambda_a_collapsed + phase_offset_rad;

    /* 4. B's Ausgang via Born-Regel auf dem kollabierten Zustand. */
    const double p_b_plus_cos = cos((theta_b - lambda_b_collapsed) * 0.5);
    const double p_b_plus = p_b_plus_cos * p_b_plus_cos;
    const int b = (pro_uniform01(rng) < p_b_plus) ? +1 : -1;

    *out_a = a;
    *out_b = b;
}

/* ==========================================================================
 * EPR-Propagation Runtime-API
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Set_EPR_Delay(ProUniverse* pu, uint32_t delay_ticks)
{
    if (!pu) return;
    pu->epr_delay_ticks = delay_ticks;
}

PROPHYSICS_API uint32_t ProPhysics_Get_EPR_Delay(const ProUniverse* pu)
{
    return pu ? pu->epr_delay_ticks : 0u;
}

PROPHYSICS_API void ProPhysics_Set_EPR_Debug(ProUniverse* pu, uint32_t level)
{
    if (!pu) return;
    pu->epr_debug = level;
}

/* ==========================================================================
 * Measure_EPR_Pair -- klassische scharfe Messung
 *
 * Aelteste Variante der vier Messfunktionen. Nutzt type_state-Guard
 * und Kantentyp-Guard. Deterministisch (rng wird ignoriert).
 *
 * Projektionsachse lambda wird aus dem Amplitudenvektor gelesen.
 * ========================================================================== */

PROPHYSICS_API int ProPhysics_Measure_EPR_Pair(ProUniverse* pu,
    uint64_t node_a,
    double   theta_a,
    double   theta_b,
    uint64_t rng[4],
    int* out_a, int* out_b)
{
    if (!out_a || !out_b) return 0;

    const uint64_t partner = pro_epr_validate_pair(
        pu, node_a,
        1 /* require_type_state */,
        1 /* require_edge_type */,
        0 /* require_amp_nonzero */);
    if (partner == UINT64_MAX) return 0;

    (void)rng;   /* deterministisch */

    pro_epr_sharp_measure_pair(
        &pu->amp_grid[node_a],
        &pu->amp_grid[partner],
        theta_a, theta_b,
        out_a, out_b);
    return 1;
}

/* ==========================================================================
 * Measure_EPR_Pair_Amp -- scharfe Messung ohne type_state-Guard
 *
 * Wie Measure_EPR_Pair, aber Paar-Existenzpruefung laeuft ueber amp_grid
 * (nicht-triviale Amplituden) statt ueber type_state. Nach Option B darf
 * die Messung nicht von der Anzeige abhaengen.
 *
 * Kein RNG. Rein lesend.
 * ========================================================================== */

PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Amp(
    ProUniverse* pu,
    uint64_t node_a,
    double   theta_a,
    double   theta_b,
    int* out_a, int* out_b)
{
    if (!out_a || !out_b) return 0;

    const uint64_t partner = pro_epr_validate_pair(
        pu, node_a,
        0 /* require_type_state */,
        0 /* require_edge_type */,
        1 /* require_amp_nonzero */);
    if (partner == UINT64_MAX) return 0;

    pro_epr_sharp_measure_pair(
        &pu->amp_grid[node_a],
        &pu->amp_grid[partner],
        theta_a, theta_b,
        out_a, out_b);
    return 1;
}

/* ==========================================================================
 * Measure_EPR_Pair_Collapse -- Kollaps-Messung mit Typ-Relation
 *
 * Kollaps-Messung gemaess U4:
 *   1. A wird bei theta_a via Born-Regel gemessen.
 *   2. v_A kollabiert auf den entsprechenden Eigenzustand; wegen U4
 *      kollabiert v_B mit, Relation durch Singlet/Triplet:
 *        Singlet: lambda_B = lambda_A + pi
 *        Triplet: lambda_B = lambda_A
 *   3. B wird bei theta_b auf dem kollabierten Zustand gemessen.
 *
 * Resultat: E(Delta) = -+cos(Delta), CHSH erreicht 2*sqrt(2).
 *
 * Rein lesend. Nutzt rng fuer die beiden Born-Samples.
 * ========================================================================== */

PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Collapse(
    ProUniverse* pu,
    uint64_t node_a,
    double   theta_a,
    double   theta_b,
    uint64_t rng[4],
    int* out_a, int* out_b)
{
    if (!out_a || !out_b || !rng) return 0;

    const uint64_t partner = pro_epr_validate_pair(
        pu, node_a,
        1 /* require_type_state */,
        1 /* require_edge_type */,
        0 /* require_amp_nonzero */);
    if (partner == UINT64_MAX) return 0;

    const ProEdge* e = &pu->edge_phases[node_a * (uint64_t)CHANNELS_MAX
        + PRO_EPR_CHANNEL];
    const double phase_offset_rad = (e->type == PRO_EDGE_SINGLET)
        ? (PRO_2PI * 0.5)
        : 0.0;

    pro_epr_measure_born_collapse(
        &pu->amp_grid[node_a],
        theta_a, theta_b,
        phase_offset_rad,
        rng,
        out_a, out_b);
    return 1;
}

/* ==========================================================================
 * Measure_EPR_Pair_Graph -- Kollaps-Messung mit Kantenphase
 *
 * Wie _Collapse, aber die Korrelationsrelation kommt aus der
 * Kantenphase e->phase (U(1)-Connection):
 *
 *   lambda_B_collapsed = lambda_A_collapsed + phase_rad
 *   phase_rad = 2*pi * e->phase / 65536
 *
 * Damit folgt die Korrelationsstruktur der U(1)-Struktur, die aus U2
 * abgeleitet ist. e->type wird NICHT gelesen.
 *
 * Rein lesend.
 * ========================================================================== */

PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Graph(
    ProUniverse* pu,
    uint64_t node_a,
    double   theta_a,
    double   theta_b,
    uint64_t rng[4],
    int* out_a, int* out_b)
{
    if (!out_a || !out_b || !rng) return 0;

    const uint64_t partner = pro_epr_validate_pair(
        pu, node_a,
        1 /* require_type_state */,
        0 /* require_edge_type */,
        0 /* require_amp_nonzero */);
    if (partner == UINT64_MAX) return 0;

    const ProEdge* e = &pu->edge_phases[node_a * (uint64_t)CHANNELS_MAX
        + PRO_EPR_CHANNEL];
    const double phase_offset_rad = ((double)e->phase / 65536.0) * PRO_2PI;

    pro_epr_measure_born_collapse(
        &pu->amp_grid[node_a],
        theta_a, theta_b,
        phase_offset_rad,
        rng,
        out_a, out_b);
    return 1;
}

/* ==========================================================================
 * Apparat-Subgraph
 *
 * REINE Topologie- und Datentraeger-Helfer. Keine Physik, NICHT Teil
 * der U1-U5-Dynamik.
 *
 * Der Apparat "berechnet" theta_a nicht emergent. Er erlaubt dem
 * Test-Harness, die geteilte Vergangenheit zwischen Apparat und
 * Teilchen topologisch zu kodieren.
 * ========================================================================== */

 /* Verdrahtet einen dim x dim-Torus ab base_node.
  * Kanaele: 0 = +y, 1 = -y, 2 = +x, 3 = -x. Andere Kanale = Self-Loop. */
PROPHYSICS_API void ProPhysics_Init_Apparatus(ProUniverse* pu,
    uint64_t base_node, uint32_t dim)
{
    if (!pu || !pu->reg_source || !pu->ur_grid) return;
    if (dim < 2) return;

    const uint64_t nodes = (uint64_t)dim * (uint64_t)dim;
    if (base_node + nodes > pu->total_nodes) return;

    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t y_n = (uint64_t)(y == 0 ? dim - 1 : y - 1);
        const uint64_t y_s = (uint64_t)(y == dim - 1 ? 0 : y + 1);
        const uint64_t y_c = (uint64_t)y;

        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = base_node + y_c * dim + x;
            pu->reg_source[idx].channels[0] = base_node + y_n * dim + x;
            pu->reg_source[idx].channels[1] = base_node + y_s * dim + x;
            pu->reg_source[idx].channels[2] =
                base_node + y_c * dim + (x == dim - 1 ? 0 : x + 1);
            pu->reg_source[idx].channels[3] =
                base_node + y_c * dim + (x == 0 ? dim - 1 : x - 1);
            for (uint8_t c = 4; c < (uint8_t)CHANNELS_MAX; ++c) {
                pu->reg_source[idx].channels[c] = idx;
            }
        }
    }
}

/* Setzt phase_accumulator aller Apparat-Knoten auf phase_fx. */
PROPHYSICS_API void ProPhysics_Set_Apparatus_Phase(ProUniverse* pu,
    uint64_t base_node, uint32_t dim, uint16_t phase_fx)
{
    if (!pu || !pu->ur_grid) return;
    if (dim < 1) return;

    const uint64_t nodes = (uint64_t)dim * (uint64_t)dim;
    if (base_node + nodes > pu->total_nodes) return;

    for (uint64_t k = 0; k < nodes; ++k) {
        pu->ur_grid[base_node + k].phase_accumulator = phase_fx;
    }
}

/* Liest den Mittelwert der Phase als Radiant in [0, 2*pi). */
PROPHYSICS_API double ProPhysics_Read_Apparatus_Theta(const ProUniverse* pu,
    uint64_t base_node, uint32_t dim)
{
    if (!pu || !pu->ur_grid) return 0.0;
    if (dim < 1) return 0.0;

    const uint64_t nodes = (uint64_t)dim * (uint64_t)dim;
    if (base_node + nodes > pu->total_nodes) return 0.0;

    uint64_t sum = 0;
    for (uint64_t k = 0; k < nodes; ++k) {
        sum += pu->ur_grid[base_node + k].phase_accumulator;
    }
    const uint32_t mean_fx = (uint32_t)((sum / nodes) & 0xFFFFu);
    return ((double)mean_fx / 65536.0) * PRO_2PI;
}

/* ==========================================================================
 * End of ProPhysics_EPR.c
 * ========================================================================== */