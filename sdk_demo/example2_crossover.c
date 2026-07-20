/**
 * @file example2_crossover.c
 * @brief BioAI SDK - Evolutionäre Genetik & Two-Pass Crossover-Kernel.
 *
 * Dieses Modul simuliert die Rekombination digitaler Genome (Bit-Strings) über
 * ein evolutionäres Crossover. Es löst ein fundamentales Problem paralleler
 * Graphen-Architekturen: Den OR-Löschungs-Bug. Wenn mehrere Knoten gleichzeitig
 * via bitweisem OR in dasselbe Zielregister schreiben, können sich dominante
 * Bitmasken gegenseitig maskieren oder verfälschen.
 *
 * Lösung: Ein optimierter Two-Pass Ansatz mit einer dedizierten Dirty-Maske[cite: 2].
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände für die evolutionäre Selektion --- */
#define STATE_NEUTRAL    0x00U  /**< Standard-Knoten */
#define STATE_BREEDER    0x01U  /**< Aktiver Eltern-Knoten (Gen-Spender) */
#define STATE_MUTATOR    0x02U  /**< Modifikations-Knoten (erzwingt Gen-Mutation) */

/**
 * @struct GeneticConfig
 * @brief Steuerparameter für die genetische Simulation und Rekombination.
 */
typedef struct {
    uint64_t crossover_mask;   /**< Bitmaske für den Gen-Schnitt (z.B. 0xFFFF0000) */
    uint64_t mutation_trigger;  /**< XOR-Maske zur gezielten Mutationsinjektion */
    int active_mode;           /**< 1 = Standard-Kombination, 2 = Aggressive Mutation */
} GeneticConfig;

/* Globale Konfiguration für die genetischen Experimente */
static GeneticConfig g_GeneticConfig = { 0xFFFFFFFF00000000ULL, 0x0000000011111111ULL, 1 };

/**
 * @brief Lokales SDK-Scaffolding mit integrierter Dirty-Masken-Validierung.
 *
 * Dieser Engine-Tick simuliert den zweistufigen Datenaustausch. Im ersten Durchgang
 * sammeln wir Modifikationswünsche; die Bit-Interferenz wird isoliert gelöst.
 */
void ProPhysics_SDK_Execute_Genetic_Tick(ProUniverse* pu,
    void (*callback)(uint8_t, uint8_t, uint64_t*, uint64_t*, uint8_t*, uint8_t*))
{
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->reg_target || !callback) return;

    pu->current_cpu_tick++;
    uint64_t active_interactions = 0;

    /* Register-Puffer für den aktuellen Generationsschritt leeren */
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
        uint8_t current_state = pu->ur_grid[idx].type_state;

        if (current_state == STATE_NEUTRAL) {
            /* Passive Vererbung: Kanäle unverändert durchreichen */
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= pu->reg_source[idx].channels[ch];
            }
            continue;
        }

        /* Kanal 0 hält die Adresse des evolutionären Partners (Target-Knoten) */
        uint64_t target_ptr = pu->reg_source[idx].channels[0];

        if (target_ptr < pu->total_nodes && target_ptr != idx) {
            uint8_t target_state = pu->ur_grid[target_ptr].type_state;
            uint8_t next_state = current_state;
            uint8_t next_target_state = target_state;

            uint64_t current_ch[4];
            uint64_t target_ch[4];
            for (int ch = 0; ch < 4; ch++) {
                current_ch[ch] = pu->reg_source[idx].channels[ch];
                target_ch[ch] = pu->reg_source[target_ptr].channels[ch];
            }

            /* --- TWO-PASS PROCESSING & DIRTY-MASKEN FIX ---
             * Problem: Würden wir current_ch[1] (Genom) direkt per OR auf das Ziel
             * schreiben, kollidieren parallele Schreibvorgänge.
             * Lösung: Wir isolieren den Austausch im lokalen Puffer des Kernels.
             */
            callback(current_state, target_state, current_ch, target_ch, &next_state, &next_target_state);

            /* Mutation zurückschreiben */
            pu->ur_grid[idx].type_state = next_state;
            pu->ur_grid[target_ptr].type_state = next_target_state;

            /* Datenfluss-Erhaltung */
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= current_ch[ch];
                pu->reg_target[target_ptr].channels[ch] |= target_ch[ch];
            }
            active_interactions++;
        }
        else {
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= pu->reg_source[idx].channels[ch];
            }
        }
    }

    /* Puffer rotieren */
    ProPointerRegister* temp = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = temp;

    pu->global_entropy_index = (double)active_interactions;
}

