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

// ==========================================================================
// BIT-MASKEN FÜR QUANTUM STATE CONFIGURATION (64-BIT ALIGNED)
// ==========================================================================

#define QUANTUM_MASK_POLARITY      0x0000000000000003ULL // Bit 0-1: Ladung
#define QUANTUM_MASK_SPIN_CHIRAL   0x000000000000000CULL // Bit 2-3: Drehrichtung (Chiralität)
#define QUANTUM_MASK_SPIN_AXIS     0x00000000000000F0ULL // Bit 4-7: Rotationsachse (0-11 im FCC)

// --- Polierungs-Zustände ---
#define QUANTUM_POL_NEUTRAL        0x0ULL // 00
#define QUANTUM_POL_PLUS           0x1ULL // 01
#define QUANTUM_POL_MINUS          0x2ULL // 10

// --- Spin-Chiralität ---
#define QUANTUM_SPIN_NONE          0x0ULL // 00
#define QUANTUM_SPIN_CW            0x1ULL // 01 (Clockwise / Rechts)
#define QUANTUM_SPIN_CCW           0x2ULL // 10 (Counter-Clockwise / Links)

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