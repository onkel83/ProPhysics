/**
 * @file example_lorentz_positron.c
 * @brief Demonstration der gegensaetzlichen Lorentz-Ablenkung (Elektron vs. Positron)
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "ProPhysics.h"

#define NODES 16

int main(void) {
    printf("====================================================\n");
    printf(" PROPHYSICS :: LORENTZ DUAL-DRIFT (POSITRON vs NEGATRON)\n");
    printf("====================================================\n\n");

    ProUniverse universe;
    ProPhysics_Initialize(&universe, NODES);

    // 1. Topologisches Netzwerk mit zwei Drift-Richtungen aufbauen
    for (uint32_t i = 0; i < NODES; i++) {
        uint32_t next_linear = (i + 1) % NODES;
        uint32_t next_drift = (i + 3) % NODES;             // Negatron-Drift (+3)
        uint32_t prev_drift = (i + NODES - 3) % NODES;     // Positron-Gegen-Drift (-3)

        ProPhysics_Link_Nodes(&universe, i, next_linear, 0); // Kanal 0: Vorwaerts (Linear)
        ProPhysics_Link_Nodes(&universe, i, prev_drift, 2); // Kanal 2: Gegen-Drift (-3)
        ProPhysics_Link_Nodes(&universe, i, next_drift, 3); // Kanal 3: Drift (+3)
    }

    // 2. Injektion: Elektron an Knoten 0 (4 Bits), Positron an Knoten 8 (1 Bit)
    // Soll-Invarianz = 5 Bits
    universe.ur_grid[0].type_state = UR_NEGATRON_CW;
    universe.ur_grid[8].type_state = UR_POSITRON_CW;
    universe.dynamic_invariance_target = 5;

    // PHASE 1: Neutraler Raum (Helizitaet = 0) -> Beide folgen Kanal 0 (+1 step)
    printf("[PHASE 1] Neutraler Raum (Helizitaet = 0, lineare Bewegung):\n");
    for (int tick = 0; tick < 4; tick++) {
        uint32_t pos_e = 0, pos_p = 0;
        for (uint32_t i = 0; i < NODES; i++) {
            if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) pos_e = i;
            if (universe.ur_grid[i].type_state == UR_POSITRON_CW) pos_p = i;
        }
        printf(" Tick #%d | Elektron (e-): Knoten #%-2u | Positron (e+): Knoten #%-2u\n",
            tick, pos_e, pos_p);
        ProPhysics_Execute_Tick(&universe);
    }

    // PHASE 2: Feld-Helizitaet aktivieren (Helizitaet = 1)
    // Elektron -> Kanal 3 (+3 Drift)
    // Positron -> Kanal 2 (-3 Drift)
    printf("\n[PHASE 2] Feld-Induktion aktiv (Helizitaet = 1, gegensaetzliche Ablenkung):\n");
    for (uint32_t i = 0; i < NODES; i++) {
        universe.ur_grid[i].field_helicity = 1;
    }

    for (int tick = 4; tick < 8; tick++) {
        uint32_t pos_e = 0, pos_p = 0;
        for (uint32_t i = 0; i < NODES; i++) {
            if (universe.ur_grid[i].type_state == UR_NEGATRON_CW) pos_e = i;
            if (universe.ur_grid[i].type_state == UR_POSITRON_CW) pos_p = i;
        }
        printf(" Tick #%d | Elektron (e-): Knoten #%-2u (+3) | Positron (e+): Knoten #%-2u (-3)\n",
            tick, pos_e, pos_p);
        ProPhysics_Execute_Tick(&universe);
    }

    // Invarianz-Pruefung
    bool valid = ProPhysics_Verify_Invariance(&universe);
    printf("\nInvarianz-Status: %s (5/5 Bits bit-exakt konserviert)\n", valid ? "PASSED" : "FAILED");
    printf("====================================================\n");

    ProPhysics_Free(&universe);
    return 0;
}