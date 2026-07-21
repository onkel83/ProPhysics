/* ==========================================================================
 * ProPhysics - Unified Configuration & Hardware Tiers (Wheeler "It from Bit")
 * File: ProPhysics_Config.h
 * Architecture: C99, Static Compile-Time Constants (Zero-Allocation Core)
 * ========================================================================== */

#ifndef PROPHYSICS_CONFIG_H
#define PROPHYSICS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

 /* --- STANDARD-GITTERGRÖSSE / NODE COUNT (Zweierpotenz für Sperrschicht) --- */
#ifndef NODE_COUNT
#define NODE_COUNT 1048576ULL /* 2^20 Knoten (Default Standard-Gitter) */
#endif

/* Legacy-Aliase für ältere SDK-Demos */
#ifndef PRO_NODE_COUNT
#define PRO_NODE_COUNT NODE_COUNT
#endif

#ifndef MAX_NODES
#define MAX_NODES NODE_COUNT
#endif

/* --- MAXIMALE SPEICHER-SCHRANKE FÜR DAS POINTER-REGISTER --- */
/* Bestimmt die maximale Anzahl der gleichzeitig trackbaren Urtypen-Knoten im 4-GB-Limit */
#ifndef MAX_SPARSE_TRACKING_NODES
#define MAX_SPARSE_TRACKING_NODES 64000000ULL
#endif

/* --- CACHE-ALIGNMENT & INLINE HELPER --- */
#ifndef PRO_INLINE
#if defined(_MSC_VER)
#define PRO_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define PRO_INLINE inline __attribute__((always_inline))
#else
#define PRO_INLINE inline
#endif
#endif

/* --- TYPDEFINITIONEN FÜR HIGH-END METRIKEN --- */
typedef double Real;
typedef uint64_t Index;

#endif // PROPHYSICS_CONFIG_H