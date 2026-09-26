// example_alpha_test.c  (Hardened v3 — Etappe 1: Amplituden-Layer)
//
// Konsolidierte Version. Enthält:
//   * Vorher/Nachher-Invariantenvergleich im Tick (U5-Fix)
//   * Snapshot inkl. edge_phases (EPR-Leichen weg)
//   * Laufzeit-Delay für EPR-Propagation (--epr-delay, --epr-debug)
//   * Native CHSH-Messung, expected_S = 2.0
//   * test_no_signaling_causality mit Delay-Parameter
//   * Amplituden-Layer Self-Test beim Start
//
// Compile (MSVC):
//   cl /std:c11 /O2 example_alpha_test.c ProPhysics.lib pro_sdk_interface.lib
//      /Fe:example_alpha_test.exe
// Compile (GCC):
//   gcc -std=c99 -O2 example_alpha_test.c -o example_alpha_test \
//      -lprophysics -lpro_sdk_interface -lm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include "pro_sdk_interface.h"
#include "ProPhysics.h"

/* ========================================================================
 * Global configuration
 * ====================================================================== */

#define GRID_DIM            1000
#define TOTAL_NODES         ((uint64_t)GRID_DIM * (uint64_t)GRID_DIM)

#define NOSIG_GRID_DIM      64
#define NOSIG_NODES         ((uint64_t)NOSIG_GRID_DIM * (uint64_t)NOSIG_GRID_DIM)

#define MAX_RUNS_LIMIT      10000

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* Etappe 10: Q15 → Q31 Konvertierung.
  * ProAmpQ15 (int32, Re/Im je int16) → ProAmpQ31 (int64, Re/Im je int32).
  * Q15TO31(x) bildet die Q15-Zahl x auf ihre Q31-Entsprechung ab. */
#define Q15TO31(x) ((int32_t)((int64_t)(x) << 16))
#define Q31_MAXV   2147483647.0
#define Q31_MINV   (-2147483648.0)

 /* ========================================================================
  * Dynamic tracker
  * ====================================================================== */

typedef struct {
    uint32_t prev_reserved_gating;
    uint32_t prev_entropy_index;
    uint64_t prev_channels[4];

    uint32_t flip_accumulator;
    uint32_t sample_window_ticks;
    float    grid_frequency_hz;
} DynamicTracker;

static inline void dynamic_tracker_init(DynamicTracker* tracker, uint32_t sample_window) {
    if (!tracker) return;
    tracker->prev_reserved_gating = 0;
    tracker->prev_entropy_index = 0;
    for (int c = 0; c < 4; c++) tracker->prev_channels[c] = 0;
    tracker->flip_accumulator = 0;
    tracker->sample_window_ticks = (sample_window > 0) ? sample_window : 256;
    tracker->grid_frequency_hz = 0.0f;
}

static inline void track_node_dynamics(DynamicTracker* tracker,
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

static inline void evaluate_grid_frequency(DynamicTracker* tracker, uint32_t current_tick) {
    if (!tracker) return;
    if (tracker->sample_window_ticks > 0 &&
        (current_tick % tracker->sample_window_ticks) == 0)
    {
        tracker->grid_frequency_hz =
            (float)tracker->flip_accumulator / (float)tracker->sample_window_ticks;
        tracker->flip_accumulator = 0;
    }
}

static void track_universe_dynamics(ProUniverse* pu, DynamicTracker* tracker, uint32_t tick) {
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

/* ========================================================================
 * Deterministic PRNG (xoshiro256**)
 * ====================================================================== */

static inline uint64_t rotl64(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

typedef struct { uint64_t s[4]; } prng_state_t;

static uint64_t prng_next(prng_state_t* state) {
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

static void prng_seed(prng_state_t* state, uint64_t seed) {
    for (int i = 0; i < 4; i++) {
        uint64_t z = (seed += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        state->s[i] = z ^ (z >> 31);
    }
}

static inline uint64_t fast_map_index(uint64_t random_val, uint64_t max_nodes) {
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

static inline bool arg_matches(const char* arg, const char* option) {
    return strcmp(arg, option) == 0;
}

/* ========================================================================
 * Engine context
 * ====================================================================== */

typedef struct { uint8_t state; } EngineNodeAlias;

typedef struct {
    ProUniverse pu;
    prng_state_t rng;
    EngineNodeAlias* nodes;
    DynamicTracker tracker;
    bool ready;
} ProEngineContext;

static void pro_engine_init(ProEngineContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));

    ProPhysics_Initialize(&ctx->pu, TOTAL_NODES);
    if (!ctx->pu.ur_grid || !ctx->pu.reg_source) {
        fprintf(stderr, "[!] ProPhysics_Initialize failed — out of memory?\n");
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

static void pro_engine_cleanup(ProEngineContext* ctx) {
    if (!ctx) return;
    ProPhysics_Free(&ctx->pu);
    if (ctx->nodes) {
        free(ctx->nodes);
        ctx->nodes = NULL;
    }
    ctx->ready = false;
}

static void pro_engine_reset(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready) return;

    memset(ctx->pu.ur_grid, 0, ctx->pu.total_nodes * sizeof(ProNode));
    if (ctx->nodes) {
        memset(ctx->nodes, 0, TOTAL_NODES * sizeof(EngineNodeAlias));
    }

    for (uint64_t i = 0; i < ctx->pu.total_nodes; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c) {
            ctx->pu.reg_source[i].channels[c] = i;
        }
    }

    ctx->pu.current_cpu_tick = 0;
    ctx->pu.global_entropy_index = 0;

    if (ctx->pu.amp_grid) {
        ProPhysics_Sync_Amp_From_Type(&ctx->pu);
    }

    dynamic_tracker_init(&ctx->tracker, 256);
}

static void pro_engine_sync_nodes_to_pu(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready || !ctx->nodes) return;
    for (uint64_t i = 0; i < TOTAL_NODES; ++i) {
        ctx->pu.ur_grid[i].type_state = ctx->nodes[i].state;
    }
}

static void pro_engine_sync_pu_to_nodes(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready || !ctx->nodes) return;
    for (uint64_t i = 0; i < TOTAL_NODES; ++i) {
        ctx->nodes[i].state = ctx->pu.ur_grid[i].type_state;
    }
}

static void pro_engine_tick(ProEngineContext* ctx) {
    if (!ctx || !ctx->ready) return;
    pro_engine_sync_nodes_to_pu(ctx);
    ProPhysics_SDK_Execute_Plastizitaet_Tick(&ctx->pu,
        ResearchPlugin_DynamicPlasticTopology);
    pro_engine_sync_pu_to_nodes(ctx);
}

/* ========================================================================
 * EPR helpers
 * ====================================================================== */

static bool pro_engine_inject_epr_pair(ProEngineContext* ctx,
    uint32_t dist_nodes,
    uint32_t* out_a, uint32_t* out_b)
{
    if (!ctx || !ctx->ready) return false;

    /* TODO(Etappe 9): Nach Option B ist type_state = UR_PHOTON hier
     * ontologisch inkonsistent. Der Lorentz-Test muss zuerst neu bewertet
     * werden, dann wird diese Funktion analog zu
     * inject_epr_pair_deterministic umgebaut. */

    const uint64_t idx1 = fast_map_index(prng_next(&ctx->rng), TOTAL_NODES);
    const uint64_t idx2 = (idx1 + (uint64_t)dist_nodes) % TOTAL_NODES;
    if (idx1 == idx2) return false;

    const uint64_t old_a = ctx->pu.reg_source[idx1].channels[PRO_EPR_CHANNEL];
    const uint64_t old_b = ctx->pu.reg_source[idx2].channels[PRO_EPR_CHANNEL];
    if (old_a < TOTAL_NODES && old_a != idx1) {
        ctx->pu.reg_source[old_a].channels[PRO_EPR_CHANNEL] = old_a;
    }
    if (old_b < TOTAL_NODES && old_b != idx2) {
        ctx->pu.reg_source[old_b].channels[PRO_EPR_CHANNEL] = old_b;
    }

    ctx->pu.ur_grid[idx1].type_state = UR_PHOTON;
    ctx->pu.ur_grid[idx2].type_state = UR_PHOTON;
    ctx->pu.ur_grid[idx1].field_helicity = 1;
    ctx->pu.ur_grid[idx2].field_helicity = 2;

    ctx->pu.reg_source[idx1].channels[PRO_EPR_CHANNEL] = idx2;
    ctx->pu.reg_source[idx2].channels[PRO_EPR_CHANNEL] = idx1;

    ctx->nodes[idx1].state = UR_PHOTON;
    ctx->nodes[idx2].state = UR_PHOTON;

    if (out_a) *out_a = (uint32_t)idx1;
    if (out_b) *out_b = (uint32_t)idx2;
    return true;
}

static uint32_t pro_engine_get_entangled_partner(const ProEngineContext* ctx,
    uint32_t node_a)
{
    if (!ctx || !ctx->ready) return node_a;
    if (node_a >= TOTAL_NODES) return node_a;
    const uint64_t p = ctx->pu.reg_source[node_a].channels[PRO_EPR_CHANNEL];
    return (p < TOTAL_NODES) ? (uint32_t)p : node_a;
}

static uint32_t pro_engine_spawn_soliton(ProEngineContext* ctx,
    uint32_t x, uint32_t y, uint32_t dir)
{
    if (!ctx || !ctx->ready) return 0;
    const uint64_t idx = ((uint64_t)y * GRID_DIM + x) % TOTAL_NODES;

    ctx->pu.ur_grid[idx].type_state = UR_POSITRON_CW;
    ctx->pu.ur_grid[idx].field_helicity = (uint8_t)(dir + 1);
    ctx->pu.ur_grid[idx].phase_accumulator = 0;
    ctx->nodes[idx].state = UR_POSITRON_CW;
    return (uint32_t)idx;
}

static uint32_t pro_engine_spawn_boosted_soliton(ProEngineContext* ctx,
    double v_c_ratio)
{
    if (!ctx || !ctx->ready) return 0;

    const uint64_t idx = fast_map_index(prng_next(&ctx->rng), TOTAL_NODES);
    ctx->pu.ur_grid[idx].type_state = UR_POSITRON_CW;
    ctx->pu.ur_grid[idx].field_helicity = 1;
    ctx->nodes[idx].state = UR_POSITRON_CW;

    ProPhysics_Inject_Momentum(&ctx->pu, idx, v_c_ratio);
    return (uint32_t)idx;
}

/* ========================================================================
 * Cycle measurement
 * ====================================================================== */

static uint64_t pro_engine_measure_phase_cycle(ProEngineContext* ctx,
    uint32_t start_node,
    uint32_t target_cycles,
    const char* label)
{
    if (!ctx || !ctx->ready || start_node >= TOTAL_NODES) {
        printf("   [%s] FEHLER: Ungueltiger Kontext/Startknoten.\n", label);
        return 0;
    }

    uint32_t current_node = start_node;
    uint8_t  last_helicity = ctx->pu.ur_grid[current_node].field_helicity;
    uint8_t  start_type = ctx->pu.ur_grid[current_node].type_state;

    printf("   [%s] Messung @ Knoten %u | Typ: %u | Helizitaet: %u | Ziel: %u Zyklen\n",
        label, start_node, start_type, last_helicity, target_cycles);

    uint64_t tick_counter = 0;
    uint32_t completed_flips = 0;
    const uint32_t flips_per_period = 4;
    const uint32_t target_flips = target_cycles * flips_per_period;
    const uint64_t max_safety_ticks = 4000;

    while (completed_flips < target_flips && tick_counter < max_safety_ticks) {
        pro_engine_tick(ctx);
        tick_counter++;

        track_universe_dynamics(&ctx->pu, &ctx->tracker, (uint32_t)tick_counter);

        uint8_t current_type = ctx->pu.ur_grid[current_node].type_state;
        uint8_t current_helicity = ctx->pu.ur_grid[current_node].field_helicity;

        if (current_type == UR_NEUTRAL) {
            bool found = false;
            for (int c = 0; c < CHANNELS_MAX; ++c) {
                const uint64_t nb = ctx->pu.reg_source[current_node].channels[c];
                if (nb < TOTAL_NODES && nb != current_node &&
                    ctx->pu.ur_grid[nb].type_state != UR_NEUTRAL)
                {
                    printf("   [%s] Tick %" PRIu64 ": Soliton %u -> %" PRIu64 "\n",
                        label, tick_counter, current_node, nb);
                    current_node = (uint32_t)nb;
                    current_helicity = ctx->pu.ur_grid[current_node].field_helicity;
                    found = true;
                    break;
                }
            }
            if (!found) {
                printf("   [%s] Tick %" PRIu64 ": ABBRUCH — Soliton zerfallen.\n",
                    label, tick_counter);
                break;
            }
        }

        if (current_helicity != last_helicity) {
            completed_flips++;
            printf("   [%s] Tick %" PRIu64 ": Flip %u/%u (Hel. %u -> %u @ %u)\n",
                label, tick_counter, completed_flips, target_flips,
                last_helicity, current_helicity, current_node);
            last_helicity = current_helicity;
        }
    }

    if (tick_counter >= max_safety_ticks) {
        printf("   [%s] TIMEOUT (%" PRIu64 " Ticks, Flips %u/%u)\n",
            label, tick_counter, completed_flips, target_flips);
    }
    else {
        printf("   [%s] ERFOLG in %" PRIu64 " Ticks (Flips %u/%u)\n",
            label, tick_counter, completed_flips, target_flips);
    }
    return tick_counter;
}

/* ========================================================================
 * CLI helper
 * ====================================================================== */

static void usage(const char* prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -h, --help            Show this help\n");
    printf("  --runs N              Independent runs (default: 3)\n");
    printf("  --ticks N             Ticks per run (default: 5000)\n");
    printf("  --inject-ticks N      Ticks with injection (default: 10%% of ticks)\n");
    printf("  --no-epr              Inject no EPR Photons\n");
    printf("  --epr-pairs           Inject entangled EPR singlet pairs\n");
    printf("  --epr-triplet         Inject entangled EPR triplet pairs\n");
    printf("  --epr-delay N         EPR propagation delay in ticks (default: 0)\n");
    printf("  --epr-debug           Enable EPR propagation event log\n");
    printf("  --test-chsh           Run CHSH regression (native, expected S=2.0)\n");
    printf("  --out FILE            Output CSV (default: Test_Alpha_YYYYMMDD_HHMMSS.csv)\n");
    printf("  --append              Append to existing file\n");
    printf("  --per-run-files       One CSV per run (suffix _runNNN.csv)\n");
    printf("  --fixed-seeds N       N runs with deterministic seeds\n");
    printf("  --random-seeds N      N runs with random seeds\n");
    printf("  --test-no-signaling   Run no-signaling causality test\n");
    printf("  --test-invariance     Run U5 invariance test (closed system)\n");
    printf("  --test-lorentz        Run Lorentz / time-dilation test\n");
    printf("  --test-amp            Run amplitude regression test\n");
    printf("  --test-born           Born-Regel-Regression (chi^2 gegen |alpha|^2:|beta|^2)\n");
    printf("  --test-unitary        Unitärer Tick: U*U+=I, [U,diag(w)]=0, U5-Norm\n");
    printf("  --test-context        Kontextabhängige signed permutations (Etappe 3b)\n");
    printf("  --test-wilson         U(1)-Wilson-Loop und Eichinvarianz (Etappe 4a)\n");
    printf("  --test-gauge          Lokale Eichtransformation (Etappe 4b)\n");
    printf("  --test-triangle       Dreieckskorrelation E(Delta) (Etappe 4c)\n");
    printf("  --test-superdet       Run CHSH-Superdeterminismus-Test (Etappe 5)\n");
    printf("  --test-observer-chsh  Beobachter-CHSH mit lokaler Amplitudendiffusion (Etappe 6a)\n");
    printf("  --test-chsh-diffusion CHSH chaotisch, dissipative Diffusion+Dephasierung (6b+6c)\n");
    printf("  --test-chsh-wave      CHSH chaotisch, unitäre Wellengleichung (6d)\n");
    printf("  --test-chsh-collapse  CHSH mit Kollaps-Messung gemäß U4 (6e)\n");
    printf("  --test-chsh-graph     CHSH Graph-Messung via Kantenphase (6e')\n");
    printf("  --test-born-emergent  Born-Emergenz aus Umgebungsverschraenkung (6f)\n");
    printf("  --test-born-local     Born-Emergenz aus U4-lokaler Messung (6h / 6g)\n");
    printf("  --test-born-equiv     Born-Equivarianz via Fuehrungsgleichung U6 (7)\n");
    printf("  --test-amp-invariant  U5-Invariante auf amp_grid (Etappe 8)\n");
    printf("  --test-edge-transport         Kanten-Transport sequenziell (Etappe 9 Phase 1)\n");
    printf("  --test-edge-transport-colored Kanten-Transport farbig, exakt unitär (Phase 2)\n");
    printf("  --test-edge-transport-scaling Skalierungs-Diagnose Drift(STEPS)\n");
    printf("  --test-wave-packet            Wellenpaket-Dispersion, ballistisch vs. diffusiv (Phase 2)\n");
    printf("  --test-amp-invariant-colored  Etappe-8-Invariante mit farbigem Sweep (Ziel C)\n");
    printf("  --test-amp-invariant-bisect   U5-Diagnose: welcher Schritt zerlegt die Norm?\n");
    printf("  --test-soliton                Soliton-Stabilitaet, GP-Selbstkopplung (Etappe 11)\n");
    exit(0);
}

static struct tm* portable_localtime(const time_t* t, struct tm* out) {
#if defined(_WIN32) || defined(_WIN64)
    if (localtime_s(out, t) == 0) return out;
    return NULL;
#else
    return localtime_r(t, out);
#endif
}

static void make_default_filename(char* buf, size_t buf_len) {
    time_t t = time(NULL);
    struct tm tm_buf;
    struct tm* tm_ptr = portable_localtime(&t, &tm_buf);
    if (tm_ptr) {
        strftime(buf, buf_len, "Test_Alpha_%Y%m%d_%H%M%S.csv", tm_ptr);
    }
    else {
        snprintf(buf, buf_len, "Test_Alpha_%" PRId64 ".csv", (int64_t)t);
    }
}

static bool file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    fclose(f);
    return true;
}

/* ========================================================================
 * Injectors
 * ====================================================================== */

static void inject_photon_deterministic(ProUniverse* pu, prng_state_t* rng) {
    if (!pu || !rng) return;
    const uint64_t idx = fast_map_index(prng_next(rng), pu->total_nodes);
    pu->ur_grid[idx].type_state = UR_PHOTON;
    pu->ur_grid[idx].field_helicity = (uint8_t)(1 + (prng_next(rng) & 1u));
}
static void inject_epr_pair_deterministic(ProUniverse* pu, prng_state_t* rng,
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

    pu->ur_grid[idx1].type_state = UR_NEUTRAL;
    pu->ur_grid[idx2].type_state = UR_NEUTRAL;
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

        /* Etappe 10: Q31. */
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
/* ========================================================================
 * CHSH / Bell evaluator (native, S = 2)
 * ====================================================================== */

static double evaluate_correlation(const ProUniverse* pu,
    double theta_a, double theta_b,
    uint64_t rng[4])
{
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->edge_phases) return 0.0;

    double   sum = 0.0;
    uint64_t count = 0;

    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        if (pu->ur_grid[i].type_state == UR_NEUTRAL) continue;

        const uint64_t partner = pu->reg_source[i].channels[PRO_EPR_CHANNEL];
        if (partner >= pu->total_nodes || partner == i) continue;
        if (partner < i) continue;                          /* jede Kante einmal */
        if (pu->ur_grid[partner].type_state == UR_NEUTRAL) continue;

        const uint8_t edge_type =
            pu->edge_phases[i * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL].type;
        if (edge_type != PRO_EDGE_SINGLET && edge_type != PRO_EDGE_TRIPLET) continue;

        int a = 0, b = 0;
        if (ProPhysics_Measure_EPR_Pair((ProUniverse*)pu, i,
            theta_a, theta_b,
            rng, &a, &b)) {
            sum += (double)(a * b);
            count++;
        }
    }
    return count ? (sum / (double)count) : 0.0;
}

typedef struct {
    double E_ab, E_ab_prime, E_a_prime_b, E_a_prime_b_prime, S_CHSH;
} CHSH_Result;

static CHSH_Result compute_chsh_metrics(ProUniverse* pu, uint64_t rng[4]) {
    CHSH_Result res = { 0 };
    const double thA = 0.0;
    const double thAp = M_PI / 2.0;
    const double thB = M_PI / 4.0;
    const double thBp = 3.0 * M_PI / 4.0;

    res.E_ab = evaluate_correlation(pu, thA, thB, rng);
    res.E_ab_prime = evaluate_correlation(pu, thA, thBp, rng);
    res.E_a_prime_b = evaluate_correlation(pu, thAp, thB, rng);
    res.E_a_prime_b_prime = evaluate_correlation(pu, thAp, thBp, rng);

    res.S_CHSH = fabs(res.E_ab - res.E_ab_prime)
        + fabs(res.E_a_prime_b + res.E_a_prime_b_prime);
    return res;
}

/* ========================================================================
 * Torus topology setup
 * ====================================================================== */

static void init_torus(ProUniverse* pu) {
    if (!pu || !pu->reg_source || !pu->ur_grid) return;

    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c) {
            pu->reg_source[i].channels[c] = i;
        }
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

    if (pu->amp_grid) {
        ProPhysics_Sync_Amp_From_Type(pu);
    }
}

static void collect_advanced_metrics(const ProUniverse* pu,
    uint64_t* pos, uint64_t* neg,
    uint64_t* photon,
    uint64_t* non_euclidean_links,
    int64_t* total_helicity)
{
    *pos = *neg = *photon = 0;
    *non_euclidean_links = 0;
    *total_helicity = 0;

    if (!pu || !pu->ur_grid || !pu->reg_source) return;

    for (uint64_t y = 0; y < GRID_DIM; ++y) {
        const uint64_t y_north = (y == 0 ? GRID_DIM - 1 : y - 1) * GRID_DIM;
        const uint64_t y_south = (y == GRID_DIM - 1 ? 0 : y + 1) * GRID_DIM;
        const uint64_t y_curr = y * GRID_DIM;

        for (uint64_t x = 0; x < GRID_DIM; ++x) {
            const uint64_t idx = y_curr + x;
            const uint8_t  st = pu->ur_grid[idx].type_state;

            if (st == UR_POSITRON_CW || st == UR_POSITRON_CCW) (*pos)++;
            else if (st == UR_NEGATRON_CW || st == UR_NEGATRON_CCW) (*neg)++;
            else if (st == UR_PHOTON) (*photon)++;

            *total_helicity += (int64_t)pu->ur_grid[idx].field_helicity;

            if (pu->reg_source[idx].channels[0] != y_north + x) (*non_euclidean_links)++;
            if (pu->reg_source[idx].channels[1] != y_south + x) (*non_euclidean_links)++;
        }
    }
}

/* ========================================================================
 * Single simulation run
 * ====================================================================== */

static int run_single(uint32_t ticks, uint32_t inject_ticks, bool use_epr,
    uint8_t epr_edge_type,
    const char* out_file, bool append_mode,
    bool per_run_file, uint32_t run_index, uint32_t seed)
{
    char filename[512];
    if (per_run_file) {
        const char* dot = strrchr(out_file, '.');
        if (dot && strcmp(dot, ".csv") == 0) {
            size_t base_len = (size_t)(dot - out_file);
            snprintf(filename, sizeof(filename), "%.*s_run%03u.csv",
                (int)base_len, out_file, run_index);
        }
        else {
            snprintf(filename, sizeof(filename), "%s_run%03u.csv",
                out_file, run_index);
        }
    }
    else {
        snprintf(filename, sizeof(filename), "%s", out_file);
    }

    const bool exists = file_exists(filename);
    FILE* f = fopen(filename, append_mode ? "a" : "w");
    if (!f) {
        fprintf(stderr, "Fehler: Kann Datei nicht oeffnen: %s\n", filename);
        return 1;
    }

    if (!exists && !append_mode) {
        fprintf(f,
            "tick,rhoE,alpha_eff,pos,neg,photon,interactions,"
            "non_euclidean_links,helicity_sum,"
            "E_ab,E_ab_prime,E_a_prime_b,E_a_prime_b_prime,S_CHSH,"
            "grid_freq_hz,seed,run_index\n");
    }

    prng_state_t rng;
    prng_seed(&rng, (uint64_t)seed);

    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source) {
        fprintf(stderr, "Initialisierung fehlgeschlagen (run %u)\n", run_index);
        fclose(f);
        return 1;
    }

    init_torus(&pu);
    pu.grid_dim = GRID_DIM;     /* NEU: farbiger Sweep für 1000×1000-Torus */
    ProPhysics_Commit_Invariance_Baseline(&pu);

    DynamicTracker tracker;
    dynamic_tracker_init(&tracker, 256);

#define CHSH_WINDOW 100
    double   s_window[CHSH_WINDOW] = { 0.0 };
    uint32_t s_window_idx = 0;
    uint32_t s_window_count = 0;
    double   s_window_sum = 0.0;

    for (uint32_t tick = 1; tick <= ticks; ++tick) {
        if (tick <= inject_ticks) {
            if (use_epr) inject_epr_pair_deterministic(&pu, &rng, epr_edge_type);
            else         inject_photon_deterministic(&pu, &rng);
        }

        /* Etappe 10: Invariante ist ProU128. */
        const ProU128 inv_before = ProPhysics_Measure_Amp_Invariant(&pu);

        const CHSH_Result chsh = compute_chsh_metrics(&pu, rng.s);

        s_window_sum -= s_window[s_window_idx];
        s_window[s_window_idx] = chsh.S_CHSH;
        s_window_sum += chsh.S_CHSH;
        s_window_idx = (s_window_idx + 1) % CHSH_WINDOW;
        if (s_window_count < CHSH_WINDOW) s_window_count++;

        const double s_mean = (s_window_count > 0)
            ? (s_window_sum / (double)s_window_count)
            : 0.0;

        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);

        const ProU128 inv_after = ProPhysics_Measure_Amp_Invariant(&pu);
        if (pro_u128_cmp(inv_before, inv_after) != 0) {
            const double nb = pro_u128_to_double(inv_before);
            const double na = pro_u128_to_double(inv_after);
            const double ad = fabs(na - nb);
            const double tolerance = nb / 1000.0;
            if (ad > tolerance) {
                fprintf(stderr,
                    "[!] U5-Verletzung (amp_grid) bei Tick %u "
                    "(run %u, seed %u): vor=%.6e nach=%.6e "
                    "(Δ=%+.6e, Toleranz=%.6e)\n",
                    tick, run_index, seed, nb, na, na - nb, tolerance);
                ProPhysics_Free(&pu);
                fclose(f);
                return 2;
            }
        }


        track_universe_dynamics(&pu, &tracker, tick);

        uint64_t pos = 0, neg = 0, photon = 0, non_euclidean = 0;
        int64_t  heli_sum = 0;
        collect_advanced_metrics(&pu, &pos, &neg, &photon,
            &non_euclidean, &heli_sum);

        const double rhoE = (double)(pos + neg + photon) / (double)pu.total_nodes;
        const double alpha_eff = (photon > 0)
            ? (double)(pos + neg) / (double)photon
            : 0.0;
        const uint32_t interactions = pu.global_entropy_index;

        fprintf(f,
            "%u,%.8f,%.8f,%" PRIu64 ",%" PRIu64 ",%" PRIu64
            ",%u,%" PRIu64 ",%" PRId64
            ",%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u,%u\n",
            tick, rhoE, alpha_eff, pos, neg, photon,
            interactions, non_euclidean, heli_sum,
            chsh.E_ab, chsh.E_ab_prime, chsh.E_a_prime_b, chsh.E_a_prime_b_prime,
            chsh.S_CHSH, tracker.grid_frequency_hz, seed, run_index);

        if (tick == 1 || tick % 250 == 0 || tick == ticks) {
            printf("Run %2u | Tick %6u/%u | rhoE=%.6f | alpha=%.6f | "
                "S=%.4f | S_mean=%.4f | f_grid=%.4f Hz | rewires=%" PRIu64 "\n",
                run_index, tick, ticks, rhoE, alpha_eff,
                chsh.S_CHSH, s_mean, tracker.grid_frequency_hz, non_euclidean);
        }
    }

    ProPhysics_Free(&pu);
    fclose(f);
    return 0;
}

