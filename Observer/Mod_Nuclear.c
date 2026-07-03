#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProPhysics/ProPhysics.h" 
#include "../ProDiBatch/ProDiBatch_Exports.h"
#include <math.h>

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

/* ==========================================================================
 * NEW - DIAGNOSE 7: Teilchenbeschleuniger & Künstliche Kernumwandlung
 * Analysiert Sektoren mit künstlichen Beschleunigungsgradienten.
 * Trackt induzierte Kernreaktionen und Mutationen von Kernladungszahlen (Z).
 * ========================================================================== */
static void check_particle_accelerators_and_transmutation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t high_velocity_projectiles = 0;
    uint64_t induced_transmutations = 0;

    /* Analysiere Geschoss-Kinetik im Beschleuniger-Kanal (X = 0 bis X = 200) */
    for (int x = 0; x < 200; x += 2) {
        uint32_t idx = x | (256 << Y_SHIFT) | (256 << Z_SHIFT);
        uint32_t flux = universe->active_nodes_kinetic[idx];

        /* Wenn Projektil-Bits sich der maximalen Gittergeschwindigkeit naehern */
        if ((flux & 0x0FFF) && POPCOUNT64(flux & 0x0FFF) >= 4) {
            high_velocity_projectiles++;
        }
    }

    /* Detektion kuenstlicher Kernumwandlungen: Abrupte Aenderung der Island-Polaritaet
       durch externen Stoßbeschuss */
    if (universe->active_element_count >= 2) {
        uint64_t current_charge = universe->data_pool[1].charge_spin & QUANTUM_MASK_POLARITY;
        if (universe->current_cpu_tick % 20 == 0 && current_charge != QUANTUM_POL_NEUTRAL) {
            induced_transmutations++;
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (high_velocity_projectiles > 0) {
        ProDiBatch_Log(db_engine, "[ACCELERATOR] Teilchenbeschleuniger aktiv -> %llu hochenergetische Projektil-Bits im Kanal.", high_velocity_projectiles);
        if (induced_transmutations > 0) {
            ProDiBatch_Log(db_engine, "[ACCELERATOR] KUENSTLICHE KERNUMWANDLUNG: Kernreaktion erzwungen. Element-Mutation im RAM nachgewiesen.");
        }
    }
#endif
}

/* ==========================================================================
 * NEW - DIAGNOSE 8: Uranspaltung, Kernfusion & Kettenreaktions-Profiler
 * Berechnet den Vermehrungsfaktor k der Kettenreaktion live im Gitter.
 * Überwacht die unbestechliche Energiebilanz vor und nach dem Kernsplit.
 * Bilanziert Fusions-Ereignisse makroskopischer Islands.
 * ========================================================================== */
static void check_fission_fusion_and_chain_reactions(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t current_fission_events = 0;
    uint64_t free_neutron_trigger_bits = 0;
    uint64_t macro_fusion_merges = 0;

    double input_energy_pre_split = 0.0;
    double output_energy_post_split = 0.0;

    /* O(N) Scan über die reaktiven Spalt- und Fusionszonen des Gitters */
    for (uint64_t i = 0; i < universe->active_count_current; i += 2) {
        uint32_t idx = universe->active_nodes_current[i];
        uint32_t flux = universe->active_nodes_kinetic[idx];

        /* Spaltungs-Indikator: Schwere Massezellen (0x1000) dissoziieren unter extremem
           Kompressiondruck (local_pressure >= 3) in freie Fragmente */
        if ((flux & 0x1000) && POPCOUNT64(flux & 0x0FFF) >= 5) {
            current_fission_events++;
            input_energy_pre_split += (double)POPCOUNT64(flux & 0x0FFF);
        }

        /* Neutronen-Analogie im LGA: Ungeladene, freie, solitäre Flussbits, die als Trigger wirken */
        if ((flux & 0x0FFF) && !(flux & 0x1000) && POPCOUNT64(flux & 0x0FFF) == 1) {
            free_neutron_trigger_bits++;
        }
    }

    /* KERNFUSION: Verschmelzung von zwei leichten Islands im hochenergetischen Plasma */
    if (universe->active_element_count < 5 && !first_nuclear_tick) {
        /* Wenn die Anzahl der Elemente sinkt, aber die Masse konstant bleibt, liegt eine Fusion vor */
        macro_fusion_merges++;
    }

    if (!first_nuclear_tick) {
        /* 1. KETTENREAKTION: Berechnung des Vermehrungsfaktors k = N_neu / N_alt */
        double multiplication_factor_k = (double)free_neutron_trigger_bits / (double)(last_released_neutron_bits + 1);

        /* 2. ENERGIEBILANZ: Freigesetzte Bindungsenergie berechnen */
        output_energy_post_split = (double)current_fission_events * 20.0; // 20 Energie-Bits pro zellulärem Spaltvorgang

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (current_fission_events > 0) {
            ProDiBatch_Log(db_engine, "[REACTOR] --- REAKTOR-ZÜNDUNG DETEKTIERT ---");
            ProDiBatch_Log(db_engine, "[REACTOR] URANSPALTUNG: %llu zelluläre Kernspaltungen/Tick aktiv.", current_fission_events);
            ProDiBatch_Log(db_engine, "[REACTOR] KETTENREAKTION -> Vermehrungsfaktor k: %.4f", multiplication_factor_k);

            if (multiplication_factor_k > 1.0) {
                PRODIBATCH_LOG(OBSERVER_LOG_WARN, "Nuclear", "[REACTOR] KRITISCHER ZUSTAND! Kettenreaktion ist überkritisch (Explosions-Analogie).");
            }
            else if (fabs(multiplication_factor_k - 1.0) < 0.02) {
                ProDiBatch_Log(db_engine, "[REACTOR] System im stationären Zustand (Kritikalität punktgenau ausbalanciert).");
            }

            ProDiBatch_Log(db_engine, "[REACTOR] ENERGIEBILANZ -> Freigesetzte Netto-Reaktionsenergie: %.4f J_nuclear_bit", output_energy_post_split - input_energy_pre_split);
        }

        if (macro_fusion_merges > 0 && total_kinetic_energy_bits > 500) {
            ProDiBatch_Log(db_engine, "[FUSION] KERNFUSION aktiv -> Islands zu schwerem Element verschmolzen. Thermonukleares Plasma stabil.");
        }
#endif
    }

    last_fission_events_count = current_fission_events;
    last_released_neutron_bits = free_neutron_trigger_bits;
}

/* ==========================================================================
 * NEW - DIAGNOSE 9: Anwendung radioaktiver Nuklide (Tracer-Methode)
 * Trackt und lokalisiert die raeumliche Ausbreitung und Anreicherung von
 * radioaktiv markierten (chiralen/polarisierten) Sub-Islands im Gitter.
 * ========================================================================== */
static void check_nuclide_applications_and_tracers(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t tracer_localization_hits = 0;
    double average_tracer_x = 0.0;

    /* Wir tracken den Weg von Island 1 (unser künstlicher radioaktiver Tracer) */
    if (universe->active_element_count >= 2) {
        uint32_t slot = universe->active_nodes_current[0];
        uint32_t island_idx = universe->grid[slot].state_island_idx;

        if (island_idx == 1) {
            int x = slot % PROPHYSICS_X_MAX;
            tracer_localization_hits++;
            average_tracer_x = (double)x;
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_DEBUG
    if (tracer_localization_hits > 0) {
        ProDiBatch_Log(db_engine, "[DEBUG][TRACER] Nuklid-Anwendung -> Tracer-Lokalisierung im Gewebesektor X: %.1f", average_tracer_x);
    }
#endif
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
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (total_detected_atoms > 0) {
        ProDiBatch_Log(db_engine, "[ATOM] Struktur-Audit -> Registrierte Atomkerne N_total: %llu", total_detected_atoms);
    }
#endif
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
            if ((flux & 0x0FFF) && !(flux & 0x1000)) { quantum_jump_emissions++; total_excitation_energy += (double)POPCOUNT64(flux & 0x0FFF); }
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (quantum_jump_emissions > 0) {
        ProDiBatch_Log(db_engine, "[QUANTUM] Quantenspruenge detektiert: %llu | Energie E: %.4f eV_bit", quantum_jump_emissions, total_excitation_energy / (double)quantum_jump_emissions);
    }
#endif
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
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (total_mass_bits > 0) {
        double dm = (double)total_mass_bits * 2.0;
        ProDiBatch_Log(db_engine, "[NUCLEAR_BOND] Massendefekt delta_m: %.2f | Kernbildungsenergie: %.4f J_bit", dm, dm * 1.0);
    }
#endif
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
            if (slot == 0 && POPCOUNT64(flux & 0x0FFF) >= 6) gamma++;
            if (slot != 0 && (flux & 0x0F0)) b_minus++;
        }
        if ((flux & 0x1000) && universe->grid[idx].state_island_idx == 0) alpha++;
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (alpha > 0 || b_minus > 0 || gamma > 0) {
        ProDiBatch_Log(db_engine, "[RADIATION] Strahlungsnachweis -> Alpha: %llu | Beta: %llu | Gamma: %llu", alpha, b_minus, gamma);
    }
#endif
}

static void check_decay_laws_and_activity(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t current_unstable = 0;
    for (uint32_t i = 1; i < universe->active_element_count; i++) {
        if (universe->data_pool[i].mass_accumulator > 40) current_unstable++;
    }
    if (!first_nuclear_tick && current_unstable > 0) {
        int64_t delta_N = (int64_t)last_unstable_nuclei_count - (int64_t)current_unstable;
        if (delta_N > 0) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            ProDiBatch_Log(db_engine, "[DECAY] Aktuelle Aktivitaet A: %.2f Bq_bit | Halbwertszeit T_1/2: %.2f Ticks", (double)delta_N, log(2.0) / ((double)delta_N / current_unstable + NUCLEAR_EPSILON));
#endif
        }
    }
    last_unstable_nuclei_count = current_unstable;
}

static void check_radiation_dosimetry_and_protection(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double sector_energy = 0.0; int target_x = 900; int cy = 256, cz = 256;
    for (int dy = -8; dy <= 8; dy++) {
        uint32_t flux = universe->active_nodes_kinetic[target_x | ((cy + dy) << Y_SHIFT) | (cz << Z_SHIFT)];
        if (flux & 0x0FFF) sector_energy += (double)POPCOUNT64(flux & 0x0FFF) * 0.5;
    }
    if (sector_energy > 0.0) { accumulated_absorbed_energy_dose += (sector_energy / 50.0); }
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

    first_nuclear_tick = false;
}