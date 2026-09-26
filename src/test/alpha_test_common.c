/* ==========================================================================
 * ProPhysics - Alpha-Test Common
 * File: alpha_test_common.c
 * Version: 3.1 (Etappe 22 + Refactoring)
 *
 * Refactoring 22-Aenderungen:
 *   - pro_engine_tick ruft jetzt ProPhysics_Tick direkt (Kernel),
 *     nicht mehr ProPhysics_SDK_Execute_Plastizitaet_Tick (SDK-Wrapper).
 *     Beide sind semantisch identisch; der direkte Weg ist der
 *     saubere Test-Harness-Zugriff.
 *   - Header-Kommentar auf v3.1 aktualisiert.
 *   - Konstanten (PRO_INV_2PI, PRO_2PI) kommen aus ProPhysics_Config.h.
 *
 * Enthaelt Utility-Funktionen fuer den Test-Harness:
 *   - DynamicTracker (Tick-Zaehler)
 *   - PRNG (xoshiro256** Wrapper)
 *   - ProEngineContext (Lifecycle-Wrapper)
 *   - Torus-Verdrahtung (2D und 3D)
 *   - Injektoren (Photon, EPR-Paar)
 *   - Snapshot (Capture / Restore / Free)
 * ========================================================================== */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 /* ==========================================================================
  * DynamicTracker
  * ========================================================================== */

void dynamic_tracker_init(DynamicTracker* tracker, uint32_t sample_window) {
    if (!tracker) return;
    tracker->prev_reserved_gating = 0;
    tracker->prev_entropy_index = 0;
    for (int c = 0; c < 4; c++) tracker->prev_channels[c] = 0;
    tracker->flip_accumulator = 0;
    tracker->sample_window_ticks = (sample_window > 0) ? sample_window : 256;
    tracker->grid_frequency_hz = 0.0f;
}

void track_node_dynamics(DynamicTracker* tracker,
    uint32_t current_gating,
    uint32_t current_entropy,
    const uint64_t current_channels[CHANNELS_MAX])
{
    if (!tracker) return;
    uint64_t delta_mask =
        ((uint64_t)current_gating ^ (uint64_t)tracker->prev_reserved_gating)
        | ((uint64_t)current_entropy ^ (uint64_t)tracker->prev_entropy_index)
        | (current_channels[0] ^ tracker->prev_channels[0])
        | (current_channels[1] ^ tracker->prev_channels[1])
        | (current_channels[2] ^ tracker->prev_channels[2])
        | (current_channels[3] ^ tracker->prev_channels[3]);
    tracker->flip_accumulator += (delta_mask != 0);
    tracker->prev_reserved_gating = current_gating;
    tracker->prev_entropy_index = current_entropy;
    tracker->prev_channels[0] = current_channels[0];
    tracker->prev_channels[1] = current_channels[1];
    tracker->prev_channels[2] = current_channels[2];
    tracker->prev_channels[3] = current_channels[3];
}

void evaluate_grid_frequency(DynamicTracker* tracker, uint32_t current_tick) {
    if (!tracker) return;
    if (tracker->sample_window_ticks > 0 &&
        (current_tick % tracker->sample_window_ticks) == 0) {
        tracker->grid_frequency_hz =
            (float)tracker->flip_accumulator / (float)tracker->sample_window_ticks;
        tracker->flip_accumulator = 0;
    }
}

void track_universe_dynamics(ProUniverse* pu, DynamicTracker* tracker, uint32_t tick) {
    if (!pu || !tracker || !pu->ur_grid) return;
    const uint64_t sample_node =
        ((uint64_t)(GRID_DIM / 2) * GRID_DIM) + (GRID_DIM / 2);
    if (sample_node >= pu->total_nodes) return;
    track_node_dynamics(tracker,
        pu->ur_grid[sample_node].reserved_gating,
        pu->global_entropy_index,
        pu->reg_source[sample_node].channels);
    evaluate_grid_frequency(tracker, tick);
}

/* ==========================================================================
 * PRNG (xoshiro256**)
 * ========================================================================== */

