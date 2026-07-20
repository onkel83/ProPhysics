/**
 * @file example6_neuromodulation.c
 * @brief BioAI SDK - Dynamische Neuromodulation & Signal-Gating.
 *
 * Dieses Modul simuliert den Einfluss diffuser neuromodulatorischer Signale
 * (z.B. Dopamin-Bursts) auf die neuronale Signalverarbeitung. Ein Modulator-Knoten
 * beschreibt das Modulationsregister (Kanal 3) eines Zielneurons, wodurch
 * dessen Signal-Gain (Kanal 1) in den Folgeticks branchless skaliert wird.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände --- */
#define STATE_NEUTRAL    0x00U
#define TYPE_NEURON      0x05U
#define TYPE_MODULATOR   0x06U  /**< Diffuser Modulations-Knoten */

/**
 * @struct ModulateConfig
 * @brief Konfigurationsmatrix für den neuromodulatorischen Verstärkungsfaktor.
 */
typedef struct {
    uint64_t amplification_factor; /**< Multiplikator bei aktivem Modulations-Gating */
    int scenario;                  /**< 1 = Dopaminerger Burst, 2 = Basale Transmission, 3 = Gating-Sperre */
} ModulateConfig;

/* Globale Struktur für die Modulations-Szenarien */
static ModulateConfig g_ModulateConfig = { 3ULL, 1 };

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless Neuromodulatory Gating.
 *
 * Berechnet die volumetrische Botenstoffinjektion und steuert die Amplitudenskalierung
 * des neuronalen Durchsatzsignals vollkommen ohne Verzweigungsbefehle.
 */
void NeuromodulationRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    /* 1. Injektions-Pfad: Modulator flutet Kanal 3 des Ziels */
    uint64_t is_modulator = (current_state == TYPE_MODULATOR);
    uint64_t dopamine_level = 50ULL;
    target_channels[3] = (dopamine_level * is_modulator) | (target_channels[3] * (!is_modulator));

    /* 2. Gating-Pfad: Signalverstärkung berechnen */
    uint64_t is_neuron = (current_state == TYPE_NEURON);
    uint64_t has_modulator = (current_channels[3] > 0ULL);
    uint64_t apply_gain = (is_neuron && has_modulator);

    uint64_t base_signal = current_channels[1];
    uint64_t amplified_signal = base_signal * g_ModulateConfig.amplification_factor;

    /* Internes Signal aktualisieren */
    current_channels[1] = (amplified_signal * apply_gain) | (base_signal * (!apply_gain));

    /* 3. PROPAGATION-PFAD (Neu): Signal branchless an das Ziel übergeben */
    /* Nur wenn es ein Neuron ist, wird das (eventuell verstärkte) Signal weitergeleitet */
    target_channels[1] |= (current_channels[1] * is_neuron);
}

/**
 * @brief Globales SDK-Scaffolding für zeitsynchrone Kausal-Ticks.
 */
void ProPhysics_SDK_Execute_Custom_Tick(ProUniverse* pu,
    void (*callback)(uint8_t, uint8_t, uint64_t*, uint64_t*, uint8_t*, uint8_t*))
{
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->reg_target || !callback) return;

    pu->current_cpu_tick++;
    uint64_t active_interactions = 0;

    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
        uint8_t cs = pu->ur_grid[idx].type_state;

        if (cs == STATE_NEUTRAL) {
            for (int c = 0; c < 4; c++) {
                pu->reg_target[idx].channels[c] |= pu->reg_source[idx].channels[c];
            }
            continue;
        }

        uint64_t tp = pu->reg_source[idx].channels[0] & 0x0000FFFFFFFFFFFFULL;

        if (tp < pu->total_nodes && tp != idx) {
            uint64_t c_ch[4], t_ch[4];
            uint8_t ts = pu->ur_grid[tp].type_state;

            for (int c = 0; c < 4; c++) {
                c_ch[c] = pu->reg_source[idx].channels[c];
                t_ch[c] = pu->reg_source[tp].channels[c];
            }

            uint8_t ns = cs, nts = ts;
            callback(cs, ts, c_ch, t_ch, &ns, &nts);

            pu->ur_grid[idx].type_state = ns;
            pu->ur_grid[tp].type_state = nts;

            for (int c = 0; c < 4; c++) {
                pu->reg_target[idx].channels[c] |= c_ch[c];
                pu->reg_target[tp].channels[c] |= t_ch[c];
            }
            active_interactions++;
        }
        else {
            for (int c = 0; c < 4; c++) {
                pu->reg_target[idx].channels[c] |= pu->reg_source[idx].channels[c];
            }
        }
    }

    ProPointerRegister* t = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = t;

    pu->global_entropy_index = (double)active_interactions;
}

int main(void) {
    printf("==================================================================\n");
    printf("[SDK Example 6]: Dynamisches Neuromodulations & Gating System\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das Modulations-Szenario:\n");
    printf(" [1] Dopaminerger Burst   (Modulator flutet Neuron -> Signal verdreifacht)\n");
    printf(" [2] Basale Transmission  (Kein Modulator vorhanden -> Signal bleibt normal)\n");
    printf(" [3] Modulations-Drossel  (Modulator aktiv, aber Faktor auf Minimum gesetzt)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_ModulateConfig.scenario = choice;

    if (choice == 3) {
        g_ModulateConfig.amplification_factor = 1ULL; // Keine Verstärkung trotz Botenstoff
    }

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    /* --- Netzwerktopologie aufbauen ---
     * Knoten 50: MODULATOR -> zielt auf Knoten 60
     * Knoten 60: NEURON    -> zielt auf Knoten 70 (Trägt das Basissignal)
     * Knoten 70: NEURON    -> Postsynaptisches Ziel
     */
    pu.ur_grid[50].type_state = (choice == 2) ? STATE_NEUTRAL : TYPE_MODULATOR;
    pu.ur_grid[60].type_state = TYPE_NEURON;
    pu.ur_grid[70].type_state = TYPE_NEURON;

    pu.reg_source[50].channels[0] = 60ULL; // Modulator steuert Neuron 60 an
    pu.reg_source[60].channels[0] = 70ULL; // Neuron 60 sendet an Knoten 70

    /* Injektion eines elektrischen Basissignals in Knoten 60 (Kanal 1) */
    pu.reg_source[60].channels[1] = 10ULL;

    /* Kanal 3 initial komplett sauber (kein Botenstoff im System) */
    pu.reg_source[60].channels[3] = 0ULL;

    printf("\n[START] Basis-Signal an Knoten [60]: %llu | Modulator-Level: %llu\n",
        pu.reg_source[60].channels[1], pu.reg_source[60].channels[3]);
    printf("--- Starte Zeitreihen-Simulation (3 Ticks) ---\n");

    for (int t = 1; t <= 3; t++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, NeuromodulationRule);

        uint64_t current_sig = pu.reg_source[60].channels[1];
        uint64_t current_mod = pu.reg_source[60].channels[3];
        uint64_t output_rec = pu.reg_source[70].channels[1];

        printf("  Tick #%d -> Neuron [60]: Signal-Staerke = %3llu, Modulator-Pegel = %2llu | Empfaenger [70] Akkumuliert = %3llu\n",
            t, current_sig, current_mod, output_rec);
    }

    ProPhysics_Free(&pu);
    return 0;
}