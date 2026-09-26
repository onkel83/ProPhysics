/* ==========================================================================
 * ProPhysics - Shared-Reference Modul (Etappe 18c + 18e + 19, Refactoring 22)
 * File: ProPhysics_Shared.c
 *
 * Kernel: 1.23.0
 * Etappe: 22
 *
 * Etappe 19: Spin-1/2-Erweiterung.
 *   - Entangle_Nodes_Singlet: markiert b als spin-flipped gegenueber a.
 *   - Get_Node_Spin_View: liest Spin-Zustand mit optionaler Flip-Anwendung.
 *
 * Spin-Flip wird in ProNode.reserved_gating kodiert (Bit 0). Kein
 * Struct-Change noetig -- Bit 0 war zuvor ungenutzt.
 *
 * Etappe 22-Refactoring:
 *   - pro_sat_i32 ersetzt die manuellen if-Ketten in Shared_Tick_Reps.
 *   - Header-Kommentare auf v3.1 aktualisiert.
 *   - PRO_NODE_SPIN_FLIP_BIT kommt jetzt aus ProPhysics_Config.h.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer: Spin-Flip-Flag
  * ========================================================================== */

static inline bool pro_node_spin_flipped(const ProUniverse* pu, uint64_t k)
{
    if (!pu || !pu->ur_grid || k >= pu->total_nodes) return false;
    return (pu->ur_grid[k].reserved_gating & PRO_NODE_SPIN_FLIP_BIT) != 0u;
}

static inline void pro_node_set_spin_flip(ProUniverse* pu, uint64_t k, int on)
{
    if (!pu || !pu->ur_grid || k >= pu->total_nodes) return;
    if (on) pu->ur_grid[k].reserved_gating |= PRO_NODE_SPIN_FLIP_BIT;
    else    pu->ur_grid[k].reserved_gating &= (uint8_t)~PRO_NODE_SPIN_FLIP_BIT;
}

/* ==========================================================================
 * Interne Helfer: Union-Find mit Pfadkompression
 * ========================================================================== */

static uint64_t pro_shared_find(ProUniverse* pu, uint64_t k)
{
    if (!pu || !pu->shared.parent) return k;
    if (k >= pu->total_nodes) return k;
    uint64_t root = k;
    while (pu->shared.parent[root] != root) root = pu->shared.parent[root];
    while (pu->shared.parent[k] != root) {
        const uint64_t next = pu->shared.parent[k];
        pu->shared.parent[k] = root;
        k = next;
    }
    return root;
}

static uint64_t pro_shared_find_const(const ProUniverse* pu, uint64_t k)
{
    if (!pu || !pu->shared.parent) return k;
    if (k >= pu->total_nodes) return k;
    while (pu->shared.parent[k] != k) k = pu->shared.parent[k];
    return k;
}

/* ==========================================================================
 * Sync
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Shared_Sync(ProUniverse* pu)
{
    if (!pu || !pu->shared.parent) return;
    if (!pu->shared.active) return;
    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const uint64_t r = pro_shared_find(pu, k);
        if (r != k) pu->amp_grid[k] = pu->amp_grid[r];
    }
}

/* ==========================================================================
 * Entangle / Dissociate
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Entangle_Nodes(ProUniverse* pu, uint64_t a, uint64_t b)
{
    if (!pu || !pu->amp_grid || !pu->shared.parent) return false;
    if (a >= pu->total_nodes || b >= pu->total_nodes) return false;
    if (a == b) return true;

    uint64_t ra = pro_shared_find(pu, a);
    uint64_t rb = pro_shared_find(pu, b);
    if (ra == rb) return true;

    if (ra != a) {
        pu->amp_grid[a] = pu->amp_grid[ra];
        pu->shared.parent[a] = a;
        pu->shared.parent[ra] = a;
        ra = a;
    }
    pu->shared.parent[rb] = a;
    pu->shared.active = 1u;
    ProPhysics_Shared_Sync(pu);
    return true;
}

/* Etappe 19: Singlet-Variante -- markiert b als spin-flipped. */
PROPHYSICS_API bool ProPhysics_Entangle_Nodes_Singlet(
    ProUniverse* pu, uint64_t a, uint64_t b)
{
    if (!ProPhysics_Entangle_Nodes(pu, a, b)) return false;

    /* b wird als spin-flipped markiert. Der Wurzel-Knoten a bleibt normal.
     * Alle Non-Roots der b-Klasse erben das Flag beim naechsten Sync. */
    pro_node_set_spin_flip(pu, b, 1);
    return true;
}

