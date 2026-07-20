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

    // 1. Target-Register vorab im RAM nullen für das bitweise OR
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

    // 2. Zustandswandlung und Pointer-Kaskadierung über flaches Feld
    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
        uint8_t current_type = pu->ur_grid[idx].type_state;
        if (current_type == UR_NEUTRAL) continue;

        ProPointerRegister current_reg = pu->reg_source[idx];
        ProPointerRegister next_reg = current_reg;

        // --- FORMEL 2: BRANCHLESS BITMANIPULATION DER POINTER ---
        // spin_direction ist strikt 0 oder 1.
        uint64_t spin_direction = (current_type & 1U);
        uint64_t is_photon = (current_type == UR_PHOTON);
        spin_direction = spin_direction * (!is_photon);

        // Sicheres, branchless Shifting ohne Undefined Behavior (Vermeidung von >> 64)
        // Wenn spin_direction == 1, wird das MSB von ch+1 um 63 Stellen nach rechts geshiftet (auf Bit 0)
        // Wenn spin_direction == 0, löscht die Multiplikation den rechten Term komplett aus
        for (int ch = 0; ch < 3; ch++) {
            next_reg.channels[ch] = (current_reg.channels[ch] << spin_direction) |
                ((current_reg.channels[ch + 1] >> 63) * spin_direction);
        }

        // --- FORMEL 4: WECHSELWIRKUNG, ANNIHILATION & STREUUNG (Lokal) ---
        uint64_t target_ptr = next_reg.channels[0];

        if (target_ptr < pu->total_nodes && target_ptr != idx) {
            uint8_t target_type = pu->ur_grid[target_ptr].type_state;

            // Kriterium 1: Klassische Annihilation (Positron + Negatron = 5)
            uint64_t is_annihilation = ((current_type + target_type) == 5) && (current_type != target_type);

            // Kriterium 2: Option B - Photonen-Streuung / Impuls-Übertrag
            uint64_t is_photon_impact = (current_type == UR_PHOTON) && (target_type != UR_NEUTRAL) && (target_type != UR_PHOTON);

            // BEHOBEN: Expliziter Cast auf uint8_t eliminiert C4244
            uint8_t next_type = (uint8_t)((UR_PHOTON * is_annihilation) |
                (target_type * is_photon_impact) |
                (current_type * (!is_annihilation && !is_photon_impact)));
            pu->ur_grid[idx].type_state = next_type;

            // BEHOBEN: Expliziter Cast auf uint8_t eliminiert C4244
            uint8_t next_target_type = (uint8_t)((target_type * (!is_annihilation && !is_photon_impact)) |
                (UR_NEUTRAL * is_annihilation) |
                (UR_PHOTON * is_photon_impact));
            pu->ur_grid[target_ptr].type_state = next_target_type;

            // --- FORMEL 5: DISKRETE SUMMATION PER OR (Kanal-Verwebung & Ablenkung) ---
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[target_ptr].channels[ch] |= next_reg.channels[ch];
            }
        }
        else {
            // BEHOBEN: Akkumulation per bitweisem OR statt harter Zuweisung.
            // Verhindert das Überschreiben von Informationen, die in diesem Tick 
            // bereits von anderen emittierenden Knoten hierher gestreut wurden.
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= next_reg.channels[ch];
            }
        }

        net_momentum += (current_type == UR_POSITRON_CW) ? 1 : 0;
    }

    // Buffer-Tausch für die Adress-Kanäle
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