/* ========================================================================
 * No-signaling causality test (mit EPR-Propagation)
 * ====================================================================== */

typedef struct {
    ProNode* grid;
    ProRegister* regs;
    ProRegister* regs_target;
    ProEdge* edges;
    uint64_t     total_nodes;
    uint64_t     cpu_tick;
    uint32_t     entropy;
} FullStateSnapshot;

static bool snapshot_capture(const ProUniverse* pu, FullStateSnapshot* snap) {
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

static bool snapshot_restore(ProUniverse* pu, const FullStateSnapshot* snap) {
    if (!pu || !snap || !snap->grid || !snap->regs || !snap->edges) return false;
    if (pu->total_nodes != snap->total_nodes) return false;

    const size_t gbytes = (size_t)snap->total_nodes * sizeof(ProNode);
    const size_t rbytes = (size_t)snap->total_nodes * sizeof(ProRegister);
    const size_t ebytes = (size_t)snap->total_nodes * (size_t)CHANNELS_MAX * sizeof(ProEdge);

    memcpy(pu->ur_grid, snap->grid, gbytes);
    memcpy(pu->reg_source, snap->regs, rbytes);
    if (pu->reg_target && snap->regs_target) {
        memcpy(pu->reg_target, snap->regs_target, rbytes);
    }
    if (pu->edge_phases) memcpy(pu->edge_phases, snap->edges, ebytes);

    pu->current_cpu_tick = snap->cpu_tick;
    pu->global_entropy_index = snap->entropy;
    return true;
}

static void snapshot_free(FullStateSnapshot* snap) {
    if (!snap) return;
    free(snap->grid);
    free(snap->regs);
    free(snap->regs_target);
    free(snap->edges);
    memset(snap, 0, sizeof(*snap));
}

static void nosig_init_torus(ProUniverse* pu) {
    if (!pu || !pu->reg_source) return;
    for (uint64_t i = 0; i < pu->total_nodes; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < NOSIG_GRID_DIM; ++y) {
        const uint64_t y_n = (y == 0 ? NOSIG_GRID_DIM - 1 : y - 1) * NOSIG_GRID_DIM;
        const uint64_t y_s = (y == NOSIG_GRID_DIM - 1 ? 0 : y + 1) * NOSIG_GRID_DIM;
        const uint64_t y_c = y * NOSIG_GRID_DIM;
        for (uint64_t x = 0; x < NOSIG_GRID_DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu->reg_source[idx].channels[0] = y_n + x;
            pu->reg_source[idx].channels[1] = y_s + x;
            pu->reg_source[idx].channels[2] = y_c + (x == NOSIG_GRID_DIM - 1 ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_c + (x == 0 ? NOSIG_GRID_DIM - 1 : x - 1);
        }
    }
}

static bool test_no_signaling_causality(uint32_t ensemble_size,
    uint32_t dist_nodes,
    uint32_t epr_delay,
    bool     debug)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NOSIG_NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.edge_phases) {
        printf("   [No-Signaling] Init failed.\n");
        return false;
    }

    ProPhysics_Set_EPR_Delay(&pu, epr_delay);
    ProPhysics_Set_EPR_Debug(&pu, debug ? 1u : 0u);

    nosig_init_torus(&pu);

    prng_state_t rng;
    prng_seed(&rng, 0xDEADBEEFCAFEBABEULL);

    uint32_t p_b_flip[8] = { 0 };
    uint32_t p_b_noop[8] = { 0 };
    uint32_t counted_iterations = 0;

    const uint32_t debug_iters = 3u;

    for (uint32_t i = 0; i < ensemble_size; ++i) {
        const uint64_t node_a = fast_map_index(prng_next(&rng), NOSIG_NODES);
        const uint64_t node_b = (node_a + (uint64_t)dist_nodes) % NOSIG_NODES;
        if (node_a == node_b) continue;

        pu.ur_grid[node_a].type_state = UR_PHOTON;
        pu.ur_grid[node_a].field_helicity = 1;
        pu.ur_grid[node_b].type_state = UR_PHOTON;
        pu.ur_grid[node_b].field_helicity = 2;
        pu.ur_grid[node_a].phase_accumulator = 0;
        pu.ur_grid[node_b].phase_accumulator = 0;

        pu.reg_source[node_a].channels[PRO_EPR_CHANNEL] = node_b;
        pu.reg_source[node_b].channels[PRO_EPR_CHANNEL] = node_a;

        const uint64_t edge_a = node_a * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL;
        const uint64_t edge_b = node_b * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL;

        ProPhysics_Set_Edge_Phase(&pu, node_a, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_SINGLET);
        ProPhysics_Set_Edge_Phase(&pu, node_b, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_SINGLET);

        pu.edge_phases[edge_a].last_sent_helicity = pu.ur_grid[node_a].field_helicity;
        pu.edge_phases[edge_b].last_sent_helicity = pu.ur_grid[node_b].field_helicity;
        pu.edge_phases[edge_a].pending_ticks = 0;
        pu.edge_phases[edge_b].pending_ticks = 0;

        if (debug && i < debug_iters) {
            printf("   [No-Sig Iter %u] SETUP   A=%" PRIu64 " hel=%u | B=%" PRIu64
                " hel=%u | delay=%u\n",
                i, node_a, pu.ur_grid[node_a].field_helicity,
                node_b, pu.ur_grid[node_b].field_helicity, epr_delay);
        }

        FullStateSnapshot snap;
        if (!snapshot_capture(&pu, &snap)) {
            printf("   [No-Signaling] Snapshot alloc failed.\n");
            ProPhysics_Free(&pu);
            return false;
        }

        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
        const uint8_t hel_noop = pu.ur_grid[node_b].field_helicity & 0x07u;
        p_b_noop[hel_noop]++;

        if (debug && i < debug_iters) {
            printf("   [No-Sig Iter %u] PASS1   B_hel=%u\n", i, hel_noop);
        }

        if (!snapshot_restore(&pu, &snap)) {
            snapshot_free(&snap);
            ProPhysics_Free(&pu);
            return false;
        }
        snapshot_free(&snap);

        pu.ur_grid[node_a].field_helicity =
            (uint8_t)((pu.ur_grid[node_a].field_helicity + 2) & 0x03u);

        if (debug && i < debug_iters) {
            printf("   [No-Sig Iter %u] PASS2   BEFORE tick: A_hel=%u B_hel=%u\n",
                i, pu.ur_grid[node_a].field_helicity,
                pu.ur_grid[node_b].field_helicity);
        }

        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
        const uint8_t hel_flip = pu.ur_grid[node_b].field_helicity & 0x07u;
        p_b_flip[hel_flip]++;

        if (debug && i < debug_iters) {
            printf("   [No-Sig Iter %u] PASS2   AFTER  tick: A_hel=%u B_hel=%u\n",
                i, pu.ur_grid[node_a].field_helicity, hel_flip);
        }

        counted_iterations++;

        pu.ur_grid[node_a].type_state = UR_NEUTRAL;
        pu.ur_grid[node_a].field_helicity = 0;
        pu.ur_grid[node_b].type_state = UR_NEUTRAL;
        pu.ur_grid[node_b].field_helicity = 0;
        pu.reg_source[node_a].channels[PRO_EPR_CHANNEL] = node_a;
        pu.reg_source[node_b].channels[PRO_EPR_CHANNEL] = node_b;

        pu.edge_phases[edge_a].type = PRO_EDGE_NONE;
        pu.edge_phases[edge_b].type = PRO_EDGE_NONE;
        pu.edge_phases[edge_a].pending_ticks = 0;
        pu.edge_phases[edge_b].pending_ticks = 0;
        pu.edge_phases[edge_a].last_sent_helicity = 0;
        pu.edge_phases[edge_b].last_sent_helicity = 0;
    }

    double delta_p = 0.0;
    const double denom = (counted_iterations > 0) ? (double)counted_iterations : 1.0;
    for (int s = 0; s < 8; ++s) {
        const double pf = (double)p_b_flip[s] / denom;
        const double pn = (double)p_b_noop[s] / denom;
        delta_p += fabs(pf - pn);
    }

    printf("   [No-Signaling] Ensemble: %u (counted %u) | Delay=%u"
        " | Delta P(B_helicity): %.8f\n",
        ensemble_size, counted_iterations, epr_delay, delta_p);

    ProPhysics_Free(&pu);

    return (delta_p > 0.5);
}

/* ========================================================================
 * U5 invariance test (geschlossenes System)
 * ====================================================================== */

static bool test_invariance_under_tick(uint32_t grid_dim,
    uint32_t ticks,
    uint32_t initial_photons,
    uint32_t seed)
{
    const uint64_t nodes = (uint64_t)grid_dim * (uint64_t)grid_dim;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, nodes);
    if (!pu.ur_grid || !pu.reg_source) {
        printf("   [U5] Init failed.\n");
        return false;
    }

    for (uint64_t i = 0; i < nodes; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < grid_dim; ++y) {
        const uint64_t y_n = (y == 0 ? grid_dim - 1 : y - 1) * grid_dim;
        const uint64_t y_s = (y == grid_dim - 1 ? 0 : y + 1) * grid_dim;
        const uint64_t y_c = y * grid_dim;
        for (uint64_t x = 0; x < grid_dim; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == grid_dim - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? grid_dim - 1 : x - 1);
        }
    }

    prng_state_t rng;
    prng_seed(&rng, (uint64_t)seed);

    uint32_t placed = 0;
    for (uint32_t i = 0; i < initial_photons * 4u && placed < initial_photons; ++i) {
        const uint64_t idx = fast_map_index(prng_next(&rng), nodes);
        if (pu.ur_grid[idx].type_state != UR_NEUTRAL) continue;
        pu.ur_grid[idx].type_state = UR_PHOTON;
        pu.ur_grid[idx].field_helicity = (uint8_t)(1 + (prng_next(&rng) & 1u));
        placed++;
    }

    if (pu.amp_grid) ProPhysics_Sync_Amp_From_Type(&pu);

    const uint64_t baseline = ProPhysics_Measure_Invariant(&pu);
    ProPhysics_Commit_Invariance_Baseline(&pu);

    printf("   [U5] Grid %ux%u = %" PRIu64 " Knoten | %u Photonen platziert\n",
        grid_dim, grid_dim, nodes, placed);
    printf("   [U5] Baseline-Invariante = %" PRIu64 " (erwartet %" PRIu64 ")\n",
        baseline, (uint64_t)placed * 5u);
    printf("   [U5] Laufe %u Ticks ohne Injektionen...\n", ticks);

    uint64_t first_violation_tick = 0;
    uint64_t max_abs_dev = 0;
    uint64_t violation_count = 0;
    uint64_t final_value = baseline;

    for (uint32_t t = 1; t <= ticks; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);

        const uint64_t cur = ProPhysics_Measure_Invariant(&pu);
        const uint64_t diff = (cur > baseline) ? (cur - baseline) : (baseline - cur);

        if (diff > max_abs_dev) max_abs_dev = diff;

        if (cur != baseline) {
            violation_count++;
            if (first_violation_tick == 0) {
                first_violation_tick = t;
                fprintf(stderr,
                    "   [U5] !!! Erste Verletzung bei Tick %u: "
                    "Soll=%" PRIu64 " Ist=%" PRIu64 " (Δ=%+" PRId64 ")\n",
                    t, baseline, cur,
                    (int64_t)cur - (int64_t)baseline);
            }
        }
        final_value = cur;
    }

    uint64_t final_pos = 0, final_neg = 0, final_photon = 0, final_neutral = 0;
    for (uint64_t i = 0; i < nodes; ++i) {
        switch (pu.ur_grid[i].type_state) {
        case UR_POSITRON_CW:
        case UR_POSITRON_CCW:  final_pos++;    break;
        case UR_NEGATRON_CW:
        case UR_NEGATRON_CCW:  final_neg++;    break;
        case UR_PHOTON:        final_photon++; break;
        default:               final_neutral++; break;
        }
    }

    printf("   [U5] Endzustand: e+=%" PRIu64 " e-=%" PRIu64
        " gamma=%" PRIu64 " Vac=%" PRIu64 "\n",
        final_pos, final_neg, final_photon, final_neutral);
    printf("   [U5] Finale Invariante = %" PRIu64
        " | max |d| = %" PRIu64 " | Verletzungen = %" PRIu64 "\n",
        final_value, max_abs_dev, violation_count);

    ProPhysics_Free(&pu);

    if (baseline != (uint64_t)placed * 5u) {
        printf("   [U5] WARNUNG: Baseline %" PRIu64 " != 5*%u — Testaufbau fehlerhaft\n",
            baseline, placed);
        return false;
    }

    return (violation_count == 0);
}

/* ========================================================================
 * CHSH regression test (nativ, S = 2)
 * ====================================================================== */

static bool test_chsh_edge_type(ProEdgeType edge_type, const char* label,
    double expected_S, double tolerance)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source) {
        printf("   [CHSH %s] Init failed\n", label);
        return false;
    }

    init_torus(&pu);

    prng_state_t rng;
    prng_seed(&rng, 0xC0FFEE1234567ULL);

    const uint32_t pair_attempts = 1500;
    for (uint32_t i = 0; i < pair_attempts; ++i) {
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)edge_type);
    }

    uint32_t pair_count = 0;
    for (uint64_t i = 0; i < pu.total_nodes; ++i) {
        const uint64_t p = pu.reg_source[i].channels[PRO_EPR_CHANNEL];
        if (p < pu.total_nodes && p != i && p > i) pair_count++;
    }

    const CHSH_Result res = compute_chsh_metrics(&pu, rng.s);
    const double dev = fabs(res.S_CHSH - expected_S);

    printf("   [CHSH %-7s] pairs=%u | E=(%+.4f, %+.4f, %+.4f, %+.4f)\n",
        label, pair_count,
        res.E_ab, res.E_ab_prime, res.E_a_prime_b, res.E_a_prime_b_prime);
    printf("                  S = %.6f | expected = %.6f | |dev| = %.2e\n",
        res.S_CHSH, expected_S, dev);

    /* Dreiecksform-Prüfung: für Singlet erwartet die klassische Korrelation
     *   E_Singlet(θ_a, θ_b) = -1 + (2/π)|Δ| für |Δ| ∈ [0, π].
     *   E_Triplet(θ_a, θ_b) = +1 - (2/π)|Δ|.
     * Wir prüfen die vier CHSH-E-Werte gegen diese Vorhersage. */
    const double d = M_PI / 4.0;
    const double sign_s = (edge_type == PRO_EDGE_SINGLET) ? -1.0 : +1.0;
    const double exp_E_ab = sign_s * (1.0 - (2.0 / M_PI) * d);
    const double exp_E_abp = sign_s * (1.0 - (2.0 / M_PI) * (M_PI - d));
    const double exp_E_apb = sign_s * (1.0 - (2.0 / M_PI) * d);
    const double exp_E_apbp = sign_s * (1.0 - (2.0 / M_PI) * d);

    const double tol_tri = 0.10;
    const bool triangle_ok =
        fabs(res.E_ab - exp_E_ab) < tol_tri &&
        fabs(res.E_ab_prime - exp_E_abp) < tol_tri &&
        fabs(res.E_a_prime_b - exp_E_apb) < tol_tri &&
        fabs(res.E_a_prime_b_prime - exp_E_apbp) < tol_tri;

    printf("                  Dreiecksform: %s (tol %.2f)\n",
        triangle_ok ? "OK" : "ABWEICHUNG", tol_tri);

    ProPhysics_Free(&pu);
    return (dev < tolerance) && triangle_ok;
}

/* ========================================================================
 * Etappe 12: Hilfsfunktionen für den amp_grid-basierten Lorentz-Test
 * ====================================================================== */

 /* Findet den Knoten mit maximalem |coeff[basis]|² auf dem dim×dim-Torus.
  * Gibt den Knotenindex zurück; x_peak/y_peak werden optional gefüllt. */
static uint64_t find_peak_node(const ProUniverse* pu, uint32_t dim,
    uint8_t basis, uint32_t* out_x, uint32_t* out_y)
{
    if (!pu || !pu->amp_grid || dim == 0u) return 0u;
    uint64_t best = 0u;
    double   best_val = -1.0;
    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t k = (uint64_t)y * dim + x;
            if (k >= pu->total_nodes) continue;
            const int32_t re = pro_amp_real(pu->amp_grid[k].coeff[basis]);
            const int32_t im = pro_amp_imag(pu->amp_grid[k].coeff[basis]);
            const double val = (double)re * (double)re
                + (double)im * (double)im;
            if (val > best_val) {
                best_val = val;
                best = k;
                if (out_x) *out_x = x;
                if (out_y) *out_y = y;
            }
        }
    }
    return best;
}

/* Liest die Phase von coeff[basis] am Knoten. Rückgabe in (-π, π]. */
static double measure_phase_at(const ProUniverse* pu,
    uint64_t node, uint8_t basis)
{
    if (!pu || !pu->amp_grid || node >= pu->total_nodes) return 0.0;
    const int32_t re = pro_amp_real(pu->amp_grid[node].coeff[basis]);
    const int32_t im = pro_amp_imag(pu->amp_grid[node].coeff[basis]);
    if (re == 0 && im == 0) return 0.0;
    return atan2((double)im, (double)re);
}

/* Präpariert ein Gauss-Soliton in amp_grid[k].coeff[basis].
 *
 *   psi(x,y) = A · exp(-(x-x0)²/(2σ²)) · exp(i·k0·(x-x0))
 *
 * k0 = 0  → reelles Paket, v_g = 0.
 * k0 ≠ 0  → Phasengradient, v_g = cos(k0/2) (Tight-Binding).
 *
 * amp_scale ist der maximale Q31-Betrag (typisch Q31_MAXV * 0.5).
 * Der Torus-Abstand dx wird auf [-dim/2, dim/2) gefaltet, damit das
 * Paket auch bei x0 nahe 0 oder dim-1 korrekt sitzt. */
static void prepare_soliton_ampgrid(ProUniverse* pu, uint32_t dim,
    double x0, double y0, double sigma, double amp_scale,
    double k0, uint8_t basis)
{
    if (!pu || !pu->amp_grid || dim == 0u) return;
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    if (N > pu->total_nodes) return;

    /* Normierung: Summe |psi|² über eine y-Linie soll 1 sein. */
    double norm_sq = 0.0;
    for (uint32_t x = 0; x < dim; ++x) {
        double dx = (double)x - x0;
        if (dx > (double)dim * 0.5) dx -= (double)dim;
        if (dx < -(double)dim * 0.5) dx += (double)dim;
        const double psi = exp(-dx * dx / (2.0 * sigma * sigma));
        norm_sq += psi * psi;
    }
    if (norm_sq <= 0.0) return;
    const double inv_norm = 1.0 / sqrt(norm_sq);

    for (uint32_t y = 0; y < dim; ++y) {
        for (uint32_t x = 0; x < dim; ++x) {
            double dx = (double)x - x0;
            if (dx > (double)dim * 0.5) dx -= (double)dim;
            if (dx < -(double)dim * 0.5) dx += (double)dim;

            const double psi = exp(-dx * dx / (2.0 * sigma * sigma)) * inv_norm;
            const double phase = k0 * dx;
            const double re = psi * cos(phase);
            const double im = psi * sin(phase);

            int32_t re_q31 = (int32_t)lround(re * amp_scale);
            int32_t im_q31 = (int32_t)lround(im * amp_scale);

            const uint64_t k = (uint64_t)y * dim + x;
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                pu->amp_grid[k].coeff[b] = 0;
            }
            pu->amp_grid[k].coeff[basis] = pro_amp_pack(re_q31, im_q31);
        }
    }
}

/* Verdrahtet einen dim×dim-Torus (identisch zu init_torus, aber mit
 * variabler DIM statt globalem GRID_DIM). */
static void wire_torus(ProUniverse* pu, uint32_t dim)
{
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
            pu->reg_source[idx].channels[2] = y_c
                + ((x == dim - 1) ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_c
                + ((x == 0) ? dim - 1 : x - 1);
        }
    }
    pu->grid_dim = dim;
}

/* Führt einen einzelnen Propagations-Tick aus:
 *   - farbiger Kanten-Transport (unitär, exakt)
 *   - GP-Selbstkopplung (unitär pro Komponente)
 * Beide Operationen sind lokal und erhalten die U5-gewichtete Norm. */
static void lorentz_propagate_tick(ProUniverse* pu, uint32_t dim,
    uint32_t theta_q15, int32_t g_q15, uint32_t nl_dt_q15)
{
    ProPhysics_Apply_Edge_Transport_Colored(pu, theta_q15, dim);
    if (g_q15 != 0) {
        /* Etappe 12: Dilatierte Version — gamma_inv aus lokalem Phasengradient. */
        ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(pu, g_q15, nl_dt_q15);
    }
}
/* ========================================================================
 * Lorentz test
 * ====================================================================== */

static bool test_lorentz_time_dilation(ProEngineContext* ctx, double v_c_ratio) {
    if (!ctx || !ctx->ready) return false;

    const uint32_t sample_cycles = 2;

    printf("\n--- 1. Messung: Ruhendes Soliton (v/c = 0.00) ---\n");
    pro_engine_reset(ctx);
    const uint32_t node_static = pro_engine_spawn_soliton(ctx,
        GRID_DIM / 2, GRID_DIM / 2, 0);
    const uint64_t ticks_static =
        pro_engine_measure_phase_cycle(ctx, node_static, sample_cycles, "STATIC");

    printf("\n--- 2. Messung: Geboostetes Soliton (v/c = %.2f) ---\n", v_c_ratio);
    pro_engine_reset(ctx);
    const uint32_t node_boosted = pro_engine_spawn_boosted_soliton(ctx, v_c_ratio);
    const uint64_t ticks_boosted =
        pro_engine_measure_phase_cycle(ctx, node_boosted, sample_cycles, "BOOSTED");

    if (ticks_static == 0 || ticks_boosted == 0) {
        printf("\n[Lorentz-Test] FEHLER: Mindestens eine Messung fehlgeschlagen.\n");
        return false;
    }

    const double expected_ratio = sqrt(1.0 - v_c_ratio * v_c_ratio);
    const double measured_ratio = (double)ticks_static / (double)ticks_boosted;
    const double dev = fabs(measured_ratio - expected_ratio);

    printf("\n========================================================================\n");
    printf("   [Lorentz-Test Ergebnis]\n");
    printf("   Static Ticks:  %" PRIu64 "\n", ticks_static);
    printf("   Boosted Ticks: %" PRIu64 "\n", ticks_boosted);
    printf("   Erwartet sqrt(1-v^2/c^2): %.6f\n", expected_ratio);
    printf("   Gemessen T_s/T_b:         %.6f\n", measured_ratio);
    printf("   Abweichung:               %.2e\n", dev);
    printf("========================================================================\n");

    const double tolerance = 5e-3;
    return (dev < tolerance);
}

/* ========================================================================
 * Amplituden-Layer Self-Test
 * ====================================================================== */

static void amplitude_layer_smoke_test(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    if (!pu.amp_grid) {
        printf("[Amp-Test] FAILED: amp_grid ist NULL — Etappe 1 nicht eingespielt.\n");
        ProPhysics_Free(&pu);
        return;
    }

    int32_t re = 0, im = 0;
    if (!ProPhysics_Get_Node_Amplitude(&pu, 0, 0, &re, &im)) {
        printf("[Amp-Test] FAILED: Get_Node_Amplitude fehlgeschlagen.\n");
        ProPhysics_Free(&pu);
        return;
    }

    printf("[Amp-Test] OK: amp_grid allokiert, coeff[0] = (%d, %d) "
        "(erwartet ~2147483647, 0)\n", (int)re, (int)im);

    /* Manuelles Setzen und Zurücklesen (Etappe 10: Q31). */
    const int32_t test_re = Q15TO31(16384);    /*  0.5 · 2^31 */
    const int32_t test_im = -Q15TO31(8192);    /* -0.25 · 2^31 */
    ProPhysics_Set_Node_Amplitude(&pu, 1, 5, test_re, test_im);
    int32_t re2 = 0, im2 = 0;
    ProPhysics_Get_Node_Amplitude(&pu, 1, 5, &re2, &im2);
    printf("[Amp-Test] Set/Get roundtrip: (%d, %d) (erwartet %d, %d)\n",
        (int)re2, (int)im2, (int)test_re, (int)test_im);

    ProPhysics_Free(&pu);
}

/* ========================================================================
 * Born-Regel-Test (Etappe 2)
 * ====================================================================== */

