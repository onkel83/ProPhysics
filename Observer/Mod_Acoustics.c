/* ==========================================================================
 * ProPhysics - Acoustics Diagnostic Core
 * File: Mod_Acoustics.c
 * Architecture: C99, Cache-Optimized Monolithic Evaluator Layer
 * Optimierung: Sparse-Tracking Spatial Filtering, O(1) Bit-Masking
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

 // SDK-Schnittstellen
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

extern ModuleTestControl global_mod_control[];

/* Interner Speicher für die Invarianten-Prüfung über Ticks hinweg */
static int64_t last_total_px = 0;

/* Historische Druckwerte zur Schallgeschwindigkeits-Messung */
static double last_p_vacuum = 0.0;
static double last_p_solid = 0.0;
static uint64_t vacuum_travel_ticks = 0;
static uint64_t solid_travel_ticks = 0;
static bool vacuum_pulse_active = false;
static bool solid_pulse_active = false;

/* Wellenanalyse-Speicher */
static double last_local_pressure = 0.0;
static uint64_t zero_crossings = 0;
static uint64_t cycle_ticks = 0;
static double current_measured_frequency = 0.0;
static double current_rms_pressure = 0.0;

/* Speicher für Schwebungen und Interferenz-Erkennung */
static double last_amplitude_envelope = 0.0;
static uint64_t beating_ticks = 0;

static bool first_acoustic_tick = true;

#define KINEMATIC_EPSILON 0.0001
#define KAMMERTON_FREQ 0.0440 

/* Richtungs-Vektoren aus ProPhysics.c spiegeln für die Schallschnelle */
static const int32_t AXIAL_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t AXIAL_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t AXIAL_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

static const double DIATONIC_RATIOS[8] = { 1.0, 1.125, 1.25, 1.3333, 1.5, 1.6667, 1.875, 2.0 };

// Synchronisierte Unpacking-Makros für die 16-Byte-Knoten-Dekomprimierung
#define UNPACK_X(idx) ((int32_t)((idx) & 0x3FF))
#define UNPACK_Y(idx) ((int32_t)(((idx) >> 10) & 0x1FF))
#define UNPACK_Z(idx) ((int32_t)((idx) >> 19))

/* ==========================================================================
 * DIAGNOSE 1: Schallfeldgrößen (Schallschnelle, Schalldruck, Schallstärke)
 * ========================================================================== */
static void check_sound_field_quantities(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double p_curr) {
    int cx = PROPHYSICS_X_MAX / 2;
    int cy = PROPHYSICS_Y_MAX / 2;
    int cz = PROPHYSICS_Z_MAX / 2;

    double sound_pressure_p = p_curr - 1.0;
    double particle_v_x = 0.0, particle_v_y = 0.0, particle_v_z = 0.0;
    uint64_t cell_count = 0;

    // --- REVOLUTIONÄR: SPARSE-FILTERING ERSETZT DIE BRUTE-FORCE 3D-RAUMSCHLEIFE ---
    // Nutzt das dichte Strom-Array der aktiven Knoten, um Cache-Misses abzuwehren[cite: 1]
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        int32_t x = UNPACK_X(idx);
        int32_t y = UNPACK_Y(idx);
        int32_t z = UNPACK_Z(idx);

        // Prüfe branchless, ob der aktive Knoten innerhalb der 5x5x5 Kontrollbox liegt
        if (abs(x - cx) <= 2 && abs(y - cy) <= 2 && abs(z - cz) <= 2) {
            uint32_t flux = universe->active_nodes_kinetic[idx] & 0x0FFF;
            if (flux) {
                for (int ch = 0; ch < 12; ch++) {
                    uint32_t bit_active = (flux >> ch) & 1U;
                    particle_v_x += bit_active * AXIAL_DX[ch];
                    particle_v_y += bit_active * AXIAL_DY[ch];
                    particle_v_z += bit_active * AXIAL_DZ[ch];
                }
                cell_count++;
            }
        }
    }

    if (cell_count > 0) {
        particle_v_x /= cell_count;
        particle_v_y /= cell_count;
        particle_v_z /= cell_count;
    }

    double abs_v_s = sqrt(particle_v_x * particle_v_x + particle_v_y * particle_v_y + particle_v_z * particle_v_z);
    double sound_intensity_I = fabs(sound_pressure_p * abs_v_s);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0 && fabs(sound_pressure_p) > 0.01) {
        ProDiBatch_Log(db_engine, "[FIELD] Schalldruck p: %.4f | Schallschnelle v_s: %.4f | Schallstaerke I: %.4f",
            sound_pressure_p, abs_v_s, sound_intensity_I);
    }
