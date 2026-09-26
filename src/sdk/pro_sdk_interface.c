/* ==========================================================================
 * ProPhysics - SDK Interface (Plugin + CLI-Runner)
 * File: pro_sdk_interface.c
 * Version: 3.1 (Etappe 22 + Refactoring)
 *
 *
 * Refactoring 22-Aenderungen:
 *   - ProPhysics_SDK_Execute_Plastizitaet_Tick ist jetzt ein duenner
 *     Wrapper um ProPhysics_Tick (Kernel-intern).
 *   - Der gesamte Tick-Body (Advance_Internal_Clocks, EPR-Propagation,
 *     Apply_Amp_Step, Apply_Guiding_Equation, Graph-Plastizitaet,
 *     reg_target-Swap) ist nach ProPhysics_Core.c verschoben.
 *   - Der Callback-Typ ist jetzt ProPhysics_RuleCallback aus
 *     ProPhysics.h. Die alte inline-Signatur bleibt kompatibel.
 *   - Der SDK-Wrapper wurde nur beibehalten, damit bestehende
 *     Aufrufer (Test-Harness, pro_engine_tick) nicht angepasst
 *     werden muessen.
 *
 * Verbleibende SDK-Aufgaben (Test/Demo, NICHT Kernel):
 *   - ResearchPlugin_DynamicPlasticTopology: Beispiel-Callback
 *   - Export_Universe_To_BMP, Render_ASCII_Viewport
 *   - Calculate_Topological_Metrics, Measure_Energy_and_Coupling
 *   - Apply_Scenario_Injections, CLI-Runner (main)
 *
 * Kern-API wird ueber ProPhysics.h eingebunden. Diese Datei definiert
 * KEINE neuen Kernel-Funktionen.
 * ========================================================================== */

 /* PRO_SDK_EXPORTS muss VOR dem Header-Include gesetzt sein, damit
  * PRO_SDK_API zu __declspec(dllexport) expandiert. Sonst meldet MSVC
  * C2375 "Neudefinition; unterschiedliche Bindung". */
#define PRO_SDK_EXPORTS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "ProPhysics.h"
#include "pro_sdk_interface.h"

#define GRID_DIM 1000

  /* =========================================================================
   * 0. REGION: KERNEL-TICK-WRAPPER
   *
   * Der einzige Zweck dieses Wrappers ist, die bestehende SDK-Signatur
   * zu erhalten. Die gesamte Tick-Logik lebt in ProPhysics_Tick
   * (ProPhysics_Core.c).
   *
   * R7: Bei callback == NULL verhaelt sich der Wrapper wie der direkte
   * Kernel-Aufruf -- kein Graph-Update, aber die volle U1-U6-Dynamik.
   * ========================================================================= */

PRO_SDK_API void ProPhysics_SDK_Execute_Plastizitaet_Tick(
    ProUniverse* pu, ProPhysics_ScientificRuleCallback callback)
{
    /* 1:1-Delegation an den Kernel-Tick.
     * Der Callback-Typ ist strukturell identisch zu ProPhysics_RuleCallback
     * (siehe ProPhysics_Config.h und ProPhysics.h); der Cast ist ein
     * reiner Typhauch, kein ABI-Unterschied. */
    ProPhysics_Tick(pu, (ProPhysics_RuleCallback)callback);
}

/* =========================================================================
 * 1. REGION: RESEARCHER PLAYGROUND
 *
 * Beispiel-Callback. Der Kernel ruft ihn pro Knoten mit nicht-NEUTRAL
 * type_state auf. Dieser konkrete Callback macht eine einfache
 * Topologie-Permutation, um Plastizitaet zu demonstrieren.
 * ========================================================================= */

PRO_SDK_API void ResearchPlugin_DynamicPlasticTopology(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state,
    uint64_t current_idx, uint64_t total_nodes)
{
    /* Etappe 8: type_state-Uebergaenge entfernt. U1-U5 wirken auf amp_grid
     * (ProPhysics_Apply_Amp_Step), nicht mehr auf type_state.
     * Dieser Callback schlaegt nur noch Topologie-Aenderungen vor. */
    (void)out_next_state;
    (void)out_next_target_state;
    (void)current_idx;
    (void)total_nodes;

    if (current_state != UR_NEUTRAL && target_state != UR_NEUTRAL) {
        const uint64_t tmp = current_channels[0];
        current_channels[0] = target_channels[1];
        target_channels[1] = tmp;
    }
}

