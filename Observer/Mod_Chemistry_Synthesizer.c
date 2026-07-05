/* ==========================================================================
 * ProPhysics - Isolated Molecular Sandbox Synthesizer (Repariertes Core-Gating)
 * File: Mod_Chemistry_Synthesizer.c
 * Optimierung: Thread-safe Sparse-Tracking Injektion & Invarianz-Abdichtung
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "ProPhysics.h"
#include "ProDiBatch.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProDiBatch/ProDiBatch_Exports.h"

#ifdef _MSC_VER
#include <intrin.h>
#define ZAEHLE_BITS(x) __popcnt64(x)
#else
#define ZAEHLE_BITS(x) __builtin_popcountll(x)
#endif

extern ModuleTestControl global_mod_control[];

typedef struct {
    uint8_t atomic_number;
    uint8_t native_valency;
    uint16_t base_mass_bits;
    char symbol[3];
} ElementAxiom;

static const ElementAxiom PERIODIC_TABLE[119] = {
    { 0,  0, 0,   "Vak" }, { 1,  1, 3,   "H"   }, { 2,  0, 12,  "He"  },
    { 6,  4, 36,  "C"   }, { 7,  3, 42,  "N"   }, { 8,  2, 48,  "O"   },
    { 11, 1, 69,  "Na"  }, { 17, 1, 105, "Cl"  }, { 79, 1, 591, "Au"  }
};

static bool sandbox_active = false;
static uint64_t accumulated_synthesized_bonds = 0;
static uint64_t broken_bonds_by_thermo_pressure = 0;

/* ==========================================================================
 * SUB-ROUTINE: Temporäre Evakuierung oder Wiederherstellung der Masseninseln
 * ========================================================================== */
static void toggle_universe_islands(ProUniverse* universe, bool evacuate) {
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t slot = universe->grid[idx].state_island_idx;

        if (slot != 0) {
            uint64_t polarity = universe->data_pool[slot].charge_spin & QUANTUM_MASK_POLARITY;
            if (polarity == QUANTUM_POL_PLUS || polarity == QUANTUM_POL_MINUS) {
                if (evacuate) {
                    universe->grid[idx].state_island_idx = 0;
                    universe->grid[idx].reserved_backup_flags = (uint16_t)slot;
                    universe->active_nodes_kinetic[idx] = 0;
                }
                else {
                    uint32_t backup_slot = universe->grid[idx].reserved_backup_flags;
                    if (backup_slot != 0) {
                        universe->grid[idx].state_island_idx = backup_slot;
                        universe->grid[idx].reserved_backup_flags = 0;
                        universe->active_nodes_kinetic[idx] = 0x1000; // Insel-Masse wiederherstellen
                    }
                }
            }
        }
    }
}

/* ==========================================================================
 * REPARIERT: Kontrollierte Injektion mit bitgenauer Registrierung im SoA-Core
 * ========================================================================== */
static void inject_sandbox_elements(ProUniverse* universe, const ElementAxiom* elemA, const ElementAxiom* elemB, double target_P, uint32_t target_T) {
    int start_x = 300, end_x = 700;
    int cy = 256, cz = 256;
    (void)elemA; (void)elemB;

    for (int x = start_x; x < end_x; x += 4) {
        uint32_t idx = x | (cy << 10) | (cz << 19);

        if (universe->grid[idx].state_island_idx == 0) {
            uint32_t temp_slot = (x % 2 == 0) ? 1 : 2;
            universe->grid[idx].state_island_idx = temp_slot;

            uint32_t initial_flux = 0;
            for (uint32_t t = 0; t < target_T; t++) {
                initial_flux |= (1U << (t % 12));
            }

            if (target_P > 1.2) {
                initial_flux |= 0x055;
            }

            universe->active_nodes_kinetic[idx] = initial_flux;

            // --- REPARATUR-KRAFTSTOSS: RECHTEINTRAG IN DEN HOST-STRAM ---
            // Schreibt die Adresse zwingend in das Bitset und registriert den Knoten für die Worker!
            if (!(universe->node_active_bitset[idx >> 3] & (1 << (idx & 7)))) {
                universe->node_active_bitset[idx >> 3] |= (1 << (idx & 7));
                if (universe->active_count_current < MAX_SPARSE_TRACKING_NODES) {
                    universe->active_nodes_current[universe->active_count_current++] = idx;
                }
            }
        }
    }

    // Erzwingt die sofortige Neuverteilung der Chunks auf die 10 Worker-Threads
    ProPhysics_Partition_Initial_Tasks(universe);
}