#endif

    current_rms_pressure = sound_pressure_p;
}

/* ==========================================================================
 * DIAGNOSE 2: Schallpegel & Psychoakustik (Hören, Hörfläche, Lautstärke)
 * ========================================================================== */
static void check_decibels_and_hearing(ProDiBatch_Engine* db_engine) {
    if (fabs(current_rms_pressure) <= KINEMATIC_EPSILON) return;

    double reference_p0 = 0.001;
    double db_level = 20.0 * log10(fabs(current_rms_pressure) / reference_p0);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0 && db_level > 0.0) {
        ProDiBatch_Log(db_engine, "[HEARING] Berechneter Schallpegel: %.1f dB", db_level);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 3: Ultraschall (Erzeugung & Eigenschaften)
 * ========================================================================== */
static void check_ultrasound_properties(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (current_measured_frequency > 0.250) {
        uint64_t linear_ray_nodes = 0;

        for (uint64_t i = 0; i < universe->active_count_current; i += 10) {
            uint32_t idx = universe->active_nodes_current[i];
            uint32_t flux = universe->active_nodes_kinetic[idx] & 0x0FFF;

            if (flux && (flux & (flux - 1)) == 0) {
                linear_ray_nodes++;
            }
        }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[ULTRASOUND] US-Strahlungscharakteristik: %llu Wellen-Nodes.", linear_ray_nodes);
        }
#endif
    }
}

/* ==========================================================================
 * DIAGNOSE 4: Schallausbreitung & Medium-abhängige Schallgeschwindigkeiten
 * ========================================================================== */
static void check_medium_dependent_speeds(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double p_vac_src = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 10, 10, 10, 1);
    double p_vac_dst = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 110, 10, 10, 1);
    double p_sol_src = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 250, 256, 256, 1);
    double p_sol_dst = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 270, 256, 256, 1);

    if (!vacuum_pulse_active && !first_acoustic_tick && (p_vac_src - last_p_vacuum > 0.15)) {
        vacuum_pulse_active = true; vacuum_travel_ticks = 0;
    }
    if (vacuum_pulse_active) {
        vacuum_travel_ticks++;
        if (p_vac_dst > 1.04) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0) {
                ProDiBatch_Log(db_engine, "[SPEED_OF_SOUND] Luft/Gas c_s: %.4f Sektoren/Tick", 100.0 / (double)vacuum_travel_ticks);
            }
#endif
            vacuum_pulse_active = false;
        }
    }
    if (!solid_pulse_active && !first_acoustic_tick && (p_sol_src - last_p_solid > 0.3)) {
        solid_pulse_active = true; solid_travel_ticks = 0;
    }
    if (solid_pulse_active) {
        solid_travel_ticks++;
        if (p_sol_dst > 1.2) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0) {
                ProDiBatch_Log(db_engine, "[SPEED_OF_SOUND] Festkoerper c_s: %.4f Sektoren/Tick", 20.0 / (double)solid_travel_ticks);
            }
#endif
            solid_pulse_active = false;
        }
    }
    last_p_vacuum = p_vac_src; last_p_solid = p_sol_src;
}

/* ==========================================================================
 * DIAGNOSE 5: Wellen-Ueberlagerung (Verstaerkung, Ausloeschung, Schwebung)
 * ========================================================================== */
