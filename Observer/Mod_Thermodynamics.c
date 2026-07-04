/* ==========================================================================
 * ProPhysics - Thermodynamics Diagnostic Core
 * File: Mod_Thermodynamics.c
 * Architecture: C99, Cache-Optimized Monolithic Evaluator Layer
 * Optimierung: Monolithic Loop Fusion, Sparse Spatial Filtering, O(1) Bit-Masking
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

static double global_system_temperature_k = 0.0;
static double last_global_density = 0.0;
static uint64_t last_total_heat_energy = 0;

/* Historische Phasenzähler */
static uint64_t last_bound_solid_nodes = 0;
static uint64_t last_free_vapor_nodes = 0;

/* Historische Zustandsgrößen für das Prozess-Gating (Erster Hauptsatz) */
static double last_p_control = 1.0;
static double last_V_control = 1.0;
static double last_T_control = 0.1;
static double last_U_control = 0.0;

/* Zyklus-Variablen (Carnot- & Entropie-Tracking) */
static double cycle_accumulated_work = 0.0;
static double cycle_heat_absorbed = 0.0;
static double global_entropy_production = 0.0;
static double T_max_seen = 0.0;
static double T_min_seen = 999.0;

static bool first_thermal_tick = true;

#define THERMO_EPSILON 0.0001

/* Gitter-Konstanten aus ProPhysics.c */
#define X_MASK  0x3FF 
#define Y_MASK  0x1FF 
#define Z_MASK  0x1FF 

static const int32_t AXIAL_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t AXIAL_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t AXIAL_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* Globale Register für den integrierten Thermo-Prüfstand */
static double current_gas_Z_factor = 1.0;
static double current_measured_eta = 0.0;
static double current_measured_carnot = 0.0;
static double current_mean_free_path = 0.0;

// Synchronisierte Unpacking-Makros für die 16-Byte-Knoten-Dekomprimierung
#define UNPACK_X(idx) ((int32_t)((idx) & 0x3FF))
#define UNPACK_Y(idx) ((int32_t)(((idx) >> 10) & 0x1FF))
#define UNPACK_Z(idx) ((int32_t)((idx) >> 19))

/* ==========================================================================
 * DIAGNOSE 13: Kinetische Wärmetheorie & Maxwell-Boltzmann-Verteilung
 * ========================================================================== */
static void check_kinetic_theory_and_maxwell_distribution(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t total_molecules = 0;
    double total_island_mass = 0.0;
    double sum_sq_velocity = 0.0;

    uint64_t bin_slow = 0, bin_medium = 0, bin_fast = 0;

    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        uint64_t mass = universe->data_pool[i].mass_accumulator;
        if (mass > 0) {
            double vx = (double)universe->data_pool[i].vx;
            double vy = (double)universe->data_pool[i].vy;
            double vz = (double)universe->data_pool[i].vz;

            double v_sq = vx * vx + vy * vy + vz * vz;
            double abs_v = sqrt(v_sq);

            sum_sq_velocity += v_sq;
            total_island_mass += (double)mass;
            total_molecules++;

            if (abs_v < 4.0) bin_slow++;
            else if (abs_v <= 12.0) bin_medium++;
            else bin_fast++;
        }
    }

    if (total_molecules > 0) {
        double v_rms = sqrt(sum_sq_velocity / (double)total_molecules);
        double avg_kinetic_energy = 0.5 * (total_island_mass / total_molecules) * (v_rms * v_rms);
        (void)avg_kinetic_energy;

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[KINETIC] Molekuelanzahl N: %llu | RMS-Geschwindigkeit v_rms: %.4f", total_molecules, v_rms);
            ProDiBatch_Log(db_engine, "[MAXWELL] Langsam: %llu | Ideal: %llu | Hochenergetisch: %llu", bin_slow, bin_medium, bin_fast);
        }
#endif
    }
}

/* ==========================================================================
 * DIAGNOSE 14: Stosszahl und mittlere freie Weglänge
 * ========================================================================== */
static void check_collisions_and_mean_free_path(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t total_active_bits = 0;
    uint64_t actual_collisions = 0;
    uint64_t total_space_nodes = (uint64_t)PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        uint32_t bits = (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);

        total_active_bits += bits;
        if (bits >= 2) {
            actual_collisions += (bits * (bits - 1)) / 2;
        }
    }

    if (total_active_bits > 0 && total_space_nodes > 0) {
        double mean_collision_rate_Z = (double)actual_collisions / (double)total_active_bits;
        current_mean_free_path = 1.0 / (mean_collision_rate_Z + THERMO_EPSILON);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0 && mean_collision_rate_Z > 0.001) {
            ProDiBatch_Log(db_engine, "[COLLISION] Mittlere freie Weglaenge lambda: %.4f", current_mean_free_path);
        }