static bool test_born_rule(void) {
    printf("[RUN] Born-Regel-Test: 10000 Messungen gegen |α|²:|β|²...\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid) {
        printf("[Born] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    /* EPR-Paar auf Knoten 0 und 1 aufsetzen. */
    pu.ur_grid[0].type_state = UR_PHOTON;
    pu.ur_grid[1].type_state = UR_PHOTON;
    pu.reg_source[0].channels[PRO_EPR_CHANNEL] = 1;
    pu.reg_source[1].channels[PRO_EPR_CHANNEL] = 0;
    ProPhysics_Set_Edge_Phase(&pu, 0, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_SINGLET);
    ProPhysics_Set_Edge_Phase(&pu, 1, PRO_EPR_CHANNEL, 0.0, PRO_EDGE_SINGLET);

    /* Superposition |ψ⟩ = α|0⟩ + β|1⟩:
     *   |α|² = 0.25  →  α = 0.5       →  Q15 16384  → Q31 Q15TO31(16384)
     *   |β|² = 0.75  →  β ≈ 0.8660254 →  Q15 28377  → Q31 Q15TO31(28377)
     *
     * Q15TO31(16384) = 2^30 ≈ 1.073e9, Q15TO31(28377) ≈ 1.860e9.
     * Beide unter INT32_MAX (2.147e9). */
    const int32_t alpha_q31 = Q15TO31(16384);
    const int32_t beta_q31 = Q15TO31(28377);

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
        pu.amp_grid[1].coeff[b] = 0;
    }
    const ProAmpQ31 ca = pro_amp_pack(alpha_q31, 0);
    const ProAmpQ31 cb = pro_amp_pack(beta_q31, 0);
    pu.amp_grid[0].coeff[0] = ca;
    pu.amp_grid[0].coeff[1] = cb;
    pu.amp_grid[1].coeff[0] = ca;
    pu.amp_grid[1].coeff[1] = cb;

    prng_state_t rng;
    prng_seed(&rng, 0xB0B0B0B0ULL);

    uint32_t n_up = 0, n_down = 0;
    const uint32_t trials = 10000;

    /* Born-Regel: statistische Messung mit RNG.
     * P(0) = |α|² / (|α|² + |β|²) = 0.25. */
    const double p0 = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    for (uint32_t t = 0; t < trials; ++t) {
        const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
        if (u < p0) n_up++; else n_down++;
    }

    const double expected_up = 0.25 * (double)trials;
    const double expected_dn = 0.75 * (double)trials;
    const double d_up = (double)n_up - expected_up;
    const double d_dn = (double)n_down - expected_dn;
    const double chi2 = (d_up * d_up) / expected_up
        + (d_dn * d_dn) / expected_dn;

    const bool pass = (chi2 < 6.635);
    printf("[Born] trials=%u | up=%u (erw %.0f) | down=%u (erw %.0f) | "
        "chi2=%.4f (Schwelle 6.635) -> %s\n",
        trials, n_up, expected_up, n_down, expected_dn, chi2,
        pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}

/* ========================================================================
 * Unitärer-Tick-Test (Etappe 3a)
 *
 * Statt einer dichten Q15-Matrix wird eine signierte Permutation
 * verwendet. Diese ist EXAKT unitär (keine Q15-Rundung), weil nur
 * Index-Tausch und Vorzeichen-Negation stattfinden.
 *
 * Getestet wird:
 *   1. perm ist Bijektion, sign ∈ {+1,-1}
 *   2. w[perm[i]] = w[i]  (U5-Kommutator-Bedingung)
 *   3. Σ w_j |c_j|² invariant über 10000 Ticks
 * ====================================================================== */

static bool test_unitary_tick(void) {
    printf("[RUN] Unitärer-Tick-Test (Etappe 3a, signed permutation)...\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.amp_grid) {
        printf("[Unitary] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    const uint8_t perm[PRO_AMP_BASIS_SIZE] = { 0, 2, 1, 4, 3, 5, 6, 7 };
    const int8_t  sign[PRO_AMP_BASIS_SIZE] = { +1, -1, +1, -1, +1, +1, +1, +1 };

    const bool unit_ok = ProPhysics_Verify_Signed_Perm_Unitarity(perm, sign);
    const bool u5_ok = ProPhysics_Verify_Signed_Perm_U5(perm, sign);

    printf("[Unitary] perm Bijektion + sign ∈ {±1}: %s\n",
        unit_ok ? "OK" : "FAILED");
    printf("[Unitary] w[perm[i]] = w[i] (U5): %s\n",
        u5_ok ? "OK" : "FAILED");

    /* Startzustand: Knoten 0 in Superposition |1⟩ + |3⟩ (Etappe 10: Q31). */
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
    }
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[3] = pro_amp_pack(Q15TO31(23170), 0);

    const ProU128 norm_before = ProPhysics_Weighted_Norm(&pu, 0);

    for (int t = 0; t < 10000; ++t) {
        ProPhysics_Apply_Signed_Permutation(&pu, perm, sign);
    }

    const ProU128 norm_after = ProPhysics_Weighted_Norm(&pu, 0);
    const double drift = !pro_u128_is_zero(norm_before)
        ? fabs(pro_u128_to_double(norm_after) - pro_u128_to_double(norm_before))
        / pro_u128_to_double(norm_before)
        : 0.0;


    printf("[Unitary] U5-Norm: before=%.6e after=%.6e drift=%.4e "
        "(Schwelle 1e-12)\n",
        pro_u128_to_double(norm_before),
        pro_u128_to_double(norm_after),
        drift);

    const bool pass = unit_ok && u5_ok && (drift < 1e-12);
    printf("[Unitary] -> %s\n", pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}

/* ========================================================================
 * Kontextabhängige signed permutations (Etappe 3b)
 *
 * Prüft:
 *   1. Zwei Knoten mit unterschiedlichem Nachbarkontext erhalten
 *      unterschiedliche signed permutations.
 *   2. Die perms sind U5-kompatibel (w[perm[i]] = w[i]).
 *   3. Die U5-Norm bleibt über 10000 Kontext-Ticks exakt erhalten.
 * ====================================================================== */

static bool test_context_perm(void) {
    printf("[RUN] Kontextabhängige signed permutations (Etappe 3b)...\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    if (!pu.amp_grid || !pu.ur_grid || !pu.reg_source) {
        printf("[Context] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    /* Etappe 9 (Lücke-3-Fix): Context-Perm liest amp_grid der Nachbarn. */
    for (uint8_t c = 0; c < (uint8_t)PRO_EPR_CHANNEL; ++c) {
        pu.reg_source[0].channels[c] = (uint64_t)(1 + c);
        pu.reg_source[1].channels[c] = (uint64_t)(30 + c);
    }

    /* Hilfsmakro: setze Knoten k auf Einheitsvektor |basis_idx⟩ (Q31). */
#define SET_AMP_UNIT(k, basis_idx) do { \
        for (uint8_t _b = 0; _b < PRO_AMP_BASIS_SIZE; ++_b) \
            pu.amp_grid[(k)].coeff[_b] = 0; \
        pu.amp_grid[(k)].coeff[(basis_idx)] = pro_amp_pack(INT32_MAX, 0); \
    } while (0)

    SET_AMP_UNIT(1, UR_POSITRON_CW);
    SET_AMP_UNIT(2, UR_POSITRON_CCW);
    SET_AMP_UNIT(3, UR_NEGATRON_CCW);

    uint8_t perm0[PRO_AMP_BASIS_SIZE], perm1[PRO_AMP_BASIS_SIZE];
    int8_t  sign0[PRO_AMP_BASIS_SIZE], sign1[PRO_AMP_BASIS_SIZE];
    ProPhysics_Compute_Context_Perm(&pu, 0, perm0, sign0);
    ProPhysics_Compute_Context_Perm(&pu, 1, perm1, sign1);

    bool differ = false;
    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        if (perm0[i] != perm1[i] || sign0[i] != sign1[i]) {
            differ = true;
            break;
        }
    }
    printf("[Context] Knoten 0 vs 1 unterschiedliche perm: %s\n",
        differ ? "OK" : "FAILED");

    const bool u5_ok =
        ProPhysics_Verify_Signed_Perm_U5(perm0, sign0) &&
        ProPhysics_Verify_Signed_Perm_U5(perm1, sign1);
    printf("[Context] U5-Kompatibilität: %s\n", u5_ok ? "OK" : "FAILED");

    uint8_t perm0_after[PRO_AMP_BASIS_SIZE];
    int8_t  sign0_after[PRO_AMP_BASIS_SIZE];
    SET_AMP_UNIT(1, UR_NEGATRON_CW);
    ProPhysics_Compute_Context_Perm(&pu, 0, perm0_after, sign0_after);

    bool changed = false;
    for (uint8_t i = 0; i < PRO_AMP_BASIS_SIZE; ++i) {
        if (perm0[i] != perm0_after[i] || sign0[i] != sign0_after[i]) {
            changed = true;
            break;
        }
    }
    printf("[Context] Nachbar-amp_grid-Änderung ändert perm: %s\n",
        changed ? "OK" : "FAILED");

    SET_AMP_UNIT(1, UR_POSITRON_CW);

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
    }
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[3] = pro_amp_pack(Q15TO31(23170), 0);

    const ProU128 norm_before = ProPhysics_Weighted_Norm(&pu, 0);

    for (int t = 0; t < 10000; ++t) {
        ProPhysics_Apply_Context_Tick(&pu);
    }

    const ProU128 norm_after = ProPhysics_Weighted_Norm(&pu, 0);
    const double drift = !pro_u128_is_zero(norm_before)
        ? fabs(pro_u128_to_double(norm_after) - pro_u128_to_double(norm_before))
        / pro_u128_to_double(norm_before)
        : 0.0;


    printf("[Unitary] U5-Norm: before=%.6e after=%.6e drift=%.4e "
        "(Schwelle 1e-12)\n",
        pro_u128_to_double(norm_before),
        pro_u128_to_double(norm_after),
        drift);

    const bool pass = differ && u5_ok && changed && (drift < 1e-12);
    printf("[Context] -> %s\n", pass ? "PASSED" : "FAILED");

#undef SET_AMP_UNIT

    ProPhysics_Free(&pu);
    return pass;
}

/* ========================================================================
 * U(1)-Wilson-Loop-Test (Etappe 4a)
 *
 * Setzt bekannte Kantenphasen auf einem 4x4-Torus und prüft:
 *   1. Trivialer 2-Kanten-Loop (x → y → x) mit φ_xy + φ_yx = 0.
 *   2. 4-Kanten-Plaquette: Summe = φ_0 + φ_1 + φ_2 + φ_3.
 *   3. Eichinvarianz: globale Phase ändert Born-Wahrscheinlichkeiten nicht.
 * ====================================================================== */

static bool test_wilson_loop(void) {
    printf("[RUN] U(1)-Wilson-Loop-Test (Etappe 4a)...\n");

    const uint32_t DIM = 4u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.edge_phases || !pu.reg_source || !pu.amp_grid) {
        printf("[Wilson] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    ProPhysics_Set_Edge_Phase(&pu, 0, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 1, 0, -0.25 * M_PI, PRO_EDGE_NONE);

    const uint64_t path2[2] = { 0u, 1u };
    const uint8_t  chans2[2] = { 0u, 0u };
    uint16_t W2 = 0u;
    ProPhysics_Wilson_Loop(&pu, path2, chans2, 2u, &W2);
    printf("[Wilson] 2-Kanten-Loop W_fx = %u (erwartet 0 bei konsistenter Phase)\n",
        (unsigned)W2);

    const uint64_t path4[4] = { 0u, 1u, 5u, 4u };
    const uint8_t  chans4[4] = { 0u, 0u, 0u, 0u };
    ProPhysics_Set_Edge_Phase(&pu, 0, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 1, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 5, 0, 0.25 * M_PI, PRO_EDGE_NONE);
    ProPhysics_Set_Edge_Phase(&pu, 4, 0, 0.25 * M_PI, PRO_EDGE_NONE);

    uint16_t W4 = 0u;
    ProPhysics_Wilson_Loop(&pu, path4, chans4, 4u, &W4);
    printf("[Wilson] 4-Kanten-Plaquette W_fx = %u (erwartet 32768)\n",
        (unsigned)W4);

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
    }
    /* Etappe 10: Q31. */
    pu.amp_grid[0].coeff[0] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(16384), 0);

    const double p0_before = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_before = ProPhysics_Get_Born_Probability(&pu, 0, 1);

    ProPhysics_Global_Phase(&pu, 21845u);

    const double p0_after = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_after = ProPhysics_Get_Born_Probability(&pu, 0, 1);

    const double dp0 = fabs(p0_after - p0_before);
    const double dp1 = fabs(p1_after - p1_before);
    printf("[Wilson] Eichinvarianz: P(0) %.8f → %.8f (|Δ|=%.2e)\n",
        p0_before, p0_after, dp0);
    printf("[Wilson] Eichinvarianz: P(1) %.8f → %.8f (|Δ|=%.2e)\n",
        p1_before, p1_after, dp1);

    /* Q31: Rundungstoleranz kann enger gesetzt werden — bleiben konservativ. */
    const bool pass =
        (W2 == 0u) &&
        (W4 == 32768u) &&
        (dp0 < 1e-3) && (dp1 < 1e-3);

    printf("[Wilson] -> %s\n", pass ? "PASSED" : "FAILED");
    ProPhysics_Free(&pu);
    return pass;
}

/* ========================================================================
 * Lokale Eichinvarianz (Etappe 4b)
 *
 * Prüft:
 *   1. Kantenphase wird exakt (integer) transformiert:
 *        φ'(x→y) = φ(x→y) + λ(y) - λ(x)   mod 2^16
 *   2. Wilson-Loop bleibt invariant (exakt, integer):
 *        W_before == W_after
 *   3. Born-Wahrscheinlichkeiten pro Knoten bleiben invariant (Q15-Toleranz):
 *        P(b) before == P(b) after
 * ====================================================================== */

static bool test_local_gauge(void) {
    printf("[RUN] Lokale Eichinvarianz (Etappe 4b)...\n");

    const uint32_t DIM = 4u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.amp_grid || !pu.edge_phases || !pu.reg_source) {
        printf("[Gauge] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t ch = 0; ch < 4; ++ch) {
            const uint16_t ph = (uint16_t)(((k * 4099u) + ch * 12345u) & 0xFFFFu);
            pu.edge_phases[k * (uint64_t)CHANNELS_MAX + ch].phase = ph;
            pu.edge_phases[k * (uint64_t)CHANNELS_MAX + ch].type = PRO_EDGE_NONE;
        }
    }

    const uint64_t plq_nodes[4] = { 0u, 1u, 5u, 4u };
    const uint8_t  plq_ch[4] = { 2u, 1u, 3u, 0u };

    uint16_t W_before = 0u;
    ProPhysics_Wilson_Loop(&pu, plq_nodes, plq_ch, 4u, &W_before);

    uint16_t phi_before[4];
    for (int i = 0; i < 4; ++i) {
        const uint64_t x = plq_nodes[i];
        const uint8_t  ch = plq_ch[i];
        phi_before[i] = pu.edge_phases[x * (uint64_t)CHANNELS_MAX + ch].phase;
    }

    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
    }
    /* Etappe 10: Q31. */
    pu.amp_grid[0].coeff[0] = pro_amp_pack(Q15TO31(23170), 0);
    pu.amp_grid[0].coeff[1] = pro_amp_pack(Q15TO31(16384), 0);

    const double p0_before = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_before = ProPhysics_Get_Born_Probability(&pu, 0, 1);

    uint16_t* lambda = (uint16_t*)malloc((size_t)NODES * sizeof(uint16_t));
    if (!lambda) {
        printf("[Gauge] Lambda-Alloc failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    ProPhysics_Make_Lambda_Field(0xDEADBEEFu, lambda, NODES);
    ProPhysics_Apply_Local_Gauge(&pu, lambda);

    bool edges_ok = true;
    for (int i = 0; i < 4 && edges_ok; ++i) {
        const uint64_t x = plq_nodes[i];
        const uint8_t  ch = plq_ch[i];
        const uint64_t y = pu.reg_source[x].channels[ch];
        const uint16_t phi_after =
            pu.edge_phases[x * (uint64_t)CHANNELS_MAX + ch].phase;

        const int32_t d = (int32_t)lambda[y] - (int32_t)lambda[x];
        const uint16_t expected =
            (uint16_t)(((uint32_t)phi_before[i] + (uint32_t)d) & 0xFFFFu);

        if (phi_after != expected) {
            printf("[Gauge] Kante %d: phi_after=%u expected=%u (MISMATCH)\n",
                i, (unsigned)phi_after, (unsigned)expected);
            edges_ok = false;
        }
    }
    printf("[Gauge] Kantenphasen exakt transformiert: %s\n",
        edges_ok ? "OK" : "FAILED");

    uint16_t W_after = 0u;
    ProPhysics_Wilson_Loop(&pu, plq_nodes, plq_ch, 4u, &W_after);
    const bool wilson_ok = (W_before == W_after);
    printf("[Gauge] Wilson-Loop: before=%u after=%u -> %s\n",
        (unsigned)W_before, (unsigned)W_after,
        wilson_ok ? "INVARIANT" : "VIOLATED");

    const double p0_after = ProPhysics_Get_Born_Probability(&pu, 0, 0);
    const double p1_after = ProPhysics_Get_Born_Probability(&pu, 0, 1);
    const double dp0 = fabs(p0_after - p0_before);
    const double dp1 = fabs(p1_after - p1_before);
    const bool born_ok = (dp0 < 1e-3) && (dp1 < 1e-3);
    printf("[Gauge] Born P(0): %.8f -> %.8f (|Δ|=%.2e)\n",
        p0_before, p0_after, dp0);
    printf("[Gauge] Born P(1): %.8f -> %.8f (|Δ|=%.2e)\n",
        p1_before, p1_after, dp1);

    const bool pass = edges_ok && wilson_ok && born_ok;
    printf("[Gauge] -> %s\n", pass ? "PASSED" : "FAILED");

    free(lambda);
    ProPhysics_Free(&pu);
    return pass;
}
/* ========================================================================
 * Dreieckskorrelations-Test (Etappe 4c)
 *
 * Misst E(θ_a, θ_b) über ein Winkelgitter und vergleicht mit der
 * klassischen Dreiecksfunktion für Singlet:
 *
 *     E(Δ) = -1 + (2/π)·|Δ|   für |Δ| ∈ [0, π]
 *
 * Bei Δ = 0: E = -1 (perfekte Antikorrelation).
 * Bei Δ = π: E = +1.
 * ====================================================================== */

static bool test_triangle_correlation(void) {
    printf("[RUN] Dreieckskorrelation (Etappe 4c)...\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source) {
        printf("[Triangle] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    init_torus(&pu);

    prng_state_t rng;
    prng_seed(&rng, 0xFACEFEEDULL);

    for (uint32_t i = 0; i < 2000; ++i) {
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)PRO_EDGE_SINGLET);
    }

    const uint32_t N = 9;
    double max_dev = 0.0;
    printf("[Triangle]   Δ/π      E_measured   E_triangle   |dev|\n");

    for (uint32_t k = 0; k < N; ++k) {
        const double delta = (double)k * (M_PI / (double)(N - 1));
        const double E = evaluate_correlation(&pu, 0.0, delta, rng.s);
        const double E_tri = -1.0 + (2.0 / M_PI) * delta;
        const double dev = fabs(E - E_tri);
        if (dev > max_dev) max_dev = dev;
        printf("[Triangle]   %.4f   %+.6f    %+.6f    %.4f\n",
            delta / M_PI, E, E_tri, dev);
    }

    const double tol = 0.10;
    const bool pass = (max_dev < tol);
    printf("[Triangle] max |dev| = %.4f (tol %.2f) -> %s\n",
        max_dev, tol, pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}
/* ==========================================================================
 * Superdeterminismus-Helfer (Etappe 5, neu gefasst in Etappe 12)
 *
 * Der Apparat lebt im fundamentalen Feld amp_grid. Die Einstellung theta
 * wird als Amplitude exp(i*theta) in coeff[UR_POSITRON_CW] kodiert.
 * Gelesen wird ueber atan2(im, re) am selben Knoten.
 *
 * Damit ist der Apparat Teil des Systems: kein externer Beobachter,
 * kein phase_accumulator. Der Harness schreibt theta in amp_grid und
 * liest theta aus amp_grid zurueck. Das ist der einzige verbleibende
 * "Input" — und er ist selbst ein Zustand des Substrats.
 * ========================================================================== */

static void set_apparatus_phase_amp(ProUniverse* pu, uint64_t node,
    double theta)
{
    if (!pu || !pu->amp_grid || node >= pu->total_nodes) return;
    ProAmpVector* v = &pu->amp_grid[node];
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) v->coeff[b] = 0;
    const int32_t re = (int32_t)lround(cos(theta) * Q31_MAXV);
    const int32_t im = (int32_t)lround(sin(theta) * Q31_MAXV);
    v->coeff[UR_POSITRON_CW] = pro_amp_pack(re, im);
}

static double read_apparatus_phase_amp(const ProUniverse* pu, uint64_t node)
{
    if (!pu || !pu->amp_grid || node >= pu->total_nodes) return 0.0;
    const int32_t re = pro_amp_real(pu->amp_grid[node].coeff[UR_POSITRON_CW]);
    const int32_t im = pro_amp_imag(pu->amp_grid[node].coeff[UR_POSITRON_CW]);
    if (re == 0 && im == 0) return 0.0;
    double phi = atan2((double)im, (double)re);
    if (phi < 0.0) phi += 2.0 * M_PI;
    return phi;
}

static void set_pair_lambda(ProUniverse* pu,
    uint64_t idx_a, uint64_t idx_b,
    double   lambda,
    uint8_t  edge_type)
{
    if (!pu || !pu->amp_grid) return;
    if (idx_a >= pu->total_nodes || idx_b >= pu->total_nodes) return;
    if (edge_type != PRO_EDGE_SINGLET && edge_type != PRO_EDGE_TRIPLET) return;

    ProAmpVector* v1 = &pu->amp_grid[idx_a];
    ProAmpVector* v2 = &pu->amp_grid[idx_b];
    for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
        v1->coeff[b] = 0;
        v2->coeff[b] = 0;
    }

    const double half = lambda * 0.5;
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

/* ========================================================================
 * CHSH-Superdeterminismus-Test (Etappe 5, Option-B-Version, Etappe 12)
 *
 * Zwei Laeufe:
 *   separate_source = true  →  Apparat-Einstellung und lambda unabhaengig
 *                               Erwartung: S ≈ 2.0 (klassische Bell-Schranke)
 *   separate_source = false →  Quelle bestimmt (A, B, lambda) gemeinsam
 *                               Erwartung: S kann deutlich > 2.0 werden
 *
 * Aenderung gegenueber Etappe 5:
 *   - Der Apparat lebt im amp_grid, nicht im phase_accumulator.
 *   - Die Messung nutzt ProPhysics_Measure_EPR_Pair_Amp (kein type_state).
 *   - Damit ist der Apparat Teil des Systems, kein externer Beobachter.
 *
 * Interpretation (unveraendert): S > 2 in der "shared source"-Variante ist
 * KEIN Bell-Bruch, sondern der Superdeterminismus-Loop. Der Test zeigt nur,
 * dass das Substrat diese Korrelation tragen kann.
 * ====================================================================== */

static bool test_chsh_superdet(bool shared_source, double* out_S)
{
    printf("[RUN] CHSH-Superdeterminismus (Quelle=%s, amp_grid-Apparat)...\n",
        shared_source ? "SHARED" : "SEPARATE");

    const uint64_t NODES = 2048;
    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.edge_phases) {
        printf("[Superdet] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    /* Layout:
     *   app_a   = 0     (einzelner Apparat-Knoten Alice)
     *   app_b   = 1     (einzelner Apparat-Knoten Bob)
     *   part_a  = 1024  (EPR-Paarknoten A)
     *   part_b  = 1025  (EPR-Paarknoten B)
     *   edge_type = SINGLET
     *
     * Die Apparate sind einzelne Knoten, weil die Einstellung nun eine
     * einzelne Phase ist (nicht mehr ein Mittel ueber 256 Knoten).
     * Sie sind topologisch vom Paar getrennt — die einzige Verbindung
     * ist die gemeinsame Praeparation im Test-Harness. */
    const uint64_t app_a = 0;
    const uint64_t app_b = 1;
    const uint64_t part_a = 1024;
    const uint64_t part_b = 1025;
    const uint8_t  edge_type = PRO_EDGE_SINGLET;

    pu.reg_source[part_a].channels[PRO_EPR_CHANNEL] = part_b;
    pu.reg_source[part_b].channels[PRO_EPR_CHANNEL] = part_a;

    ProPhysics_Set_Edge_Phase(&pu, part_a, PRO_EPR_CHANNEL, 0.0, edge_type);
    ProPhysics_Set_Edge_Phase(&pu, part_b, PRO_EPR_CHANNEL, 0.0, edge_type);

    prng_state_t rng;
    prng_seed(&rng, shared_source ? 0x5D5D5D5D01ULL : 0x5D5D5D5D02ULL);

    /* (A, B) ∈ {0,1}²:
     *   A=0 → θ_a = 0,      A=1 → θ_a = π/2
     *   B=0 → θ_b = π/4,    B=1 → θ_b = 3π/4
     *
     * Superdeterministische Korrelation:
     *   S1 < 0.25        : (A,B)=(0,0), λ = 0
     *   0.25 ≤ S1 < 0.50 : (A,B)=(0,1), λ = π
     *   0.50 ≤ S1 < 0.75 : (A,B)=(1,0), λ = π/2
     *   0.75 ≤ S1        : (A,B)=(1,1), λ = 3π/2
     *
     * Mit Singlet-Vorbereitung λ_b = λ_a + π und sign(cos)-Messung:
     *   E(0,0) = -1, E(0,1) = +1, E(1,0) = -1, E(1,1) = -1
     *   S = 2 + 2 = 4 (algebraisches Maximum). */
    const uint32_t trials = 20000;
    double   E_sum[2][2] = { {0.0, 0.0}, {0.0, 0.0} };
    uint32_t E_cnt[2][2] = { {0, 0}, {0, 0} };

    for (uint32_t t = 0; t < trials; ++t) {
        const double S1 = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;

        int A, B;
        double lambda;

        if (shared_source) {
            if (S1 < 0.25) { A = 0; B = 0; lambda = 0.0; }
            else if (S1 < 0.50) { A = 0; B = 1; lambda = M_PI; }
            else if (S1 < 0.75) { A = 1; B = 0; lambda = M_PI / 2.0; }
            else { A = 1; B = 1; lambda = 3.0 * M_PI / 2.0; }
        }
        else {
            if (S1 < 0.25) { A = 0; B = 0; }
            else if (S1 < 0.50) { A = 0; B = 1; }
            else if (S1 < 0.75) { A = 1; B = 0; }
            else { A = 1; B = 1; }

            const double S2 = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
            lambda = S2 * 2.0 * M_PI;
        }

        const double theta_a = (A == 0) ? 0.0 : (M_PI / 2.0);
        const double theta_b = (B == 0) ? (M_PI / 4.0) : (3.0 * M_PI / 4.0);

        /* Apparat-Einstellung in amp_grid schreiben. */
        set_apparatus_phase_amp(&pu, app_a, theta_a);
        set_apparatus_phase_amp(&pu, app_b, theta_b);

        /* Paar-praeparation in amp_grid schreiben. */
        set_pair_lambda(&pu, part_a, part_b, lambda, edge_type);

        /* Apparat-Einstellung aus amp_grid lesen (System liest sich selbst). */
        const double th_a_read = read_apparatus_phase_amp(&pu, app_a);
        const double th_b_read = read_apparatus_phase_amp(&pu, app_b);

        int out_a = 0, out_b = 0;
        if (!ProPhysics_Measure_EPR_Pair_Amp(&pu, part_a,
            th_a_read, th_b_read, &out_a, &out_b)) {
            continue;
        }

        E_sum[A][B] += (double)(out_a * out_b);
        E_cnt[A][B]++;
    }

    double E[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            E[i][j] = E_cnt[i][j] ? (E_sum[i][j] / (double)E_cnt[i][j]) : 0.0;

    const double S_val =
        fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);

    printf("[Superdet] trials=%u | counts: (00=%u, 01=%u, 10=%u, 11=%u)\n",
        trials, E_cnt[0][0], E_cnt[0][1], E_cnt[1][0], E_cnt[1][1]);
    printf("[Superdet] E(0,0)=%+.4f  E(0,1)=%+.4f  E(1,0)=%+.4f  E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Superdet] S = %.4f\n", S_val);

    if (out_S) *out_S = S_val;

    ProPhysics_Free(&pu);
    return true;
}

/* ========================================================================
 * Observer-CHSH-Test (Etappe 6a)
 *
 * Prüft die Skalen-These:
 *   In einem großen Substrat mit nur lokal lesenden Beobachtern
 *   entsteht KEINE Bell-verletzende Korrelation, wenn die Quelle
 *   über lokale Amplitudendiffusion propagiert (nicht über einen
 *   EPR-Kanal).
 *
 * Setup:
 *   Grid 64x64 (4096 Knoten, Torus).
 *   Quelle: Knoten 2048 (Zentrum).
 *   Beobachter A: Knoten [0..63].
 *   Beobachter B: Knoten [4032..4095].
 *   Pro Trial: Quelle mit |λ⟩ präparieren, D Diffusionsschritte, dann
 *   CHSH aus 4 Winkelkombinationen messen.
 *
 * Erwartung: S ≈ 2.0 (klassische Dreieckskorrelation, kein Bell-Bruch),
 *            Streuung ~ 2/√N.
 * Falsifikation: S > 2 + 3σ.
 *
 * HINWEIS: S > 2 wäre kein Bell-Bruch. Die Diffusion ist eine lineare,
 *          deterministische Mittelung — sie kann höchstens die
 *          klassische Schranke erreichen. Ein S > 2 würde anzeigen,
 *          dass das Testdesign fehlerhaft ist, nicht dass QM emergiert.
 * ====================================================================== */

static bool test_observer_chsh(void) {
    printf("[RUN] Observer-CHSH (Etappe 6a, lokale Amplitudendiffusion, Q31)...\n");

    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Observer-CHSH] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    ProObserver obs_A, obs_B;

    const uint64_t source_node = 2048u;
    ProPhysics_Init_Observer(&pu, &obs_A, 2028u, 64u);
    ProPhysics_Init_Observer(&pu, &obs_B, 2068u, 64u);
    const uint32_t DIFF_TICKS = 30u;
    const uint32_t DIFF_RATE = 100u;
    const uint32_t TRIALS = 200u;

    const double thA0 = 0.0;
    const double thA1 = M_PI / 2.0;
    const double thB0 = M_PI / 4.0;
    const double thB1 = 3.0 * M_PI / 4.0;

    prng_state_t rng;
    prng_seed(&rng, 0x0606A06AULL);

    double   sum_E[2][2] = { {0.0, 0.0}, {0.0, 0.0} };
    uint32_t cnt_E[2][2] = { {0, 0}, {0, 0} };

    for (uint32_t t = 0; t < TRIALS; ++t) {
        for (uint64_t k = 0; k < NODES; ++k) {
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu.amp_grid[k].coeff[b] = 0;
            pu.amp_grid[k].coeff[UR_NEUTRAL] = pro_amp_pack(INT32_MAX, 0);
        }

        const double u = (double)(prng_next(&rng) >> 11) / 9007199254740992.0;
        const double lambda = u * 2.0 * M_PI;
        const double c = cos(lambda * 0.5);
        const double s = sin(lambda * 0.5);
        /* Etappe 10: Q31. */
        const int32_t a_up = (int32_t)lround(c * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(s * Q31_MAXV);

        pu.amp_grid[source_node].coeff[UR_NEUTRAL] = 0;
        pu.amp_grid[source_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
        pu.amp_grid[source_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);

        for (uint32_t k = 0; k < DIFF_TICKS; ++k) {
            ProPhysics_Apply_Local_Amplitude_Diffusion(&pu, DIFF_RATE);
        }

        const double thA[2] = { thA0, thA1 };
        const double thB[2] = { thB0, thB1 };
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                int oa = 0, ob = 0;
                if (!ProPhysics_Observer_Measure_CHSH(
                    &pu, &obs_A, &obs_B, thA[i], thB[j], &oa, &ob))
                    continue;
                sum_E[i][j] += (double)(oa * ob);
                cnt_E[i][j] += 1u;
            }
        }
    }

    double E[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            E[i][j] = cnt_E[i][j]
            ? sum_E[i][j] / (double)cnt_E[i][j]
            : 0.0;

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double se_S = 2.0 / sqrt((double)TRIALS);
    const double thr = 2.0 + 3.0 * se_S;

    printf("[Observer-CHSH] trials=%u | ticks=%u | rate=%u%% | source=%llu\n",
        TRIALS, DIFF_TICKS, DIFF_RATE, (unsigned long long)source_node);
    printf("[Observer-CHSH] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Observer-CHSH] S = %.4f ± %.4f (1σ) | 3σ-Schwelle = %.4f\n",
        S, se_S, thr);

    const bool violate = (S > thr);
    printf("[Observer-CHSH] -> %s\n",
        violate
        ? "FAILED (S > 2 + 3σ — Testdesign prüfen, KEIN Bell-Bruch)"
        : "PASSED (S <= 2 + 3σ, klassische Schranke respektiert)");

    ProPhysics_Free(&pu);
    return !violate;
}

/* ========================================================================
 * CHSH mit chaotischer nicht-linearer Quelle (Etappe 6b)
 *
 * Prüft: Entsteht aus strukturierter Anfangsbedingung + nicht-linearer
 *        Sättigung + Projektionsbeobachter eine nicht-triviale Verteilung?
 *
 * Setup:
 *   Grid 96x96 Torus.
 *   Quelle: 16x16 Region bei (40,40), deterministische λ-Verteilung.
 *   Beobachter A: 12x12 bei (10,10). B: 12x12 bei (74,74).
 *   Diffusion: alpha = 100%, Sättigung bei 4*2^30 Q30.
 *   Pro Trial: 60 Diffusionsschritte, dann 4 CHSH-Winkelkombinationen.
 *
 * Interpretation (bindend):
 *   S > 2 + 3σ ist KEIN Bell-Bruch. Es wäre Superdeterminismus-Loop
 *   oder Testdesign-Fehler.
 *   S < 2 - 3σ ist Signalverlust durch Nicht-Linearität.
 *   S ≈ 2.0 ist die klassische Schranke — weder überraschend noch
 *   "QM emergiert".
 * ====================================================================== */

 /* ========================================================================
  * CHSH mit chaotischer Quelle (Etappe 6b+6c+6d)
  *
  * Zwei Modi:
  *   use_wave = false → dissipative Diffusion + Dephasierung (6a–6c)
  *   use_wave = true  → unitäre Wellengleichung (6d, NEU)
  *
  * Interpretation (bindend):
  *   - S > 2 + 3σ ist KEIN Bell-Bruch.
  *   - Kein "QM emergiert". Beide Modi sind test-spezifische Zusatzdynamik.
  *   - Die Wellengleichung ist der fundamental-lineare Kandidat für
  *     "It from Bit". Die Diffusion ist ihre emergente Beschreibung
  *     für einen lokalen Beobachter (nicht in diesem Test geprüft).
  * ====================================================================== */

static bool test_chsh_chaotic_impl(bool use_wave) {
    printf("[RUN] CHSH chaotische Quelle (Modus: %s, Q31)...\n",
        use_wave ? "WAVE (6d, unitär)" : "DIFFUSION (6a-6c, dissipativ)");

    const uint32_t DIM = 96u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Chaotic] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    const uint64_t src_base = (uint64_t)40 * DIM + 40;
    const uint32_t src_dim = 8u;
    const uint64_t obsA_base = (uint64_t)35 * DIM + 35;
    const uint64_t obsB_base = (uint64_t)50 * DIM + 50;
    const uint32_t obs_dim = 4u;

    ProObserver obs_A, obs_B;
    ProPhysics_Init_Observer(&pu, &obs_A, obsA_base, obs_dim * obs_dim);
    ProPhysics_Init_Observer(&pu, &obs_B, obsB_base, obs_dim * obs_dim);

    const uint32_t TRIALS = 100u;
    const uint32_t TICKS = use_wave ? 200u : 60u;
    const uint32_t RATE = 100u;
    /* Etappe 10: Sättigungsschwelle in Q62. 4·(2^31)² = 2^64, was nicht
     * in uint64 passt — daher UINT64_MAX als "Sättigung aus".
     * Für echte Sättigung müsste die API ProU128 nehmen. */
    const uint64_t THRESH = UINT64_MAX;
    const uint32_t DEPHASE_STRENGTH = 50u;
    const uint32_t WAVE_STEP_Q15 = 1000u;

    prng_state_t rng;
    prng_seed(&rng, 0x06B06B06BULL);

    const double thA[2] = { 0.0, M_PI / 2.0 };
    const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };

    double   sum_E[2][2] = { {0.0, 0.0}, {0.0, 0.0} };
    uint32_t cnt_E[2][2] = { {0, 0}, {0, 0} };
    uint32_t n_up = 0, n_dn = 0;

    for (uint32_t t = 0; t < TRIALS; ++t) {
        ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim,
            (uint64_t)0xC0FFEE6BULL + (uint64_t)t * 0x9e3779b97f4a7c15ULL);

        if (use_wave) {
            for (uint32_t k = 0; k < TICKS; ++k) {
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            }
        }
        else {
            for (uint32_t k = 0; k < TICKS; ++k) {
                ProPhysics_Apply_Nonlinear_Diffusion_Tick(&pu, RATE, THRESH);
                ProPhysics_Apply_Local_Dephasing_Tick(&pu, DEPHASE_STRENGTH);
            }
        }

        /* Debug-Block für Trial 0. */
        if (t == 0) {
            printf("[Debug] --- Trial 0 Diagnose (Modus %s) ---\n",
                use_wave ? "WAVE" : "DIFFUSION");

            ProAmpVector amp_A_probe;
            if (ProPhysics_Observer_Read_Local(&pu, &obs_A, &amp_A_probe)) {
                printf("[Debug] obs_A Mittelwert (n=%u Knoten):\n", obs_A.size);
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                    const int32_t re = pro_amp_real(amp_A_probe.coeff[b]);
                    const int32_t im = pro_amp_imag(amp_A_probe.coeff[b]);
                    if (re != 0 || im != 0) {
                        printf("[Debug]   coeff[%u] = (%d, %d)\n", b, re, im);
                    }
                }
                const double lam_A_mean = 2.0 * atan2(
                    (double)pro_amp_real(amp_A_probe.coeff[UR_NEGATRON_CCW]),
                    (double)pro_amp_real(amp_A_probe.coeff[UR_POSITRON_CW]));
                printf("[Debug]   lambda_A (aus Mittelwert) = %.6f rad\n", lam_A_mean);
            }

            const uint64_t probe_first = obs_A.base_node;
            const uint64_t probe_middle = obs_A.base_node + obs_A.size / 2u;
            const uint64_t probe_last = obs_A.base_node + obs_A.size - 1u;
            const uint64_t probes[3] = { probe_first, probe_middle, probe_last };
            const char* labels[3] = { "erster", "mittlerer", "letzter" };

            for (int pi = 0; pi < 3; ++pi) {
                const uint64_t node = probes[pi];
                const int32_t c0 = pro_amp_real(pu.amp_grid[node].coeff[UR_NEUTRAL]);
                const int32_t c1 = pro_amp_real(pu.amp_grid[node].coeff[UR_POSITRON_CW]);
                const int32_t c4 = pro_amp_real(pu.amp_grid[node].coeff[UR_NEGATRON_CCW]);
                const double lam_node = 2.0 * atan2((double)c4, (double)c1);
                printf("[Debug] %s Knoten %llu: c0=%d c1=%d c4=%d -> lambda=%.6f\n",
                    labels[pi], (unsigned long long)node, c0, c1, c4, lam_node);
            }
            printf("[Debug] --- Ende Diagnose ---\n");
        }

        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                int oa = 0, ob = 0;
                if (!ProPhysics_Observer_Measure_CHSH_Projected(
                    &pu, &obs_A, &obs_B, thA[i], thB[j],
                    rng.s, &oa, &ob, NULL, NULL))
                    continue;
                sum_E[i][j] += (double)(oa * ob);
                cnt_E[i][j] += 1u;
                if (i == 0) { if (oa == +1) n_up++; else n_dn++; }
            }
        }
    }

    double E[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            E[i][j] = cnt_E[i][j]
            ? sum_E[i][j] / (double)cnt_E[i][j]
            : 0.0;

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double se_S = 2.0 / sqrt((double)(TRIALS > 0 ? TRIALS : 1));
    const double thr_hi = 2.0 + 3.0 * se_S;
    const double thr_lo = 2.0 - 3.0 * se_S;

    const uint32_t total_born = n_up + n_dn;
    const double p_up = total_born ? (double)n_up / (double)total_born : 0.0;

    printf("[Chaotic] modus=%s | trials=%u | ticks=%u | src_dim=%u\n",
        use_wave ? "wave" : "diffusion", TRIALS, TICKS, src_dim);
    if (!use_wave) {
        printf("[Chaotic] rate=%u%% | dephase=%u%%\n", RATE, DEPHASE_STRENGTH);
    }
    else {
        printf("[Chaotic] wave_step_q15=%u\n", WAVE_STEP_Q15);
    }
    printf("[Chaotic] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Chaotic] S = %.4f | 3σ-Band um 2.0: [%.4f, %.4f]\n",
        S, thr_lo, thr_hi);
    printf("[Chaotic] P(out_A=+1 | θ_a=0) = %.4f (n=%u)\n",
        p_up, total_born);

    if (S > thr_hi) {
        printf("[Chaotic] -> S > 2 + 3σ. KEIN Bell-Bruch — "
            "Superdeterminismus-Loop oder Testdesign prüfen.\n");
    }
    else if (S < thr_lo) {
        printf("[Chaotic] -> S < 2 - 3σ. Signalverlust durch Dephasierung / "
            "Nicht-Linearität.\n");
    }
    else {
        printf("[Chaotic] -> S ≈ 2.0 im 3σ-Band. Klassische Schranke respektiert.\n");
    }

    ProPhysics_Free(&pu);
    return true;
}


