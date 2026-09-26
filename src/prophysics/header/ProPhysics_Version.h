/* ==========================================================================
 * ProPhysics - Version Register
 * File: ProPhysics_Version.h
 * Architecture: Etappen-Versionierung (MAJOR.MINOR.PATCH).
 *
 *   MAJOR = Phase
 *           1 = Fundament + Eichfeld + erste Validierung (aktuell)
 *           2 = Komplette QM (nach Etappe 24-27)
 *           3 = Makrophysik (nach M1-M3)
 *   MINOR = Etappen-Nummer
 *   PATCH = Fix innerhalb der Etappe
 *
 * Aktueller Stand: Phase 1, Etappe 23, kein Fix.
 * ========================================================================== */

#ifndef PROPHYSICS_VERSION_H
#define PROPHYSICS_VERSION_H

#define PROPHYSICS_VERSION_MAJOR    1
#define PROPHYSICS_VERSION_MINOR    23
#define PROPHYSICS_VERSION_PATCH    0

 /* Etappen-Nummer. Unabhaengig von PATCH. */
#define PROPHYSICS_ETAPPE           23

/* String-Form fuer BUILD_INFO und Diagnose. */
#define PROPHYSICS_STR_(x) #x
#define PROPHYSICS_STR(x)  PROPHYSICS_STR_(x)
#define PROPHYSICS_VERSION_STRING \
    PROPHYSICS_STR(PROPHYSICS_VERSION_MAJOR) "." \
    PROPHYSICS_STR(PROPHYSICS_VERSION_MINOR) "." \
    PROPHYSICS_STR(PROPHYSICS_VERSION_PATCH)

#endif /* PROPHYSICS_VERSION_H */