/**
 * @file example3_hebbian.c
 * @brief BioAI SDK - Neuro-symbolische Hebbian Plasticity & LTD-Dämpfung (Interaktiv).
 *
 * Dieses Modul implementiert die lokale synaptische Plastizität direkt auf den
 * Kausal-Vektoren der Engine. Die synaptische Gewichtung (Stärke der Verbindung)
 * wird im oberen 16-Bit-Segment von Kanal 0 codiert, während die unteren 48 Bit
 * die Zieladresse des Zielneurons (Postsynapse) abbilden.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände der Netzwerkknoten --- */
#define STATE_NEUTRAL 0x00U  /**< Inaktiver Knoten / Ruhezustand */
#define TYPE_NEURON   0x05U  /**< Aktiver neuronaler Knoten (Feuer-Zustand) */

/**
 * @struct HebbianConfig
 * @brief Konfigurationsmatrix für das plastische neuronale Verhalten.
 */
typedef struct {
    int decay_mode;        /**< 1 = Aktiviert LTD (Gewichtszerfall bei Non-Koinzidenz) */
    uint64_t start_weight; /**< Initiales Gewicht für Sättigungstests */
    int scenario;          /**< Gewähltes wissenschaftliches Szenario */
} HebbianConfig;

/* Globale Konfigurationsinstanz für die neuronale Simulation */
static HebbianConfig g_HebbianConfig = { 1, 5ULL, 1 };

/**
 * @brief Hilfsfunktion zur Analyse und Dekodierung des 64-Bit Kausal-Vektors.
 */
static void Print_CausalVector_Analysis(uint64_t raw_vector) {
    uint16_t weight = (uint16_t)(raw_vector >> 48);
    uint64_t target_addr = raw_vector & 0x0000FFFFFFFFFFFFULL;

    printf("0x%016llX | Gewicht: %5u (0x%04X) | Ziel-Knoten ID: %llu\n",
        (unsigned long long)raw_vector,
        (unsigned int)weight,
        (unsigned int)weight,
        (unsigned long long)target_addr);
}

/**
 * @brief Wissenschaftlicher Callback-Kernel: Neuro-symbolische Hebbian Plasticity.
 *
 * Berechnet die Gewichtsveränderung einer Synapse vollkommen ohne CPU-Verzweigungen.
 * Nutzt mathematische Prädikate zur Absicherung gegen Arithmetischen Überlauf.
 */
void HebbianPlasticityRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    (void)target_channels; /* In dieser lokalen Regel nicht benötigt */

    /* Zustände standardmäßig beibehalten (Persistenz der neuronalen Aktivität) */
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    /* Hebbian-Bedingung (Koinzidenz): Beide koppelnden Enden müssen zeitgleich feuern */
    uint64_t both_active = (current_state == TYPE_NEURON) && (target_state == TYPE_NEURON);

    /* Extrahiere das obere 16-Bit Gewicht aus Channel 0 (Bit 48 bis 63) */
    uint64_t current_weight = (current_channels[0] >> 48);

    /* --- BRANCHLESS SATURATION & DECAY ---
     * 1. Berechne potenziellen LTP-Anstieg (Sättigungsgrenze liegt strikt bei 0xFFFF)
     *    Inkrementiert nur, wenn das aktuelle Gewicht kleiner als 0xFFFF ist.
     */
    uint64_t next_weight_ltp = current_weight + (current_weight < 0xFFFFULL);

    /* 2. Berechne potenziellen LTD-Zerfall (Untergrenze liegt strikt bei 0x0000)
     *    Dekrementiert nur, wenn Gewicht > 0 und der globale Decay-Modus aktiv ist.
     */
    uint64_t next_weight_ltd = current_weight - (current_weight > 0ULL && g_HebbianConfig.decay_mode);

    /* 3. Multiplexing der Gewichte basierend auf der koinzidenten Aktivität */
    uint64_t masked_weight = (next_weight_ltp * both_active) | (next_weight_ltd * (!both_active));

    /* Kausal-Adresse im unteren 48-Bit-Raum isolieren (Maske) */
    uint64_t address_mask = 0x0000FFFFFFFFFFFFULL;

    /* Altsignal mit der neuen, nach oben geshifteten Gewichtung bitweise verweben */
    current_channels[0] = (current_channels[0] & address_mask) | (masked_weight << 48);
}

/**
 * @brief Globales SDK-Scaffolding mit 48-Bit Adress-Isolierung.
 *
 * Garantiert, dass Modifikationen am synaptischen Gewicht die darunterliegende
 * topologische Routing-Adresse der Kausalkette niemals kompromittieren.
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

        /* WICHTIG: Nur die reinen unteren 48 Bit für die topologische Verknüpfung verwenden! */
        uint64_t tp = pu->reg_source[idx].channels[0] & 0x0000FFFFFFFFFFFFULL;

        if (tp < pu->total_nodes && tp != idx) {
            uint64_t c_ch[4], t_ch[4];
            uint8_t ts = pu->ur_grid[tp].type_state;

            for (int c = 0; c < 4; c++) {
                c_ch[c] = pu->reg_source[idx].channels[c];
                t_ch[c] = pu->reg_source[tp].channels[c];
            }

            uint8_t ns = cs, nts = ts;

            /* Berechne synaptische Modifikation */
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

    /* Behebung von Warning C4244: Expliziter Cast auf uint32_t */
    pu->global_entropy_index = (uint32_t)active_interactions;
}

/**
 * @brief Setzt die Testnetz-Topologie auf definierte Startwerte zurück.
 */
