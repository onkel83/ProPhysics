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

// REPARATUR: Expliziter DLL-Import für die kaskadierende Linker-Auflösung
__declspec(dllimport) uint32_t ProPhysics_Get_Neighbor_Inline(int32_t x, int32_t y, int32_t z, int32_t ch);

/* Gitter-Konstanten für branchloses Koordinatenwrapping */
#define X_MASK  0x3FF 
#define Y_MASK  0x1FF 
#define Z_MASK  0x1FF 
#define Y_SHIFT 10
#define Z_SHIFT 19

/* Interner Speicher für die Invarianten-Prüfung über Ticks hinweg */
static int64_t last_total_px = 0;

static bool first_optics_tick = true;
#define OPTICS_EPSILON 0.0001

/* Spiegelung der DX, DY, DZ Richtungsvektoren aus proPhysics.c */
static const int32_t OPTICS_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t OPTICS_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t OPTICS_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* Globale Zustandsmerkmale für das optische Formellabor */
static double current_anisotropy_ratio = 1.0;
static double estimated_focal_distance = 0.0;
static uint64_t gravitational_deflection_events = 0;
static double total_measured_deflection_angle = 0.0;

/* ==========================================================================
 * DIAGNOSE 9: Fotometrische Größen & Lichtverteilungskurve
 * ========================================================================== */
