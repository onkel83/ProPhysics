#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// SDK-Schnittstellen aus den bin/ Verzeichnissen (wichtig für ProDiBatch_Engine)
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

/* Interner Speicher für die Invarianten-Prüfung über Ticks hinweg */
static int64_t last_total_px = 0;

static bool first_nuclear_tick = true;
#define NUCLEAR_EPSILON 0.0001

#define X_MASK  0x3FF 
#define Y_MASK  0x1FF 
#define Z_MASK  0x1FF 
#define Y_SHIFT 10
#define Z_SHIFT 19

/* Richtungsvektoren aus proPhysics.c spiegeln */
static const int32_t NUCL_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t NUCL_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t NUCL_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* Historische Akkumulatoren */
static uint64_t last_unstable_nuclei_count = 0;
static double accumulated_absorbed_energy_dose = 0.0;

/* Historische Variablen für Hochenergetik und Kettenreaktionen */
static uint64_t last_fission_events_count = 0;
static uint64_t last_released_neutron_bits = 0;

/* Globale Zustandsindikatoren für den integrierten Kernphysik-Prüfstand */
static uint64_t current_fission_events = 0;
static uint64_t free_neutron_trigger_bits = 0;
static uint64_t macro_fusion_merges = 0;
static double input_energy_pre_split = 0.0;
static double output_energy_post_split = 0.0;
static uint64_t unprotected_zone_hits = 0;
static uint64_t protected_zone_hits = 0;

/* ==========================================================================
 * DIAGNOSE 7: Teilchenbeschleuniger & Künstliche Kernumwandlung
 * ========================================================================== */
