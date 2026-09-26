/* ==========================================================================
 * ProPhysics - Alpha-Test Common Header
 * File: alpha_test_common.h
 * Version: 3.1 (Etappe 22 + Refactoring)
 *
 * Refactoring 22-Aenderungen:
 *   - Header-Kommentar auf v3.1 aktualisiert.
 *   - Neuer Testprototyp test_su2_wilson_loop (Etappe 22).
 *   - Der Callback-Typ im Test-Harness ist jetzt der zentrale
 *     ProPhysics_RuleCallback aus ProPhysics.h. Bestehende Aufrufe
 *     mit ProPhysics_ScientificRuleCallback funktionieren ueber den
 *     Alias in pro_sdk_interface.h weiter.
 *
 * Etappe 22-Aenderungen:
 *   - test_su2_wilson_loop Prototyp.
 *
 * Enthaelt:
 *   - Kern-Typedefs (DynamicTracker, prng_state_t, EngineNodeAlias,
 *     ProEngineContext, CHSH_Result, FullStateSnapshot).
 *   - Utility-Funktionsprototypen (Engine, Torus, Injektoren, Snapshot).
 *   - Alle Test-Funktionsprototypen aus alpha_test_*.c.
 * ========================================================================== */

#ifndef ALPHA_TEST_COMMON_H
#define ALPHA_TEST_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "ProPhysics.h"
#include "pro_sdk_interface.h"

 /* ==========================================================================
  * Grid-Konfiguration fuer die Tests
  * ========================================================================== */

#define GRID_DIM            1000
#define TOTAL_NODES         ((uint64_t)GRID_DIM * (uint64_t)GRID_DIM)
#define NOSIG_GRID_DIM      64
#define NOSIG_NODES         ((uint64_t)NOSIG_GRID_DIM * (uint64_t)NOSIG_GRID_DIM)
#define MAX_RUNS_LIMIT      10000

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

  /* Q15 -> Q31 Konvertierung (Q31 = int64, Re/Im je int32).
   * Shift um 16 Bit, weil Q15 eine Skala von 2^15, Q31 von 2^31 hat. */
#define Q15TO31(x) ((int32_t)((int64_t)(x) << 16))
#define Q31_MAXV   2147483647.0
#define Q31_MINV   (-2147483648.0)

   /* ==========================================================================
    * Typedefs
    * ========================================================================== */

typedef struct {
    uint32_t prev_reserved_gating;
    uint32_t prev_entropy_index;
    uint64_t prev_channels[4];
    uint32_t flip_accumulator;
    uint32_t sample_window_ticks;
    float    grid_frequency_hz;
} DynamicTracker;

typedef struct { uint64_t s[4]; } prng_state_t;

typedef struct { uint8_t state; } EngineNodeAlias;

typedef struct {
    ProUniverse pu;
    prng_state_t rng;
    EngineNodeAlias* nodes;
    DynamicTracker tracker;
    bool ready;
} ProEngineContext;

typedef struct {
    double E_ab, E_ab_prime, E_a_prime_b, E_a_prime_b_prime, S_CHSH;
} CHSH_Result;

typedef struct {
    ProNode* grid;
    ProRegister* regs;
    ProRegister* regs_target;
    ProEdge* edges;
    uint64_t     total_nodes;
    uint64_t     cpu_tick;
    uint32_t     entropy;
} FullStateSnapshot;

/* ==========================================================================
 * Utility-Funktionen (alpha_test_common.c)
 * ========================================================================== */

void dynamic_tracker_init(DynamicTracker* tracker, uint32_t sample_window);
void track_node_dynamics(DynamicTracker* tracker,
    uint32_t current_gating,
    uint32_t current_entropy,
    const uint64_t current_channels[CHANNELS_MAX]);
void evaluate_grid_frequency(DynamicTracker* tracker, uint32_t current_tick);
void track_universe_dynamics(ProUniverse* pu, DynamicTracker* tracker, uint32_t tick);

uint64_t rotl64(uint64_t x, int k);
uint64_t prng_next(prng_state_t* state);
void     prng_seed(prng_state_t* state, uint64_t seed);
uint64_t fast_map_index(uint64_t random_val, uint64_t max_nodes);

void pro_engine_init(ProEngineContext* ctx);
void pro_engine_cleanup(ProEngineContext* ctx);
void pro_engine_reset(ProEngineContext* ctx);
void pro_engine_tick(ProEngineContext* ctx);