uint64_t rotl64(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

uint64_t prng_next(prng_state_t* state) {
    const uint64_t result = rotl64(state->s[1] * 5, 7) * 9;
    const uint64_t t = state->s[1] << 17;
    state->s[2] ^= state->s[0];
    state->s[3] ^= state->s[1];
    state->s[1] ^= state->s[2];
    state->s[0] ^= state->s[3];
    state->s[2] ^= t;
    state->s[3] = rotl64(state->s[3], 45);
    return result;
}

void prng_seed(prng_state_t* state, uint64_t seed) {
    for (int i = 0; i < 4; i++) {
        uint64_t z = (seed += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        state->s[i] = z ^ (z >> 31);
    }
}

uint64_t fast_map_index(uint64_t random_val, uint64_t max_nodes) {
    if (max_nodes == 0) return 0;
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned __int64 high;
    _umul128(random_val, max_nodes, &high);
    return high;
#elif defined(__SIZEOF_INT128__)
    return (uint64_t)(((ProU128)random_val * (ProU128)max_nodes) >> 64);
#else
    return random_val % max_nodes;
#endif
}

/* ==========================================================================
 * Engine-Context
 *
 * pro_engine_tick:
 *   - synct nodes -> pu.ur_grid (Alias-Zustand uebernehmen).
 *   - ruft ProPhysics_Tick (Kernel, Refactoring 22).
 *   - synct pu.ur_grid -> nodes.
 *
 * Refactoring 22: der Tick geht direkt an den Kernel, nicht mehr
 * ueber den SDK-Wrapper. Die Callback-Signatur ist ueber den Alias
 * ProPhysics_ScientificRuleCallback == ProPhysics_RuleCallback
 * strukturell identisch.
 * ========================================================================== */

void pro_engine_init(ProEngineContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ProPhysics_Initialize(&ctx->pu, TOTAL_NODES);
    if (!ctx->pu.ur_grid || !ctx->pu.reg_source) {
        fprintf(stderr, "[!] ProPhysics_Initialize failed -- out of memory?\n");
        ctx->ready = false;
        return;
    }
    ctx->nodes = (EngineNodeAlias*)calloc(TOTAL_NODES, sizeof(EngineNodeAlias));
    if (!ctx->nodes) {
        fprintf(stderr, "[!] Node alias allocation failed.\n");
        ProPhysics_Free(&ctx->pu);
        ctx->ready = false;
        return;
    }
    prng_seed(&ctx->rng, 0x12345678ULL);
    dynamic_tracker_init(&ctx->tracker, 256);
    ctx->ready = true;
}

void pro_engine_cleanup(ProEngineContext* ctx) {
    if (!ctx) return;
    ProPhysics_Free(&ctx->pu);
    if (ctx->nodes) { free(ctx->nodes); ctx->nodes = NULL; }
    ctx->ready = false;
}

void pro_engine_reset(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready) return;
    memset(ctx->pu.ur_grid, 0, ctx->pu.total_nodes * sizeof(ProNode));
    if (ctx->nodes) memset(ctx->nodes, 0, TOTAL_NODES * sizeof(EngineNodeAlias));
    for (uint64_t i = 0; i < ctx->pu.total_nodes; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            ctx->pu.reg_source[i].channels[c] = i;
    }
    ctx->pu.current_cpu_tick = 0;
    ctx->pu.global_entropy_index = 0;
    if (ctx->pu.amp_grid) ProPhysics_Sync_Amp_From_Type(&ctx->pu);
    dynamic_tracker_init(&ctx->tracker, 256);
}

static void pro_engine_sync_nodes_to_pu(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready || !ctx->nodes) return;
    for (uint64_t i = 0; i < TOTAL_NODES; ++i)
        ctx->pu.ur_grid[i].type_state = ctx->nodes[i].state;
}
static void pro_engine_sync_pu_to_nodes(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready || !ctx->nodes) return;
    for (uint64_t i = 0; i < TOTAL_NODES; ++i)
        ctx->nodes[i].state = ctx->pu.ur_grid[i].type_state;
}

