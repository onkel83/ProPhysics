/**
 * @file example4_pruning.c
 * @brief BioAI SDK - Strukturelles Synaptisches Pruning & Topologie-Bereinigung.
 *
 * Dieses Modul simuliert den biologischen Pruning-Prozess (Synapsen-Eliminierung).
 * Fällt das synaptische Gewicht (obere 16 Bit von Kanal 0) unter eine kritische
 * wissenschaftliche Schwelle, wird der gesamte Kausal-Vektor (die Zieladresse)
 * branchless genullt. Die Verbindung gilt damit als physisch getrennt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände --- */
#define STATE_NEUTRAL 0x00U
#define TYPE_NEURON   0x05U

/**
 * @struct PruningConfig
 * @brief Experimentelle Parameter für den strukturellen Netzwerkabbau.
 */
typedef struct {
    uint64_t pruning_threshold; /**< Gewichts-Untergrenze, unter der gekappt wird */
    uint64_t metabolic_decay;   /**< Abzugs-Koeffizient pro Zeitschritt (Metabolischer Druck) */
    int scenario;               /**< 1 = Sanfter Abbau, 2 = Schock-Pruning, 3 = Protektion durch Aktivität */
} PruningConfig;

/* Globale Struktur für die Pruning-Szenarien */
static PruningConfig g_PruningConfig = { 10ULL, 2ULL, 1 };

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

    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

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

    pu->global_entropy_index = (double)active_interactions;
}

int main(void) {
    printf("==================================================================\n");
    printf("[SDK Example 4]: Strukturelles Synaptisches Pruning-System\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das Pruning-Szenario:\n");
    printf(" [1] Sanfter Zerfall (Kappung unterschreitet Schwelle 10)\n");
    printf(" [2] Schock-Pruning  (Aggressiver Abbau, hohe Degradierung)\n");
    printf(" [3] Funktionale Protektion (Gewicht hoch genug, Pfad bleibt stabil)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_PruningConfig.scenario = choice;

    /* Szenarienspezifische Injektions-Werte definieren */
    uint64_t initial_weight = 15ULL; // Startgewicht
    if (choice == 2) {
        g_PruningConfig.metabolic_decay = 12ULL;   // Riesiger Verlust pro Tick
        g_PruningConfig.pruning_threshold = 5ULL;
    }
    else if (choice == 3) {
        initial_weight = 50ULL; // Sehr stark verankerte Verbindung
        g_PruningConfig.metabolic_decay = 2ULL;
        g_PruningConfig.pruning_threshold = 5ULL;
    }

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    /* Aktivierung der beteiligten neuronalen Einheiten */
    pu.ur_grid[10].type_state = TYPE_NEURON;
    pu.ur_grid[20].type_state = TYPE_NEURON;

    /* Kausal-Verknuepfung von Knoten 10 auf Knoten 20 mit Startgewicht */
    pu.reg_source[10].channels[0] = 20ULL | (initial_weight << 48);

    printf("\n[START] Synapse [10->20] geladen. Init-Staerke: %llu, Schwelle: %llu\n",
        initial_weight, g_PruningConfig.pruning_threshold);
    printf("--- Starte Zeitreihen-Simulation (4 Ticks) ---\n");

    for (int t = 1; t <= 4; t++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, SynapticPruningRule);

        uint64_t current_weight = (pu.reg_source[10].channels[0] >> 48);
        uint64_t target_address = (pu.reg_source[10].channels[0] & 0x0000FFFFFFFFFFFFULL);

        if (target_address == 0) {
            printf("  Tick #%d -> [GEPRUNT] Verbindung wurde irreversibel geloescht!\n", t);
        }
        else {
            printf("  Tick #%d -> Synaptische Staerke: %2llu | Ziel-Knoten: %2llu\n",
                t, current_weight, target_address);
        }
    }

    ProPhysics_Free(&pu);
    return 0;
}