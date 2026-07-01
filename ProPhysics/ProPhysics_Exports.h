/* ==========================================================================
 * ProPhysics - DLL / Shared Object Export Gates
 * File: ProPhysics_Exports.h
 * Architecture: Cross-Platform Compiler Gating
 * ========================================================================== */

#ifndef PROPHYSICS_EXPORTS_H
#define PROPHYSICS_EXPORTS_H

#ifdef _WIN32
#ifdef PROPHYSICS_EXPORTS
#define PROPHYSICS_API __declspec(dllexport)
#elif defined(PROPHYSICS_DLL_IMPORT)
#define PROPHYSICS_API __declspec(dllimport)
#else
#define PROPHYSICS_API // Leer für statisches Linken im Monolithen
#endif
#define PRO_EXPORT PROPHYSICS_API // Abwärtskompatibilität für Altsysteme
#else
#if __GNUC__ >= 4
#define PROPHYSICS_API __attribute__((visibility("default")))
#else
#define PROPHYSICS_API
#endif
#define PRO_EXPORT PROPHYSICS_API
#endif

#endif // PROPHYSICS_EXPORTS_H