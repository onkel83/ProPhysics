/* ==========================================================================
 * ProPhysics - Centralized Data Structure Register (Wheeler "It from Bit")
 * File: ProPhysics_Types.h
 * Architecture: C99, 256-Bit Pointer-Register Layout, Zero-Allocation
 * ========================================================================== */

#ifndef PROPHYSICS_TYPES_H
#define PROPHYSICS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

 // --- DIE 5 URTYPEN & RECHTS/LINKS-SPINS (8-BIT ENCODING) ---
#define UR_NEUTRAL       0x00U
#define UR_POSITRON_CW   0x01U
#define UR_POSITRON_CCW  0x02U
#define UR_NEGATRON_CW   0x03U
#define UR_NEGATRON_CCW  0x04U

// 12er-Symmetrie Definition für die Bitmaskierungs-Pfade
#define SYMMETRY_CHANNELS 12

#pragma pack(push, 1)
// Ein Knoten im flachen Urtypen-Array (Extrem cache-lokal)
typedef struct {
    uint8_t type_state;        // Ladung + Spin-Zustand (Urtypen)
    uint8_t reserved_gating;   // Ausrichtung / Padding
} ProUrNode;

// 256-Bit Adress-Register Struktur (Verwaltet 12 Ausgänge + Verschränkung)
typedef struct {
    uint64_t channels[4];      // 4 * 64-Bit = 256-Bit virtueller Adressraum pro Kanal
} ProPointerRegister;
#pragma pack(pop)

typedef struct {
    ProUrNode* __restrict ur_grid;               // Flacher, kleiner Urtypen-Array
    ProPointerRegister* __restrict reg_source;   // Gelesenes Pointer-Register (4 GB Grenze)
    ProPointerRegister* __restrict reg_target;   // Geschriebenes Pointer-Register (4 GB Grenze)

    uint64_t total_nodes;
    uint64_t current_cpu_tick;
    uint64_t dynamic_invariance_target;          // Invarianz-Sollwert (Erwähnte Summation)
    double   global_entropy_index;
} ProUniverse;

#endif // PROPHYSICS_TYPES_H