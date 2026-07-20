/**
 * @file example3_hebbian.c
 * @brief BioAI SDK - Neuro-symbolische Hebbian Plasticity & LTD-Dämpfung.
 *
 * Dieses Modul implementiert die lokale synaptische Plastizität direkt auf den
 * Kausal-Vektoren der Engine. Die synaptische Gewichtung (Stärke der Verbindung)
 * wird im oberen 16-Bit-Segment von Kanal 0 codiert, während die unteren 48 Bit
 * die Zieladresse des Zielneurons (Postsynapse) abbilden.
 *
 * Neuerungen: Interaktive Szenarien zur Simulation von LTP (Langzeit-Potenzierung),
 * LTD (Langzeit-Depression/Zerfall) und deterministischen Überlauf-Schutzmechanismen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
static HebbianConfig g_HebbianConfig = { 0, 0ULL, 1 };

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

    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

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

    pu->global_entropy_index = (double)active_interactions;
}

/**
 * @brief Hauptprogramm zur Verifikation neuro-symbolischer Lernprozesse.
 */
int main(void) {
    printf("==================================================================\n");
    printf("[SDK Example 3]: Neuro-symbolische Hebbian Plasticity Engine\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das neuronale Lern-Szenario:\n");
    printf(" [1] Klassische LTP (Koinzidenz-Lernen: Beide Neuronen aktiv)\n");
    printf(" [2] Synaptischer LTD-Zerfall (Asynchron: Postsynapse schlaeft)\n");
    printf(" [3] Strikter Saettigungs-Schutz (Branchless Overflow-Check)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_HebbianConfig.scenario = choice;

    /* Standardparameter je nach Szenario setzen */
    if (choice == 2) {
        g_HebbianConfig.decay_mode = 1;       /* Schalte LTD-Gewichtszerfall frei */
        g_HebbianConfig.start_weight = 5ULL;  /* Starte erhöht, um Dämpfung zu sehen */
    }
    else if (choice == 3) {
        g_HebbianConfig.start_weight = 0xFFFEULL; /* Knapp unter 16-Bit-Maximum (65534) */
    }

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 500);

    /* --- Topologie-Setup & Zustandsinjektion --- */
    /* Initiiere das präsynaptische Neuron auf Index 5 */
    pu.ur_grid[5].type_state = TYPE_NEURON;

    /* Zustand des postsynaptischen Neurons auf Index 15 basierend auf Szenario bestimmen */
    if (choice == 1 || choice == 3) {
        pu.ur_grid[15].type_state = TYPE_NEURON;   /* Aktivierte Koinzidenz */
    }
    else {
        pu.ur_grid[15].type_state = STATE_NEUTRAL; /* Postsynapse schläft -> führt zu LTD */
    }

    /* Ziel-Adresse 15 im 48-Bit-Raum verankern und konfiguriertes Startgewicht injizieren */
    pu.reg_source[5].channels[0] = 15ULL | (g_HebbianConfig.start_weight << 48);

    uint64_t init_w = (pu.reg_source[5].channels[0] >> 48);
    printf("\n[START] Initialwert der Synapse [5->15]: %u (0x%04X)\n", (uint32_t)init_w, (uint32_t)init_w);
    printf("--- Starte Zeitreihen-Simulation (5 Ticks) ---\n");

    for (int t = 1; t <= 5; t++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, HebbianPlasticityRule);

        uint64_t current_weight = (pu.reg_source[5].channels[0] >> 48);
        uint64_t target_address = (pu.reg_source[5].channels[0] & 0x0000FFFFFFFFFFFFULL);

        printf("  Tick #%d -> Synaptische Staerke: %5u | Ziel-Knoten: %2llu\n",
            t, (uint32_t)current_weight, (unsigned long long)target_address);
    }

    /* Wissenschaftliche End-Auswertung im Log */
    uint64_t final_w = (pu.reg_source[5].channels[0] >> 48);
    printf("\n[RESULTAT] ");
    if (choice == 1) {
        printf("LTP erfolgreich. Die synaptische Kopplung wurde deterministisch verstaerkt.\n");
    }
    else if (choice == 2) {
        printf("LTD erfolgreich. Verbindung wurde aufgrund mangelnder Koinzidenz abgewertet.\n");
    }
    else if (choice == 3) {
        if (final_w == 0xFFFFULL) {
            printf("Ueberlauf-Schutz intakt. Das Gewicht wurde präzise bei 65535 (0xFFFF) gedeckelt.\n");
        }
        else {
            printf("WARNUNG: Bit-Kollision oder unerwarteter Register-Ueberlauf detektiert!\n");
        }
    }

    ProPhysics_Free(&pu);
    return 0;
}