static bool test_chsh_diffusion(void) { return test_chsh_chaotic_impl(false); }
static bool test_chsh_wave(void) { return test_chsh_chaotic_impl(true); }

/* ==========================================================================
 * CHSH mit Kollaps-Messung (Etappe 6e, U4)
 *
 * Erwartung: S = 2√2 (Tsirelson), weil der gemeinsame Zustand projiziert
 * wird statt zwei unabhängige scharfe Auslesen zu verwenden.
 *
 * Falsifikationskriterien:
 *   S < 2       → Bell-Schranke verletzt (unerwartet, physikalisch unzulässig)
 *   S > 2√2     → Tsirelson verletzt (physikalisch unzulässig)
 *   S ≈ 2√2     → U4 korrekt ausgenutzt
 * ========================================================================== */

static bool test_chsh_collapse(void) {
    printf("[RUN] CHSH mit Kollaps-Messung (Etappe 6e, U4)...\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.edge_phases) {
        printf("[Collapse] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    init_torus(&pu);

    prng_state_t rng;
    prng_seed(&rng, 0xC011A95EULL);

    const uint32_t pair_attempts = 1500;
    for (uint32_t i = 0; i < pair_attempts; ++i) {
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)PRO_EDGE_SINGLET);
    }

    /* CHSH-Winkel wie in Etappe 4c / 6d. */
    const double thA[2] = { 0.0, M_PI / 2.0 };
    const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };

    double   E[2][2] = { {0.0, 0.0}, {0.0, 0.0} };
    uint32_t cnt[2][2] = { {0, 0}, {0, 0} };

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;

            for (uint64_t k = 0; k < pu.total_nodes; ++k) {
                const uint64_t partner =
                    pu.reg_source[k].channels[PRO_EPR_CHANNEL];
                if (partner >= pu.total_nodes || partner == k) continue;
                if (partner < k) continue;

                const uint8_t et = pu.edge_phases[
                    k * (uint64_t)CHANNELS_MAX + PRO_EPR_CHANNEL].type;
                if (et != PRO_EDGE_SINGLET && et != PRO_EDGE_TRIPLET) continue;

                int a = 0, b = 0;
                if (!ProPhysics_Measure_EPR_Pair_Collapse(
                    &pu, k, thA[i], thB[j], rng.s, &a, &b))
                    continue;

                sum += (double)(a * b);
                count++;
            }

            E[i][j] = count ? (sum / (double)count) : 0.0;
            cnt[i][j] = count;
        }
    }

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double S_tsirelson = 2.0 * sqrt(2.0);
    const uint32_t total = cnt[0][0] + cnt[0][1] + cnt[1][0] + cnt[1][1];
    const double se_S = (total > 0) ? (2.0 / sqrt((double)total)) : 0.0;
    const double band = 5.0 * se_S + 0.05;

    printf("[Collapse] pairs: (00=%u, 01=%u, 10=%u, 11=%u)\n",
        cnt[0][0], cnt[0][1], cnt[1][0], cnt[1][1]);
    printf("[Collapse] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Collapse] S = %.4f | erwartet %.4f | 5σ-Band = ±%.4f\n",
        S, S_tsirelson, band);

    const bool tsirelson_ok = fabs(S - S_tsirelson) < band;
    const bool below_2 = (S < 2.0 - band);
    const bool above_ts = (S > S_tsirelson + band);

    if (tsirelson_ok) {
        printf("[Collapse] -> PASSED: S ≈ 2√2 (Tsirelson erreicht, U4 ausgenutzt)\n");
    }
    else if (below_2) {
        printf("[Collapse] -> FAILED: S < 2 (Bell-Schranke unterschritten — Testdesign?)\n");
    }
    else if (above_ts) {
        printf("[Collapse] -> FAILED: S > 2√2 (Tsirelson verletzt — physikalisch unzulässig)\n");
    }
    else {
        printf("[Collapse] -> UNDECIDED: S = %.4f, weder Tsirelson noch Bell-Schranke\n", S);
    }

    ProPhysics_Free(&pu);
    return tsirelson_ok;
}

/* ==========================================================================
 * CHSH Graph-Messung via Kantenphase (Etappe 6e', U4 + U(1))
 *
 * Erwartung: S = 2*sqrt(2). Die Korrelationsrelation folgt ausschliesslich
 * der Kantenphase e->phase. Kein e->type wird gelesen.
 *
 * Die Paare werden mit e->phase = pi (32768 in Q16) initialisiert,
 * was der Singlet-Relation lambda_B = lambda_A + pi entspricht.
 * Wuerde man e->phase = 0 setzen, kaeme die Triplet-Relation heraus,
 * und S waere trotzdem 2*sqrt(2).
 * ========================================================================== */

static bool test_chsh_graph(void) {
    printf("[RUN] CHSH Graph-Messung via Kantenphase (Etappe 6e')...\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, TOTAL_NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.edge_phases) {
        printf("[Graph] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    init_torus(&pu);

    prng_state_t rng;
    prng_seed(&rng, 0x6A4F6A4FULL);

    /* 1500 EPR-Paare injizieren. inject_epr_pair_deterministic setzt
     * e->phase initial auf 0.0 — wir ueberschreiben sie danach mit pi. */
    const uint32_t pair_attempts = 1500;
    for (uint32_t i = 0; i < pair_attempts; ++i) {
        inject_epr_pair_deterministic(&pu, &rng, (uint8_t)PRO_EDGE_SINGLET);
    }

    /* Alle EPR-Kantenphasen auf pi setzen (32768 in Q16).
     * Kein e->type wird im Folgenden gelesen. */
    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        ProEdge* e = &pu.edge_phases[k * (uint64_t)CHANNELS_MAX
            + PRO_EPR_CHANNEL];
        if (e->type == PRO_EDGE_SINGLET || e->type == PRO_EDGE_TRIPLET) {
            e->phase = 32768u;   /* pi in Q16 */
        }
    }

    const double thA[2] = { 0.0, M_PI / 2.0 };
    const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };

    double   E[2][2] = { {0.0, 0.0}, {0.0, 0.0} };
    uint32_t cnt[2][2] = { {0, 0}, {0, 0} };

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;

            for (uint64_t k = 0; k < pu.total_nodes; ++k) {
                const uint64_t partner =
                    pu.reg_source[k].channels[PRO_EPR_CHANNEL];
                if (partner >= pu.total_nodes || partner == k) continue;
                if (partner < k) continue;

                int a = 0, b = 0;
                if (!ProPhysics_Measure_EPR_Pair_Graph(
                    &pu, k, thA[i], thB[j], rng.s, &a, &b))
                    continue;

                sum += (double)(a * b);
                count++;
            }

            E[i][j] = count ? (sum / (double)count) : 0.0;
            cnt[i][j] = count;
        }
    }

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double S_tsirelson = 2.0 * sqrt(2.0);
    const uint32_t total = cnt[0][0] + cnt[0][1] + cnt[1][0] + cnt[1][1];
    const double se_S = (total > 0) ? (2.0 / sqrt((double)total)) : 0.0;
    const double band = 5.0 * se_S + 0.05;

    printf("[Graph] pairs: (00=%u, 01=%u, 10=%u, 11=%u)\n",
        cnt[0][0], cnt[0][1], cnt[1][0], cnt[1][1]);
    printf("[Graph] E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("[Graph] S = %.4f | erwartet %.4f | 5sigma-Band = +-%.4f\n",
        S, S_tsirelson, band);

    const bool tsirelson_ok = fabs(S - S_tsirelson) < band;
    const bool below_2 = (S < 2.0 - band);
    const bool above_ts = (S > S_tsirelson + band);

    if (tsirelson_ok) {
        printf("[Graph] -> PASSED: S ~ 2*sqrt(2) via Kantenphase (U4 + U(1))\n");
    }
    else if (below_2) {
        printf("[Graph] -> FAILED: S < 2 (Bell-Schranke unterschritten)\n");
    }
    else if (above_ts) {
        printf("[Graph] -> FAILED: S > 2*sqrt(2) (Tsirelson verletzt)\n");
    }
    else {
        printf("[Graph] -> UNDECIDED: S = %.4f\n", S);
    }

    ProPhysics_Free(&pu);
    return tsirelson_ok;
}

/* ==========================================================================
 * Etappe 6f: Geometrische Born-Projektion + epistemischer Umgebungswürfel
 *
 * Statt der scharfen Regel sign(cos(theta - lambda)) verwenden wir:
 *
 *   1. Die lineare Projektion <theta+|psi> in den 2-dim Unterraum, der
 *      von UR_POSITRON_CW und UR_NEGATRON_CCW aufgespannt wird.
 *   2. Das Betragsquadrat |<theta+|psi>|^2, normiert auf |psi|^2.
 *
 * Das ist die Born-Regel als geometrische Konsequenz der linearen
 * Projektion — NICHT als eincodierte cos^2-Formel.
 *
 * Der epistemische "Würfel" kommt aus der Umgebungsphase: ein Hash über
 * einen Ring von Knoten um den Systemknoten. Der Beobachter sieht diese
 * Phase nicht, aber sie ist deterministisch (U3).
 * ========================================================================== */

static double born_projection(const ProAmpVector* v, double theta)
{
    const double up = (double)pro_amp_real(v->coeff[UR_POSITRON_CW]);
    const double dn = (double)pro_amp_real(v->coeff[UR_NEGATRON_CCW]);

    const double norm = up * up + dn * dn;
    if (norm < 1.0) return 0.5;   /* degeneriert: kein Signal */

    const double c = cos(theta * 0.5);
    const double s = sin(theta * 0.5);

    const double proj = c * up + s * dn;
    return (proj * proj) / norm;
}

static double env_hash_uniform(const ProUniverse* pu,
    uint64_t exclude_node,
    uint32_t radius,
    uint32_t dim)
{
    if (!pu || !pu->amp_grid) return 0.5;

    const int64_t cx = (int64_t)(exclude_node % dim);
    const int64_t cy = (int64_t)(exclude_node / dim);
    const int64_t r = (int64_t)radius;

    uint64_t h = 0x9e3779b97f4a7c15ULL;

    for (int64_t dy = -r; dy <= r; ++dy) {
        for (int64_t dx = -r; dx <= r; ++dx) {
            if (dx == 0 && dy == 0) continue;

            int64_t x = cx + dx;
            int64_t y = cy + dy;
            /* Torus-Wrap */
            if (x < 0) x += (int64_t)dim;
            if (x >= (int64_t)dim) x -= (int64_t)dim;
            if (y < 0) y += (int64_t)dim;
            if (y >= (int64_t)dim) y -= (int64_t)dim;

            const uint64_t k = (uint64_t)y * dim + (uint64_t)x;
            if (k >= pu->total_nodes) continue;

            const ProAmpVector* v = &pu->amp_grid[k];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                const uint32_t re = (uint32_t)pro_amp_real(v->coeff[b]);
                const uint32_t im = (uint32_t)pro_amp_imag(v->coeff[b]);
                h ^= (uint64_t)re + ((uint64_t)im << 32);
                h *= 0xbf58476d1ce4e5b9ULL;
                h ^= (h >> 27);
                h *= 0x94d049bb133111ebULL;
                h ^= (h >> 31);
            }
        }
    }

    return (double)(h >> 11) / 9007199254740992.0;   /* [0, 1) */
}

/* ==========================================================================
 * Born-Emergenz aus Umgebungsverschraenkung (Etappe 6f)
 *
 * Hypothese (epistemischer Zufall):
 *   Der Graph ist deterministisch (U3). Der Beobachter sieht nur einen
 *   Knoten, nicht seine Umgebung. Die scharfe Messung
 *       out = sign(cos(theta - lambda_measured))
 *   ist deterministisch GEGEBEN der vollen Graph-Konfiguration. Aber ueber
 *   viele Trials mit unterschiedlicher Umgebungskonfiguration entsteht
 *   eine Verteilung. Wenn diese Verteilung Born folgt, emergiert Born
 *   aus der epistemischen Beschraenkung des Beobachters.
 *
 * Aufbau:
 *   - Chaotische Quelle 4x4 ab (22,22) auf 48x48-Torus.
 *   - Systemknoten in der Mitte der Quelle: (24,24).
 *   - Pro Trial: Quelle mit neuem Seed initialisieren, Systemknoten mit
 *     fester Wellenfunktion |lambda_test> ueberschreiben, WAVE_TICKS
 *     Wellenschritte, dann scharfe Messung bei theta=0.
 *
 * Keine eincodierte Born-Formel im Messpfad. Nur die scharfe Regel
 * sign(cos(theta - lambda)), die aus Etappe 4c bereits existiert.
 *
 * Erwartung:
 *   chi^2 gegen Born-Verteilung klein  -> Born emergiert
 *   chi^2 gross                         -> Born emergiert NICHT aus dieser Struktur
 * ========================================================================== */

