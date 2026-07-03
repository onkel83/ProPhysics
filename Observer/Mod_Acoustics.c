#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProPhysics/ProPhysics.h" 
#include "../ProDiBatch/ProDiBatch_Exports.h"
#include <math.h>
#include <stdlib.h>

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

/* ==========================================================================
 * DIAGNOSE 1: Schallfeldgrößen (Schallschnelle, Schalldruck, Schallstärke)
 * Berechnet die fundamentalen vektoriellen Feldgrößen des akustischen Raums.
 * ========================================================================== */
static void check_sound_field_quantities(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double p_curr) {
    int cx = PROPHYSICS_X_MAX / 2;
    int cy = PROPHYSICS_Y_MAX / 2;
    int cz = PROPHYSICS_Z_MAX / 2;

    /* 1. Schalldruck p bestimmen (Abweichung vom Gleichgewicht) */
    double sound_pressure_p = p_curr - 1.0;

    /* 2. Schallschnelle v_s über den Netto-Photonen-Flussvektor im Sektor aggregieren */
    double particle_v_x = 0.0;
    double particle_v_y = 0.0;
    double particle_v_z = 0.0;
    uint64_t cell_count = 0;

    for (int z = cz - 2; z <= cz + 2; z++) {
        for (int y = cy - 2; y <= cy + 2; y++) {
            for (int x = cx - 2; x <= cx + 2; x++) {
                uint32_t idx = FCC_INDEX(x, y, z);
                uint32_t flux = universe->active_nodes_kinetic[idx] & 0x0FFF;

                if (flux) {
                    for (int ch = 0; ch < 12; ch++) {
                        if (flux & (1U << ch)) {
                            particle_v_x += AXIAL_DX[ch];
                            particle_v_y += AXIAL_DY[ch];
                            particle_v_z += AXIAL_DZ[ch];
                        }
                    }
                    cell_count++;
                }
            }
        }
    }

    if (cell_count > 0) {
        particle_v_x /= cell_count;
        particle_v_y /= cell_count;
        particle_v_z /= cell_count;
    }

    double abs_v_s = sqrt(particle_v_x * particle_v_x + particle_v_y * particle_v_y + particle_v_z * particle_v_z);

    /* 3. Schalldichte E (Energiedichte) & Schallstärke I (p * v_s) */
    double sound_energy_density = 0.5 * (sound_pressure_p * sound_pressure_p);
    double sound_intensity_I = fabs(sound_pressure_p * abs_v_s);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (fabs(sound_pressure_p) > 0.01) {
        ProDiBatch_Log(db_engine, "[FIELD] Schalldruck p: %.4f | Schallschnelle v_s: %.4f | Schallstaerke I: %.4f",
            sound_pressure_p, abs_v_s, sound_intensity_I);
    }
#endif

    current_rms_pressure = sound_pressure_p;
}

/* ==========================================================================
 * DIAGNOSE 2: Schallpegel & Psychoakustik (Hören, Hörfläche, Lautstärke)
 * Konvertiert Feldstärken in ein logarithmisches Dezibel-System (dB) und
 * prueft, ob die Welle innerhalb der virtuellen biologischen Hörfläche liegt.
 * ========================================================================== */
