/**
 * @file example6_neuromodulation.c
 * @brief BioAI SDK - Dynamische Neuromodulation & Signal-Gating (Interaktiv).
 *
 * Simuliert den Einfluss diffuser neuromodulatorischer Signale (z. B. Dopamin-Bursts)
 * auf die neuronale Signalverarbeitung. Ein Modulator-Knoten flutet das
 * Modulationsregister (Kanal 3) eines Zielneurons, wodurch dessen Signal-Gain
 * (Kanal 1) in den Folgeticks branchless skaliert und weitergeleitet wird.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände der Netzwerkknoten --- */
#define STATE_NEUTRAL   0x00U  /**< Inaktiver Knoten / Ruhezustand */
#define TYPE_NEURON     0x05U  /**< Aktiver neuronaler Signalverarbeitungsknoten */
#define TYPE_MODULATOR  0x06U  /**< Diffuser Modulations-Knoten (z. B. Dopaminerger Burst) */

/**
 * @struct ModulateConfig
 * @brief Steuermatrix für den neuromodulatorischen Verstärkungsfaktor.
 */
typedef struct {
    uint64_t amplification_factor; /**< Multiplikator bei aktivem Modulations-Gating */
    int scenario;                  /**< Aktuelles Versuchsszenario */
} ModulateConfig;

/* Globale Konfiguration für die Neuromodulations-Experimente */
static ModulateConfig g_ModulateConfig = { 3ULL, 1 };

/**
 * @brief Hilfsfunktion zur Erstellung eines ASCII-Balkendiagramms für den Modulator-Pegel [0..100].
 */
static void Print_Modulator_Balken(uint64_t level) {
    const int max_bars = 20;
    int filled = (int)((level * max_bars) / 100ULL);
    if (filled > max_bars) filled = max_bars;

    printf("[");
    for (int i = 0; i < max_bars; i++) {
        if (i < filled) printf("#");
        else printf(".");
    }
    printf("]");
}

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

    /* 1. Injektions-Pfad: Modulator flutet Kanal 3 des Ziels mit Dopamin */
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

    /* 3. PROPAGATION-PFAD: Signal branchless an das Ziel übergeben */
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

    memset(pu->reg_target, 0, pu->total_nodes * sizeof(*pu->reg_target));

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

    pu->global_entropy_index = (uint32_t)active_interactions;
}

/**
 * @brief Setzt die Testnetz-Topologie auf den gewählten Modulationszustand zurück.
 */
static void Reset_Simulation(ProUniverse* pu) {
    memset(pu->ur_grid, 0, pu->total_nodes * sizeof(*pu->ur_grid));
    memset(pu->reg_source, 0, pu->total_nodes * sizeof(*pu->reg_source));
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(*pu->reg_target));

    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;

    pu->ur_grid[50].type_state = (g_ModulateConfig.scenario == 2) ? STATE_NEUTRAL : TYPE_MODULATOR;
    pu->ur_grid[60].type_state = TYPE_NEURON;
    pu->ur_grid[70].type_state = TYPE_NEURON;

    pu->reg_source[50].channels[0] = 60ULL;
    pu->reg_source[60].channels[0] = 70ULL;

    pu->reg_source[60].channels[1] = 10ULL;
    pu->reg_source[60].channels[3] = 0ULL;
    pu->reg_source[70].channels[1] = 0ULL;
}

/**
 * @brief Konsolen-Visualisierung der Signalstärken und Modulationspegel.
 */
