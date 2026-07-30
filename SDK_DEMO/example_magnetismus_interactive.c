/**
 * @file example_magnetismus_interactive.c
 * @brief Interaktiver 2D-Konsolen-Visualisierer für die Lorentz-Ablenkung im ProPhysics SDK
 *
 * Stellt das 16-Knoten-Netzwerk als 2D-ASCII-Ring dar.
 * Visualisiert den Sprung von Kanal 0 (+1, linear) auf Kanal 3 (+3, Lorentz-Drift).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#define CLEAR_SCREEN() system("cls")
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#define CLEAR_SCREEN() printf("\033[H\033[J")
#endif

#include "ProPhysics.h"

#define NODES        16
#define CANVAS_W     66
#define CANVAS_H     21
#define CENTER_X     32
#define CENTER_Y     10
#define RADIUS_X     24.0f
#define RADIUS_Y     8.0f

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    int x;
    int y;
} Point2D;

/**
 * Berechnet die 2D-Koordinaten der 16 Knoten auf dem Ring.
 */
static void Compute_Node_Positions(Point2D pos[NODES]) {
    for (uint32_t i = 0; i < NODES; i++) {
        float angle = (float)(2.0 * M_PI * i / NODES);
        pos[i].x = (int)(CENTER_X + RADIUS_X * cosf(angle));
        pos[i].y = (int)(CENTER_Y + RADIUS_Y * sinf(angle));
    }
}

/**
 * Rendert das 2D-Netzwerk im ASCII-Puffer.
 */
static void Render_Console_2D(ProUniverse* pu, uint32_t tick, int helicity, uint32_t active_node) {
    char canvas[CANVAS_H][CANVAS_W];
    Point2D node_pos[NODES];

    Compute_Node_Positions(node_pos);

    // Canvas mit Leerzeichen füllen
    for (int y = 0; y < CANVAS_H; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            canvas[y][x] = ' ';
        }
        canvas[y][CANVAS_W - 1] = '\0';
    }

    // Verbindungs-Punkte zeichnen (Netzwerk-Ring)
    for (uint32_t i = 0; i < NODES; i++) {
        int x = node_pos[i].x;
        int y = node_pos[i].y;
        if (x >= 0 && x < CANVAS_W - 3 && y >= 0 && y < CANVAS_H) {
            // Knotenbeschriftung
            snprintf(&canvas[y][x], 4, "%02u", i);
        }
    }

    // Elektron auf dem aktiven Knoten markieren
    int ex = node_pos[active_node].x;
    int ey = node_pos[active_node].y;
    if (ey > 0 && ey < CANVAS_H - 1) {
        // Markierung oberhalb/unterhalb des Knotenlabels
        canvas[ey - 1][ex] = 'v';
        canvas[ey + 1][ex] = '^';
    }

    CLEAR_SCREEN();
    printf("==================================================================\n");
    printf(" PROPHYSICS :: LORENTZ-FORCE 2D RING VISUALIZER                   \n");
    printf("==================================================================\n");
    printf(" Tick: #%-3u | Active Node: #%-2u | Helicity: %d (%s)\n",
        tick, active_node, helicity,
        helicity == 0 ? "NEUTRAL -> Kanal 0 (+1)" : "MAGNETISCH -> Kanal 3 (+3 Drift)");
    printf("------------------------------------------------------------------\n\n");

    // Canvas ausgeben
    for (int y = 0; y < CANVAS_H; y++) {
        printf("  %s\n", canvas[y]);
    }

    printf("\n------------------------------------------------------------------\n");
    bool valid = ProPhysics_Verify_Invariance(pu);
    printf(" Legende: [v/^^] Elektron (UR_NEGATRON_CW)  |  Knoten: 00..15\n");
    printf(" Invarianz-Status: %s (4 Bits bit-exakt)\n",
        valid ? "PASSED [100% BIT CONSERVED]" : "FAILED");
    printf("==================================================================\n");
}

