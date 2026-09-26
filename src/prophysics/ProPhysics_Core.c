/* ==========================================================================
 * ProPhysics - Core Modul
 * File: ProPhysics_Core.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Lifecycle, Topologie, Amp-Grid-Zugriff, Injektoren und
 * Kernel-Tick-Orchestrierung.
 *
 * Verantwortlich fuer:
 *   - ProPhysics_Initialize / ProPhysics_Free
 *   - ProPhysics_Link_Nodes / Unlink_Node / Set_Edge_Phase
 *   - ProPhysics_Inject_Momentum / Spawn_Body
 *   - ProPhysics_Set_Node_Amplitude / Get_Node_Amplitude
 *   - ProPhysics_Sync_Amp_From_Type
 *   - ProPhysics_Advance_Internal_Clocks
 *   - ProPhysics_Tick (Kernel-Tick-Orchestrierung)
 *
 * Interne Helfer:
 *   - Debug-Ringbuffer (pro_debug_push / pro_debug_reset)
 *   - Register-Default-Init (pro_registers_self_init)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Debug-Ringbuffer
  *
  * Single-Thread, lock-frei. Der Kernel schreibt EPR-Propagations-Events
  * hier hinein; ein externer Konsument kann sie spaeter auslesen. Kein
  * printf im Hotpath.
  *
  * Overflow-Verhalten: neue Events werden verworfen, wenn der Ring voll
  * ist. Der Zaehler `dropped` zaehlt die verworfenen Events. Alte Events
  * bleiben erhalten.
  *
  * Ringgroesse ist Zweierpotenz (PRO_DEBUG_RING_SIZE), daher Bit-Maske
  * statt Modulo (R1-konform).
  * ========================================================================== */

#define PRO_DEBUG_RING_MASK ((uint32_t)(PRO_DEBUG_RING_SIZE - 1u))

typedef enum {
    PRO_DEBUG_EVENT_EPR_ENQUEUE = 0,
    PRO_DEBUG_EVENT_EPR_DELIVER = 1
} ProDebugEventKind;

typedef struct {
    uint64_t tick;
    uint64_t src;
    uint64_t dst;
    uint8_t  helicity;
    uint8_t  kind;
    uint8_t  _pad[6];
} ProDebugEvent;

typedef struct {
    ProDebugEvent events[PRO_DEBUG_RING_SIZE];
    uint32_t      head;        /* naechster Schreibslot */
    uint32_t      tail;        /* naechster Leseslot */
    uint64_t      dropped;     /* verworfene Events */
} ProDebugRing;

static ProDebugRing g_debug_ring;

/* Push: schreibt ein Event. Verwirft bei vollem Ring und zaehlt. */
static inline void pro_debug_push(uint64_t tick, uint64_t src, uint64_t dst,
    uint8_t helicity, uint8_t kind)
{
    const uint32_t next = (g_debug_ring.head + 1u) & PRO_DEBUG_RING_MASK;
    if (next == g_debug_ring.tail) {
        g_debug_ring.dropped++;
        return;
    }
    ProDebugEvent* e = &g_debug_ring.events[g_debug_ring.head];
    e->tick = tick;
    e->src = src;
    e->dst = dst;
    e->helicity = helicity;
    e->kind = kind;
    g_debug_ring.head = next;
}

/* Reset: leert den Ring. Wird von ProPhysics_Initialize aufgerufen. */
static inline void pro_debug_reset(void)
{
    g_debug_ring.head = 0u;
    g_debug_ring.tail = 0u;
    g_debug_ring.dropped = 0u;
}

/* ==========================================================================
 * Interne Helfer
 * ========================================================================== */

 /* Setzt alle Kanaele eines Register-Arrays auf Self-Loop.
  *   regs[i].channels[c] = i fuer alle i in [0, n), c in [0, CHANNELS_MAX).
  *
  * Wird an zwei Stellen genutzt:
  *   - ProPhysics_Initialize: fuer reg_source und reg_target.
  *   - ProPhysics_Tick: Lazy-Init von reg_target, falls NULL. */
static void pro_registers_self_init(ProRegister* regs, uint64_t n)
{
    if (!regs) return;
    for (uint64_t i = 0; i < n; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c) {
            regs[i].channels[c] = i;
        }
    }
}

