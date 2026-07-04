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

// Hardware-beschleunigtes Popcount-Macro für x64-Systeme
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

static bool first_chem_tick = true;
#define CHEM_EPSILON 0.0001

#define X_MASK  0x3FF 
#define Y_MASK  0x1FF 
#define Z_MASK  0x1FF 
#define Y_SHIFT 10
#define Z_SHIFT 19

/* Historische Bilanzwerte für Stöchiometrie-Audits */
static uint64_t last_total_substance_mass = 0;
static double last_reaction_energy_enthalpy = 0.0;
static uint64_t initial_universe_mass = 0;

/* Globale Zustandsindikatoren für das Chemie-Labor */
static double current_kinetic_enthalpy = 0.0;
static uint64_t total_substance_mass = 0;
static uint64_t active_chiral_catalysts = 0;

/* ==========================================================================
 * DIAGNOSE 1: Chemische Elemente, Verbindungen & Molekül-Cluster
 * ========================================================================== */
static void check_elements_and_compounds(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t isolated_atoms_count = 0;
    uint64_t molecular_compounds_count = 0;
    uint64_t current_mass_sum = 0;

    total_substance_mass = 0; // Zurücksetzen vor Akkumulation

    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint64_t mass = universe->data_pool[i].mass_accumulator;

        if (mass > 0) {
            total_substance_mass += mass;
            current_mass_sum += mass;

            if (mass <= 16) {
                isolated_atoms_count++;
            }
            else {
                molecular_compounds_count++;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_CHEMISTRY].active_test_id == 0) {
        ProDiBatch_Log(db_engine, "[CHEM] Elemente (Atome): %llu | Verbindungen (Molekuele): %llu",
            isolated_atoms_count, molecular_compounds_count);
    }
#endif

    if (first_chem_tick) {
        initial_universe_mass = current_mass_sum;
    }
    last_total_substance_mass = current_mass_sum;
}

/* ==========================================================================
 * DIAGNOSE 2: Chemische Bindung & Valenzelektronen-Analogie
 * ========================================================================== */
static void check_chemical_bonds_and_valency(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t open_valency_channels = 0;
    uint64_t active_bond_nodes = 0;

    for (uint64_t i = 0; i < universe->active_count_current; i += 4) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];

        if (flux & 0x1000) {
            active_bond_nodes++;
            uint32_t free_channels = 12 - (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);
            if (free_channels > 0 && free_channels < 12) {
                open_valency_channels += free_channels;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_CHEMISTRY].active_test_id == 0 && active_bond_nodes > 0) {
        double average_valency = (double)open_valency_channels / (double)active_bond_nodes;
        ProDiBatch_Log(db_engine, "[CHEM] Mittlere molekulare Valenzkapazitaet: %.2f freie Pfade/Knoten", average_valency);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 3: Reaktionskinetik, Enthalpie & Katalyse-Wirkungsgrad
 * ========================================================================== */
static void check_reaction_kinetics_and_catalysis(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    active_chiral_catalysts = 0;
    current_kinetic_enthalpy = 0.0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        current_kinetic_enthalpy += (double)ZAEHLE_BITS(flux & 0x0FFF);

        uint32_t slot = universe->grid[idx].state_island_idx;
        if (slot != 0) {
            uint32_t spin = (uint32_t)((universe->data_pool[slot].charge_spin & QUANTUM_MASK_SPIN_CHIRAL) >> 2);
            if (spin == QUANTUM_SPIN_CW || spin == QUANTUM_SPIN_CCW) {
                active_chiral_catalysts++;
            }
        }
    }

    if (!first_chem_tick) {
        double delta_H = current_kinetic_enthalpy - last_reaction_energy_enthalpy;

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_CHEMISTRY].active_test_id == 0) {
            if (delta_H > 5.0) {
                ProDiBatch_Log(db_engine, "[KINETICS] ENDOTHERM -> Absorbiert: delta_H = +%.2f J_bit", delta_H);
            }
            else if (delta_H < -5.0) {
                ProDiBatch_Log(db_engine, "[KINETICS] EXOTHERM -> Freigesetzt: delta_H = %.2f J_bit", delta_H);
            }
        }
#endif
    }
    last_reaction_energy_enthalpy = current_kinetic_enthalpy;
}

/* ==========================================================================
 * DIAGNOSE 4: Stöchiometrie & Massenerhaltungsgesetz (Lavoisier-Audit)
 * ========================================================================== */
static void check_stoichiometry_and_conservation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (first_chem_tick) return;
    (void)universe;

    if (last_total_substance_mass != initial_universe_mass) {
        int64_t mass_drift = (int64_t)last_total_substance_mass - (int64_t)initial_universe_mass;
        if (mass_drift != 0 && global_mod_control[MOD_INDEX_CHEMISTRY].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[WARN][INVARIANCE] Stoeechiometrie-Abweichung! Massendrift detektiert: %lld M_bit", mass_drift);
        }
    }
}

