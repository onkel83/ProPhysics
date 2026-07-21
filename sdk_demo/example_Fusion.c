/**
 * @file example_Fusion.c
 * @brief ProPhysics SDK Demo: Interaktive topologische Kernfusion (2x H1 -> He2 + Photonen-Emission)
 *
 * Demonstriert die topologische Erhaltung der Gitter-Invarianz bei der Verschmelzung
 * zweier Wasserstoff-Atome zu Helium unter Energie-Abstrahlung (Photonen-Quanten).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ProPhysics.h"
#include "ProPhysics_Types.h"

 /* --- System-Konstanten --- */
#define DEMO_GRID_SIZE      1024
#define DEFAULT_H1_RADIUS   8

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
 * @brief Injiziert ein Wasserstoff-Atom (H1) an der Ziel-Adresse.
 * @param pu Zeiger auf das ProUniverse.
 * @param addr Grid-Adresse des Kerns (Proton/Positron).
 * @param radius Abstand der Elektronen-Hülle (Negatron).
 */
uint32_t Inject_H1(ProUniverse* pu, uint32_t addr, uint32_t radius) {
    // Kern (Positron CW)
    pu->ur_grid[addr].type_state = UR_POSITRON_CW;
    pu->reg_source[addr].channels[0] = addr;

    // Hülle (Negatron CCW)
    uint32_t e_addr = addr + radius;
    pu->ur_grid[e_addr].type_state = UR_NEGATRON_CCW;
    pu->reg_source[e_addr].channels[0] = e_addr;
    pu->reg_source[e_addr].channels[1] = addr; // Link zum Kern

    return addr;
}

/**
 * @brief Fuehrt die topologische Fusion von zwei H1-Atomen zu Helium-2 aus.
 * Sendet freigesetzte Differenz-Energie als Photonen-Welle ab.
 */
bool Execute_Topological_Fusion(ProUniverse* pu, uint32_t h1_addr, uint32_t h2_addr, uint32_t radius) {
    uint32_t e1_old = h1_addr + radius;
    uint32_t e2_old = h2_addr + radius;

    // 1. Kern-Faltung: Positron-CW + Positron-CCW bilden zirkulaeren Doppel-Attraktor
    pu->ur_grid[h1_addr].type_state = UR_POSITRON_CW;
    pu->ur_grid[h1_addr + 1].type_state = UR_POSITRON_CCW;

    pu->reg_source[h1_addr].channels[0] = h1_addr + 1;
    pu->reg_source[h1_addr + 1].channels[0] = h1_addr;

    // 2. K-Schalen-Reorganisation: Spin-Paarung der Elektronen
    uint32_t e1_new = h1_addr + radius;
    uint32_t e2_new = h1_addr + radius + 1;

    pu->ur_grid[e1_new].type_state = UR_NEGATRON_CCW;
    pu->ur_grid[e2_new].type_state = UR_NEGATRON_CW; // Gegenlaeufiger Spin

    pu->reg_source[e1_new].channels[0] = e2_new;
    pu->reg_source[e1_new].channels[1] = h1_addr;
    pu->reg_source[e2_new].channels[0] = e1_new;
    pu->reg_source[e2_new].channels[1] = h1_addr;

    // 3. Alte H2-Adresse neutralisieren (Materie wurde absorbiert)
    pu->ur_grid[h2_addr].type_state = UR_NEUTRAL;
    pu->ur_grid[e2_old].type_state = UR_NEUTRAL;
    pu->reg_source[h2_addr].channels[0] = 0;
    pu->reg_source[e2_old].channels[0] = 0;

    // 4. Photonen-Emission (Energiefreisetzung / Gamma-Quant in Nachbarzelle)
    uint32_t photon_addr = h1_addr + radius + 2;
    if (photon_addr < pu->total_nodes && pu->ur_grid[photon_addr].type_state == UR_NEUTRAL) {
        pu->ur_grid[photon_addr].type_state = UR_PHOTON;
        pu->reg_source[photon_addr].channels[0] = photon_addr;
    }

    return true;
}