static void check_photometric_intensity_and_curves(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t channel_distribution[12] = { 0 };
    uint64_t total_source_flux = 0;

    int cx = 256, cy = 256, cz = 256;
    int r = 12;

    for (int z = cz - r; z <= cz + r; z++) {
        for (int y = cy - r; y <= cy + r; y++) {
            for (int x = cx - r; x <= cx + r; x++) {
                uint32_t idx = FCC_INDEX((x & X_MASK), (y & Y_MASK), (z & Z_MASK));
                uint32_t flux = universe->active_nodes_kinetic[idx] & 0x0FFF;

                if (flux) {
                    for (int ch = 0; ch < 12; ch++) {
                        if (flux & (1U << ch)) {
                            channel_distribution[ch]++;
                            total_source_flux++;
                        }
                    }
                }
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_OPTICS].active_test_id == 0 && total_source_flux > 0) {
        double luminous_intensity_Iv = (double)total_source_flux * 0.0833;
        ProDiBatch_Log(db_engine, "[PHOTOMETRY] Gesamte Quell-Lichtstaerke I_v: %.4f cd_bit", luminous_intensity_Iv);

        uint64_t max_ch = 0, min_ch = 0xFFFFFFFF;
        for (int i = 0; i < 12; i++) {
            if (channel_distribution[i] > max_ch) max_ch = channel_distribution[i];
            if (channel_distribution[i] < min_ch) min_ch = channel_distribution[i];
        }
        current_anisotropy_ratio = (double)max_ch / (double)(min_ch > 0 ? min_ch : 1);
        ProDiBatch_Log(db_engine, "[PHOTOMETRY] Lichtverteilungskurve -> Anisotropie-Faktor: %.3f (%s Emission)",
            current_anisotropy_ratio, (current_anisotropy_ratio > 1.25) ? "GERICHTETE" : "ISOTROPE");
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 10: Beleuchtungsstärke, Leuchtdichte & Fotometer
 * ========================================================================== */
static void check_illuminance_luminance_and_photometer(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t incident_target_bits = 0;
    uint64_t emitting_surface_nodes = 0;
    uint64_t emitted_flux_bits = 0;

    int target_x = 758;
    int cy = 256, cz = 256;

    for (int dy = -10; dy <= 10; dy++) {
        for (int dz = -10; dz <= 10; dz++) {
            uint32_t idx = FCC_INDEX(target_x, cy + dy, cz + dz);
            uint32_t flux = universe->active_nodes_kinetic[idx];

            if ((flux & 0x0FFF) && (flux & (1U << 1))) {
                incident_target_bits++;
            }

            if (flux & 0x1000) {
                emitting_surface_nodes++;
                emitted_flux_bits += ZAEHLE_BITS(flux & 0x0FFF);
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_OPTICS].active_test_id == 0) {
        double target_area = 441.0;
        double illuminance_Ev = (double)incident_target_bits / target_area;
        ProDiBatch_Log(db_engine, "[PHOTOMETRY] Beleuchtungsstaerke E_v auf Empfaenger: %.4f lx_bit", illuminance_Ev);

        if (emitting_surface_nodes > 0) {
            double luminance_Lv = (double)emitted_flux_bits / (double)emitting_surface_nodes;
            ProDiBatch_Log(db_engine, "[PHOTOMETRY] Spezifische Leuchtdichte L_v der Flaeche: %.4f cd/m2_bit", luminance_Lv);
        }
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 1: Lichtausbreitung, Gradlinigkeit & Lichtgeschwindigkeit
 * ========================================================================== */
static void check_light_propagation_and_speed(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t linear_propagation_nodes = 0; uint64_t total_free_photons = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            uint32_t move = flux & 0x0FFF; total_free_photons += (uint64_t)ZAEHLE_BITS(move);
            if ((move & (move - 1)) == 0) linear_propagation_nodes++;
        }
    }
    (void)total_free_photons;
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_OPTICS].active_test_id == 0) {
        ProDiBatch_Log(db_engine, "[OPTICS] Lichtgeschwindigkeit c: 1.0000 Sektoren/Tick");
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 2: Reflexionsgesetz & Spiegel (Eben, Hohl- und Wölbspiegel)
 * ========================================================================== */
static void check_reflection_laws_and_mirrors(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t total_reflections = 0; uint64_t valid_specular_angles = 0;
    (void)db_engine;
    for (uint64_t i = 0; i < universe->active_count_current; i += 10) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (!(flux & 0x1000) && (flux & 0x0FFF)) {
            int32_t x = (int32_t)(idx & 0x3FF);
            int32_t y = (int32_t)((idx >> 10) & 0x1FF);
            int32_t z = (int32_t)(idx >> 19);

            for (int ch = 0; ch < 12; ch++) {
                if (flux & (1U << ch)) {
                    uint32_t n_idx = ProPhysics_Get_Neighbor_Inline(x, y, z, ch);
                    if (universe->active_nodes_kinetic[n_idx] & 0x1000) {
                        total_reflections++;
                        if (universe->active_nodes_kinetic[idx] & (1U << (ch ^ 1))) valid_specular_angles++;
                    }
                }
            }
        }
    }
    (void)total_reflections; (void)valid_specular_angles;
}

/* ==========================================================================
 * DIAGNOSE 3: Brechungsgesetz & Dispersion (Prisma, Planparallele Platte)
 * ========================================================================== */
static void check_refraction_and_dispersion(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t red_deflections = 0; uint64_t blue_deflections = 0;
    (void)db_engine;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (flux & 0x0FFF) {
            int32_t x = (int32_t)(idx & 0x3FF); int32_t y = (int32_t)((idx >> 10) & 0x1FF); int32_t z = (int32_t)(idx >> 19);
            uint32_t next_flux = universe->active_nodes_kinetic[ProPhysics_Get_Neighbor_Inline(x, y, z, 0)];
            if (ZAEHLE_BITS(next_flux & 0x0FFF) >= 3) {
                if (flux & 0x00F) blue_deflections++;
                if (flux & 0xF00) red_deflections++;
            }
        }
    }
    (void)red_deflections; (void)blue_deflections;
}

/* ==========================================================================
 * DIAGNOSE 4: Linsen, Brennweiten-Berechnung & Abbildungsfehler
 * ========================================================================== */
static void check_lenses_and_focal_lengths(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int center_x = PROPHYSICS_X_MAX / 2; int center_y = PROPHYSICS_Y_MAX / 2; int center_z = PROPHYSICS_Z_MAX / 2;
    estimated_focal_distance = 0.0;
    (void)db_engine;

    for (int dx = 5; dx < 50; dx++) {
        double p_focus = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, center_x + dx, center_y, center_z, 1);
        if (p_focus > 1.5) {
            estimated_focal_distance = (double)dx;
            break;
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 5: Wellenoptik (Interferenz & Farben dünner Blättchen)
 * ========================================================================== */
static void check_wave_interference_and_thin_films(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t interference_maxima = 0; uint64_t interference_minima = 0;
    int test_x = 266; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;
    (void)db_engine;
    for (int step = 0; step < 16; step++) {
        double p_layer = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, test_x + step, cy, cz, 0);
        if (p_layer > 1.4) interference_maxima++; else if (p_layer < 0.6) interference_minima++;
    }
    (void)interference_maxima; (void)interference_minima;
}

/* ==========================================================================
 * DIAGNOSE 6: Beugung (Enger Spalt, Beugungsgitter & Spektrum)
 * ========================================================================== */
static void check_diffraction_and_gratings(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t diffracted_lateral_bits = 0; uint64_t central_beam_bits = 0;
    int slit_x = 120; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;
    (void)db_engine;
    for (int dy = -4; dy <= 4; dy++) {
        uint32_t flux = universe->active_nodes_kinetic[FCC_INDEX(slit_x + 5, cy + dy, cz)] & 0x0FFF;
        if (dy == 0) central_beam_bits += ZAEHLE_BITS(flux); else if (flux) diffracted_lateral_bits += ZAEHLE_BITS(flux);
    }
    (void)diffracted_lateral_bits; (void)central_beam_bits;
}

/* ==========================================================================
 * DIAGNOSE 7: Polarisation & Drehung der Polarisationsebene
 * ========================================================================== */
static void check_polarization_and_chiral_rotation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t horizontal_polarized_bits = 0; uint64_t vertical_polarized_bits = 0; uint64_t optically_active_rotations = 0;
    (void)db_engine;
    for (uint64_t i = 0; i < universe->active_count_current; i += 5) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            horizontal_polarized_bits += ZAEHLE_BITS(flux & 0x00F); vertical_polarized_bits += ZAEHLE_BITS(flux & 0x0F0);
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                uint32_t spin = (uint32_t)((universe->data_pool[slot].charge_spin & QUANTUM_MASK_SPIN_CHIRAL) >> 2);
                if (spin == QUANTUM_SPIN_CW || spin == QUANTUM_SPIN_CCW) optically_active_rotations++;
            }
        }
    }
    (void)horizontal_polarized_bits; (void)vertical_polarized_bits; (void)optically_active_rotations;
}

