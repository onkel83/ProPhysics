/* ==========================================================================
 * ProPhysics - Observer UI Configuration
 * File: Observer_Config.h
 * Architecture: Static Application Layer Constants
 * ========================================================================== */

#ifndef OBSERVER_CONFIG_H
#define OBSERVER_CONFIG_H

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