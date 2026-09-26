#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

/* Etappe 18d-Fix: stdout/stderr unbuffered. */
static void force_unbuffered_io(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

static bool arg_matches(const char* arg, const char* option) {
    return strcmp(arg, option) == 0;
}

static void usage(const char* prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -h, --help            Show this help\n");
    printf("  --log file            Write to Logfile\n");
    printf("  --runs N              Independent runs (default: 3)\n");
    printf("  --ticks N             Ticks per run (default: 5000)\n");
    printf("  --inject-ticks N      Ticks with injection\n");
    printf("  --no-epr              Inject no EPR Photons\n");
    printf("  --epr-pairs           Inject entangled EPR singlet pairs\n");
    printf("  --epr-triplet         Inject entangled EPR triplet pairs\n");
    printf("  --epr-delay N         EPR propagation delay in ticks\n");
    printf("  --epr-debug           Enable EPR propagation event log\n");
    printf("  --test-chsh           Run CHSH regression (native, expected S=2.0)\n");
    printf("  --out FILE            Output CSV\n");
    printf("  --append              Append to existing file\n");
    printf("  --per-run-files       One CSV per run\n");
    printf("  --fixed-seeds N       N runs with deterministic seeds\n");
    printf("  --random-seeds N      N runs with random seeds\n");
    printf("  --test-no-signaling   Run no-signaling causality test\n");
    printf("  --test-invariance     Run U5 invariance test (closed system)\n");
    printf("  --test-lorentz        Run Lorentz / time-dilation test\n");
    printf("  --test-amp            Run amplitude regression test\n");
    printf("  --test-born           Born-Regel-Regression\n");
    printf("  --test-unitary        Unitärer Tick\n");
    printf("  --test-context        Kontextabhängige signed permutations\n");
    printf("  --test-wilson         U(1)-Wilson-Loop\n");
    printf("  --test-gauge          Lokale Eichtransformation\n");
    printf("  --test-triangle       Dreieckskorrelation\n");
    printf("  --test-superdet       CHSH-Superdeterminismus\n");
    printf("  --test-observer-chsh  Beobachter-CHSH\n");
    printf("  --test-chsh-diffusion CHSH chaotisch, Diffusion\n");
    printf("  --test-chsh-wave      CHSH chaotisch, Wellengleichung\n");
    printf("  --test-chsh-collapse  CHSH Kollaps-Messung\n");
    printf("  --test-chsh-graph     CHSH Graph-Messung\n");
    printf("  --test-born-emergent  Born-Emergenz\n");
    printf("  --test-born-local     Born-Emergenz lokal\n");
    printf("  --test-born-equiv     Born-Equivarianz\n");
    printf("  --test-amp-invariant  U5-Invariante auf amp_grid\n");
    printf("  --test-edge-transport         Kanten-Transport sequenziell\n");
    printf("  --test-edge-transport-colored Kanten-Transport farbig\n");
    printf("  --test-edge-transport-scaling Skalierungs-Diagnose\n");
    printf("  --test-wave-packet            Wellenpaket-Dispersion\n");
    printf("  --test-amp-invariant-colored  Etappe-8-Invariante farbig\n");
    printf("  --test-amp-invariant-bisect   U5-Diagnose\n");
    printf("  --test-soliton                Soliton-Stabilitaet\n");
    printf("  --test-qm-basics              QM-Basics: Interferenz, Doppelspalt\n");
    printf("  --test-qm-advanced            QM-Advanced: Born<->N, Lorentz ueber v, Kohaerenz\n");
    printf("  --test-qm-emergent            QM-Emergent: Dispersion, Soliton-Streuung\n");
    printf("  --test-3d-smoke               3D-Smoke: ballistische Ausbreitung\n");
    printf("  --test-3d-invariance          3D-U5-Invariante ueber 2000 Ticks\n");
    printf("  --test-3d-dispersion          3D-Bloch-Dispersion omega(kx,ky,kz)\n");
    printf("  --test-hydrogen               Coulomb/Wasserstoff-Spektrum (Etappe 18)\n");
    printf("  --test-hydrogen-48            Coulomb/Wasserstoff bei dim=64 (Etappe 18d)\n");
    printf("  --test-shared-reference       U4 als shared reference (Etappe 18c)\n");
    printf("  --test-shared-formula-tournament  Formel-Vergleich Klassen-Transport (18e)\n");
    printf("  --test-spin-half               Spin-1/2-Emergenz (Etappe 19)\n");
    printf("  --test-dirac                   Dirac-Struktur (Etappe 21)\n");
    printf("  --test-su2-wilson-loop         SU(2)-Eichfeld (Etappe 22)\n");
    printf("  --test-running-coupling       Running-Coupling / Beta-Funktion (Etappe 23)\n");
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
    if (tm_ptr) strftime(buf, buf_len, "Test_Alpha_%Y%m%d_%H%M%S.csv", tm_ptr);
    else snprintf(buf, buf_len, "Test_Alpha_%" PRId64 ".csv", (int64_t)t);
}

static bool file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    fclose(f);
    return true;
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

int run_single(uint32_t ticks, uint32_t inject_ticks, bool use_epr,
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
            snprintf(filename, sizeof(filename), "%s_run%03u.csv", out_file, run_index);
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
    pu.grid_dim = GRID_DIM;

    const ProU128 inv_baseline = ProPhysics_Measure_Amp_Invariant(&pu);

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
        const ProU128 inv_before = ProPhysics_Measure_Amp_Invariant(&pu);
        const CHSH_Result chsh = compute_chsh_metrics(&pu, rng.s);
        s_window_sum -= s_window[s_window_idx];
        s_window[s_window_idx] = chsh.S_CHSH;
        s_window_sum += chsh.S_CHSH;
        s_window_idx = (s_window_idx + 1) % CHSH_WINDOW;
        if (s_window_count < CHSH_WINDOW) s_window_count++;
        const double s_mean = (s_window_count > 0)
            ? (s_window_sum / (double)s_window_count) : 0.0;
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
                    "(delta=%+.6e, Toleranz=%.6e)\n",
                    tick, run_index, seed, nb, na, na - nb, tolerance);
                ProPhysics_Free(&pu);
                fclose(f);
                return 2;
            }
        }
        track_universe_dynamics(&pu, &tracker, tick);
        uint64_t pos = 0, neg = 0, photon = 0, non_euclidean = 0;
        int64_t  heli_sum = 0;
        collect_advanced_metrics(&pu, &pos, &neg, &photon, &non_euclidean, &heli_sum);
        const double rhoE = (double)(pos + neg + photon) / (double)pu.total_nodes;
        const double alpha_eff = (photon > 0)
            ? (double)(pos + neg) / (double)photon : 0.0;
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
    (void)inv_baseline;
    ProPhysics_Free(&pu);
    fclose(f);
    return 0;
}