/* ==========================================================================
 * Advance_Internal_Clocks -- innere Uhr
 *
 * Pro Knoten mit type_state != NEUTRAL, PHOTON:
 *   - Lorentz-Faktor gamma_inv = sqrt(1 - v^2) aus momentum_phase (Q7).
 *   - Phasen-Akkumulator waechst mit 256 * gamma_inv pro Tick.
 *   - Bei Ueberlauf 65536: Helizitaet toggelt zwischen 1 und 2.
 *
 * Rein lesend auf amp_grid; nur ur_grid wird modifiziert.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Advance_Internal_Clocks(ProUniverse* pu)
{
    if (!pu || !pu->ur_grid) return;

    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        ProNode* n = &pu->ur_grid[i];
        const uint8_t st = n->type_state;

        if (st == UR_NEUTRAL || st == UR_PHOTON) continue;

        double v = (double)n->momentum_phase * PRO_INV_127;
        if (v >= 0.999) v = 0.999;
        const double gamma_inv = sqrt(1.0 - v * v);

        uint32_t delta = (uint32_t)(256.0 * gamma_inv + 0.5);
        if (delta == 0) delta = 1;

        uint32_t phase = (uint32_t)n->phase_accumulator + delta;
        if (phase >= 65536u) {
            phase -= 65536u;
            n->field_helicity = (n->field_helicity == 1) ? 2 : 1;
        }
        n->phase_accumulator = (uint16_t)phase;
    }
}

/* ==========================================================================
 * Lifecycle
 *
 * Alle Hot-Arrays werden via pro_aligned_calloc (Cache-Line-aligned)
 * angelegt. Rest-Arrays via calloc. Overflow-Schutz durch alloc_size_ok
 * vor jeder Allokation.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count)
{
    if (!pu) return;
    pu->_shared_tick_count = 0;
    if (pu->_core_magic == PRO_CORE_MAGIC) {
        ProPhysics_Free(pu);
    }
    else {
        memset(pu, 0, sizeof(*pu));
    }

    if (node_count == 0) return;

    /* Overflow-Checks vor jeder Allokation. */
    if (!alloc_size_ok(node_count, sizeof(ProNode)))       return;
    if (!alloc_size_ok(node_count, sizeof(ProRegister)))   return;
    if (!alloc_size_ok(node_count, sizeof(ProEdge)))       return;
    if (!alloc_size_ok(node_count, sizeof(ProAmpVector)))  return;
    if (!alloc_size_ok(node_count, sizeof(uint16_t)))      return;
    if (!alloc_size_ok(node_count, sizeof(uint64_t)))      return;
    if (node_count > (uint64_t)(SIZE_MAX / (uint64_t)CHANNELS_MAX)) return;

    const uint64_t edge_count = node_count * (uint64_t)CHANNELS_MAX;
    if (!alloc_size_ok(edge_count, sizeof(ProEdge)))       return;

    /* Hot-Arrays (aligned). */
    ProNode* grid = (ProNode*)pro_aligned_calloc(
        (size_t)node_count, sizeof(ProNode));
    ProRegister* regs = (ProRegister*)pro_aligned_calloc(
        (size_t)node_count, sizeof(ProRegister));
    ProRegister* regs_target = (ProRegister*)pro_aligned_calloc(
        (size_t)node_count, sizeof(ProRegister));
    ProEdge* edges = (ProEdge*)pro_aligned_calloc(
        (size_t)edge_count, sizeof(ProEdge));
    ProAmpVector* amps = (ProAmpVector*)pro_aligned_calloc(
        (size_t)node_count, sizeof(ProAmpVector));
    ProAmpVector* amps_scr = (ProAmpVector*)pro_aligned_calloc(
        (size_t)node_count, sizeof(ProAmpVector));
    uint16_t* uf = (uint16_t*)pro_aligned_calloc(
        (size_t)node_count, sizeof(uint16_t));
    uint64_t* shared_parent = (uint64_t*)pro_aligned_calloc(
        (size_t)node_count, sizeof(uint64_t));

    /* Rest-Arrays (calloc). */
    ProDensityMatrix* dms = (ProDensityMatrix*)calloc(
        PRO_DENSITY_MAX, sizeof(ProDensityMatrix));
    ProAmpTensorPair* tps = (ProAmpTensorPair*)calloc(
        PRO_TENSOR_MAX_PAIRS, sizeof(ProAmpTensorPair));
    uint8_t* tmarks = (uint8_t*)calloc(
        (size_t)node_count, sizeof(uint8_t));
    ProFockState* focks = (ProFockState*)calloc(
        PRO_FOCK_MAX_STATES, sizeof(ProFockState));
    ProTensorDensity* tds = (ProTensorDensity*)calloc(
        PRO_TENSOR_DENSITY_MAX, sizeof(ProTensorDensity));
    ProFockDensity* fds = (ProFockDensity*)calloc(
        PRO_FOCK_DENSITY_MAX, sizeof(ProFockDensity));

    /* Vollstaendigkeitspruefung. */
    if (!grid || !regs || !regs_target
        || !edges || !amps || !amps_scr || !uf || !shared_parent
        || !tps || !tmarks || !focks || !tds || !fds) {
        pro_aligned_free(grid);
        pro_aligned_free(regs);
        pro_aligned_free(regs_target);
        pro_aligned_free(edges);
        pro_aligned_free(amps);
        pro_aligned_free(amps_scr);
        pro_aligned_free(uf);
        pro_aligned_free(shared_parent);
        free(dms); free(tps); free(tmarks);
        free(focks); free(tds); free(fds);
        memset(pu, 0, sizeof(*pu));
        return;
    }

    /* Register-Default: Self-Loop in allen Kanaelen.
     * SU(2)-Link auf Identitaet (a = 1, b = 0 in Skala 2^30). */
    pro_registers_self_init(regs, node_count);
    pro_registers_self_init(regs_target, node_count);

    for (uint64_t i = 0; i < node_count; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c) {
            ProEdge* e = &edges[i * (uint64_t)CHANNELS_MAX + (uint64_t)c];
            e->su2_a_re = PRO_SU2_IDENT_RE;
            e->su2_a_im = PRO_SU2_IDENT_IM;
            e->su2_b_re = 0;
            e->su2_b_im = 0;
        }
    }

    /* Default: jeder Knoten ist seine eigene Klasse. */
    for (uint64_t i = 0; i < node_count; ++i) {
        shared_parent[i] = i;
    }

    pu->total_nodes = node_count;
    pu->grid_dim = 0u;
    pu->grid_dim_shift = 0u;
    pu->grid_dim_mask = 0u;
    pu->grid_ndim = 0u;

    pu->ur_grid = grid;
    pu->reg_source = regs;
    pu->reg_target = regs_target;
    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;
    pu->_tick_pad = 0u;

    pu->edge_phases = edges;
    pu->amp_grid = amps;
    pu->amp_scratch = amps_scr;

    pu->tensor_pairs = tps;
    pu->tensor_pair_count = 0;
    pu->tensor_pair_capacity = PRO_TENSOR_MAX_PAIRS;

    pu->tensor_marks = tmarks;

    pu->fock_states = focks;
    pu->fock_state_count = 0;
    pu->fock_state_capacity = PRO_FOCK_MAX_STATES;

    pu->density_matrices = dms;
    pu->density_count = 0;
    pu->density_capacity = PRO_DENSITY_MAX;

    pu->tensor_densities = tds;
    pu->tensor_density_count = 0;
    pu->tensor_density_capacity = PRO_TENSOR_DENSITY_MAX;

    pu->fock_densities = fds;
    pu->fock_density_count = 0;
    pu->fock_density_capacity = PRO_FOCK_DENSITY_MAX;

    pu->auto_sync_tensor = 0;
    pu->auto_sync_density = 0;
    for (int i = 0; i < 6; ++i) pu->_tensor_flags_pad[i] = 0u;

    pu->last_theta_q15 = 0;
    pu->last_c_num = 0;
    pu->last_s_num = 0;
    pu->last_denom = 1;
    pu->transport_cache_valid = 0u;
    for (int i = 0; i < 7; ++i) pu->_transport_pad[i] = 0u;

    pu->u_field = uf;

    for (uint64_t i = 0; i < node_count; ++i) {
        uint64_t h = i * 0x9e3779b97f4a7c15ULL;
        h = pro_splitmix64(h);
        pu->u_field[i] = (uint16_t)(h >> 48);
    }

    pu->epr_delay_ticks = 0;
    pu->epr_debug = 0;
    pu->epr_signal_count = 0;

    pu->shared.parent = shared_parent;
    pu->shared.active = 0u;
    for (int i = 0; i < 7; ++i) pu->shared._pad[i] = 0u;

    /* Dirac-Struktur. Default deaktiviert (R7). */
    pu->dirac_active = 0u;
    pu->dirac_gamma_basis = (uint8_t)PRO_GAMMA_BASIS_DIRAC;
    pu->_dirac_pad[0] = 0u;
    pu->_dirac_pad[1] = 0u;
    pu->dirac_mass_q15 = 0;

    /* SU(2)-Eichfeld. Default deaktiviert (R7). */
    pu->su2_active = 0u;
    pu->su2_gauge_basis = (uint8_t)PRO_SU2_BASIS_PAULI;
    pu->_su2_pad[0] = 0u;
    pu->_su2_pad[1] = 0u;
    pu->su2_coupling_q15 = 0;

    /* SU(2)-Dynamik. Default deaktiviert (R7). */
    pu->su2_dynamics_active = 0u;
    pu->_su2b_pad[0] = 0u;
    pu->_su2b_pad[1] = 0u;
    pu->_su2b_pad[2] = 0u;
    pu->su2_yang_mills_q15 = (int32_t)PRO_SU2_YM_DEFAULT_Q15;

    pu->_core_magic = PRO_CORE_MAGIC;
    pu->_core_pad = 0u;

    pro_debug_reset();

    ProPhysics_Sync_Amp_From_Type(pu);
}

PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu)
{
    if (!pu) return;

    pro_aligned_free(pu->ur_grid);
    pro_aligned_free(pu->reg_source);
    pro_aligned_free(pu->reg_target);
    pro_aligned_free(pu->edge_phases);
    pro_aligned_free(pu->amp_grid);
    pro_aligned_free(pu->amp_scratch);
    pro_aligned_free(pu->u_field);
    pro_aligned_free(pu->shared.parent);

    free(pu->tensor_marks);
    free(pu->tensor_pairs);
    free(pu->fock_states);
    free(pu->density_matrices);
    free(pu->tensor_densities);
    free(pu->fock_densities);

    memset(pu, 0, sizeof(*pu));
}

/* ==========================================================================
 * Amplitude-Layer Zugriff
 * ========================================================================== */

PROPHYSICS_API ProAmpVector* ProPhysics_Get_Amp_Grid(ProUniverse* pu)
{
    return pu ? pu->amp_grid : NULL;
}

PROPHYSICS_API bool ProPhysics_Set_Node_Amplitude(ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx,
    int32_t re_q31, int32_t im_q31)
{
    if (!pu || !pu->amp_grid) return false;
    if (node_idx >= pu->total_nodes) return false;
    if (basis_idx >= PRO_AMP_BASIS_SIZE) return false;
    pu->amp_grid[node_idx].coeff[basis_idx] = pro_amp_pack(re_q31, im_q31);
    return true;
}