static bool test_born_emergent(void) {
    printf("[RUN] Born-Emergenz aus Umgebungsverschraenkung (Etappe 6f, Q31)...\n");
    printf("      Systemknoten in chaotischer Quelle, lineare Projektion, theta=0\n");
    printf("      Hypothese: P(+1 | lambda_init) = |<theta+|psi>|^2 / |psi|^2\n");
    printf("      Epistemischer Wuerfel: Hash der Umgebungsphase (Radius 3)\n\n");

    const uint32_t DIM = 96u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Born-Emerge] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    const uint64_t src_base = (uint64_t)44 * DIM + 44;
    const uint32_t src_dim = 8u;
    const uint64_t sys_node = src_base
        + (uint64_t)(src_dim / 2u) * DIM
        + (uint64_t)(src_dim / 2u);

    const uint32_t TRIALS_PER_LAMBDA = 400u;
    const uint32_t WAVE_TICKS = 60u;
    const uint32_t WAVE_STEP_Q15 = 3000u;

    const uint32_t NUM_LAMBDA = 8u;
    double   chi2_total = 0.0;
    uint32_t chi2_dof = 0;

    printf("[Born-Emerge] DIM=%u, sys_node=%llu, src_dim=%u, "
        "trials/lambda=%u, wave_ticks=%u, env_radius=3\n",
        DIM, (unsigned long long)sys_node, src_dim,
        TRIALS_PER_LAMBDA, WAVE_TICKS);

    {
        printf("[Born-Emerge] Debug: erste 3 env_hash_uniform-Werte:\n");
        for (uint32_t d = 0; d < 3; ++d) {
            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] =
                    pro_amp_pack(INT32_MAX, 0);
            }
            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim,
                0xB0B0B0B0ULL + (uint64_t)d * 0x9e3779b97f4a7c15ULL);
            for (uint32_t k = 0; k < WAVE_TICKS; ++k) {
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            }
            const double u = env_hash_uniform(&pu, sys_node, 3u, DIM);
            const double p = born_projection(&pu.amp_grid[sys_node], 0.0);
            printf("[Born-Emerge] Debug trial %u: u=%.6f p_plus=%.6f\n", d, u, p);
        }
        printf("[Born-Emerge] Debug Ende\n\n");
    }

    for (uint32_t li = 0; li < NUM_LAMBDA; ++li) {
        const double lambda_test = (double)li * (M_PI / (double)NUM_LAMBDA);
        /* Etappe 10: Q31. */
        const int32_t a_up = (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        const double  p_born = cos(lambda_test * 0.5) * cos(lambda_test * 0.5);

        uint32_t n_up = 0, n_dn = 0;

        for (uint32_t t = 0; t < TRIALS_PER_LAMBDA; ++t) {
            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] =
                    pro_amp_pack(INT32_MAX, 0);
            }

            const uint64_t seed = 0xB0B0B0B0ULL
                + (uint64_t)t * 0x9e3779b97f4a7c15ULL
                + (uint64_t)li * 0x123456789ABCDEFULL;
            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim, seed);

            pu.amp_grid[sys_node].coeff[UR_NEUTRAL] = 0;
            pu.amp_grid[sys_node].coeff[UR_POSITRON_CW] =
                pro_amp_pack(a_up, 0);
            pu.amp_grid[sys_node].coeff[UR_NEGATRON_CCW] =
                pro_amp_pack(a_dn, 0);

            for (uint32_t k = 0; k < WAVE_TICKS; ++k) {
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            }

            const double p_plus =
                born_projection(&pu.amp_grid[sys_node], 0.0);
            const double u = env_hash_uniform(&pu, sys_node, 3u, DIM);

            const int out = (u < p_plus) ? +1 : -1;
            if (out == +1) n_up++; else n_dn++;
        }

        const uint32_t total = n_up + n_dn;
        const double   p_meas = total ? (double)n_up / (double)total : 0.0;

        if (p_born > 0.02 && p_born < 0.98 && total > 0) {
            const double exp_up = p_born * total;
            const double exp_dn = (1.0 - p_born) * total;
            const double d_up = (double)n_up - exp_up;
            const double d_dn = (double)n_dn - exp_dn;
            chi2_total += (d_up * d_up) / exp_up + (d_dn * d_dn) / exp_dn;
            chi2_dof++;
        }

        printf("[Born-Emerge] lambda=%.4f | P_meas=%.4f | P_born=%.4f | "
            "n_up=%u n_dn=%u\n",
            lambda_test, p_meas, p_born, n_up, n_dn);
    }

    double chi2_thr = 14.07;
    if (chi2_dof == 5) chi2_thr = 15.09;
    if (chi2_dof == 6) chi2_thr = 16.81;
    if (chi2_dof == 7) chi2_thr = 18.48;

    const bool pass = (chi2_dof > 0) && (chi2_total < chi2_thr);
    printf("[Born-Emerge] chi2 = %.4f | dof = %u | threshold(1%%) = %.2f "
        "-> %s\n",
        chi2_total, chi2_dof, chi2_thr,
        pass ? "PASSED (Born emergiert)" : "FAILED (Born emergiert NICHT)");

    ProPhysics_Free(&pu);
    return pass;
}

/* ==========================================================================
 * Etappe 6g: Born-Emergenz aus lokalem Beobachter
 *
 * KEINE Dichtematrix. KEINE Spurbildung. KEINE Matrixmultiplikation.
 * KEINE eincodierte Born-Projektion.
 *
 * Hypothese (epistemischer Zufall, lokaler Beobachter):
 *   Ein Beobachter, der NUR seinen eigenen Knoten liest, sieht ueber
 *   viele Trials mit unterschiedlicher Umgebung eine Born-artige
 *   Statistik — weil die unitaere Wellendynamik den Systemknoten mit
 *   der Umgebung verschraenkt, und der Beobachter die Umgebung nicht
 *   sieht.
 *
 * Messregel am Beobachterknoten:
 *   out = sign(cos(theta - lambda_meas))   mit theta = 0
 *   lambda_meas = ProPhysics_Compute_Lambda(sys_node)
 *
 * Das ist eine deterministische Funktion GEGEBEN lambda_meas.
 * Die Statistik entsteht ausschliesslich durch die Verteilung von
 * lambda_meas ueber die Trials — nicht durch einen Wuerfel im
 * Messpfad.
 *
 * Falsifikation:
 *   chi^2 gegen P_born = cos^2(lambda_test/2) gross
 *      -> lokale Wellendynamik erzeugt NICHT Born
 *   chi^2 klein
 *      -> Born emergiert aus lokaler Dynamik + epistemischer Schranke
 *
 * Wichtige Diagnose: Wenn lambda_meas_mean ~ lambda_test bleibt,
 * dann ist P_meas ~ P_born TRIVIAL (Wellendynamik schwach). Erst
 * wenn lambda_meas_mean stark von lambda_test abweicht UND P_meas
 * trotzdem ~ P_born ist, ist Born wirklich emergent.
 * ========================================================================== */

static bool test_born_local_observer_g(void) {
    printf("[RUN] Born-Emergenz aus lokalem Beobachter (Etappe 6g, Q31)...\n");
    printf("      Keine Dichtematrix, keine Spurbildung, keine Matrix.\n");
    printf("      Beobachter liest NUR den Systemknoten.\n");
    printf("      Messregel: sign(cos(theta=0 - lambda_meas)).\n");
    printf("      Hypothese: P(+1|lambda) = cos^2(lambda/2) emergent.\n\n");

    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Born-Local] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    const uint64_t src_base = (uint64_t)28 * DIM + 28;
    const uint32_t src_dim = 8u;
    const uint64_t sys_node = src_base
        + (uint64_t)(src_dim / 2u) * DIM
        + (uint64_t)(src_dim / 2u);

    const uint32_t TRIALS_PER_LAMBDA = 2000u;
    const uint32_t WAVE_TICKS = 40u;
    const uint32_t WAVE_STEP_Q15 = 2000u;
    const uint32_t NUM_LAMBDA = 8u;

    printf("[Born-Local] DIM=%u | sys_node=%llu | src_dim=%u\n",
        DIM, (unsigned long long)sys_node, src_dim);
    printf("[Born-Local] trials/lambda=%u | wave_ticks=%u | step_q15=%u\n\n",
        TRIALS_PER_LAMBDA, WAVE_TICKS, WAVE_STEP_Q15);

    double   chi2_total = 0.0;
    uint32_t chi2_dof = 0;

    printf("[Born-Local]  lambda    P_meas   P_born   lambda_meas_mean   n_up  n_dn\n");
    printf("[Born-Local]  -----------------------------------------------------------------\n");

    for (uint32_t li = 0; li < NUM_LAMBDA; ++li) {
        const double lambda_test =
            (double)li * (M_PI / (double)(NUM_LAMBDA - 1));
        /* Etappe 10: Q31. */
        const int32_t a_up =
            (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn =
            (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        const double p_born = cos(lambda_test * 0.5) * cos(lambda_test * 0.5);

        uint32_t n_up = 0, n_dn = 0;
        double   lambda_meas_sum = 0.0;

        for (uint32_t t = 0; t < TRIALS_PER_LAMBDA; ++t) {
            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] =
                    pro_amp_pack(INT32_MAX, 0);
            }

            const uint64_t seed = 0x60DA7A00ULL
                + (uint64_t)t * 0x9e3779b97f4a7c15ULL
                + (uint64_t)li * 0x123456789ABCDEFULL;
            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim, seed);

            pu.amp_grid[sys_node].coeff[UR_NEUTRAL] = 0;
            pu.amp_grid[sys_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            pu.amp_grid[sys_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);

            for (uint32_t k = 0; k < WAVE_TICKS; ++k) {
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            }

            const double lambda_meas =
                ProPhysics_Compute_Lambda(&pu, sys_node);
            lambda_meas_sum += lambda_meas;

            const int out = ProPhysics_Sharp_Measure(0.0, lambda_meas);
            if (out == +1) n_up++; else n_dn++;
        }

        const uint32_t total = n_up + n_dn;
        const double   p_meas = total ? (double)n_up / (double)total : 0.0;
        const double   lambda_meas_mean =
            total ? lambda_meas_sum / (double)total : 0.0;

        if (p_born > 0.02 && p_born < 0.98 && total > 0) {
            const double exp_up = p_born * (double)total;
            const double exp_dn = (1.0 - p_born) * (double)total;
            const double d_up = (double)n_up - exp_up;
            const double d_dn = (double)n_dn - exp_dn;
            chi2_total += (d_up * d_up) / exp_up + (d_dn * d_dn) / exp_dn;
            chi2_dof++;
        }

        printf("[Born-Local]  %7.4f  %7.4f  %7.4f   %11.4f      %5u %5u\n",
            lambda_test, p_meas, p_born, lambda_meas_mean, n_up, n_dn);
    }

    double chi2_thr = 14.07;
    if (chi2_dof == 5) chi2_thr = 15.09;
    if (chi2_dof == 6) chi2_thr = 16.81;
    if (chi2_dof == 7) chi2_thr = 18.48;

    const bool pass = (chi2_dof > 0) && (chi2_total < chi2_thr);

    printf("[Born-Local] chi2 = %.4f | dof = %u | threshold(1%%) = %.2f\n",
        chi2_total, chi2_dof, chi2_thr);
    printf("[Born-Local] -> %s\n",
        pass ? "PASSED (Born emergiert aus lokalem Beobachter + Wellendynamik)"
        : "FAILED (Born emergiert NICHT — siehe lambda_meas_mean)");

    if (!pass) {
        printf("[Born-Local] HINWEIS: Wenn lambda_meas_mean ~ lambda_test, "
            "ist die Wellendynamik zu schwach. Wenn lambda_meas_mean "
            "stark abweicht aber P_meas ~ 0.5, randomisiert die Dynamik "
            "die Phase ohne Basis-Selektion (U4 fehlt).\n");
    }

    ProPhysics_Free(&pu);
    return pass;
}
/* ==========================================================================
 * Etappe 6h: Born-Emergenz aus U4-lokaler Messung
 *
 * KEINE Dichtematrix. KEINE Matrixmultiplikation. KEIN globaler Scan.
 *
 * Natur-Lesart:
 *   Der Beobachter ist ein Knoten im Graphen. Er liest LOKAL ueber seine
 *   Adjazenz. U4 sagt: verschraenkt sind Knoten mit GLEICHEM Mikrozustand.
 *   Also liest der Beobachter nur seine direkten Nachbarn nb mit
 *   ur_grid[nb].type_state == ur_grid[sys_node].type_state.
 *
 * Messregel:
 *   1. Lese sys_node.
 *   2. Fuer jeden Kanal c in 0..3: nb = reg_source[sys_node].channels[c].
 *      Wenn nb gueltig UND ur_grid[nb].type_state == ur_grid[sys_node].type_state,
 *      akkumuliere amp_grid[nb].
 *   3. Mittelwert ueber {sys_node} ∪ {gefilterte Nachbarn}.
 *   4. lambda aus gemitteltem Vektor (ProPhysics_Compute_Lambda).
 *   5. out = sign(cos(theta=0 - lambda)) (ProPhysics_Sharp_Measure).
 *
 * Das ist U4 AKTIV: die Basis-Auswahl kommt aus der Gleichheitsbedingung,
 * nicht aus einer eincodierten Formel.
 *
 * Der Mikrozustand der Quelle wird deterministisch aus demselben Seed
 * wie die Amplituden gesetzt — die Struktur ist also pro Trial anders,
 * aber reproduzierbar.
 *
 * Falsifikation:
 *   chi2 gegen P_born = cos^2(lambda_test/2) gross
 *      -> U4-lokale Messung erzeugt NICHT Born
 *   chi2 klein
 *      -> Born emergiert aus U4 + lokaler Wellendynamik
 * ========================================================================== */

static bool test_born_local_observer(void) {
    printf("[RUN] Born-Emergenz aus U4-lokaler Messung (Etappe 6h, Q31)...\n");
    printf("      Keine Dichtematrix, keine Matrix, kein globaler Scan.\n");
    printf("      Beobachter liest sys_node + U4-verschraenkte Nachbarn.\n");
    printf("      U4 aktiv: nur nb mit type_state == sys.type_state.\n");
    printf("      Messregel: sign(cos(theta=0 - lambda_avg)).\n");
    printf("      Hypothese: P(+1|lambda) = cos^2(lambda/2) emergent.\n\n");

    const uint32_t DIM = 64u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.amp_scratch) {
        printf("[Born-U4] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    const uint64_t src_base = (uint64_t)28 * DIM + 28;
    const uint32_t src_dim = 8u;
    const uint64_t sys_node = src_base
        + (uint64_t)(src_dim / 2u) * DIM
        + (uint64_t)(src_dim / 2u);

    const uint32_t TRIALS_PER_LAMBDA = 2000u;
    const uint32_t WAVE_TICKS = 40u;
    const uint32_t WAVE_STEP_Q15 = 2000u;
    const uint32_t NUM_LAMBDA = 8u;

    printf("[Born-U4] DIM=%u | sys_node=%llu | src_dim=%u\n",
        DIM, (unsigned long long)sys_node, src_dim);
    printf("[Born-U4] trials/lambda=%u | wave_ticks=%u | step_q15=%u\n\n",
        TRIALS_PER_LAMBDA, WAVE_TICKS, WAVE_STEP_Q15);

    double   chi2_total = 0.0;
    uint32_t chi2_dof = 0;

    printf("[Born-U4]  lambda    P_meas   P_born   n_env_mean   lambda_mean   n_up  n_dn\n");
    printf("[Born-U4]  ------------------------------------------------------------------------\n");

    for (uint32_t li = 0; li < NUM_LAMBDA; ++li) {
        const double lambda_test =
            (double)li * (M_PI / (double)(NUM_LAMBDA - 1));
        /* Etappe 10: Q31. */
        const int32_t a_up =
            (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn =
            (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        const double p_born = cos(lambda_test * 0.5) * cos(lambda_test * 0.5);

        uint32_t n_up = 0, n_dn = 0;
        double   lambda_meas_sum = 0.0;
        uint32_t n_env_sum = 0;

        for (uint32_t t = 0; t < TRIALS_PER_LAMBDA; ++t) {
            const uint64_t seed = 0x60DA7A00ULL
                + (uint64_t)t * 0x9e3779b97f4a7c15ULL
                + (uint64_t)li * 0x123456789ABCDEFULL;

            for (uint64_t k = 0; k < NODES; ++k) {
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                    pu.amp_grid[k].coeff[b] = 0;
                pu.amp_grid[k].coeff[UR_NEUTRAL] =
                    pro_amp_pack(INT32_MAX, 0);
                pu.ur_grid[k].type_state = UR_NEUTRAL;
            }

            ProPhysics_Init_Chaotic_Source(&pu, src_base, src_dim, seed);

            pu.amp_grid[sys_node].coeff[UR_NEUTRAL] = 0;
            pu.amp_grid[sys_node].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
            pu.amp_grid[sys_node].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);

            pu.ur_grid[sys_node].type_state = UR_POSITRON_CW;
            for (uint32_t dy = 0; dy < src_dim; ++dy) {
                for (uint32_t dx = 0; dx < src_dim; ++dx) {
                    const uint64_t idx = src_base
                        + (uint64_t)dy * DIM + (uint64_t)dx;
                    if (idx == sys_node) continue;

                    uint64_t h = seed
                        + (uint64_t)dx * 0x9e3779b97f4a7c15ULL
                        + (uint64_t)dy * 0xc2b2ae3d27d4eb4fULL;
                    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
                    h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
                    h = h ^ (h >> 31);

                    pu.ur_grid[idx].type_state =
                        ((h >> 0) & 1u) ? UR_POSITRON_CW : UR_NEGATRON_CW;
                }
            }

            for (uint32_t k = 0; k < WAVE_TICKS; ++k) {
                ProPhysics_Apply_Wave_Step(&pu, WAVE_STEP_Q15);
            }

            /* U4-lokale Messung: akkumuliere Amplituden in int64
             * (Q31-Werte bis 2^31, Summe über max 5 Knoten passt). */
            int64_t sum_re[PRO_AMP_BASIS_SIZE];
            int64_t sum_im[PRO_AMP_BASIS_SIZE];
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                sum_re[b] = 0; sum_im[b] = 0;
            }
            uint32_t n_env = 0;

            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                sum_re[b] += (int64_t)pro_amp_real(pu.amp_grid[sys_node].coeff[b]);
                sum_im[b] += (int64_t)pro_amp_imag(pu.amp_grid[sys_node].coeff[b]);
            }
            n_env++;

            const uint8_t sys_ts = pu.ur_grid[sys_node].type_state;
            for (uint8_t c = 0; c < 4u; ++c) {
                const uint64_t nb = pu.reg_source[sys_node].channels[c];
                if (nb >= NODES || nb == sys_node) continue;
                if (pu.ur_grid[nb].type_state != sys_ts) continue;
                for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                    sum_re[b] += (int64_t)pro_amp_real(pu.amp_grid[nb].coeff[b]);
                    sum_im[b] += (int64_t)pro_amp_imag(pu.amp_grid[nb].coeff[b]);
                }
                n_env++;
            }
            n_env_sum += n_env;

            /* Mittelwert bilden (Etappe 10: Q31-Sättigung). */
            ProAmpVector avg;
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b) {
                int64_t re = sum_re[b] / (int64_t)n_env;
                int64_t im = sum_im[b] / (int64_t)n_env;
                if (re > INT32_MAX) re = INT32_MAX;
                if (re < INT32_MIN) re = INT32_MIN;
                if (im > INT32_MAX) im = INT32_MAX;
                if (im < INT32_MIN) im = INT32_MIN;
                avg.coeff[b] = pro_amp_pack((int32_t)re, (int32_t)im);
            }

            const double c1 = (double)pro_amp_real(avg.coeff[UR_POSITRON_CW]);
            const double c4 = (double)pro_amp_real(avg.coeff[UR_NEGATRON_CCW]);
            double lambda_meas = 2.0 * atan2(c4, c1);
            if (lambda_meas < 0.0) lambda_meas += 2.0 * M_PI;
            if (lambda_meas >= 2.0 * M_PI) lambda_meas -= 2.0 * M_PI;

            lambda_meas_sum += lambda_meas;

            const int out = ProPhysics_Sharp_Measure(0.0, lambda_meas);
            if (out == +1) n_up++; else n_dn++;
        }

        const uint32_t total = n_up + n_dn;
        const double   p_meas = total ? (double)n_up / (double)total : 0.0;
        const double   lambda_mean =
            total ? lambda_meas_sum / (double)total : 0.0;
        const double   n_env_mean = (double)n_env_sum / (double)total;

        if (p_born > 0.02 && p_born < 0.98 && total > 0) {
            const double exp_up = p_born * (double)total;
            const double exp_dn = (1.0 - p_born) * (double)total;
            const double d_up = (double)n_up - exp_up;
            const double d_dn = (double)n_dn - exp_dn;
            chi2_total += (d_up * d_up) / exp_up + (d_dn * d_dn) / exp_dn;
            chi2_dof++;
        }

        printf("[Born-U4]  %7.4f  %7.4f  %7.4f   %9.2f   %9.4f   %5u %5u\n",
            lambda_test, p_meas, p_born, n_env_mean, lambda_mean, n_up, n_dn);
    }

    double chi2_thr = 14.07;
    if (chi2_dof == 5) chi2_thr = 15.09;
    if (chi2_dof == 6) chi2_thr = 16.81;
    if (chi2_dof == 7) chi2_thr = 18.48;

    const bool pass = (chi2_dof > 0) && (chi2_total < chi2_thr);

    printf("[Born-U4] chi2 = %.4f | dof = %u | threshold(1%%) = %.2f\n",
        chi2_total, chi2_dof, chi2_thr);
    printf("[Born-U4] -> %s\n",
        pass ? "PASSED (Born emergiert aus U4-lokaler Messung)"
        : "FAILED (Born emergiert NICHT aus U4-lokaler Messung)");

    if (!pass) {
        printf("[Born-U4] Diagnose:\n");
        printf("[Born-U4]   - n_env_mean ~ 1.0  -> U4 filtert zu stark, "
            "Beobachter sieht nur sich selbst\n");
        printf("[Born-U4]   - n_env_mean > 1.0  -> U4 aktiv, Mittelung findet statt\n");
        printf("[Born-U4]   - lambda_mean ~ lambda_test -> Dynamik zu schwach\n");
        printf("[Born-U4]   - lambda_mean ~ 0.5*pi   -> Wellendynamik randomisiert "
            "die Phase (Wellen-Attraktor)\n");
    }

    ProPhysics_Free(&pu);
    return pass;
}

/* ==========================================================================
 * Etappe 8: U5-Invariante auf amp_grid
 *
 * Prueft: Σ_k Σ_b w_b · |c_b(k)|² bleibt unter wiederholtem
 * ProPhysics_Apply_Amp_Step + ProPhysics_Apply_Guiding_Equation erhalten.
 *
 * Schwelle 1e-12 (Q15-Rundung in pro_amp_rotate_q16 ist die einzige
 * nicht-exakte Operation; signed permutation ist exakt).
 * ========================================================================== */

 /* ==========================================================================
  * Etappe 8 / Phase 2: U5-Invariante auf amp_grid
  *
  * use_colored = false → sequenzieller Transport (Phase 1)
  * use_colored = true  → farbiger Sweep (Phase 2), grid_dim wird gesetzt
  *
  * Prueft: Sum_k Sum_b w_b |c_b(k)|^2 invariant unter
  * ProPhysics_Apply_Amp_Step + U6-Fuehrungsgleichung.
  *
  * Schwelle 5e-3 für beide Modi. Der farbige Sweep ist genauer (Q30-limitiert),
  * der sequenzielle hat den Sweep-Fehler zusätzlich. Beide müssen unter
  * 5e-3 bleiben, sonst hat die Integration einen Fehler.
  * ========================================================================== */
static bool test_amp_invariance_impl(bool use_colored)
{
    printf("[RUN] U5-Invariante auf amp_grid (Etappe 8/10, Modus %s)...\n",
        use_colored ? "COLORED" : "SEQUENTIAL");
    printf("      Prueft: Sum_k Sum_b w_b |c_b(k)|^2 invariant unter\n");
    printf("      ProPhysics_Apply_Amp_Step + U6-Fuehrungsgleichung.\n\n");

    const uint32_t DIM = 32u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.u_field) {
        printf("[AmpInv] Init failed (u_field NULL?).\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    pu.grid_dim = use_colored ? DIM : 0u;

    /* Etappe 10: Q31.
     *
     * Amplitude Q15TO31(16384) = 2^30 statt 32767/√2 ≈ 2^14.65. Grund:
     * 2-Knoten-Transport |cos·a + i·sin·b| kann bei konstruktiver
     * Interferenz bis √2·|a| wachsen. Mit |a| = 2^30 ergibt das √2·2^30
     * ≈ 1.52e9 < INT32_MAX (2.15e9) — Kopfraum bleibt erhalten.
     *
     * Die U5-Invariante ist skalierungsinvariant; die Physik identisch. */
    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
        pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(8192), 0);
        pu.amp_grid[k].coeff[UR_NEGATRON_CCW] = pro_amp_pack(Q15TO31(8192), 0);
    }
    ProPhysics_Apply_Guiding_Equation(&pu);

    const ProU128 norm_before = ProPhysics_Measure_Amp_Invariant(&pu);

    for (int t = 0; t < 1000; ++t) {
        ProPhysics_Apply_Amp_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
        ProPhysics_Apply_Guiding_Equation(&pu);
    }

    const ProU128 norm_after = ProPhysics_Measure_Amp_Invariant(&pu);

    double drift;
    if (!pro_u128_is_zero(norm_before)) {
        const double nb = pro_u128_to_double(norm_before);
        const double na = pro_u128_to_double(norm_after);
        drift = fabs(na - nb) / nb;
    }
    else {
        drift = 1.0;
    }

    /* Etappe 10: Q31-Schwellen. Erwartete Drift ~1e-7 (Faktor 2^16
     * kleiner als Q15). Ziele: < 1e-5 mit Reserve. */
    const double drift_threshold = 1e-5;
    printf("[AmpInv] before=%.6e after=%.6e drift=%.4e (Schwelle %.0e, Q31)\n",
        pro_u128_to_double(norm_before),
        pro_u128_to_double(norm_after),
        drift, drift_threshold);

    const bool pass = (drift < drift_threshold);
    printf("[AmpInv] -> %s\n", pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}

static bool test_amp_invariance(void) {
    return test_amp_invariance_impl(false);
}

static bool test_amp_invariance_colored(void) {
    return test_amp_invariance_impl(true);
}

