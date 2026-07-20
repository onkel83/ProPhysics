/**
 * @file example_BioAI.c
 * @brief BioAI SDK - Einheitlicher neuromorpher Mikroschaltkreis.
 *
 * Kombiniert alle 9 Kern-Mechanismen (Wave, Crossover, Hebbian, Pruning,
 * Homeostasis, Neuromodulation, STDP, Structural Plasticity, Temporal Binding)
 * in einem einzigen, branchless ausgeführten System-Kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- System-Konstanten --- */
#define TYPE_NEURON         0x05U
#define UNLINKED_SENTINEL   0xFFFFFFFFFFFFFFFFULL

/* Homöostatische und strukturelle Schwellenwerte */
#define TARGET_AMPLITUDE    50ULL
#define PRUNING_THRESHOLD   5ULL
#define PHASE_TOLERANCE     15ULL
#define STDP_WINDOW         5ULL

/**
 * @brief Der BioAI Unified Kernel.
 * Vollsymmetrische, branchless Auswertung aller 9 biologischen Core-Regeln.
 */
void BioAI_Unified_Kernel(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    // Zustands-Ducting initialisieren
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    uint64_t is_valid = (current_state == TYPE_NEURON) && (target_state == TYPE_NEURON);

    /* ====================================================================
     * REGISTER-ZUORDNUNG (Kanal-Architektur):
     * current_channels[0] = Topologische Ziel-ID
     * current_channels[1] = Aktueller Zeitstempel (Spike-Tick) / Phase
     * current_channels[2] = Modulator-Pegel & Trophische Schutzfaktoren
     * current_channels[3] = Synaptische Stärke (Gewicht) / Signal-Amplitude
     * ==================================================================== */

    uint64_t target_id = current_channels[0];
    uint64_t pre_tick = current_channels[1];
    uint64_t post_tick = target_channels[1];
    uint64_t modulator = current_channels[2];
    uint64_t weight = current_channels[3];
    uint64_t target_amp = target_channels[3];

    /* 1. NEUROMODULATION & SIGNAL-TRANSMISSION [Modul 1, 2, 6] */
    // Signalverdreifachung bei Modulation (Modulator-Level > 0)
    uint64_t modulation_active = (modulator > 0ULL);
    uint64_t signal_gain = (3ULL * modulation_active) | (1ULL * !modulation_active);
    uint64_t transmitted_energy = 10ULL * signal_gain;

    /* 2. TEMPORALE BINDUNG & KOHÄRENZ-GATING [Modul 9] */
    // Absolute Phasendifferenz berechnen (Branchless)
    int64_t phase_diff = (int64_t)pre_tick - (int64_t)post_tick;
    uint64_t phase_mask = (uint64_t)(phase_diff >> 63);
    uint64_t abs_phase_diff = (uint64_t)((phase_diff ^ phase_mask) - phase_mask);

    // Kohärenz-Verstärkung oder Dämpfung (Crossover-Interferenz)
    uint64_t is_coherent = (abs_phase_diff <= PHASE_TOLERANCE);
    transmitted_energy = (transmitted_energy * 2ULL * (is_coherent && is_valid)) |
        ((transmitted_energy / 2ULL) * (!is_coherent && is_valid)) |
        (transmitted_energy * !is_valid);

    /* 3. LERNEN: STDP & HEBBIAN PLASTIZITÄT [Modul 3, 7] */
    // LTP (Kausale Korrelation)
    uint64_t is_ltp = is_valid && (post_tick > pre_tick) && ((post_tick - pre_tick) <= STDP_WINDOW);
    // LTD (Anti-kausale Depression)
    uint64_t is_ltd = is_valid && (pre_tick > post_tick) && ((pre_tick - post_tick) <= STDP_WINDOW);

    uint64_t next_weight = weight;
    uint64_t weight_inc = next_weight + 4ULL;
    uint64_t weight_dec = next_weight - (4ULL * (next_weight >= 4ULL));

    // Obergrenze bei 100 kappen
    weight_inc = (100ULL * (weight_inc > 100ULL)) | (weight_inc * (weight_inc <= 100ULL));

    next_weight = (weight_inc * is_ltp) | (next_weight * !is_ltp);
    next_weight = (weight_dec * is_ltd) | (next_weight * !is_ltd);
    current_channels[3] = next_weight;

    /* 4. HOMÖOSTASE (Sättigungsschutz) [Modul 5] */
    // Empfänger-Knoten akkumuliert die Energie und regelt homöostatisch gegen
    uint64_t new_target_amp = target_amp + transmitted_energy;
    uint64_t homeostatic_damping = 2ULL * (new_target_amp > TARGET_AMPLITUDE);
    target_channels[3] = new_target_amp - (homeostatic_damping * (new_target_amp >= homeostatic_damping));

    /* 5. STRUKTURELLES PRUNING [Modul 4, 8] */
    // Wenn das synaptische Gewicht unter die Schwelle fällt und kein Schutzfaktor (Modulator) stützt
    uint64_t is_degenerated = (next_weight < PRUNING_THRESHOLD);
    uint64_t has_no_protection = (modulator == 0ULL);
    uint64_t trigger_pruning = is_valid && is_degenerated && has_no_protection;

    // Topologischen Link kappen (auf Sentinel setzen)
    current_channels[0] = (target_id * !trigger_pruning) | (UNLINKED_SENTINEL * trigger_pruning);
}

