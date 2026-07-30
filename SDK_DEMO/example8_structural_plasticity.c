/**
 * @file example8_structural_plasticity.c
 * @brief BioAI SDK - Strukturelle Plastizität, Synaptisches Pruning & Konsolidierung (Interaktiv).
 *
 * Simuliert die strukturelle Reorganisation des Netzwerks. Fällt das synaptische
 * Gewicht (Kanal 3) unter eine kritische Schwelle, wird die topologische Verbindung
 * (Kanal 0) branchless gekappt (Pruning), es sei denn, ein neurotropher Schutzfaktor
 * (Kanal 2) stabilisiert die Synapse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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
 * @brief Hilfsfunktion zur Erstellung eines ASCII-Balkendiagramms [0..100].
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
    uint64_t is_degenerated = (weight < PRUNING_THRESHOLD);
    uint64_t has_no_protection = (trophic_factor == 0ULL);

    uint64_t trigger_pruning = is_neuron && is_degenerated && has_no_protection;

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

    /* Sauberer Cast auf uint32_t zur Behebung von Warning C4244 */
    pu->global_entropy_index = (uint32_t)active_interactions;
}

/**
 * @brief Setzt die Simulation auf den Initialzustand des gewählten Szenarios zurück.
 */
static void Reset_Simulation(ProUniverse* pu) {
    memset(pu->ur_grid, 0, pu->total_nodes * sizeof(*pu->ur_grid));
    memset(pu->reg_source, 0, pu->total_nodes * sizeof(*pu->reg_source));
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(*pu->reg_target));

    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;

    pu->ur_grid[42].type_state = TYPE_NEURON;
    pu->ur_grid[84].type_state = TYPE_NEURON;
    pu->reg_source[42].channels[0] = 84ULL;

    if (g_PlasticityConfig.scenario == 1) {
        pu->reg_source[42].channels[3] = 3ULL;  /* Degeneriertes Gewicht */
        pu->reg_source[42].channels[2] = 0ULL;  /* Ungeschützt */
    }
    else if (g_PlasticityConfig.scenario == 2) {
        pu->reg_source[42].channels[3] = 3ULL;  /* Degeneriertes Gewicht */
        pu->reg_source[42].channels[2] = 1ULL;  /* Schutzfaktor aktiv */
    }
    else {
        pu->reg_source[42].channels[3] = 75ULL; /* Gesundes Gewicht */
        pu->reg_source[42].channels[2] = 0ULL;  /* Kein Schutz nötig */
    }
}

/**
 * @brief Konsolen-Visualisierung des aktuellen Strukturzustands.
 */
static void Print_Structural_State(const ProUniverse* pu) {
    uint64_t target = pu->reg_source[42].channels[0];
    uint64_t weight = pu->reg_source[42].channels[3];
    uint64_t trophic = pu->reg_source[42].channels[2];

    printf("\n========================================================================================\n");
    printf(" BIOAI STRUCTURAL PLASTICITY CORE | Tick #%-3llu | Threshold: %llu\n",
        (unsigned long long)pu->current_cpu_tick,
        (unsigned long long)PRUNING_THRESHOLD);
    printf("========================================================================================\n");

    printf(" KNOTEN #42 [Presynaptisch] : Ziel-ID = ");
    if (target == UNLINKED_SENTINEL) {
        printf("UNLINKED (Gekappt)     \n");
    }
    else {
        printf("%-3llu (Aktiv)             \n", (unsigned long long)target);
    }

    printf(" SYNAPSESTÄRKE (Ch 3)     : ");
    Print_Weight_Balken(weight);
    printf("\n");

    printf(" TROPHISCHER FAKTOR (Ch 2)  : %llu %s\n",
        (unsigned long long)trophic,
        (trophic > 0ULL) ? "(Protektion Aktiv)" : "(Kein Schutz)");

    printf(" STRUKTUR-STATUS          : ");
    if (target == UNLINKED_SENTINEL) {
        printf("[ PRUNING EXECUTED / VERBINDUNG GELÖSCHT ]\n");
    }
    else if (weight < PRUNING_THRESHOLD && trophic == 0ULL) {
        printf("[ KRITISCH: FÄLLT IM NÄCHSTEN TICK ][\n");
    }
    else {
        printf("[ STRUKTURELL STABIL / INTAKT ]\n");
    }
    printf("========================================================================================\n\n");
}

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 100);

    Reset_Simulation(&pu);

    char choice = 0;
    while (true) {
        Print_Structural_State(&pu);

        printf("--- INTERAKTIVE STRUKTURELLE PLASTIZITÄT CONSOLE ---\n");
        printf(" [1] 1 Kausal-Tick ausfuehren\n");
        printf(" [2] 3 Ticks in Folge ausfuehren\n");
        printf(" [3] Preset: Kritisches Decay     (Gewicht = 3, Schutz = 0 -> Pruning)\n");
        printf(" [4] Preset: Protektiver Faktor   (Gewicht = 3, Schutz = 1 -> Stabil)\n");
        printf(" [5] Preset: Gesunde Konsistenz   (Gewicht = 75, Schutz = 0 -> Stabil)\n");
        printf(" [6] Synapsengewicht (Ch 3) manuell setzen\n");
        printf(" [7] Neurotropen Schutzfaktor (Ch 2) umschalten (0/1)\n");
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
            ProPhysics_SDK_Execute_Custom_Tick(&pu, StructuralPlasticityRule);
            printf("\n>> Tick #%llu verarbeitet.\n", (unsigned long long)pu.current_cpu_tick);
            break;

        case '2':
            for (int i = 0; i < 3; i++) {
                ProPhysics_SDK_Execute_Custom_Tick(&pu, StructuralPlasticityRule);
            }
            printf("\n>> 3 Ticks vollzogen.\n");
            break;

        case '3':
            g_PlasticityConfig.scenario = 1;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Kritisches Decay geladen.\n");
            break;

        case '4':
            g_PlasticityConfig.scenario = 2;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Protektiver Faktor geladen.\n");
            break;

        case '5':
            g_PlasticityConfig.scenario = 3;
            Reset_Simulation(&pu);
            printf("\n[PRESET OK] Gesunde Konsistenz geladen.\n");
            break;

        case '6': {
            uint64_t w = 0;
            printf("\nNeues Synapsengewicht (0-100) eingeben > ");
            if (scanf("%llu", &w) == 1) {
                pu.reg_source[42].channels[3] = (w > 100ULL) ? 100ULL : w;
                printf("[OK] Gewicht auf %llu gesetzt.\n", (unsigned long long)pu.reg_source[42].channels[3]);
            }
            break;
        }

        case '7': {
            uint64_t factor = 0;
            printf("\nTrophischen Schutzfaktor (0 = Aus, 1 = Aktiv) eingeben > ");
            if (scanf("%llu", &factor) == 1) {
                pu.reg_source[42].channels[2] = (factor > 0ULL) ? 1ULL : 0ULL;
                printf("[OK] Schutzfaktor auf %llu gesetzt.\n", (unsigned long long)pu.reg_source[42].channels[2]);
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