static void check_particle_accelerators_and_transmutation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t high_velocity_projectiles = 0;
    uint64_t induced_transmutations = 0;

    for (int x = 0; x < 200; x += 2) {
        uint32_t idx = x | (256 << Y_SHIFT) | (256 << Z_SHIFT);
        uint32_t flux = universe->active_nodes_kinetic[idx];

        if ((flux & 0x0FFF) && ZAEHLE_BITS(flux & 0x0FFF) >= 4) {
            high_velocity_projectiles++;
        }
    }

    if (universe->active_element_count >= 2) {
        uint64_t current_charge = universe->data_pool[1].charge_spin & QUANTUM_MASK_POLARITY;
        if (universe->current_cpu_tick % 20 == 0 && current_charge != QUANTUM_POL_NEUTRAL) {
            induced_transmutations++;
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_NUCLEAR].active_test_id == 0 && high_velocity_projectiles > 0) {
        ProDiBatch_Log(db_engine, "[ACCELERATOR] Beschleuniger aktiv -> %llu hochenergetische Projektil-Bits im Kanal.", high_velocity_projectiles);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 8: Uranspaltung, Kernfusion & Kettenreaktions-Profiler
 * ========================================================================== */
static void check_fission_fusion_and_chain_reactions(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    current_fission_events = 0;
    free_neutron_trigger_bits = 0;
    macro_fusion_merges = 0;
    input_energy_pre_split = 0.0;

    for (uint64_t i = 0; i < universe->active_count_current; i += 2) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];

        if ((flux & 0x1000) && ZAEHLE_BITS(flux & 0x0FFF) >= 5) {
            current_fission_events++;
            input_energy_pre_split += (double)ZAEHLE_BITS(flux & 0x0FFF);
        }

        if ((flux & 0x0FFF) && !(flux & 0x1000) && ZAEHLE_BITS(flux & 0x0FFF) == 1) {
            free_neutron_trigger_bits++;
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

    last_fission_events_count = current_fission_events;
    last_released_neutron_bits = free_neutron_trigger_bits;
}

/* ==========================================================================
 * DIAGNOSE 9: Anwendung radioaktiver Nuklide (Tracer-Methode)
 * ========================================================================== */
static void check_nuclide_applications_and_tracers(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t tracer_localization_hits = 0;
    double average_tracer_x = 0.0;

    if (universe->active_element_count >= 2) {
        uint32_t slot = universe->active_nodes_current[0];
        uint32_t island_idx = universe->grid[slot].state_island_idx;

        if (island_idx == 1) {
            /* REPARATUR: Schnelle Bitmaske für Torus-X-Koordinate */
            int32_t x = slot & 0x3FF;
            tracer_localization_hits++;
            average_tracer_x = (double)x;
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 1: Atomaufbau, Kernladung & Isotope (Legacy)
 * ========================================================================== */
static void check_atomic_structure_and_isotopes(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t total_detected_atoms = 0; uint64_t heavy_isotopes_count = 0;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint64_t mass = universe->data_pool[i].mass_accumulator;
        if (mass > 0) { total_detected_atoms++; if (mass > 80) heavy_isotopes_count++; }
    }
}

/* ==========================================================================
 * DIAGNOSE 2: Bohrsches Atommodell & Quantensprünge (Legacy)
 * ========================================================================== */
static void check_bohr_orbit_transitions(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t quantum_jump_emissions = 0; double total_excitation_energy = 0.0;
    int core_x = 256, cy = 256, cz = 256; int shell_start_r = 11, shell_end_r = 15;

    for (int r = shell_start_r; r <= shell_end_r; r++) {
        for (int ch = 0; ch < 12; ch++) {
            int32_t px = (core_x + NUCL_DX[ch] * r) & X_MASK; int32_t py = (cy + NUCL_DY[ch] * r) & Y_MASK; int32_t pz = (cz + NUCL_DZ[ch] * r) & Z_MASK;
            uint32_t flux = universe->active_nodes_kinetic[FCC_INDEX(px, py, pz)];
            if ((flux & 0x0FFF) && !(flux & 0x1000)) { quantum_jump_emissions++; total_excitation_energy += (double)ZAEHLE_BITS(flux & 0x0FFF); }
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 3: Fusions-Kinetik, Massendefekt & Bindungsenergie (Legacy)
 * ========================================================================== */
static void check_mass_defect_and_binding_energy(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (first_nuclear_tick) return;
    uint64_t total_mass_bits = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i]; if (universe->active_nodes_kinetic[idx] & 0x1000) total_mass_bits++;
    }
}

/* ==========================================================================
 * DIAGNOSE 4: Radioaktive Zerfallsarten (Alpha, Beta, Gamma) (Legacy)
 * ========================================================================== */
static void check_nuclear_decay_types(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t alpha = 0; uint64_t b_minus = 0; uint64_t gamma = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i += 5) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (flux & 0x0FFF) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot == 0 && ZAEHLE_BITS(flux & 0x0FFF) >= 6) gamma++;
            if (slot != 0 && (flux & 0x0F0)) b_minus++;
        }
        if ((flux & 0x1000) && universe->grid[idx].state_island_idx == 0) alpha++;
    }
}

static void check_decay_laws_and_activity(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t current_unstable = 0;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        if (universe->data_pool[i].mass_accumulator > 40) current_unstable++;
    }
    if (!first_nuclear_tick && current_unstable > 0) {
        int64_t delta_N = (int64_t)last_unstable_nuclei_count - (int64_t)current_unstable;
    }
    last_unstable_nuclei_count = current_unstable;
}

/* ==========================================================================
 * DIAGNOSE 6: Strahlungsdosimetrie & Abschirmungs-Messung
 * ========================================================================== */
static void check_radiation_dosimetry_and_protection(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double sector_energy = 0.0; int target_x = 900; int cy = 256, cz = 256;
    unprotected_zone_hits = 0;
    protected_zone_hits = 0;

    /* Scan der ungeschützten vs. geschützten Testsektoren */
    for (int dy = -8; dy <= 8; dy++) {
        uint32_t flux_unprot = universe->active_nodes_kinetic[target_x | ((cy + dy) << Y_SHIFT) | (cz << Z_SHIFT)];
        uint32_t flux_prot = universe->active_nodes_kinetic[(target_x + 50) | ((cy + dy) << Y_SHIFT) | (cz << Z_SHIFT)];

        if (flux_unprot & 0x0FFF) {
            sector_energy += (double)ZAEHLE_BITS(flux_unprot & 0x0FFF) * 0.5;
            unprotected_zone_hits += ZAEHLE_BITS(flux_unprot & 0x0FFF);
        }
        if (flux_prot & 0x0FFF) {
            protected_zone_hits += ZAEHLE_BITS(flux_prot & 0x0FFF);
        }
    }
    if (sector_energy > 0.0) { accumulated_absorbed_energy_dose += (sector_energy / 50.0); }
}

