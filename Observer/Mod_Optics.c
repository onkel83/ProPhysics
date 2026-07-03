#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProPhysics/ProPhysics.h" 
#include "../ProDiBatch/ProDiBatch_Exports.h"
#include <math.h>

static bool first_optics_tick = true;
#define OPTICS_EPSILON 0.0001

/* Spiegelung der DX, DY, DZ Richtungsvektoren aus proPhysics.c */
static const int32_t OPTICS_DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t OPTICS_DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t OPTICS_DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

/* ==========================================================================
 * NEW - DIAGNOSE 9: Fotometrische Größen & Lichtverteilungskurve
 * Analysiert die Lichtstärke (Iv) aktiver Strahlungsquellen und ermittelt
 * die Lichtverteilungskurve über die Belegung der 12 diskreten FCC-Kanäle.
 * ========================================================================== */
static void check_photometric_intensity_and_curves(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t channel_distribution[12] = { 0 };
    uint64_t total_source_flux = 0;

    /* Wir analysieren die Lichtverteilung im Sektor um Planet A (Zentraler Emitter) */
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
    if (total_source_flux > 0) {
        /* Lichtstärke Iv proportional zum Gesamtlumen-Bitstrom */
        double luminous_intensity_Iv = (double)total_source_flux * 0.0833;
        ProDiBatch_Log(db_engine, "[PHOTOMETRY] Gesamte Quell-Lichtstaerke I_v: %.4f cd_bit", luminous_intensity_Iv);

        /* Asymmetrie-Check für die Lichtverteilungskurve (z.B. gerichteter Strahler vs. isotroper Punktstrahler) */
        uint64_t max_ch = 0, min_ch = 0xFFFFFFFF;
        for (int i = 0; i < 12; i++) {
            if (channel_distribution[i] > max_ch) max_ch = channel_distribution[i];
            if (channel_distribution[i] < min_ch) min_ch = channel_distribution[i];
        }
        double anisotropy_ratio = (double)max_ch / (double)(min_ch > 0 ? min_ch : 1);
        ProDiBatch_Log(db_engine, "[PHOTOMETRY] Lichtverteilungskurve -> Anisotropie-Faktor: %.3f (%s Emission)",
            anisotropy_ratio, (anisotropy_ratio > 1.25) ? "GERICHTETE" : "ISOTROPE");
    }
#endif
}

/* ==========================================================================
 * NEW - DIAGNOSE 10: Beleuchtungsstärke, Leuchtdichte & Fotometer
 * Misst die aufschlagende Lichtmenge pro Flaeche (Ev) sowie die spezifische
 * Abstrahlhelligkeit (Lv) von Oberflaechen. Simuliert ein physikalisches Fotometer.
 * ========================================================================== */
