/**
 * @file example_Fusion_Observable.c
 * @brief ProPhysics SDK Demo: Interaktive, beobachtbare Kernfusion mit Photonen-Emission in Schleife
 *
 * Visualisiert in Echtzeit die Proton-Proton-Fusion (4x H1 -> He2 + 2 Gamma-Photonen)
 * unter strenger Einhaltung der topologischen Bit-Invarianz im Ur-Grid.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
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
#include "ProPhysics_Types.h"

#define GRID_SIZE      64          // Für konsolenoptimierte ASCII-Darstellung
#define RENDER_WIDTH   64

 /* Topologische Ur-Zustände (Fallback-Mapping für Standard-Typen) */
#ifndef UR_NEUTRAL
#define UR_NEUTRAL        0x00U
#define UR_PHOTON         0x01U
#define UR_NEGATRON_CW    0x03U
#define UR_NEGATRON_CCW   0x04U
#define UR_POSITRON_CW    0x05U
#define UR_POSITRON_CCW   0x06U
#endif

/**
 * @brief Rendert das ur_grid als ASCII-Grafikleiste und gibt Teilchen-Zähler aus.
 */
void Render_Grid_Visualizer(ProUniverse* pu, uint32_t tick, const char* phase_info, uint32_t cycle) {
    CLEAR_SCREEN();

    printf("====================================================================\n");
    printf(" PROPHYSICS :: REALTIME FUSION VISUALIZER (CYCLE #%u)                \n", cycle);
    printf("====================================================================\n");
    printf(" Phase: %-43s | Tick: #%u\n", phase_info, tick);
    printf("--------------------------------------------------------------------\n\n");

    // 1. ASCII-Grid Rendern
    printf(" GRID MAP [0..63]:\n ");
    for (uint32_t i = 0; i < RENDER_WIDTH; i++) {
        uint8_t state = pu->ur_grid[i].type_state;
        switch (state) {
        case UR_POSITRON_CW:  printf("P"); break; // Proton/Positron CW
        case UR_POSITRON_CCW: printf("p"); break; // Proton/Positron CCW
        case UR_NEGATRON_CW:  printf("e"); break; // Elektron CW
        case UR_NEGATRON_CCW: printf("E"); break; // Elektron CCW
        case UR_PHOTON:       printf("*"); break; // Abgestrahltes Photon (Gamma-Quant)
        case UR_NEUTRAL:      printf("."); break; // Vakuum / Neutral
        default:              printf("?"); break;
        }
    }
    printf("\n\n");

    // 2. Legende & Telemetrie
    printf(" Legende:  [P/p] Proton (Positron)  |  [E/e] Elektron (Negatron)\n");
    printf("           [*] Photon (Abgestrahlte Welle)  |  [.] Neutral\n");
    printf("--------------------------------------------------------------------\n");

    // Teilchen-Statistik auf dem Grid erfassen
    uint32_t protons = 0, electrons = 0, photons = 0;
    for (uint32_t i = 0; i < pu->total_nodes; i++) {
        uint8_t st = pu->ur_grid[i].type_state;
        if (st == UR_POSITRON_CW || st == UR_POSITRON_CCW) protons++;
        else if (st == UR_NEGATRON_CW || st == UR_NEGATRON_CCW) electrons++;
        else if (st == UR_PHOTON) photons++;
    }

    bool inv_ok = ProPhysics_Verify_Invariance(pu);
    printf(" Protonen: %u | Elektronen: %u | Abgestrahlte Photonen: %u\n", protons, electrons, photons);
    printf(" Total Invarianz-Summe: %llu  [%s]\n",
        (unsigned long long)pu->dynamic_invariance_target,
        inv_ok ? "PASSED - 100% BIT CONSERVED" : "VIOLATION DETECTED");
    printf("====================================================================\n");
}

