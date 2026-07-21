/**
 * @file example7_stdp.c
 * @brief BioAI SDK - Kausales Spike-Timing-Dependent Plasticity (STDP) System (Interaktiv).
 *
 * Simuliert die zeitabhängige synaptische Plastizität. Die Stärke einer Verbindung
 * (Kanal 3) skaliert basierend auf der prä- und postsynaptischen Millisekunden-Differenz
 * (Kanal 1) vollkommen ohne Verzweigungsbefehle.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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
    uint64_t delta_weight;    /**< Modifikations-Schrittweite (LTP / LTD) */
    int scenario;             /**< 1 = Kausales Feuern (LTP), 2 = Anti-kausales Feuern (LTD), 3 = Unkorreliert */
} STDPConfig;

/* Globale Konfiguration für das STDP-Lernverhalten */
static STDPConfig g_STDPConfig = { 5ULL, 4ULL, 1 };

/**
 * @brief Hilfsfunktion zur Erstellung eines ASCII-Balkendiagramms für das Synapsengewicht [0..100].
 */
static void Print_Weight_Balken(uint64_t weight) {
    const int max_bars = 20;
    int filled = (int)((weight * max_bars) / 100ULL);
    if (filled > max_bars) filled = max_bars;
    if (filled < 0) filled = 0;

    printf("[");
    for (int i = 0; i < max_bars; i++) {
        if (i < filled) printf("#");
        else printf(".");
    }
    printf("] %3llu / 100", (unsigned long long)weight);
}

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
    uint64_t is_ltp = valid_interaction && (post_tick > pre_tick) && ((post_tick - pre_tick) <= window);
    uint64_t is_ltd = valid_interaction && (pre_tick > post_tick) && ((pre_tick - post_tick) <= window);

    /* Gewichtsanpassung berechnen unter Beachtung der biologischen Grenzen (Safe-Guards) */
    uint64_t potentiated = weight + delta;
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

    /* Sauberer Cast auf uint32_t zur Behebung von Warning C4244 */
    pu->global_entropy_index = (uint32_t)active_interactions;
}

/**
 * @brief Setzt die Testnetz-Topologie auf das gewählte STDP-Szenario zurück.
 */
static void Reset_Simulation(ProUniverse* pu) {
    memset(pu->ur_grid, 0, pu->total_nodes * sizeof(*pu->ur_grid));
    memset(pu->reg_source, 0, pu->total_nodes * sizeof(*pu->reg_source));
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(*pu->reg_target));

    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;

    pu->ur_grid[15].type_state = TYPE_NEURON;
    pu->ur_grid[25].type_state = TYPE_NEURON;

    pu->reg_source[15].channels[0] = 25ULL;
    pu->reg_source[15].channels[3] = 40ULL; /* Startgewicht der Synapse */

    if (g_STDPConfig.scenario == 1) {
        pu->reg_source[15].channels[1] = 10ULL; /* Pre-Spike zuerst */
        pu->reg_source[25].channels[1] = 12ULL; /* Post-Spike folgt (+2) */
    }
    else if (g_STDPConfig.scenario == 2) {
        pu->reg_source[15].channels[1] = 13ULL; /* Pre-Spike verspätet */
        pu->reg_source[25].channels[1] = 10ULL; /* Post-Spike zuerst (-3) */
    }
    else {
        pu->reg_source[15].channels[1] = 5ULL;  /* Weit außerhalb */
        pu->reg_source[25].channels[1] = 40ULL;
    }
}

/**
 * @brief Konsolen-Visualisierung des aktuellen STDP-Zustands.
 */
