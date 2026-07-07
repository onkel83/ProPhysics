/* ==========================================================================
 * ProPhysics - Observer UI Configuration
 * File: Observer_Config.h
 * Architecture: Static Application Layer Constants & Runtime Gating Matrix
 * ========================================================================== */

#ifndef OBSERVER_CONFIG_H
#define OBSERVER_CONFIG_H

#include <stdint.h>

 // --- Fenster-Geometrie für das ProDiBatch-Dashboard ---
#define OBSERVER_CONSOLE_COLS     125
#define OBSERVER_CONSOLE_LINES     32

/* Log-Level Definitionen */
#define OBSERVER_LOG_ERROR 1
#define OBSERVER_LOG_WARN  2
#define OBSERVER_LOG_INFO  3
#define OBSERVER_LOG_DEBUG 4

/* Aktives globales Log-Level fuer die Module.
   Kann von OBSERVER_LOG_ERROR bis OBSERVER_LOG_DEBUG angepasst werden. */
#define OBSERVER_CURRENT_LOG_LEVEL OBSERVER_LOG_INFO

   /* ==========================================================================
    * INTERAKTIVES RUNTIME-GATING (HEISS-INJEKTION)
    * Kapselt die Teststeuerungs-Parameter für die 7 Systemmodule.
    * ========================================================================== */
typedef struct {
    uint32_t active_test_id;   /* 0 = Passiver Observer, >0 = Aktivierter Testmodus */
    double target_intensity;   /* Skalierungsintensität (z.B. Energieeinspeisung, Feldstärke) */
    int32_t custom_param;      /* Modulspezifischer Geometrie- oder Struktur-Parameter */
} ModuleTestControl;

/* Globales Kontrollregister – Exponiert für die Steuerzentrale und alle Module */
extern ModuleTestControl global_mod_control[7];

/* Eindeutige Index-Zuweisung für die caching-optimierte Array-Indizierung */
#define MOD_INDEX_MECHANICS       0
#define MOD_INDEX_ACOUSTICS       1
#define MOD_INDEX_THERMODYNAMICS  2
#define MOD_INDEX_OPTICS          3
#define MOD_INDEX_ELECTROTECH     4
#define MOD_INDEX_CHEMISTRY       5
#define MOD_INDEX_NUCLEAR         6

/*
   Das zentrale X-Macro zur Compile-Time Registrierung aller physikalischen Module.
   Format: X(Eindeutiger_Identifier, Funktions_Suffix)

   Wenn du ein neues Modul aus deiner Physiksammlung hinzufügst, musst du es nur
   hier eintragen und die entsprechende .c Datei im Makefile listen.
*/
#define OBSERVER_MODULE_LIST \
    X(MECHANICS,       mechanics)       \
    X(ACOUSTICS,       acoustics)       \
    X(THERMODYNAMICS,  thermodynamics)  \
    X(OPTICS,          optics)          \
    X(ELECTROTECH,     electrotech)     \
    X(CHEMISTRY,       chemistry)       \
    X(NUCLEAR,         nuclear)

#endif /* OBSERVER_CONFIG_H */