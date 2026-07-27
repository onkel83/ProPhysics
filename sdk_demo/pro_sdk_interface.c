/* ==========================================================================
 * ProPhysics - Dynamic 2D Mesh + Topological Metrics & BMP/ASCII Renderer
 * File: pro_sdk_interface.c
 * Architecture: Non-Euclidean Graph Dynamics & Visual Observable Layer
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ProPhysics.h"

#define GRID_DIM 1000 // 1000 x 1000 = 1.000.000 Knoten

typedef void (*ProPhysics_ScientificRuleCallback)(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state,
    uint64_t current_idx, uint64_t total_nodes
    );

// =========================================================================
// 0. REGION: EXPERIMENT & SZENARIO KONFIGURATION
// =========================================================================

typedef struct {
    // Punkt A: Impuls-Injektor
    bool     pulse_active;
    uint32_t pulse_x, pulse_y;
    uint8_t  pulse_type;        // 0x05 = Photon, 0x02 = Spin-Up, 0x01 = Positron, etc.
    uint32_t pulse_length;      // Anzahlt Ticks, die ein Puls injiziert wird
    uint32_t pulse_interval;    // Pause zwischen zwei Pulsen in Ticks
    uint32_t pulse_repeats;     // Anzahl der Gesamtwiederholungen

    // Punkt B: Vakuum-Barriere
    bool     wall_active;
    uint32_t wall_x, wall_y;
    uint32_t wall_w, wall_h;
    uint32_t wall_growth_rate; // 0 = statisch, N > 0 = wächst alle N Ticks um 2 Einheiten in Höhe

    // Punkt C: BMP-Zeitraffer & Simulationsticks
    uint32_t bmp_interval;      // Alle X Ticks ein BMP exportieren (0 = Deaktiviert)
    uint32_t total_ticks;       // Gesamte Simulationsdauer
    bool     export_final_bmp;  // Standard-BMP am Ende
} SimulationScenario;

// Standardszenario (Kommandozeile überschreibt diese Werte)
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

// =========================================================================
// 1. REGION: RESEARCHER PLAYGROUND (Plastische Topologie)
// =========================================================================

void ResearchPlugin_DynamicPlasticTopology(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state,
    uint64_t current_idx, uint64_t total_nodes)
{
    // 1. Photonen-Paarerzeugung / Annihilation
    if (current_state == 0x05U && target_state == 0x05U) {
        *out_next_state = 0x01U; // Positron
        *out_next_target_state = 0x03U; // Negatron
        return;
    }

    // 2. ISING-SPIN & MAGNETISCHE WECHSELWIRKUNG
    if (current_state == target_state && current_state != 0) {
        *out_next_state = current_state;
        *out_next_target_state = target_state;

        int strongest_ch = -1;
        uint64_t max_target = total_nodes;

        for (int ch = 0; ch < 4; ch++) {
            uint64_t neighbor_idx = target_channels[ch];
            if (neighbor_idx < total_nodes) {
                if (strongest_ch == -1 || (neighbor_idx % 2 == current_state % 2)) {
                    strongest_ch = ch;
                    max_target = neighbor_idx;
                }
            }
        }

        // HEBB'SCHES REWIRING ENTLANG DES MAGNETISCHEN GRADIENTEN
        if (strongest_ch != -1 && max_target < total_nodes) {
            uint32_t channel_to_rewire = (strongest_ch + 2) % 4;
            current_channels[channel_to_rewire] = max_target;
        }
    }
    // 3. Antiparallele Spins (Energetische Spannung / Repulsion)
    else if (current_state != target_state && current_state != 0 && target_state != 0) {
        *out_next_state = target_state;
        *out_next_target_state = current_state;

        current_channels[0] = (current_channels[0] + 1) % total_nodes;
    }
    else {
        *out_next_state = current_state;
        *out_next_target_state = target_state;
    }
}

// =========================================================================
// 2. REGION: VISUALISIERUNG & TOPOLOGISCHE METRIKEN
// =========================================================================

static void Export_Universe_To_BMP(ProUniverse* pu, const char* filename) {
    if (!pu || !pu->ur_grid) return;

    uint32_t width = GRID_DIM;
    uint32_t height = GRID_DIM;
    uint32_t row_size = (width * 3 + 3) & ~3;
    uint32_t image_size = row_size * height;

    uint8_t header[54] = {
        'B', 'M',
        0, 0, 0, 0, 0, 0, 0, 0,
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
    if (!f) {
        printf("[!] Fehler beim Erstellen der BMP-Datei: %s\n", filename);
        return;
    }

    fwrite(header, 1, 54, f);

    uint8_t* row_buf = (uint8_t*)calloc(1, row_size);
    if (!row_buf) {
        fclose(f);
        return;
    }

    for (int32_t y = height - 1; y >= 0; y--) {
        for (uint32_t x = 0; x < width; x++) {
            uint64_t idx = (uint64_t)y * width + x;
            uint8_t st = pu->ur_grid[idx].type_state;

            uint8_t r = 10, g = 10, b = 15; // Vakuum / Leer (Dunkelblau)

            if (st == 0x01U) { r = 255; g = 50;  b = 50; } // Positron (Rot)
            else if (st == 0x03U) { r = 50;  g = 50;  b = 255; } // Negatron (Blau)
            else if (st == 0x05U) { r = 255; g = 255; b = 255; } // Photon (Weiß)
            else if (st == 0x02U) { r = 255; g = 200; b = 0; } // Spin Up (Gelb)
            else if (st == 0x04U) { r = 0;   g = 255; b = 100; } // Spin Down (Grün)

            row_buf[x * 3 + 0] = b;
            row_buf[x * 3 + 1] = g;
            row_buf[x * 3 + 2] = r;
        }
        fwrite(row_buf, 1, row_size, f);
    }

    free(row_buf);
    fclose(f);
}

static void Render_ASCII_Viewport(ProUniverse* pu) {
    if (!pu || !pu->ur_grid) return;

    uint32_t size = 40;
    uint32_t start_x = (GRID_DIM / 2) - (size / 2);
    uint32_t start_y = (GRID_DIM / 2) - (size / 2);

    printf("\n--- ASCII-GITTERSCHNITT (Mitte: %ux%u) ---\n", size, size);
    printf(" Legende: [.] Vakuum  [^] Spin-Up  [v] Spin-Down  [*] Photon  [+] Positron  [-] Negatron\n\n");

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
    printf("-------------------------------------------\n");
}

static void Calculate_Topological_Metrics(ProUniverse* pu) {
    if (!pu || !pu->reg_source) return;

    uint64_t non_euclidean_links = 0;
    uint64_t total_links = pu->total_nodes * 4;

    for (uint64_t y = 0; y < GRID_DIM; y++) {
        for (uint64_t x = 0; x < GRID_DIM; x++) {
            uint64_t idx = y * GRID_DIM + x;

            uint64_t expected_north = ((y == 0 ? GRID_DIM - 1 : y - 1) * GRID_DIM) + x;
            uint64_t expected_south = ((y == GRID_DIM - 1 ? 0 : y + 1) * GRID_DIM) + x;

            if (pu->reg_source[idx].channels[0] != expected_north) non_euclidean_links++;
            if (pu->reg_source[idx].channels[1] != expected_south) non_euclidean_links++;
        }
    }

    double plasticity_ratio = ((double)non_euclidean_links / (double)total_links) * 100.0;
    printf("  [Topologische Metrik]\n");
    printf("    -> Nicht-euklidische Kanäle (Wurmlöcher/Rewirings): %llu (%.2f%% des Raums)\n",
        non_euclidean_links, plasticity_ratio);
}

// =========================================================================
// 3. REGION: EXPERIMENT-INJEKTOR & RUNTIME ENGINE
// =========================================================================

static void Apply_Scenario_Injections(ProUniverse* pu, uint32_t current_tick) {
    if (!pu || !pu->ur_grid) return;

    // --- PUNKT A: IMPULS-INJEKTOR ---
    if (g_config.pulse_active && g_config.pulse_repeats > 0) {
        uint32_t cycle_length = g_config.pulse_length + g_config.pulse_interval;
        uint32_t current_cycle = (current_tick - 1) / cycle_length;
        uint32_t tick_in_cycle = (current_tick - 1) % cycle_length;

        if (current_cycle < g_config.pulse_repeats && tick_in_cycle < g_config.pulse_length) {
            if (g_config.pulse_x < GRID_DIM && g_config.pulse_y < GRID_DIM) {
                uint64_t idx = g_config.pulse_y * GRID_DIM + g_config.pulse_x;
                pu->ur_grid[idx].type_state = g_config.pulse_type;

                if (tick_in_cycle == 0) {
                    printf("[PULS] Injiziere Typ 0x%02X an (%u, %u) | Puls #%u (Tick %u)\n",
                        g_config.pulse_type, g_config.pulse_x, g_config.pulse_y,
                        current_cycle + 1, current_tick);
                }
            }
        }
    }

    // --- PUNKT B: VAKUUM-BARRIERE ---
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
                    pu->ur_grid[idx].type_state = 0x00U; // Vakuum-Sperre erzwingen
                }
            }
        }
    }
}

void ProPhysics_SDK_Execute_Plastizitaet_Tick(ProUniverse* pu, ProPhysics_ScientificRuleCallback callback) {
    if (!pu || !pu->ur_grid || !pu->reg_source || !callback) return;

    if (!pu->reg_target) {
        pu->reg_target = (ProRegister*)calloc(pu->total_nodes, sizeof(ProRegister));
        if (!pu->reg_target) return;
    }

    pu->current_cpu_tick++;
    uint64_t active_interactions = 0;

    memcpy(pu->reg_target, pu->reg_source, pu->total_nodes * sizeof(ProRegister));

    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
        uint8_t current_state = pu->ur_grid[idx].type_state;
        if (current_state == 0) continue;

        for (int ch = 0; ch < 4; ch++) {
            uint64_t target_ptr = pu->reg_source[idx].channels[ch];

            if (target_ptr < pu->total_nodes && target_ptr != idx) {
                uint8_t target_state = pu->ur_grid[target_ptr].type_state;

                if (target_state != 0) {
                    uint8_t next_state = current_state;
                    uint8_t next_target_state = target_state;

                    uint64_t current_ch[CHANNELS_MAX];
                    uint64_t target_ch[CHANNELS_MAX];

                    for (int c = 0; c < CHANNELS_MAX; c++) {
                        current_ch[c] = pu->reg_source[idx].channels[c];
                        target_ch[c] = pu->reg_source[target_ptr].channels[c];
                    }

                    callback(current_state, target_state, current_ch, target_ch,
                        &next_state, &next_target_state, idx, pu->total_nodes);

                    pu->ur_grid[idx].type_state = next_state;
                    pu->ur_grid[target_ptr].type_state = next_target_state;

                    for (int c = 0; c < CHANNELS_MAX; c++) {
                        pu->reg_target[idx].channels[c] = current_ch[c];
                        pu->reg_target[target_ptr].channels[c] = target_ch[c];
                    }

                    active_interactions++;
                }
            }
        }
    }

    ProRegister* temp = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = temp;

    pu->global_entropy_index = (uint32_t)active_interactions;
}

// =========================================================================
// 4. REGION: CLI-PARSER & MAIN
// =========================================================================

static void Parse_CLI_Args(int argc, char* argv[]) {
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
        // --pulse x,y,type,length,interval,repeats
        else if (strcmp(argv[i], "--pulse") == 0 && i + 1 < argc) {
            g_config.pulse_active = true;
            uint32_t raw_type = 5;
            sscanf(argv[++i], "%u,%u,%i,%u,%u,%u",
                &g_config.pulse_x, &g_config.pulse_y, &raw_type,
                &g_config.pulse_length, &g_config.pulse_interval, &g_config.pulse_repeats);
            g_config.pulse_type = (uint8_t)raw_type;
        }
        // --wall x,y,w,h
        else if (strcmp(argv[i], "--wall") == 0 && i + 1 < argc) {
            g_config.wall_active = true;
            sscanf(argv[++i], "%u,%u,%u,%u",
                &g_config.wall_x, &g_config.wall_y, &g_config.wall_w, &g_config.wall_h);
        }
        else if (strcmp(argv[i], "--wall-grow") == 0 && i + 1 < argc) {
            g_config.wall_growth_rate = (uint32_t)atoi(argv[++i]);
        }
    }
}

int main(int argc, char* argv[]) {
    Parse_CLI_Args(argc, argv);

    printf("================================================================================\n");
    printf("     PROPHYSICS 2D GRAPH DYNAMICS & ADVANCED OBSERVABLE ENGINE\n");
    printf("================================================================================\n");
    printf("[*] Lade native ProPhysics Core-DLL und allokiere Substrat...\n");

    ProUniverse pu;
    uint64_t total_nodes = GRID_DIM * GRID_DIM;

    ProPhysics_Initialize(&pu, total_nodes);
    printf("[+] 2D-Mesh Substrat (%llu Knoten) verankert.\n", total_nodes);

    // Torus- & Substrat-Initialisierung
    for (uint64_t y = 0; y < GRID_DIM; y++) {
        for (uint64_t x = 0; x < GRID_DIM; x++) {
            uint64_t idx = y * GRID_DIM + x;

            pu.reg_source[idx].channels[0] = ((y == 0 ? GRID_DIM - 1 : y - 1) * GRID_DIM) + x;
            pu.reg_source[idx].channels[1] = ((y == GRID_DIM - 1 ? 0 : y + 1) * GRID_DIM) + x;
            pu.reg_source[idx].channels[2] = (y * GRID_DIM) + (x == GRID_DIM - 1 ? 0 : x + 1);
            pu.reg_source[idx].channels[3] = (y * GRID_DIM) + (x == 0 ? GRID_DIM - 1 : x - 1);

            uint32_t hash = (uint32_t)(x * 0x85ebca6bU ^ y * 0xc2b2ae35U);
            hash ^= hash >> 16;
            hash *= 0x45d9f3bU;
            hash ^= hash >> 16;

            if (hash % 7 == 0) { pu.ur_grid[idx].type_state = 0x02U; } // Spin-Up
            else if (hash % 11 == 0) { pu.ur_grid[idx].type_state = 0x04U; } // Spin-Down
            else if (hash % 23 == 0) { pu.ur_grid[idx].type_state = 0x05U; } // Photon
        }
    }

    printf("[+] Topologie verdrahtet. Starte Simulation (%u Ticks)...\n\n", g_config.total_ticks);

    char frame_buf[128];

    for (uint32_t tick = 1; tick <= g_config.total_ticks; tick++) {
        // A & B: Impuls- & Barrieren-Einflüsse anwenden
        Apply_Scenario_Injections(&pu, tick);

        // Core Physics Tick ausführen
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu, ResearchPlugin_DynamicPlasticTopology);

        if (tick % 10 == 0 || tick == 1) {
            printf("  -> Tick #%2u | Aktive Interaktionen = %6u\n", tick, pu.global_entropy_index);
        }

        // C: Automatische BMP-Bilderserie (Zeitraffer)
        if (g_config.bmp_interval > 0 && (tick % g_config.bmp_interval == 0)) {
            snprintf(frame_buf, sizeof(frame_buf), "frame_%04u.bmp", tick);
            Export_Universe_To_BMP(&pu, frame_buf);
            printf("  [FRAME] Exportiert: '%s'\n", frame_buf);
        }
    }

    printf("\n--------------------------------------------------------------------------------\n");
    Calculate_Topological_Metrics(&pu);

    if (g_config.export_final_bmp) {
        Export_Universe_To_BMP(&pu, "simulation_output.bmp");
        printf("[+] finale 'simulation_output.bmp' wurde gespeichert.\n");
    }
    else if (g_config.bmp_interval == 0) {
        Render_ASCII_Viewport(&pu);
        printf("[TIPP] Nutze '--bmp' oder '--bmp-interval N' für Bild-Exports!\n");
    }

    printf("--------------------------------------------------------------------------------\n");
    ProPhysics_Free(&pu);
    printf("[SUCCESS] Testlauf fehlerfrei beendet.\n");
    printf("================================================================================\n");

    return 0;
}