static void check_illuminance_luminance_and_photometer(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t incident_target_bits = 0;
    uint64_t emitting_surface_nodes = 0;
    uint64_t emitted_flux_bits = 0;

    /* 1. Beleuchtungsstärke Ev auf der Target-Flanke von Planet B (x = 758) */
    int target_x = 758;
    int cy = 256, cz = 256;

    for (int dy = -10; dy <= 10; dy++) {
        for (int dz = -10; dz <= 10; dz++) {
            uint32_t idx = FCC_INDEX(target_x, cy + dy, cz + dz);
            uint32_t flux = universe->active_nodes_kinetic[idx];

            /* Einstrahlende Photonenbits zählen */
            if ((flux & 0x0FFF) && (flux & (1U << 1))) { // Einlaufende Gegenachse
                incident_target_bits++;
            }

            /* 2. Leuchtdichte Lv: Strahlenabgabe besetzter Quell-Nodes evaluieren */
            if (flux & 0x1000) {
                emitting_surface_nodes++;
                emitted_flux_bits += POPCOUNT64(flux & 0x0FFF);
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    double target_area = 441.0; // 21 * 21 Matrix-Zellen
    double illuminance_Ev = (double)incident_target_bits / target_area;

    ProDiBatch_Log(db_engine, "[PHOTOMETRY] Beleuchtungsstaerke E_v auf Empfaenger: %.4f lx_bit", illuminance_Ev);

    if (emitting_surface_nodes > 0) {
        double luminance_Lv = (double)emitted_flux_bits / (double)emitting_surface_nodes;
        ProDiBatch_Log(db_engine, "[PHOTOMETRY] Spezifische Leuchtdichte L_v der Flaeche: %.4f cd/m2_bit", luminance_Lv);
    }

    /* 3. Virtueller FOTOMETER-Vergleich (Differenzmessung Sektor Links vs. Sektor Rechts) */
    double p_reference_lux = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 100, 256, 256, 2);
    double p_measure_lux = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, 924, 256, 256, 2);

    ProDiBatch_Log(db_engine, "[PHOTOMETRY] Fotometer-Abgleich -> Kontrast-Verhältnis (Mess/Referenz): %.4f",
        p_measure_lux / (p_reference_lux > 0.0 ? p_reference_lux : 1.0));
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
            uint32_t move = flux & 0x0FFF; total_free_photons += (uint64_t)POPCOUNT64(move);
            if ((move & (move - 1)) == 0) linear_propagation_nodes++;
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    ProDiBatch_Log(db_engine, "[OPTICS] Lichtgeschwindigkeit c: 1.0000 Sektoren/Tick");
    if (total_free_photons > 0) {
        ProDiBatch_Log(db_engine, "[OPTICS] Gradlinigkeit: %.2f%%", ((double)linear_propagation_nodes / total_free_photons) * 100.0);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 2: Reflexionsgesetz & Spiegel (Eben, Hohl- und Wölbspiegel)
 * ========================================================================== */
static void check_reflection_laws_and_mirrors(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t total_reflections = 0; uint64_t valid_specular_angles = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i += 10) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (!(flux & 0x1000) && (flux & 0x0FFF)) {
            int x = idx % PROPHYSICS_X_MAX; int y = (idx / PROPHYSICS_X_MAX) % PROPHYSICS_Y_MAX; int z = idx / (PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX);
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
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (total_reflections > 0) {
        ProDiBatch_Log(db_engine, "[MIRROR] Reflexionsanalyse: Einfall = Reflexion zu %.2f%% verifiziert.", ((double)valid_specular_angles / total_reflections) * 100.0);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 3: Brechungsgesetz & Dispersion (Prisma, Planparallele Platte)
 * ========================================================================== */
static void check_refraction_and_dispersion(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t red_deflections = 0; uint64_t blue_deflections = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (flux & 0x0FFF) {
            int x = idx % PROPHYSICS_X_MAX; int y = (idx / PROPHYSICS_X_MAX) % PROPHYSICS_Y_MAX; int z = idx / (PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX);
            uint32_t next_flux = universe->active_nodes_kinetic[ProPhysics_Get_Neighbor_Inline(x, y, z, 0)];
            if (POPCOUNT64(next_flux & 0x0FFF) >= 3) {
                if (flux & 0x00F) blue_deflections++;
                if (flux & 0xF00) red_deflections++;
            }
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (blue_deflections > 0 || red_deflections > 0) {
        ProDiBatch_Log(db_engine, "[REFRACTION] Effektiver Brechungsindex n_medium: %.4f", 1.0 + ((double)(blue_deflections + red_deflections) * 0.001));
        double dispersion_index = (double)blue_deflections / (double)(red_deflections > 0 ? red_deflections : 1);
        if (fabs(dispersion_index - 1.0) > 0.05) ProDiBatch_Log(db_engine, "[DISPERSION] Prismen-Faktor: %.4f", dispersion_index);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 4: Linsen, Brennweiten-Berechnung & Abbildungsfehler
 * ========================================================================== */
static void check_lenses_and_focal_lengths(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int center_x = PROPHYSICS_X_MAX / 2; int center_y = PROPHYSICS_Y_MAX / 2; int center_z = PROPHYSICS_Z_MAX / 2;
    uint64_t focal_convergence_hits = 0; int estimated_focal_distance = 0;
    for (int dx = 5; dx < 50; dx++) {
        double p_focus = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, center_x + dx, center_y, center_z, 1);
        if (p_focus > 1.5) { focal_convergence_hits++; estimated_focal_distance = dx; }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (estimated_focal_distance > 0) {
        ProDiBatch_Log(db_engine, "[LENS] Berechnete Brennweite f: %.2f", (double)estimated_focal_distance);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 5: Wellenoptik (Interferenz & Farben dünner Blättchen)
 * ========================================================================== */
static void check_wave_interference_and_thin_films(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t interference_maxima = 0; uint64_t interference_minima = 0;
    int test_x = 266; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;
    for (int step = 0; step < 16; step++) {
        double p_layer = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, test_x + step, cy, cz, 0);
        if (p_layer > 1.4) interference_maxima++; else if (p_layer < 0.6) interference_minima++;
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (interference_maxima > 0 && interference_minima > 0) {
        ProDiBatch_Log(db_engine, "[WAVE_OPTICS] Duennewand-Interferenz: Maxima: %llu | Minima: %llu", interference_maxima, interference_minima);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 6: Beugung (Enger Spalt, Beugungsgitter & Spektrum)
 * ========================================================================== */
static void check_diffraction_and_gratings(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t diffracted_lateral_bits = 0; uint64_t central_beam_bits = 0;
    int slit_x = 120; int cy = PROPHYSICS_Y_MAX / 2; int cz = PROPHYSICS_Z_MAX / 2;
    for (int dy = -4; dy <= 4; dy++) {
        uint32_t flux = universe->active_nodes_kinetic[FCC_INDEX(slit_x + 5, cy + dy, cz)] & 0x0FFF;
        if (dy == 0) central_beam_bits += POPCOUNT64(flux); else if (flux) diffracted_lateral_bits += POPCOUNT64(flux);
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (diffracted_lateral_bits > 0) {
        double ratio = (double)diffracted_lateral_bits / (double)(central_beam_bits + 1);
        ProDiBatch_Log(db_engine, "[DIFFRACTION] Spalt-Beugungsfaktor: %.4f | Aufloesungsvermoegen: %.4f", ratio, 1.22 / (ratio + OPTICS_EPSILON));
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 7: Polarisation & Drehung der Polarisationsebene
 * ========================================================================== */
static void check_polarization_and_chiral_rotation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t horizontal_polarized_bits = 0; uint64_t vertical_polarized_bits = 0; uint64_t optically_active_rotations = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i += 5) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if ((flux & 0x0FFF) && !(flux & 0x1000)) {
            horizontal_polarized_bits += POPCOUNT64(flux & 0x00F); vertical_polarized_bits += POPCOUNT64(flux & 0x0F0);
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                uint32_t spin = (uint32_t)((universe->data_pool[slot].charge_spin & QUANTUM_MASK_SPIN_CHIRAL) >> 2);
                if (spin == QUANTUM_SPIN_CW || spin == QUANTUM_SPIN_CCW) optically_active_rotations++;
            }
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    uint64_t total_pol = horizontal_polarized_bits + vertical_polarized_bits;
    if (total_pol > 100) {
        double polarization_degree = (double)((int64_t)horizontal_polarized_bits - (int64_t)vertical_polarized_bits) / (double)total_pol;
        ProDiBatch_Log(db_engine, "[POLARIZATION] Polarisationsgrad P: %.4f (%s)", polarization_degree, (fabs(polarization_degree) > 0.2) ? "LINEAR" : "UNPOLARISIERT");
        if (optically_active_rotations > 0) ProDiBatch_Log(db_engine, "[POLARIZATION] Polarisationsebenen-Drehung: %.2f Grad", (double)optically_active_rotations * 57.2958 * 0.01);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 8: Strahlungsquellen & Spektrenarten (Temperaturstrahler, Lumineszenz)
 * ========================================================================== */
static void check_light_sources_and_spectra(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t thermal_radiator_bits = 0; uint64_t luminescence_bits = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i += 5) {
        uint32_t idx = universe->active_nodes_current[i]; uint32_t flux = universe->active_nodes_kinetic[idx];
        if (flux & 0x0FFF) {
            if (flux & 0x1000) thermal_radiator_bits += POPCOUNT64(flux & 0x0FFF);
            else if (POPCOUNT64(flux & 0x0FFF) == 1) luminescence_bits++;
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (thermal_radiator_bits > 0 || luminescence_bits > 0) {
        ProDiBatch_Log(db_engine, "[SPECTRA] Spektrum -> Temperaturstrahler: %llu | Lumineszenz: %llu", thermal_radiator_bits, luminescence_bits);
    }
#endif
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

    /* 6. NEU: Fotometrie, Lichtverteilungskurven & Fotometerabgleich */
    check_photometric_intensity_and_curves(universe, db_engine);
    check_illuminance_luminance_and_photometer(universe, db_engine);

    first_optics_tick = false;
}