/**
 * @file example_Fusion.c
 * @brief ProPhysics SDK Demo: Interaktive topologische Kernfusion mit Physik-Kalibrierung (SI/MeV-Mapping)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ProPhysics.h"
#include "ProPhysics_Types.h"

#define DEMO_GRID_SIZE      1024
#define DEFAULT_H1_RADIUS   8

 // --- PHYSIGALISCHE KALIBRIERUNGS-KONSTANTEN ---
#define SKALAR_MEV_PER_INVARIANCE 3.518      // 1 Invarianz-Punkt = 3.518 MeV
#define SKALAR_FEMTOMETER_PER_CELL 1.0       // 1 Grid-Zelle = 1.0 Femtometer (10^-15 m)
#define MEV_TO_JOULE              1.60218e-13 // 1 MeV in Joule
#define KELVIN_PER_MEV            1.16045e10  // 1 MeV in Kelvin (kB-Umrechnung)

uint64_t Inject_H1(ProUniverse* pu, uint64_t addr, uint64_t radius) {
    pu->ur_grid[addr].type_state = UR_POSITRON_CW;
    pu->reg_source[addr].channels[0] = addr;

    uint64_t e_addr = addr + radius;
    pu->ur_grid[e_addr].type_state = UR_NEGATRON_CCW;
    pu->reg_source[e_addr].channels[0] = e_addr;
    pu->reg_source[e_addr].channels[1] = addr;

    return addr;
}

bool Execute_Topological_Fusion(ProUniverse* pu, uint64_t h1_addr, uint64_t h2_addr, uint64_t radius) {
    uint64_t e1_old = h1_addr + radius;
    uint64_t e2_old = h2_addr + radius;

    // 0. Gesamten alten Raumzustand neutralisieren
    pu->ur_grid[h1_addr].type_state = UR_NEUTRAL;
    pu->ur_grid[e1_old].type_state = UR_NEUTRAL;
    pu->ur_grid[h2_addr].type_state = UR_NEUTRAL;
    pu->ur_grid[e2_old].type_state = UR_NEUTRAL;

    // 1. Kern-Faltung: Helium-2 Doppelattraktor
    pu->ur_grid[h1_addr].type_state = UR_POSITRON_CW;
    pu->ur_grid[h1_addr + 1].type_state = UR_POSITRON_CCW;

    pu->reg_source[h1_addr].channels[0] = h1_addr + 1;
    pu->reg_source[h1_addr + 1].channels[0] = h1_addr;

    // 2. K-Schalen-Reorganisation (Spin-Paarung)
    uint64_t e1_new = h1_addr + radius;
    uint64_t e2_new = h1_addr + radius + 1;

    pu->ur_grid[e1_new].type_state = UR_NEGATRON_CCW;
    pu->ur_grid[e2_new].type_state = UR_NEGATRON_CW;

    pu->reg_source[e1_new].channels[0] = e2_new;
    pu->reg_source[e1_new].channels[1] = h1_addr;
    pu->reg_source[e2_new].channels[0] = e1_new;
    pu->reg_source[e2_new].channels[1] = h1_addr;

    // 3. Photonen-Emission
    uint64_t photon_addr = h1_addr + radius + 2;
    if (photon_addr < pu->total_nodes) {
        pu->ur_grid[photon_addr].type_state = UR_PHOTON;
        pu->reg_source[photon_addr].channels[0] = photon_addr;
    }

    return true;
}

int main(void) {
    printf("====================================================================\n");
    printf(" PROPHYSICS SDK :: KERNFUSION & PHYSIVALISCHE KALIBRIERUNG (SI/MeV)\n");
    printf("====================================================================\n\n");

    printf("Waehlen Sie das Fusions-Szenario:\n");
    printf(" [1] Kinetische Kompression (Atome naehern sich dynamisch an)\n");
    printf(" [2] Thermonukleare Schock-Fusion (Sofortige Zündung & Gamma-Emission)\n");
    printf(" [3] Interaktive Sandbox (Alle Parameter frei konfiguriert)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;

    uint64_t h1_start_addr = 100;
    uint64_t h2_start_addr = 160;
    uint64_t shell_radius = DEFAULT_H1_RADIUS;
    uint64_t fusion_trigger_dist = 10;
    int total_ticks = 60;
    bool auto_move = true;

    if (choice == 2) {
        h2_start_addr = h1_start_addr + fusion_trigger_dist;
        auto_move = false;
    }
    else if (choice == 3) {
        printf("\n--- SANDBOX PARAMS ---\n");
        printf("Start-Adresse H1-Kern [z.B. 100]: ");
        if (scanf("%llu", &h1_start_addr) != 1) h1_start_addr = 100;

        printf("Start-Adresse H2-Kern [z.B. 180]: ");
        if (scanf("%llu", &h2_start_addr) != 1) h2_start_addr = 180;

        printf("Elektronen-Hüllen-Radius [z.B. 8]: ");
        if (scanf("%llu", &shell_radius) != 1) shell_radius = 8;

        printf("Fusions-Distanzschwelle [z.B. 10]: ");
        if (scanf("%llu", &fusion_trigger_dist) != 1) fusion_trigger_dist = 10;

        if (fusion_trigger_dist <= shell_radius) {
            fusion_trigger_dist = shell_radius + 2;
            printf(" -> Auto-Korrektur: Fusions-Distanz auf %llu angepasst (muss > Radius sein).\n",
                (unsigned long long)fusion_trigger_dist);
        }

        printf("Automatischer Vortrieb pro Tick? (1 = Ja, 0 = Nein): ");
        int am = 1;
        if (scanf("%d", &am) != 1) am = 1;
        auto_move = (am != 0);

        printf("Anzahl Ticks (1-200) [z.B. 60]: ");
        if (scanf("%d", &total_ticks) != 1 || total_ticks < 1) total_ticks = 60;
    }

    ProUniverse pu;
    ProPhysics_Initialize(&pu, DEMO_GRID_SIZE);

    Inject_H1(&pu, h1_start_addr, shell_radius);
    Inject_H1(&pu, h2_start_addr, shell_radius);

    uint64_t base_invariance = 10;
    double energy_delta_mev = 5 * SKALAR_MEV_PER_INVARIANCE;
    double energy_delta_joule = energy_delta_mev * MEV_TO_JOULE;

    printf("\n[INITIALISIERUNG ERFOLGREICH]\n");
    printf(" - H1_A Kern @ #%llu, Huelle @ #%llu\n", h1_start_addr, h1_start_addr + shell_radius);
    printf(" - H1_B Kern @ #%llu, Huelle @ #%llu\n", h2_start_addr, h2_start_addr + shell_radius);
    printf(" - Basis-Invarianz: %llu -> Fusions-Invarianz: %llu (+5 Delta)\n",
        (unsigned long long)base_invariance, (unsigned long long)(base_invariance + 5));
    printf(" - KALIBRIERUNG: 1 Grid-Unit = %.1f fm | 1 Invarianz = %.3f MeV\n",
        SKALAR_FEMTOMETER_PER_CELL, SKALAR_MEV_PER_INVARIANCE);
    printf(" - Erwartete Synthese-Energieausbeute (Gamma): %.2f MeV (%.3e Joule)\n\n",
        energy_delta_mev, energy_delta_joule);

    printf("=========================================================================================================\n");
    printf("%-5s | %-8s | %-12s | %-12s | %-18s | %-16s\n",
        "Tick", "H2-Pos", "Abstand (fm)", "Status", "Invarianz (Target)", "Real-Äquivalent");
    printf("=========================================================================================================\n");

    bool fused = false;
    bool fusion_just_occurred = false;
    uint64_t current_h2_addr = h2_start_addr;

    for (int tick = 1; tick <= total_ticks; tick++) {
        uint64_t dist = (current_h2_addr > h1_start_addr) ? (current_h2_addr - h1_start_addr) : 0;
        double dist_fm = (double)dist * SKALAR_FEMTOMETER_PER_CELL;
        fusion_just_occurred = false;

        if (!fused && dist <= fusion_trigger_dist) {
            Execute_Topological_Fusion(&pu, h1_start_addr, current_h2_addr, shell_radius);
            fused = true;
            fusion_just_occurred = true;
        }

        pu.dynamic_invariance_target = fused ? (base_invariance + 5) : base_invariance;

        ProPhysics_Execute_Tick(&pu);

        if (!fused && auto_move && current_h2_addr > (h1_start_addr + fusion_trigger_dist)) {
            pu.ur_grid[current_h2_addr].type_state = UR_NEUTRAL;
            pu.ur_grid[current_h2_addr + shell_radius].type_state = UR_NEUTRAL;

            current_h2_addr--;
            Inject_H1(&pu, current_h2_addr, shell_radius);
        }

        bool inv_ok = ProPhysics_Verify_Invariance(&pu);
        const char* status_str = fused ? "HELIUM-2 (Fused)" : "ISOLATED";

        char phys_str[32];
        if (fused) {
            snprintf(phys_str, sizeof(phys_str), "+%.2f MeV (Gamma)", energy_delta_mev);
        }
        else {
            // Ungefähre kinetische/Coulomb-Äquivalenz basierend auf Grid-Kompression
            double est_temp_k = (10.0 / dist_fm) * 1e7;
            snprintf(phys_str, sizeof(phys_str), "~%.1fe6 K (Plasma)", est_temp_k / 1e6);
        }

        if (tick == 1 || tick % 10 == 0 || fusion_just_occurred) {
            printf("#%-4d | #%-6llu | %-12.1f | %-12s | %-3llu (%-2llu) [%s] | %s\n",
                tick,
                current_h2_addr,
                dist_fm,
                status_str,
                (unsigned long long)pu.dynamic_invariance_target,
                (unsigned long long)pu.dynamic_invariance_target,
                inv_ok ? "OK" : "ERR",
                phys_str);
        }
    }

    printf("=========================================================================================================\n\n");
    printf("[SIMULATION BEENDET] Synthese: %s\n", fused ? "Helium-2 vollzogen" : "Keine Fusion");
    printf(" -> Energetische Bilanz: Total %.2f MeV (%.3e J) ins Gitter emittiert.\n",
        fused ? energy_delta_mev : 0.0, fused ? energy_delta_joule : 0.0);

    ProPhysics_Free(&pu);
    return EXIT_SUCCESS;
}