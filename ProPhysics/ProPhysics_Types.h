/**
 * @file ProPhysics_Types.h
 * @brief Rein topologische Erweiterung: Verschränkungen über Pointer-Register
 *        (inkl. 64-Bit Kanal-Indizes & Legacy-Kompatibilitätsschicht)
 */

#ifndef PROPHYSICS_TYPES_H
#define PROPHYSICS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

 // Erhöhung der Pointer-Kanäle pro Knoten für komplexe Moleküle & Feld-Verschränkungen
#define CHANNELS_MAX 4

/* --- LEGACY MACRO ALIASES --- */
#ifndef SYMMETRY_CHANNELS
#define SYMMETRY_CHANNELS CHANNELS_MAX
#endif

typedef enum {
    UR_NEUTRAL = 0x00, // Neutraler Raum-Knoten
    UR_POSITRON_CW = 0x01, // Spin +1/2
    UR_POSITRON_CCW = 0x02, // Spin -1/2
    UR_NEGATRON_CW = 0x03, // Spin +1/2
    UR_NEGATRON_CCW = 0x04, // Spin -1/2
    UR_PHOTON = 0x05  // Energie-Quant / Welle
} ProUrState;

// Ein Knoten im Verschränkungs-Graphen
typedef struct {
    uint8_t type_state;     // Bit-Zustand / Chiralität
    uint8_t field_helicity; // Topologische Feld-Ausrichtung (0 = neutral, >0 = magnetischer Drall)

    /* --- LEGACY COMPATIBILITY --- */
    uint32_t reserved_gating; // Für ältere SDK-Demos
} ProNode;

// Multi-Kanal Pointer-Register (Das Beziehungs-Netzwerk)
// 64-Bit Kanal-Indizes verhindern Type-Mismatch (C4133 / C4477) mit BioAI & ProYori!
typedef struct {
    uint64_t channels[CHANNELS_MAX]; // Zeiger auf Nachbar-Knoten im Graph
} ProRegister;

/* --- LEGACY TYPE ALIASE FÜR PROYORI --- */
typedef ProNode     ProUrNode;
typedef ProRegister ProPointerRegister;

// Das universelle Graph-Netzwerk (Keine Dim-Grenzen, reine Knoten-Anzahl!)
typedef struct {
    uint64_t total_nodes;
    uint64_t dynamic_invariance_target;

    ProNode* ur_grid;      // Knoten-Array (Topologischer Raum)
    ProRegister* reg_source;   // Verschränkungs-Register

    /* --- LEGACY API-ALTLASTEN (ProYori & SDK Demos) --- */
    ProRegister* reg_target;
    uint64_t     current_cpu_tick;
    uint32_t     global_entropy_index;
} ProUniverse;

#endif // PROPHYSICS_TYPES_H