static void Print_STDP_State(const ProUniverse* pu) {
    uint64_t pre_tick = pu->reg_source[15].channels[1];
    uint64_t post_tick = pu->reg_source[25].channels[1];
    uint64_t weight = pu->reg_source[15].channels[3];

    printf("\n========================================================================================\n");
    printf(" BIOAI STDP PLASTICITY CORE | Tick #%-3llu | Window: %llu | Delta: %llu\n",
        (unsigned long long)pu->current_cpu_tick,
        (unsigned long long)g_STDPConfig.learning_window,
        (unsigned long long)g_STDPConfig.delta_weight);
    printf("========================================================================================\n");

    printf(" KNOTEN #15 [Presynaptisch] : Spike-Tick = %-3llu  | Synapsengewicht (Ch 3): ",
        (unsigned long long)pre_tick);
    Print_Weight_Balken(weight);
    printf("\n");

    printf(" KNOTEN #25 [Postsynaptisch]: Spike-Tick = %-3llu\n",
        (unsigned long long)post_tick);

    uint64_t window = g_STDPConfig.learning_window;
    int is_ltp = (post_tick > pre_tick) && ((post_tick - pre_tick) <= window);
    int is_ltd = (pre_tick > post_tick) && ((pre_tick - post_tick) <= window);

    printf(" PLASTIZITAETS-STATUS      : ");
    if (is_ltp) {
        printf("[ LTP AKTIV (Kausale Korrelation / Verstärkung) ]\n");
    }
    else if (is_ltd) {
        printf("[ LTD AKTIV (Anti-kausal / Drosselung) ]\n");
    }
    else {
        printf("[ ASYNCHRON / NEUTRAL (Außerhalb des Lernfensters) ]\n");
    }
    printf("========================================================================================\n\n");
}

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    Reset_Simulation(&pu);

    char choice = 0;
    while (true) {
        Print_STDP_State(&pu);

        printf("--- INTERAKTIVE STDP PLASTICITY CONSOLE ---\n");
        printf(" [1] 1 Kausal-Tick ausfuehren\n");
        printf(" [2] 3 Ticks in Folge ausfuehren\n");
        printf(" [3] Preset: Kausales Feuern      (Pre: 10, Post: 12 -> LTP)\n");
        printf(" [4] Preset: Anti-kausales Feuern (Pre: 13, Post: 10 -> LTD)\n");
        printf(" [5] Preset: Asynchrones Rauschen   (Außerhalb des Lernfensters)\n");
        printf(" [6] Manuelle Spike-Zeiten (Ch 1) anpassen\n");
        printf(" [7] Synapsengewicht manuell setzen\n");
        printf(" [8] Lernfenster (Learning Window) anpassen\n");
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
            ProPhysics_SDK_Execute_Custom_Tick(&pu, STDPPlasticityRule);
            printf("\n>> Tick #%llu verarbeitet.\n", (unsigned long long)pu.current_cpu_tick);
            break;

        case '2':
            for (int i = 0; i < 3; i++) {
                ProPhysics_SDK_Execute_Custom_Tick(&pu, STDPPlasticityRule);
            }
            printf("\n>> 3 Ticks vollzogen.\n");
            break;

        case '3':
            g_STDPConfig.scenario = 1;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Kausales Feuern (LTP) geladen.\n");
            break;

        case '4':
            g_STDPConfig.scenario = 2;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Anti-kausales Feuern (LTD) geladen.\n");
            break;

        case '5':
            g_STDPConfig.scenario = 3;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Asynchrones Rauschen geladen.\n");
            break;

        case '6': {
            uint64_t pre = 0, post = 0;
            printf("\nNeuen Spike-Tick fuer Knoten #15 (Pre) eingeben > ");
            if (scanf("%llu", &pre) == 1) pu.reg_source[15].channels[1] = pre;
            printf("Neuen Spike-Tick fuer Knoten #25 (Post) eingeben > ");
            if (scanf("%llu", &post) == 1) pu.reg_source[25].channels[1] = post;
            printf("[OK] Spike-Zeiten aktualisiert.\n");
            break;
        }

        case '7': {
            uint64_t w = 0;
            printf("\nNeues Synapsengewicht (0-100) eingeben > ");
            if (scanf("%llu", &w) == 1) {
                pu.reg_source[15].channels[3] = (w > 100ULL) ? 100ULL : w;
                printf("[OK] Gewicht auf %llu gesetzt.\n", (unsigned long long)pu.reg_source[15].channels[3]);
            }
            break;
        }

        case '8': {
            uint64_t win = 0;
            printf("\nNeues Lernfenster (Ticks) eingeben > ");
            if (scanf("%llu", &win) == 1) {
                g_STDPConfig.learning_window = win;
                printf("[OK] Lernfenster auf %llu Ticks angepasst.\n", (unsigned long long)win);
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