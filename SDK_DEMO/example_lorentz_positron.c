/**
 * @file example_lorentz_positron.c
 * @brief Interaktive V2.0 Lorentz Dual-Drift Visualisierung & Konsole
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "ProPhysics.h"

#ifdef _WIN32
#include <windows.h>
static void enable_ansi_terminal(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
static void enable_ansi_terminal(void) {}
#endif

#define NODES 16

#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_GRAY    "\x1b[90m"
#define ANSI_RED     "\x1b[91m"
#define ANSI_GREEN   "\x1b[92m"
#define ANSI_YELLOW  "\x1b[93m"
#define ANSI_CYAN    "\x1b[96m"
#define ANSI_MAGENTA "\x1b[95m"

static uint8_t g_override_helicity = 0;

static void update_invariance_target(ProUniverse* pu) {
    uint64_t current_sum = 0;
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        switch (pu->ur_grid[i].type_state) {
        case UR_POSITRON_CW:
        case UR_POSITRON_CCW: current_sum += 1; break;
        case UR_NEGATRON_CW:
        case UR_NEGATRON_CCW: current_sum += 4; break;
        case UR_PHOTON:       current_sum += 5; break;
        default: break;
        }
    }
    pu->dynamic_invariance_target = current_sum;
}

static void setup_lorentz_topology(ProUniverse* pu) {
    ProPhysics_Initialize(pu, NODES);

    for (uint32_t i = 0; i < NODES; i++) {
        uint32_t fwd_1 = (i + 1) % NODES;         // Linear (+1)
        uint32_t rev_1 = (i + NODES - 1) % NODES; // Positron-Drift / Ausweichschritt (-1)
        uint32_t fwd_3 = (i + 3) % NODES;         // Negatron-Drift (+3)

        // Kernel-konforme Kanalbelegung (entspricht exakt ProPhysics.c):
        ProPhysics_Link_Nodes(pu, i, fwd_1, 0); // Kanal 0: Standard Linear Vorwärts (+1)
        ProPhysics_Link_Nodes(pu, i, rev_1, 1); // Kanal 1: Positronen-Kanal (wird von t_pos = reg->channels[1] gelesen)
        ProPhysics_Link_Nodes(pu, i, fwd_3, 2); // Kanal 2: Negatronen-Kanal (wird von t_neg = reg->channels[2] gelesen)
        ProPhysics_Link_Nodes(pu, i, fwd_1, 3); // Kanal 3: Fallback-Kanal (+1)
    }

    // Start-Injektion
    pu->ur_grid[0].type_state = UR_NEGATRON_CW; // 4 Bits
    pu->ur_grid[8].type_state = UR_POSITRON_CW; // 1 Bit
    update_invariance_target(pu);
}

static void apply_field_helicity(ProUniverse* pu, uint8_t helicity) {
    g_override_helicity = helicity;
    for (uint32_t i = 0; i < NODES; i++) {
        pu->ur_grid[i].field_helicity = helicity;
    }
}

static const char* get_node_symbol(uint8_t state, char* buf, size_t len) {
    switch (state) {
    case UR_NEGATRON_CW:
    case UR_NEGATRON_CCW:
        snprintf(buf, len, ANSI_CYAN ANSI_BOLD "[e-]" ANSI_RESET);
        break;
    case UR_POSITRON_CW:
    case UR_POSITRON_CCW:
        snprintf(buf, len, ANSI_MAGENTA ANSI_BOLD "[e+]" ANSI_RESET);
        break;
    case UR_PHOTON:
        snprintf(buf, len, ANSI_GREEN ANSI_BOLD "[y ]" ANSI_RESET);
        break;
    case UR_NEUTRAL:
    default:
        snprintf(buf, len, ANSI_GRAY "[. ]" ANSI_RESET);
        break;
    }
    return buf;
}

static void render_ui(const ProUniverse* pu) {
    printf("\x1b[H\x1b[J");
    printf(ANSI_BOLD ANSI_YELLOW "=======================================================================\n" ANSI_RESET);
    printf(ANSI_BOLD " PROPHYSICS V2.0 :: LORENTZ DUAL-DRIFT INTERACTIVE DASHBOARD\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_YELLOW "=======================================================================\n\n" ANSI_RESET);

    bool valid = ProPhysics_Verify_Invariance(pu);
    printf(" Tick: " ANSI_BOLD "%-6llu" ANSI_RESET " | Feld-Helizitaet: %s | Invarianz: %s (%llu Bits im System)\n\n",
        pu->current_cpu_tick,
        (g_override_helicity > 0) ? ANSI_RED "AKTIV (B-Feld = 1)" ANSI_RESET : ANSI_GRAY "OFF (Neutral = 0)" ANSI_RESET,
        valid ? ANSI_GREEN "PASSED" ANSI_RESET : ANSI_RED "FAILED" ANSI_RESET,
        pu->dynamic_invariance_target);

    printf(" " ANSI_BOLD "Topologischer Knoten-Ring (00 .. 15):" ANSI_RESET "\n ");
    for (uint32_t i = 0; i < NODES; i++) {
        char symbol[64];
        printf("%s ", get_node_symbol(pu->ur_grid[i].type_state, symbol, sizeof(symbol)));
    }
    printf("\n ");
    for (uint32_t i = 0; i < NODES; i++) {
        printf("%02u   ", i);
    }
    printf("\n\n");

    printf(" " ANSI_BOLD "Aktive Teilchen & Routing-Vektoren:" ANSI_RESET "\n");
    printf(" +--------+------------+--------------+--------------+--------------+--------------+\n");
    printf(" | Knoten | Typ        | Ch 0 (Lin)   | Ch 1 (e+ Drf)| Ch 2 (e- Drf)| Ch 3 (FB)    |\n");
    printf(" +--------+------------+--------------+--------------+--------------+--------------+\n");

    bool found = false;
    for (uint32_t i = 0; i < NODES; i++) {
        uint8_t st = pu->ur_grid[i].type_state;
        if (st != UR_NEUTRAL) {
            found = true;
            char symbol[64];
            get_node_symbol(st, symbol, sizeof(symbol));
            uint64_t ch0 = pu->reg_source[i].channels[0];
            uint64_t ch1 = pu->reg_source[i].channels[1];
            uint64_t ch2 = pu->reg_source[i].channels[2];
            uint64_t ch3 = pu->reg_source[i].channels[3];

            printf(" | #%-6u | %-19s | Node #%-5llu | Node #%-5llu | Node #%-5llu | Node #%-5llu |\n",
                i, symbol, ch0, ch1, ch2, ch3);
        }
    }
    if (!found) {
        printf(" | Keine aktiven Quell-Zustande im Gitter vorhanden.                                   |\n");
    }
    printf(" +--------+------------+--------------+--------------+--------------+--------------+\n\n");

    printf(ANSI_BOLD " Befehle:" ANSI_RESET "\n");
    printf("  [Enter] / s  : 1 Tick ausfuehren\n");
    printf("  r <n>        : n Ticks automatisch ausfuehren (z.B. 'r 5')\n");
    printf("  h <0|1>      : Feld-Helizitaet umschalten (0 = Neutral, 1 = B-Feld Aktiv)\n");
    printf("  i <n> <t>    : Teilchen an Knoten n injizieren (t: e-, e+, y, 0)\n");
    printf("  reset        : Universum auf Startzustand zuruecksetzen\n");
    printf("  q            : Beenden\n\n");
}

int main(void) {
    enable_ansi_terminal();

    ProUniverse universe;
    setup_lorentz_topology(&universe);

    char input_buf[128];

    while (true) {
        render_ui(&universe);
        printf(ANSI_YELLOW "ProPhysics-CLI> " ANSI_RESET);

        if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
        input_buf[strcspn(input_buf, "\r\n")] = 0;

        if (strlen(input_buf) == 0 || strcmp(input_buf, "s") == 0) {
            ProPhysics_Execute_Tick(&universe);
            if (g_override_helicity > 0) apply_field_helicity(&universe, g_override_helicity);
        }
        else if (input_buf[0] == 'r') {
            int ticks = 1;
            if (sscanf(input_buf, "r %d", &ticks) == 1 && ticks > 0) {
                for (int t = 0; t < ticks; t++) {
                    ProPhysics_Execute_Tick(&universe);
                    if (g_override_helicity > 0) apply_field_helicity(&universe, g_override_helicity);
                }
            }
        }
        else if (input_buf[0] == 'h') {
            int h = 0;
            if (sscanf(input_buf, "h %d", &h) == 1) {
                apply_field_helicity(&universe, (uint8_t)h);
            }
        }
        else if (input_buf[0] == 'i') {
            uint32_t node = 0;
            char type[16] = { 0 };
            if (sscanf(input_buf, "i %u %15s", &node, type) == 2 && node < NODES) {
                if (strcmp(type, "e-") == 0)      universe.ur_grid[node].type_state = UR_NEGATRON_CW;
                else if (strcmp(type, "e+") == 0) universe.ur_grid[node].type_state = UR_POSITRON_CW;
                else if (strcmp(type, "y") == 0)  universe.ur_grid[node].type_state = UR_PHOTON;
                else if (strcmp(type, "0") == 0)  universe.ur_grid[node].type_state = UR_NEUTRAL;

                update_invariance_target(&universe);
            }
        }
        else if (strcmp(input_buf, "reset") == 0) {
            ProPhysics_Free(&universe);
            setup_lorentz_topology(&universe);
            g_override_helicity = 0;
        }
        else if (strcmp(input_buf, "q") == 0) {
            break;
        }
    }

    ProPhysics_Free(&universe);
    printf("\n[PROPHYSICS] Konsole beendet.\n");
    return 0;
}