void pro_engine_tick(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready) return;
    pro_engine_sync_nodes_to_pu(ctx);

    /* Refactoring 22: direkter Kernel-Tick statt SDK-Wrapper. */
    ProPhysics_Tick(&ctx->pu, ResearchPlugin_DynamicPlasticTopology);

    pro_engine_sync_pu_to_nodes(ctx);
}

/* ==========================================================================
 * Torus-Verdrahtung (2D)
 * ========================================================================== */

void init_torus(ProUniverse* pu) {
    if (!pu || !pu->reg_source || !pu->ur_grid) return;
    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
        pu->ur_grid[i].type_state = UR_NEUTRAL;
        pu->ur_grid[i].field_helicity = 0;
        pu->ur_grid[i].momentum_phase = 0;
        pu->ur_grid[i].reserved_gating = 0;
    }
    for (uint64_t y = 0; y < GRID_DIM; ++y) {
        const uint64_t y_north = (y == 0 ? GRID_DIM - 1 : y - 1) * GRID_DIM;
        const uint64_t y_south = (y == GRID_DIM - 1 ? 0 : y + 1) * GRID_DIM;
        const uint64_t y_curr = y * GRID_DIM;
        for (uint64_t x = 0; x < GRID_DIM; ++x) {
            const uint64_t idx = y_curr + x;
            pu->reg_source[idx].channels[0] = y_north + x;
            pu->reg_source[idx].channels[1] = y_south + x;
            pu->reg_source[idx].channels[2] = y_curr + (x == GRID_DIM - 1 ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_curr + (x == 0 ? GRID_DIM - 1 : x - 1);
        }
    }
    if (pu->amp_grid) ProPhysics_Sync_Amp_From_Type(pu);
}

void wire_torus(ProUniverse* pu, uint32_t dim) {
    if (!pu || !pu->reg_source || !pu->ur_grid) return;
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    for (uint64_t i = 0; i < N; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
        pu->ur_grid[i].type_state = UR_NEUTRAL;
        pu->ur_grid[i].field_helicity = 0;
    }
    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t y_n = (uint64_t)((y == 0) ? dim - 1 : y - 1) * dim;
        const uint64_t y_s = (uint64_t)((y == dim - 1) ? 0 : y + 1) * dim;
        const uint64_t y_c = (uint64_t)y * dim;
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = y_c + x;
            pu->reg_source[idx].channels[0] = y_n + x;
            pu->reg_source[idx].channels[1] = y_s + x;
            pu->reg_source[idx].channels[2] = y_c + ((x == dim - 1) ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_c + ((x == 0) ? dim - 1 : x - 1);
        }
    }
    pu->grid_dim = dim;
}

/* ==========================================================================
 * Injektoren
 * ========================================================================== */

void inject_photon_deterministic(ProUniverse* pu, prng_state_t* rng) {
    if (!pu || !rng) return;
    const uint64_t idx = fast_map_index(prng_next(rng), pu->total_nodes);
    pu->ur_grid[idx].type_state = UR_PHOTON;
    pu->ur_grid[idx].field_helicity = (uint8_t)(1 + (prng_next(rng) & 1u));
}

