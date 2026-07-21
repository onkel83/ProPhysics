/**
 * @file example4_pruning.c
 * @brief BioAI SDK - Strukturelles Synaptisches Pruning & Topologie-Bereinigung (Interaktiv).
 *
 * Dieses Modul simuliert den biologischen Pruning-Prozess (Synapsen-Eliminierung).
 * Fällt das synaptische Gewicht (obere 16 Bit von Kanal 0) unter eine kritische
 * wissenschaftliche Schwelle, wird der gesamte Kausal-Vektor (die Zieladresse)
 * branchless genullt. Die Verbindung gilt damit als physisch getrennt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände der Netzwerkknoten --- */
#define STATE_NEUTRAL 0x00U  /**< Inaktiver Knoten / Ruhezustand */
#define TYPE_NEURON   0x05U  /**< Aktiver neuronaler Knoten */

/**
 * @struct PruningConfig
 * @brief Experimentelle Parameter für den strukturellen Netzwerkabbau.
 */
typedef struct {
    uint64_t pruning_threshold; /**< Gewichts-Untergrenze, unter der gekappt wird */
    uint64_t metabolic_decay;   /**< Abzugs-Koeffizient pro Zeitschritt (Metabolischer Druck) */
    uint64_t initial_weight;    /**< Startgewicht für Rekonstruktion */
    int scenario;                /**< Aktuelles Pruning-Szenario */
} PruningConfig;

/* Globale Konfiguration für die Pruning-Simulation */
static PruningConfig g_PruningConfig = { 10ULL, 2ULL, 15ULL, 1 };

/**
 * @brief Hilfsfunktion zur Analyse und Dekodierung des 64-Bit Kausal-Vektors.
 */
static void Print_CausalVector_Analysis(uint64_t raw_vector) {
    uint16_t weight = (uint16_t)(raw_vector >> 48);
    uint64_t target_addr = raw_vector & 0x0000FFFFFFFFFFFFULL;

    if (target_addr == 0) {
        printf("0x0000000000000000 | [GEPRUNT / PHYSISCH GETRENNT]\n");
    }
    else {
        printf("0x%016llX | Gewicht: %5u (0x%04X) | Ziel-Knoten ID: %llu\n",
            (unsigned long long)raw_vector,
            (unsigned int)weight,
            (unsigned int)weight,
            (unsigned long long)target_addr);
    }
}

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless Synaptic Pruning.
 *
 * Berechnet den metabolischen Gewichtszerfall und isoliert die Trennung der
 * Kausalkette mathematisch über Bitmasken, um CPU-Branch-Stalls zu verhindern.
 */
void SynapticPruningRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    (void)target_channels;
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    /* Extrahiere aktuelles Gewicht (obere 16 Bit) und die Routing-Adresse (untere 48 Bit) */
    uint64_t current_weight = (current_channels[0] >> 48);
    uint64_t target_address = (current_channels[0] & 0x0000FFFFFFFFFFFFULL);

    /* --- METABOLISCHER ZERFALL ---
     * Das Gewicht sinkt kontinuierlich durch den konfigurierten Druck,
     * fängt sich jedoch branchless bei 0 ab (Unterlauf-Schutz).
     */
    uint64_t decay = g_PruningConfig.metabolic_decay;
    uint64_t next_weight = current_weight - (decay * (current_weight >= decay));

    /* --- PRUNING EVALUATION (BRANCHLESS) ---
     * Pruning-Bedingung: Verbindung wird gekappt, wenn das neue Gewicht
     * STRENG UNTER dem Schwellenwert liegt.
     */
    uint64_t keep_connection = (next_weight >= g_PruningConfig.pruning_threshold);

    /* Masken-Generierung:
     * Wenn Verbindung gehalten wird -> volles Bitmuster (0xFFFFFFFFFFFFFFFF)
     * Wenn gekappt wird -> Null-Maske (0x0000000000000000)
     */
    uint64_t pruning_mask = 0ULL - keep_connection;

    /* Gewicht oben aufsetzen; falls gekappt, wird durch das Teilstück alles genullt */
    uint64_t final_composite = (target_address & pruning_mask) | ((next_weight & pruning_mask) << 48);

    /* Aktualisierten Kausal-Vektor zurückschreiben */
    current_channels[0] = final_composite;
}

/**
 * @brief Globales SDK-Scaffolding für deterministische Topologie-Ticks.
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

        /* Adressauflösung über die unteren 48 Bit */
        uint64_t tp = pu->reg_source[idx].channels[0] & 0x0000FFFFFFFFFFFFULL;

        /* Ein Kausal-Vektor mit Adresse 0 gilt als getrennt/geprunt */
        if (tp < pu->total_nodes && tp != idx && tp != 0) {
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
            /* Passiver Durchreich-Pfad für isolierte Knoten */
            for (int c = 0; c < 4; c++) {
                pu->reg_target[idx].channels[c] |= pu->reg_source[idx].channels[c];
            }
        }
    }

    ProPointerRegister* t = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = t;

    /* Behebung von Warning C4244: Expliziter Cast auf uint32_t */
    pu->global_entropy_index = (uint32_t)active_interactions;
}