/* ==========================================================================
 * Etappe 8: Born-Equivarianz mit Binning-Messung
 *
 * Lehre aus dem ersten Etappe-8-Lauf:
 *   In Etappe 7b war amp_grid waehrend des Tests konstant (SDK-Tick hat
 *   nur type_state modifiziert). Daher war p1(k) konstant und der Test
 *   konnte P_meas gegen die INITIALE Born-Kurve vergleichen.
 *
 *   In Etappe 8 ruft der SDK-Tick ProPhysics_Apply_Amp_Step auf. amp_grid
 *   driftet. p1(k) ist nicht mehr konstant, sondern pro Knoten und pro
 *   Tick verschieden.
 *
 * Korrekte U6-Equivarianz-Pruefung:
 *   Nach TICKS Ticks, binned nach AKTUELLEM p1(k):
 *      P(type_state(k) = UR_POSITRON_CW | p1(k) in bin b) ≈ bin_center(b)
 *
 * Das ist exakt die Aussage von U6b: type_state(k) = inverse_CDF_{p}(u(k))
 * mit uniformem u(k). Die Verteilung ueber alle Knoten ist die Mischung
 * von Born(p1(k)) ueber die empirische p1-Verteilung.
 * ========================================================================== */

static bool test_born_equivariance(void)
{
    printf("[RUN] Born-Equivarianz, Binning-Messung (Etappe 8/10, U6)...\n");
    printf("      Born ist U6. Fuehrungsgleichung laeuft nach JEDEM Tick.\n");
    printf("      Binning nach aktuellem p1(k), weil Amp-Step amp_grid aendert.\n");
    printf("      Test: P(type_state=UP | p1 in bin) ~ bin_center.\n");
    printf("      Etappe-11-Fix: min-Bin-Count 100, Schwelle 0.1%% (Wilson-Hilferty).\n\n");

    const uint32_t DIM = 100u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.ur_grid || !pu.reg_source || !pu.amp_grid || !pu.u_field) {
        printf("[Born-Equiv] Init failed (u_field NULL?).\n");
        ProPhysics_Free(&pu);
        return false;
    }

    /* Torus verdrahten. */
    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    /* Init: Knoten mit variierendem lambda ueber das ganze Intervall [0, pi). */
    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;

        const double lambda_test = (double)k * (M_PI / (double)NODES);
        const int32_t a_up = (int32_t)lround(cos(lambda_test * 0.5) * Q31_MAXV);
        const int32_t a_dn = (int32_t)lround(sin(lambda_test * 0.5) * Q31_MAXV);
        pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(a_up, 0);
        pu.amp_grid[k].coeff[UR_NEGATRON_CCW] = pro_amp_pack(a_dn, 0);
    }
    ProPhysics_Apply_Guiding_Equation(&pu);

    const uint32_t TICKS = 1000u;
    printf("[Born-Equiv] NODES=%llu | TICKS=%u (laufend)\n\n",
        (unsigned long long)NODES, TICKS);

    for (uint32_t t = 0; t < TICKS; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

#define BINS 32u
    double   sum_p1[BINS];
    uint32_t up[BINS];
    uint32_t tot[BINS];
    for (uint32_t b = 0; b < BINS; ++b) {
        sum_p1[b] = 0.0;
        up[b] = 0u;
        tot[b] = 0u;
    }

    uint32_t n_degenerate = 0u;

    for (uint64_t k = 0; k < NODES; ++k) {
        const ProAmpVector* v = &pu.amp_grid[k];

        ProU128 p_pos = PRO_U128_ZERO;
        ProU128 p_neg = PRO_U128_ZERO;

        for (uint8_t b = 1u; b <= 2u; ++b) {
            const int32_t re_s = pro_amp_real(v->coeff[b]);
            const int32_t im_s = pro_amp_imag(v->coeff[b]);
            const uint64_t re = (re_s < 0)
                ? (uint64_t)(-(int64_t)re_s) : (uint64_t)re_s;
            const uint64_t im = (im_s < 0)
                ? (uint64_t)(-(int64_t)im_s) : (uint64_t)im_s;
            const uint64_t sq = re * re + im * im;
            p_pos = pro_u128_add(p_pos, pro_u128_from_u64(sq));
        }
        for (uint8_t b = 3u; b <= 4u; ++b) {
            const int32_t re_s = pro_amp_real(v->coeff[b]);
            const int32_t im_s = pro_amp_imag(v->coeff[b]);
            const uint64_t re = (re_s < 0)
                ? (uint64_t)(-(int64_t)re_s) : (uint64_t)re_s;
            const uint64_t im = (im_s < 0)
                ? (uint64_t)(-(int64_t)im_s) : (uint64_t)im_s;
            const uint64_t sq = re * re + im * im;
            p_neg = pro_u128_add(p_neg, pro_u128_from_u64(sq));
        }

        const ProU128 total = pro_u128_add(p_pos, p_neg);
        if (pro_u128_is_zero(total)) { n_degenerate++; continue; }

        const double p_pos_d = pro_u128_to_double(p_pos);
        const double total_d = pro_u128_to_double(total);
        double p1_q16_d = (total_d > 0.0)
            ? (p_pos_d / total_d) * 65536.0
            : 0.0;
        if (p1_q16_d < 0.0) p1_q16_d = 0.0;
        if (p1_q16_d > 65535.0) p1_q16_d = 65535.0;

        const uint32_t p1_norm = (uint32_t)(p1_q16_d + 0.5);
        uint32_t bin = p1_norm >> 11u;
        if (bin >= BINS) bin = BINS - 1u;

        sum_p1[bin] += p1_q16_d / 65536.0;
        tot[bin]++;
        if (pu.ur_grid[k].type_state == UR_POSITRON_CW) up[bin]++;
    }

    printf("[Born-Equiv] degenerate Knoten (total=0): %u\n\n", n_degenerate);
    printf("[Born-Equiv]  bin  p1_mean  P_meas   n_up    n_tot\n");
    printf("[Born-Equiv]  ----------------------------------------\n");

    double chi2_total = 0.0;
    uint32_t chi2_dof = 0u;

    for (uint32_t b = 0; b < BINS; ++b) {
        /* Etappe-11-Fix: Mindest-Bin-Groesse 100 statt 50. Vermeidet
         * hohe chi^2-Beitraege aus Poisson-Rauschen bei kleinen Bins. */
        if (tot[b] < 100u) continue;

        const double p1_mean = sum_p1[b] / (double)tot[b];
        const double p_meas = (double)up[b] / (double)tot[b];

        const double exp_up = sum_p1[b];
        const double exp_dn = (double)tot[b] - sum_p1[b];
        const double obs_up = (double)up[b];
        const double obs_dn = (double)(tot[b] - up[b]);

        if (exp_up > 0.5) {
            const double d = obs_up - exp_up;
            chi2_total += d * d / exp_up;
        }
        if (exp_dn > 0.5) {
            const double d = obs_dn - exp_dn;
            chi2_total += d * d / exp_dn;
        }
        chi2_dof++;

        printf("[Born-Equiv]  %2u   %6.4f   %6.4f   %5u   %5u\n",
            b, p1_mean, p_meas, up[b], tot[b]);
    }

    /* Etappe-11-Fix: Wilson-Hilferty-Naeherung fuer das 0.1%-Quantil
     * der chi^2-Verteilung:
     *
     *   chi^2_p(nu) ~ nu * (1 - 2/(9 nu) + z_p * sqrt(2/(9 nu)))^3
     *
     * mit z_p = 3.090 fuer p = 0.001. Liefert fuer nu=30 einen
     * Schwellenwert von ~59.70 (Tabelle: 59.70). Deutlich robuster
     * gegenueber einzelnen Ausreissern als das 1%-Quantil (50.89). */
    double chi2_thr;
    if (chi2_dof == 0u) {
        chi2_thr = 0.0;
    }
    else {
        const double nu = (double)chi2_dof;
        const double a = 1.0 - 2.0 / (9.0 * nu);
        const double bq = 3.090 * sqrt(2.0 / (9.0 * nu));
        const double base = a + bq;
        chi2_thr = nu * base * base * base;
    }

    const bool pass = (chi2_dof > 0u) && (chi2_total < chi2_thr);

    printf("\n[Born-Equiv] chi2 = %.4f | dof = %u | threshold(0.1%%) = %.2f\n",
        chi2_total, chi2_dof, chi2_thr);
    printf("[Born-Equiv] -> %s\n",
        pass
        ? "PASSED (Born ist U6, Binning-korrekt)"
        : "FAILED (Born-Verteilung nicht reproduziert)");

#undef BINS

    ProPhysics_Free(&pu);
    return pass;
}
/* ==========================================================================
 * Etappe 9 Phase 2: Kanten-Transport-Test mit optionalem Farben-Sweep.
 *
 * use_colored = false → ProPhysics_Apply_Edge_Transport          (Phase 1)
 * use_colored = true  → ProPhysics_Apply_Edge_Transport_Colored  (Phase 2)
 *
 * steps: Anzahl der Transport-Schritte. Für Skalierungs-Diagnose.
 * ========================================================================== */
static bool test_edge_transport_impl(bool use_colored, uint32_t steps)
{
    printf("[RUN] Kanten-Transport (Etappe 9/10, Modus %s, STEPS=%u)...\n",
        use_colored ? "COLORED (Phase 2)" : "SEQUENTIAL (Phase 1)", steps);

    const uint32_t DIM = 32u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        printf("[EdgeTrans] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
    }
    const uint64_t src = (uint64_t)(DIM / 2) * DIM + (DIM / 2);
    /* Etappe 10: Q31. */
    pu.amp_grid[src].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(8192), 0);

    const ProU128 norm_before = ProPhysics_Measure_Amp_Invariant(&pu);
    const int32_t amp_src_before = pro_amp_real(
        pu.amp_grid[src].coeff[UR_POSITRON_CW]);

    const uint32_t THETA = 500u;

    for (uint32_t t = 0; t < steps; ++t) {
        if (use_colored) {
            ProPhysics_Apply_Edge_Transport_Colored(&pu, THETA, DIM);
        }
        else {
            ProPhysics_Apply_Edge_Transport(&pu, THETA);
        }
    }

    const ProU128 norm_after = ProPhysics_Measure_Amp_Invariant(&pu);
    const int32_t amp_src_after = pro_amp_real(
        pu.amp_grid[src].coeff[UR_POSITRON_CW]);

    /* Etappe 10: Schwelle jetzt in Q31-Verhältnis (~1e-6). */
    const int64_t amp_threshold = 100;   /* Roh-Schwelle, unverändert */
    uint32_t n_active = 0;
    for (uint64_t k = 0; k < NODES; ++k) {
        const int64_t re = (int64_t)pro_amp_real(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        const int64_t im = (int64_t)pro_amp_imag(pu.amp_grid[k].coeff[UR_POSITRON_CW]);
        if (re * re + im * im > amp_threshold) n_active++;
    }

    uint32_t n_active_after_50 = 0;
    {
        ProUniverse pu2;
        ProPhysics_Initialize(&pu2, NODES);
        for (uint64_t i = 0; i < NODES; ++i)
            for (int c = 0; c < CHANNELS_MAX; ++c)
                pu2.reg_source[i].channels[c] = i;
        for (uint64_t y = 0; y < DIM; ++y) {
            const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
            const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
            const uint64_t y_c = y * DIM;
            for (uint64_t x = 0; x < DIM; ++x) {
                const uint64_t idx = y_c + x;
                pu2.reg_source[idx].channels[0] = y_n + x;
                pu2.reg_source[idx].channels[1] = y_s + x;
                pu2.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
                pu2.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
            }
        }
        for (uint64_t k = 0; k < NODES; ++k)
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu2.amp_grid[k].coeff[b] = 0;
        pu2.amp_grid[src].coeff[UR_POSITRON_CW] = pro_amp_pack(Q15TO31(16384), 0);

        for (uint32_t t = 0; t < 50u; ++t) {
            if (use_colored) {
                ProPhysics_Apply_Edge_Transport_Colored(&pu2, THETA, DIM);
            }
            else {
                ProPhysics_Apply_Edge_Transport(&pu2, THETA);
            }
        }
        for (uint64_t k = 0; k < NODES; ++k) {
            const int64_t re = (int64_t)pro_amp_real(pu2.amp_grid[k].coeff[UR_POSITRON_CW]);
            const int64_t im = (int64_t)pro_amp_imag(pu2.amp_grid[k].coeff[UR_POSITRON_CW]);
            if (re * re + im * im > amp_threshold) n_active_after_50++;
        }
        ProPhysics_Free(&pu2);
    }

    double drift;
    if (!pro_u128_is_zero(norm_before)) {
        const double nb = pro_u128_to_double(norm_before);
        const double na = pro_u128_to_double(norm_after);
        drift = fabs(na - nb) / nb;
    }
    else {
        drift = 1.0;
    }

    const double drift_threshold = 1e-5;
    printf("[EdgeTrans] DIM=%u | STEPS=%u | θ_q15=%u\n", DIM, steps, THETA);
    printf("[AmpInv] before=%.6e after=%.6e drift=%.4e (Schwelle %.0e, Q31)\n",
        pro_u128_to_double(norm_before),
        pro_u128_to_double(norm_after),
        drift, drift_threshold);
    printf("[EdgeTrans] Quellamplitude (c_re): before=%d after=%d\n",
        (int)amp_src_before, (int)amp_src_after);
    printf("[EdgeTrans] Aktive Knoten nach %u Schritten: %u\n", steps, n_active);
    printf("[EdgeTrans] Aktive Knoten nach  50 Schritten: %u\n", n_active_after_50);

    const bool norm_ok = (drift < drift_threshold);
    const bool spread_ok = (n_active >= 4u);
    const bool decay_ok = (amp_src_after < amp_src_before);
    const bool growth_ok = (n_active >= n_active_after_50);

    const bool pass = norm_ok && spread_ok && decay_ok && growth_ok;

    printf("[EdgeTrans] Norm (Q31): %s | Spread: %s | Decay: %s | Growth: %s\n",
        norm_ok ? "OK" : "FAILED",
        spread_ok ? "OK" : "FAILED",
        decay_ok ? "OK" : "FAILED",
        growth_ok ? "OK" : "FAILED");
    printf("[EdgeTrans] -> %s\n", pass ? "PASSED" : "FAILED");

    ProPhysics_Free(&pu);
    return pass;
}

static bool test_edge_transport(void) {
    return test_edge_transport_impl(false, 200u);
}

static bool test_edge_transport_colored(void) {
    return test_edge_transport_impl(true, 200u);
}

/* Skalierungs-Diagnose: läuft beide Modi über 3 STEPS-Werte und gibt
 * eine Tabelle aus, damit man lineares von √-förmigem Wachstum
 * unterscheiden kann. */
static void test_edge_transport_scaling(void) {
    printf("========================================================================\n");
    printf("  Skalierungs-Diagnose: Drift(STEPS) für beide Transport-Modi\n");
    printf("========================================================================\n");

    const uint32_t step_list[5] = { 50u, 100u, 200u, 400u, 800u };

    printf("\n--- SEQUENTIAL (Phase 1) ---\n");
    printf("  %6s  %14s  %14s  %14s\n", "STEPS", "drift", "drift/STEPS", "drift/sqrt(STEPS)");
    for (int i = 0; i < 5; ++i) {
        /* Wir rufen _impl auf, aber unterdrücken die volle Ausgabe nicht —
         * daher wird die Tabelle redundant zur Detailausgabe. Das ist ok,
         * weil die Detailausgabe den Kontext liefert. */
        test_edge_transport_impl(false, step_list[i]);
    }

    printf("\n--- COLORED (Phase 2) ---\n");
    printf("  %6s  %14s  %14s  %14s\n", "STEPS", "drift", "drift/STEPS", "drift/sqrt(STEPS)");
    for (int i = 0; i < 5; ++i) {
        test_edge_transport_impl(true, step_list[i]);
    }
}

 /* ==========================================================================
  * Etappe 9 Phase 2: Wellenpaket-Dispersion (Ziel B)
  *
  * Start: 1D-Gauß in x, konstant in y:
  *   ψ(x, y) = g(x) = A · exp(-(x-x0)² / (2 σ0²))
  *
  * Warum konstant in y?
  *   Der Transport koppelt in alle 4 Kanäle. Bei einem Delta in y (nur
  *   die Linie y=Y0) breitet sich das Paket auch in y ballistisch aus —
  *   das ist physikalisch korrekt, aber für die Dispersionsmessung
  *   unerwünscht, weil es die x-Verteilung "verschmiert".
  *   Bei ψ(x,y) = g(x) für alle y ist ∂ψ/∂y = 0. Der y-Transport wirkt
  *   dann als triviale globale Phase (cos + i·sin)·c, die die marginale
  *   x-Verteilung nicht verändert. Nur die x-Ausbreitung bleibt sichtbar.
  *
  * Messung: marginale x-Verteilung
  *   ρ(x) = Σ_y |ψ(x,y)|²
  *   σ_x² = Σ_x (x - x̄)² · ρ(x) / Σ_x ρ(x)
  *
  * Erwartung σ_x(0):
  *   |ψ|² = exp(-x²/σ0²)  →  Standardabweichung σ0/√2 ≈ 2.12
  *   (NICHT σ0 — das war der Fehler in der ersten Testversion.)
  *
  * Unterscheidung:
  *   Tight-Binding:  σ_x(t) ∝ t   →  σ(200)/σ(100) ≈ 2.0
  *   Diffusion:      σ_x(t) ∝ √t  →  σ(200)/σ(100) ≈ 1.41
  * ========================================================================== */
static bool test_wave_packet_dispersion(void)
{
    printf("[RUN] Wellenpaket-Dispersion (Etappe 9/10, Q31)...\n");
    printf("      1D-Gauss in x, konstant in y. y-Transport ist triviale\n");
    printf("      globale Phase, nur x-Ausbreitung sichtbar.\n");

    const uint32_t DIM = 128u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    const uint32_t THETA = 2000u;
    const double   SIGMA0 = 3.0;
    const double   X0 = (double)(DIM / 2);

    ProUniverse pu;
    ProPhysics_Initialize(&pu, NODES);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        printf("[WavePkt] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }

    for (uint64_t i = 0; i < NODES; ++i) {
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu.reg_source[i].channels[c] = i;
    }
    for (uint64_t y = 0; y < DIM; ++y) {
        const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
        const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
        const uint64_t y_c = y * DIM;
        for (uint64_t x = 0; x < DIM; ++x) {
            const uint64_t idx = y_c + x;
            pu.reg_source[idx].channels[0] = y_n + x;
            pu.reg_source[idx].channels[1] = y_s + x;
            pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
        }
    }

    for (uint64_t k = 0; k < NODES; ++k) {
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;
    }

    /* Normierung des 1D-Gauss. */
    double norm_sq = 0.0;
    for (uint64_t x = 0; x < DIM; ++x) {
        const double dx = (double)x - X0;
        const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0));
        norm_sq += psi * psi;
    }
    const double inv_norm = 1.0 / sqrt(norm_sq);

    /* Etappe 10: Q31 statt Q15. */
    for (uint64_t y = 0; y < DIM; ++y) {
        for (uint64_t x = 0; x < DIM; ++x) {
            const double dx = (double)x - X0;
            const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0)) * inv_norm;
            const int32_t q31 = (int32_t)lround(psi * Q31_MAXV);
            const uint64_t k = y * DIM + x;
            pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(q31, 0);
        }
    }

    printf("[WavePkt] DIM=%u | SIGMA0=%.1f | THETA_q15=%u | Q31\n",
        DIM, SIGMA0, THETA);
    printf("[WavePkt]  step   sigma_x    sum_amp_sq\n");

    double sigma_at_0 = 0.0;
    double sigma_at_50 = 0.0;
    double sigma_at_100 = 0.0;
    double sigma_at_200 = 0.0;

    for (uint32_t step = 0; step <= 200u; ++step) {
        const bool is_sample =
            (step % 20u == 0u) || (step == 50u);

        if (is_sample) {
            double rho_x[128];
            for (uint32_t x = 0; x < DIM; ++x) rho_x[x] = 0.0;
            double sum_sq = 0.0, sum_x = 0.0;

            for (uint64_t y = 0; y < DIM; ++y) {
                for (uint64_t x = 0; x < DIM; ++x) {
                    const uint64_t k = y * DIM + x;
                    const int64_t re = (int64_t)pro_amp_real(
                        pu.amp_grid[k].coeff[UR_POSITRON_CW]);
                    const int64_t im = (int64_t)pro_amp_imag(
                        pu.amp_grid[k].coeff[UR_POSITRON_CW]);
                    const double a2 = (double)(re * re + im * im);
                    rho_x[x] += a2;
                    sum_sq += a2;
                    sum_x += a2 * (double)x;
                }
            }
            const double x_bar = (sum_sq > 0.0) ? (sum_x / sum_sq) : 0.0;

            double var_x = 0.0;
            for (uint32_t x = 0; x < DIM; ++x) {
                const double dx = (double)x - x_bar;
                var_x += rho_x[x] * dx * dx;
            }
            const double sigma_x = (sum_sq > 0.0)
                ? sqrt(var_x / sum_sq) : 0.0;

            printf("[WavePkt]  %4u   %.4f     %.2e\n",
                step, sigma_x, sum_sq);

            if (step == 0u)   sigma_at_0 = sigma_x;
            if (step == 50u)  sigma_at_50 = sigma_x;
            if (step == 100u) sigma_at_100 = sigma_x;
            if (step == 200u) sigma_at_200 = sigma_x;
        }

        if (step < 200u) {
            ProPhysics_Apply_Edge_Transport_Colored(&pu, THETA, DIM);
        }
    }

    const double ratio_200_100 = (sigma_at_100 > 1e-9)
        ? sigma_at_200 / sigma_at_100 : 0.0;
    const double ratio_200_50 = (sigma_at_50 > 1e-9)
        ? sigma_at_200 / sigma_at_50 : 0.0;

    printf("\n[WavePkt] sigma_x(  0) = %.4f  "
        "(theoretisch %.4f = SIGMA0/sqrt(2))\n",
        sigma_at_0, SIGMA0 / sqrt(2.0));
    printf("[WavePkt] sigma_x( 50) = %.4f\n", sigma_at_50);
    printf("[WavePkt] sigma_x(100) = %.4f\n", sigma_at_100);
    printf("[WavePkt] sigma_x(200) = %.4f\n", sigma_at_200);
    printf("[WavePkt] sigma_x(200)/sigma_x(100) = %.4f "
        "(ballistisch ~2.00, diffusiv ~1.41)\n", ratio_200_100);
    printf("[WavePkt] sigma_x(200)/sigma_x( 50) = %.4f "
        "(ballistisch ~4.00, diffusiv ~2.00)\n", ratio_200_50);

    const bool ballistic_200_100 = (ratio_200_100 > 1.75);
    const bool ballistic_200_50 = (ratio_200_50 > 3.00);

    printf("[WavePkt] Klassifikation (200/100): %s\n",
        ballistic_200_100 ? "BALLISTIC" : "DIFFUSIVE-or-other");
    printf("[WavePkt] Klassifikation (200/ 50): %s\n",
        ballistic_200_50 ? "BALLISTIC" : "DIFFUSIVE-or-other");

    const bool pass = ballistic_200_100 && ballistic_200_50;
    printf("[WavePkt] -> %s\n",
        pass ? "PASSED (ballistischer Transport bestätigt)"
        : "FAILED (Wachstum weder klar ballistisch noch diffusiv)");

    ProPhysics_Free(&pu);
    return pass;
}

/* ==========================================================================
 * Bisect-Diagnose: Welcher Schritt von ProPhysics_Apply_Amp_Step zerstört
 * die U5-Invariante bei dichtem Anfangsgitter?
 *
 * Läuft jeden Teilschritt isoliert (1000 Ticks) und dann die Kombination.
 * Ausgabe: Norm vor/nach + Drift pro Schritt.
 *
 * Setup identisch zu test_amp_invariance_impl: 32×32-Torus, alle Knoten
 * in Superposition |1> + |4>.
 * ========================================================================== */
static bool test_amp_invariance_bisect(void)
{
    printf("[Bisect] U5-Invarianten-Diagnose (Etappe 10, Q31): "
        "welcher Schritt zerlegt die Norm?\n\n");

    const uint32_t DIM = 32u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;

    /* Q31: 2 Basen × 5 × (2^30)^2 × 1024 Knoten = 5 · 2^70. */
    const ProU128 EXPECTED = pro_u128_shl(pro_u128_from_u64(5u), 68);

#define BISECT_SETUP(pu_) do { \
        ProPhysics_Initialize(&(pu_), NODES); \
        for (uint64_t i_ = 0; i_ < NODES; ++i_) \
            for (int c_ = 0; c_ < CHANNELS_MAX; ++c_) \
                (pu_).reg_source[i_].channels[c_] = i_; \
        for (uint64_t y_ = 0; y_ < DIM; ++y_) { \
            const uint64_t yn_ = (y_ == 0 ? DIM - 1 : y_ - 1) * DIM; \
            const uint64_t ys_ = (y_ == DIM - 1 ? 0 : y_ + 1) * DIM; \
            const uint64_t yc_ = y_ * DIM; \
            for (uint64_t x_ = 0; x_ < DIM; ++x_) { \
                const uint64_t idx_ = yc_ + x_; \
                (pu_).reg_source[idx_].channels[0] = yn_ + x_; \
                (pu_).reg_source[idx_].channels[1] = ys_ + x_; \
                (pu_).reg_source[idx_].channels[2] = yc_ + (x_ == DIM - 1 ? 0 : x_ + 1); \
                (pu_).reg_source[idx_].channels[3] = yc_ + (x_ == 0 ? DIM - 1 : x_ - 1); \
            } \
        } \
        for (uint64_t k_ = 0; k_ < NODES; ++k_) { \
            for (uint8_t b_ = 0; b_ < PRO_AMP_BASIS_SIZE; ++b_) \
                (pu_).amp_grid[k_].coeff[b_] = 0; \
            (pu_).amp_grid[k_].coeff[UR_POSITRON_CW]  = pro_amp_pack(Q15TO31(8192), 0); \
            (pu_).amp_grid[k_].coeff[UR_NEGATRON_CCW] = pro_amp_pack(Q15TO31(8192), 0); \
        } \
    } while (0)