/* =========================================================================
 * 2. REGION: VISUALISIERUNG
 *
 * BMP-Export und ASCII-Viewport. Test-/Demo-spezifisch, nicht Kernel.
 * ========================================================================= */

static void Export_Universe_To_BMP(ProUniverse* pu, const char* filename)
{
    if (!pu || !pu->ur_grid) return;

    uint32_t width = GRID_DIM;
    uint32_t height = GRID_DIM;
    uint32_t row_size = (width * 3 + 3) & ~3;
    uint32_t image_size = row_size * height;

    uint8_t header[54] = {
        'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0,
        54, 0, 0, 0, 40, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 0, 24, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    uint32_t file_size = 54 + image_size;
    memcpy(&header[2], &file_size, 4);
    memcpy(&header[18], &width, 4);
    memcpy(&header[22], &height, 4);
    memcpy(&header[34], &image_size, 4);

    FILE* f = fopen(filename, "wb");
    if (!f) { printf("[!] BMP-Fehler: %s\n", filename); return; }
    fwrite(header, 1, 54, f);

    uint8_t* row_buf = (uint8_t*)calloc(1, row_size);
    if (!row_buf) { fclose(f); return; }

    for (int32_t y = height - 1; y >= 0; y--) {
        for (uint32_t x = 0; x < width; x++) {
            uint64_t idx = (uint64_t)y * width + x;
            uint8_t st = pu->ur_grid[idx].type_state;
            uint8_t r = 10, g = 10, b = 15;
            if (st == 0x01U) { r = 255; g = 50;  b = 50; }
            else if (st == 0x03U) { r = 50;  g = 50;  b = 255; }
            else if (st == 0x05U) { r = 255; g = 255; b = 255; }
            else if (st == 0x02U) { r = 255; g = 200; b = 0; }
            else if (st == 0x04U) { r = 0;   g = 255; b = 100; }
            row_buf[x * 3 + 0] = b;
            row_buf[x * 3 + 1] = g;
            row_buf[x * 3 + 2] = r;
        }
        fwrite(row_buf, 1, row_size, f);
    }
    free(row_buf);
    fclose(f);
}

static void Render_ASCII_Viewport(ProUniverse* pu)
{
    if (!pu || !pu->ur_grid) return;
    uint32_t size = 40;
    uint32_t start_x = (GRID_DIM / 2) - (size / 2);
    uint32_t start_y = (GRID_DIM / 2) - (size / 2);
    printf("\n--- ASCII-GITTERSCHNITT (Mitte: %ux%u) ---\n", size, size);
    for (uint32_t y = start_y; y < start_y + size; y++) {
        printf(" ");
        for (uint32_t x = start_x; x < start_x + size; x++) {
            uint64_t idx = y * GRID_DIM + x;
            uint8_t st = pu->ur_grid[idx].type_state;
            char symbol = '.';
            if (st == 0x01U) symbol = '+';
            else if (st == 0x03U) symbol = '-';
            else if (st == 0x05U) symbol = '*';
            else if (st == 0x02U) symbol = '^';
            else if (st == 0x04U) symbol = 'v';
            printf("%c ", symbol);
        }
        printf("\n");
    }
}

static void Calculate_Topological_Metrics(ProUniverse* pu)
{
    if (!pu || !pu->reg_source) return;
    uint64_t non_euclidean_links = 0;
    uint64_t total_links = pu->total_nodes * 4;
    for (uint64_t y = 0; y < GRID_DIM; y++) {
        for (uint64_t x = 0; x < GRID_DIM; x++) {
            uint64_t idx = y * GRID_DIM + x;
            uint64_t expected_north =
                ((y == 0 ? GRID_DIM - 1 : y - 1) * GRID_DIM) + x;
            uint64_t expected_south =
                ((y == GRID_DIM - 1 ? 0 : y + 1) * GRID_DIM) + x;
            if (pu->reg_source[idx].channels[0] != expected_north)
                non_euclidean_links++;
            if (pu->reg_source[idx].channels[1] != expected_south)
                non_euclidean_links++;
        }
    }
    double plasticity_ratio =
        ((double)non_euclidean_links / (double)total_links) * 100.0;
    printf("  [Topologische Metrik]\n");
    printf("    -> Nicht-euklidische Kanaele: %" PRIu64 " (%.2f%%)\n",
        non_euclidean_links, plasticity_ratio);
}

static void Measure_Energy_and_Coupling(ProUniverse* pu, uint32_t tick)
{
    if (!pu || !pu->ur_grid) return;
    uint64_t total = pu->total_nodes;
    uint64_t non_neutral = 0, count_pos = 0, count_neg = 0, count_photon = 0;
    for (uint64_t i = 0; i < total; i++) {
        uint8_t st = pu->ur_grid[i].type_state;
        if (st != UR_NEUTRAL) {
            non_neutral++;
            if (st == UR_POSITRON_CW || st == UR_POSITRON_CCW) count_pos++;
            else if (st == UR_NEGATRON_CW || st == UR_NEGATRON_CCW) count_neg++;
            else if (st == UR_PHOTON) count_photon++;
        }
    }
    double rho_E = (double)non_neutral / (double)total;
    double coupling = (count_photon > 0)
        ? (double)(count_pos + count_neg) / (double)count_photon
        : 0.0;
    printf("[METRIC] Tick %4u | rho_E = %.6f | alpha_eff ~ %.6f | "
        "pos=%" PRIu64 " neg=%" PRIu64 " prot=%" PRIu64
        " | interactions=%u\n",
        tick, rho_E, coupling, count_pos, count_neg, count_photon,
        pu->global_entropy_index);
}

/* =========================================================================
 * 3. REGION: INJEKTOR & SCENARIO
 *
 * Szenario-Injektionen. Test-/Demo-spezifisch.
 * ========================================================================= */

typedef struct {
    bool     pulse_active;
    uint32_t pulse_x, pulse_y;
    uint8_t  pulse_type;
    uint32_t pulse_length;
    uint32_t pulse_interval;
    uint32_t pulse_repeats;

    bool     wall_active;
    uint32_t wall_x, wall_y;
    uint32_t wall_w, wall_h;
    uint32_t wall_growth_rate;

    uint32_t bmp_interval;
    uint32_t total_ticks;
    bool     export_final_bmp;
} SimulationScenario;

static SimulationScenario g_config = {
    .pulse_active = false,
    .pulse_x = 100, .pulse_y = 500,
    .pulse_type = 0x05U,
    .pulse_length = 5,
    .pulse_interval = 10,
    .pulse_repeats = 3,

    .wall_active = false,
    .wall_x = 500, .wall_y = 200,
    .wall_w = 20,  .wall_h = 600,
    .wall_growth_rate = 0,

    .bmp_interval = 0,
    .total_ticks = 30,
    .export_final_bmp = false
};

static void Apply_Scenario_Injections(ProUniverse* pu, uint32_t current_tick)
{
    if (!pu || !pu->ur_grid) return;

    if (g_config.pulse_active && g_config.pulse_repeats > 0) {
        uint32_t cycle_length = g_config.pulse_length + g_config.pulse_interval;
        uint32_t current_cycle = (current_tick - 1) / cycle_length;
        uint32_t tick_in_cycle = (current_tick - 1) % cycle_length;
        if (current_cycle < g_config.pulse_repeats
            && tick_in_cycle < g_config.pulse_length) {
            if (g_config.pulse_x < GRID_DIM && g_config.pulse_y < GRID_DIM) {
                uint64_t idx = g_config.pulse_y * GRID_DIM + g_config.pulse_x;
                pu->ur_grid[idx].type_state = g_config.pulse_type;
            }
        }
    }

    if (g_config.wall_active) {
        uint32_t current_h = g_config.wall_h;
        if (g_config.wall_growth_rate > 0) {
            current_h += (current_tick / g_config.wall_growth_rate) * 2;
        }
        for (uint32_t dy = 0; dy < current_h; dy++) {
            for (uint32_t dx = 0; dx < g_config.wall_w; dx++) {
                uint32_t wx = g_config.wall_x + dx;
                uint32_t wy = g_config.wall_y + dy;
                if (wx < GRID_DIM && wy < GRID_DIM) {
                    uint64_t idx = wy * GRID_DIM + wx;
                    pu->ur_grid[idx].type_state = 0x00U;
                }
            }
        }
    }
}

/* =========================================================================
 * 4. REGION: CLI-PARSER & MAIN
 * ========================================================================= */

static void Parse_CLI_Args(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--bmp") == 0) {
            g_config.export_final_bmp = true;
        }
        else if (strcmp(argv[i], "--ticks") == 0 && i + 1 < argc) {
            g_config.total_ticks = (uint32_t)atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--bmp-interval") == 0 && i + 1 < argc) {
            g_config.bmp_interval = (uint32_t)atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--pulse") == 0 && i + 1 < argc) {
            g_config.pulse_active = true;
            uint32_t raw_type = 5;
            sscanf(argv[++i], "%u,%u,%i,%u,%u,%u",
                &g_config.pulse_x, &g_config.pulse_y, &raw_type,
                &g_config.pulse_length, &g_config.pulse_interval,
                &g_config.pulse_repeats);
            g_config.pulse_type = (uint8_t)raw_type;
        }
        else if (strcmp(argv[i], "--wall") == 0 && i + 1 < argc) {
            g_config.wall_active = true;
            sscanf(argv[++i], "%u,%u,%u,%u",
                &g_config.wall_x, &g_config.wall_y,
                &g_config.wall_w, &g_config.wall_h);
        }
        else if (strcmp(argv[i], "--wall-grow") == 0 && i + 1 < argc) {
            g_config.wall_growth_rate = (uint32_t)atoi(argv[++i]);
        }
    }
}