static void Reset_Simulation(ProUniverse* pu) {
    /* Behebung von Error C2065: Verwende sizeof(*pointer) statt expliziten Typnamen */
    memset(pu->ur_grid, 0, pu->total_nodes * sizeof(*pu->ur_grid));
    memset(pu->reg_source, 0, pu->total_nodes * sizeof(*pu->reg_source));
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(*pu->reg_target));

    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;

    /* Praesasynchrones Neuron #5 feuert */
    pu->ur_grid[5].type_state = TYPE_NEURON;

    /* Postsynaptisches Neuron #15 basierend auf Szenario initialisieren */
    if (g_HebbianConfig.scenario == 2) {
        pu->ur_grid[15].type_state = STATE_NEUTRAL; /* LTD Szenario */
    }
    else {
        pu->ur_grid[15].type_state = TYPE_NEURON;  /* Standard Koinzidenz (LTP) */
    }

    /* Ziel-Adresse 15 im 48-Bit-Raum verankern und Startgewicht injizieren */
    pu->reg_source[5].channels[0] = 15ULL | (g_HebbianConfig.start_weight << 48);
}

/**
 * @brief Konsolen-Visualisierung der plastischen Synapse.
 */
static void Print_Network_State(const ProUniverse* pu) {
    uint64_t ch0 = pu->reg_source[5].channels[0];
    uint16_t weight = (uint16_t)(ch0 >> 48);
    uint64_t target = ch0 & 0x0000FFFFFFFFFFFFULL;

    uint8_t pre_st = pu->ur_grid[5].type_state;
    uint8_t post_st = (target < pu->total_nodes) ? pu->ur_grid[target].type_state : STATE_NEUTRAL;

    printf("\n========================================================================================\n");
    printf(" BIOAI HEBBIAN CORE | Tick #%-3llu | Decay Mode (LTD): %s\n",
        (unsigned long long)pu->current_cpu_tick,
        g_HebbianConfig.decay_mode ? "AKTIV" : "INAKTIV");
    printf("========================================================================================\n");
    printf(" Prae-Neuron  [#5] : Status = %-18s (0x%02X)\n",
        (pre_st == TYPE_NEURON) ? "FEUERT [TYPE_NEURON]" : "INAKTIV [STATE_NEUTRAL]", pre_st);
    printf(" Post-Neuron [#%llu] : Status = %-18s (0x%02X)\n",
        (unsigned long long)target,
        (post_st == TYPE_NEURON) ? "FEUERT [TYPE_NEURON]" : "INAKTIV [STATE_NEUTRAL]", post_st);
    printf("----------------------------------------------------------------------------------------\n");
    printf(" Kausal-Vektor (Ch 0) : ");
    Print_CausalVector_Analysis(ch0);
    printf(" Synaptische Staerke  : %u / 65535 (0xFFFF)\n", weight);
    printf("========================================================================================\n\n");
}

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 500);

    Reset_Simulation(&pu);

    char choice = 0;
    while (true) {
        Print_Network_State(&pu);

        printf("--- INTERAKTIVE NEURO-PLASTIZITAETS CONSOLE ---\n");
        printf(" [1] 1 Tick ausfuehren (Hebbian Pass)\n");
        printf(" [2] 5 Ticks in Folge ausfuehren\n");
        printf(" [3] Post-Neuron [#15] Zustand umschalten (Feuern vs. Ruhe)\n");
        printf(" [4] LTD-Gewichtszerfall (Decay Mode) umschalten\n");
        printf(" [5] Preset: LTP Koinzidenz-Lernen (Beide aktiv)\n");
        printf(" [6] Preset: LTD Zerfall (Postsynapse inaktiv, Decay On)\n");
        printf(" [7] Preset: Saettigungsschutz (Startgewicht 0xFFFE)\n");
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
            ProPhysics_SDK_Execute_Custom_Tick(&pu, HebbianPlasticityRule);
            printf("\n>> Tick #%llu verarbeitet.\n", (unsigned long long)pu.current_cpu_tick);
            break;

        case '2':
            for (int i = 0; i < 5; i++) {
                ProPhysics_SDK_Execute_Custom_Tick(&pu, HebbianPlasticityRule);
            }
            printf("\n>> 5 Ticks vollzogen.\n");
            break;

        case '3':
            if (pu.ur_grid[15].type_state == TYPE_NEURON) {
                pu.ur_grid[15].type_state = STATE_NEUTRAL;
                printf("\n[OK] Post-Neuron #15 ist nun RUHEND (0x00).\n");
            }
            else {
                pu.ur_grid[15].type_state = TYPE_NEURON;
                printf("\n[OK] Post-Neuron #15 ist nun AKTIV (0x05).\n");
            }
            break;

        case '4':
            g_HebbianConfig.decay_mode = !g_HebbianConfig.decay_mode;
            printf("\n[OK] LTD Decay Mode ist jetzt %s.\n",
                g_HebbianConfig.decay_mode ? "AKTIV" : "INAKTIV");
            break;

        case '5':
            g_HebbianConfig.scenario = 1;
            g_HebbianConfig.decay_mode = 0;
            g_HebbianConfig.start_weight = 0ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] LTP Koinzidenz geladen.\n");
            break;

        case '6':
            g_HebbianConfig.scenario = 2;
            g_HebbianConfig.decay_mode = 1;
            g_HebbianConfig.start_weight = 10ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] LTD Zerfall geladen.\n");
            break;

        case '7':
            g_HebbianConfig.scenario = 3;
            g_HebbianConfig.decay_mode = 0;
            g_HebbianConfig.start_weight = 0xFFFEULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Saettigungstest geladen.\n");
            break;

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