int main(void) {
    printf("==================================================================\n");
    printf("[BioAI Core Unified Kernel]: 9-Module Integrations-Engine         \n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie den Test-Modus fuer die Gesamtfusion:\n");
    printf(" [1] Optimaler Kausal-Pfad  (Kohaerent, Synchronisiert -> LTP & Boosting)\n");
    printf(" [2] Degenerativer Zerfall  (Asynchron, Niedriges Gewicht -> Pruning-Aktivierung)\n");
    printf("Auswahl (1-2): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    // Architektonischer Aufbau: Knoten 10 treibt Knoten 20
    pu.ur_grid[10].type_state = TYPE_NEURON;
    pu.ur_grid[20].type_state = TYPE_NEURON;
    pu.reg_source[10].channels[0] = 20ULL; // Topologie-Link

    if (choice == 1) {
        /* Szenario 1: Perfekt funktionierender Pfad mit Dopamin-Burst */
        pu.reg_source[10].channels[1] = 10ULL; // Pre-Spike bei Tick 10
        pu.reg_source[20].channels[1] = 12ULL; // Post-Spike bei Tick 12 (In-Phase & Kausal)
        pu.reg_source[10].channels[2] = 50ULL; // Modulator aktiv
        pu.reg_source[10].channels[3] = 40ULL; // Gesundes synaptisches Ausgangsgewicht
        pu.reg_source[20].channels[3] = 10ULL; // Start-Amplitude Empfaenger
    }
    else {
        /* Szenario 2: Absterbender, asynchroner Pfad ohne Schutz */
        pu.reg_source[10].channels[1] = 10ULL; // Pre-Spike
        pu.reg_source[20].channels[1] = 90ULL; // Post-Spike weit weg (Asynchron)
        pu.reg_source[10].channels[2] = 0ULL;  // Kein Modulator / Schutz
        pu.reg_source[10].channels[3] = 4ULL;  // Bereits unterm Pruning-Limit (Gewicht = 4)
        pu.reg_source[20].channels[3] = 10ULL;
    }

    printf("\n--- Zustand VOR Kernel-Ausfuehrung ---\n");
    printf("  Synapsen-Gewicht [10]: %llu\n", pu.reg_source[10].channels[3]);
    printf("  Ziel-Verknuepfung [10]: %llu\n", pu.reg_source[10].channels[0]);
    printf("  Ziel-Amplitude [20]:   %llu\n", pu.reg_source[20].channels[3]);

    /* Systemtakt simulieren (Ausführung über den kombinierten Kernel) */
    uint8_t ns = pu.ur_grid[10].type_state;
    uint8_t nts = pu.ur_grid[20].type_state;

    BioAI_Unified_Kernel(
        pu.ur_grid[10].type_state, pu.ur_grid[20].type_state,
        pu.reg_source[10].channels, pu.reg_source[20].channels,
        &ns, &nts
    );

    pu.ur_grid[10].type_state = ns;
    pu.ur_grid[20].type_state = nts;

    printf("\n--- Zustand NACH Kernel-Ausfuehrung ---\n");
    printf("  Synapsen-Gewicht [10]: %llu\n", pu.reg_source[10].channels[3]);
    printf("  Ziel-Amplitude [20]:   %llu (Moduliert & Homoeostatisch gedrosselt)\n", pu.reg_source[20].channels[3]);

    uint64_t final_target = pu.reg_source[10].channels[0];
    printf("  Ziel-Verknuepfung [10]: ");
    if (final_target == UNLINKED_SENTINEL) {
        printf("UNLINKED (Pruning erfolgreich!)\n");
    }
    else {
        printf("%llu (Aktiv & Stabilisiert)\n", final_target);
    }

    ProPhysics_Free(&pu);
    return 0;
}