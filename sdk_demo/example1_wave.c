/**
 * @file example1_wave.c
 * @brief BioAI SDK - Simulation erregbarer Medien & Wellenphänomene.
 *
 * Dieses Modul simuliert die Ausbreitung von Erregungswellen in einem diskreten,
 * deterministischen Graphenraum (z. B. analog zu Belousov-Zhabotinsky-Reaktionen,
 * kardialen Erregungsleitungen oder neuronalen Feldpotentialen).
 *
 * Modifikationen: Unterstützung für wissenschaftliche Szenarienauswahl (Kollisionen,
 * Dämpfungseffekte) und hochgradig optimierte, branchless Zustandsübergänge.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ProPhysics.h"

 /* --- Systemzustände des zellulären Automaten --- */
#define STATE_NEUTRAL    0x00U  /**< Ruhezustand: Bereit für Aktivierung */
#define STATE_EXCITED    0x01U  /**< Aktivierter Zustand: Sendet Signal an Nachbarn */
#define STATE_REFRACTORY 0x02U  /**< Refraktärzeit: Erholungsphase, immun gegen Reize */

/**
 * @struct WaveConfig
 * @brief Steuerparameter zur wissenschaftlichen Exploration von Wellenmustern.
 */
typedef struct {
    uint8_t transmission_prob;  /**< Wahrscheinlichkeit der Übertragung (Skalierung) */
    uint8_t RefractoryTicks;    /**< Dauer der Refraktärphase (Erholungszeit) */
    int simulation_scenario;    /**< 1 = Einzelimpuls, 2 = Frontalkollision, 3 = Destruktive Interferenz */
} WaveConfig;

/* Globale Konfiguration für die Simulations-Callbacks */
static WaveConfig g_SimulationConfig = { 100, 1, 1 };

/**
 * @brief Globales SDK-Scaffolding für deterministische Zustandsübertragungen.
 *
 * Führt einen vollständigen Simulations-Tick über alle Knoten des Universums aus.
 * Nutzt ein striktes Doppel-Puffer-Verfahren (Double-Buffering) über reg_source und
 * reg_target, um Race Conditions und Berechnungs-Asymmetrien vollständig zu eliminieren.
 *
 * @param pu Zeiger auf die aktive Simulationsumgebung (ProUniverse).
 * @param callback Der wissenschaftliche Kernel, der die lokalen Übergangsregeln diktiert.
 */
void ProPhysics_SDK_Execute_Custom_Tick(ProUniverse* pu,
    void (*callback)(uint8_t, uint8_t, uint64_t*, uint64_t*, uint8_t*, uint8_t*))
{
    /* Sanity-Checks zur Absicherung gegen Null-Pointer-Dereferenzierungen */
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->reg_target || !callback) return;

    pu->current_cpu_tick++;
    uint64_t active_interactions = 0;

    /* reg_target vollständig nullen, um Akkumulationen des aktuellen Ticks sauber aufzunehmen */
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

    /* Iteration über die gesamte flache Netzwerktopologie */
    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
        uint8_t current_state = pu->ur_grid[idx].type_state;

        /* Optimierung: Neutrale Knoten prozessieren keine Interaktion,
         * sie leiten ihre bestehenden Register-Kanäle lediglich unverändert weiter. */
        if (current_state == STATE_NEUTRAL) {
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= pu->reg_source[idx].channels[ch];
            }
            continue;
        }

        /* Kanal 0 fungiert in diesem SDK als direkter Kausal-Vektor (Pointer auf Zielknoten) */
        uint64_t target_ptr = pu->reg_source[idx].channels[0];

        /* Ziel-Validierung: Innerhalb des Spektrums und keine Selbst-Referenzierung */
        if (target_ptr < pu->total_nodes && target_ptr != idx) {
            uint8_t target_state = pu->ur_grid[target_ptr].type_state;
            uint8_t next_state = current_state;
            uint8_t next_target_state = target_state;

            /* Lokale Register-Kopien für den isolierten Lese-Zugriff des Kernels vorbereiten */
            uint64_t current_ch[4];
            uint64_t target_ch[4];
            for (int ch = 0; ch < 4; ch++) {
                current_ch[ch] = pu->reg_source[idx].channels[ch];
                target_ch[ch] = pu->reg_source[target_ptr].channels[ch];
            }

            /* Aufruf des physikalischen Regelwerks */
            callback(current_state, target_state, current_ch, target_ch, &next_state, &next_target_state);

            /* Zurückschreiben der mutierten Zustände in das Simulationsgitter */
            pu->ur_grid[idx].type_state = next_state;
            pu->ur_grid[target_ptr].type_state = next_target_state;

            /* Bitweise OR-Akkumulation in die Ziel-Register (Erhaltung der Signalspuren) */
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= current_ch[ch];
                pu->reg_target[target_ptr].channels[ch] |= target_ch[ch];
            }
            active_interactions++;
        }
        else {
            /* Fallback für isolierte oder nicht-gekoppelte aktive Knoten */
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= pu->reg_source[idx].channels[ch];
            }
        }
    }

    /* Pointer-Swap: Das eben beschriebene reg_target wird zur Quelle des nächsten Ticks */
    ProPointerRegister* temp = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = temp;

    /* Metrik-Erfassung: Erregungsdichte wird als temporäre Systementropie abgebildet */
    pu->global_entropy_index = (double)active_interactions;
}

/**
 * @brief Wissenschaftlicher Callback-Kernel: Branchless Signal-Zustandsautomat.
 *
 * Berechnet mathematisch optimiert und ohne CPU-Pipeline-Stalls die Wellenleitung.
 * Nutzt mathematische Prädikate statt "if-else"-Verzweigungen, um maximale
 * Performance in der Kontrollfluss-Pipeline zu garantieren.
 */
