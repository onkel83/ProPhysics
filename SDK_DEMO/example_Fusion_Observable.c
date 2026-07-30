/**
 * @file example_Fusion_Observable.c
 * @brief ProPhysics SDK Demo: Interaktive, beobachtbare Kernfusion mit Photonen-Emission & SI/MeV-Kalibrierung
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

#define GRID_SIZE        64          // Für konsolenoptimierte ASCII-Darstellung
#define RENDER_WIDTH    64

 // --- PHYSIKALISCHE KALIBRIERUNGS-KONSTANTEN ---
#define SKALAR_MEV_PER_INVARIANCE 3.518      // 1 Invarianz-Punkt = 3.518 MeV
#define SKALAR_FEMTOMETER_PER_CELL 1.0       // 1 Grid-Zelle = 1.0 Femtometer (10^-15 m)
#define MEV_TO_JOULE              1.60218e-13 // 1 MeV in Joule

/**
 * @brief Rendert das ur_grid als ASCII-Grafikleiste und gibt Teilchen-Zähler sowie SI-Physik-Daten aus.
 */
void Render_Grid_Visualizer(ProUniverse* pu, uint32_t tick, const char* phase_info, uint32_t cycle, double current_dist_fm) {
    CLEAR_SCREEN();

    printf("====================================================================\n");
    printf(" PROPHYSICS :: REALTIME FUSION VISUALIZER (CYCLE #%u)                \n", cycle);
    printf("====================================================================\n");
    printf(" Phase: %-43s | Tick: #%u\n", phase_info, tick);
    printf("--------------------------------------------------------------------\n\n");

    // 1. ASCII-Grid Rendern
    printf(" GRID MAP [0..63] (1 Cell = %.1f fm):\n ", SKALAR_FEMTOMETER_PER_CELL);
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
    printf("           [*] Photon (Gamma-Quant)  |  [.] Neutral / Vakuum\n");
    printf("--------------------------------------------------------------------\n");

    // Teilchen-Statistik auf dem Grid erfassen
    uint32_t protons = 0, electrons = 0, photons = 0;
    for (uint32_t i = 0; i < pu->total_nodes; i++) {
        uint8_t st = pu->ur_grid[i].type_state;
        if (st == UR_POSITRON_CW || st == UR_POSITRON_CCW) protons++;
        else if (st == UR_NEGATRON_CW || st == UR_NEGATRON_CCW) electrons++;
        else if (st == UR_PHOTON) photons++;
    }

    // Physikalische SI-Metriken berechnen
    double total_mev = (double)pu->dynamic_invariance_target * SKALAR_MEV_PER_INVARIANCE;
    double total_joule = total_mev * MEV_TO_JOULE;

    bool inv_ok = ProPhysics_Verify_Invariance(pu);
    printf(" Teilchen-Status : Protonen: %u | Elektronen: %u | Photonen: %u\n", protons, electrons, photons);
    if (current_dist_fm >= 0.0) {
        printf(" Kern-Abstand    : %.1f fm (10^-15 m)\n", current_dist_fm);
    }
    else {
        printf(" Kern-Abstand    : FUSED / GEBUNDEN (< 2.0 fm)\n");
    }
    printf(" System-Energie  : %.2f MeV  (%.3e J)  [Invarianz: %llu Bits]\n",
        total_mev, total_joule, (unsigned long long)pu->dynamic_invariance_target);
    printf(" Bit-Konservierung: [%s]\n",
        inv_ok ? "PASSED - 100% BIT CONSERVED" : "VIOLATION DETECTED");
    printf("====================================================================\n");
}

