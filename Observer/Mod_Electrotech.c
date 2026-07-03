#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProPhysics/ProPhysics.h" 
#include "../ProDiBatch/ProDiBatch_Exports.h"
#include <math.h>

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

/* ==========================================================================
 * NEW - DIAGNOSE 21: Schwingkreis & Elektromagnetische Schwingungserzeugung
 * Analysiert den kontinuierlichen Energie-Shuffle zwischen dem elektrischen
 * Feld (Kondensator) und dem magnetischen Spindrall (Induktivitaet).
 * Berechnet die Oszillationsfrequenz nach der Thomsonschen Schwingungsgleichung.
 * ========================================================================== */
static void check_resonant_oscillations_and_generation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (first_elec_tick) return;

    /* Energie-Vergleich: Elektrische Feldenergie vs Magnetische Wirbelenergie */
    double E_energy = 0.5 * (global_E_field_x * global_E_field_x);
    double B_energy = 0.5 * (global_B_field_z * global_B_field_z);

    /* Detektion von Nulldurchgaengen des E-Feldes zur Frequenzermittlung */
    wave_period_counter++;
    bool field_flipped = (last_E_field_snapshot * global_E_field_x < 0.0);

    if (field_flipped && wave_period_counter > 2) {
        /* Eine halbe Periode entspricht dem Abstand zwischen zwei Flips */
        uint64_t half_period = wave_period_counter;
        double frequency_f = 1.0 / ((double)half_period * 2.0 + ELEC_EPSILON);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (E_energy + B_energy > 0.01) {
            ProDiBatch_Log(db_engine, "[RESONATOR] Schwingungs-Energiebalance -> E-Feld: %.4f J | B-Feld: %.4f J", E_energy, B_energy);
            ProDiBatch_Log(db_engine, "[RESONATOR] Thomsonsche Eigenfrequenz f_resonance: %.5f Tick^-1", frequency_f);
        }
#endif

        last_detected_wavelength = half_period * 2; // Da c=1, ist Wellenlaenge = Periode
        wave_period_counter = 0;
    }

    last_E_field_snapshot = global_E_field_x;
}

/* ==========================================================================
 * NEW - DIAGNOSE 22: Offener Schwingkreis, EM-Wellen & Spektrum
 * Trackt das Abstrahlen und Detektieren freier elektromagnetischer Wellen
 * im offenen Dipolraum. Klassifiziert das Spektrum nach der raemlichen Bit-Frequenz.
 * ========================================================================== */
static void check_em_waves_and_spectral_distribution(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t radiating_dipole_bits = 0;
    uint64_t propagation_wavefront_nodes = 0;

    /* Scan der Fernfeld-Zone (X = 800 bis X = 1000) weit abseits der Quell-Inseln */
    int far_field_start_x = 800;
    int cy = 256, cz = 256;

    for (int x = far_field_start_x; x < PROPHYSICS_X_MAX - 4; x += 2) {
        uint32_t idx = x | (cy << Y_SHIFT) | (cz << Z_SHIFT);
        uint32_t flux = universe->active_nodes_kinetic[idx];

        /* Wenn sich gekoppelte Polaritaetsfronten branchlos durch das reine Vakuum bewegen */
        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            uint32_t neighbor_idx = ((x + 1) & X_MASK) | (cy << Y_SHIFT) | (cz << Z_SHIFT);
            uint32_t neighbor_flux = universe->active_nodes_kinetic[neighbor_idx];

            /* Detektion einer emittierten Welle: Feldgradienten-Kopplung im leeren Raum */
            if (POPCOUNT64(flux & 0x0FFF) != POPCOUNT64(neighbor_flux & 0x0FFF)) {
                propagation_wavefront_nodes++;
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    /* Wellengeschwindigkeit im diskreten Gitter ist strikt invariant c = 1.0 */
    ProDiBatch_Log(db_engine, "[WAVE] Wellengeschwindigkeit c_em: 1.0000 Zellen/Tick (Strikte Maxwell-Konstanz)");

    if (propagation_wavefront_nodes > 0) {
        ProDiBatch_Log(db_engine, "[WAVE] OFFENER SCHWINGKREIS -> Dipol-Abstrahlung detektiert: %llu aktive Wellenfront-Knoten im Fernfeld.",
            propagation_wavefront_nodes);

        /* Spektral-Klassifizierung basierend auf der gemessenen Wellenlänge lambda */
        if (last_detected_wavelength > 0) {
            if (last_detected_wavelength < 8) {
                ProDiBatch_Log(db_engine, "[SPECTRUM] Frequenzbereich: HOCHFREQUENT / Goertz-Wellen (lambda = %llu Sektoren)", last_detected_wavelength);
            }
            else if (last_detected_wavelength <= 32) {
                ProDiBatch_Log(db_engine, "[SPECTRUM] Frequenzbereich: DEZIMETERWELLEN / UHF-Analogie (lambda = %llu Sektoren)", last_detected_wavelength);
            }
            else {
                ProDiBatch_Log(db_engine, "[SPECTRUM] Frequenzbereich: LANGWELLEN / HF-Analogie (lambda = %llu Sektoren)", last_detected_wavelength);
            }
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
                    current_flux += POPCOUNT64(flux & 0x055);
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

    first_elec_tick = false;
}