/**
 * @brief Setzt die Testnetz-Topologie auf definierte Startwerte zurück.
 */
static void Reset_Simulation(ProUniverse* pu) {
    memset(pu->ur_grid, 0, pu->total_nodes * sizeof(*pu->ur_grid));
    memset(pu->reg_source, 0, pu->total_nodes * sizeof(*pu->reg_source));
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(*pu->reg_target));

    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;

    /* Aktivierung der beteiligten neuronalen Einheiten (#10 und #20) */
    pu->ur_grid[10].type_state = TYPE_NEURON;
    pu->ur_grid[20].type_state = TYPE_NEURON;

    /* Kausal-Verknüpfung von Knoten 10 auf Knoten 20 mit Startgewicht */
    pu->reg_source[10].channels[0] = 20ULL | (g_PruningConfig.initial_weight << 48);
}

/**
 * @brief Konsolen-Visualisierung der Synapse und des Pruning-Zustands.
 */
static void Print_Network_State(const ProUniverse* pu) {
    uint64_t ch0 = pu->reg_source[10].channels[0];
    uint16_t weight = (uint16_t)(ch0 >> 48);
    uint64_t target = ch0 & 0x0000FFFFFFFFFFFFULL;

    printf("\n========================================================================================\n");
    printf(" BIOAI PRUNING CORE | Tick #%-3llu | Decay Rate: %llu | Threshold: %llu\n",
        (unsigned long long)pu->current_cpu_tick,
        (unsigned long long)g_PruningConfig.metabolic_decay,
        (unsigned long long)g_PruningConfig.pruning_threshold);
    printf("========================================================================================\n");

    /* Topologie-Graph im ASCII-Format */
    printf(" TOPOLOGIE:  [Knoten #10] ");
    if (target == 0) {
        printf(" --x-- ( GEPRUNT / ELEMINIERT ) --x-- ");
    }
    else if (weight < g_PruningConfig.pruning_threshold + 5) {
        printf(" --?-- (DEGRADIERT: %2u) --------> ", weight);
    }
    else {
        printf(" ===== (STABIL: %2u) ===========> ", weight);
    }
    printf("[Knoten #20]\n");

    printf("----------------------------------------------------------------------------------------\n");
    printf(" Kausal-Vektor (Ch 0) : ");
    Print_CausalVector_Analysis(ch0);
    printf(" Synaptischer Status  : %s\n",
        (target == 0) ? "INAKTIV (Null-Maske angewendet)" : "STABIL / VERBUNDEN");
    printf("========================================================================================\n\n");
}

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    Reset_Simulation(&pu);

    char choice = 0;
    while (true) {
        Print_Network_State(&pu);

        printf("--- INTERAKTIVE SYNAPTIC PRUNING CONSOLE ---\n");
        printf(" [1] 1 Tick ausfuehren (Metabolischen Zerfall berechnen)\n");
        printf(" [2] 5 Ticks in Folge ausfuehren\n");
        printf(" [3] Preset: Sanfter Zerfall (Init: 15, Decay: 2, Threshold: 10)\n");
        printf(" [4] Preset: Schock-Pruning   (Init: 15, Decay: 12, Threshold: 5)\n");
        printf(" [5] Preset: Protektion       (Init: 50, Decay: 2, Threshold: 5)\n");
        printf(" [6] Parameter manuell anpassen (Decay / Threshold)\n");
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
            ProPhysics_SDK_Execute_Custom_Tick(&pu, SynapticPruningRule);
            printf("\n>> Tick #%llu verarbeitet.\n", (unsigned long long)pu.current_cpu_tick);
            break;

        case '2':
            for (int i = 0; i < 5; i++) {
                ProPhysics_SDK_Execute_Custom_Tick(&pu, SynapticPruningRule);
            }
            printf("\n>> 5 Ticks vollzogen.\n");
            break;

        case '3':
            g_PruningConfig.scenario = 1;
            g_PruningConfig.initial_weight = 15ULL;
            g_PruningConfig.metabolic_decay = 2ULL;
            g_PruningConfig.pruning_threshold = 10ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Sanfter Zerfall geladen.\n");
            break;

        case '4':
            g_PruningConfig.scenario = 2;
            g_PruningConfig.initial_weight = 15ULL;
            g_PruningConfig.metabolic_decay = 12ULL;
            g_PruningConfig.pruning_threshold = 5ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Schock-Pruning geladen.\n");
            break;

        case '5':
            g_PruningConfig.scenario = 3;
            g_PruningConfig.initial_weight = 50ULL;
            g_PruningConfig.metabolic_decay = 2ULL;
            g_PruningConfig.pruning_threshold = 5ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Funktionale Protektion geladen.\n");
            break;

        case '6': {
            uint64_t d = 0, t = 0;
            printf("\nNeuer Decay-Koeffizient > ");
            if (scanf("%llu", &d) == 1) g_PruningConfig.metabolic_decay = d;
            printf("Neuer Pruning-Schwellenwert > ");
            if (scanf("%llu", &t) == 1) g_PruningConfig.pruning_threshold = t;
            printf("\n[OK] Parameter aktualisiert.\n");
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