/* ==========================================================================
 * NEW: RUNTIME CHEMICAL FORMULA VALIDATOR LAB
 * ========================================================================== */
static void execute_interactive_chemistry_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint32_t test_id = global_mod_control[MOD_INDEX_CHEMISTRY].active_test_id;
    double input_intensity = global_mod_control[MOD_INDEX_CHEMISTRY].target_intensity;
    int32_t custom_param = global_mod_control[MOD_INDEX_CHEMISTRY].custom_param;

    if (test_id == 0) return;

    ProDiBatch_Log(db_engine, "[LAB-CHEMISTRY] === CHEMISCHES PRUEFSTAND-LABOR GEKOPPELT (TEST %u) ===", test_id);

    /* FORMEL-TEST 1: Validierung des Massenerhaltungsgesetzes (Stöchiometrie-Umsatz) */
    if (test_id == 1) {
        double expected_mass_preservation = 1.0000;
        double actual_mass_ratio = (double)last_total_substance_mass / ((double)initial_universe_mass + CHEM_EPSILON);
        double delta_compliance = actual_mass_ratio - expected_mass_preservation;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Stoechiometrisches Lavoisier-Audit");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Molares Erwartungs-Verhaeltnis: %.4f | Reales Gitter-Verhaeltnis: %.4f", expected_mass_preservation, actual_mass_ratio);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Massen-Invarianzfehler: %.6f (%s)",
            delta_compliance, (fabs(delta_compliance) < CHEM_EPSILON) ? "STRENG STÖCHIOMETRISCH" : "SYSTEMLECK");
    }

    /* FORMEL-TEST 2: Reaktionsgeschwindigkeit & Arrhenius-Katalyse */
    if (test_id == 2) {
        double E_a = input_intensity;
        double T_env = (custom_param > 0) ? (double)custom_param : (current_kinetic_enthalpy * 0.001);

        double exponent = -E_a / (T_env + CHEM_EPSILON);
        double theoretical_rate_k = exp(exponent);
        double emergent_rate_k = (double)active_chiral_catalysts * 0.1337;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Arrhenius-Kinetik unter Thermo-Druck");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Angewandte Aktivierungsbarriere E_a: %.2f | Lokale Temperatur T: %.4f", E_a, T_env);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Theorie-Kinetik_k (Formel): %.5f | Ist-Katalysator-Drift_k: %.5f", theoretical_rate_k, emergent_rate_k);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Katalytischer Wirkungsgrad-Abgleich: %.2f%%",
            (emergent_rate_k / (theoretical_rate_k + CHEM_EPSILON)) * 100.0);
    }

    /* FORMEL-TEST 3: Faradaysches Gesetz der Elektrolyse */
    if (test_id == 3) {
        double simulated_I = input_intensity;
        double simulated_t = (custom_param > 0) ? (double)custom_param : 1.0;

        double theoretical_separated_mass = 0.5 * simulated_I * simulated_t;
        double emergent_separated_mass = 0.0;

        for (uint32_t i = 1; i < universe->active_element_count; i++) {
            uint32_t charge = (uint32_t)(universe->data_pool[i].charge_spin & QUANTUM_MASK_POLARITY);
            if (charge == QUANTUM_POL_PLUS || charge == QUANTUM_POL_MINUS) {
                emergent_separated_mass += (double)universe->data_pool[i].vx * 0.05;
            }
        }
        if (emergent_separated_mass < 0.0) emergent_separated_mass = 0.0;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Faraday-Elektrolyse im Ionenfeld");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Theorie-Abscheidung m (Formel): %.4f M_bit", theoretical_separated_mass);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Realer Ionen-Abscheidungsdrift (Gitter): %.4f M_bit", emergent_separated_mass);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Elektrochemische Konvergenz-Differenz: %.4f M_bit", theoretical_separated_mass - emergent_separated_mass);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Chemie-Modul
 * ========================================================================== */
void observer_mod_chemistry_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    /* 1. Molekulare Topologie- und Clusteranalysen */
    check_elements_and_compounds(universe, db_engine);

    /* 2. Bindungsmechanik und Sättigungs-Wertigkeiten */
    check_chemical_bonds_and_valency(universe, db_engine);

    /* 3. Reaktionskinetik, Thermochemie & Chiral-Katalyse */
    check_reaction_kinetics_and_catalysis(universe, db_engine);

    /* 4. Stöchiometrische Invariantenprüfung */
    check_stoichiometry_and_conservation(universe, db_engine);

    /* 5. INTERAKTIVER FORMEL-ABGLEICH (Gating-Zentrale) */
    execute_interactive_chemistry_lab(universe, db_engine);

    (void)last_total_px;
    first_chem_tick = false;
}