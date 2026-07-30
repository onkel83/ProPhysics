/**
 * @file example9_temporal_binding.c
 * @brief BioAI SDK - Temporale Bindung durch Phasen-Synchronisation & Kohaerenz-Gating.
 *
 * Simuliert das synchrone Feuern zweier Knoten. Liegen die internen Phasen
 * (Kanal 1) innerhalb eines Toleranzfensters, kommt es zur Resonanz und
 * Amplitudenverstaerkung (Kanal 3). Weichen sie ab, wird das Signal destruktiv gedaempft.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

#define TYPE_NEURON         0x05U
#define PHASE_TOLERANCE     15ULL   /**< Maximale Phasenabweichung fuer Bindung */
#define RESONANCE_BOOST     25ULL   /**< Amplitudengewinn bei Synchronitaet */
#define DAMPING_FACTOR      5ULL    /**< Signalverlust bei Asynchronitaet */

typedef struct {
    int scenario; /**< 1 = In-Phase (Resonanz), 2 = Out-of-Phase (Gating) */
} PhaseConfig;

static PhaseConfig g_PhaseConfig = { 1 };

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless Phase-Locking & Binding.
 */
void TemporalBindingRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    uint64_t is_valid = (current_state == TYPE_NEURON) && (target_state == TYPE_NEURON);

    uint64_t phase_pre = current_channels[1];
    uint64_t phase_post = target_channels[1];

    /* --- BRANCHLESS ABSOLUTE DIFFERENZ --- */
    int64_t diff = (int64_t)phase_pre - (int64_t)phase_post;
    uint64_t mask = (uint64_t)(diff >> 63);
    uint64_t abs_diff = (uint64_t)((diff ^ mask) - mask);

    /* Pruefen, ob die Phasendifferenz innerhalb der Toleranz liegt */
    uint64_t is_coherent = (abs_diff <= PHASE_TOLERANCE);
    uint64_t is_resonant = is_valid && is_coherent;
    uint64_t is_blocked = is_valid && !is_coherent;

    /* --- SIGNAL-MODULATION (BRANCHLESS) --- */
    uint64_t current_amplitude = target_channels[3];

    /* Bei Resonanz: Verstaerken. Bei Blockade: Daempfen */
    uint64_t boosted_amp = current_amplitude + RESONANCE_BOOST;
    uint64_t damped_amp = (current_amplitude > DAMPING_FACTOR) ? (current_amplitude - DAMPING_FACTOR) : 0ULL;

    target_channels[3] = (current_amplitude * (!is_valid)) |
        (boosted_amp * is_resonant) |
        (damped_amp * is_blocked);
}

int main(void) {
    printf("==================================================================\n");
    printf("[SDK Example 9]: Temporale Bindung & Phasen-Kopplung (Gating)\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das Phasen-Szenario:\n");
    printf(" [1] Synchrone Phase (Pre: 45 Grad, Post: 50 Grad -> Resonanz & Bindung)\n");
    printf(" [2] Asynchrone Phase (Pre: 45 Grad, Post: 120 Grad -> Destruktives Gating)\n");
    printf("Auswahl (1-2): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_PhaseConfig.scenario = choice;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    pu.ur_grid[10].type_state = TYPE_NEURON;
    pu.ur_grid[20].type_state = TYPE_NEURON;

    pu.reg_source[10].channels[0] = 20ULL;   // Verbindung von 10 nach 20
    pu.reg_source[10].channels[1] = 45ULL;   // Phase des Pre-Neurons: 45 Grad
    pu.reg_source[20].channels[3] = 50ULL;   // Start-Amplitude des Ziel-Neurons

    if (choice == 1) {
        pu.reg_source[20].channels[1] = 50ULL;  // Post-Phase nahe dran (Delta = 5)
    }
    else {
        pu.reg_source[20].channels[1] = 120ULL; // Post-Phase weit weg (Delta = 75)
    }

    printf("\n[START] Verbindung [10->20] geladen.\n");
    printf(" Pre-Phase:   %llu Grad\n", pu.reg_source[10].channels[1]);
    printf(" Post-Phase:  %llu Grad (Toleranzfenster: +/- %llu Grad)\n", pu.reg_source[20].channels[1], (uint64_t)PHASE_TOLERANCE);
    printf(" Start-Amp:   %llu\n", pu.reg_source[20].channels[3]);
    printf("--- Starte Zeitreihen-Simulation (1 Evaluierungs-Tick) ---\n");

    /* Direkte Ausfuehrung des Kernels auf den Zielregistern */
    uint8_t next_state = pu.ur_grid[10].type_state;
    uint8_t next_target_state = pu.ur_grid[20].type_state;

    TemporalBindingRule(
        pu.ur_grid[10].type_state, pu.ur_grid[20].type_state,
        pu.reg_source[10].channels, pu.reg_source[20].channels,
        &next_state, &next_target_state
    );

    pu.ur_grid[10].type_state = next_state;
    pu.ur_grid[20].type_state = next_target_state;

    uint64_t final_amp = pu.reg_source[20].channels[3];
    printf("  Tick #1 -> Ziel-Amplitude: %llu\n", final_amp);

    printf("\n[RESULTAT] ");
    if (final_amp > 50ULL) {
        printf("Kohaerenz erkannt! Signal-Resonanz erzeugt temporaere logische Bindung.\n");
    }
    else {
        printf("Kohaerenz-Gating aktiv! Asynchronitaet hat das Signal destruktiv unterdrueckt.\n");
    }

    ProPhysics_Free(&pu);
    return 0;
}