int main(void) {
    ProUniverse universe;
    ProPhysics_Initialize(&universe, NODES);

    // 1. Topologisches Routing aufbauen
    for (uint32_t i = 0; i < NODES; i++) {
        uint32_t next_linear = (i + 1) % NODES;
        uint32_t next_drift = (i + 3) % NODES;

        ProPhysics_Link_Nodes(&universe, i, next_linear, 0); // Kanal 0: +1
        ProPhysics_Link_Nodes(&universe, i, next_drift, 3); // Kanal 3: +3
    }

    // Elektron injizieren (4 Bit Invarianz)
    universe.ur_grid[0].type_state = UR_NEGATRON_CW;
    universe.dynamic_invariance_target = 4;

    CLEAR_SCREEN();
    printf("==================================================================\n");
    printf(" PROPHYSICS :: DEMO 1 - LORENTZ-ABLENKUNG (INTERAKTIV)            \n");
    printf("==================================================================\n\n");
    printf("Waehlen Sie den Ausfuehrungsmodus:\n");
    printf(" [1] Automatische Demo (Phase 1: Neutral -> Phase 2: Magnetisch)\n");
    printf(" [2] Interaktiver Stepper (Manuell Helizitaet umschalten & Ticken)\n");
    printf("Auswahl (1-2): ");

    int mode = 1;
    if (scanf("%d", &mode) != 1) mode = 1;

    // Eingabepuffer leeren
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    if (mode == 1) {
        // --- MODUS 1: AUTOMATISCHE SHOWCASE-DEMO ---
        uint32_t delay_ms = 250;

        // Phase 1: Helizität = 0 (4 Ticks)
        for (int tick = 0; tick < 5; tick++) {
            uint32_t pos = 0;
            for (uint32_t i = 0; i < NODES; i++) {
                if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) { pos = i; break; }
            }
            Render_Console_2D(&universe, tick, 0, pos);
            SLEEP_MS(delay_ms);
            ProPhysics_Execute_Tick(&universe);
        }

        // Feld aktivieren
        for (uint32_t i = 0; i < NODES; i++) {
            universe.ur_grid[i].field_helicity = 1;
        }

        // Phase 2: Helizität = 1 (8 Ticks)
        for (int tick = 5; tick <= 12; tick++) {
            uint32_t pos = 0;
            for (uint32_t i = 0; i < NODES; i++) {
                if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) { pos = i; break; }
            }
            Render_Console_2D(&universe, tick, 1, pos);
            SLEEP_MS(delay_ms);
            ProPhysics_Execute_Tick(&universe);
        }

    }
    else {
        // --- MODUS 2: INTERAKTIVER STEPPER ---
        uint32_t tick = 0;
        int current_helicity = 0;

        while (1) {
            uint32_t pos = 0;
            for (uint32_t i = 0; i < NODES; i++) {
                if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) { pos = i; break; }
            }

            Render_Console_2D(&universe, tick, current_helicity, pos);

            printf("\n STEUERUNG:\n");
            printf(" [ENTER] Nächster Tick\n");
            printf(" [h] Helizitaet umschalten (Aktuell: %d)\n", current_helicity);
            printf(" [q] Beenden\n");
            printf(" Eingabe: ");

            char input = (char)getchar();
            if (input == 'q' || input == 'Q') {
                break;
            }
            else if (input == 'h' || input == 'H') {
                current_helicity = (current_helicity == 0) ? 1 : 0;
                for (uint32_t i = 0; i < NODES; i++) {
                    universe.ur_grid[i].field_helicity = current_helicity;
                }
                // Zeilenumbruch konsumieren
                while ((c = getchar()) != '\n' && c != EOF);
                continue;
            }
            else if (input != '\n') {
                while ((c = getchar()) != '\n' && c != EOF);
            }

            ProPhysics_Execute_Tick(&universe);
            tick++;
        }
    }

    printf("\n[DEMO BEENDET] Universe Speicher freigegeben.\n");
    ProPhysics_Free(&universe);
    return 0;
}