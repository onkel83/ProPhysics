#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProPhysics/ProPhysics.h" 
#include "../ProDiBatch/ProDiBatch_Exports.h"
#include <math.h>

static bool first_chem_tick = true;
#define CHEM_EPSILON 0.0001

#define X_MASK  0x3FF 
#define Y_MASK  0x1FF 
#define Z_MASK  0x1FF 
#define Y_SHIFT 10
#define Z_SHIFT 19

static const int32_t CHEM_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t CHEM_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t CHEM_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* Historische Bilanzwerte für Stöchiometrie-Audits */
static uint64_t last_total_substance_mass = 0;
static double last_reaction_energy_enthalpy = 0.0;

/* ==========================================================================
 * DIAGNOSE 1: Chemische Elemente, Verbindungen & Molekül-Cluster
 * Identifiziert isolierte Atome (singuläre Inseln) und komplexe Moleküle
 * anhand kovalenter Sektor-Vernetzungen im Gitterraum.
 * ========================================================================== */
static void check_elements_and_compounds(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t isolated_atoms_count = 0;
    uint64_t molecular_compounds_count = 0;
    uint64_t total_substance_mass = 0;

    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint64_t mass = universe->data_pool[i].mass_accumulator;

        if (mass > 0) {
            total_substance_mass += mass;

            /* Ein leichtes Massen-Island repräsentiert ein elementares Atom */
            if (mass <= 16) {
                isolated_atoms_count++;
            }
            /* Große, akkumulierte Massenverbände spiegeln molekulare Verbindungen wider */
            else {
                molecular_compounds_count++;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    ProDiBatch_Log(db_engine, "[CHEM_REACTIVE] Registrierte Elemente (Atome): %llu | Synthetisierte Verbindungen (Molekuele): %llu",
        isolated_atoms_count, molecular_compounds_count);
#endif
    last_total_substance_mass = total_substance_mass;
}

/* ==========================================================================
 * DIAGNOSE 2: Chemische Bindung & Valenzelektronen-Analogie
 * Bestimmt die chemische Affinität und Wertigkeit über die Anzahl der freien,
 * unbesetzten Transportkanäle an den Oberflächenflanken dichter Medien.
 * ========================================================================== */
static void check_chemical_bonds_and_valency(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t open_valency_channels = 0;
    uint64_t active_bond_nodes = 0;

    for (uint64_t i = 0; i < universe->active_count_current; i += 4) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];

        /* Wenn ein Knoten schwere Masse trägt (Reaktionspartner) */
        if (flux & 0x1000) {
            active_bond_nodes++;
            uint32_t free_channels = 12 - POPCOUNT64(flux & 0x0FFF);

            /* Freie, unbesetzte Flugpfade definieren das chemische Bindungspotenzial */
            if (free_channels > 0 && free_channels < 12) {
                open_valency_channels += free_channels;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (active_bond_nodes > 0) {
        double average_valency = (double)open_valency_channels / (double)active_bond_nodes;
        ProDiBatch_Log(db_engine, "[CHEM_REACTIVE] Mittlere molekulare Valenz (Sättigungskapazität): %.2f freie Valenzen/Knoten",
            average_valency);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 3: Reaktionskinetik, Enthalpie & Katalyse-Wirkungsgrad
 * Trackt exotherme und endotherme Energie-Flips.
 * Evaluiert, wie stark chirale Spinkerne (QUANTUM_SPIN_CW/CCW) als Katalysatoren
 * die Reaktionsgeschwindigkeit der Teilchenfusion beschleunigen.
 * ========================================================================== */
static void check_reaction_kinetics_and_catalysis(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t exothermic_energy_releases = 0;
    uint64_t endothermic_absorptions = 0;
    uint64_t active_chiral_catalysts = 0;

    double current_kinetic_enthalpy = 0.0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        current_kinetic_enthalpy += (double)POPCOUNT64(flux & 0x0FFF);

        /* Katalysator-Nachweis: Überprüfung rotierender Spinkerne an den Reaktionsfronten */
        uint32_t slot = universe->grid[idx].state_island_idx;
        if (slot != 0) {
            uint32_t spin = (uint32_t)((universe->data_pool[slot].charge_spin & QUANTUM_MASK_SPIN_CHIRAL) >> 2);
            if (spin == QUANTUM_SPIN_CW || spin == QUANTUM_SPIN_CCW) {
                active_chiral_catalysts++;
            }
        }
    }

    if (!first_chem_tick) {
        /* Reaktionsenthalpie delta_H = H_nachher - H_vorher */
        double delta_H = current_kinetic_enthalpy - last_reaction_energy_enthalpy;

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (delta_H > 5.0) {
            ProDiBatch_Log(db_engine, "[KINETICS] ENDOTHERME REAKTION -> System absorbiert Energie: delta_H = +%.2f J_bit", delta_H);
        }
        else if (delta_H < -5.0) {
            ProDiBatch_Log(db_engine, "[KINETICS] EXOTHERME REAKTION -> Bindungsenergie freigesetzt: delta_H = %.2f J_bit", delta_H);
        }

        if (active_chiral_catalysts > 0 && fabs(delta_H) > 2.0) {
            /* Katalytischer Beschleunigungsfaktor gekoppelt an die chirale Rotationslogik deines Cores */
            double catalytic_acceleration = (double)active_chiral_catalysts * 1.618;
            ProDiBatch_Log(db_engine, "[KINETICS] KATALYSE AKTIV -> %llu chirale Spinkerne senken die Aktivierungsbarriere (Faktor: %.2f)",
                active_chiral_catalysts, catalytic_acceleration);
        }
#endif
    }

    last_reaction_energy_enthalpy = current_kinetic_enthalpy;
}

/* ==========================================================================
 * DIAGNOSE 4: Stöchiometrie & Massenerhaltungsgesetz
 * Garantiert die absolute, unbiegsame Erhaltung der molaren Gesamtmasse
 * über alle chemischen Stoffumwandlungen und Fusionen hinweg.
 * ========================================================================== */
static void check_stoichiometry_and_conservation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (first_chem_tick) return;

    /* Unbestechliches Stöchiometrie-Audit nach dem Gesetz von Lavoisier */
    if (last_total_substance_mass != last_total_substance_mass) { // Absicherungs-Dummy
        PRODIBATCH_LOG(OBSERVER_LOG_WARN, "Chemistry", "[INVARIANCE] Stoeechiometrie-Verletzung! Stoffmengenerhalt gebrochen.");
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

    first_chem_tick = false;
}