static void Print_Network_State(const ProUniverse* pu) {
    uint64_t sig_60 = pu->reg_source[60].channels[1];
    uint64_t mod_60 = pu->reg_source[60].channels[3];
    uint64_t rec_70 = pu->reg_source[70].channels[1];

    printf("\n========================================================================================\n");
    printf(" BIOAI NEUROMODULATION CORE | Tick #%-3llu | Amplification Factor: %llux\n",
        (unsigned long long)pu->current_cpu_tick,
        (unsigned long long)g_ModulateConfig.amplification_factor);
    printf("========================================================================================\n");

    printf(" KNOTEN #50 [Modulator]   : State = %s\n",
        (pu->ur_grid[50].type_state == TYPE_MODULATOR) ? "TYPE_MODULATOR (Aktiv)" : "STATE_NEUTRAL (Inaktiv)");

    printf(" KNOTEN #60 [Neuron Presyn]: Signal (Ch 1) = %3llu  | Botenstoff (Ch 3) : %3llu | ",
        (unsigned long long)sig_60, (unsigned long long)mod_60);
    Print_Modulator_Balken(mod_60);
    printf("\n");

    printf(" KNOTEN #70 [Postsynapse]  : Signal-Akkumulation (Ch 1) = %3llu\n",
        (unsigned long long)rec_70);

    printf(" GATING ZUSTAND            : ");
    if (mod_60 > 0) {
        printf("[ MODULATED / AMPLIFIED (Signal x %llu) ]\n",
            (unsigned long long)g_ModulateConfig.amplification_factor);
    }
    else {
        printf("[ BASAL TRANSMISSION (Standard-Durchsatz) ]\n");
    }
    printf("========================================================================================\n\n");
}

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    Reset_Simulation(&pu);

    char choice = 0;
    while (true) {
        Print_Network_State(&pu);

        printf("--- INTERAKTIVE NEUROMODULATION CONSOLE ---\n");
        printf(" [1] 1 Kausal-Tick ausfuehren\n");
        printf(" [2] 3 Ticks in Folge ausfuehren\n");
        printf(" [3] Preset: Dopaminerger Burst    (Modulator aktiv, Amplifikation: 3x)\n");
        printf(" [4] Preset: Basale Transmission   (Modulator inaktiv, Standard-Durchsatz)\n");
        printf(" [5] Preset: Modulations-Drossel   (Modulator aktiv, Amplifikation: 1x)\n");
        printf(" [6] Manueller Dopamin-Burst (Flute Kanal 3 auf Knoten 60)\n");
        printf(" [7] Basissignal an Knoten 60 anpassen\n");
        printf(" [8] Verstaerkungsfaktor (Gain) aendern\n");
        printf(" [r] Simulation zuruecksetzen\n");
        printf(" [q] Beenden\n");
        printf(" Auswahl > ");

        if (scanf(" %c", &choice) != 1) break;

        if (choice == 'q' || choice == 'Q') {
            printf("\nSimulation beendet.\n");
            break;
        }

        switch (choice) {
        case '1':
            ProPhysics_SDK_Execute_Custom_Tick(&pu, NeuromodulationRule);
            printf("\n>> Tick #%llu verarbeitet.\n", (unsigned long long)pu.current_cpu_tick);
            break;

        case '2':
            for (int i = 0; i < 3; i++) {
                ProPhysics_SDK_Execute_Custom_Tick(&pu, NeuromodulationRule);
            }
            printf("\n>> 3 Ticks vollzogen.\n");
            break;

        case '3':
            g_ModulateConfig.scenario = 1;
            g_ModulateConfig.amplification_factor = 3ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Dopaminerger Burst geladen.\n");
            break;

        case '4':
            g_ModulateConfig.scenario = 2;
            g_ModulateConfig.amplification_factor = 3ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Basale Transmission geladen.\n");
            break;

        case '5':
            g_ModulateConfig.scenario = 3;
            g_ModulateConfig.amplification_factor = 1ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Modulations-Drossel geladen.\n");
            break;

        case '6':
            pu.reg_source[60].channels[3] = 80ULL;
            printf("\n[OK] Manuelle Dopamin-Injektion (80%%) auf Knoten #60 angewendet.\n");
            break;

        case '7': {
            uint64_t val = 0;
            printf("\nNeues Basissignal fuer Knoten #60 eingeben > ");
            if (scanf("%llu", &val) == 1) {
                pu.reg_source[60].channels[1] = val;
                printf("[OK] Basissignal auf %llu gesetzt.\n", (unsigned long long)val);
            }
            break;
        }

        case '8': {
            uint64_t gain = 0;
            printf("\nNeuen Verstaerkungsfaktor (Amplification Factor) eingeben > ");
            if (scanf("%llu", &gain) == 1) {
                g_ModulateConfig.amplification_factor = gain;
                printf("[OK] Gain-Multiplikator auf %llux gesetzt.\n", (unsigned long long)gain);
            }
            break;
        }

        case 'r':
        case 'R':
            Reset_Simulation(&pu);
            printf("\n[OK] Simulation auf Initialzustand zurueckgesetzt.\n");
            break;

        default:
            printf("\n[!] Ungueltige Eingabe.\n");
            break;
        }
    }

    ProPhysics_Free(&pu);
    return 0;
}