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

static bool first_elec_tick = true;
#define ELEC_EPSILON 0.0001

#define X_MASK  0x3FF 
#define Y_MASK  0x1FF 
#define Z_MASK  0x1FF 
#define Y_SHIFT 10
#define Z_SHIFT 19

static const int32_t ELEC_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t ELEC_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t ELEC_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* Historische Feld-, AC- und Maschinenmetriken */
static double last_total_charge_Q = 0.0;
static double monitored_voltage_U = 0.0;
static double monitored_current_I = 0.0;
static double accumulated_elec_work_W = 0.0;
static double global_E_field_x = 0.0;
static double global_B_field_z = 0.0;
static double last_magnetic_flux_Phi = 0.0;
static double accumulated_magnetic_field_energy = 0.0;
static double last_generator_emf = 0.0;
static uint64_t phase_wave_ticks = 0;
static double ac_sum_u_sq = 0.0;
static double ac_sum_i_sq = 0.0;
static uint64_t ac_sample_ticks = 0;
static double last_instant_u = 0.0;
static double last_instant_i = 0.0;
static uint64_t phase_shift_lag_ticks = 0;
static double last_chemical_pot_A = 0.0;

/* Historische Variablen für Hochfrequenz-Wellenmetriken */
static double last_E_field_snapshot = 0.0;
static uint64_t wave_period_counter = 0;
static uint64_t last_detected_wavelength = 0;
static double current_detected_frequency = 0.0;

/* ==========================================================================
 * DIAGNOSE 21: Schwingkreis & Elektromagnetische Schwingungserzeugung
 * ========================================================================== */
static void check_resonant_oscillations_and_generation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (first_elec_tick) return;

    double E_energy = 0.5 * (global_E_field_x * global_E_field_x);
    double B_energy = 0.5 * (global_B_field_z * global_B_field_z);

    wave_period_counter++;
    bool field_flipped = (last_E_field_snapshot * global_E_field_x < 0.0);

    if (field_flipped && wave_period_counter > 2) {
        uint64_t half_period = wave_period_counter;
        current_detected_frequency = 1.0 / ((double)half_period * 2.0 + ELEC_EPSILON);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_ELECTROTECH].active_test_id == 0 && (E_energy + B_energy > 0.01)) {
            ProDiBatch_Log(db_engine, "[RESONATOR] Energiebalance -> E-Feld: %.4f J | B-Feld: %.4f J", E_energy, B_energy);
            ProDiBatch_Log(db_engine, "[RESONATOR] Eigenfrequenz f_resonance: %.5f Tick^-1", current_detected_frequency);
        }
#endif

        last_detected_wavelength = half_period * 2;
        wave_period_counter = 0;
    }

    last_E_field_snapshot = global_E_field_x;
}

/* ==========================================================================
 * DIAGNOSE 22: Offener Schwingkreis, EM-Wellen & Spektrum
 * ========================================================================== */
static void check_em_waves_and_spectral_distribution(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t propagation_wavefront_nodes = 0;
    int far_field_start_x = 800;
    int cy = 256, cz = 256;

    for (int x = far_field_start_x; x < PROPHYSICS_X_MAX - 4; x += 2) {
        uint32_t idx = x | (cy << Y_SHIFT) | (cz << Z_SHIFT);
        uint32_t flux = universe->active_nodes_kinetic[idx];

        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            uint32_t neighbor_idx = ((x + 1) & X_MASK) | (cy << Y_SHIFT) | (cz << Z_SHIFT);
            uint32_t neighbor_flux = universe->active_nodes_kinetic[neighbor_idx];

            if (ZAEHLE_BITS(flux & 0x0FFF) != ZAEHLE_BITS(neighbor_flux & 0x0FFF)) {
                propagation_wavefront_nodes++;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_ELECTROTECH].active_test_id == 0) {
        if (propagation_wavefront_nodes > 0) {
            ProDiBatch_Log(db_engine, "[WAVE] Dipol-Abstrahlung detektiert: %llu aktive Wellenfront-Knoten.", propagation_wavefront_nodes);
        }
    }
#endif
}

/* ==========================================================================
 * LEGACY DIAGNOSEN (Gleichstrom, AC, Magnetismus, Halbleiter, Röhren)
 * ========================================================================== */
static void check_charge_and_current_flux(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t positive_charges = 0; uint64_t negative_charges = 0; uint64_t current_flux = 0;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint64_t charge = universe->data_pool[i].charge_spin & QUANTUM_MASK_POLARITY;
        uint64_t mass = universe->data_pool[i].mass_accumulator;
        if (charge == QUANTUM_POL_PLUS) positive_charges += mass;
        if (charge == QUANTUM_POL_MINUS) negative_charges += mass;
    }
    int cross_x = 512;
    for (int pz = 128; pz < 384; pz++) {
        for (int py = 128; py < 384; py++) {
            uint32_t idx = (cross_x) | (py << Y_SHIFT) | (pz << Z_SHIFT);
            uint32_t flux = universe->active_nodes_kinetic[idx];
            if (flux & 0x0FFF) {
                uint32_t slot = universe->grid[idx].state_island_idx;
                if (slot != 0 && (universe->data_pool[slot].charge_spin & QUANTUM_MASK_POLARITY) != QUANTUM_POL_NEUTRAL) {
                    current_flux += ZAEHLE_BITS(flux & 0x055);
                }
            }
        }
    }
    monitored_current_I = (double)current_flux * 0.1;
    last_total_charge_Q = (double)positive_charges + (double)negative_charges;
}

