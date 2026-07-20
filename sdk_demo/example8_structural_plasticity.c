/**
 * @file example8_structural_plasticity.c
 * @brief BioAI SDK - Strukturelle Plastizität, Synaptisches Pruning & Konsolidierung.
 *
 * Dieses Modul simuliert die strukturelle Reorganisation des Netzwerks.
 * Fällt das synaptische Gewicht (Kanal 3) unter eine kritische Schwelle, wird
 * die topologische Verbindung (Kanal 0) branchless gekappt (Pruning), es sei denn,
 * ein neurotropher Schutzfaktor (Kanal 2) stabilisiert die Synapse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände --- */
#define STATE_NEUTRAL       0x00U
#define TYPE_NEURON         0x05U

#define UNLINKED_SENTINEL   0xFFFFFFFFFFFFFFFFULL  /**< Signalisiert eine gekappte Verbindung */
#define PRUNING_THRESHOLD   5ULL                   /**< Kritische Gewichtsschwelle */

/**
 * @struct PlasticityConfig
 * @brief Szenarienkonfiguration für die strukturelle Bereinigung.
 */
typedef struct {
    int scenario; /**< 1 = Kritisches Decay (Pruning), 2 = Protektion (Trophischer Faktor), 3 = Gesunde Struktur */
} PlasticityConfig;

static PlasticityConfig g_PlasticityConfig = { 1 };

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless Structural Pruning.
 *
 * Überprüft die strukturelle Integrität der Verbindung und löscht bei Degeneration
 * die Ziel-ID aus dem Adressregister, ohne CPU-Branch-Mispredictions zu erzeugen.
 */
void StructuralPlasticityRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    uint64_t is_neuron = (current_state == TYPE_NEURON);

    uint64_t weight = current_channels[3];
    uint64_t trophic_factor = current_channels[2];
    uint64_t current_target = current_channels[0];

    /* --- STRUKTURELLE EVALUATION (BRANCHLESS) --- */
    /* Bedingung A: Gewicht liegt unter dem kritischen Grenzwert */
    uint64_t is_degenerated = (weight < PRUNING_THRESHOLD);

    /* Bedingung B: Kein neurotropher Schutzfaktor vorhanden, der die Synapse stützt */
    uint64_t has_no_protection = (trophic_factor == 0ULL);

    /* Pruning ausführen, wenn Neuron degeneriert UND ungeschützt ist */
    uint64_t trigger_pruning = is_neuron && is_degenerated && has_no_protection;

    /* Wenn getrennt, wird die Ziel-ID durch den Sentinel-Wert überschrieben */
    current_channels[0] = (current_target * (!trigger_pruning)) | (UNLINKED_SENTINEL * trigger_pruning);
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

        /* Validierung: Verhindert Interaktion bei gesetztem Sentinel-Wert */
        if (tp < pu->total_nodes && tp != idx && pu->reg_source[idx].channels[0] != UNLINKED_SENTINEL) {
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
    printf("[SDK Example 8]: Strukturelles Synapsen-Pruning & Protektion\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das Plastizitaets-Szenario:\n");
    printf(" [1] Kritisches Decay     (Gewicht = 3 -> Unterschreitet Schwelle -> Verbindung gekappt)\n");
    printf(" [2] Protektiver Faktor   (Gewicht = 3, aber Schutzfaktor aktiv -> Struktur bleibt)\n");
    printf(" [3] Gesunde Konsistenz   (Gewicht = 75 -> Weit ueber Schwelle -> Struktur intakt)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_PlasticityConfig.scenario = choice;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    /* Topologie initialisieren: Knoten 42 zielt auf Knoten 84 */
    pu.ur_grid[42].type_state = TYPE_NEURON;
    pu.ur_grid[84].type_state = TYPE_NEURON;
    pu.reg_source[42].channels[0] = 84ULL;

    /* Konfiguration der Kanäle basierend auf Szenario */
    if (choice == 1) {
        pu.reg_source[42].channels[3] = 3ULL;  // Degeneriertes Gewicht
        pu.reg_source[42].channels[2] = 0ULL;  // Ungeschützt
    }
    else if (choice == 2) {
        pu.reg_source[42].channels[3] = 3ULL;  // Degeneriertes Gewicht
        pu.reg_source[42].channels[2] = 1ULL;  // Neurotropher Schutzfaktor aktiv!
    }
    else {
        pu.reg_source[42].channels[3] = 75ULL; // Gesundes Gewicht
        pu.reg_source[42].channels[2] = 0ULL;  // Kein extra Schutz nötig
    }

    printf("\n[START] Knoten [42] initialisiert. Ziel-ID: %llu\n", pu.reg_source[42].channels[0]);
    printf(" Synaptisches Gewicht: %llu (Schwelle: %llu)\n", pu.reg_source[42].channels[3], (uint64_t)PRUNING_THRESHOLD);
    printf(" Neurotropher Faktor:  %llu\n", pu.reg_source[42].channels[2]);
    printf("--- Starte Zeitreihen-Simulation (2 Ticks) ---\n");

    for (int t = 1; t <= 2; t++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, StructuralPlasticityRule);

        uint64_t target = pu.reg_source[42].channels[0];
        printf("  Tick #%d -> Aktuelle Ziel-ID im Adressregister: ", t);
        if (target == UNLINKED_SENTINEL) {
            printf("UNLINKED (Gekappt)\n");
        }
        else {
            printf("%llu (Aktiv)\n", target);
        }
    }

    /* Endauswertung */
    uint64_t final_target = pu.reg_source[42].channels[0];
    printf("\n[RESULTAT] ");
    if (final_target == UNLINKED_SENTINEL) {
        printf("Pruning erfolgreich executed. Die degenerative Verbindung wurde physisch isoliert.\n");
    }
    else {
        printf("Strukturelle Stabilitaet erfolgreich. Die Verbindung bleibt im System aktiv.\n");
    }

    ProPhysics_Free(&pu);
    return 0;
}