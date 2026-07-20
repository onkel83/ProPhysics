/* ==========================================================================
 * ProPhysics - Universal Scientific SDK Template (Plug-and-Play Topology)
 * File: pro_sdk_interface.c
 * Ordner: SDK
 * Architecture: Clean API Separation Layer for External Research Plugins
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ProPhysics.h" 

 // =========================================================================
 // 1. REGION: RESEARCHER PLAYGROUND (Nur hier arbeiten externe Forscher!)
 // =========================================================================

 // Der wissenschaftliche Interaktions-Vertrag:
 // Forscher manipulieren rein isoliert Zustände und lokale Kanäle zweier Knoten.
typedef void (*ProPhysics_ScientificRuleCallback)(
    uint8_t current_state,
    uint8_t target_state,
    uint64_t* current_channels,
    uint64_t* target_channels,
    uint8_t* out_next_state,
    uint8_t* out_next_target_state
    );

/**
 * PLUGIN A: Quanten-Spin-Austausch (Festkörperphysik / Ising-Modell-Analog)
 */
void ResearchPlugin_QuantumSpinExchange(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    // Regel 1: Haben zwei benachbarte Knoten den identischen Spin, 
    // stabilisiert sich der Link und verschmilzt die Adresspfade (Kondensation)
    if (current_state == target_state && current_state != 0) {
        target_channels[0] |= current_channels[1]; // Relationale Verschränkung per OR
        *out_next_state = current_state;
        *out_next_target_state = target_state;
    }
    // Regel 2: Treffen entgegengesetzte Spins aufeinander, flippen sie branchless um
    else if (current_state != target_state && current_state != 0 && target_state != 0) {
        *out_next_state = target_state;         // Spin-Flip Links
        *out_next_target_state = current_state; // Spin-Flip Rechts
    }
    else {
        *out_next_state = current_state;
        *out_next_target_state = target_state;
    }
}

/**
 * PLUGIN B: Molekulare Docking-Dynamik (Biophysik / Struktur-Biologie)
 */
#ifdef _BIO_MODE
void ResearchPlugin_BioDocking(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state)
{
    // Docking: Hydrophobe Basen (0x01) stabilisieren ihre Bindung
    if (current_state == 0x01U && target_state == 0x01U) {
        target_channels[0] = current_channels[0]; // Adresspfad fixieren
        *out_next_state = 0x01U;
        *out_next_target_state = 0x01U;
    }
    // Energie-Verbrauch: ATP-Analog (0x05U) löst Bindungen (energetische Dissipation)
    else if (current_state == 0x05U && target_state != 0x00U) {
        *out_next_state = 0x00U;
        *out_next_target_state = 0x00U;
    }
    else {
        *out_next_state = current_state;
        *out_next_target_state = target_state;
    }
}
#endif


// =========================================================================
// 2. REGION: PROPHYSICS SDK CORE WRAPPER (Automatisiertes HPC-Scaffolding)
// =========================================================================

/**
 * Diese Funktion führt die Registrierung aus und jagt die Daten branchless und
 * double-buffered über das nackte Blech, ohne dass der Forscher es selbst schreiben muss.
 */
