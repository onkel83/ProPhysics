/**
 * @file example5_homeostasis.c
 * @brief BioAI SDK - Intrinsische Homeostase & Exzitabilitäts-Regulierung.
 *
 * Dieses Modul simuliert die homeostatische Selbstregulation von Netzknoten.
 * Um ein biologisches System in einem stabilen energetischen Gleichgewicht zu
 * halten, passen die Knoten ihre Empfindlichkeit (intrinsische Exzitabilität)
 * invers zum historischen Aktivitätslevel vollkommen branchless an.
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
 * @struct HomeostasisConfig
 * @brief Steuermatrix für das zelluläre energetische Gleichgewicht.
 */
typedef struct {
    uint64_t target_activity;  /**< Der energetische Soll-Wert (z.B. 2 Aktivierungen) */
    uint64_t plasticity_rate;   /**< Korrektur-Schrittweite pro Regelkreis-Tick */
    int scenario;               /**< 1 = Chronische Überreizung, 2 = Sensorische Deprivation, 3 = Fließgleichgewicht */
} HomeostasisConfig;

/* Globale Konfiguration für die homeostatischen Experimente */
static HomeostasisConfig g_HomeoConfig = { 2ULL, 5ULL, 1 };

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless Homeostatic Scaler.
 *
 * Gleicht das historische Aktivitätslevel (Kanal 1) mit dem biologischen
 * Soll-Wert ab und mutiert den Skalierungsfaktor (Kanal 2) rein mathematisch.
 */
void HomeostaticRegulationRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    (void)target_channels;
    *out_next_state = current_state;
    *out_next_target_state = target_state;

    /* Kanal 1: Akkumulierter Aktivitätszähler (Ladungsverlauf)
     * Kanal 2: Intrinsischer Sensitivitätsfaktor (Start-Basiswert: 100) */
    uint64_t activity = current_channels[1];
    uint64_t sensitivity = current_channels[2];
    uint64_t target = g_HomeoConfig.target_activity;
    uint64_t rate = g_HomeoConfig.plasticity_rate;

    /* --- HOMEOSTATIC FEEDBACK (BRANCHLESS) --- */
    /* Prädikat A: System ist überlastet (Aktivität > Soll-Wert) -> Sensitivität drosseln */
    uint64_t over_excited = (activity > target);
    /* Prädikat B: System ist unterversorgt (Aktivität < Soll-Wert) -> Sensitivität anheben */
    uint64_t under_excited = (activity < target);

    /* Berechne Dämpfung (Untergrenze branchless bei 0 sichern) */
    uint64_t decremented = sensitivity - (rate * (sensitivity >= rate));
    /* Berechne Sensitivierung (Obergrenze branchless bei 250 deckeln) */
    uint64_t incremented = sensitivity + (rate * ((sensitivity + rate) <= 250ULL));

    /* Multiplexing ohne Kontrollfluss-Verzweigung */
    uint64_t next_sensitivity = sensitivity;
    next_sensitivity = (decremented * over_excited) | (next_sensitivity * (!over_excited));
    next_sensitivity = (incremented * under_excited) | (next_sensitivity * (!under_excited));

    /* Modifizierte interne Zustandsvariablen zurückspeichern */
    current_channels[2] = next_sensitivity;
}

/**
 * @brief Globales SDK-Scaffolding für deterministische System-Ticks.
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
    printf("[SDK Example 5]: Intrinsisches Homeostatisches Reglersystem\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie das homeostatische Testszenario:\n");
    printf(" [1] Chronische Ueberreizung  (Aktivitaet liegt dauerhaft bei 5 -> Soll: 2)\n");
    printf(" [2] Sensorische Deprivation (Aktivitaet liegt dauerhaft bei 0 -> Soll: 2)\n");
    printf(" [3] Stables Fliessgleichgewicht (Aktivitaet trifft exakt den Soll-Wert 2)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;
    g_HomeoConfig.scenario = choice;

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    /* Initialisiere Testknoten 30 als aktives Funktionselement */
    pu.ur_grid[30].type_state = TYPE_NEURON;
    pu.ur_grid[40].type_state = TYPE_NEURON;

    /* Routing-Vektor legen: 30 zielt auf 40 */
    pu.reg_source[30].channels[0] = 40ULL;

    /* Injektion der historischen Simulationsdaten basierend auf Szenariowahl */
    if (choice == 1) {
        pu.reg_source[30].channels[1] = 5ULL;   /**< Extrem hoher Aktivitätswert */
    }
    else if (choice == 2) {
        pu.reg_source[30].channels[1] = 0ULL;   /**< Nullaktivität (Isoliert) */
    }
    else {
        pu.reg_source[30].channels[1] = 2ULL;   /**< Perfekte Balance */
    }

    /* Start-Exzitabilität (Sensitivität) auf einen neutralen Basiswert von 100 setzen */
    pu.reg_source[30].channels[2] = 100ULL;

    printf("\n[START] Knoten [30] geladen. Init-Sensitivitaet: %llu | Soll-Aktivitaet: %llu\n",
        pu.reg_source[30].channels[2], g_HomeoConfig.target_activity);
    printf("--- Starte Zeitreihen-Simulation (4 feedback-Regelzyklen) ---\n");

    for (int t = 1; t <= 4; t++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, HomeostaticRegulationRule);

        uint64_t current_sens = pu.reg_source[30].channels[2];
        uint64_t current_act = pu.reg_source[30].channels[1];

        printf("  Tick #%d -> Aktuelle Sensitivitaet: %3llu | Gemessenes Signal: %llu\n",
            t, current_sens, current_act);
    }

    /* Wissenschaftliche Bewertung im Log ausgeben */
    uint64_t final_sens = pu.reg_source[30].channels[2];
    printf("\n[RESULTAT] ");
    if (choice == 1) {
        if (final_sens < 100) printf("Dämpfung erfolgreich. Die Exzitabilitaet wurde autonom herunterskaliert.\n");
    }
    else if (choice == 2) {
        if (final_sens > 100) printf("Sensitivierung erfolgreich. Die Zelle steuert gegen den Signalverlust an.\n");
    }
    else {
        if (final_sens == 100) printf("System im Gleichgewicht. Es waren keine regulatorischen Eingriffe notwendig.\n");
    }

    ProPhysics_Free(&pu);
    return 0;
}