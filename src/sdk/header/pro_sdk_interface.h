/* ==========================================================================
 * ProPhysics - SDK Interface Header
 * File: pro_sdk_interface.h
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Das SDK-Interface folgt der Kernel-Version. Es gibt keine eigene
 * SDK-Version -- jede Aenderung am Kernel-Pfad (insbesondere am
 * Tick und an ProUniverse) wirkt sich unmittelbar auf die
 * SDK-ABI aus.
 *
 * Refactoring 22-Aenderungen:
 *   - ProPhysics_ScientificRuleCallback ist jetzt ein Alias fuer
 *     ProPhysics_RuleCallback (aus ProPhysics.h). Damit gibt es genau
 *     EINEN Callback-Typ im gesamten System -- keine parallele
 *     Definition mehr.
 *   - ProPhysics_SDK_Execute_Plastizitaet_Tick nimmt den zentralen
 *     Callback-Typ. Die SDK-Signatur bleibt strukturell identisch,
 *     sodass bestehende Aufrufer unveraendert weiterlaufen.
 *
 * Symbole:
 *   - ResearchPlugin_DynamicPlasticTopology: Beispiel-Callback aus dem
 *     SDK-Runner. Bleibt mit voller Signatur (Rueckwaertskompatibilitaet).
 *   - ProPhysics_SDK_Execute_Plastizitaet_Tick: duenner Wrapper um
 *     ProPhysics_Tick. Delegiert die Tick-Orchestrierung an den Kernel.
 *
 * Build-Modus:
 *   In pro_sdk_interface.c wird PRO_SDK_EXPORTS VOR dem Include gesetzt.
 *   Damit expandiert PRO_SDK_API zu __declspec(dllexport) und die
 *   Deklaration im Header stimmt exakt mit der Definition in der .c
 *   ueberein (MSVC C2375: "unterschiedliche Bindung" wird vermieden).
 * ========================================================================== */

#ifndef PRO_SDK_INTERFACE_H
#define PRO_SDK_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "ProPhysics.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef PRO_SDK_EXPORTS
#define PRO_SDK_API __declspec(dllexport)
#elif defined(PRO_SDK_DLL_IMPORT)
#define PRO_SDK_API __declspec(dllimport)
#else
#define PRO_SDK_API
#endif
#else
#if __GNUC__ >= 4
#define PRO_SDK_API __attribute__((visibility("default")))
#else
#define PRO_SDK_API
#endif
#endif

    /* ==========================================================================
     * Callback-Typ-Alias (Refactoring 22)
     *
     * Der zentrale Callback-Typ lebt in ProPhysics.h als
     * ProPhysics_RuleCallback. Damit SDK-Code, der den alten Namen
     * benutzt, unveraendert weiterlaeuft, definieren wir hier einen
     * Alias. Beide Namen sind strukturell identisch -- der Compiler
     * sieht denselben Typ.
     *
     * Neue Aufrufer sollten ProPhysics_RuleCallback direkt nutzen.
     * ========================================================================== */

    typedef ProPhysics_RuleCallback ProPhysics_ScientificRuleCallback;

    /* ==========================================================================
     * SDK-Symbole
     * ========================================================================== */

     /* Beispiel-Callback: einfache Topologie-Permutation. Der Kernel ruft
      * ihn pro Knoten mit nicht-NEUTRAL type_state auf. Siehe
      * pro_sdk_interface.c fuer die Implementierung und ProPhysics.h fuer
      * die Callback-Semantik. */
    PRO_SDK_API void ResearchPlugin_DynamicPlasticTopology(
        uint8_t current_state,
        uint8_t target_state,
        uint64_t* current_channels,
        uint64_t* target_channels,
        uint8_t* out_next_state,
        uint8_t* out_next_target_state,
        uint64_t current_idx,
        uint64_t total_nodes);

    /* Tick-Executor: duenner Wrapper um ProPhysics_Tick.
     *
     * Der Callback darf NULL sein. Dann laeuft die Tick-Dynamik ohne
     * Graph-Update (R7-konform zum Kernel-Verhalten bei callback == NULL).
     *
     * Die Signatur entspricht 1:1 der Kernel-Funktion; der Cast auf
     * ProPhysics_RuleCallback ist ein reiner Typhauch ohne ABI-Aenderung. */
    PRO_SDK_API void ProPhysics_SDK_Execute_Plastizitaet_Tick(
        ProUniverse* pu,
        ProPhysics_RuleCallback callback);

    /* ==========================================================================
     * Versions-Re-Export
     *
     * SDK-Konsumenten koennen die Kernel-Version direkt abfragen, ohne
     * ProPhysics_Version.h separat einzubinden. PRO_SDK_VERSION_STRING
     * ist ein String-Literal ("1.23.0"), PRO_SDK_ETAPPE eine Zahl (23).
     *
     * Nutzen: SDK-Logs und Diagnosen koennen die Version ausgeben, ohne
     * die Header-Struktur des Kernels zu kennen.
     * ========================================================================== */

#define PRO_SDK_VERSION_STRING   PROPHYSICS_VERSION_STRING
#define PRO_SDK_ETAPPE           PROPHYSICS_ETAPPE

#ifdef __cplusplus
}
#endif

#endif /* PRO_SDK_INTERFACE_H */