static void check_decibels_and_hearing(ProDiBatch_Engine* db_engine) {
    if (fabs(current_rms_pressure) <= KINEMATIC_EPSILON) return;

    /* Relative Pegelberechnung L_p = 20 * log10(p / p_0) */
    double reference_p0 = 0.001;
    double db_level = 20.0 * log10(fabs(current_rms_pressure) / reference_p0);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (db_level > 0.0) {
        ProDiBatch_Log(db_engine, "[HEARING] Berechneter Schallpegel: %.1f dB (Lautstaerke-Indikator)", db_level);

        /* Gating über die virtuelle Hörfläche (Frequenzgrenzen 0.005 bis 0.250 Hz-Gitterfrequenz) */
        if (current_measured_frequency < 0.005) {
            ProDiBatch_Log(db_engine, "[HEARING] Infraschall detektiert (Frequenz zu tief fuer Standard-Hoerflaeche).");
        }
        else if (current_measured_frequency > 0.250) {
            ProDiBatch_Log(db_engine, "[HEARING] Ultraschall detektiert (Frequenz oberhalb der Standard-Hoerflaeche).");
        }
        else {
            /* Lautstärke-Klassifizierung */
            if (db_level > 80.0) {
                ProDiBatch_Log(db_engine, "[HEARING] Hoerstatus: Schmerzgrenze / Brutale Compression!");
            }
            else if (db_level > 40.0) {
                ProDiBatch_Log(db_engine, "[HEARING] Hoerstatus: Gut hoerbare, stabile Lautstaerke.");
            }
            else {
                ProDiBatch_Log(db_engine, "[HEARING] Hoerstatus: Nahe an der Hoerschwelle (Leises Fluestern).");
            }
        }
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 3: Ultraschall (Erzeugung & Eigenschaften)
 * Analysiert extrem hochfrequente Gitteroszillationen (Wellenlaenge nahe der
 * Gitterkonstante) auf Richtwirkung und absorptionsfreie Transmission.
 * ========================================================================== */
static void check_ultrasound_properties(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    /* Ultraschall zeichnet sich im LGA durch ultrakurze Perioden aus (cycle_ticks <= 3) */
    if (current_measured_frequency > 0.250) {
        uint64_t linear_ray_nodes = 0;

        /* Ultraschall-Eigenschaft: Extreme Richtwirkung (Strahlengeometrie).
           Wir messen, ob die Ausbreitung rein entlang einer Hauptachse gebuendelt bleibt. */
        for (uint64_t i = 0; i < universe->active_count_current; i += 10) {
            uint32_t idx = universe->active_nodes_current[i];
            uint32_t flux = universe->active_nodes_kinetic[idx] & 0x0FFF;

            /* Wenn nur ein einziger Vorwaertskanal gefeuert wird = Perfekter US-Strahl */
            if (flux && (flux & (flux - 1)) == 0) {
                linear_ray_nodes++;
            }
        }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        ProDiBatch_Log(db_engine, "[ULTRASOUND] US-Strahlungscharakteristik: %llu linear gebuendelte Wellen-Nodes.", linear_ray_nodes);
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
            ProDiBatch_Log(db_engine, "[SPEED_OF_SOUND] Luft/Gas (Vakuum) c_s: %.4f Sektoren/Tick", 100.0 / (double)vacuum_travel_ticks);
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
            ProDiBatch_Log(db_engine, "[SPEED_OF_SOUND] Festkoerper c_s: %.4f Sektoren/Tick", 20.0 / (double)solid_travel_ticks);
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
    if (first_tick) return;

    double current_envelope = fabs(p_curr - 1.0);
    double envelope_delta = current_envelope - last_amplitude_envelope;
    beating_ticks++;

    if (envelope_delta * last_amplitude_envelope < 0 && fabs(envelope_delta) > 0.01) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        ProDiBatch_Log(db_engine, "[INTERFERENCE] SCHWEBUNG aktiv! Modulations-Schwingung: %.4f Hz", 1.0 / (double)beating_ticks);
#endif
        beating_ticks = 0;
    }

    uint64_t amplification_nodes = 0; uint64_t cancellation_nodes = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i += 50) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];
        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            int x = idx % PROPHYSICS_X_MAX; int y = (idx / PROPHYSICS_X_MAX) % PROPHYSICS_Y_MAX; int z = idx / (PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX);
            double p_local = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z, 0);
            if (p_local > 1.8) amplification_nodes++;
            else if (fabs(p_local - 1.0) < 0.001 && POPCOUNT64(flux & 0x0FFF) >= 4) cancellation_nodes++;
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (amplification_nodes > 0 || cancellation_nodes > 0) {
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
                int x = idx % PROPHYSICS_X_MAX; int y = (idx / PROPHYSICS_X_MAX) % PROPHYSICS_Y_MAX; int z = idx / (PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX);
                int sign = (vx > 0) ? 1 : -1;
                double p_front = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + (sign * 5), y, z, 1);
                double p_rear = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x - (sign * 5), y, z, 1);
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
                double ratio = p_front / (p_rear > 0.0 ? p_rear : 1.0);
                if (ratio > 1.1 || ratio < 0.9) {
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
            ProDiBatch_Log(db_engine, "[HARMONY] Diatonische Stufe %d erkannt! Freq: %.4f", i + 1, current_measured_frequency);
#endif
            found_diatonic = true;
            break;
        }
    }
    if (!found_diatonic) {
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
 * Haupt-Einstiegspunkt für das Akustik-Modul
 * ========================================================================== */
void observer_mod_acoustics_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    int cx = PROPHYSICS_X_MAX / 2; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;
    double p_curr = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, cx, cy, cz, 3);

    /* 1. Wellen-Feldgrössen & Logarithmische Pegel (dB) */
    check_sound_field_quantities(universe, db_engine, p_curr);
    check_decibels_and_hearing(db_engine);

    /* 2. Dichtemessung und Ausbreitung (Gase, Fluessigkeiten, Feststoffe) */
    check_medium_dependent_speeds(universe, db_engine);

    /* 3. Wellen-Interferenzen (Ueberlagerung, Ausloeschung, Schwebung) */
    check_frequency_and_amplitude(universe, db_engine, p_curr);
    check_wave_interference_and_modulation(universe, db_engine, p_curr);
    check_acoustic_doppler_effect(universe, db_engine);

    /* 4. Tonsysteme & Ultraschall-Eigenschaften */
    check_musical_scales_and_temperament(db_engine);
    check_ultrasound_properties(universe, db_engine);

    first_acoustic_tick = false;
}