static void nosig_init_torus(ProUniverse* pu) {
    if (!pu || !pu->reg_source) return;
    for (uint64_t i = 0; i < pu->total_nodes; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
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
    bool debug)
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
    uint32_t p_b_flip[8] = { 0 }, p_b_noop[8] = { 0 };
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
        if (!snapshot_restore(&pu, &snap)) {
            snapshot_free(&snap);
            ProPhysics_Free(&pu);
            return false;
        }
        snapshot_free(&snap);
        pu.ur_grid[node_a].field_helicity =
            (uint8_t)((pu.ur_grid[node_a].field_helicity + 2) & 0x03u);
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
        const uint8_t hel_flip = pu.ur_grid[node_b].field_helicity & 0x07u;
        p_b_flip[hel_flip]++;
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

int main(int argc, char** argv) {
    for (int i = 1; i < argc - 1; ++i) {
        if (strcmp(argv[i], "--log") == 0) {
            freopen(argv[i + 1], "w", stdout);
            setvbuf(stdout, NULL, _IONBF, 0);
            break;
        }
    }
    force_unbuffered_io();

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

    bool want_test_chsh = false, want_test_no_signal = false,
        want_test_invariance = false, want_test_lorentz = false,
        want_test_amp = false, want_test_born = false,
        want_test_unitary = false, want_test_context = false,
        want_test_wilson = false, want_test_gauge = false,
        want_test_triangle = false, want_test_chsh_superdet = false,
        want_test_observer_chsh = false, want_test_chsh_diffusion = false,
        want_test_chsh_wave = false, want_test_chsh_collapse = false,
        want_test_chsh_graph = false, want_test_born_emergent = false,
        want_test_born_local = false, want_test_born_equiv = false,
        want_test_amp_invariant = false, want_test_edge_transport = false,
        want_test_edge_transport_colored = false,
        want_test_edge_transport_scaling = false,
        want_test_wave_packet = false, want_test_amp_invariance_colored = false,
        want_test_amp_invariance_bisect = false, want_test_soliton = false,
        want_test_qm_basics = false, want_test_qm_advanced = false,
        want_test_qm_emergent = false,
        want_test_3d_smoke = false, want_test_3d_invariance = false,
        want_test_3d_dispersion = false,
        want_test_hydrogen = false,
        want_test_hydrogen_48 = false,
        want_test_shared_reference = false,
        want_test_shared_formula_tournament = false,
        want_test_spin_half = false,
        want_test_dirac = false,
        want_test_su2_wilson_loop = false, want_test_running_coupling = false;

    for (int i = 1; i < argc; ++i) {
        if (arg_matches(argv[i], "-h") || arg_matches(argv[i], "--help")) usage(argv[0]);
        else if (arg_matches(argv[i], "--runs") && i + 1 < argc) runs = (uint32_t)atoi(argv[++i]);
        else if (arg_matches(argv[i], "--ticks") && i + 1 < argc) ticks = (uint32_t)atoi(argv[++i]);
        else if (arg_matches(argv[i], "--inject-ticks") && i + 1 < argc) inject_ticks_arg = atoi(argv[++i]);
        else if (arg_matches(argv[i], "--no-epr")) use_epr = false;
        else if (arg_matches(argv[i], "--epr-pairs")) { use_epr = true; epr_edge_type = PRO_EDGE_SINGLET; }
        else if (arg_matches(argv[i], "--epr-triplet")) { use_epr = true; epr_edge_type = PRO_EDGE_TRIPLET; }
        else if (arg_matches(argv[i], "--epr-delay") && i + 1 < argc) epr_delay = (uint32_t)atoi(argv[++i]);
        else if (arg_matches(argv[i], "--epr-debug")) epr_debug = true;
        else if (arg_matches(argv[i], "--out") && i + 1 < argc) {
            snprintf(out_file_buf, sizeof(out_file_buf), "%s", argv[++i]);
            out_file = out_file_buf;
        }
        else if (arg_matches(argv[i], "--append")) append_mode = true;
        else if (arg_matches(argv[i], "--per-run-files")) per_run_file = true;
        else if (arg_matches(argv[i], "--fixed-seeds") && i + 1 < argc) fixed_seeds = (uint32_t)atoi(argv[++i]);
        else if (arg_matches(argv[i], "--random-seeds") && i + 1 < argc) random_seeds = (uint32_t)atoi(argv[++i]);
        else if (arg_matches(argv[i], "--test-chsh")) want_test_chsh = true;
        else if (arg_matches(argv[i], "--test-no-signaling")) want_test_no_signal = true;
        else if (arg_matches(argv[i], "--test-invariance")) want_test_invariance = true;
        else if (arg_matches(argv[i], "--test-amp")) want_test_amp = true;
        else if (arg_matches(argv[i], "--test-born")) want_test_born = true;
        else if (arg_matches(argv[i], "--test-unitary")) want_test_unitary = true;
        else if (arg_matches(argv[i], "--test-context")) want_test_context = true;
        else if (arg_matches(argv[i], "--test-wilson")) want_test_wilson = true;
        else if (arg_matches(argv[i], "--test-gauge")) want_test_gauge = true;
        else if (arg_matches(argv[i], "--test-triangle")) want_test_triangle = true;
        else if (arg_matches(argv[i], "--test-superdet")) want_test_chsh_superdet = true;
        else if (arg_matches(argv[i], "--test-observer-chsh")) want_test_observer_chsh = true;
        else if (arg_matches(argv[i], "--test-chsh-diffusion")) want_test_chsh_diffusion = true;
        else if (arg_matches(argv[i], "--test-chsh-wave")) want_test_chsh_wave = true;
        else if (arg_matches(argv[i], "--test-chsh-collapse")) want_test_chsh_collapse = true;
        else if (arg_matches(argv[i], "--test-chsh-graph")) want_test_chsh_graph = true;
        else if (arg_matches(argv[i], "--test-born-emergent")) want_test_born_emergent = true;
        else if (arg_matches(argv[i], "--test-born-local")) want_test_born_local = true;
        else if (arg_matches(argv[i], "--test-born-equiv")) want_test_born_equiv = true;
        else if (arg_matches(argv[i], "--test-amp-invariant")) want_test_amp_invariant = true;
        else if (arg_matches(argv[i], "--test-edge-transport")) want_test_edge_transport = true;
        else if (arg_matches(argv[i], "--test-edge-transport-colored")) want_test_edge_transport_colored = true;
        else if (arg_matches(argv[i], "--test-edge-transport-scaling")) want_test_edge_transport_scaling = true;
        else if (arg_matches(argv[i], "--test-wave-packet")) want_test_wave_packet = true;
        else if (arg_matches(argv[i], "--test-amp-invariant-colored")) want_test_amp_invariance_colored = true;
        else if (arg_matches(argv[i], "--test-amp-invariant-bisect")) want_test_amp_invariance_bisect = true;
        else if (arg_matches(argv[i], "--test-soliton")) want_test_soliton = true;
        else if (arg_matches(argv[i], "--test-lorentz")) want_test_lorentz = true;
        else if (arg_matches(argv[i], "--test-qm-basics")) want_test_qm_basics = true;
        else if (arg_matches(argv[i], "--test-qm-advanced")) want_test_qm_advanced = true;
        else if (arg_matches(argv[i], "--test-qm-emergent")) want_test_qm_emergent = true;
        else if (arg_matches(argv[i], "--test-3d-smoke")) want_test_3d_smoke = true;
        else if (arg_matches(argv[i], "--test-3d-invariance")) want_test_3d_invariance = true;
        else if (arg_matches(argv[i], "--test-3d-dispersion")) want_test_3d_dispersion = true;
        else if (arg_matches(argv[i], "--test-hydrogen")) want_test_hydrogen = true;
        else if (arg_matches(argv[i], "--test-hydrogen-48")) want_test_hydrogen_48 = true;
        else if (arg_matches(argv[i], "--test-shared-reference")) want_test_shared_reference = true;
        else if (arg_matches(argv[i], "--test-shared-formula-tournament")) want_test_shared_formula_tournament = true;
        else if (arg_matches(argv[i], "--test-spin-half")) want_test_spin_half = true;
        else if (arg_matches(argv[i], "--test-dirac")) want_test_dirac = true;
        else if (arg_matches(argv[i], "--test-su2-wilson-loop")) want_test_su2_wilson_loop = true;
        else if (arg_matches(argv[i], "--test-running-coupling")) want_test_running_coupling = true;

        else if (arg_matches(argv[i], "--log") && i + 1 < argc) { i++; /* Wert in Pre-Pass bereits konsumiert */ }
        else { fprintf(stderr, "Unbekannter Parameter: %s\n", argv[i]); usage(argv[0]); }
    }

    if (want_test_amp) amplitude_layer_smoke_test();
    if (want_test_born) test_born_rule();
    if (want_test_unitary) test_unitary_tick();
    if (want_test_context) test_context_perm();
    if (want_test_wilson) test_wilson_loop();
    if (want_test_gauge) test_local_gauge();
    if (want_test_triangle) test_triangle_correlation();
    if (want_test_chsh_superdet) {
        printf("[RUN] CHSH-Superdeterminismus-Test (Etappe 5)...\n");
        double S_sep = 0.0, S_shr = 0.0;
        test_chsh_superdet(false, &S_sep);
        test_chsh_superdet(true, &S_shr);
        printf("\n[Superdet] Zusammenfassung:\n");
        printf("           getrennte Quelle: S = %.4f\n", S_sep);
        printf("           geteilte Quelle:  S = %.4f\n", S_shr);
    }
    if (want_test_invariance) {
        printf("[RUN] Starte U5-Invarianztest (geschlossenes System)...\n");
        const bool pass = test_amp_invariance_under_tick(NOSIG_GRID_DIM, 2000u, 200u, 0xC0FFEEu);
        printf("--> Ergebnis: %s\n", pass ? "PASSED" : "FAILED");
    }
    if (want_test_no_signal) {
        const uint32_t sig_delay = (epr_delay > 0u) ? epr_delay : 5u;
        printf("[RUN] No-Signaling mit EPR-Propagation (Delay=%u, Debug=%s)...\n",
            sig_delay, epr_debug ? "ON" : "OFF");
        const bool broke = test_no_signaling_causality(10000, 500, sig_delay, epr_debug);
        printf("--> Ergebnis: %s\n", broke ? "BROKEN" : "HELD");
    }
    if (want_test_lorentz) {
        printf("[RUN] Starte Lorentz-Test (amp_grid-basiert, Etappe 12)...\n");
        const bool pass = test_lorentz_time_dilation_ampgrid(0.60);
        printf("--> Ergebnis: %s\n", pass ? "PASSED" : "FAILED");
    }
    if (want_test_chsh) {
        printf("[RUN] Native-Graph-CHSH-Regression fuer Singlet + Triplet...\n");
        const double expected_S = 2.0, tol = 1.0e-1;
        const bool ok_s = test_chsh_edge_type(PRO_EDGE_SINGLET, "Singlet", expected_S, tol);
        const bool ok_t = test_chsh_edge_type(PRO_EDGE_TRIPLET, "Triplet", expected_S, tol);
        printf("--> Ergebnis: %s\n", (ok_s && ok_t) ? "PASSED" : "FAILED");
    }
    if (want_test_observer_chsh) test_observer_chsh();
    if (want_test_chsh_diffusion) test_chsh_diffusion();
    if (want_test_chsh_wave) test_chsh_wave();
    if (want_test_chsh_collapse) test_chsh_collapse();
    if (want_test_chsh_graph) test_chsh_graph();
    if (want_test_born_emergent) test_born_emergent();
    if (want_test_born_local) test_born_local_observer();
    if (want_test_born_equiv) test_born_equivariance();
    if (want_test_amp_invariant) test_amp_invariance();
    if (want_test_edge_transport) test_edge_transport();
    if (want_test_edge_transport_colored) test_edge_transport_colored();
    if (want_test_edge_transport_scaling) test_edge_transport_scaling();
    if (want_test_wave_packet) test_wave_packet_dispersion();
    if (want_test_amp_invariance_colored) test_amp_invariance_colored();
    if (want_test_amp_invariance_bisect) test_amp_invariance_bisect();
    if (want_test_soliton) test_soliton_stability();
    if (want_test_qm_basics) test_qm_basics_all();
    if (want_test_qm_advanced) test_qm_advanced_all();
    if (want_test_qm_emergent) test_qm_emergent_all();

    if (want_test_3d_smoke)      test_3d_smoke_ballistic();
    if (want_test_3d_invariance) test_3d_invariance_under_tick(32u, 2000u, 0xC0FFEEu);
    if (want_test_3d_dispersion) test_3d_dispersion();

    if (want_test_hydrogen)      test_hydrogen_spectrum();
    if (want_test_hydrogen_48)   test_hydrogen_spectrum_48();

    if (want_test_shared_reference) test_shared_reference();
    if (want_test_shared_formula_tournament) test_shared_formula_tournament();

    if (want_test_spin_half) test_spin_half_emergence();
    if (want_test_dirac) test_dirac_all();

    if (want_test_su2_wilson_loop) test_su2_wilson_loop();
    if (want_test_running_coupling) test_running_coupling();

    bool any_test = want_test_chsh || want_test_no_signal || want_test_invariance
        || want_test_lorentz || want_test_amp || want_test_born || want_test_unitary
        || want_test_context || want_test_wilson || want_test_gauge
        || want_test_triangle || want_test_chsh_superdet
        || want_test_observer_chsh || want_test_chsh_diffusion
        || want_test_chsh_wave || want_test_chsh_collapse || want_test_chsh_graph
        || want_test_born_emergent || want_test_born_local || want_test_born_equiv
        || want_test_amp_invariant || want_test_edge_transport
        || want_test_edge_transport_colored || want_test_edge_transport_scaling
        || want_test_wave_packet || want_test_amp_invariance_colored
        || want_test_amp_invariance_bisect || want_test_soliton
        || want_test_qm_basics || want_test_qm_advanced
        || want_test_qm_emergent
        || want_test_3d_smoke || want_test_3d_invariance || want_test_3d_dispersion
        || want_test_hydrogen || want_test_hydrogen_48
        || want_test_shared_reference
        || want_test_shared_formula_tournament
        || want_test_spin_half || want_test_dirac
        || want_test_su2_wilson_loop || want_test_running_coupling;
    if (any_test) return 0;

    const uint32_t inject_ticks = (inject_ticks_arg >= 0) ? (uint32_t)inject_ticks_arg : (ticks / 10);
    if (!out_file) { make_default_filename(out_file_buf, sizeof(out_file_buf)); out_file = out_file_buf; }
    printf("========================================================================\n");
    printf("  PROPHYSICS ALPHA & BELL/CHSH TEST GENERATOR (C99 Engine, V3)\n");
    printf("========================================================================\n");
    printf("Konfiguration:\n");
    printf("  Runs Total:        %u\n", (fixed_seeds + random_seeds > 0) ? (fixed_seeds + random_seeds) : runs);
    printf("  Ticks Pro Run:     %u\n", ticks);
    printf("  Injektions-Window: Ticks 1 bis %u\n", inject_ticks);
    printf("  Injektions-Modus:  %s\n",
        use_epr ? (epr_edge_type == PRO_EDGE_TRIPLET ? "EPR-Triplet-Kanten" : "EPR-Singlet-Kanten")
        : "Standard Photonen");
    printf("  Ausgabedatei:      %s\n", out_file);
    printf("  Modus:             Append=%s | PerRunFiles=%s\n",
        append_mode ? "YES" : "NO", per_run_file ? "YES" : "NO");
    printf("------------------------------------------------------------------------\n");

    uint32_t total_runs = 0;
    static uint32_t seeds[MAX_RUNS_LIMIT];
    const uint32_t requested = (fixed_seeds + random_seeds > 0) ? (fixed_seeds + random_seeds) : runs;
    if (requested > MAX_RUNS_LIMIT) {
        fprintf(stderr, "[!] requested %u runs > MAX_RUNS_LIMIT %u\n", requested, (unsigned)MAX_RUNS_LIMIT);
        return 2;
    }
    const uint32_t base_seed = 0x414C5048u;
    for (uint32_t i = 0; i < fixed_seeds && total_runs < MAX_RUNS_LIMIT; ++i)
        seeds[total_runs++] = base_seed + i;
    srand((unsigned)time(NULL));
    for (uint32_t i = 0; i < random_seeds && total_runs < MAX_RUNS_LIMIT; ++i)
        seeds[total_runs++] = (uint32_t)rand();
    if (fixed_seeds == 0 && random_seeds == 0)
        for (uint32_t i = 0; i < runs && total_runs < MAX_RUNS_LIMIT; ++i)
            seeds[total_runs++] = (uint32_t)rand();

    for (uint32_t r = 0; r < total_runs; ++r) {
        const uint32_t seed = seeds[r];
        const uint32_t run_index = r + 1;
        printf("\n=== Start Run %u/%u (Seed: %u) ===\n", run_index, total_runs, seed);
        const bool current_append = append_mode || (!per_run_file && r > 0);
        const int rc = run_single(ticks, inject_ticks, use_epr, epr_edge_type,
            out_file, current_append, per_run_file, run_index, seed);
        if (rc != 0) fprintf(stderr, "[!] Run %u fehlgeschlagen (rc=%d)\n", run_index, rc);
        else printf("[+] Run %u erfolgreich abgeschlossen.\n", run_index);
    }
    printf("\n========================================================================\n");
    printf("[SUCCESS] Alle Datensaetze erfolgreich generiert.\n");
    printf("========================================================================\n");
    return 0;
}