void inject_epr_pair_deterministic(ProUniverse* pu, prng_state_t* rng,
    uint8_t edge_type)
{
    if (!pu || !rng) return;
    const uint64_t idx1 = fast_map_index(prng_next(rng), pu->total_nodes);
    const uint64_t idx2 = fast_map_index(prng_next(rng), pu->total_nodes);
    if (idx1 == idx2) return;

    const uint64_t old_a = pu->reg_source[idx1].channels[PRO_EPR_CHANNEL];
    const uint64_t old_b = pu->reg_source[idx2].channels[PRO_EPR_CHANNEL];
    if (old_a < pu->total_nodes && old_a != idx1) {
        pu->reg_source[old_a].channels[PRO_EPR_CHANNEL] = old_a;
        ProPhysics_Set_Edge_Phase(pu, old_a, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_NONE);
    }
    if (old_b < pu->total_nodes && old_b != idx2) {
        pu->reg_source[old_b].channels[PRO_EPR_CHANNEL] = old_b;
        ProPhysics_Set_Edge_Phase(pu, old_b, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_NONE);
    }

    /* Etappe 16e': type_state als Anzeige auf UR_PHOTON setzen, damit
     * die type_state-Guards in Measure_EPR_Pair{,Collapse,Graph} passieren.
     * Nach Option B ist das ontologisch nicht fundamental -- der Guard ist
     * eine v2.3-Altlast, aber die vier Messvarianten bleiben produktiv. */
    pu->ur_grid[idx1].type_state = UR_PHOTON;
    pu->ur_grid[idx2].type_state = UR_PHOTON;
    pu->ur_grid[idx1].field_helicity = 1;
    pu->ur_grid[idx2].field_helicity = 2;

    pu->reg_source[idx1].channels[PRO_EPR_CHANNEL] = idx2;
    pu->reg_source[idx2].channels[PRO_EPR_CHANNEL] = idx1;

    ProPhysics_Set_Edge_Phase(pu, idx1, PRO_EPR_CHANNEL, 0.0, edge_type);
    ProPhysics_Set_Edge_Phase(pu, idx2, PRO_EPR_CHANNEL, 0.0, edge_type);

    if (pu->amp_grid) {
        ProAmpVector* v1 = &pu->amp_grid[idx1];
        ProAmpVector* v2 = &pu->amp_grid[idx2];
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
            v1->coeff[b] = 0;
            v2->coeff[b] = 0;
        }
        const double u = (double)(prng_next(rng) >> 11) / 9007199254740992.0;
        const double lam = u * (2.0 * M_PI);
        const double half = lam * 0.5;
        const double c = cos(half);
        const double s = sin(half);
        const int32_t a_up = (int32_t)lround(c * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(s * Q31_MAXV);
        v1->coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
        v1->coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
        if (edge_type == PRO_EDGE_SINGLET) {
            const int32_t b_up = (int32_t)lround(-s * Q31_MAXV);
            const int32_t b_dn = (int32_t)lround(c * Q31_MAXV);
            v2->coeff[UR_POSITRON_CW] = pro_amp_pack(b_up, 0);
            v2->coeff[UR_NEGATRON_CCW] = pro_amp_pack(b_dn, 0);
        }
        else {
            v2->coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            v2->coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
        }
    }
}

/* ==========================================================================
 * Snapshot
 * ========================================================================== */

bool snapshot_capture(const ProUniverse* pu, FullStateSnapshot* snap) {
    if (!pu || !snap || !pu->ur_grid || !pu->reg_source) return false;
    if (!pu->edge_phases) return false;
    snap->total_nodes = pu->total_nodes;
    snap->cpu_tick = pu->current_cpu_tick;
    snap->entropy = pu->global_entropy_index;
    const size_t gbytes = (size_t)pu->total_nodes * sizeof(ProNode);
    const size_t rbytes = (size_t)pu->total_nodes * sizeof(ProRegister);
    const size_t ebytes = (size_t)pu->total_nodes * (size_t)CHANNELS_MAX * sizeof(ProEdge);
    snap->grid = (ProNode*)malloc(gbytes);
    snap->regs = (ProRegister*)malloc(rbytes);
    snap->regs_target = pu->reg_target ? (ProRegister*)malloc(rbytes) : NULL;
    snap->edges = (ProEdge*)malloc(ebytes);
    if (!snap->grid || !snap->regs || !snap->edges
        || (pu->reg_target && !snap->regs_target)) {
        free(snap->grid); free(snap->regs);
        free(snap->regs_target); free(snap->edges);
        memset(snap, 0, sizeof(*snap));
        return false;
    }
    memcpy(snap->grid, pu->ur_grid, gbytes);
    memcpy(snap->regs, pu->reg_source, rbytes);
    if (pu->reg_target) memcpy(snap->regs_target, pu->reg_target, rbytes);
    memcpy(snap->edges, pu->edge_phases, ebytes);
    return true;
}

