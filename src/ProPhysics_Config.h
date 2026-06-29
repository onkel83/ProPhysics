/* ==========================================================================
 * ProPhysics - Unified Configuration & Hardware Tiers
 * File: ProPhysics_Config.h
 * Architecture: C99, Static Compile-Time Constants (Zero-Allocation Core)
 * ========================================================================== */

#ifndef PROPHYSICS_CONFIG_H
#define PROPHYSICS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

 // --- Gitterdimensionen (FCC Space Layout) ---
#define PROPHYSICS_X_MAX              1024
#define PROPHYSICS_Y_MAX              512
#define PROPHYSICS_Z_MAX              512

// Allokationsfreie 3D-Gitterauflösung im flachen RAM-Segment
#define FCC_INDEX(x, y, z) ((uint32_t)(x) + ((uint32_t)(y) * PROPHYSICS_X_MAX) + ((uint32_t)(z) * PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX))

// --- Physikalische Axiome & Schranken ---
#define TOTAL_ELEMENT_AXIOMS       118  
#define MAX_MOLECULE_ATOMS          128  

// --- Typdefinitionen für Next-Core High-End (64-Bit / Double Precision) ---
typedef double Real;     // Höhere Präzision für Thermodynamische Entropie
typedef uint64_t Index;  // Maximaler nativer Adressraum

// --- DLL Export Macros ---
#ifdef _WIN32
#ifdef PROPHYSICS_EXPORTS
#define PRO_EXPORT __declspec(dllexport)
#elif defined(PROPHYSICS_DLL_IMPORT)
#define PRO_EXPORT __declspec(dllimport)
#else
#define PRO_EXPORT // Leer für statisches Linken in die Console-EXE
#endif
#else
#define PRO_EXPORT __attribute__((visibility("default")))
#endif

#endif // PROPHYSICS_CONFIG_H