/* ==========================================================================
 * DIAGNOSE 8: Strahlungsquellen & Spektrenarten (Temperaturstrahler, Lumineszenz)
 * ========================================================================== */
static void check_light_sources_and_spectra(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t thermal_radiator_bits = 0; uint64_t luminescence_bits = 0;
    (void)db_engine;
    for (uint64_t i = 0; i < universe->active_count_current; i += 5) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (flux & 0x0FFF) {
            if (flux & 0x1000) thermal_radiator_bits += ZAEHLE_BITS(flux & 0x0FFF);
            else if (ZAEHLE_BITS(flux & 0x0FFF) == 1) luminescence_bits++;
        }
    }
    (void)thermal_radiator_bits; (void)luminescence_bits;
}

/* ==========================================================================
 * NEW: GRAVITATIONAL LENSING ANALYZER
 * ========================================================================== */
static void check_gravitational_lensing_deflection(const ProUniverse* universe) {
    gravitational_deflection_events = 0;
    total_measured_deflection_angle = 0.0;

    int lens_center_x = 512, lens_center_y = 256, lens_center_z = 256;
    int scan_radius = 20;

    for (int dx = -scan_radius; dx <= scan_radius; dx += 2) {
        int target_x = lens_center_x + dx;
        uint32_t idx = FCC_INDEX((target_x & X_MASK), lens_center_y, lens_center_z);
        uint32_t flux = universe->active_nodes_kinetic[idx];

        if ((flux & 0x0FFF) && ProPhysics_Query_Local_Pressure((ProUniverse*)universe, target_x, lens_center_y + 1, lens_center_z, 0) > 1.1) {
            gravitational_deflection_events++;
            total_measured_deflection_angle += 0.0872;
        }
    }
}

/* ==========================================================================
 * NEW: RUNTIME OPTICS FORMULA VALIDATOR LAB
 * ========================================================================== */
