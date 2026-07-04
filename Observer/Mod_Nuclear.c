/* ==========================================================================
 * ProPhysics - Nuclear Physics Diagnostic Core
 * File: Mod_Nuclear.c
 * Architecture: C99, Cache-Optimized Monolithic Evaluator Layer
 * Optimierung: Monolithic Loop Fusion, O(1) Bit-Masking, Sparse Filtering
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

 // SDK-Schnittstellen aus den bin/ Verzeichnissen
#include "ProPhysics.h"
#include "ProDiBatch.h"

// Lokale Konfigurations-Axiome
#include "Observer_Version.h"
#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProDiBatch/ProDiBatch_Exports.h"

#ifdef _MSC_VER
#include <intrin.h>
#define ZAEHLE_BITS(x) __popcnt64(x)
#else
#define ZAEHLE_BITS(x) __builtin_popcountll(x)
#endif

// Macht die in der Observer.c instanziierte Gating-Matrix hier bekannt
extern ModuleTestControl global_mod_control[];

static bool first_nuclear_tick = true;
#define NUCLEAR_EPSILON 0.0001

/* Richtungsvektoren aus proPhysics.c spiegeln */
static const int32_t NUCL_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t NUCL_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t NUCL_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* Historische Akkumulatoren */
static uint64_t last_unstable_nuclei_count = 0;
static double accumulated_absorbed_energy_dose = 0.0;

/* Historische Variablen für Hochenergetik und Kettenreaktionen */
static uint64_t last_fission_events_count = 0; // REPARATUR: Fehlende Variable deklariert!
static uint64_t last_released_neutron_bits = 0;

/* Globale Zustandsindikatoren für den integrierten Kernphysik-Prüfstand */
static uint64_t current_fission_events = 0;
static uint64_t free_neutron_trigger_bits = 0;
static uint64_t macro_fusion_merges = 0;
static double input_energy_pre_split = 0.0;
static double output_energy_post_split = 0.0;
static uint64_t unprotected_zone_hits = 0;
static uint64_t protected_zone_hits = 0;

// Synchronisierte Unpacking-Makros für die 16-Byte-Knoten-Dekomprimierung
#define UNPACK_X(idx) ((int32_t)((idx) & 0x3FF))
#define UNPACK_Y(idx) ((int32_t)(((idx) >> 10) & 0x1FF))
#define UNPACK_Z(idx) ((int32_t)((idx) >> 19))

/* ==========================================================================
 * DIAGNOSE 7: Teilchenbeschleuniger & Künstliche Kernumwandlung
 * ========================================================================== */
