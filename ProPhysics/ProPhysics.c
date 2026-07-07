/* ==========================================================================
 * ProPhysics - Engine Kernel Execution (Pure Wheeler Information-Engine)
 * File: ProPhysics.c
 * Architektur: Sequentielle Dual-Register Abarbeitung, Branchless Bit-Shifts
 * ========================================================================== */

#include "ProPhysics.h"
#include <stdlib.h>
#include <string.h>

 /* ==========================================================================
  * ProPhysics_Initialize
  * Allokiert das flache Urtypen-Feld und die zwei gespiegelten 4-GB-Register
  * ========================================================================== */
PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count) {
    if (!pu) return;

    pu->total_nodes = node_count;
    pu->current_cpu_tick = 0;

    // Flaches, hochverdichtetes Feld für die Urtypen
    pu->ur_grid = (ProUrNode*)malloc(node_count * sizeof(ProUrNode));
    memset(pu->ur_grid, 0, node_count * sizeof(ProUrNode));

    // Die zwei getrennten Register für das sequentielle Lese-/Schreib-Wechselspiel
    pu->reg_source = (ProPointerRegister*)malloc(node_count * sizeof(ProPointerRegister));
    pu->reg_target = (ProPointerRegister*)malloc(node_count * sizeof(ProPointerRegister));

    memset(pu->reg_source, 0, node_count * sizeof(ProPointerRegister));
    memset(pu->reg_target, 0, node_count * sizeof(ProPointerRegister));

    pu->dynamic_invariance_target = 0;
    pu->global_entropy_index = 0.0;
}

/* ==========================================================================
 * ProPhysics_Execute_Tick (Die 5 Urformeln auf dem nackten Blech)
 * ========================================================================== */
PROPHYSICS_API void ProPhysics_Execute_Tick(ProUniverse* pu) {
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->reg_target) return;

    pu->current_cpu_tick++;
    uint64_t net_momentum = 0;

    // Sequentielle Abarbeitung Schritt für Schritt - St+1 = f(St)
    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {

        // --- FORMEL 1 & 3: MENGENLEHRE & DISKRETE SYSTEMDYNAMIK ---
        uint8_t current_type = pu->ur_grid[idx].type_state;
        if (current_type == UR_NEUTRAL) continue;

        // --- FORMEL 2: FUNKTIONALE TOPOLOGIE (Pointer extrahieren per Maske/Shift) ---
        // Durch die 12er-Symmetrie manipulieren wir die 256-Bit Kanäle extrem rasant
        ProPointerRegister current_reg = pu->reg_source[idx];
        ProPointerRegister next_reg = current_reg;

        // Beispiel für branchless Bitmanipulation der Pointer-Kanäle basierend auf dem Spin
        uint64_t spin_direction = (current_type & 1U); // Evaluierung CW oder CCW

        for (int ch = 0; ch < 3; ch++) {
            // Schnelle Bit-Shifts zur Neuausrichtung der Adressat-Segmente ohne Conditionals
            next_reg.channels[ch] = (current_reg.channels[ch] << spin_direction) |
                (current_reg.channels[ch + 1] >> (64 - spin_direction));
        }

        // --- FORMEL 4: PRÄDIKATSSAMMLUNG & VERSCHRÄNKUNG ---
        // Pointer-auf-Pointer Kaskade auflösen (Verschränkungs-Check)
        uint64_t target_ptr = next_reg.channels[0];
        if (target_ptr < pu->total_nodes) {
            // Wenn der Pointer auf einen anderen gültigen Pointer zeigt -> Funktionale Symmetrie
            if (pu->ur_grid[target_ptr].type_state == current_type) {
                // Verschränkungs-Kopplung erzwingt Erhalt des Adressaten
                next_reg.channels[0] = current_reg.channels[0];
            }
        }

        // Write-Back in das isolierte Target-Register (Double-Buffering)
        pu->reg_target[idx] = next_reg;

        // Impuls-Akkumulation für die Entropie-Messung
        net_momentum += (current_type == UR_POSITRON_CW) ? 1 : 0;
    }

    // --- FORMEL 5: DISKRETE SUMMATION & INVARIANZ-FLIP ---
    // Buffer-Tausch (Ping-Pong-Verfahren) für den nächsten Zeitschritt
    ProPointerRegister* temp = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = temp;

    pu->global_entropy_index = (double)net_momentum;
}

/* ==========================================================================
 * ProPhysics_Verify_Invariance
 * Prüft bit-perfekt, ob im System Information verloren ging (Erhaltungsatz)
 * ========================================================================== */
PROPHYSICS_API bool ProPhysics_Verify_Invariance(const ProUniverse* pu) {
    if (!pu) return false;

    uint64_t current_sum = 0;
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        if (pu->ur_grid[i].type_state != UR_NEUTRAL) {
            current_sum += pu->ur_grid[i].type_state;
        }
    }
    return (current_sum == pu->dynamic_invariance_target);
}

/* ==========================================================================
 * ProPhysics_Free
 * ========================================================================== */
PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu) {
    if (!pu) return;
    if (pu->ur_grid) free(pu->ur_grid);
    if (pu->reg_source) free(pu->reg_source);
    if (pu->reg_target) free(pu->reg_target);
}