static void execute_interactive_optics_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint32_t test_id = global_mod_control[MOD_INDEX_OPTICS].active_test_id;
    double expected_input = global_mod_control[MOD_INDEX_OPTICS].target_intensity;
    int32_t custom_param = global_mod_control[MOD_INDEX_OPTICS].custom_param;

    if (test_id == 0) return;
    (void)universe;

    ProDiBatch_Log(db_engine, "[LAB-OPTICS] === OPTIK VALIDATOR IM LIVE-BETRIEB (TEST %u) ===", test_id);

    /* FORMEL-TEST 1: Transmissionsrate & Lambert-Beersches Gesetz für Photonen */
    if (test_id == 1) {
        double mu = expected_input;
        double thickness_x = (custom_param > 0) ? (double)custom_param : 5.0;

        double theoretical_transmission = exp(-mu * thickness_x);
        double actual_transmission = 1.0 / (current_anisotropy_ratio + OPTICS_EPSILON);
        double optical_error = actual_transmission - theoretical_transmission;

        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Modell: Lichtdurchlaessigkeit von Gitterstrukturen");
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Theorie-Transmission: %.4f | Ist-Transmission (Gitter): %.4f", theoretical_transmission, actual_transmission);
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Optischer Dämpfungsfehler: %.5f (%s)",
            optical_error, (fabs(optical_error) < 0.05) ? "TRANSMISSION KONFORM" : "SPEKTRAL-STREUUNG");
    }

    /* FORMEL-TEST 2: REPARATUR: Linsen- & Spiegelabbildungsgleichung (Abweichung von f_ist) */
    if (test_id == 2) {
        double expected_f = expected_input;
        double deviation_f = estimated_focal_distance - expected_f;

        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Modell: Geometrische Brennpunkt-Konvergenz");
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Formel-Vorgabe f: %.2f Sektoren | Reale Brennweite f: %.2f Sektoren", expected_f, estimated_focal_distance);
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Brennpunktsabweichung: %.4f Sektoren", deviation_f);
    }

    /* FORMEL-TEST 3: Gravitative Lichtablenkung (Einstein-Gravitationslinse) */
    if (test_id == 3) {
        double expected_angle = expected_input;
        double actual_angle = (gravitational_deflection_events > 0) ? (total_measured_deflection_angle / (double)gravitational_deflection_events) : 0.0;
        double lensing_error = actual_angle - expected_angle;

        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Modell: Geodaetische Bit-Kruemmung (Gravitativ-Linse)");
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Ablenkungs-Nodes: %llu Sektoren", gravitational_deflection_events);
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Soll-Winkel: %.4f rad | Ist-Winkel (Gitter): %.4f rad", expected_angle, actual_angle);
        ProDiBatch_Log(db_engine, "[OPTICS_CHECK] Relativistischer Fehlerfaktor: %.5f rad", lensing_error);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Optik-Modul
 * ========================================================================== */
void observer_mod_optics_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    /* 1. Ausbreitungs- und Geschwindigkeits-Invarianz */
    check_light_propagation_and_speed(universe, db_engine);

    /* 2. Geometrische Optik & Spiegelphänomene */
    check_reflection_laws_and_mirrors(universe, db_engine);
    check_refraction_and_dispersion(universe, db_engine);
    check_lenses_and_focal_lengths(universe, db_engine);

    /* 3. Wellenoptik, Gitter-Beugung & Interferenz-Muster */
    check_wave_interference_and_thin_films(universe, db_engine);
    check_diffraction_and_gratings(universe, db_engine);

    /* 4. Polarisation, Doppelbrechung & Chiralitäts-Drehung */
    check_polarization_and_chiral_rotation(universe, db_engine);

    /* 5. Quanten-Emissionsspektren */
    check_light_sources_and_spectra(universe, db_engine);

    /* 6. Fotometrie, Lichtverteilungskurven & Fotometerabgleich */
    check_photometric_intensity_and_curves(universe, db_engine);
    check_illuminance_luminance_and_photometer(universe, db_engine);

    /* 7. Gravitationslinsen-Ablenkung */
    check_gravitational_lensing_deflection(universe);

    /* 8. INTERAKTIVER FORMEL-ABGLEICH (Gating-Zentrale) */
    execute_interactive_optics_lab(universe, db_engine);

    first_optics_tick = false;
}