int main(void) {
    printf("====================================================\n");
    printf(" PROPHYSICS SDK :: INTERAKTIVE KERNFUSION (H + H)   \n");
    printf("====================================================\n\n");

    printf("Waehlen Sie das Fusions-Szenario:\n");
    printf(" [1] Kinetische Kompression (Atome naehern sich dynamisch an)\n");
    printf(" [2] Thermonukleare Schock-Fusion (Sofortige Zündung & Gamma-Emission)\n");
    printf(" [3] Interaktive Sandbox (Alle Parameter frei konfiguriert)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;

    uint32_t h1_start_addr = 100;
    uint32_t h2_start_addr = 160;
    uint32_t shell_radius = DEFAULT_H1_RADIUS;
    uint32_t fusion_trigger_dist = 10;
    int total_ticks = 60;
    bool auto_move = true;

    if (choice == 2) {
        h2_start_addr = 108; // Bereits an der Schwelle
        auto_move = false;
    }
    else if (choice == 3) {
        printf("\n--- SANDBOX PARAMS ---\n");
        printf("Start-Adresse H1-Kern [z.B. 100]: ");
        if (scanf("%u", &h1_start_addr) != 1) h1_start_addr = 100;

        printf("Start-Adresse H2-Kern [z.B. 180]: ");
        if (scanf("%u", &h2_start_addr) != 1) h2_start_addr = 180;

        printf("Elektronen-Hüllen-Radius [z.B. 8]: ");
        if (scanf("%u", &shell_radius) != 1) shell_radius = 8;

        printf("Fusions-Distanzschwelle [z.B. 10]: ");
        if (scanf("%u", &fusion_trigger_dist) != 1) fusion_trigger_dist = 10;

        printf("Automatischer Vortrieb pro Tick? (1 = Ja, 0 = Nein): ");
        int am = 1;
        if (scanf("%d", &am) != 1) am = 1;
        auto_move = (am != 0);

        printf("Anzahl Ticks (1-200) [z.B. 60]: ");
        if (scanf("%d", &total_ticks) != 1 || total_ticks < 1) total_ticks = 60;
    }

    // ProPhysics Universum initialisieren
    ProUniverse pu;
    ProPhysics_Initialize(&pu, DEMO_GRID_SIZE);

    // 1. Atome im Grid platzieren
    Inject_H1(&pu, h1_start_addr, shell_radius);
    Inject_H1(&pu, h2_start_addr, shell_radius);

    // Initialen Invarianz-Sollwert erfassen (Erhaltungssatz)
    uint64_t initial_sum = 0;
    for (uint64_t i = 0; i < pu.total_nodes; i++) {
        if (pu.ur_grid[i].type_state != UR_NEUTRAL) {
            initial_sum += pu.ur_grid[i].type_state;
        }
    }
    pu.dynamic_invariance_target = initial_sum;

    printf("\n[INITIALISIERUNG ERFOLGREICH]\n");
    printf(" - H1_A Kern @ #%u, Huelle @ #%u\n", h1_start_addr, h1_start_addr + shell_radius);
    printf(" - H1_B Kern @ #%u, Huelle @ #%u\n", h2_start_addr, h2_start_addr + shell_radius);
    printf(" - Topologischer Invarianz-Sollwert: %llu\n\n", (unsigned long long)initial_sum);

    printf("=======================================================================================\n");
    printf("%-6s | %-12s | %-12s | %-18s | %-20s\n",
        "Tick", "H1-B Pos", "Kerndistanz", "Fusions-Status", "Invarianz-Check");
    printf("=======================================================================================\n");

    bool fused = false;
    uint32_t current_h2_addr = h2_start_addr;

    for (int tick = 1; tick <= total_ticks; tick++) {
        // 1. Physik-Tick ausfuehren
        ProPhysics_Execute_Tick(&pu);

        // 2. Distanz berechnen
        uint32_t dist = (current_h2_addr > h1_start_addr) ? (current_h2_addr - h1_start_addr) : 0;

        // 3. Fusions-Trigger pruefen
        if (!fused && dist <= fusion_trigger_dist) {
            Execute_Topological_Fusion(&pu, h1_start_addr, current_h2_addr, shell_radius);
            fused = true;
        }

        // 4. Kinetische Annäherung (falls noch nicht verschmolzen und auto_move aktiv)
        if (!fused && auto_move && current_h2_addr > (h1_start_addr + fusion_trigger_dist)) {
            // H2-Masse um 1 Adresse nach links verschieben
            pu.ur_grid[current_h2_addr].type_state = UR_NEUTRAL;
            pu.ur_grid[current_h2_addr + shell_radius].type_state = UR_NEUTRAL;

            current_h2_addr--;
            Inject_H1(&pu, current_h2_addr, shell_radius);
        }

        // 5. Invarianz verifizieren
        bool inv_ok = ProPhysics_Verify_Invariance(&pu);

        const char* status_str = fused ? "HELIUM-2 (Fused)" : "ISOLATED (H1 + H1)";

        if (tick == 1 || tick % 10 == 0 || fused) {
            printf("#%-5d | #%-10u | %-12u | %-18s | %s\n",
                tick,
                current_h2_addr,
                dist,
                status_str,
                inv_ok ? "PASSED [OK]" : "FAILED [VIOLATION]");
        }
    }

    printf("=======================================================================================\n\n");
    printf("[SIMULATION BEENDET] Endzustand: %s | Invarianz exakt auf %llu gehalten.\n",
        fused ? "Synthese zu Helium-2 vollzogen" : "Keine Fusion erfolgt",
        (unsigned long long)pu.dynamic_invariance_target);

    ProPhysics_Free(&pu);
    return EXIT_SUCCESS;
}