/* ==========================================================================
 * MOLEKULARER SANDBOX SYNTHESIZER
 * ========================================================================== */
void chemistry_synthesize_elements(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    ProUniverse* mut_universe = (ProUniverse*)universe;
    uint32_t test_id = global_mod_control[MOD_INDEX_CHEMISTRY].active_test_id;

    if (test_id != 4 && sandbox_active) {
        ProDiBatch_Log(db_engine, "[SANDBOX] Beende Laborbetrieb. Invertiere Gittertopologie...");

        // Lösche verbliebene Gasreste cache-konform
        for (uint64_t i = 0; i < mut_universe->active_count_current; i++) {
            uint32_t idx = mut_universe->active_nodes_current[i];
            if (mut_universe->grid[idx].reserved_backup_flags == 0) {
                mut_universe->active_nodes_kinetic[idx] = 0;
                mut_universe->grid[idx].state_island_idx = 0;
            }
        }

        toggle_universe_islands(mut_universe, false);
        ProPhysics_Partition_Initial_Tasks(mut_universe);
        sandbox_active = false;
        return;
    }

    if (test_id != 4) return;

    uint32_t element_a_idx = (uint32_t)global_mod_control[MOD_INDEX_CHEMISTRY].target_intensity;
    int32_t element_b_idx = global_mod_control[MOD_INDEX_CHEMISTRY].custom_param;

    if (element_a_idx >= 119 || element_b_idx >= 119) return;

    const ElementAxiom* elemA = &PERIODIC_TABLE[element_a_idx];
    const ElementAxiom* elemB = &PERIODIC_TABLE[element_b_idx];

    if (!sandbox_active) {
        toggle_universe_islands(mut_universe, true);

        double user_P = global_mod_control[MOD_INDEX_THERMODYNAMICS].target_intensity;
        int32_t user_T = global_mod_control[MOD_INDEX_THERMODYNAMICS].custom_param;
        if (user_P < 0.1) user_P = 1.3;
        if (user_T <= 0) user_T = 4;

        ProDiBatch_Log(db_engine, "[SANDBOX] Injektiere Gaswolke: %s + %s (P:%.2f, T:%d)",
            elemA->symbol, elemB->symbol, user_P, user_T);

        inject_sandbox_elements(mut_universe, elemA, elemB, user_P, (uint32_t)user_T);
        sandbox_active = true;
        return;
    }

    accumulated_synthesized_bonds = 0;
    broken_bonds_by_thermo_pressure = 0;

    for (uint64_t i = 0; i < mut_universe->active_count_current; i++) {
        uint32_t idx = mut_universe->active_nodes_current[i];
        uint32_t flux = mut_universe->active_nodes_kinetic[idx];
        uint32_t slot = mut_universe->grid[idx].state_island_idx;

        if (slot == 0) continue;

        int32_t x = idx & X_MASK;
        int32_t y = (idx >> Y_SHIFT) & Y_MASK;
        int32_t z = idx >> Z_SHIFT;

        double local_P = ProPhysics_Query_Local_Pressure(mut_universe, x, y, z, 1);
        uint32_t local_T_bits = (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);

        if (local_P > 1.4 && local_T_bits >= 2 && local_T_bits <= 6) {
            uint32_t allocated_bonds = (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);
            if (allocated_bonds < (12U - elemA->native_valency)) {
                mut_universe->active_nodes_kinetic[idx] |= 0x1000;
                accumulated_synthesized_bonds++;
            }
        }

        if (flux & 0x1000) {
            uint32_t dissociation_threshold = 8U - (elemA->native_valency);
            if (local_T_bits > dissociation_threshold) {
                mut_universe->active_nodes_kinetic[idx] &= ~0x1000;
                broken_bonds_by_thermo_pressure++;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (mut_universe->current_cpu_tick % 50 == 0) {
        ProDiBatch_Log(db_engine, "[SANDBOX-LIVE] Verbindungen: %llu | Kaskaden-Brüche: %llu",
            accumulated_synthesized_bonds, broken_bonds_by_thermo_pressure);
    }
#endif
}