static void check_voltage_and_potential_drops(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double p_source = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 256, 256, 256, 4);
    double p_sink = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 768, 256, 256, 4);
    monitored_voltage_U = fabs(p_source - p_sink) * 10.0;
}

static void check_electric_fields_and_displacement(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double field_gradient_x = 0.0; int start_x = 240, end_x = 780; int cy = 256, cz = 256;
    for (int x = start_x; x <= end_x; x += 4) {
        uint32_t idx = x | (cy << Y_SHIFT) | (cz << Z_SHIFT); uint32_t slot = universe->grid[idx].state_island_idx;
        double val = 0.0; if (slot != 0) { uint64_t pol = universe->data_pool[slot].charge_spin & QUANTUM_MASK_POLARITY; val = (pol == QUANTUM_POL_PLUS) ? 1.0 : ((pol == QUANTUM_POL_MINUS) ? -1.0 : 0.0); }
        uint32_t idx_n = ((x + 2) & X_MASK) | (cy << Y_SHIFT) | (cz << Z_SHIFT); uint32_t slot_n = universe->grid[idx_n].state_island_idx;
        double val_n = 0.0; if (slot_n != 0) { uint64_t pol_n = universe->data_pool[slot_n].charge_spin & QUANTUM_MASK_POLARITY; val_n = (pol_n == QUANTUM_POL_PLUS) ? 1.0 : ((pol_n == QUANTUM_POL_MINUS) ? -1.0 : 0.0); }
        field_gradient_x += (val_n - val) * 5.0;
    }
    global_E_field_x = field_gradient_x * 0.01;
}

static void check_magnetic_fields_and_dipoles(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double spin_alignment_z = 0.0;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint32_t spin = (uint32_t)((universe->data_pool[i].charge_spin & QUANTUM_MASK_SPIN_CHIRAL) >> 2);
        uint64_t mass = universe->data_pool[i].mass_accumulator;
        if (spin != QUANTUM_SPIN_NONE && mass > 0) {
            spin_alignment_z += ((spin == QUANTUM_SPIN_CW) ? 1.0 : -1.0) * (double)mass;
        }
    }
    global_B_field_z = spin_alignment_z * 0.001;
}