PROPHYSICS_API bool ProPhysics_Dissociate_Node(ProUniverse* pu, uint64_t a)
{
    if (!pu || !pu->amp_grid || !pu->shared.parent) return false;
    if (a >= pu->total_nodes) return false;
    const uint64_t ra = pro_shared_find(pu, a);
    if (ra != a) {
        pu->amp_grid[a] = pu->amp_grid[ra];
        pu->shared.parent[a] = a;
        pro_node_set_spin_flip(pu, a, 0);
        ProPhysics_Shared_Sync(pu);
        return true;
    }
    ProPhysics_Shared_Sync(pu);
    uint64_t other = UINT64_MAX;
    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        if (k == a) continue;
        if (pu->shared.parent[k] == a) { other = k; break; }
    }
    if (other == UINT64_MAX) {
        pro_node_set_spin_flip(pu, a, 0);
        return true;
    }
    pu->amp_grid[other] = pu->amp_grid[a];
    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        if (k == a || k == other) continue;
        if (pu->shared.parent[k] == a) pu->shared.parent[k] = other;
    }
    pu->shared.parent[other] = other;
    pu->shared.parent[a] = a;
    pro_node_set_spin_flip(pu, a, 0);
    ProPhysics_Shared_Sync(pu);
    return true;
}

/* ==========================================================================
 * Query
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Is_Entangled(
    const ProUniverse* pu, uint64_t a, uint64_t b)
{
    if (!pu || !pu->shared.parent) return false;
    if (a >= pu->total_nodes || b >= pu->total_nodes) return false;
    return pro_shared_find_const(pu, a) == pro_shared_find_const(pu, b);
}

PROPHYSICS_API uint64_t ProPhysics_Get_Representative(
    const ProUniverse* pu, uint64_t a)
{
    if (!pu || !pu->shared.parent) return a;
    if (a >= pu->total_nodes) return a;
    return pro_shared_find_const(pu, a);
}

PROPHYSICS_API uint64_t ProPhysics_Shared_Class_Count(const ProUniverse* pu)
{
    if (!pu || !pu->shared.parent) return pu ? pu->total_nodes : 0u;
    if (!pu->shared.active) return pu->total_nodes;
    uint64_t count = 0;
    for (uint64_t k = 0; k < pu->total_nodes; ++k)
        if (pro_shared_find_const(pu, k) == k) count++;
    return count;
}

/* ==========================================================================
 * Spin-Flip-Query (Etappe 19)
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Is_Spin_Flipped(
    const ProUniverse* pu, uint64_t k)
{
    return pro_node_spin_flipped(pu, k);
}

/* Etappe 19: Spin-View lesen. Wenn der Knoten spin-flipped ist, wird die
 * {up, down}-Komponente transformiert (c_up, c_down) -> (c_down, -c_up). */
PROPHYSICS_API bool ProPhysics_Get_Node_Spin_View(
    const ProUniverse* pu, uint64_t k,
    int32_t* out_re_up, int32_t* out_re_down)
{
    if (!pu || !pu->amp_grid || !out_re_up || !out_re_down) return false;
    if (k >= pu->total_nodes) return false;

    const ProAmpQ31 c_up = pu->amp_grid[k].coeff[UR_POSITRON_CW];
    const ProAmpQ31 c_down = pu->amp_grid[k].coeff[UR_POSITRON_CCW];

    if (pro_node_spin_flipped(pu, k)) {
        /* Flip: (c_up, c_down) -> (c_down, -c_up) */
        *out_re_up = pro_amp_real(c_down);
        *out_re_down = -pro_amp_real(c_up);
    }
    else {
        *out_re_up = pro_amp_real(c_up);
        *out_re_down = pro_amp_real(c_down);
    }
    return true;
}

