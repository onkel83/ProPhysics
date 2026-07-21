/**
 * @file example5_homeostasis.c
 * @brief BioAI SDK - Intrinsische Homeostase & Exzitabilitäts-Regulierung (Interaktiv).
 *
 * Dieses Modul simuliert die homeostatische Selbstregulation von Netzknoten.
 * Um ein biologisches System in einem stabilen energetischen Gleichgewicht zu
 * halten, passen die Knoten ihre Empfindlichkeit (intrinsische Exzitabilität)
 * invers zum historischen Aktivitätslevel vollkommen branchless an.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände der Netzwerkknoten --- */
#define STATE_NEUTRAL   0x00U  /**< Inaktiver Knoten / Ruhezustand */
#define TYPE_NEURON     0x05U  /**< Aktiver neuronaler Knoten */

/**
 * @struct HomeostasisConfig
 * @brief Steuermatrix für das zelluläre energetische Gleichgewicht.
 */
typedef struct {
    uint64_t target_activity;  /**< Der energetische Soll-Wert (z.B. 2 Aktivierungen) */
    uint64_t plasticity_rate;  /**< Korrektur-Schrittweite pro Regelkreis-Tick */
    int scenario;                /**< Aktuelles Versuchsszenario */
} HomeostasisConfig;

/* Globale Konfiguration für die homeostatischen Experimente */
static HomeostasisConfig g_HomeoConfig = { 2ULL, 5ULL, 1 };

/**
 * @brief Hilfsfunktion zur Erstellung eines ASCII-Balkendiagramms für Wertebereich [0..250].
 */
static void Print_Sensitivitaets_Balken(uint64_t sensitivity) {
    const int max_bars = 25;
    int filled = (int)((sensitivity * max_bars) / 250ULL);
    if (filled > max_bars) filled = max_bars;

    printf("[");
    for (int i = 0; i < max_bars; i++) {
        if (i < filled) printf("#");
        else printf(".");
    }
    printf("] %3llu / 250", (unsigned long long)sensitivity);
}

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

    /* Initialisiere Testknoten 30 als aktives Funktionselement */
    pu->ur_grid[30].type_state = TYPE_NEURON;
    pu->ur_grid[40].type_state = TYPE_NEURON;

    /* Routing-Vektor legen: Knoten 30 zielt auf Knoten 40 */
    pu->reg_source[30].channels[0] = 40ULL;

    /* Start-Aktivität basierend auf gewähltem Szenario */
    if (g_HomeoConfig.scenario == 1) {
        pu->reg_source[30].channels[1] = 5ULL;  /* Überreizung */
    }
    else if (g_HomeoConfig.scenario == 2) {
        pu->reg_source[30].channels[1] = 0ULL;  /* Deprivation */
    }
    else {
        pu->reg_source[30].channels[1] = g_HomeoConfig.target_activity; /* Balance */
    }

    /* Neutraler Start-Sensitivitätswert: 100 */
    pu->reg_source[30].channels[2] = 100ULL;
}

/**
 * @brief Konsolen-Visualisierung des homeostatischen Systemzustands.
 */
static void Print_Network_State(const ProUniverse* pu) {
    uint64_t act = pu->reg_source[30].channels[1];
    uint64_t sens = pu->reg_source[30].channels[2];
    uint64_t target = g_HomeoConfig.target_activity;

    printf("\n========================================================================================\n");
    printf(" BIOAI HOMEOSTASIS CORE | Tick #%-3llu | Step-Rate: %llu\n",
        (unsigned long long)pu->current_cpu_tick,
        (unsigned long long)g_HomeoConfig.plasticity_rate);
    printf("========================================================================================\n");

    printf(" ZELLULÄRER ZUSTAND [Knoten #30]:\n");
    printf("  Ist-Aktivitaet (Ch 1) : %llu  |  Soll-Wert (Target) : %llu  --> ",
        (unsigned long long)act, (unsigned long long)target);

    if (act > target) {
        printf("[ OVER-EXCITED -> DAEMPFUNG ]\n");
    }
    else if (act < target) {
        printf("[ UNDER-EXCITED -> SENSITIVIERUNG ]\n");
    }
    else {
        printf("[ OPTIMAL -> HOMEOSTASE / STABIL ]\n");
    }

    printf("  Exzitabilitaet (Ch 2) : ");
    Print_Sensitivitaets_Balken(sens);
    printf("\n========================================================================================\n\n");
}

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    Reset_Simulation(&pu);

    char choice = 0;
    while (true) {
        Print_Network_State(&pu);

        printf("--- INTERAKTIVE HOMEOSTASIS REGLERKREIS CONSOLE ---\n");
        printf(" [1] 1 Regelkreis-Tick ausfuehren\n");
        printf(" [2] 5 Ticks in Folge ausfuehren\n");
        printf(" [3] Preset: Chronische Ueberreizung  (Aktivitaet: 5, Target: 2)\n");
        printf(" [4] Preset: Sensorische Deprivation (Aktivitaet: 0, Target: 2)\n");
        printf(" [5] Preset: Fliessgleichgewicht     (Aktivitaet: 2, Target: 2)\n");
        printf(" [6] Ist-Aktivitaet manuell setzen\n");
        printf(" [7] Soll-Wert & Plastizitaetsrate anpassen\n");
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
            ProPhysics_SDK_Execute_Custom_Tick(&pu, HomeostaticRegulationRule);
            printf("\n>> Tick #%llu verarbeitet.\n", (unsigned long long)pu.current_cpu_tick);
            break;

        case '2':
            for (int i = 0; i < 5; i++) {
                ProPhysics_SDK_Execute_Custom_Tick(&pu, HomeostaticRegulationRule);
            }
            printf("\n>> 5 Ticks vollzogen.\n");
            break;

        case '3':
            g_HomeoConfig.scenario = 1;
            g_HomeoConfig.target_activity = 2ULL;
            g_HomeoConfig.plasticity_rate = 5ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Ueberreizungs-Szenario geladen.\n");
            break;

        case '4':
            g_HomeoConfig.scenario = 2;
            g_HomeoConfig.target_activity = 2ULL;
            g_HomeoConfig.plasticity_rate = 5ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Deprivations-Szenario geladen.\n");
            break;

        case '5':
            g_HomeoConfig.scenario = 3;
            g_HomeoConfig.target_activity = 2ULL;
            g_HomeoConfig.plasticity_rate = 5ULL;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Fliessgleichgewicht geladen.\n");
            break;

        case '6': {
            uint64_t val = 0;
            printf("\nNeue Ist-Aktivitaet fuer Knoten #30 eingeben > ");
            if (scanf("%llu", &val) == 1) {
                pu.reg_source[30].channels[1] = val;
                printf("[OK] Ist-Aktivitaet auf %llu gesetzt.\n", (unsigned long long)val);
            }
            break;
        }

        case '7': {
            uint64_t t = 0, r = 0;
            printf("\nNeuer Soll-Wert (Target Activity) > ");
            if (scanf("%llu", &t) == 1) g_HomeoConfig.target_activity = t;
            printf("Neue Korrektur-Schrittweite (Plasticity Rate) > ");
            if (scanf("%llu", &r) == 1) g_HomeoConfig.plasticity_rate = r;
            printf("\n[OK] Reglerparameter aktualisiert.\n");
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