/* ==========================================================================
 * ProPhysics - Centralized Data Structure Register
 * File: ProPhysics_Types.h
 * Architecture: C99, Monolithic Cache-Optimized Type Boundary
 * ========================================================================== */

#ifndef PROPHYSICS_TYPES_H
#define PROPHYSICS_TYPES_H

#include "ProPhysics_Config.h"

 // --- NEU: Globaler Hardware-Popcount für MSVC Compiler-Gating ---
#ifdef _WIN32
#include <intrin.h>
#define __builtin_popcountll(x) _mm_popcnt_u64((unsigned __int64)(x))
#endif

typedef struct {
    int8_t   dx, dy, dz;
    uint8_t  atomic_number;
} RelativeAtom;

typedef struct {
    int8_t   vx, vy, vz;
    uint32_t tick_divider;
    uint32_t tick_counter;
} KineticVector;

typedef struct {
    uint64_t hash;
    RelativeAtom atoms[MAX_MOLECULE_ATOMS];
    uint32_t     atom_count;
} StructuralForm;

#pragma pack(push, 4) // Erzwingt striktes 4-Byte-Alignment für die Gitter-Arrays

typedef struct {
    uint64_t charge_spin;       // Spin- und Ladungskonfiguration
    uint64_t mass_accumulator;  // Emergente lokale Dichte-Masse
    uint32_t element_token;     // Axiom-Identifikator
    uint32_t tick_counter;      // Relativistischer Takt-Teiler

    // Maximale kinetische Tiefe durch 64-Bit-Akkumulatoren
    int64_t vx;
    int64_t vy;
    int64_t vz;
} QuantumStateIsland;

typedef struct {
    /*
     * Daten-Pool (16 Byte kompakt)
     * Die 48 Byte statische Topologie (neighbor_idx) wurden vollständig eliminiert.
     * Nachbarschafts-Indizes werden on-the-fly berechnet.
     */
    uint32_t active_flux;
    uint32_t state_island_idx;
    uint32_t local_anisotropy;
    uint32_t reserved_gating;
} FCCNode;

#pragma pack(pop) // Schließt das 4-Byte-Packing hier ab, damit double natürlich fließen kann

typedef struct {
    FCCNode* __restrict grid;
    QuantumStateIsland* __restrict data_pool;

    uint32_t* active_nodes_current;
    uint32_t* active_nodes_next;

    uint32_t* active_nodes_kinetic;
    uint64_t  active_count_kinetic;

    uint64_t  active_count_current;
    uint64_t  active_count_next;
    uint8_t* node_active_bitset;

    uint64_t current_cpu_tick;
    uint32_t observer_node_idx;
    uint32_t active_element_count;
    bool     is_hardened;

    // --- NEU: Dynamischer Invarianz-Sollwert für den kosmischen Horizont ---
    uint64_t dynamic_invariance_target;

    double   global_entropy_index;
    double   observer_field_vx;
    double   observer_field_vy;
    double   observer_field_vz;
} ProUniverse;

#endif // PROPHYSICS_TYPES_H