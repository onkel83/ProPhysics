/**
 * @file example7_stdp.c
 * @brief BioAI SDK - Kausales Spike-Timing-Dependent Plasticity (STDP) System.
 *
 * Dieses Modul simuliert die zeitabhängige synaptische Plastizität. Die Stärke
 * einer Verbindung (Kanal 3) skaliert basierend auf der prä- und postsynaptischen
 * Millisekunden-Differenz (Kanal 1) vollkommen ohne Verzweigungsbefehle.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände --- */
#define STATE_NEUTRAL   0x00U
#define TYPE_NEURON     0x05U

/**
 * @struct STDPConfig
 * @brief Konfigurationsmatrix für das zeitabhängige Lernfenster.
 */
typedef struct {
    uint64_t learning_window; /**< Maximales zeitliches Delta für Plastizität */
    uint64_t delta_weight;     /**< Modifikations-Schrittweite (LTP / LTD) */
    int scenario;              /**< 1 = Kausales Feuern (LTP), 2 = Anti-kausales Feuern (LTD), 3 = Unkorreliert */
} STDPConfig;

/* Globale Konfiguration für das STDP-Lernverhalten */
static STDPConfig g_STDPConfig = { 5ULL, 4ULL, 1 };

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless STDP Engine.
 *
 * Berechnet das zeitliche Intervall zwischen prä- und postsynaptischen Impulsen
 * und mutiert das strukturelle Gewicht rein mathematisch über Prädikatenmasken.
 */
void STDPPlasticityRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    /* Nur aktiv, wenn beide Interaktionspartner valide Funktionseinheiten sind */
    uint64_t valid_interaction = (current_state == TYPE_NEURON) && (target_state == TYPE_NEURON);

    /* Kanal 1: Letzter dokumentierter Spike-Zeitstempel (CPU-Tick)
     * Kanal 3: Aktuelle Synapsenstärke (Gewichts-Ressource) */
    uint64_t pre_tick = current_channels[1];
    uint64_t post_tick = target_channels[1];
    uint64_t weight = current_channels[3];

    uint64_t window = g_STDPConfig.learning_window;
    uint64_t delta = g_STDPConfig.delta_weight;

    /* --- TIMING ANALYSEN (BRANCHLESS) --- */
    /* Prädikat A: Kausales Timing (Pre vor Post) -> Long-Term Potentiation (LTP) */
    uint64_t is_ltp = valid_interaction && (post_tick > pre_tick) && ((post_tick - pre_tick) <= window);

    /* Prädikat B: Anti-Kausales Timing (Post vor Pre) -> Long-Term Depression (LTD) */
    uint64_t is_ltd = valid_interaction && (pre_tick > post_tick) && ((pre_tick - post_tick) <= window);

    /* Gewichtsanpassung berechnen unter Beachtung der biologischen Grenzen (Safe-Guards) */
    uint64_t potentiated = weight + delta;
    /* Obergrenze bei 100 deckeln */
    potentiated = (100ULL * (potentiated > 100ULL)) | (potentiated * (potentiated <= 100ULL));

    uint64_t depressed = weight - (delta * (weight >= delta));

    /* Multiplexing des Zielgewichts ohne Sprungbefehle */
    uint64_t next_weight = weight;
    next_weight = (potentiated * is_ltp) | (next_weight * (!is_ltp));
    next_weight = (depressed * is_ltd) | (next_weight * (!is_ltd));

    /* Synaptisches Gewicht im Quellkanal sichern */
    current_channels[3] = next_weight;
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
    printf("[SDK Example 7]: Kausales STDP (Spike-Timing) Plastizitaets-System\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das STDP-Lernszenario:\n");
    printf(" [1] Kausales Feuern      (Pre-Spike bei Tick 10, Post-Spike bei Tick 12 -> LTP)\n");
    printf(" [2] Anti-kausales Feuern (Post-Spike bei Tick 10, Pre-Spike bei Tick 13 -> LTD)\n");
    printf(" [3] Asynchrones Rauschen (Zeitdifferenz ausserhalb des Lernfensters)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_STDPConfig.scenario = choice;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    /* Aufbau der prä- zu postsynaptischen Testachse */
    pu.ur_grid[15].type_state = TYPE_NEURON; // Prä-synaptischer Knoten
    pu.ur_grid[25].type_state = TYPE_NEURON; // Post-synaptischer Knoten

    pu.reg_source[15].channels[0] = 25ULL;   // Topologie verknüpfen

    /* Startgewicht der Synapse definieren */
    pu.reg_source[15].channels[3] = 40ULL;

    /* Zeitstempel-Injektion basierend auf Szenariowahl */
    if (choice == 1) {
        pu.reg_source[15].channels[1] = 10ULL; // Pre-Spike zuerst
        pu.reg_source[25].channels[1] = 12ULL; // Post-Spike folgt prompt (+2)
    }
    else if (choice == 2) {
        pu.reg_source[15].channels[1] = 13ULL; // Pre-Spike kommt zu spät
        pu.reg_source[25].channels[1] = 10ULL; // Post-Spike geschah zuerst (-3)
    }
    else {
        pu.reg_source[15].channels[1] = 5ULL;  // Weit außerhalb des Fensters
        pu.reg_source[25].channels[1] = 40ULL; // Differenz von 35 Ticks
    }

    printf("\n[START] Synapse [15->25] geladen. Init-Gewicht: %llu\n", pu.reg_source[15].channels[3]);
    printf(" Prä-Spike (Pre):  Tick #%llu\n", pu.reg_source[15].channels[1]);
    printf(" Post-Spike (Post): Tick #%llu\n", pu.reg_source[25].channels[1]);
    printf("--- Starte Zeitreihen-Simulation (3 Evaluierungs-Ticks) ---\n");

    for (int t = 1; t <= 3; t++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, STDPPlasticityRule);

        uint64_t current_weight = pu.reg_source[15].channels[3];
        printf("  Tick #%d -> Aktuelles Synapsengewicht: %llu\n", t, current_weight);
    }

    /* Endauswertung im Terminal protokollieren */
    uint64_t final_weight = pu.reg_source[15].channels[3];
    printf("\n[RESULTAT] ");
    if (choice == 1) {
        if (final_weight > 40) printf("LTP erfolgreich: Kausale Korrelation erkannt. Verbindung verstaerkt.\n");
    }
    else if (choice == 2) {
        if (final_weight < 40) printf("LTD erfolgreich: Destruktive Asynchronitaet erkannt. Verbindung gedrosselt.\n");
    }
    else {
        if (final_weight == 40) printf("No-Change erfolgreich: Keine zeitliche Relevanz innerhalb des Fensters.\n");
    }

    ProPhysics_Free(&pu);
    return 0;
}