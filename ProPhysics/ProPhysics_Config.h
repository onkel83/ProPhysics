/* ==========================================================================
 * ProPhysics - Unified Configuration & Hardware Tiers (Wheeler "It from Bit")
 * File: ProPhysics_Config.h
 * Architecture: C99, Static Compile-Time Constants (Zero-Allocation Core)
 * ========================================================================== */

#ifndef PROPHYSICS_CONFIG_H
#define PROPHYSICS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

 // --- MAXIMALE SPEICHER-SCHRANKE FÜR DAS POINTER-REGISTER ---
 // Bestimmt die maximale Anzahl der gleichzeitig trackbaren Urtypen-Knoten im 4-GB-Limit
#ifndef MAX_SPARSE_TRACKING_NODES
#define MAX_SPARSE_TRACKING_NODES 64000000ULL
#endif

// --- TYPDEFINITIONEN FÜR HIGH-END METRIKEN ---
typedef double Real;
typedef uint64_t Index;

#endif // PROPHYSICS_CONFIG_H