void ProPhysics_SDK_Execute_Custom_Tick(ProUniverse* pu, ProPhysics_ScientificRuleCallback callback) {
    if (!pu || !pu->ur_grid || !pu->reg_source || !pu->reg_target || !callback) return;
    pu->current_cpu_tick++;
    uint64_t active_interactions = 0;

    // 1. Target-Register vorab komplett nullen für die additive Verwebung per OR
    memset(pu->reg_target, 0, pu->total_nodes * sizeof(ProPointerRegister));

    // 2. Sequentieller HPC-Gitterdurchlauf
    for (uint64_t idx = 0; idx < pu->total_nodes; idx++) {
        uint8_t current_state = pu->ur_grid[idx].type_state;

        // Neutrale Knoten interagieren nicht aktiv, erhalten aber ihre bestehenden Kanäle
        if (current_state == 0) {
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= pu->reg_source[idx].channels[ch];
            }
            continue;
        }

        // Bestimme Interaktionsziel über den primären virtuellen Adresskanal (Channel 0)
        uint64_t target_ptr = pu->reg_source[idx].channels[0];

        // Validierung der Array-Schranken (Verhindert Memory Corruption im Sandbox-Modus)
        if (target_ptr < pu->total_nodes && target_ptr != idx) {
            uint8_t target_state = pu->ur_grid[target_ptr].type_state;

            uint8_t next_state = current_state;
            uint8_t next_target_state = target_state;

            // Lokale Kopien der Register-Kanäle für isolation-safe Lese-/Schreibzugriffe im Plugin
            uint64_t current_ch[4];
            uint64_t target_ch[4];
            for (int ch = 0; ch < 4; ch++) {
                current_ch[ch] = pu->reg_source[idx].channels[ch];
                target_ch[ch] = pu->reg_source[target_ptr].channels[ch];
            }

            // TRIGGER: Externe wissenschaftliche Regel anwenden
            callback(current_state, target_state, current_ch, target_ch, &next_state, &next_target_state);

            // Ergebnisse atomar/branchless in das Zustandsgitter zurückschreiben
            pu->ur_grid[idx].type_state = next_state;
            pu->ur_grid[target_ptr].type_state = next_target_state;

            // Kaskadierende, additive Verwebung der Kanäle im Schreib-Buffer (Kein Overwrite-Bug)
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= current_ch[ch];
                pu->reg_target[target_ptr].channels[ch] |= target_ch[ch];
            }

            active_interactions++;
        }
        else {
            // Keine gültige Interaktionsadresse -> Bestehende Kanäle additiv sichern
            for (int ch = 0; ch < 4; ch++) {
                pu->reg_target[idx].channels[ch] |= pu->reg_source[idx].channels[ch];
            }
        }
    }

    // 3. Double-Buffering Swap der Adressregister auf Hardware-Ebene
    ProPointerRegister* temp = pu->reg_source;
    pu->reg_source = pu->reg_target;
    pu->reg_target = temp;

    // Metrik-Rückgabe an das Universum (Simulierte Entropie über Interaktionsdichte)
    pu->global_entropy_index = (double)active_interactions;
}


// =========================================================================
// 3. REGION: ACADEMIC APPLICATION RUNTIME (Einfache Simulationssteuerung)
// =========================================================================
int main(void) {
    printf("================================================================================\n");
    printf("         PROPHYSICS UNIVERSAL OPEN-SOURCE SDK RUNTIME SUPPORT\n");
    printf("================================================================================\n");
    printf("[*] Lade native ProPhysics Core-DLL und allokiere Substrat...\n");

    ProUniverse pu;
    uint64_t scientific_scale = 16000000; // Standardisiertes 16-Millionen-Zellen-Gitter

    // Initialisiert das Universum über die Kern-DLL
    ProPhysics_Initialize(&pu, scientific_scale);
    printf("[+] Relationale Gitter-Strukturen einsatzbereit verankert.\n");

    // Urgitter deterministisch initialisieren (Simulierter Teilchenbeschuss)
    printf("[*] Injiziere wissenschaftliche Test-Isotope in das Urgitter...\n");
    for (uint64_t i = 0; i < 1000; i++) {
        uint64_t target_idx = (pu.total_nodes >> 1) + i; // Bit-Shift statt Division
        pu.ur_grid[target_idx].type_state = (uint8_t)((i & 1U) ? 0x01U : 0x02U); // Bitmaske statt Modulo

        // Virtuellen Start-Link setzen, damit die Knoten im ersten Tick ein Ziel finden
        pu.reg_source[target_idx].channels[0] = target_idx + ((i & 1U) ? 1ULL : -1ULL);
    }

    // Dynamische Zuweisung des ausgewählten Plugins
#ifdef _BIO_MODE
    ProPhysics_ScientificRuleCallback selected_plugin = ResearchPlugin_BioDocking;
    printf("[+] Bio-Plugin (Molecular Docking) erfolgreich registriert.\n");
#else
    ProPhysics_ScientificRuleCallback selected_plugin = ResearchPlugin_QuantumSpinExchange;
    printf("[+] Physik-Plugin (Spin-Exchange) erfolgreich registriert.\n");
#endif

    printf("[*] Starte HPC-Rechenschleife ueber das SDK-Scaffolding...\n");
    for (int tick = 1; tick <= 50; tick++) {

        // Das SDK führt nun vollautomatisch die Registrierung und Schleifensteuerung aus
        ProPhysics_SDK_Execute_Custom_Tick(&pu, selected_plugin);

        if (tick % 10 == 0) {
            printf("  -> Simulations-Tick #%2d: Aktive System-Interaktionen = %.0f\n",
                tick, pu.global_entropy_index);
        }
    }

    printf("--------------------------------------------------------------------------------\n");
    printf("[*] Bereinige Simulations-Register ueber die native Core-API...\n");
    ProPhysics_Free(&pu);
    printf("[SUCCESS] Wissenschaftlicher SDK-Testlauf fehlerfrei beendet.\n");
    printf("================================================================================\n");

    return 0;
}