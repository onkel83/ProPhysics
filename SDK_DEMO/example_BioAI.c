/**
 * @file example_BioAI.c
 * @brief BioAI SDK - Interaktiver, neuromorpher Multi-Tick Mikroschaltkreis.
 *
 * Kombiniert alle 9 Kern-Mechanismen (Wave, Crossover, Hebbian, Pruning,
 * Homeostasis, Neuromodulation, STDP, Structural Plasticity, Temporal Binding)
 * in einem branchless ausgeführten System-Kernel.
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
 * Vollsymmetrische, branchless Auswertung aller 9 biologischen Core-Regeln in O(1).
 */
void BioAI_Unified_Kernel(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    // Zustands-Ducting initialisieren
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    // Symmetrieprüfung: Beide Knoten müssen valide Neuronen sein
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
    // Branchless-Muxing: (3 * active) | (1 * !active) verhindert Pipeline-Flushes
    uint64_t modulation_active = (modulator > 0ULL);
    uint64_t signal_gain = (3ULL * modulation_active) | (1ULL * !modulation_active);
    uint64_t transmitted_energy = 10ULL * signal_gain;

    /* 2. TEMPORALE BINDUNG & KOHÄRENZ-GATING [Modul 9] */
    // Absolute Phasendifferenz berechnen (Branchless Betrag ohne abs())
    int64_t phase_diff = (int64_t)pre_tick - (int64_t)post_tick;
    uint64_t phase_mask = (uint64_t)(phase_diff >> 63);
    uint64_t abs_phase_diff = (uint64_t)((phase_diff ^ phase_mask) - phase_mask);

    // Kohärenz-Verstärkung oder Dämpfung (Crossover-Interferenz)
    uint64_t is_coherent = (abs_phase_diff <= PHASE_TOLERANCE);
    transmitted_energy = (transmitted_energy * 2ULL * (is_coherent && is_valid)) |
        ((transmitted_energy / 2ULL) * (!is_coherent && is_valid)) |
        (transmitted_energy * !is_valid);

    /* 3. LERNEN: STDP & HEBBIAN PLASTIZITÄT [Modul 3, 7] */
    // LTP (Kausale Korrelation: Post folgt kurz auf Pre)
    uint64_t is_ltp = is_valid && (post_tick > pre_tick) && ((post_tick - pre_tick) <= STDP_WINDOW);
    // LTD (Anti-kausale Depression: Pre folgt auf Post)
    uint64_t is_ltd = is_valid && (pre_tick > post_tick) && ((pre_tick - post_tick) <= STDP_WINDOW);

    uint64_t next_weight = weight;
    uint64_t weight_inc = next_weight + 4ULL;
    uint64_t weight_dec = next_weight - (4ULL * (next_weight >= 4ULL)); // Unterlauf-Schutz

    // Obergrenze bei 100 kappen (Branchless Clamp)
    weight_inc = (100ULL * (weight_inc > 100ULL)) | (weight_inc * (weight_inc <= 100ULL));

    next_weight = (weight_inc * is_ltp) | (next_weight * !is_ltp);
    next_weight = (weight_dec * is_ltd) | (next_weight * !is_ltd);
    current_channels[3] = next_weight;

    /* 4. HOMÖOSTASE (Sättigungsschutz) [Modul 5] */
    // Empfänger-Knoten akkumuliert Energie und regelt bei Überschreiten homöostatisch gegen
    uint64_t new_target_amp = target_amp + transmitted_energy;
    uint64_t homeostatic_damping = 2ULL * (new_target_amp > TARGET_AMPLITUDE);
    target_channels[3] = new_target_amp - (homeostatic_damping * (new_target_amp >= homeostatic_damping));

    /* 5. STRUKTURELLES PRUNING [Modul 4, 8] */
    // Wenn synaptisches Gewicht unter Schwelle fällt und kein Schutzfaktor (Modulator) stützt
    uint64_t is_degenerated = (next_weight < PRUNING_THRESHOLD);
    uint64_t has_no_protection = (modulator == 0ULL);
    uint64_t trigger_pruning = is_valid && is_degenerated && has_no_protection;

    // Topologischen Link kappen (auf Sentinel setzen)
    current_channels[0] = (target_id * !trigger_pruning) | (UNLINKED_SENTINEL * trigger_pruning);
}