int main(int argc, char* argv[])
{
    Parse_CLI_Args(argc, argv);

    printf("======================================================"
        "==========================\n");
    printf("     PROPHYSICS 2D GRAPH DYNAMICS & ADVANCED "
        "OBSERVABLE ENGINE\n");
    printf("======================================================"
        "==========================\n");
    printf("[*] Lade native ProPhysics Core-DLL und allokiere Substrat...\n");

    ProUniverse pu;
    uint64_t total_nodes = GRID_DIM * GRID_DIM;

    ProPhysics_Initialize(&pu, total_nodes);
    printf("[+] 2D-Mesh Substrat (%" PRIu64 " Knoten) verankert.\n",
        total_nodes);

    for (uint64_t y = 0; y < GRID_DIM; y++) {
        for (uint64_t x = 0; x < GRID_DIM; x++) {
            uint64_t idx = y * GRID_DIM + x;
            pu.reg_source[idx].channels[0] =
                ((y == 0 ? GRID_DIM - 1 : y - 1) * GRID_DIM) + x;
            pu.reg_source[idx].channels[1] =
                ((y == GRID_DIM - 1 ? 0 : y + 1) * GRID_DIM) + x;
            pu.reg_source[idx].channels[2] =
                (y * GRID_DIM) + (x == GRID_DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] =
                (y * GRID_DIM) + (x == 0 ? GRID_DIM - 1 : x - 1);

            uint32_t hash = (uint32_t)(x * 0x85ebca6bU ^ y * 0xc2b2ae35U);
            hash ^= hash >> 16;
            hash *= 0x45d9f3bU;
            hash ^= hash >> 16;

            if (hash % 7 == 0) { pu.ur_grid[idx].type_state = 0x02U; }
            else if (hash % 11 == 0) { pu.ur_grid[idx].type_state = 0x04U; }
            else if (hash % 23 == 0) { pu.ur_grid[idx].type_state = 0x05U; }
        }
    }

    printf("[+] Topologie verdrahtet. Starte Simulation (%u Ticks)...\n\n",
        g_config.total_ticks);

    char frame_buf[128];

    for (uint32_t tick = 1; tick <= g_config.total_ticks; tick++) {
        Apply_Scenario_Injections(&pu, tick);
        ProPhysics_SDK_Execute_Plastizitaet_Tick(
            &pu, ResearchPlugin_DynamicPlasticTopology);

        if (tick % 10 == 0 || tick == 1) {
            printf("  -> Tick #%2u | Aktive Interaktionen = %6u\n",
                tick, pu.global_entropy_index);
            Measure_Energy_and_Coupling(&pu, tick);
        }

        if (g_config.bmp_interval > 0 && (tick % g_config.bmp_interval == 0)) {
            snprintf(frame_buf, sizeof(frame_buf), "frame_%04u.bmp", tick);
            Export_Universe_To_BMP(&pu, frame_buf);
            printf("  [FRAME] Exportiert: '%s'\n", frame_buf);
        }
    }

    printf("\n------------------------------------------------------"
        "--------------------------\n");
    Calculate_Topological_Metrics(&pu);

    if (g_config.export_final_bmp) {
        Export_Universe_To_BMP(&pu, "simulation_output.bmp");
        printf("[+] finale 'simulation_output.bmp' wurde gespeichert.\n");
    }
    else if (g_config.bmp_interval == 0) {
        Render_ASCII_Viewport(&pu);
        printf("[TIPP] Nutze '--bmp' oder '--bmp-interval N' "
            "fuer Bild-Exports!\n");
    }

    printf("------------------------------------------------------"
        "--------------------------\n");
    ProPhysics_Free(&pu);
    printf("[SUCCESS] Testlauf fehlerfrei beendet.\n");
    printf("======================================================"
        "==========================\n");

    return 0;
}