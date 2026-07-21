/* ==========================================================================
 * ProPhysics - DLL / Shared Object Export Gates
 * File: ProPhysics_Exports.h
 * Architecture: Cross-Platform Compiler Gating & Unified Legacy Bridge
 * ========================================================================== */

#ifndef PROPHYSICS_EXPORTS_H
#define PROPHYSICS_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef PROPHYSICS_EXPORTS
#define PROPHYSICS_API __declspec(dllexport)
#elif defined(PROPHYSICS_DLL_IMPORT)
#define PROPHYSICS_API __declspec(dllimport)
#else
#define PROPHYSICS_API /* Leer für statisches Linken im Monolithen */
#endif
#else
#if __GNUC__ >= 4
#define PROPHYSICS_API __attribute__((visibility("default")))
#else
#define PROPHYSICS_API
#endif
#endif

    /* --- LEGACY ALIASES (Abwärtskompatibilität für Altsysteme & alte SDK-Demos) --- */
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

#endif // PROPHYSICS_EXPORTS_H