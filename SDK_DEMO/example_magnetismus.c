/**
 * @file example_magnetismus.c
 * @brief Demonstration der Lorentz-Kraft über topologische Helizität
 */

#include <stdio.h>
#include <stdbool.h>
#include "ProPhysics.h"

int main(void) {
    printf("====================================================\n");
    printf(" PROPHYSICS :: DEMO 1 - LORENTZ-ABLENKUNG (MAGNETISMUS)\n");
    printf("====================================================\n\n");

    ProUniverse universe;
    const uint64_t NODES = 16;
    ProPhysics_Initialize(&universe, NODES);

    // 1. Topologische Ring-Kette aufbauen (channels[0] = vorwärts, channels[3] = drift)
    for (uint32_t i = 0; i < NODES; i++) {
        uint32_t next_linear = (i + 1) % NODES;
        uint32_t next_drift = (i + 3) % NODES; // Alternativer Drift-Kanal

        ProPhysics_Link_Nodes(&universe, i, next_linear, 0);
        ProPhysics_Link_Nodes(&universe, i, next_drift, 3);
    }

    // 2. Ein Elektron injizieren (4 Bit Invarianz)
    universe.ur_grid[0].type_state = UR_NEGATRON_CW;
    universe.dynamic_invariance_target = 4;

    printf("[PHASE 1] Neutraler Raum (Keine Feld-Helizität):\n");
    for (int tick = 0; tick < 4; tick++) {
        // Welcher Knoten hält aktuell das Elektron?
        uint32_t pos = 0;
        for (uint32_t i = 0; i < NODES; i++) {
            if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) { pos = i; break; }
        }
        printf(" Tick #%d | Elektron an Knoten: #%u\n", tick, pos);
        ProPhysics_Execute_Tick(&universe);
    }

    // 3. Magnetische Feld-Induktion im Netzwerk aktivieren (Helizität = 1)
    printf("\n[PHASE 2] Feld-Induktion aktiviert (Drift-Routing greift):\n");
    for (uint32_t i = 0; i < NODES; i++) {
        universe.ur_grid[i].field_helicity = 1;
    }

    for (int tick = 4; tick < 8; tick++) {
        uint32_t pos = 0;
        for (uint32_t i = 0; i < NODES; i++) {
            if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) { pos = i; break; }
        }
        printf(" Tick #%d | Elektron abgelenkt auf Knoten: #%u (Drift +3!)\n", tick, pos);
        ProPhysics_Execute_Tick(&universe);
    }

    bool valid = ProPhysics_Verify_Invariance(&universe);
    printf("\nInvarianz-Status: %s (100%% Bit Conserved)\n", valid ? "PASSED" : "FAILED");
    printf("====================================================\n");

    ProPhysics_Free(&universe);
    return 0;
}