/* ==========================================================================
 * NEW: RUNTIME NUCLEAR PHYSICS VALIDATOR LAB
 * Prüft den atomaren Vermehrungsfaktor (k), kinetische Fusionsschwellen und
 * lineare Schwächungskoeffizienten von Abschirmmaterialien gegen den RAM.
 * ========================================================================== */
static void execute_interactive_nuclear_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint32_t test_id = global_mod_control[MOD_INDEX_NUCLEAR].active_test_id;
    double input_intensity = global_mod_control[MOD_INDEX_NUCLEAR].target_intensity;
    int32_t custom_param = global_mod_control[MOD_INDEX_NUCLEAR].custom_param;

    if (test_id == 0) return;

    ProDiBatch_Log(db_engine, "[LAB-NUCLEAR] === KERNPHYSIKALISCHES VALIDATOR-LABOR AKTIV (TEST %u) ===", test_id);

    /* FORMEL-TEST 1: Kritikalität & Kettenreaktion (Soll-Vermehrungsfaktor k_soll)
       Eingabe: set nucl 1 <Soll_k_Faktor> */
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

    /* FORMEL-TEST 2: Thermonukleare Fusionsschwelle & Kinetische Barriere
       Eingabe: set nucl 2 <Erwartete_Mindest_Enthalpie> */
    if (test_id == 2) {
        double required_threshold = input_intensity; // Kinetische Energiebarriere
        double current_plasma_energy = output_energy_post_split;

        bool fusion_allowed = (current_plasma_energy >= required_threshold);

        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Modell: Thermonukleares Gating (Coulomb-Wall-Brechung)");
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Geforderte Barriere-Energie: %.2f J | Vorhandene Plasma-Kinetik: %.2f J", required_threshold, current_plasma_energy);
        ProDiBatch_Log(db_engine, "[NUCLEAR_CHECK] Systemstatus: %s", fusion_allowed ? "FUSIONSSCHWELLE ERREICHT (ZÜNDUNG)" : "THERMISCH GEBLOCKT");
    }

    /* FORMEL-TEST 3: Strahlungsabschirmung & Lineares Schwaechungsgesetz (I = I0 * exp(-mu * x))
       Eingabe: set nucl 3 <Schwaechungskoeffizient_mu> [Dicke_x] */
    if (test_id == 3) {
        double mu = input_intensity; // Linearer Schwächungskoeffizient des Materials
        double thickness_x = (custom_param > 0) ? (double)custom_param : 2.0; // Materialdicke in Gitterzellen

        /* Klassisches Lambert-Beersches Schwächungsgesetz für Teilchenströme:
           I_theorie = I_0 * exp(-mu * x) */
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
    check_mass_defect_and_binding_energy(universe, db_engine);

    /* 2. Radioaktive Verfalls-Modi, Gesetze & Dosimetrie (Legacy) */
    check_nuclear_decay_types(universe, db_engine);
    check_decay_laws_and_activity(universe, db_engine);
    check_radiation_dosimetry_and_protection(universe, db_engine);

    /* 3. NEU: Teilchenbeschleunigung & induzierte Transmutation */
    check_particle_accelerators_and_transmutation(universe, db_engine);

    /* 4. NEU: Spaltung, Fusion & Kettenreaktions-Kritikalität */
    check_fission_fusion_and_chain_reactions(universe, db_engine);
    check_nuclide_applications_and_tracers(universe, db_engine);

    /* 5. INTERAKTIVER FORMEL-ABGLEICH (Gating-Zentrale) */
    execute_interactive_nuclear_lab(universe, db_engine);

    first_nuclear_tick = false;
}