int main(void) {
    printf("====================================================================\n");
    printf(" PROPHYSICS SDK :: OBSERVABLE FUSION VISUALIZER (4x H1 -> He2 + 2*) \n");
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
        printf("Frame-Verzoegerung in ms [z.B. 80 fuer schnell, 200 fuer langsam]: ");
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

        // --- PHASE 1: Vier Wasserstoff-Kerne (4x H1) erzeugen & annähern ---
        pu.dynamic_invariance_target = 20;

        uint32_t base_c1 = 8, base_e1 = 12;
        uint32_t base_c2 = 16, base_e2 = 20;
        uint32_t base_c3 = 44, base_e3 = 40;
        uint32_t base_c4 = 56, base_e4 = 52;

        for (int step = 0; step < 10; step++) {
            // 1. Grid säubern
            for (uint32_t i = 0; i < GRID_SIZE; i++) {
                pu.ur_grid[i].type_state = UR_NEUTRAL;
                pu.ur_grid[i].field_helicity = 0;
                pu.reg_source[i].channels[0] = i;
                pu.reg_source[i].channels[1] = i;
            }

            // 2. Positionen berechnen
            uint32_t c1 = base_c1, e1 = base_e1;
            uint32_t c2 = base_c2 + step, e2 = base_e2 + step;
            uint32_t c3 = base_c3 - step, e3 = base_e3 - step;
            uint32_t c4 = base_c4, e4 = base_e4;

            // Atom 1
            pu.ur_grid[c1].type_state = UR_POSITRON_CW;
            pu.ur_grid[e1].type_state = UR_NEGATRON_CCW;
            ProPhysics_Link_Nodes(&pu, e1, e1, 0); ProPhysics_Link_Nodes(&pu, e1, c1, 1);
            ProPhysics_Link_Nodes(&pu, c1, c1, 0); ProPhysics_Link_Nodes(&pu, c1, e1, 1);

            // Atom 2 (wandert)
            pu.ur_grid[c2].type_state = UR_POSITRON_CCW;
            pu.ur_grid[e2].type_state = UR_NEGATRON_CW;
            ProPhysics_Link_Nodes(&pu, e2, e2, 0); ProPhysics_Link_Nodes(&pu, e2, c2, 1);
            ProPhysics_Link_Nodes(&pu, c2, c2, 0); ProPhysics_Link_Nodes(&pu, c2, e2, 1);

            // Atom 3 (wandert)
            pu.ur_grid[c3].type_state = UR_POSITRON_CW;
            pu.ur_grid[e3].type_state = UR_NEGATRON_CCW;
            ProPhysics_Link_Nodes(&pu, e3, e3, 0); ProPhysics_Link_Nodes(&pu, e3, c3, 1);
            ProPhysics_Link_Nodes(&pu, c3, c3, 0); ProPhysics_Link_Nodes(&pu, c3, e3, 1);

            // Atom 4
            pu.ur_grid[c4].type_state = UR_POSITRON_CCW;
            pu.ur_grid[e4].type_state = UR_NEGATRON_CW;
            ProPhysics_Link_Nodes(&pu, e4, e4, 0); ProPhysics_Link_Nodes(&pu, e4, c4, 1);
            ProPhysics_Link_Nodes(&pu, c4, c4, 0); ProPhysics_Link_Nodes(&pu, c4, e4, 1);

            // Engine-Tick zur Feld-Induktion & Lorentz-Sicherung ausführen
            ProPhysics_Execute_Tick(&pu);

            char title[64];
            snprintf(title, sizeof(title), "Isoliertes System (4x H1 Annaeherung)");
            double dist_fm = (double)(c3 - c2) * SKALAR_FEMTOMETER_PER_CELL;

            Render_Grid_Visualizer(&pu, step, title, cycle_counter, dist_fm);

            if (step_by_step) {
                printf(" Press [ENTER] for next tick...");
                getchar();
            }
            else {
                SLEEP_MS(frame_delay_ms);
            }
        }

        // --- PHASE 2: FUSION TRIGGER & INITIALES ROUTING ---
        for (uint32_t i = 0; i < GRID_SIZE; i++) {
            pu.ur_grid[i].type_state = UR_NEUTRAL;
            pu.ur_grid[i].field_helicity = 0;
            pu.reg_source[i].channels[0] = i;
            pu.reg_source[i].channels[1] = i;
        }

        // Gebundener Helium-Kern (He2)
        uint32_t he_core_1 = 31, he_core_2 = 32;
        uint32_t he_elec_1 = 28, he_elec_2 = 35;

        pu.ur_grid[he_core_1].type_state = UR_POSITRON_CW;
        pu.ur_grid[he_core_2].type_state = UR_POSITRON_CCW;
        pu.ur_grid[he_elec_1].type_state = UR_NEGATRON_CCW;
        pu.ur_grid[he_elec_2].type_state = UR_NEGATRON_CW;

        ProPhysics_Link_Nodes(&pu, he_elec_1, he_elec_2, 0);
        ProPhysics_Link_Nodes(&pu, he_elec_1, he_core_1, 1);
        ProPhysics_Link_Nodes(&pu, he_elec_2, he_elec_1, 0);
        ProPhysics_Link_Nodes(&pu, he_elec_2, he_core_2, 1);

        // Abgestrahlte Photonen initialisieren
        uint32_t photon_left = 22;
        uint32_t photon_right = 41;
        pu.ur_grid[photon_left].type_state = UR_PHOTON;
        pu.ur_grid[photon_right].type_state = UR_PHOTON;

        // Register-Kanal 0: Richtungs-Vektor für Torus-Welle setzen
        ProPhysics_Link_Nodes(&pu, photon_left, (photon_left == 0) ? (GRID_SIZE - 1) : (photon_left - 1), 0);
        ProPhysics_Link_Nodes(&pu, photon_right, (photon_right + 1) % GRID_SIZE, 0);

        // Register-Kanal 1: Ur-Heliumquelle zur Invarianz-Verankerung
        ProPhysics_Link_Nodes(&pu, photon_left, he_core_1, 1);
        ProPhysics_Link_Nodes(&pu, photon_right, he_core_2, 1);

        // --- PHASE 3: EXPANDIERENDE PHOTONEN-WELLEN ÜBER ENGINE-TICKS ---
        for (uint32_t tick = 10; tick <= 30; tick++) {
            // 1. Positionen über Graph-Kanäle ermitteln
            uint64_t next_left = pu.reg_source[photon_left].channels[0];
            uint64_t next_right = pu.reg_source[photon_right].channels[0];

            // 2. Altem Knoten Zustand entziehen
            pu.ur_grid[photon_left].type_state = UR_NEUTRAL;
            pu.reg_source[photon_left].channels[0] = photon_left;

            pu.ur_grid[photon_right].type_state = UR_NEUTRAL;
            pu.reg_source[photon_right].channels[0] = photon_right;

            // 3. Zeiger aktualisieren
            photon_left = (uint32_t)next_left;
            photon_right = (uint32_t)next_right;

            // 4. Neuen Knoten mit Photonen-Zustand besetzen
            pu.ur_grid[photon_left].type_state = UR_PHOTON;
            pu.ur_grid[photon_right].type_state = UR_PHOTON;

            // 5. Nächsten Torus-Schritt im Routing hinterlegen
            ProPhysics_Link_Nodes(&pu, photon_left, (photon_left == 0) ? (GRID_SIZE - 1) : (photon_left - 1), 0);
            ProPhysics_Link_Nodes(&pu, photon_right, (photon_right + 1) % GRID_SIZE, 0);

            ProPhysics_Link_Nodes(&pu, photon_left, he_core_1, 1);
            ProPhysics_Link_Nodes(&pu, photon_right, he_core_2, 1);

            // 6. Engine-Tick zur Berechnung des Netzwerks ausführen
            ProPhysics_Execute_Tick(&pu);

            char title[64];
            snprintf(title, sizeof(title), "FUSION ERFOLGT! Photonen (*) strahlen ab");
            Render_Grid_Visualizer(&pu, tick, title, cycle_counter, -1.0);

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
            ProPhysics_Execute_Tick(&pu);

            char title[64];
            snprintf(title, sizeof(title), "Stabilisierter He2-Kern + Freie Strahlung");
            Render_Grid_Visualizer(&pu, 40, title, cycle_counter, -1.0);

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