PROPHYSICS_API bool ProPhysics_Get_Node_Amplitude(const ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx,
    int32_t* out_re_q31, int32_t* out_im_q31)
{
    if (!pu || !pu->amp_grid) return false;
    if (node_idx >= pu->total_nodes) return false;
    if (basis_idx >= PRO_AMP_BASIS_SIZE) return false;
    if (!out_re_q31 || !out_im_q31) return false;

    const ProAmpQ31 a = pu->amp_grid[node_idx].coeff[basis_idx];
    *out_re_q31 = pro_amp_real(a);
    *out_im_q31 = pro_amp_imag(a);
    return true;
}

PROPHYSICS_API void ProPhysics_Sync_Amp_From_Type(ProUniverse* pu)
{
    if (!pu || !pu->amp_grid || !pu->ur_grid) return;

    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        ProAmpVector* v = &pu->amp_grid[i];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            v->coeff[b] = 0;
        }
        const uint8_t st = pu->ur_grid[i].type_state;
        if (st < PRO_AMP_BASIS_SIZE) {
            v->coeff[st] = pro_amp_pack(INT32_MAX, 0);
        }
    }
}

/* ==========================================================================
 * Topologie
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Link_Nodes(ProUniverse* pu,
    uint64_t src, uint64_t target, uint8_t channel_idx)
{
    if (!pu || !pu->reg_source) return;
    if (channel_idx >= CHANNELS_MAX) return;
    if (src >= pu->total_nodes || target >= pu->total_nodes) return;
    pu->reg_source[src].channels[channel_idx] = target;
}

PROPHYSICS_API void ProPhysics_Unlink_Node(ProUniverse* pu,
    uint64_t src, uint8_t channel_idx)
{
    if (!pu || !pu->reg_source) return;
    if (channel_idx >= CHANNELS_MAX) return;
    if (src >= pu->total_nodes) return;
    pu->reg_source[src].channels[channel_idx] = src;
}

/* ==========================================================================
 * Kantenphase (U(1)-Connection)
 *
 * Modifiziert nur ProEdge.phase und ProEdge.type. Die SU(2)-Felder
 * bleiben unangetastet.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Set_Edge_Phase(ProUniverse* pu,
    uint64_t src, uint8_t ch, double phase_radians, uint8_t edge_type)
{
    if (!pu || !pu->edge_phases) return;
    if (src >= pu->total_nodes) return;
    if (ch >= CHANNELS_MAX) return;
    if (edge_type > PRO_EDGE_PRODUCT) return;

    if (!(phase_radians >= -1e300 && phase_radians <= 1e300)) return;
    double p = fmod(phase_radians, PRO_2PI);
    if (p < 0.0) p += PRO_2PI;

    uint32_t fx = (uint32_t)(p * PRO_INV_2PI * 65536.0 + 0.5);
    if (fx >= 65536u) fx = 0;

    ProEdge* e = &pu->edge_phases[src * (uint64_t)CHANNELS_MAX + ch];
    e->phase = (uint16_t)fx;
    e->type = edge_type;
}

/* ==========================================================================
 * Injektoren
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Inject_Momentum(ProUniverse* pu,
    uint64_t node_idx, double velocity_ratio)
{
    if (!pu || !pu->ur_grid) return;
    if (node_idx >= pu->total_nodes) return;
    if (!(velocity_ratio >= 0.0)) return;
    if (velocity_ratio > 1.0) velocity_ratio = 1.0;

    int phase_val = (int)(velocity_ratio * 127.0 + 0.5);
    if (phase_val < 0)   phase_val = 0;
    if (phase_val > 255) phase_val = 255;
    pu->ur_grid[node_idx].momentum_phase = (uint8_t)phase_val;
}

PROPHYSICS_API void ProPhysics_Spawn_Body(ProUniverse* pu, uint64_t node_idx,
    uint8_t state, uint8_t helicity)
{
    if (!pu || !pu->ur_grid) return;
    if (node_idx >= pu->total_nodes) return;
    if (!is_valid_state(state)) return;
    if (helicity > 2) return;

    pu->ur_grid[node_idx].type_state = state;
    pu->ur_grid[node_idx].field_helicity = helicity;
    pu->ur_grid[node_idx].phase_accumulator = 0;

    if (pu->amp_grid) {
        ProAmpVector* v = &pu->amp_grid[node_idx];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) v->coeff[b] = 0;
        if (state < PRO_AMP_BASIS_SIZE) {
            v->coeff[state] = pro_amp_pack(INT32_MAX, 0);
        }
    }
}

/* ==========================================================================
 * ProPhysics_Tick -- Kernel-Tick-Orchestrierung
 *
 * Reihenfolge:
 *   1. Lazy-Init reg_target (falls NULL).
 *   2. current_cpu_tick++.
 *   3. Advance_Internal_Clocks.
 *   4. EPR-Propagation (Debug-Events in Ring).
 *   5. Apply_Amp_Step.
 *   6. Apply_Guiding_Equation.
 *   7. Graph-Plastizitaet (callback, swap).
 *   8. global_entropy_index.
 *
 * R7: callback == NULL erlaubt. Dann ueberspringt der Tick den
 * Graph-Plastizitaets-Block komplett (kein swap, kein Callback-Aufruf).
 * Der Rest laeuft bit-identisch zu einem Tick mit No-Op-Callback.
 *
 * Kein div/mod im Hotpath (R1). Alle Zugriffe sind durch total_nodes
 * begrenzt.
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Tick(ProUniverse* pu,
    ProPhysics_RuleCallback callback)
{
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->edge_phases) return;

    /* --- 1. Lazy-Init reg_target. --- */
    if (!pu->reg_target) {
        pu->reg_target = (ProRegister*)pro_aligned_calloc(
            (size_t)pu->total_nodes, sizeof(ProRegister));
        if (!pu->reg_target) return;
        pro_registers_self_init(pu->reg_target, pu->total_nodes);
    }

    pu->current_cpu_tick++;

    /* --- 3. Innere Uhr. --- */
    ProPhysics_Advance_Internal_Clocks(pu);

    /* --- 4. EPR-Propagationsphase. ---
     *
     * Iteration ueber alle Knoten. Fuer jeden EPR-Kanal mit aktiver
     * Singlet/Triplet-Verbindung:
     *   - Wenn die Quell-Helizitaet sich aendert und kein Signal
     *     unterwegs ist: enqueue (pending_ticks = delay + 1).
     *   - Wenn pending_ticks > 0: dekrementieren, bei 0 an
     *     Partner-Knoten ausliefern.
     *
     * Debug-Events landen im Ring, nicht in printf. */
    const uint32_t delay = pu->epr_delay_ticks;
    const uint32_t dbg = pu->epr_debug;

    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        const uint64_t partner = pu->reg_source[i].channels[PRO_EPR_CHANNEL];
        if (partner >= pu->total_nodes || partner == i) continue;
        if (partner < i) continue;

        ProEdge* e = &pu->edge_phases[i * (uint64_t)CHANNELS_MAX
            + PRO_EPR_CHANNEL];
        if (e->type != PRO_EDGE_SINGLET && e->type != PRO_EDGE_TRIPLET)
            continue;

        const uint8_t src_h = pu->ur_grid[i].field_helicity;

        if (src_h != e->last_sent_helicity && e->pending_ticks == 0) {
            e->pending_helicity = src_h;
            e->pending_ticks = (uint16_t)(delay + 1u);
            e->last_sent_helicity = src_h;
            pu->epr_signal_count++;

            if (dbg >= 1 && pu->epr_signal_count <= 32) {
                pro_debug_push(pu->current_cpu_tick, i, partner, src_h,
                    (uint8_t)PRO_DEBUG_EVENT_EPR_ENQUEUE);
            }
        }

        if (e->pending_ticks > 0) {
            e->pending_ticks--;
            if (e->pending_ticks == 0) {
                pu->ur_grid[partner].field_helicity = e->pending_helicity;
                if (dbg >= 1 && pu->epr_signal_count <= 32) {
                    pro_debug_push(pu->current_cpu_tick, i, partner,
                        e->pending_helicity,
                        (uint8_t)PRO_DEBUG_EVENT_EPR_DELIVER);
                }
            }
        }
    }

    /* --- 5. Unitare Evolution auf amp_grid. --- */
    ProPhysics_Apply_Amp_Step(pu, PRO_DEFAULT_PHASE_STEP_Q15);

    /* --- 6. U6 -- Anzeige rekonstruieren. --- */
    ProPhysics_Apply_Guiding_Equation(pu);

    /* --- 7. Graph-Plastizitaet. ---
     *
     * Nur wenn ein Callback uebergeben wurde. Bei NULL bleibt die
     * Topologie unveraendert; kein swap. R7-konform. */
    if (callback) {
        uint64_t active_interactions = 0;
        memcpy(pu->reg_target, pu->reg_source,
            (size_t)pu->total_nodes * sizeof(ProRegister));

        for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
            const uint8_t current_state = pu->ur_grid[idx].type_state;
            if (current_state == UR_NEUTRAL) continue;

            const bool current_is_photon = (current_state == UR_PHOTON);

            for (int ch = 0; ch < CHANNELS_MAX; ch++) {
                if (ch == PRO_EPR_CHANNEL) continue;

                const uint64_t target_ptr =
                    pu->reg_source[idx].channels[ch];
                if (target_ptr >= pu->total_nodes || target_ptr == idx)
                    continue;

                const uint8_t target_state =
                    pu->ur_grid[target_ptr].type_state;
                if (target_state == UR_NEUTRAL && !current_is_photon)
                    continue;

                uint8_t next_state = current_state;
                uint8_t next_target_state = target_state;

                /* Kanaele in lokale Puffer kopieren. Der Callback
                 * darf sie in-place permutieren. */
                uint64_t current_ch[CHANNELS_MAX];
                uint64_t target_ch[CHANNELS_MAX];
                for (int c = 0; c < CHANNELS_MAX; c++) {
                    current_ch[c] = pu->reg_target[idx].channels[c];
                    target_ch[c] =
                        pu->reg_target[target_ptr].channels[c];
                }

                callback(current_state, target_state,
                    current_ch, target_ch,
                    &next_state, &next_target_state,
                    idx, pu->total_nodes);

                (void)next_state;
                (void)next_target_state;

                for (int c = 0; c < CHANNELS_MAX; c++) {
                    pu->reg_target[idx].channels[c] = current_ch[c];
                    pu->reg_target[target_ptr].channels[c] = target_ch[c];
                }

                active_interactions++;
            }
        }

        /* Swap: reg_target wird zur neuen reg_source. */
        ProRegister* temp = pu->reg_source;
        pu->reg_source = pu->reg_target;
        pu->reg_target = temp;

        pu->global_entropy_index = (uint32_t)active_interactions;
    }
    else {
        pu->global_entropy_index = 0u;
    }
}

/* ==========================================================================
 * End of ProPhysics_Core.c
 * ========================================================================== */