/* ==========================================================================
 * Klassen-Tick (Etappe 18e, unveraendert)
 *
 * Sammelt Kanten zwischen verschiedenen Klassen, rotiert die Repraesentanten
 * paarweise, synchronisiert anschliessend alle Mitglieder.
 *
 * Etappe 22-Refactoring: pro_sat_i32 fuer die 8 Saturationen.
 * ========================================================================== */

#define SHARED_MAX_PAIRS 1024u

typedef struct {
    uint64_t rep_a;
    uint64_t rep_b;
    uint32_t count;
} SharedKantenPaar;

PROPHYSICS_API void ProPhysics_Shared_Tick_Reps(
    ProUniverse* pu, uint32_t phase_step_q15)
{
    if (!pu || !pu->shared.parent || !pu->amp_grid) return;
    if (!pu->shared.active) return;
    (void)phase_step_q15;

    ProPhysics_Shared_Sync(pu);

    SharedKantenPaar pairs[SHARED_MAX_PAIRS];
    uint32_t n_pairs = 0u;

    for (uint64_t k = 0; k < pu->total_nodes; ++k) {
        const uint64_t rep_a = pro_shared_find(pu, k);
        const ProRegister* r = &pu->reg_source[k];

        for (uint8_t ch = 0; ch < (uint8_t)CHANNELS_MAX; ++ch) {
            if (ch == (uint8_t)PRO_EPR_CHANNEL) continue;
            if (ch == (uint8_t)PRO_DEPHASE_CHANNEL) continue;

            const uint64_t nb = r->channels[ch];
            if (nb >= pu->total_nodes || nb == k) continue;
            if (nb <= k) continue;

            const uint64_t rep_b = pro_shared_find(pu, nb);
            if (rep_a == rep_b) continue;
            if (rep_a == k && rep_b == nb) continue;

            uint64_t ra = rep_a, rb = rep_b;
            if (ra > rb) { uint64_t t = ra; ra = rb; rb = t; }

            uint32_t slot = UINT32_MAX;
            for (uint32_t i = 0; i < n_pairs; ++i) {
                if (pairs[i].rep_a == ra && pairs[i].rep_b == rb) {
                    slot = i; break;
                }
            }
            if (slot == UINT32_MAX) {
                if (n_pairs >= SHARED_MAX_PAIRS) continue;
                slot = n_pairs++;
                pairs[slot].rep_a = ra;
                pairs[slot].rep_b = rb;
                pairs[slot].count = 0u;
            }
            pairs[slot].count++;
        }
    }

    const double theta_rad =
        ((double)PRO_DEFAULT_TRANSPORT_THETA_Q15 / 32768.0) * PRO_2PI * 0.5;

    for (uint32_t i = 0; i < n_pairs; ++i) {
        const uint64_t rep_a = pairs[i].rep_a;
        const uint64_t rep_b = pairs[i].rep_b;

        double theta_eff = theta_rad * (double)pairs[i].count;
        if (theta_eff > PRO_2PI * 0.25) theta_eff = PRO_2PI * 0.25;

        const double c = cos(theta_eff);
        const double s = sin(theta_eff);

        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            const int64_t ar = (int64_t)pro_amp_real(pu->amp_grid[rep_a].coeff[b]);
            const int64_t ai = (int64_t)pro_amp_imag(pu->amp_grid[rep_a].coeff[b]);
            const int64_t br = (int64_t)pro_amp_real(pu->amp_grid[rep_b].coeff[b]);
            const int64_t bi = (int64_t)pro_amp_imag(pu->amp_grid[rep_b].coeff[b]);

            const int64_t na_re = (int64_t)llround(c * (double)ar - s * (double)bi);
            const int64_t na_im = (int64_t)llround(c * (double)ai + s * (double)br);
            const int64_t nb_re = (int64_t)llround(c * (double)br - s * (double)ai);
            const int64_t nb_im = (int64_t)llround(c * (double)bi + s * (double)ar);

            pu->amp_grid[rep_a].coeff[b] =
                pro_amp_pack(pro_sat_i32(na_re), pro_sat_i32(na_im));
            pu->amp_grid[rep_b].coeff[b] =
                pro_amp_pack(pro_sat_i32(nb_re), pro_sat_i32(nb_im));
        }
    }

    ProPhysics_Shared_Sync(pu);
}

/* ==========================================================================
 * End of ProPhysics_Shared.c
 * ========================================================================== */