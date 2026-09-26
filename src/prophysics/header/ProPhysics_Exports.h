/* ==========================================================================
 * ProPhysics - DLL / Shared Object Export Gates
 * File: ProPhysics_Exports.h
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Zweck: Einheitliche Definition von PROPHYSICS_API fuer
 *        Windows-DLL-Build, Windows-DLL-Import und statisches
 *        Linken (Monolith). Enthaelt zusaetzlich Legacy-Aliase
 *        fuer Rueckwaertskompatibilitaet mit aelteren SDK-Demos.
 *
 * Build-Modi:
 *   PROPHYSICS_EXPORTS    -> __declspec(dllexport)  (DLL bauen)
 *   PROPHYSICS_DLL_IMPORT -> __declspec(dllimport)  (DLL nutzen)
 *   (kein Macro)          -> leer                   (statisch linken)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * ========================================================================== */

#ifndef PROPHYSICS_EXPORTS_H
#define PROPHYSICS_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

	/* --- Basis-Macro --- */
#ifdef _WIN32
#ifdef PROPHYSICS_EXPORTS
#define PROPHYSICS_API __declspec(dllexport)
#elif defined(PROPHYSICS_DLL_IMPORT)
#define PROPHYSICS_API __declspec(dllimport)
#else
#define PROPHYSICS_API /* Leer fuer statisches Linken im Monolithen */
#endif
#else
#if __GNUC__ >= 4
#define PROPHYSICS_API __attribute__((visibility("default")))
#else
#define PROPHYSICS_API
#endif
#endif

/* --- Legacy-Aliase (Abwaertskompatibilitaet) ---
 * Aeltere SDK-Demos und Altsysteme referenzieren PRO_EXPORT, PRO_API
 * oder PROPHYSICS_EXPORT. Diese Aliase mappen sie auf PROPHYSICS_API,
 * damit bestehender Code ohne Anpassung weiterlaeuft. Neue Aufrufer
 * sollten direkt PROPHYSICS_API verwenden. */
#ifndef PRO_EXPORT
#define PRO_EXPORT PROPHYSICS_API
#endif

#ifndef PRO_API
#define PRO_API PROPHYSICS_API
#endif

#ifndef PROPHYSICS_EXPORT
#define PROPHYSICS_EXPORT PROPHYSICS_API
#endif

#ifdef __cplusplus
}
#endif

#endif /* PROPHYSICS_EXPORTS_H */