#endif
    }
}

/* ==========================================================================
 * DIAGNOSE 3: Dampfsättigung, Luftfeuchtigkeit & Tripelpunkt
 * ========================================================================== */
static void check_vapor_saturation_and_humidity(const ProUniverse* universe, ProDiBatch_Engine* db_engine, uint64_t bound_nodes, uint64_t free_vapor_nodes) {
    if (first_thermal_tick) return;
    (void)bound_nodes;
    (void)universe;
    double max_sat = (global_system_temperature_k * global_system_temperature_k) * (double)(PROPHYSICS_X_MAX) * 2.0;
    if (max_sat > 0.0) {
        double rel_hum = (double)free_vapor_nodes / max_sat;
        if (rel_hum > 1.0) rel_hum = 1.0;
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[VAPOR] relative Feuchte: %.1f%%", rel_hum * 100.0);
        }
#endif
    }
}

/* ==========================================================================
 * DIAGNOSE 11 & 2: Integrierte Sektordichtemessung für Zustandsänderungen & Realgase
 * ========================================================================== */
static void evaluate_localized_gas_chambers(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int radius = 4;
    double V_curr = (4.0 / 3.0) * 3.1415926535 * (radius * radius * radius);

    // Ziel-Zentren der beiden unabhängigen Messkammern
    int t1_x = 256, t1_y = 256, t1_z = 256;
    int t2_x = 50, t2_y = 50, t2_z = 50;

    uint64_t n_particles_ch1 = 0;
    uint64_t n_particles_ch2 = 0;

    // --- REVOLUTIONÄR: EIN EINZIGER SWEEP FILTERT BEIDE MESSKAMMERN CACHE-KONFORM ---
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        int32_t x = UNPACK_X(idx);
        int32_t y = UNPACK_Y(idx);
        int32_t z = UNPACK_Z(idx);

        uint32_t bits = (uint32_t)ZAEHLE_BITS(universe->active_nodes_kinetic[idx] & 0x0FFF);

        if (abs(x - t1_x) <= radius && abs(y - t1_y) <= radius && abs(z - t1_z) <= radius) {
            n_particles_ch1 += bits;
        }
        if (abs(x - t2_x) <= radius && abs(y - t2_y) <= radius && abs(z - t2_z) <= radius) {
            n_particles_ch2 += bits;
        }
    }

    // 1. Auswertung Kammer 1: Polytrope Prozesse & Hauptsatz
    double p_curr = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, t1_x, t1_y, t1_z, radius);
    double V_dynamic = V_curr;
    if (p_curr > 1.2) V_dynamic *= 0.85;
    if (p_curr < 0.8) V_dynamic *= 1.15;

    double T_curr = n_particles_ch1 > 0 ? (double)n_particles_ch1 / V_dynamic : THERMO_EPSILON;
    double U_curr = (double)n_particles_ch1 * T_curr;

    if (!first_thermal_tick) {
        double dU = U_curr - last_U_control;
        double dV = V_dynamic - last_V_control;
        double dW = -0.5 * (p_curr + last_p_control) * dV;
        double dQ = dU - dW;

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0) {
            if (fabs(dV) <= 0.005) ProDiBatch_Log(db_engine, "[PROCESS] Typ: ISOCHORE Zustandsaenderung | dQ = dU = %.4f", dU);
            else if (fabs(p_curr - last_p_control) <= 0.002) ProDiBatch_Log(db_engine, "[PROCESS] Typ: ISOBARE Zustandsaenderung | dW: %.4f", dW);
        }
#endif
        cycle_accumulated_work += dW;
        if (dQ > 0.0) cycle_heat_absorbed += dQ;
        if (T_curr > T_max_seen) T_max_seen = T_curr;
        if (T_curr < T_min_seen) T_min_seen = T_curr;
    }
    last_p_control = p_curr; last_V_control = V_dynamic; last_T_control = T_curr; last_U_control = U_curr;

    // 2. Auswertung Kammer 2: Realgas-Kompressibilität (Z-Faktor)
    double p_pressure = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, t2_x, t2_y, t2_z, radius);
    double T_local = n_particles_ch2 > 0 ? (double)n_particles_ch2 / V_curr : THERMO_EPSILON;

    if (n_particles_ch2 > 0 && global_system_temperature_k > THERMO_EPSILON) {
        current_gas_Z_factor = (p_pressure * V_curr) / ((double)n_particles_ch2 * 1.0 * T_local);
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[REAL_GAS] Kompressibilitaet Z: %.4f", current_gas_Z_factor);
        }