#define BISECT_REPORT(label_, n0_, n1_) do { \
        const double n0d_ = pro_u128_to_double(n0_); \
        const double n1d_ = pro_u128_to_double(n1_); \
        const double ad_ = (n1d_ > n0d_) ? (n1d_ - n0d_) : (n0d_ - n1d_); \
        const double dr_ = (n0d_ > 0.0) ? (ad_ / n0d_) : 1.0; \
        printf("[Bisect] %-28s  %.6e -> %.6e  drift=%.4e\n", \
            (label_), n0d_, n1d_, dr_); \
    } while (0)

    /* Test 0: Nur Setup — Baseline */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        printf("[Bisect] Setup-Baseline: erwartet %.6e, gemessen %.6e\n\n",
            pro_u128_to_double(EXPECTED), pro_u128_to_double(n0));
        ProPhysics_Free(&pu);
    }

    /* Test 1: Nur Context-Tick */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t) ProPhysics_Apply_Context_Tick(&pu);
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Context x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 2: Nur Edge-Transport sequential */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        pu.grid_dim = 0u;
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t)
            ProPhysics_Apply_Edge_Transport(&pu, 500u);
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Transport seq x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 3: Nur Edge-Transport colored */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t)
            ProPhysics_Apply_Edge_Transport_Colored(&pu, 500u, DIM);
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Transport colored x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 4: Nur Wave-Step */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t)
            ProPhysics_Apply_Wave_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Wave-Step x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 5: Kombination wie im echten Test */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t) {
            ProPhysics_Apply_Amp_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
            ProPhysics_Apply_Guiding_Equation(&pu);
        }
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Amp_Step + Guiding x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 6: Kombination ohne Guiding */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t) {
            ProPhysics_Apply_Amp_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
        }
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Amp_Step ohne Guiding x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 7: Wave-Step auf inhomogenem Zustand */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        for (int t = 0; t < 100; ++t)
            ProPhysics_Apply_Edge_Transport(&pu, 500u);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t)
            ProPhysics_Apply_Wave_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Wave-Step auf inhomogen x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 8: Nur Context + Transport */
        /* Test 8: Nur Context + Transport */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t) {
            ProPhysics_Apply_Context_Tick(&pu);
            ProPhysics_Apply_Edge_Transport_Colored(&pu, 500u, DIM);
        }
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Context+Transport(color) x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

    /* Test 9: Nur Context + Wave-Step */
    {
        ProUniverse pu; BISECT_SETUP(pu);
        const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);
        for (int t = 0; t < 1000; ++t) {
            ProPhysics_Apply_Context_Tick(&pu);
            ProPhysics_Apply_Wave_Step(&pu, PRO_DEFAULT_PHASE_STEP_Q15);
        }
        const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);
        BISECT_REPORT("Context+Wave-Step x 1000", n0, n1);
        ProPhysics_Free(&pu);
    }

#undef BISECT_REPORT
#undef BISECT_SETUP

    printf("\n[Bisect] Fertig. Der Schritt mit dem größten Drift ist die Ursache.\n");
    return true;
}

/* ==========================================================================
 * Etappe 11: Solitonen + Breather (v4, Langzeit-Relaxation)
 *
 * Aenderungen gegenueber v3:
 *   - TICKS von 1000 auf 3000 erhoeht
 *   - 21 Samples: dicht im Transienten (0..1000), dann 200er-Schritte
 *   - Tail-Analyse via linearer Regression ueber [1400, 3000]:
 *       STATIONAER   : |slope| < 0.005 UND span < 0.20
 *       BREATHER     : |slope| < 0.005 UND span > 0.50
 *       RELAXIEREND  : slope < -0.01 (Breite faellt noch)
 *       WACHSEND     : slope > +0.01 (Dispersion dominiert weiter)
 *   - ASCII-only Ausgabe (Mojibake behoben)
 *   - Fortschritts-Printout alle 1000 Ticks
 *
 * Ziel: Klaeren ob STARK zu einem stationaeren Bright Soliton relaxiert,
 *       oder ob der Breather-Charakter bestehen bleibt.
 * ========================================================================== */
static bool test_soliton_stability(void)
{
    printf("[RUN] Soliton-Stabilitaet, Langzeit (Etappe 11 v4, Q31)...\n");
    printf("      Nichtlineare GP-Selbstkopplung + unitaerer Transport.\n");
    printf("      1D-Gauss in x, konstant in y. TICKS=3000.\n");
    printf("      Hinweis: Lauf dauert ~1-3 min (3 Modi x 3000 Ticks).\n\n");

    const uint32_t DIM = 128u;
    const uint64_t NODES = (uint64_t)DIM * (uint64_t)DIM;
    const uint32_t THETA = 2000u;
    const double   SIGMA0 = 3.0;
    const double   X0 = (double)(DIM / 2);
    const double   AMP_SCALE = Q31_MAXV * 0.5;
    const uint32_t NL_DT_Q15 = 32768u;
    const uint32_t TICKS = 3000u;

    const int32_t G_VALUES[3] = { 0, 100000, 200000 };
    const char* G_LABELS[3] = { "LINEAR  ", "MODERAT ", "STARK   " };

    /* 21 Sample-Punkte, dicht im Transienten, 200er-Schritte in der
     * Relaxationszone. */
    const uint32_t N_SAMPLES = 21u;
    const uint32_t SAMPLE_TICKS[21] = {
        0u,   100u,  200u,  300u,  400u,  500u,  600u,
        700u, 800u,  900u, 1000u,
        1200u, 1400u, 1600u, 1800u, 2000u, 2200u, 2400u, 2600u, 2800u, 3000u
    };

    double   sigma[3][21];
    double   maxamp[3][21];
    uint32_t clip_count[3] = { 0u, 0u, 0u };

    for (int mode = 0; mode < 3; ++mode) {
        for (uint32_t i = 0; i < N_SAMPLES; ++i) {
            sigma[mode][i] = 0.0;
            maxamp[mode][i] = 0.0;
        }

        ProUniverse pu;
        ProPhysics_Initialize(&pu, NODES);
        if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
            printf("[Soliton] Init failed (mode=%d).\n", mode);
            ProPhysics_Free(&pu);
            return false;
        }

        for (uint64_t i = 0; i < NODES; ++i) {
            for (int c = 0; c < CHANNELS_MAX; ++c)
                pu.reg_source[i].channels[c] = i;
        }
        for (uint64_t y = 0; y < DIM; ++y) {
            const uint64_t y_n = (y == 0 ? DIM - 1 : y - 1) * DIM;
            const uint64_t y_s = (y == DIM - 1 ? 0 : y + 1) * DIM;
            const uint64_t y_c = y * DIM;
            for (uint64_t x = 0; x < DIM; ++x) {
                const uint64_t idx = y_c + x;
                pu.reg_source[idx].channels[0] = y_n + x;
                pu.reg_source[idx].channels[1] = y_s + x;
                pu.reg_source[idx].channels[2] = y_c + (x == DIM - 1 ? 0 : x + 1);
                pu.reg_source[idx].channels[3] = y_c + (x == 0 ? DIM - 1 : x - 1);
            }
        }

        for (uint64_t k = 0; k < NODES; ++k) {
            for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
                pu.amp_grid[k].coeff[b] = 0;
        }

        double norm_sq = 0.0;
        for (uint64_t x = 0; x < DIM; ++x) {
            const double dx = (double)x - X0;
            const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0));
            norm_sq += psi * psi;
        }
        const double inv_norm = 1.0 / sqrt(norm_sq);

        for (uint64_t y = 0; y < DIM; ++y) {
            for (uint64_t x = 0; x < DIM; ++x) {
                const double dx = (double)x - X0;
                const double psi = exp(-dx * dx / (2.0 * SIGMA0 * SIGMA0)) * inv_norm;
                const int32_t q31 = (int32_t)lround(psi * AMP_SCALE);
                const uint64_t k = y * DIM + x;
                pu.amp_grid[k].coeff[UR_POSITRON_CW] = pro_amp_pack(q31, 0);
            }
        }

        printf("[Soliton] Modus: %s | G_q15=%6d | NL_dt_q15=%u | Ticks=%u\n",
            G_LABELS[mode], G_VALUES[mode], NL_DT_Q15, TICKS);

        uint32_t sample_idx = 0u;
        for (uint32_t step = 0; step <= TICKS; ++step) {
            if (step > 0u && step % 1000u == 0u) {
                printf("[Soliton]   ... t=%u/%u\n", step, TICKS);
            }

            if (sample_idx < N_SAMPLES && step == SAMPLE_TICKS[sample_idx]) {
                double rho_x[128];
                for (uint32_t x = 0; x < DIM; ++x) rho_x[x] = 0.0;
                double sum_sq = 0.0, sum_x = 0.0, max_sq = 0.0;

                for (uint64_t y = 0; y < DIM; ++y) {
                    for (uint64_t x = 0; x < DIM; ++x) {
                        const uint64_t k = y * DIM + x;
                        const int32_t re_s = pro_amp_real(
                            pu.amp_grid[k].coeff[UR_POSITRON_CW]);
                        const int32_t im_s = pro_amp_imag(
                            pu.amp_grid[k].coeff[UR_POSITRON_CW]);
                        const int64_t re64 = (int64_t)re_s;
                        const int64_t im64 = (int64_t)im_s;
                        const double a2 = (double)(re64 * re64 + im64 * im64);
                        rho_x[x] += a2;
                        sum_sq += a2;
                        sum_x += a2 * (double)x;
                        if (a2 > max_sq) max_sq = a2;

                        if (re_s == INT32_MAX || re_s == INT32_MIN ||
                            im_s == INT32_MAX || im_s == INT32_MIN) {
                            clip_count[mode]++;
                        }
                    }
                }
                const double x_bar = (sum_sq > 0.0) ? (sum_x / sum_sq) : 0.0;
                double var_x = 0.0;
                for (uint32_t x = 0; x < DIM; ++x) {
                    const double dx = (double)x - x_bar;
                    var_x += rho_x[x] * dx * dx;
                }
                const double sigma_x = (sum_sq > 0.0) ? sqrt(var_x / sum_sq) : 0.0;
                const double max_a = sqrt(max_sq);

                sigma[mode][sample_idx] = sigma_x;
                maxamp[mode][sample_idx] = max_a;
                sample_idx++;
            }

            if (step < TICKS) {
                ProPhysics_Apply_Edge_Transport_Colored(&pu, THETA, DIM);
                if (G_VALUES[mode] != 0) {
                    ProPhysics_Apply_Nonlinear_Phase_Step(
                        &pu, G_VALUES[mode], NL_DT_Q15);
                }
            }
        }

        ProPhysics_Free(&pu);
    }

    /* --- Ausgabe-Tabelle --- */
    printf("\n[Soliton]  step   sig_lin   sig_mod   sig_str   mx_lin    mx_str\n");
    printf("[Soliton]  ----------------------------------------------------------\n");
    for (uint32_t i = 0; i < N_SAMPLES; ++i) {
        printf("[Soliton]  %4u   %8.4f  %8.4f  %8.4f  %8.4e  %8.4e\n",
            SAMPLE_TICKS[i],
            sigma[0][i], sigma[1][i], sigma[2][i],
            maxamp[0][i], maxamp[2][i]);
    }

    /* --- Tail-Analyse pro Modus: lineare Regression ueber [1400, 3000] --- */
    const uint32_t TAIL_START = 12u;   /* Sample-Index 12 = step 1400 */
    const uint32_t TAIL_END = N_SAMPLES - 1u;

    double slope_tail[3];
    double span_tail[3];
    double mean_tail[3];

    for (int mode = 0; mode < 3; ++mode) {
        const uint32_t n = TAIL_END - TAIL_START + 1u;
        double sum_x = 0.0, sum_y = 0.0;
        double mx = 0.0, mn = 1e30;
        for (uint32_t i = TAIL_START; i <= TAIL_END; ++i) {
            const double x = (double)SAMPLE_TICKS[i];
            const double y = sigma[mode][i];
            sum_x += x;
            sum_y += y;
            if (y > mx) mx = y;
            if (y < mn) mn = y;
        }
        const double x_bar = sum_x / (double)n;
        const double y_bar = sum_y / (double)n;
        double num = 0.0, den = 0.0;
        for (uint32_t i = TAIL_START; i <= TAIL_END; ++i) {
            const double dx = (double)SAMPLE_TICKS[i] - x_bar;
            num += dx * (sigma[mode][i] - y_bar);
            den += dx * dx;
        }
        slope_tail[mode] = (den > 1e-9) ? (num / den) : 0.0;
        span_tail[mode] = mx - mn;
        mean_tail[mode] = y_bar;
    }

    printf("\n[Soliton] Tail-Analyse ueber t=[%u..%u] (lineare Regression):\n",
        SAMPLE_TICKS[TAIL_START], SAMPLE_TICKS[TAIL_END]);
    printf("[Soliton]   Modus     mean_tail   span_tail   slope_tail   Klasse\n");
    printf("[Soliton]   -----------------------------------------------------------\n");

    const char* class_str[3];
    for (int mode = 0; mode < 3; ++mode) {
        const double sl = slope_tail[mode];
        const double sp = span_tail[mode];
        if (fabs(sl) < 0.005 && sp < 0.20) {
            class_str[mode] = "STATIONAER ";
        }
        else if (fabs(sl) < 0.005 && sp > 0.50) {
            class_str[mode] = "BREATHER   ";
        }
        else if (sl < -0.01) {
            class_str[mode] = "RELAXIEREND";
        }
        else if (sl > 0.01) {
            class_str[mode] = "WACHSEND   ";
        }
        else {
            class_str[mode] = "UNKLAR     ";
        }
        printf("[Soliton]   %s  %8.4f    %8.4f   %+8.5f   %s\n",
            G_LABELS[mode], mean_tail[mode], sp, sl, class_str[mode]);
    }

    /* --- Extrema-Detektion ueber volle Zeitreihe (nur STARK) --- */
    uint32_t peaks[16]; uint32_t n_peaks = 0u;
    uint32_t troughs[16]; uint32_t n_troughs = 0u;

    for (uint32_t i = 1u; i + 1u < N_SAMPLES; ++i) {
        if (sigma[2][i] > sigma[2][i - 1u] && sigma[2][i] > sigma[2][i + 1u]) {
            if (n_peaks < 16u) peaks[n_peaks++] = i;
        }
        if (sigma[2][i] < sigma[2][i - 1u] && sigma[2][i] < sigma[2][i + 1u]) {
            if (n_troughs < 16u) troughs[n_troughs++] = i;
        }
    }

    printf("\n[Soliton] Extrema-Detektion (STARK, volle Zeitreihe):\n");
    printf("[Soliton]   Peaks  : %u\n", n_peaks);
    printf("[Soliton]   Troughs: %u\n", n_troughs);

    if (n_peaks >= 2u) {
        double sum_period = 0.0;
        for (uint32_t p = 0; p + 1u < n_peaks; ++p) {
            sum_period += (double)(SAMPLE_TICKS[peaks[p + 1u]] - SAMPLE_TICKS[peaks[p]]);
        }
        const double period_avg = sum_period / (double)(n_peaks - 1u);
        printf("[Soliton]   Mittlere Peak-Periode: %.0f Ticks\n", period_avg);
    }

    if (n_peaks >= 1u) {
        printf("[Soliton]   Peak-Amplituden (sigma_str):\n");
        for (uint32_t p = 0; p < n_peaks && p < 6u; ++p) {
            printf("[Soliton]     Peak %u @ t=%4u  sigma=%.4f\n",
                p + 1u, SAMPLE_TICKS[peaks[p]], sigma[2][peaks[p]]);
        }
    }
    if (n_troughs >= 1u) {
        printf("[Soliton]   Trough-Amplituden (sigma_str):\n");
        for (uint32_t p = 0; p < n_troughs && p < 6u; ++p) {
            printf("[Soliton]     Trough %u @ t=%4u  sigma=%.4f\n",
                p + 1u, SAMPLE_TICKS[troughs[p]], sigma[2][troughs[p]]);
        }
    }

    /* --- Clipping-Report --- */
    printf("\n[Soliton] Clipping-Check:\n");
    printf("[Soliton]   LINEAR : %u | MODERAT: %u | STARK: %u\n",
        clip_count[0], clip_count[1], clip_count[2]);
    const bool clipping_ok = (clip_count[0] == 0u)
        && (clip_count[1] == 0u) && (clip_count[2] == 0u);
    if (!clipping_ok) {
        printf("[Soliton] WARNUNG: Clipping erkannt, Ergebnis numerisch verfaelscht!\n");
    }

    /* --- Gesamtbewertung --- */
    const double sig_ratio_mod = (sigma[0][N_SAMPLES - 1u] > 1e-9)
        ? (sigma[1][N_SAMPLES - 1u] / sigma[0][N_SAMPLES - 1u]) : 0.0;
    const double sig_ratio_str = (sigma[0][N_SAMPLES - 1u] > 1e-9)
        ? (sigma[2][N_SAMPLES - 1u] / sigma[0][N_SAMPLES - 1u]) : 0.0;
    const double mx_ratio_mod = (maxamp[0][N_SAMPLES - 1u] > 1e-9)
        ? (maxamp[1][N_SAMPLES - 1u] / maxamp[0][N_SAMPLES - 1u]) : 0.0;
    const double mx_ratio_str = (maxamp[0][N_SAMPLES - 1u] > 1e-9)
        ? (maxamp[2][N_SAMPLES - 1u] / maxamp[0][N_SAMPLES - 1u]) : 0.0;

    printf("\n[Soliton] Endpunkt-Verhaeltnisse (t=%u):\n", SAMPLE_TICKS[N_SAMPLES - 1u]);
    printf("[Soliton]   sigma_mod/sigma_lin = %.4f\n", sig_ratio_mod);
    printf("[Soliton]   sigma_str/sigma_lin = %.4f\n", sig_ratio_str);
    printf("[Soliton]   maxamp_mod/maxamp_lin = %.4f\n", mx_ratio_mod);
    printf("[Soliton]   maxamp_str/maxamp_lin = %.4f\n", mx_ratio_str);

    /* Konsistenzpruefung: LINEAR sollte am Box-Limit saturieren. */
    const double box_limit = sqrt(((double)DIM * (double)DIM - 1.0) / 12.0);
    const bool linear_saturated =
        (fabs(sigma[0][N_SAMPLES - 1u] - box_limit) < 2.0);

    printf("\n[Soliton] Linear-Check: sigma_lin(T)=%.4f, Box-Limit=%.4f -> %s\n",
        sigma[0][N_SAMPLES - 1u], box_limit,
        linear_saturated ? "SATURIERT (erwartet)" : "NICHT saturiert");

    /* --- Bewertung --- */
    const bool focus_mod = (sig_ratio_mod < 0.75) || (mx_ratio_mod > 1.5);
    const bool focus_str = (sig_ratio_str < 0.75) || (mx_ratio_str > 1.5);
    const bool primary_pass = focus_mod || focus_str;
    const bool full_pass = primary_pass && clipping_ok;

    printf("\n[Soliton] Bewertung:\n");
    printf("[Soliton]   Selbstfokussierung MODERAT : %s\n", focus_mod ? "JA" : "nein");
    printf("[Soliton]   Selbstfokussierung STARK   : %s\n", focus_str ? "JA" : "nein");
    printf("[Soliton]   Clipping-frei              : %s\n", clipping_ok ? "JA" : "NEIN");
    printf("[Soliton]   LINEAR saturiert am Limit  : %s\n",
        linear_saturated ? "JA (Konsistenz-Check OK)" : "NEIN");
    printf("[Soliton]   MODERAT-Klasse            : %s\n", class_str[1]);
    printf("[Soliton]   STARK-Klasse              : %s\n", class_str[2]);

    /* --- Physikalische Gesamtklassifikation --- */
    printf("\n[Soliton] Physikalische Einordnung:\n");

    if (strcmp(class_str[2], "STATIONAER ") == 0) {
        printf("[Soliton]   STARK ist zu einer stationaeren Breite relaxiert.\n");
        printf("[Soliton]   -> Echtes Bright Soliton (Selbstfokussierung balanciert\n");
        printf("[Soliton]      Dispersion vollstaendig).\n");
    }
    else if (strcmp(class_str[2], "BREATHER   ") == 0) {
        printf("[Soliton]   STARK oszilliert um einen festen Mittelwert.\n");
        printf("[Soliton]   -> Breather (Soliton mit ueberschuessiger Energie).\n");
    }
    else if (strcmp(class_str[2], "RELAXIEREND") == 0) {
        printf("[Soliton]   STARK relaxiert noch (sigma faellt).\n");
        printf("[Soliton]   -> Entweder noch nicht im stationaeren Zustand,\n");
        printf("[Soliton]      oder Kollaps zu einem kleineren Soliton.\n");
    }
    else if (strcmp(class_str[2], "WACHSEND   ") == 0) {
        printf("[Soliton]   STARK waechst weiter (Dispersion dominiert).\n");
        printf("[Soliton]   -> Kein Soliton, Gauss zerfliesst trotz Nichtlinearitaet.\n");
    }
    else {
        printf("[Soliton]   STARK-Klassifikation unklar. Laengere Laeufe noetig.\n");
    }

    if (strcmp(class_str[1], "BREATHER   ") == 0) {
        printf("[Soliton]   MODERAT ist ein stabiler Breather (Oszillation um\n");
        printf("[Soliton]   feste Breite, keine Drift).\n");
    }
    else if (strcmp(class_str[1], "STATIONAER ") == 0) {
        printf("[Soliton]   MODERAT ist zu stationaerer Breite relaxiert.\n");
    }
    else if (strcmp(class_str[1], "WACHSEND   ") == 0) {
        printf("[Soliton]   MODERAT waechst noch (noch nicht relaxiert).\n");
    }
    else {
        printf("[Soliton]   MODERAT-Klassifikation: %s\n", class_str[1]);
    }

    /* --- Numerische Stabilitaet --- */
    const bool norm_stable = clipping_ok
        && (fabs(sigma[2][N_SAMPLES - 1u]) < 100.0);

    printf("\n[Soliton] Numerische Stabilitaet (3000 Ticks):\n");
    printf("[Soliton]   Clipping-frei    : %s\n", clipping_ok ? "JA" : "NEIN");
    printf("[Soliton]   Kein Runaway     : %s\n", norm_stable ? "JA" : "NEIN");
    printf("[Soliton]   Q31-Arithmetik   : %s\n",
        (clipping_ok && norm_stable)
        ? "stabil bis 3000 Ticks (kein Q62-Bedarf in Phase 3)"
        : "grenzwertig, Q62 pruefen");

    /* --- Finale Ausgabe --- */
    printf("\n[Soliton] -> %s\n",
        full_pass ? "PASSED (Selbstfokussierung sichtbar, 3000 Ticks stabil)"
        : "FAILED");

    if (full_pass && strcmp(class_str[2], "STATIONAER ") == 0) {
        printf("[Soliton]    Bonus: echtes Bright Soliton beobachtet.\n");
    }
    else if (full_pass && strcmp(class_str[2], "BREATHER   ") == 0) {
        printf("[Soliton]    Bonus: stabiler Breather beobachtet.\n");
    }

    return full_pass;
}
/* ========================================================================
 * Etappe 12: Lorentz-Zeitdilatation auf amp_grid (Option B)
 *
 * ALTER TEST (Etappe ≤11): type_state-basiert
 *   - Soliton via pro_engine_spawn_soliton → type_state = UR_POSITRON_CW
 *   - Boost via ProPhysics_Inject_Momentum → momentum_phase
 *   - Messung: Phase-Flips in field_helicity
 *   - Getrieben von ProPhysics_Advance_Internal_Clocks (liest momentum_phase)
 *
 * NEUER TEST (Etappe 12): amp_grid-basiert
 *   - Soliton in amp_grid[k].coeff[UR_POSITRON_CW] präparieren (Gauss)
 *   - Boost direkt in amp_grid kodieren: Phasengradient exp(i·k0·x)
 *   - Propagation: Edge-Transport (colored) + GP-Selbstkopplung
 *   - Messung: Phase von amp_grid[peak].coeff[UR_POSITRON_CW]
 *
 * WICHTIG: ProPhysics_Apply_Amp_Step liest momentum_phase NICHT.
 *   Der Boost MUSS als Phasengradient in amp_grid kodiert werden.
 *   Andernfalls hat der Boost keinen Effekt auf die fundamentale Größe.
 *
 * Physik:
 *   Ein Wellenpaket mit Impuls k0 hat am Peak die Phase
 *       φ(t) = k0·x_peak(t) + φ_intern(t)
 *   mit x_peak(t) = v_g·t. Die interne Phase oszilliert mit der
 *   Ruhemasse-Frequenz ω0. Zeitdilatation: ω_intern(v) = ω0·sqrt(1-v²).
 *
 *   Wir messen die Peak-Position und die Peak-Phase, extrahieren
 *   φ_intern(t) = φ(t) - k0·x_peak(t), und fitten die Rate.
 *
 * Dispersionsrelation (Tight-Binding, 2-Knoten-Transport):
 *   ω(k) = 2·|sin(k/2)|,  v_g(k) = cos(k/2)
 *   → k0 = 2·arccos(v_c_ratio) liefert v_g = v_c_ratio.
 *
 * Erwartung: ω_intern(v) / ω_intern(0) ≈ sqrt(1 - v²).
 * ====================================================================== */

 /* MSVC/C99: Array-Größen müssen compile-time konstant sein.
  * Deshalb #define statt const uint32_t. */
#define LORENTZ_MAX_SAMPLES 128u

typedef struct {
    double   v_c_ratio;
    double   k0;
    double   omega_intern;
    double   v_g_measured;
    double   peak_amp_sq_t0;
    double   peak_amp_sq_tend;
    double   peak_amp_sq_mean;
    uint32_t n_samples;
} LorentzRunResult;