int main(void) {
    printf("==================================================================\n");
    printf("[BioAI Core Unified Kernel]: 9-Module Multi-Tick Simulator        \n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie den Szenario-Modus:\n");
    printf(" [1] Optimaler Kausal-Pfad  (Kohaerent, Synchronisiert -> LTP & Boosting)\n");
    printf(" [2] Degenerativer Zerfall  (Asynchron, Niedriges Gewicht -> Pruning)\n");
    printf(" [3] Interaktive Sandbox   (Alle Parameter frei konfigurierbar)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;

    // Standard-Parameter initialisieren
    uint64_t init_weight = 40ULL;
    uint64_t init_pre_tick = 10ULL;
    uint64_t init_post_tick = 12ULL;
    uint64_t init_modulator = 50ULL;
    uint64_t init_target_amp = 10ULL;
    int mode_spike = 1; // 1 = Rhythmisch, 2 = Einmaliges Ereignis
    int total_ticks = 10;

    if (choice == 2) {
        init_weight = 4ULL;
        init_pre_tick = 10ULL;
        init_post_tick = 90ULL;
        init_modulator = 0ULL;
        init_target_amp = 10ULL;
    }
    else if (choice == 3) {
        printf("\n--- SANDBOX PARAMS ---\n");
        printf("Start-Gewicht der Synapse (0-100) [z.B. 20]: ");
        if (scanf("%llu", &init_weight) != 1) init_weight = 20ULL;

        printf("Pre-Spike Tick (Sender) [z.B. 10]: ");
        if (scanf("%llu", &init_pre_tick) != 1) init_pre_tick = 10ULL;

        printf("Post-Spike Tick (Empfaenger) [z.B. 12]: ");
        if (scanf("%llu", &init_post_tick) != 1) init_post_tick = 12ULL;

        printf("Modulator-Level / Schutzfaktor [0 = Kein Schutz, >0 = Aktiv]: ");
        if (scanf("%llu", &init_modulator) != 1) init_modulator = 0ULL;

        printf("Start-Amplitude des Empfaengers [z.B. 10]: ");
        if (scanf("%llu", &init_target_amp) != 1) init_target_amp = 10ULL;
    }

    printf("\nWaehlen Sie das Zeitverhalten der Spikes über Ticks:\n");
    printf(" [1] Rhythmisch / Periodisch (Phase/Delta bleibt pro Tick konstant)\n");
    printf(" [2] Einmaliges Spike-Ereignis (Spike-Tick verfaellt dynamisch im Zeitverlauf)\n");
    printf("Auswahl (1-2): ");
    if (scanf("%d", &mode_spike) != 1) mode_spike = 1;

    printf("Anzahl der zu simulierenden Ticks (1-100) [z.B. 10]: ");
    if (scanf("%d", &total_ticks) != 1 || total_ticks < 1) total_ticks = 10;

    // ProPhysics Universum initialisieren
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    // Architektonischer Aufbau: Knoten 10 treibt Knoten 20
    pu.ur_grid[10].type_state = TYPE_NEURON;
    pu.ur_grid[20].type_state = TYPE_NEURON;
    pu.reg_source[10].channels[0] = 20ULL; // Topologie-Link

    pu.reg_source[10].channels[1] = init_pre_tick;
    pu.reg_source[20].channels[1] = init_post_tick;
    pu.reg_source[10].channels[2] = init_modulator;
    pu.reg_source[10].channels[3] = init_weight;
    pu.reg_source[20].channels[3] = init_target_amp;

    printf("\n=======================================================================================\n");
    printf("%-5s | %-8s | %-9s | %-13s | %-13s | %-18s\n",
        "Tick", "Pre-Tick", "Post-Tick", "Syn. Gewicht", "Empf. Amp", "Topologie-Link");
    printf("=======================================================================================\n");

    for (int t = 1; t <= total_ticks; t++) {
        uint8_t ns = pu.ur_grid[10].type_state;
        uint8_t nts = pu.ur_grid[20].type_state;

        BioAI_Unified_Kernel(
            pu.ur_grid[10].type_state, pu.ur_grid[20].type_state,
            pu.reg_source[10].channels, pu.reg_source[20].channels,
            &ns, &nts
        );

        pu.ur_grid[10].type_state = ns;
        pu.ur_grid[20].type_state = nts;

        uint64_t link = pu.reg_source[10].channels[0];
        char link_str[32];
        if (link == UNLINKED_SENTINEL) {
            snprintf(link_str, sizeof(link_str), "UNLINKED (Pruned)");
        }
        else {
            snprintf(link_str, sizeof(link_str), "Node %llu", link);
        }

        printf("%-5d | %-8llu | %-9llu | %-13llu | %-13llu | %-18s\n",
            t,
            pu.reg_source[10].channels[1],
            pu.reg_source[20].channels[1],
            pu.reg_source[10].channels[3],
            pu.reg_source[20].channels[3],
            link_str);

        // Fortlaufende Zeitdynamik anpassen
        if (mode_spike == 1) {
            // Rhythmisch: Spikes wandern synchron mit
            pu.reg_source[10].channels[1]++;
            pu.reg_source[20].channels[1]++;
        }
        else {
            // Einmaliges Ereignis: Nur der Empfänger-Tick / System-Tick driftet ab
            pu.reg_source[20].channels[1]++;
        }
    }

    printf("=======================================================================================\n\n");

    ProPhysics_Free(&pu);
    return 0;
}