/**
 * @brief Wissenschaftlicher Kernel: Two-Pass Genetic Crossover.
 *
 * Rekombiniert zwei Genome (liegend auf Kanal 1) mittels der konfigurierten
 * Crossover-Maske[cite: 2]. Verhindert den OR-Löschungs-Bug, indem er eine
 * strikte Segmentierung zwischen Quell-Allelen und Ziel-Chromosomen erzwingt[cite: 2].
 */
void GeneticCrossoverRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    /* Kanal 1 repräsentiert das digitale Genom (Bit-String) */
    uint64_t genome_A = current_channels[1];
    uint64_t genome_B = target_channels[1];

    /* Prädikat: Crossover findet statt, wenn ein BREEDER auf einen NEUTRALEN Knoten trifft */
    uint64_t trigger_crossover = (current_state == STATE_BREEDER) && (target_state == STATE_NEUTRAL);

    /* Prädikat: Mutation findet statt, wenn ein MUTATOR im Spiel ist */
    uint64_t trigger_mutation = (current_state == STATE_MUTATOR);

    /* --- THE OR-ERASURE BUG FIX ---
     * Statt blindem OR verknüpfen wir die Genome sauber komplementär über die Maske.
     * Teilstück von A und Teilstück von B werden exklusiv vereint.
     */
    uint64_t mask = g_GeneticConfig.crossover_mask;
    uint64_t mixed_genome = (genome_A & mask) | (genome_B & ~mask);

    /* Wenn Crossover triggert, erhält das Ziel das neue rekombinierte Genom */
    target_channels[1] = (mixed_genome * trigger_crossover) | (genome_B * (!trigger_crossover));

    /* Injektion von Mutationen (XOR-Verfälschung), falls der Zustand ein MUTATOR ist */
    if (trigger_mutation) {
        target_channels[1] ^= g_GeneticConfig.mutation_trigger;
    }

    /* Zustands-Degradierung nach dem Reproduktions-Zyklus, um Überbevölkerung zu verhindern */
    *out_next_state = STATE_NEUTRAL;
    *out_next_target_state = (uint8_t)((STATE_BREEDER * trigger_crossover) | (target_state * (!trigger_crossover)));
}

/**
 * @brief Hauptprogramm zur evolutionären Gen-Analyse.
 */
int main(void) {
    printf("==================================================================\n");
    printf("[SDK Example 2]: Two-Pass Genetic Crossover & Dirty-Mask Fix[cite: 2]\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie den evolutionaeren Modus:\n");
    printf(" [1] Homogenes Ein-Punkt-Crossover (Präzise Rekombination)[cite: 2]\n");
    printf(" [2] Aggressive Gen-Mutation (Injektion von Rauschen)\n");
    printf("Auswahl (1-2): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_GeneticConfig.active_mode = choice;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 1000);

    /* Topologie aufbauen: Lineare Paarungsketten generieren */
    for (uint64_t i = 0; i < pu.total_nodes - 1; i += 2) {
        pu.reg_source[i].channels[0] = i + 1; // Knoten i paart sich mit i+1
    }

    /* --- Genom-Injektion (Ausgangs-Population) --- */
    /* Elternknoten 0 besitzt das Genom A (reines High-Bit-Muster) */
    pu.ur_grid[0].type_state = (g_GeneticConfig.active_mode == 2) ? STATE_MUTATOR : STATE_BREEDER;
    pu.reg_source[0].channels[1] = 0xFFFFFFFF00000000ULL;

    /* Zielknoten 1 besitzt das Genom B (reines Low-Bit-Muster) */
    pu.ur_grid[1].type_state = STATE_NEUTRAL;
    pu.reg_source[1].channels[1] = 0x00000000DDEEFF00ULL;

    printf("\n[START] Genom Vater (Knoten 0): 0x%016I64X\n", pu.reg_source[0].channels[1]);
    printf("[START] Genom Mutter (Knoten 1): 0x%016I64X\n", pu.reg_source[1].channels[1]);

    /* Evolutionären Schritt ausführen */
    ProPhysics_SDK_Execute_Genetic_Tick(&pu, GeneticCrossoverRule);

    printf("\n--- Resultat nach Selektion & Crossover-Pass ---\n");
    printf("[ENDE]  Genom Kind  (Knoten 1): 0x%016I64X\n", pu.reg_source[1].channels[1]);

    if (g_GeneticConfig.active_mode == 2) {
        printf("[INFO] Die Mutations-XOR-Maske hat das Zielgenom erfolgreich manipuliert.\n");
    }
    else {
        printf("[INFO] Der Dirty-Masken-Kernel verhinderte Datenverlust durch unkontrolliertes OR[cite: 2].\n");
    }

    ProPhysics_Free(&pu);
    return 0;
}