#endif
    }
}

/* ==========================================================================
 * DIAGNOSE 12: Kreisprozesse, Carnot & Zweiter Hauptsatz (Entropie)
 * ========================================================================== */
static void check_cycle_processes_and_second_law(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (first_thermal_tick) return;
    double delta_entropy_field = fabs(universe->global_entropy_index) * 0.00001;
    global_entropy_production += delta_entropy_field;

    if (cycle_heat_absorbed > 0.0 && cycle_accumulated_work < 0.0) {
        current_measured_eta = fabs(cycle_accumulated_work) / cycle_heat_absorbed;
    }
    current_measured_carnot = 1.0 - (T_min_seen / (T_max_seen + THERMO_EPSILON));

    if (universe->current_cpu_tick % 100 == 0) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[CYCLE] Wirkungsgrad eta: %.2f%% | Carnot-Limit: %.2f%%", current_measured_eta * 100.0, current_measured_carnot * 100.0);
            ProDiBatch_Log(db_engine, "[CYCLE] Akkumulierte System-Entropie S_global: %.4f", global_entropy_production);
        }
#endif
        cycle_accumulated_work = 0.0; cycle_heat_absorbed = 0.0;
        T_max_seen = last_T_control; T_min_seen = last_T_control;
    }
}

/* ==========================================================================
 * UNIFIED HARVEST LAYER: Fused Transport- & Strahlungssensorik
 * ========================================================================== */
static void check_heat_propagation_conduction_convection(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    (void)db_engine;
    double cond = 0.0, conv = 0.0; uint64_t boundary_transfers = 0;
    int cx = PROPHYSICS_X_MAX / 2; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;

    for (int z = cz - 2; z <= cz + 2; z++) {
        for (int y = cy - 2; y <= cy + 2; y++) {
            for (int x = cx - 2; x <= cx + 2; x++) {
                uint32_t idx = FCC_INDEX(x, y, z); uint32_t flux = universe->active_nodes_kinetic[idx]; uint32_t bits = (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);
                if (bits > 0) {
                    uint32_t n_idx = FCC_INDEX(((x + 1) & X_MASK), y, z);
                    cond += fabs((double)bits - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + 1, y, z, 0));
                    double n_vx = 0.0, n_vy = 0.0;
                    for (int ch = 0; ch < 12; ch++) {
                        uint32_t bit_active = (flux >> ch) & 1U;
                        n_vx += bit_active * AXIAL_DX[ch];
                        n_vy += bit_active * AXIAL_DY[ch];
                    }
                    conv += sqrt(n_vx * n_vx + n_vy * n_vy);
                    if ((flux & 0x1000) && !(universe->active_nodes_kinetic[n_idx] & 0x1000)) boundary_transfers += bits;
                }
            }
        }
    }
    (void)cond; (void)conv; (void)boundary_transfers;
}

static void check_thermal_mixing_law(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }
static void check_heat_sources_and_conversion(const ProUniverse* universe, ProDiBatch_Engine* db_engine) { (void)universe; (void)db_engine; }

/* ==========================================================================
 * NEW: RUNTIME THERMODYNAMICS FORMULA VALIDATOR LAB
 * ========================================================================== */