static void check_wave_interference_and_modulation(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double p_curr) {
    if (first_acoustic_tick) return;

    double current_envelope = fabs(p_curr - 1.0);
    double envelope_delta = current_envelope - last_amplitude_envelope;
    beating_ticks++;

    if (envelope_delta * last_amplitude_envelope < 0 && fabs(envelope_delta) > 0.01) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[INTERFERENCE] SCHWEBUNG aktiv! Modulations-Schwingung: %.4f Hz", 1.0 / (double)beating_ticks);
        }
#endif
        beating_ticks = 0;
    }

    uint64_t amplification_nodes = 0; uint64_t cancellation_nodes = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i += 50) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            int32_t x = UNPACK_X(idx);
            int32_t y = UNPACK_Y(idx);
            int32_t z = UNPACK_Z(idx);

            double p_local = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z, 0);
            if (p_local > 1.8) amplification_nodes++;
            else if (fabs(p_local - 1.0) < 0.001 && ZAEHLE_BITS(flux & 0x0FFF) >= 4) cancellation_nodes++;
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0 && (amplification_nodes > 0 || cancellation_nodes > 0)) {
        ProDiBatch_Log(db_engine, "[INTERFERENCE] Superposition -> Verstaerkung: %llu | Ausloeschung: %llu", amplification_nodes, cancellation_nodes);
    }
#endif
    last_amplitude_envelope = current_envelope;
}

/* ==========================================================================
 * DIAGNOSE 6: Akustischer Doppler-Effekt
 * ========================================================================== */
static void check_acoustic_doppler_effect(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (universe->active_count_current > 0) {
        uint32_t idx = universe->active_nodes_current[0];
        uint32_t slot = universe->grid[idx].state_island_idx;
        if (slot != 0) {
            int32_t vx = universe->data_pool[slot].vx;
            if (abs(vx) > 5) {
                int32_t x = UNPACK_X(idx);
                int32_t y = UNPACK_Y(idx);
                int32_t z = UNPACK_Z(idx);

                int sign = (vx > 0) ? 1 : -1;
                double p_front = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + (sign * 5), y, z, 1);
                double p_rear = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x - (sign * 5), y, z, 1);
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
                double ratio = p_front / (p_rear > 0.0 ? p_rear : 1.0);
                if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0 && (ratio > 1.1 || ratio < 0.9)) {
                    ProDiBatch_Log(db_engine, "[DOPPLER] Wellen-Kompression! Front-Staudruck: %.3f | Heck-Dehnung: %.3f", p_front, p_rear);
                }
#endif
            }
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 7: Tonleitern & Stimmungssysteme
 * ========================================================================== */