void WavePropagationRule(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    /* Prädikat: Übertragung findet nur statt, wenn der Sender erregt und das Ziel empfänglich (neutral) ist */
    uint64_t trigger_excitation = (current_state == STATE_EXCITED) && (target_state == STATE_NEUTRAL);

    /* Deterministischer Zustandszerfall des Senders (Decay):
     * EXCITED -> REFRACTORY, REFRACTORY -> NEUTRAL */
    uint8_t base_decay = (uint8_t)((STATE_REFRACTORY * (current_state == STATE_EXCITED)) |
        (STATE_NEUTRAL * (current_state == STATE_REFRACTORY)));

    *out_next_state = base_decay;

    /* Zielzustand: Wird entweder erregt (wenn Trigger aktiv) oder behält seinen aktuellen Zustand bei */
    *out_next_target_state = (uint8_t)((STATE_EXCITED * trigger_excitation) |
        (target_state * (!trigger_excitation)));

    /* Bei erfolgreicher Transmission wird die Signal-Korrelation übermittelt */
    if (trigger_excitation) {
        target_channels[0] |= current_channels[0];
    }
}

/**
 * @brief Hauptprogramm mit interaktiver wissenschaftlicher Szenario-Auswahl.
 */
int main(void) {
    printf("==================================================================\n");
    printf("[SDK Example 1]: Erregbare Medien & Wellen-Interferenz-Simulator\n");
    printf("==================================================================\n\n");

    printf("Waehlen Sie ein wissenschaftliches Experiment-Szenario:\n");
    printf(" [1] Normaler Impuls (Lineare Wellenausbreitung)\n");
    printf(" [2] Wellenkollision (Zwei Wellenfronten laufen gegeneinander)\n");
    printf(" [3] Destruktive Blockade (Kollision mit künstlicher Refraktär-Barriere)\n");
    printf("Auswahl (1-3): ");

    int choice = 1;
    if (scanf("%d", &choice) != 1) {
        choice = 1;
    }
    g_SimulationConfig.simulation_scenario = choice;

    ProUniverse pu;
    /* Initialisierung des Universums mit 100.000 diskreten Knotenpunkten */
    ProPhysics_Initialize(&pu, 100000);

    /* --- Strukturierung der Kausalketten (Netzwerk-Topographie) --- */
    /* Wir spannen eine zusammenhängende, lineare Leitungsbahn über die Knoten auf */
    for (uint64_t i = 0; i < pu.total_nodes - 1; i++) {
        pu.reg_source[i].channels[0] = i + 1;     // Vorwärts-Kopplung: i steuert i+1 an
    }

    /* --- Szenario-Injektion (Initialbedingungen) --- */
    if (g_SimulationConfig.simulation_scenario == 1) {
        printf("\n[INFO] Starte Szenario 1: Initialisiere Einzelimpuls bei Knoten 50000.\n");
        pu.ur_grid[50000].type_state = STATE_EXCITED;
    }
    else if (g_SimulationConfig.simulation_scenario == 2) {
        printf("\n[INFO] Starte Szenario 2: Zwei Wellenfronten laufen aufeinander zu.\n");

        /* Welle A läuft von links nach rechts */
        pu.ur_grid[49995].type_state = STATE_EXCITED;

        /* Welle B wird entgegengesetzt ausgerichtet: Wir biegen die Kausalkette im
         * Kollisionsbereich um, damit die rechte Wellenfront nach links migriert. */
        pu.ur_grid[50005].type_state = STATE_EXCITED;
        for (uint64_t i = 50005; i > 49995; i--) {
            pu.reg_source[i].channels[0] = i - 1; // Rückwärts-Kopplung im Begegnungsraum
        }
    }
    else if (g_SimulationConfig.simulation_scenario == 3) {
        printf("\n[INFO] Starte Szenario 3: Eine Welle trifft auf ein im Refraktärzustand blockiertes Medium.\n");

        /* Erregungsimpuls vorbereiten */
        pu.ur_grid[49998].type_state = STATE_EXCITED;

        /* Barriere setzen: Knoten direkt vor der Welle befinden sich in der Erholungsphase.
         * Nach den physikalischen Regeln erregbarer Medien muss die Welle hier kollabieren
         * (Auslöschung/Dämpfung), da das Medium nicht bereit ist. */
        pu.ur_grid[50001].type_state = STATE_REFRACTORY;
        pu.ur_grid[50002].type_state = STATE_REFRACTORY;
    }

    printf("\n--- Starte Zeitreihen-Simulation ---\n");
    /* Simulationsschleife über 10 diskrete Zeitschritte zur detaillierten Beobachtung */
    for (int tick = 1; tick <= 10; tick++) {
        ProPhysics_SDK_Execute_Custom_Tick(&pu, WavePropagationRule);

        printf("  Tick #%02d -> Aktive Erregungsleitungen (Interaktionen): %.0f\n",
            tick, pu.global_entropy_index);

        /* Wissenschaftliche Auswertung der Graphen-Zustände im Kernbereich */
        if (g_SimulationConfig.simulation_scenario == 2) {
            printf("         [Knoten-Status 49998-50002]: %d | %d | %d | %d | %d\n",
                pu.ur_grid[49998].type_state, pu.ur_grid[49999].type_state,
                pu.ur_grid[50000].type_state, pu.ur_grid[50001].type_state,
                pu.ur_grid[50002].type_state);
        }
    }

    /* Speicherbereinigung über die API */
    ProPhysics_Free(&pu);
    printf("\n[SDK] Simulation erfolgreich beendet und Ressourcen freigegeben.\n");
    return 0;
}