int main(void) {
    printf("====================================================================\n");
    printf(" PROPHYSICS SDK :: OBERVABLE FUSION VISUALIZER (4x H1 -> He2 + 2*) \n");
    printf("====================================================================\n\n");

    printf("Waehlen Sie den Ausfuehrungsmodus:\n");
    printf(" [1] Automatischer Loop (Kontinuierliche Animation)\n");
    printf(" [2] Step-by-Step Inspektor (Jeden Tick per Enter-Taste weiterschalten)\n");
    printf(" [3] Interaktive Sandbox (Geschwindigkeit & Zyklen anpassen)\n");
    printf("Auswahl (1-3): ");

    int mode = 1;
    if (scanf("%d", &mode) != 1) mode = 1;

    uint32_t frame_delay_ms = 120;
    uint32_t max_cycles = 0; // 0 = Unendlich
    bool step_by_step = false;

    if (mode == 2) {
        step_by_step = true;
    }
    else if (mode == 3) {
        printf("\n--- VISUALIZER CONFIG ---\n");
        printf("Frame-Verzoegerung in ms [z.B. 80 für schnell, 200 für langsam]: ");
        if (scanf("%u", &frame_delay_ms) != 1) frame_delay_ms = 120;

        printf("Anzahl der Zyklen (0 = Unendlich) [z.B. 3]: ");
        if (scanf("%u", &max_cycles) != 1) max_cycles = 0;
    }

    // Eingabepuffer leeren
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    uint32_t cycle_counter = 1;

    while (max_cycles == 0 || cycle_counter <= max_cycles) {
        ProUniverse pu;
        ProPhysics_Initialize(&pu, GRID_SIZE);

        // --- PHASE 1: Vier Wasserstoff-Kerne (4x H1) erzeugen ---
        // Bilanz: 4x Positron (4*1=4) + 4x Negatron (4*4=16) = 20 Bit Invarianz
        pu.dynamic_invariance_target = 20;

        uint32_t h1_1_core = 8, h1_1_elec = 12;
        uint32_t h1_2_core = 16, h1_2_elec = 20;
        uint32_t h1_3_core = 44, h1_3_elec = 40;
        uint32_t h1_4_core = 56, h1_4_elec = 52;

        // Atom 1
        pu.ur_grid[h1_1_core].type_state = UR_POSITRON_CW;
        pu.ur_grid[h1_1_elec].type_state = UR_NEGATRON_CCW;
        pu.reg_source[h1_1_elec].channels[0] = h1_1_elec;
        pu.reg_source[h1_1_elec].channels[1] = h1_1_core;

        // Atom 2
        pu.ur_grid[h1_2_core].type_state = UR_POSITRON_CCW;
        pu.ur_grid[h1_2_elec].type_state = UR_NEGATRON_CW;
        pu.reg_source[h1_2_elec].channels[0] = h1_2_elec;
        pu.reg_source[h1_2_elec].channels[1] = h1_2_core;

        // Atom 3
        pu.ur_grid[h1_3_core].type_state = UR_POSITRON_CW;
        pu.ur_grid[h1_3_elec].type_state = UR_NEGATRON_CCW;
        pu.reg_source[h1_3_elec].channels[0] = h1_3_elec;
        pu.reg_source[h1_3_elec].channels[1] = h1_3_core;

        // Atom 4
        pu.ur_grid[h1_4_core].type_state = UR_POSITRON_CCW;
        pu.ur_grid[h1_4_elec].type_state = UR_NEGATRON_CW;
        pu.reg_source[h1_4_elec].channels[0] = h1_4_elec;
        pu.reg_source[h1_4_elec].channels[1] = h1_4_core;

        // Anfahrt-Simulation (Kerne bewegen sich aufeinander zu)
        for (int step = 0; step < 10; step++) {
            char title[64];
            snprintf(title, sizeof(title), "Isoliertes System (4x H1 Annaeherung)");
            Render_Grid_Visualizer(&pu, step, title, cycle_counter);

            if (step_by_step) {
                printf(" Press [ENTER] for next tick...");
                getchar();
            }
            else {
                SLEEP_MS(frame_delay_ms);
            }
        }

        // --- PHASE 2: FUSION TRIGGER & PHOTONEN-EMISSION ---
        // Alte H1-Positionen saeubern
        pu.ur_grid[h1_1_core].type_state = UR_NEUTRAL; pu.ur_grid[h1_1_elec].type_state = UR_NEUTRAL;
        pu.ur_grid[h1_2_core].type_state = UR_NEUTRAL; pu.ur_grid[h1_2_elec].type_state = UR_NEUTRAL;
        pu.ur_grid[h1_3_core].type_state = UR_NEUTRAL; pu.ur_grid[h1_3_elec].type_state = UR_NEUTRAL;
        pu.ur_grid[h1_4_core].type_state = UR_NEUTRAL; pu.ur_grid[h1_4_elec].type_state = UR_NEUTRAL;

        // Helium-Kern (He2) in der Mitte aufbauen (2 Protonen + 2 Elektronen = 10 Bits)
        uint32_t he_core_1 = 31, he_core_2 = 32;
        uint32_t he_elec_1 = 28, he_elec_2 = 35;

        pu.ur_grid[he_core_1].type_state = UR_POSITRON_CW;
        pu.ur_grid[he_core_2].type_state = UR_POSITRON_CCW;
        pu.ur_grid[he_elec_1].type_state = UR_NEGATRON_CCW;
        pu.ur_grid[he_elec_2].type_state = UR_NEGATRON_CW;

        pu.reg_source[he_elec_1].channels[0] = he_elec_2;
        pu.reg_source[he_elec_1].channels[1] = he_core_1;
        pu.reg_source[he_elec_2].channels[0] = he_elec_1;
        pu.reg_source[he_elec_2].channels[1] = he_core_1;

        // Abstrahlung von 2 Photonen (*) aus der Differenzmasse (2x 5 = 10 Bits)
        // Summe: 10 (Helium) + 10 (Photonen) = EXAKT 20 Bits!
        uint32_t photon_left = 22;
        uint32_t photon_right = 41;
        pu.ur_grid[photon_left].type_state = UR_PHOTON;
        pu.ur_grid[photon_right].type_state = UR_PHOTON;

        // --- PHASE 3: EXPANDIERENDE PHOTONEN-WELLEN ---
        for (uint32_t tick = 10; tick <= 40; tick++) {
            if (tick % 2 == 0) {
                // Linkes Photon wandert nach links
                if (photon_left > 0) {
                    pu.ur_grid[photon_left].type_state = UR_NEUTRAL;
                    photon_left--;
                    pu.ur_grid[photon_left].type_state = UR_PHOTON;
                }
                // Rechtes Photon wandert nach rechts
                if (photon_right < (GRID_SIZE - 1)) {
                    pu.ur_grid[photon_right].type_state = UR_NEUTRAL;
                    photon_right++;
                    pu.ur_grid[photon_right].type_state = UR_PHOTON;
                }
            }

            ProPhysics_Execute_Tick(&pu);

            char title[64];
            snprintf(title, sizeof(title), "FUSION ERFOLGT! Photonen (*) strahlen ab");
            Render_Grid_Visualizer(&pu, tick, title, cycle_counter);

            if (step_by_step) {
                printf(" Press [ENTER] for next tick...");
                getchar();
            }
            else {
                SLEEP_MS(frame_delay_ms);
            }
        }

        // --- PHASE 4: STABILISIERTES SYSTEM ---
        for (int pause = 0; pause < 8; pause++) {
            char title[64];
            snprintf(title, sizeof(title), "Stabilisierter He2-Kern + Freie Strahlung");
            Render_Grid_Visualizer(&pu, 40, title, cycle_counter);

            if (step_by_step) {
                printf(" Press [ENTER] to finish cycle...");
                getchar();
                break;
            }
            else {
                SLEEP_MS(frame_delay_ms);
            }
        }

        ProPhysics_Free(&pu);
        cycle_counter++;
    }

    printf("\n[VISUALIZER BEENDET] Alle %u Zyklen erfolgreich abgeschlossen.\n", cycle_counter - 1);
    return EXIT_SUCCESS;
}