bool snapshot_restore(ProUniverse* pu, const FullStateSnapshot* snap) {
    if (!pu || !snap || !snap->grid || !snap->regs || !snap->edges) return false;
    if (pu->total_nodes != snap->total_nodes) return false;
    const size_t gbytes = (size_t)snap->total_nodes * sizeof(ProNode);
    const size_t rbytes = (size_t)snap->total_nodes * sizeof(ProRegister);
    const size_t ebytes = (size_t)snap->total_nodes * (size_t)CHANNELS_MAX * sizeof(ProEdge);
    memcpy(pu->ur_grid, snap->grid, gbytes);
    memcpy(pu->reg_source, snap->regs, rbytes);
    if (pu->reg_target && snap->regs_target)
        memcpy(pu->reg_target, snap->regs_target, rbytes);
    if (pu->edge_phases) memcpy(pu->edge_phases, snap->edges, ebytes);
    pu->current_cpu_tick = snap->cpu_tick;
    pu->global_entropy_index = snap->entropy;
    return true;
}

void snapshot_free(FullStateSnapshot* snap) {
    if (!snap) return;
    free(snap->grid);
    free(snap->regs);
    free(snap->regs_target);
    free(snap->edges);
    memset(snap, 0, sizeof(*snap));
}

/* ==========================================================================
 * Etappe 17: 3D-Torus-Verdrahtung.
 *
 * Layout: bit-interleaved (z, y, x) mit shift = log2(dim):
 *   k = (z << 2*shift) | (y << shift) | x
 *
 * Nachbarn in Kanaelen:
 *   0 = +x  (1 = -x), 2 = +y  (3 = -y), 4 = +z  (6 = -z).
 *   Kanal 5 bleibt self-loop (PRO_DEPHASE_CHANNEL).
 * ========================================================================== */

void wire_torus_3d(ProUniverse* pu, uint32_t dim)
{
    if (!pu || !pu->reg_source || !pu->ur_grid) return;
    if (dim < 2u || (dim & (dim - 1u)) != 0u) return;

    const uint64_t N = (uint64_t)dim * (uint64_t)dim * (uint64_t)dim;
    if (N != pu->total_nodes) return;

    uint32_t shift = 0u;
    while ((1u << shift) < dim) shift++;
    const uint32_t mask = dim - 1u;

    /* Self-Loop in allen Kanaelen, State neutral. */
    for (uint64_t k = 0; k < N; ++k) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[k].channels[c] = k;
        pu->ur_grid[k].type_state = UR_NEUTRAL;
        pu->ur_grid[k].field_helicity = 0;
    }

    for (uint32_t z = 0; z < dim; ++z) {
        const uint64_t zc = (uint64_t)z << (2u * shift);
        const uint64_t zp = (uint64_t)((z + 1u) & mask) << (2u * shift);
        const uint64_t zm = (uint64_t)((z + dim - 1u) & mask) << (2u * shift);

        for (uint32_t y = 0; y < dim; ++y) {
            const uint64_t yc = (uint64_t)y << shift;
            const uint64_t yp = (uint64_t)((y + 1u) & mask) << shift;
            const uint64_t ym = (uint64_t)((y + dim - 1u) & mask) << shift;

            for (uint32_t x = 0; x < dim; ++x) {
                const uint64_t xc = (uint64_t)x;
                const uint64_t xp = (uint64_t)((x + 1u) & mask);
                const uint64_t xm = (uint64_t)((x + dim - 1u) & mask);

                const uint64_t idx = zc | yc | xc;

                pu->reg_source[idx].channels[0] = zc | yc | xp;   /* +x */
                pu->reg_source[idx].channels[1] = zc | yc | xm;   /* -x */
                pu->reg_source[idx].channels[2] = zc | yp | xc;   /* +y */
                pu->reg_source[idx].channels[3] = zc | ym | xc;   /* -y */
                pu->reg_source[idx].channels[4] = zp | yc | xc;   /* +z */
                pu->reg_source[idx].channels[6] = zm | yc | xc;   /* -z */
                /* Kanal 5 bleibt self-loop (idx == idx). */
            }
        }
    }

    pu->grid_dim = dim;
    pu->grid_dim_shift = shift;
    pu->grid_dim_mask = mask;
    pu->grid_ndim = 3u;
}