void init_torus(ProUniverse* pu);
void wire_torus(ProUniverse* pu, uint32_t dim);

void inject_photon_deterministic(ProUniverse* pu, prng_state_t* rng);
void inject_epr_pair_deterministic(ProUniverse* pu, prng_state_t* rng,
    uint8_t edge_type);

bool snapshot_capture(const ProUniverse* pu, FullStateSnapshot* snap);
bool snapshot_restore(ProUniverse* pu, const FullStateSnapshot* snap);
void snapshot_free(FullStateSnapshot* snap);

void wire_torus_3d(ProUniverse* pu, uint32_t dim);

int run_single(uint32_t ticks, uint32_t inject_ticks, bool use_epr,
    uint8_t epr_edge_type,
    const char* out_file, bool append_mode,
    bool per_run_file, uint32_t run_index, uint32_t seed);

CHSH_Result compute_chsh_metrics(ProUniverse* pu, uint64_t rng[4]);
double evaluate_correlation(const ProUniverse* pu,
    double theta_a, double theta_b,
    uint64_t rng[4]);

/* ==========================================================================
 * Test-Prototypen
 *
 * Gruppiert nach Prio. Jede Funktion liefert true (PASS) oder false
 * (FAIL). Der Test-Harness (alpha_test_main.c) dispatched anhand der
 * CLI-Flags.
 * ========================================================================== */

 /* --- Prio 1: 2D-Basis --- */
bool test_born_rule(void);
bool test_unitary_tick(void);
bool test_context_perm(void);
bool test_wilson_loop(void);
bool test_local_gauge(void);
bool test_triangle_correlation(void);
void amplitude_layer_smoke_test(void);

bool test_chsh_edge_type(ProEdgeType edge_type, const char* label,
    double expected_S, double tolerance);
bool test_chsh_collapse(void);
bool test_chsh_graph(void);

/* --- Prio 2: Emergenz --- */
bool test_chsh_superdet(bool shared_source, double* out_S);
bool test_observer_chsh(void);
bool test_chsh_diffusion(void);
bool test_chsh_wave(void);
bool test_born_emergent(void);
bool test_born_local_observer_g(void);
bool test_born_local_observer(void);
bool test_born_equivariance(void);

bool test_qm_basics_all(void);
bool test_two_source_interference(void);
bool test_double_slit(void);

bool test_qm_advanced_all(void);
bool test_born_small_n(void);
bool test_lorentz_multi_v(void);
bool test_phase_flip_coherence(void);
bool test_double_slit_phase_plate(void);

bool test_qm_emergent_all(void);
bool test_dispersion_relation(void);
bool test_two_soliton_scattering(void);

/* --- Prio 3: Langlauf --- */
bool test_amp_invariance_under_tick(uint32_t grid_dim,
    uint32_t ticks,
    uint32_t initial_photons,
    uint32_t seed);
bool test_amp_invariance(void);
bool test_amp_invariance_colored(void);
bool test_amp_invariance_bisect(void);
bool test_edge_transport(void);
bool test_edge_transport_colored(void);
void test_edge_transport_scaling(void);
bool test_wave_packet_dispersion(void);

bool test_soliton_stability(void);
bool test_lorentz_time_dilation_ampgrid(double v_c_ratio);

/* --- Prio 4: 3D-Torus --- */
bool test_3d_smoke_ballistic(void);
bool test_3d_invariance_under_tick(uint32_t dim, uint32_t ticks, uint32_t seed);
bool test_3d_dispersion(void);

/* --- Prio 5: Hydrogen + Shared-Ref + Tournament --- */
bool test_hydrogen_spectrum(void);
bool test_hydrogen_spectrum_48(void);

bool test_shared_reference(void);
bool test_shared_formula_tournament(void);

/* --- Prio 6: Spin-1/2 (Etappe 19) --- */
bool test_spin_half_emergence(void);

/* --- Prio 7: Dirac (Etappe 21) --- */
bool test_dirac_all(void);
bool test_dirac_dispersion_entry(void);

/* --- Prio 8: SU(2)-Eichfeld (Etappe 22) --- */
bool test_su2_wilson_loop(void);
/* --- Prio 8-Erweiterung: Renormierung / Running Coupling (Etappe 23) --- */
bool test_running_coupling(void);
#endif /* ALPHA_TEST_COMMON_H */