static void check_musical_scales_and_temperament(ProDiBatch_Engine* db_engine) {
    if (current_measured_frequency <= KINEMATIC_EPSILON) return;
    bool found_diatonic = false;
    for (int i = 0; i < 8; i++) {
        double target_diatonic = KAMMERTON_FREQ * DIATONIC_RATIOS[i];
        if (fabs(current_measured_frequency - target_diatonic) < 0.002) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            if (global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0) {
                ProDiBatch_Log(db_engine, "[HARMONY] Diatonische Stufe %d erkannt! Freq: %.4f", i + 1, current_measured_frequency);
            }
#endif
            found_diatonic = true;
            break;
        }
    }
    if (!found_diatonic && global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id == 0) {
        double n_semitones = 12.0 * (log(current_measured_frequency / KAMMERTON_FREQ) / log(2.0));
        double nearest_semitone = round(n_semitones);
        if (fabs(n_semitones - nearest_semitone) < 0.15) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            ProDiBatch_Log(db_engine, "[TUNING] Gleichschwebender Halbton relativ zum Kammerton: %.0f", nearest_semitone);
#endif
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 8: Basis-Frequenz & Schalldruckpegel
 * ========================================================================== */
static void check_frequency_and_amplitude(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double p_curr) {
    if (first_acoustic_tick) {
        last_local_pressure = p_curr; return;
    }
    (void)universe;
    double p_background = 1.0;
    double amplitude_delta = p_curr - p_background;
    cycle_ticks++;

    if ((last_local_pressure - p_background > 0 && amplitude_delta <= 0) ||
        (last_local_pressure - p_background < 0 && amplitude_delta >= 0)) {
        zero_crossings++;
        if (zero_crossings >= 2) {
            current_measured_frequency = 1.0 / (double)cycle_ticks;
            zero_crossings = 0; cycle_ticks = 0;
        }
    }
    last_local_pressure = p_curr;
}

/* ==========================================================================
 * DIAGNOSE 9: RUNTIME FORMULA VALIDATOR LAB
 * ========================================================================== */
static void execute_interactive_acoustic_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double p_curr) {
    uint32_t test_id = global_mod_control[MOD_INDEX_ACOUSTICS].active_test_id;
    double expected_input_c_s = global_mod_control[MOD_INDEX_ACOUSTICS].target_intensity;
    int32_t distance_param = global_mod_control[MOD_INDEX_ACOUSTICS].custom_param;

    if (test_id == 0) return;
    (void)p_curr;

    ProDiBatch_Log(db_engine, "[LAB-ACOUSTICS] === AKUSTIK VALIDATOR IN BETRIEB (TEST %u) ===", test_id);

    if (test_id == 1) {
        if (distance_param <= 0) distance_param = 100;
        double measured_c_s = (double)distance_param / ((double)vacuum_travel_ticks + KINEMATIC_EPSILON);
        double error_delta = measured_c_s - expected_input_c_s;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Vakuumpuls-Ausbreitung über %d Sektoren", distance_param);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Soll-Vorgabe c_s: %.4f | Ist-Emergenz c_s: %.4f", expected_input_c_s, measured_c_s);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Absoluter Rechenfehler: %.5f (%s)",
            error_delta, (fabs(error_delta) < 0.05) ? "INVARIANZ ERFUELLT" : "REAKTIONSDRIFT");
    }
    else if (test_id == 2) {
        if (universe->active_count_current > 0) {
            uint32_t idx = universe->active_nodes_current[0];
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                double v_source = fabs((double)universe->data_pool[slot].vx);
                double c_medium = 1.0;

                double ratio_theoretical = (1.0 + (v_source / c_medium)) / (1.0 - (v_source / c_medium) + KINEMATIC_EPSILON);

                int32_t x = UNPACK_X(idx);
                int32_t y = UNPACK_Y(idx);
                int32_t z = UNPACK_Z(idx);
                double p_front = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + 5, y, z, 1);
                double p_rear = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x - 5, y, z, 1);
                double ratio_emergent = p_front / (p_rear > 0.0 ? p_rear : 1.0);

                ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Doppler-Verschiebung bei Quellgeschwindigkeit v = %.1f", v_source);
                ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Theorie-Ratio (Formel): %.4f | Real-Ratio (Gitter): %.4f", ratio_theoretical, ratio_emergent);
                ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Abweichung der Wellenfront-Stauchung: %.4f%%", fabs(ratio_emergent - ratio_theoretical) * 100.0);
            }
        }
    }
    else if (test_id == 3) {
        double expected_frequency = expected_input_c_s;
        double delta_f = current_measured_frequency - expected_frequency;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Diatonischer Stimmungskomparator");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Soll-Frequenz: %.4f Hz | Gemessene Gitter-Frequenz: %.4f Hz", expected_frequency, current_measured_frequency);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Frequenzfehler delta_f: %.5f Hz", delta_f);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Akustik-Modul
 * ========================================================================== */
void observer_mod_acoustics_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    int cx = PROPHYSICS_X_MAX / 2; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;
    double p_curr = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, cx, cy, cz, 3);

    check_sound_field_quantities(universe, db_engine, p_curr);
    check_decibels_and_hearing(db_engine);
    check_medium_dependent_speeds(universe, db_engine);
    check_frequency_and_amplitude(universe, db_engine, p_curr);
    check_wave_interference_and_modulation(universe, db_engine, p_curr);
    check_acoustic_doppler_effect(universe, db_engine);
    check_musical_scales_and_temperament(db_engine);
    check_ultrasound_properties(universe, db_engine);

    execute_interactive_acoustic_lab(universe, db_engine, p_curr);

    first_acoustic_tick = false;
}