static void check_particle_accelerators_and_transmutation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t high_velocity_projectiles = 0;

    for (int x = 0; x < 200; x += 2) {
        uint32_t idx = x | (256 << 10) | (256 << 19);
        uint32_t flux = universe->active_nodes_kinetic[idx];

        if ((flux & 0x0FFF) && ZAEHLE_BITS(flux & 0x0FFF) >= 4) {
            high_velocity_projectiles++;
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_NUCLEAR].active_test_id == 0 && high_velocity_projectiles > 0) {
        ProDiBatch_Log(db_engine, "[ACCELERATOR] Beschleuniger aktiv -> %llu hochenergetische Projektil-Bits im Kanal.", high_velocity_projectiles);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 9: Anwendung radioaktiver Nuklide (Tracer-Methode)
 * ========================================================================== */
static void check_nuclide_applications_and_tracers(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    (void)db_engine;
    if (universe->active_count_current > 0) {
        uint32_t slot = universe->active_nodes_current[0];
        uint32_t island_idx = universe->grid[slot].state_island_idx;

        if (island_idx == 1) {
            (void)UNPACK_X(slot);
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 1: Atomaufbau, Kernladung & Isotope (Legacy)
 * ========================================================================== */
static void check_atomic_structure_and_isotopes(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    (void)db_engine;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint64_t mass = universe->data_pool[i].mass_accumulator;
        (void)mass;
    }
}

/* ==========================================================================
 * DIAGNOSE 2: Bohrsches Atommodell & Quantensprünge (Legacy)
 * ========================================================================== */
static void check_bohr_orbit_transitions(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t quantum_jump_emissions = 0;
    double total_excitation_energy = 0.0;
    int core_x = 256, cy = 256, cz = 256;

    for (int r = 11; r <= 15; r++) {
        for (int ch = 0; ch < 12; ch++) {
            int32_t px = (core_x + NUCL_DX[ch] * r) & 0x3FF;
            int32_t py = (cy + NUCL_DY[ch] * r) & 0x1FF;
            int32_t pz = (cz + NUCL_DZ[ch] * r) & 0x1FF;
            uint32_t flux = universe->active_nodes_kinetic[FCC_INDEX(px, py, pz)];
            if ((flux & 0x0FFF) && !(flux & 0x1000)) {
                quantum_jump_emissions++;
                total_excitation_energy += (double)ZAEHLE_BITS(flux & 0x0FFF);
            }
        }
    }
    (void)quantum_jump_emissions;
    (void)total_excitation_energy;
}

static void check_decay_laws_and_activity(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    (void)db_engine;
    uint64_t current_unstable = 0;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        if (universe->data_pool[i].mass_accumulator > 40) current_unstable++;
    }
    last_unstable_nuclei_count = current_unstable;
}

/* ==========================================================================
 * DIAGNOSE 6: Strahlungsdosimetrie & Abschirmungs-Messung
 * ========================================================================== */
static void check_radiation_dosimetry_and_protection(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    (void)db_engine;
    double sector_energy = 0.0; int target_x = 900; int cy = 256, cz = 256;
    unprotected_zone_hits = 0;
    protected_zone_hits = 0;

    for (int dy = -8; dy <= 8; dy++) {
        uint32_t flux_unprot = universe->active_nodes_kinetic[target_x | ((cy + dy) << 10) | (cz << 19)];
        uint32_t flux_prot = universe->active_nodes_kinetic[(target_x + 50) | ((cy + dy) << 10) | (cz << 19)];

        if (flux_unprot & 0x0FFF) {
            uint32_t bits = (uint32_t)ZAEHLE_BITS(flux_unprot & 0x0FFF);
            sector_energy += (double)bits * 0.5;
            unprotected_zone_hits += bits;
        }
        if (flux_prot & 0x0FFF) {
            protected_zone_hits += ZAEHLE_BITS(flux_prot & 0x0FFF);
        }
    }
    if (sector_energy > 0.0) { accumulated_absorbed_energy_dose += (sector_energy / 50.0); }
}

/* ==========================================================================
 * NEW: RUNTIME NUCLEAR PHYSICS VALIDATOR LAB
 * ========================================================================== */
static void execute_interactive_nuclear_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint32_t test_id = global_mod_control[MOD_INDEX_NUCLEAR].active_test_id;
    double input_intensity = global_mod_control[MOD_INDEX_NUCLEAR].target_intensity;
    int32_t custom_param = global_mod_control[MOD_INDEX_NUCLEAR].custom_param;

    if (test_id == 0) return;

    ProDiBatch_Log(db_engine, "[LAB-NUCLEAR] === KERNPHYSIKALISCHES VALIDATOR-LABOR AKTIV (TEST %u) ===", test_id);

    if (test_id == 1) {
        double expected_k = input_intensity;
        double actual_k = (double)free_neutron_trigger_bits / (double)(last_released_neutron_bits + 1);
        double k_error = actual_k - expected_k;

        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Modell: Multiplikationsfaktor der Spalt-Neutronen");
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Reale Spaltereignisse: %llu | Freie Trigger-Bits: %llu", current_fission_events, free_neutron_trigger_bits);
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Soll-Vorgabe k: %.4f | Ist-Gitter-Kritikalitaet k: %.4f", expected_k, actual_k);
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Kritikalitäts-Fehlerrate: %.5f (%s)",
            k_error, (actual_k > 1.0) ? "ÜBERKRITISCH (PROPROTIONALE EXPANION)" : "UNTERKRITISCH/STABIL");
    }
    else if (test_id == 2) {
        double required_threshold = input_intensity;
        double current_plasma_energy = output_energy_post_split;
        bool fusion_allowed = (current_plasma_energy >= required_threshold);

        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Modell: Thermonukleares Gating (Coulomb-Wall-Brechung)");
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Geforderte Barriere-Energie: %.2f J | Vorhandene Plasma-Kinetik: %.2f J", required_threshold, current_plasma_energy);
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Systemstatus: %s", fusion_allowed ? "FUSIONSSCHWELLE ERREICHT (ZÜNDUNG)" : "THERMISCH GEBLOCKT");
    }
    else if (test_id == 3) {
        double mu = input_intensity;
        double thickness_x = (custom_param > 0) ? (double)custom_param : 2.0;

        double I_0 = (double)unprotected_zone_hits;
        double theoretical_intensity = I_0 * exp(-mu * thickness_x);
        double emergent_intensity = (double)protected_zone_hits;

        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Modell: Materialabschirmung gegen Korpuskularstrahlung");
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Input-Intensitaet I0: %.1f | Barriere-Dicke x: %.1f", I_0, thickness_x);
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Theorie-Transmissionswert (Formel): %.4f | Realer Durchlass (Gitter): %.4f", theoretical_intensity, emergent_intensity);
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Dämpfungs-Konvergenzfehler: %.4f Bits", theoretical_intensity - emergent_intensity);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Atom- und Kernphysik-Modul
 * ========================================================================== */
void observer_mod_nuclear_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    /* 1. Atombau & Bohr-Orbitale (Legacy) */
    check_atomic_structure_and_isotopes(universe, db_engine);
    check_bohr_orbit_transitions(universe, db_engine);

    // --- MONOLITHIC FUSED LOOP INTERLEAVING ---
    // Verschmilzt Spaltung, Fusion, Massendefekt und Zerfalls-Modi in einen einzigen dichten Durchlauf
    current_fission_events = 0;
    free_neutron_trigger_bits = 0;
    macro_fusion_merges = 0;
    input_energy_pre_split = 0.0;
    uint64_t total_mass_bits = 0;
    uint64_t alpha = 0; uint64_t b_minus = 0; uint64_t gamma = 0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        uint32_t bit_pop = (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);

        // Fusions-Kinetik
        if (flux & 0x1000) total_mass_bits++;

        // Spaltung & Trigger
        if ((flux & 0x1000) && bit_pop >= 5) {
            current_fission_events++;
            input_energy_pre_split += (double)bit_pop;
        }
        if (bit_pop == 1 && !(flux & 0x1000)) {
            free_neutron_trigger_bits++;
        }

        // Zerfallsarten (Sampling-äquivalente Auswertung im Haupt-Sweep)
        if (i % 5 == 0) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot == 0 && bit_pop >= 6) gamma++;
            if (slot != 0 && (flux & 0x0F0)) b_minus++;
            if ((flux & 0x1000) && slot == 0) alpha++;
        }
    }

    if (universe->active_element_count < 5 && !first_nuclear_tick) {
        macro_fusion_merges++;
    }

    if (!first_nuclear_tick) {
        double multiplication_factor_k = (double)free_neutron_trigger_bits / (double)(last_released_neutron_bits + 1);
        output_energy_post_split = (double)current_fission_events * 20.0;

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_NUCLEAR].active_test_id == 0 && current_fission_events > 0) {
            ProDiBatch_Log(db_engine, "[REACTOR] Spaltung aktiv: %llu Ereignisse | Vermehrungsfaktor k: %.4f", current_fission_events, multiplication_factor_k);
        }
#endif
    }

    last_fission_events_count = current_fission_events; // Jetzt im Scope bekannt!
    last_released_neutron_bits = free_neutron_trigger_bits;
    (void)total_mass_bits;
    (void)alpha; (void)b_minus; (void)gamma;

    /* 2. Radioaktive Verfalls-Gesetze & Dosimetrie */
    check_decay_laws_and_activity(universe, db_engine);
    check_radiation_dosimetry_and_protection(universe, db_engine);

    /* 3. Teilchenbeschleunigung & Tracer */
    check_particle_accelerators_and_transmutation(universe, db_engine);
    check_nuclide_applications_and_tracers(universe, db_engine);

    /* 5. INTERAKTIVER FORMEL-ABGLEICH (Gating-Zentrale) */
    execute_interactive_nuclear_lab(universe, db_engine);

    first_nuclear_tick = false;
}