static void check_electromagnetism_and_induction_laws(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { last_magnetic_flux_Phi = global_B_field_z * 62500.0; }
static void check_ac_circuits_and_impedance(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    ac_sum_u_sq += monitored_voltage_U * monitored_voltage_U; ac_sum_i_sq += monitored_current_I * monitored_current_I; ac_sample_ticks++;
    if (ac_sample_ticks >= 50) { ac_sum_u_sq = 0.0; ac_sum_i_sq = 0.0; ac_sample_ticks = 0; }
    last_instant_u = monitored_voltage_U; last_instant_i = monitored_current_I;
}

static void check_resistance_and_temperature_coefficients(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_kirchhoff_laws_and_circuits(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_capacitors_and_circuits(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_field_energy_and_forces(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_moving_conductor_and_self_induction(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_magnetic_energy_and_lorentz_forces(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_generators_and_electromechanical_induction(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_motors_and_mechanical_efficiency(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_transformer_coupling(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_semiconductors_and_transistors(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_electrolysis_and_galvanic_cells(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_vacuum_emission_and_radiation_tubes(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }

/* ==========================================================================
 * NEW: RUNTIME ELECTROTECH FORMULA VALIDATOR LAB
 * Berechnet ideale Ohmsche Netzwerke, Thomsonsche Resonanzfrequenzen und
 * induktive Übersetzungsverhältnisse und validiert sie live gegen den RAM.
 * ========================================================================== */
static void execute_interactive_electrotech_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint32_t test_id = global_mod_control[MOD_INDEX_ELECTROTECH].active_test_id;
    double input_intensity = global_mod_control[MOD_INDEX_ELECTROTECH].target_intensity;
    int32_t custom_param = global_mod_control[MOD_INDEX_ELECTROTECH].custom_param;

    if (test_id == 0) return;

    ProDiBatch_Log(db_engine, "[LAB-ELECTRO] === ELEKTROTECHNIK PRUEFSTAND DER ENGINES (TEST %u) ===", test_id);

    /* FORMEL-TEST 1: Ohmsches Gesetz & Kirchhoff-Netzwerk-Abgleich (U = R * I)
       Eingabe: set elec 1 <Soll_Widerstand_R> */
    if (test_id == 1) {
        double expected_R = input_intensity;
        /* R_ist = U / I */
        double measured_R = monitored_voltage_U / (monitored_current_I + ELEC_EPSILON);
        double ohm_error = measured_R - expected_R;

        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Modell: Virtueller Widerstands-Verbraucher im Maschennetz");
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Gitterspannung U: %.4f V | Gitterstrom I: %.4f A", monitored_voltage_U, monitored_current_I);
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Soll-Widerstand R: %.4f Ohm | Ist-Gitter-Widerstand R: %.4f Ohm", expected_R, measured_R);
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Ohmsche Last-Abweichung: %.5f Ohm (%s)",
            ohm_error, (fabs(ohm_error) < 0.1) ? "MASCHENREGEL KONFORM" : "NETZWERKDRIFT");
    }

    /* FORMEL-TEST 2: Thomsonsche Schwingungsgleichung (f = 1 / (2 * pi * sqrt(L * C)))
       Eingabe: set elec 2 <Kapazitaet_C> [Induktivitaet_L] */
    if (test_id == 2) {
        double C_val = input_intensity;
        double L_val = (custom_param > 0) ? (double)custom_param : 1.0;

        /* Thomson-Formel: f = 1 / (2 * pi * sqrt(L * C)) */
        double theoretical_f = 1.0 / (2.0 * 3.1415926535 * sqrt(L_val * C_val) + ELEC_EPSILON);
        double frequency_error = current_detected_frequency - theoretical_f;

        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Modell: LC-Schwingkreis Resonanz-Abgleich");
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Vorgabe C: %.4f | Vorgabe L: %.4f", C_val, L_val);
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Theorie-Frequenz f: %.5f Hz | Emergiere Gitter-Frequenz f: %.5f Hz", theoretical_f, current_detected_frequency);
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Resonanz-Frequenzfehler: %.6f Tick^-1", frequency_error);
    }

    /* FORMEL-TEST 3: Induktive Kopplung & Transformator-Uebersetzung (u_e = U2 / U1 = N2 / N1)
       Eingabe: set elec 3 <Soll_Windungsverhaeltnis_ue> */
    if (test_id == 3) {
        double expected_ue = input_intensity; // N2/N1 Windungsverhältnis

        double u_primary = fabs(ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 256, 256, 256, 2) - 1.0) * 10.0;
        double u_secondary = fabs(ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 768, 256, 256, 2) - 1.0) * 10.0;

        double actual_ue = u_secondary / (u_primary + ELEC_EPSILON);
        double transformer_error = actual_ue - expected_ue;

        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Modell: Magnetische Transformator-Kopplung");
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Primärkreis U1: %.3f V | Sekundärkreis U2: %.3f V", u_primary, u_secondary);
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Soll-Uebersetzung ue: %.4f | Ist-Gitter-Uebersetzung ue: %.4f", expected_ue, actual_ue);
        ProDiBatch_Log(db_engine, "[CIRCUIT_CHECK] Abweichung des Induktionsfaktors: %.4f", transformer_error);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Elektrotechnik-Modul
 * ========================================================================== */
void observer_mod_electrotech_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    /* 1. Kinetische Stromkreise & Felder (Legacy) */
    check_charge_and_current_flux(universe, db_engine);
    check_voltage_and_potential_drops(universe, db_engine);
    check_resistance_and_temperature_coefficients(universe, db_engine);
    check_kirchhoff_laws_and_circuits(universe, db_engine);
    check_electric_fields_and_displacement(universe, db_engine);
    check_capacitors_and_circuits(universe, db_engine);
    check_field_energy_and_forces(universe, db_engine);

    /* 2. Magnetismus & AC-Analytik (Legacy) */
    check_magnetic_fields_and_dipoles(universe, db_engine);
    check_electromagnetism_and_induction_laws(universe, db_engine);
    check_moving_conductor_and_self_induction(universe, db_engine);
    check_magnetic_energy_and_lorentz_forces(universe, db_engine);
    check_generators_and_electromechanical_induction(universe, db_engine);
    check_motors_and_mechanical_efficiency(universe, db_engine);
    check_ac_circuits_and_impedance(universe, db_engine);
    check_transformer_coupling(universe, db_engine);
    check_semiconductors_and_transistors(universe, db_engine);
    check_electrolysis_and_galvanic_cells(universe, db_engine);
    check_vacuum_emission_and_radiation_tubes(universe, db_engine);

    /* 3. NEU: Hochfrequenztechnik & Schwingkreis-Oszillationen */
    check_resonant_oscillations_and_generation(universe, db_engine);

    /* 4. NEU: Abstrahlung im offenen Dipolraum & Spektralanalyse */
    check_em_waves_and_spectral_distribution(universe, db_engine);

    /* 5. INTERAKTIVER FORMEL-ABGLEICH (Gating-Zentrale) */
    execute_interactive_electrotech_lab(universe, db_engine);

    first_elec_tick = false;
}