static void execute_interactive_thermodynamics_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint32_t test_id = global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id;
    double expected_input = global_mod_control[MOD_INDEX_THERMODYNAMICS].target_intensity;
    int32_t custom_param = global_mod_control[MOD_INDEX_THERMODYNAMICS].custom_param;

    if (test_id == 0) return;

    ProDiBatch_Log(db_engine, "[LAB-THERMO] === THERMODYNAMIK VALIDATOR IM LIVE-BETRIEB (TEST %u) ===", test_id);

    if (test_id == 1) {
        double expected_Z = expected_input;
        double gas_error = current_gas_Z_factor - expected_Z;

        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Modell: Realgas-Kompressibilitaetsfaktor");
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Soll-Faktor Z: %.4f | Ist-Gitter-Faktor Z: %.4f", expected_Z, current_gas_Z_factor);
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Reale Abweichungsrate: %.5f (%s)",
            gas_error, (fabs(gas_error) < 0.05) ? "IDEAL_GAS APPROXIMATION" : "REAKTIONSDRIFT");
    }
    else if (test_id == 2) {
        double expected_efficiency = expected_input;
        double T_hot = last_T_control;
        double T_cold = (custom_param > 0) ? (double)custom_param : 1.0;

        double theoretical_efficiency = 1.0 - (T_cold / (T_hot + THERMO_EPSILON));
        double efficiency_error = current_measured_eta - expected_efficiency;

        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Modell: Carnot-Wirkungsgrad-Abgleich");
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Plasma-T_hot: %.4f | Senken-T_cold: %.4f", T_hot, T_cold);
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Gitter-Effizienz eta: %.4f | Soll-Vorgabe: %.4f", current_measured_eta, expected_efficiency);
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Thermischer Abweichungsfehler: %.5f (Theorie-Limit: %.4f)", efficiency_error, theoretical_efficiency);
    }
    else if (test_id == 3) {
        double expected_lambda = expected_input;
        double particle_density_rho = (double)universe->active_count_current / (double)(PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX);
        double micro_error = current_mean_free_path - expected_lambda;

        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Modell: Mittlere freie Weglaenge der Quanten-Teilchen");
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Lokale Bit-Dichte rho: %.6f Nodes/Zelle", particle_density_rho);
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Soll-Weglaenge: %.4f | Ist-Gitter-Laufweg: %.4f Zellen", expected_lambda, current_mean_free_path);
        ProDiBatch_Log(db_engine, "[THERMO_CHECK] Kinetischer Daempfungsfehler: %.4f Zellen", micro_error);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Thermodynamik-Modul
 * ========================================================================== */
void observer_mod_thermodynamics_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    if (universe->active_count_current > 0) {
        global_system_temperature_k = (double)universe->active_count_current / (double)(PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX);
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_THERMODYNAMICS].active_test_id == 0) {
        ProDiBatch_Log(db_engine, "[THERMO] Globale Temperatur: %.4f K_bit", global_system_temperature_k);
    }
#endif

    // 1. Konsolidierte Lokalisations-Messkammern (Prozesse, Hauptsatz & Realgas Z-Faktor)
    evaluate_localized_gas_chambers(universe, db_engine);

    // 2. MONOLITHIC FUSED LOOP INTERLEAVING
    // Verschmilzt Phasenübergänge, Kalorimetrie und Strahlungsverluste in einen einzigen $O(N)$ Sweep
    uint64_t q_vac = 0; uint64_t q_mat = 0;
    uint64_t bound_solid_liquid_nodes = 0; uint64_t free_vapor_gas_nodes = 0;
    uint64_t emitted = 0; double max_t = 0.0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        uint32_t bits = (uint32_t)ZAEHLE_BITS(flux & 0x0FFF);

        // Kalorimetrie
        if (flux & 0x1000) { q_mat += bits; }
        else { q_vac += bits; }

        // Phasen-Tracking
        if (flux & 0x1000) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0 && universe->data_pool[slot].mass_accumulator > 10) bound_solid_liquid_nodes++;
            else free_vapor_gas_nodes++;
        }

        // Wärmestrahlung (Kopplung im Haupt-Sweep über Sektor-Intervall)
        if (i % 10 == 0 && (flux & 0x1000)) {
            int32_t x = UNPACK_X(idx); int32_t y = UNPACK_Y(idx); int32_t z = UNPACK_Z(idx);
            double local_t = (double)bits; if (local_t > max_t) max_t = local_t;
            uint32_t n_idx = FCC_INDEX(((x + 1) & 0x3FF), y, z);
            if (!(universe->active_nodes_kinetic[n_idx] & 0x1000)) {
                emitted += ZAEHLE_BITS((universe->active_nodes_kinetic[n_idx] & 0x0FFF) & 0x0555);
            }
        }
    }

    check_vapor_saturation_and_humidity(universe, db_engine, bound_solid_liquid_nodes, free_vapor_gas_nodes);

    last_total_heat_energy = q_vac + q_mat;
    last_bound_solid_nodes = bound_solid_liquid_nodes;
    last_free_vapor_nodes = free_vapor_gas_nodes;
    (void)emitted;

    // 3. Modulare Transport- und Wellenprüfer
    check_thermal_mixing_law(universe, db_engine);
    check_heat_sources_and_conversion(universe, db_engine);
    check_heat_propagation_conduction_convection(universe, db_engine);
    check_cycle_processes_and_second_law(universe, db_engine);

    /* --- Kinetische Wärmetheorie, Stoßzahlen & Maxwell-Boltzmann --- */
    check_kinetic_theory_and_maxwell_distribution(universe, db_engine);
    check_collisions_and_mean_free_path(universe, db_engine);

    /* --- INTERAKTIVER FORMEL-ABGLEICH (Gating-Zentrale) --- */
    execute_interactive_thermodynamics_lab(universe, db_engine);

    first_thermal_tick = false;
}