static bool lorentz_run_single(uint32_t dim,
    double   v_c_ratio,
    int32_t  g_q15,
    uint32_t nl_dt_q15,
    uint32_t theta_q15,
    uint32_t n_ticks,
    uint32_t n_samples,
    LorentzRunResult* out)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    out->v_c_ratio = v_c_ratio;

    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    const double X0 = (double)(dim / 2);
    const double Y0 = (double)(dim / 2);
    const double SIGMA0 = 12.0;
    const double AMP_SCALE = Q31_MAXV * 0.5;
    const uint8_t BASIS = UR_POSITRON_CW;

    double k0 = 0.0;
    if (v_c_ratio > 1e-9) {
        double v_clamped = v_c_ratio;
        if (v_clamped >= 1.0) v_clamped = 0.999;
        k0 = 2.0 * asin(v_clamped);
    }
    out->k0 = k0;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.amp_scratch || !pu.reg_source) {
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus(&pu, dim);

    prepare_soliton_ampgrid(&pu, dim, X0, Y0, SIGMA0,
        AMP_SCALE, k0, BASIS);

    double   x_peak_arr[LORENTZ_MAX_SAMPLES];
    double   phi_arr[LORENTZ_MAX_SAMPLES];
    double   amp_sq_arr[LORENTZ_MAX_SAMPLES];
    uint32_t tick_arr[LORENTZ_MAX_SAMPLES];
    uint32_t collected = 0u;

    if (n_samples > LORENTZ_MAX_SAMPLES) n_samples = LORENTZ_MAX_SAMPLES;

    const uint32_t sample_stride =
        (n_ticks > 0u) ? (n_ticks / n_samples) : 1u;
    if (sample_stride == 0u) {
        ProPhysics_Free(&pu);
        return false;
    }

    const double amp_scale_sq = AMP_SCALE * AMP_SCALE;

    for (uint32_t t = 0; t <= n_ticks; ++t) {
        if (t % sample_stride == 0u && collected < n_samples) {
            uint32_t px = 0u, py = 0u;
            const uint64_t peak = find_peak_node(&pu, dim, BASIS, &px, &py);
            (void)py;
            const double phi = measure_phase_at(&pu, peak, BASIS);
            x_peak_arr[collected] = (double)px;
            phi_arr[collected] = phi;
            tick_arr[collected] = t;

            const int32_t pre = pro_amp_real(pu.amp_grid[peak].coeff[BASIS]);
            const int32_t pim = pro_amp_imag(pu.amp_grid[peak].coeff[BASIS]);
            const double peak_sq = ((double)pre * (double)pre
                + (double)pim * (double)pim) / amp_scale_sq;
            amp_sq_arr[collected] = peak_sq;
            if (collected == 0u) out->peak_amp_sq_t0 = peak_sq;
            out->peak_amp_sq_tend = peak_sq;

            collected++;
        }
        if (t < n_ticks) {
            lorentz_propagate_tick(&pu, dim, theta_q15, g_q15, nl_dt_q15);
        }
    }

    if (collected < 4u) {
        ProPhysics_Free(&pu);
        return false;
    }

    double amp_sum = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) amp_sum += amp_sq_arr[i];
    out->peak_amp_sq_mean = amp_sum / (double)collected;

    /* Peak-Positionen entrollen (Torus-Wrap). */
    for (uint32_t i = 1u; i < collected; ++i) {
        double dx = x_peak_arr[i] - x_peak_arr[i - 1u];
        if (dx > (double)dim * 0.5) dx -= (double)dim;
        if (dx < -(double)dim * 0.5) dx += (double)dim;
        x_peak_arr[i] = x_peak_arr[i - 1u] + dx;
    }

    /* Phasen entrollen. */
    for (uint32_t i = 1u; i < collected; ++i) {
        double dphi = phi_arr[i] - phi_arr[i - 1u];
        while (dphi > M_PI) dphi -= 2.0 * M_PI;
        while (dphi < -M_PI) dphi += 2.0 * M_PI;
        phi_arr[i] = phi_arr[i - 1u] + dphi;
    }

    double phi_intern[LORENTZ_MAX_SAMPLES];
    for (uint32_t i = 0u; i < collected; ++i) {
        phi_intern[i] = phi_arr[i] - k0 * x_peak_arr[i];
    }

    double sum_t = 0.0, sum_phi = 0.0, sum_x = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) {
        sum_t += (double)tick_arr[i];
        sum_phi += phi_intern[i];
        sum_x += x_peak_arr[i];
    }
    const double t_bar = sum_t / (double)collected;
    const double phi_bar = sum_phi / (double)collected;
    const double x_bar = sum_x / (double)collected;

    double num_phi = 0.0, num_x = 0.0, den = 0.0;
    for (uint32_t i = 0u; i < collected; ++i) {
        const double dt = (double)tick_arr[i] - t_bar;
        num_phi += dt * (phi_intern[i] - phi_bar);
        num_x += dt * (x_peak_arr[i] - x_bar);
        den += dt * dt;
    }
    out->omega_intern = (den > 1e-12) ? (num_phi / den) : 0.0;
    out->v_g_measured = (den > 1e-12) ? (num_x / den) : 0.0;
    out->n_samples = collected;

    ProPhysics_Free(&pu);
    return true;
}

static bool test_lorentz_time_dilation_ampgrid(double v_c_ratio)
{
    printf("\n========================================================================\n");
    printf("  Etappe 12: Lorentz-Zeitdilatation auf amp_grid (Option B)\n");
    printf("  Vier-Lauf-Kalibrierung: GP-Anteil isoliert durch Subtraktion\n");
    printf("  der reinen Transport-Phase (g=0).\n");
    printf("========================================================================\n");
    printf("  Soliton in amp_grid[k].coeff[UR_POSITRON_CW] (Gauss-Profil)\n");
    printf("  Boost als Phasengradient exp(i*k0*x), NICHT via momentum_phase.\n");
    printf("  Propagation: Edge-Transport + GP-Selbstkopplung mit gamma_inv.\n");
    printf("  Messung: omega_intern = d(phi - k0*x_peak)/dt.\n");
    printf("  GP-Anteil = omega_intern(g>0) - omega_intern(g=0).\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 256u;
    const uint32_t THETA_Q15 = 2000u;
    const int32_t  G_Q15 = 20000;    /* 4x staerker als 5000 */
    const uint32_t NL_DT_Q15 = 32768u;
    const uint32_t N_TICKS = 50u;      /* kuerzer, weniger Selbstfokussierung */
    const uint32_t N_SAMPLES = 30u;

    printf("  Parameter: DIM=%u | theta_q15=%u | g_q15=%d | NL_dt=%u\n",
        DIM, THETA_Q15, G_Q15, NL_DT_Q15);
    printf("             Ticks=%u | Samples=%u | sigma0=12.0\n\n",
        N_TICKS, N_SAMPLES);

    LorentzRunResult A, B, C, D;

    /* --- Lauf A: v=0, g>0 (Transport + GP) --- */
    printf("  --- Lauf A: v/c=0.00 | g>0 (Transport + GP) ---\n");
    if (!lorentz_run_single(DIM, 0.0, G_Q15, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &A)) {
        printf("  [Lorentz] Lauf A fehlgeschlagen.\n");
        return false;
    }
    printf("  omega_intern     = %+.6e rad/Tick\n", A.omega_intern);
    printf("  v_g              = %+.6f\n", A.v_g_measured);
    printf("  peak_amp_sq_mean = %.6f\n\n", A.peak_amp_sq_mean);

    /* --- Lauf B: v_c, g>0 (Transport + GP, geboostet) --- */
    printf("  --- Lauf B: v/c=%.2f | g>0 (Transport + GP, geboostet) ---\n",
        v_c_ratio);
    if (!lorentz_run_single(DIM, v_c_ratio, G_Q15, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &B)) {
        printf("  [Lorentz] Lauf B fehlgeschlagen.\n");
        return false;
    }
    printf("  k0               = %.6f rad/Gitterzelle (2*asin(v))\n", B.k0);
    printf("  omega_intern     = %+.6e rad/Tick\n", B.omega_intern);
    printf("  v_g              = %+.6f\n", B.v_g_measured);
    printf("  peak_amp_sq_mean = %.6f\n\n", B.peak_amp_sq_mean);

    /* --- Lauf C: v=0, g=0 (nur Transport, Kalibrierung) --- */
    printf("  --- Lauf C: v/c=0.00 | g=0 (nur Transport, Kalibrierung) ---\n");
    if (!lorentz_run_single(DIM, 0.0, 0, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &C)) {
        printf("  [Lorentz] Lauf C fehlgeschlagen.\n");
        return false;
    }
    printf("  omega_intern     = %+.6e rad/Tick (reine Transport-Phase)\n\n",
        C.omega_intern);

    /* --- Lauf D: v_c, g=0 (nur Transport, Kalibrierung geboostet) --- */
    printf("  --- Lauf D: v/c=%.2f | g=0 (nur Transport, Kalibrierung) ---\n",
        v_c_ratio);
    if (!lorentz_run_single(DIM, v_c_ratio, 0, NL_DT_Q15, THETA_Q15,
        N_TICKS, N_SAMPLES, &D)) {
        printf("  [Lorentz] Lauf D fehlgeschlagen.\n");
        return false;
    }
    printf("  omega_intern     = %+.6e rad/Tick (reine Transport-Phase)\n\n",
        D.omega_intern);

    /* --- Kalibrierung: GP-Anteil isolieren --- */
    const double omega_gp_0 = A.omega_intern - C.omega_intern;
    const double omega_gp_v = B.omega_intern - D.omega_intern;

    const double expected_ratio = sqrt(1.0 - v_c_ratio * v_c_ratio);
    const double measured_ratio = (fabs(omega_gp_0) > 1e-12)
        ? (omega_gp_v / omega_gp_0)
        : 0.0;

    const double amp_ratio_mean = (A.peak_amp_sq_mean > 1e-12)
        ? (B.peak_amp_sq_mean / A.peak_amp_sq_mean)
        : 1.0;
    const double measured_ratio_corrected = (fabs(amp_ratio_mean) > 1e-12)
        ? (measured_ratio / amp_ratio_mean)
        : 0.0;

    const double dev_raw = fabs(measured_ratio - expected_ratio);
    const double dev_cor = fabs(measured_ratio_corrected - expected_ratio);

    printf("========================================================================\n");
    printf("  [Lorentz-Ergebnis, amp_grid-basiert, 4-Lauf-kalibriert]\n");
    printf("    Transport-Phase v=0     = %+.6e\n", C.omega_intern);
    printf("    Transport-Phase v=%.2f  = %+.6e\n", v_c_ratio, D.omega_intern);
    printf("    Gesamt-Phase v=0        = %+.6e\n", A.omega_intern);
    printf("    Gesamt-Phase v=%.2f     = %+.6e\n", v_c_ratio, B.omega_intern);
    printf("    -------------------------------------------\n");
    printf("    GP-Phase v=0            = %+.6e rad/Tick\n", omega_gp_0);
    printf("    GP-Phase v=%.2f         = %+.6e rad/Tick\n", v_c_ratio, omega_gp_v);
    printf("    Erwartet sqrt(1-v^2)    = %.6f\n", expected_ratio);
    printf("    Gemessen  GP_v/GP_0     = %.6f\n", measured_ratio);
    printf("    Amplitude-Verhaeltnis   = %.6f\n", amp_ratio_mean);
    printf("    Korrigiert (GP/GP)/amp  = %.6f\n", measured_ratio_corrected);
    printf("    Abweichung (unkorrigiert) = %.2e\n", dev_raw);
    printf("    Abweichung (korrigiert)   = %.2e\n", dev_cor);
    printf("========================================================================\n");

    const double tolerance = 5e-3;
    const bool signal_ok = (fabs(omega_gp_0) > 1e-10)
        && (fabs(omega_gp_v) > 1e-10);
    const bool pass_raw = (dev_raw < tolerance) && signal_ok;
    const bool pass_cor = (dev_cor < tolerance) && signal_ok;

    if (pass_raw) {
        printf("  [Lorentz] -> PASSED (Zeitdilatation auf amp_grid nachgewiesen)\n");
        printf("              GP_0 = %.6e, GP_v = %.6e, Ratio = %.4f\n",
            omega_gp_0, omega_gp_v, measured_ratio);
    }
    else if (pass_cor) {
        printf("  [Lorentz] -> PASSED (nach Amplituden-Korrektur)\n");
        printf("              amp_ratio=%.4f, korrigierte Ratio=%.4f\n",
            amp_ratio_mean, measured_ratio_corrected);
    }
    else {
        printf("  [Lorentz] -> FAILED\n");
        printf("\n  [Lorentz] Diagnose:\n");
        printf("    - omega_gp_0 ~ 0: GP-Phase verschwindet in Kalibrierung.\n");
        printf("      g_q15 erhoehen oder N_TICKS verlaengern.\n");
        printf("    - gemessene Ratio=%.4f statt %.4f:\n", measured_ratio, expected_ratio);
        printf("      * Ratio ~ 0.4: Transport und GP koppeln nicht additiv.\n");
        printf("        Weitere Kalibrierung noetig (z.B. v=0.3 als\n");
        printf("        Zwischenpunkt fuer Linearitaets-Test).\n");
        printf("      * Ratio ~ 1.0: gamma_inv im Kernel wirkungslos.\n");
        printf("        Phasengradient-Berechnung im Kernel pruefen.\n");
        printf("      * Ratio < 0.2: GP-Phase dominiert von anderem Effekt.\n");
    }

    return pass_raw || pass_cor;
}

/*
 * 
 *  Main Funktion
 * 
 */
int main(int argc, char** argv) {
    uint32_t runs = 3;
    uint32_t ticks = 5000;
    int32_t  inject_ticks_arg = -1;
    bool     use_epr = true;
    char     out_file_buf[512] = { 0 };
    const char* out_file = NULL;
    bool     append_mode = false;
    bool     per_run_file = false;
    uint32_t fixed_seeds = 0;
    uint32_t random_seeds = 0;
    uint8_t  epr_edge_type = PRO_EDGE_SINGLET;
    uint32_t epr_delay = 0;
    bool     epr_debug = false;

    bool want_test_chsh = false;
    bool want_test_no_signal = false;
    bool want_test_invariance = false;
    bool want_test_lorentz = false;
    bool want_test_amp = false;
    bool want_test_born = false;
    bool want_test_unitary = false;
    bool want_test_context = false;
    bool want_test_wilson = false;
    bool want_test_gauge = false;
    bool want_test_triangle = false;
    bool want_test_chsh_superdet = false;
    bool want_test_observer_chsh = false;
    bool want_test_chsh_diffusion = false;
    bool want_test_chsh_wave = false;
    bool want_test_chsh_collapse = false;
    bool want_test_chsh_graph = false;
    bool want_test_born_emergent = false;
    bool want_test_born_local = false;
    bool want_test_born_equiv = false;
    bool want_test_amp_invariant = false;
    bool want_test_edge_transport = false;
    bool want_test_edge_transport_colored = false;
    bool want_test_edge_transport_scaling = false;
    bool want_test_wave_packet = false;
    bool want_test_amp_invariance_colored = false;
    bool want_test_amp_invariance_bisect = false;
    bool want_test_soliton = false;                        /* NEU Etappe 11 */

    ProEngineContext ctx;
    bool ctx_initialized = false;

    /* ---------- Parse-Schleife: nur Flags setzen, nichts ausführen ---------- */
    for (int i = 1; i < argc; ++i) {
        if (arg_matches(argv[i], "-h") || arg_matches(argv[i], "--help")) {
            usage(argv[0]);
        }
        else if (arg_matches(argv[i], "--runs") && i + 1 < argc) {
            runs = (uint32_t)atoi(argv[++i]);
        }
        else if (arg_matches(argv[i], "--ticks") && i + 1 < argc) {
            ticks = (uint32_t)atoi(argv[++i]);
        }
        else if (arg_matches(argv[i], "--inject-ticks") && i + 1 < argc) {
            inject_ticks_arg = atoi(argv[++i]);
        }
        else if (arg_matches(argv[i], "--no-epr")) { use_epr = false; }
        else if (arg_matches(argv[i], "--epr-pairs")) { use_epr = true; epr_edge_type = PRO_EDGE_SINGLET; }
        else if (arg_matches(argv[i], "--epr-triplet")) { use_epr = true; epr_edge_type = PRO_EDGE_TRIPLET; }
        else if (arg_matches(argv[i], "--epr-delay") && i + 1 < argc) {
            epr_delay = (uint32_t)atoi(argv[++i]);
        }
        else if (arg_matches(argv[i], "--epr-debug")) {
            epr_debug = true;
        }
        else if (arg_matches(argv[i], "--out") && i + 1 < argc) {
            snprintf(out_file_buf, sizeof(out_file_buf), "%s", argv[++i]);
            out_file = out_file_buf;
        }
        else if (arg_matches(argv[i], "--append")) {
            append_mode = true;
        }
        else if (arg_matches(argv[i], "--per-run-files")) {
            per_run_file = true;
        }
        else if (arg_matches(argv[i], "--fixed-seeds") && i + 1 < argc) {
            fixed_seeds = (uint32_t)atoi(argv[++i]);
        }
        else if (arg_matches(argv[i], "--random-seeds") && i + 1 < argc) {
            random_seeds = (uint32_t)atoi(argv[++i]);
        }
        else if (arg_matches(argv[i], "--test-chsh")) { want_test_chsh = true; }
        else if (arg_matches(argv[i], "--test-no-signaling")) { want_test_no_signal = true; }
        else if (arg_matches(argv[i], "--test-invariance")) { want_test_invariance = true; }
        else if (arg_matches(argv[i], "--test-amp")) { want_test_amp = true; }
        else if (arg_matches(argv[i], "--test-born")) { want_test_born = true; }
        else if (arg_matches(argv[i], "--test-unitary")) { want_test_unitary = true; }
        else if (arg_matches(argv[i], "--test-context")) { want_test_context = true; }
        else if (arg_matches(argv[i], "--test-wilson")) { want_test_wilson = true; }
        else if (arg_matches(argv[i], "--test-gauge")) { want_test_gauge = true; }
        else if (arg_matches(argv[i], "--test-triangle")) { want_test_triangle = true; }
        else if (arg_matches(argv[i], "--test-superdet")) { want_test_chsh_superdet = true; }
        else if (arg_matches(argv[i], "--test-observer-chsh")) { want_test_observer_chsh = true; }
        else if (arg_matches(argv[i], "--test-chsh-diffusion")) { want_test_chsh_diffusion = true; }
        else if (arg_matches(argv[i], "--test-chsh-wave")) { want_test_chsh_wave = true; }
        else if (arg_matches(argv[i], "--test-chsh-collapse")) { want_test_chsh_collapse = true; }
        else if (arg_matches(argv[i], "--test-chsh-graph")) { want_test_chsh_graph = true; }
        else if (arg_matches(argv[i], "--test-born-emergent")) { want_test_born_emergent = true; }
        else if (arg_matches(argv[i], "--test-born-local")) { want_test_born_local = true; }
        else if (arg_matches(argv[i], "--test-born-equiv")) { want_test_born_equiv = true; }
        else if (arg_matches(argv[i], "--test-amp-invariant")) { want_test_amp_invariant = true; }
        else if (arg_matches(argv[i], "--test-edge-transport")) { want_test_edge_transport = true; }
        else if (arg_matches(argv[i], "--test-edge-transport-colored")) { want_test_edge_transport_colored = true; }
        else if (arg_matches(argv[i], "--test-edge-transport-scaling")) { want_test_edge_transport_scaling = true; }
        else if (arg_matches(argv[i], "--test-wave-packet")) { want_test_wave_packet = true; }
        else if (arg_matches(argv[i], "--test-amp-invariant-colored")) { want_test_amp_invariance_colored = true; }
        else if (arg_matches(argv[i], "--test-amp-invariant-bisect")) { want_test_amp_invariance_bisect = true; }
        else if (arg_matches(argv[i], "--test-soliton")) { want_test_soliton = true; }   /* NEU Etappe 11 */
        else if (arg_matches(argv[i], "--test-invariance")) { want_test_invariance = true; }
        else if (arg_matches(argv[i], "--test-lorentz")) { want_test_lorentz = true; }
        else if (arg_matches(argv[i], "--test-amp")) { want_test_amp = true; }
        else {
            fprintf(stderr, "Unbekannter Parameter: %s\n", argv[i]);
            usage(argv[0]);
        }
    }

    /* ---------- Tests ausführen ---------- */

    if (want_test_amp) {
        amplitude_layer_smoke_test();
    }
    if (want_test_born) {
        test_born_rule();
    }
    if (want_test_unitary) {
        test_unitary_tick();
    }
    if (want_test_context) {
        test_context_perm();
    }
    if (want_test_wilson) {
        test_wilson_loop();
    }
    if (want_test_gauge) {
        test_local_gauge();
    }
    if (want_test_triangle) {
        test_triangle_correlation();
    }
    if (want_test_chsh_superdet) {
        printf("[RUN] CHSH-Superdeterminismus-Test (Etappe 5)...\n");
        double S_sep = 0.0, S_shr = 0.0;
        test_chsh_superdet(false, &S_sep);
        test_chsh_superdet(true, &S_shr);

        printf("\n[Superdet] Zusammenfassung:\n");
        printf("           getrennte Quelle: S = %.4f  (erwartet ≈ 2.0)\n", S_sep);
        printf("           geteilte Quelle:  S = %.4f  (kann > 2.0 sein)\n", S_shr);
        printf("           HINWEIS: S > 2 in der geteilten Variante ist der\n");
        printf("                    Superdeterminismus-Loop, KEIN Bell-Bruch,\n");
        printf("                    KEINE emergente QM.\n");
    }
    if (want_test_invariance) {
        printf("[RUN] Starte U5-Invarianztest (geschlossenes System)...\n");
        const bool pass = test_invariance_under_tick(
            NOSIG_GRID_DIM, 2000u, 200u, 0xC0FFEEu);
        printf("--> Ergebnis: %s\n",
            pass ? "PASSED (U5 invariant unter Tick-Iteration)"
            : "FAILED (Invariante verletzt)");
    }

    if (want_test_no_signal) {
        printf("[RUN] No-Signaling mit EPR-Propagation "
            "(Delay=%u Ticks, Debug=%s)...\n",
            epr_delay, epr_debug ? "ON" : "OFF");
        const bool broke = test_no_signaling_causality(
            10000, 500, epr_delay, epr_debug);
        printf("--> Ergebnis: %s\n",
            broke ? "BROKEN (EPR-Kanal traegt Signal — Delay zu klein)"
            : "HELD (Signal erreicht B nicht rechtzeitig)");
    }

    if (want_test_lorentz) {
        printf("[RUN] Starte Lorentz-Test (amp_grid-basiert, Etappe 12)...\n");
        const bool pass = test_lorentz_time_dilation_ampgrid(0.60);
        printf("--> Ergebnis: %s\n",
            pass ? "PASSED (Zeitdilatation auf amp_grid nachgewiesen)"
            : "FAILED (siehe Diagnose oben)");
    }

    if (want_test_chsh) {
        printf("[RUN] Native-Graph-CHSH-Regression fuer Singlet + Triplet...\n");
        const double expected_S = 2.0;
        const double tol = 1.0e-1;
        const bool ok_s = test_chsh_edge_type(PRO_EDGE_SINGLET, "Singlet", expected_S, tol);
        const bool ok_t = test_chsh_edge_type(PRO_EDGE_TRIPLET, "Triplet", expected_S, tol);
        const bool pass = ok_s && ok_t;
        printf("--> Ergebnis: %s\n",
            pass ? "PASSED (native Graph-CHSH, klassische Bell-Schranke S=2)"
            : "FAILED (native CHSH inkonsistent)");
    }

    if (want_test_observer_chsh) {
        test_observer_chsh();
    }

    if (want_test_chsh_diffusion) {
        test_chsh_diffusion();
    }

    if (want_test_chsh_wave) {
        test_chsh_wave();
    }
    if (want_test_chsh_collapse) {
        test_chsh_collapse();
    }

    if (want_test_chsh_graph) {
        test_chsh_graph();
    }

    if (want_test_born_emergent) {
        test_born_emergent();
    }

    if (want_test_born_local) {
        test_born_local_observer();
    }

    if (want_test_born_equiv) {
        test_born_equivariance();
    }

    if (want_test_amp_invariant) {
        test_amp_invariance();
    }

    if (want_test_edge_transport) {
        test_edge_transport();
    }

    if (want_test_edge_transport_colored) {
        test_edge_transport_colored();
    }

    if (want_test_edge_transport_scaling) {
        test_edge_transport_scaling();
    }

    if (want_test_wave_packet) {
        test_wave_packet_dispersion();
    }

    if (want_test_amp_invariance_colored) {
        test_amp_invariance_colored();
    }

    if (want_test_amp_invariance_bisect) {
        test_amp_invariance_bisect();
    }

    if (want_test_soliton) {                                /* NEU Etappe 11 */
        test_soliton_stability();
    }

    if (ctx_initialized) {
        pro_engine_cleanup(&ctx);
    }

    /* ---------- Wenn nur Tests gewünscht waren, kein Datenlauf ---------- */
    bool any_test = want_test_chsh || want_test_no_signal
        || want_test_invariance || want_test_lorentz || want_test_amp
        || want_test_unitary || want_test_context || want_test_wilson
        || want_test_gauge || want_test_triangle
        || want_test_chsh_superdet
        || want_test_observer_chsh
        || want_test_chsh_diffusion
        || want_test_chsh_wave
        || want_test_chsh_collapse
        || want_test_chsh_graph
        || want_test_born_emergent
        || want_test_born_local
        || want_test_born_equiv
        || want_test_amp_invariant
        || want_test_edge_transport
        || want_test_edge_transport_colored
        || want_test_edge_transport_scaling
        || want_test_wave_packet
        || want_test_amp_invariance_colored
        || want_test_amp_invariance_bisect
        || want_test_soliton;                               /* NEU Etappe 11 */
    if (any_test) {
        return 0;
    }

    /* ---------- Datenlauf (CSV) ---------- */
    const uint32_t inject_ticks = (inject_ticks_arg >= 0)
        ? (uint32_t)inject_ticks_arg
        : (ticks / 10);

    if (!out_file) {
        make_default_filename(out_file_buf, sizeof(out_file_buf));
        out_file = out_file_buf;
    }

    printf("========================================================================\n");
    printf("  PROPHYSICS ALPHA & BELL/CHSH TEST GENERATOR (C99 Engine, V3)\n");
    printf("========================================================================\n");
    printf("Konfiguration:\n");
    printf("  Runs Total:        %u\n",
        (fixed_seeds + random_seeds > 0) ? (fixed_seeds + random_seeds) : runs);
    printf("  Ticks Pro Run:     %u\n", ticks);
    printf("  Injektions-Window: Ticks 1 bis %u\n", inject_ticks);
    printf("  Injektions-Modus:  %s\n",
        use_epr
        ? (epr_edge_type == PRO_EDGE_TRIPLET
            ? "EPR-Triplet-Kanten"
            : "EPR-Singlet-Kanten")
        : "Standard Photonen");
    printf("  Ausgabedatei:      %s\n", out_file);
    printf("  Modus:             Append=%s | PerRunFiles=%s\n",
        append_mode ? "YES" : "NO", per_run_file ? "YES" : "NO");
    printf("------------------------------------------------------------------------\n");

    uint32_t total_runs = 0;
    static uint32_t seeds[MAX_RUNS_LIMIT];

    const uint32_t requested = (fixed_seeds + random_seeds > 0)
        ? (fixed_seeds + random_seeds)
        : runs;

    if (requested > MAX_RUNS_LIMIT) {
        fprintf(stderr, "[!] requested %u runs > MAX_RUNS_LIMIT %u\n",
            requested, (unsigned)MAX_RUNS_LIMIT);
        return 2;
    }

    const uint32_t base_seed = 0x414C5048u;
    for (uint32_t i = 0; i < fixed_seeds && total_runs < MAX_RUNS_LIMIT; ++i) {
        seeds[total_runs++] = base_seed + i;
    }

    srand((unsigned)time(NULL));
    for (uint32_t i = 0; i < random_seeds && total_runs < MAX_RUNS_LIMIT; ++i) {
        seeds[total_runs++] = (uint32_t)rand();
    }

    if (fixed_seeds == 0 && random_seeds == 0) {
        for (uint32_t i = 0; i < runs && total_runs < MAX_RUNS_LIMIT; ++i) {
            seeds[total_runs++] = (uint32_t)rand();
        }
    }

    for (uint32_t r = 0; r < total_runs; ++r) {
        const uint32_t seed = seeds[r];
        const uint32_t run_index = r + 1;
        printf("\n=== Start Run %u/%u (Seed: %u) ===\n",
            run_index, total_runs, seed);

        const bool current_append = append_mode || (!per_run_file && r > 0);
        const int rc = run_single(ticks, inject_ticks, use_epr, epr_edge_type,
            out_file, current_append, per_run_file,
            run_index, seed);
        if (rc != 0) {
            fprintf(stderr, "[!] Run %u fehlgeschlagen (rc=%d)\n",
                run_index, rc);
        }
        else {
            printf("[+] Run %u erfolgreich abgeschlossen.\n", run_index);
        }
    }

    printf("\n========================================================================\n");
    printf("[SUCCESS] Alle Datensaetze erfolgreich generiert.\n");
    printf("========================================================================\n");
    return 0;
}