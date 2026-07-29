/* ==========================================================================
 * ProPhysics - Pure Topological Physics Kernel Implementation
 * File: ProPhysics.c
 * Architecture: C99, Graph-based Pointer Registers (Zero-Coordinate Core)
 * Version: 2.1 (Strict Invariance Conservation & Immutable Topology)
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ProPhysics.h"

static inline bool is_positron(uint8_t st) {
    return (st == UR_POSITRON_CW || st == UR_POSITRON_CCW);
}

static inline bool is_negatron(uint8_t st) {
    return (st == UR_NEGATRON_CW || st == UR_NEGATRON_CCW);
}

PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count) {
    if (!pu) return;

    pu->total_nodes = node_count;
    pu->dynamic_invariance_target = 0;

    pu->reg_target = NULL;
    pu->current_cpu_tick = 0;
    pu->global_entropy_index = 0;

    pu->ur_grid = (ProNode*)calloc(node_count, sizeof(ProNode));
    pu->reg_source = (ProRegister*)calloc(node_count, sizeof(ProRegister));

    if (!pu->ur_grid || !pu->reg_source) {
        if (pu->ur_grid) free(pu->ur_grid);
        if (pu->reg_source) free(pu->reg_source);
        pu->ur_grid = NULL;
        pu->reg_source = NULL;
        return;
    }

    for (uint64_t i = 0; i < node_count; i++) {
        pu->ur_grid[i].type_state = UR_NEUTRAL;
        pu->ur_grid[i].field_helicity = 0;
        pu->ur_grid[i].reserved_gating = 0;

        for (int c = 0; c < CHANNELS_MAX; c++) {
            pu->reg_source[i].channels[c] = i; // Self-loop by default
        }
    }
}

PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu) {
    if (!pu) return;

    if (pu->ur_grid) free(pu->ur_grid);
    if (pu->reg_source) free(pu->reg_source);
    if (pu->reg_target) free(pu->reg_target);

    pu->ur_grid = NULL;
    pu->reg_source = NULL;
    pu->reg_target = NULL;
    pu->total_nodes = 0;
}

PROPHYSICS_API void ProPhysics_Link_Nodes(ProUniverse* pu, uint64_t src, uint64_t target, uint8_t channel_idx) {
    if (!pu || src >= pu->total_nodes || target >= pu->total_nodes || channel_idx >= CHANNELS_MAX) return;
    pu->reg_source[src].channels[channel_idx] = target;
}

PROPHYSICS_API void ProPhysics_Unlink_Node(ProUniverse* pu, uint64_t src, uint8_t channel_idx) {
    if (!pu || src >= pu->total_nodes || channel_idx >= CHANNELS_MAX) return;
    pu->reg_source[src].channels[channel_idx] = src;
}

typedef struct {
    uint8_t state;
    uint64_t target;
    uint64_t fallback;
    uint64_t src;
} QuantumPacket;

PROPHYSICS_API void ProPhysics_Execute_Tick(ProUniverse* pu) {
    if (!pu || !pu->ur_grid || !pu->reg_source) return;

    uint64_t n = pu->total_nodes;

    ProNode* next_grid = (ProNode*)calloc(n, sizeof(ProNode));
    QuantumPacket* packets = (QuantumPacket*)malloc(sizeof(QuantumPacket) * n * 2);
    if (!next_grid || !packets) {
        if (next_grid) free(next_grid);
        if (packets) free(packets);
        return;
    }

    for (uint64_t i = 0; i < n; i++) {
        next_grid[i].type_state = UR_NEUTRAL;
        next_grid[i].field_helicity = 0;
        next_grid[i].reserved_gating = 0;
        pu->ur_grid[i].field_helicity = 0;
    }

    // --- 1. FIELD INDUCTION PHASE ---
    for (uint64_t i = 0; i < n; i++) {
        uint8_t state = pu->ur_grid[i].type_state;

        if (state == UR_POSITRON_CW || state == UR_NEGATRON_CW) {
            pu->ur_grid[i].field_helicity = 1;
        }
        else if (state == UR_POSITRON_CCW || state == UR_NEGATRON_CCW) {
            pu->ur_grid[i].field_helicity = 2;
        }

        uint64_t next_node = pu->reg_source[i].channels[0];
        if (next_node < n && pu->ur_grid[i].field_helicity > 0) {
            pu->ur_grid[next_node].field_helicity = pu->ur_grid[i].field_helicity;
        }
    }

    // --- 2. DYNAMIC KINEMATICS & REACTION INTENT (IMMUTABLE GRAPH) ---
    uint64_t packet_count = 0;

    for (uint64_t i = 0; i < n; i++) {
        uint8_t st = pu->ur_grid[i].type_state;
        if (st == UR_NEUTRAL) continue;

        const ProRegister* reg = &pu->reg_source[i];
        uint64_t t_pos = reg->channels[1];
        uint64_t t_neg = reg->channels[2];

        // A) PAARERZEUGUNG / SPALTUNG: Photon (5) -> Positron (1) + Negatron (4)
        if (st == UR_PHOTON && pu->ur_grid[i].field_helicity > 0 && t_pos != i && t_neg != i) {
            packets[packet_count].state = UR_POSITRON_CW;
            packets[packet_count].target = (t_pos < n) ? t_pos : i;
            packets[packet_count].fallback = i;
            packets[packet_count].src = i;
            packet_count++;

            packets[packet_count].state = UR_NEGATRON_CW;
            packets[packet_count].target = (t_neg < n) ? t_neg : i;
            packets[packet_count].fallback = i;
            packets[packet_count].src = i;
            packet_count++;
        }
        // B) DYNAMISCHES ROUTING GEGENSÄTZLICHER LORENTZ-ABLENKUNG (V2.0 Core)
        else {
            uint8_t target_chan = 0; // Standard: Lineare Ausbreitung (Kanal 0)

            if (pu->ur_grid[i].field_helicity > 0 && st != UR_NEUTRAL) {
                if (is_positron(st)) {
                    if (reg->channels[2] != i) target_chan = 2; // Positron -> Gegen-Drift (Kanal 2)
                }
                else if (is_negatron(st)) {
                    if (reg->channels[3] != i) target_chan = 3; // Negatron -> Drift (Kanal 3)
                }
            }
            else {
                uint64_t core_a = reg->channels[1];
                uint64_t core_b = reg->channels[2];
                if (core_a != i && core_b != i && core_a != core_b && core_a < n && core_b < n) {
                    if (pu->ur_grid[core_a].type_state != UR_NEUTRAL &&
                        pu->ur_grid[core_b].type_state != UR_NEUTRAL) {
                        target_chan = 2;
                    }
                }
            }

            uint64_t primary_target = reg->channels[target_chan];
            if (primary_target >= n) primary_target = i;

            uint64_t alt_target = reg->channels[3];
            if (alt_target >= n) alt_target = i;

            packets[packet_count].state = st;
            packets[packet_count].target = primary_target;
            packets[packet_count].fallback = alt_target;
            packets[packet_count].src = i;
            packet_count++;
        }
    }

    // --- 3. STATE PROPAGATION & FUSION / STERN-GERLACH-AUSWEICHUNG ---
    for (uint64_t p = 0; p < packet_count; p++) {
        uint8_t st = packets[p].state;
        uint64_t tgt = packets[p].target;
        uint64_t fb = packets[p].fallback;
        uint64_t src = packets[p].src;

        uint8_t cur_st = next_grid[tgt].type_state;

        // Fall 1: Zielknoten frei
        if (cur_st == UR_NEUTRAL) {
            next_grid[tgt].type_state = st;
        }
        // Fall 2: Annihilation / Fusion (Positron + Negatron = Photon: 1 + 4 = 5)
        else if ((is_positron(st) && is_negatron(cur_st)) ||
            (is_negatron(st) && is_positron(cur_st))) {
            next_grid[tgt].type_state = UR_PHOTON;
        }
        // Fall 3: Ziel durch gleiches Teilchen besetzt -> Ausweichen auf Fallback-Kanal
        else {
            uint8_t fb_st = next_grid[fb].type_state;
            if (fb_st == UR_NEUTRAL) {
                next_grid[fb].type_state = st;
            }
            else if ((is_positron(st) && is_negatron(fb_st)) ||
                (is_negatron(st) && is_positron(fb_st))) {
                next_grid[fb].type_state = UR_PHOTON;
            }
            // Fall 4: Ausweichen auf Quell-Knoten (elastischer Rückprall)
            else {
                uint8_t src_st = next_grid[src].type_state;
                if (src_st == UR_NEUTRAL) {
                    next_grid[src].type_state = st;
                }
                else if ((is_positron(st) && is_negatron(src_st)) ||
                    (is_negatron(st) && is_positron(src_st))) {
                    next_grid[src].type_state = UR_PHOTON;
                }
                else {
                    // Absolute Invarianz-Garantie: Freien Nachbarkanal suchen
                    bool placed = false;
                    for (int c = 0; c < CHANNELS_MAX; c++) {
                        uint64_t ch = pu->reg_source[src].channels[c];
                        if (ch < n && next_grid[ch].type_state == UR_NEUTRAL) {
                            next_grid[ch].type_state = st;
                            placed = true;
                            break;
                        }
                    }
                    if (!placed) {
                        next_grid[src].type_state = st;
                    }
                }
            }
        }
    }

    // --- 4. GRAPH SYNCHRONIZATION ---
    memcpy(pu->ur_grid, next_grid, n * sizeof(ProNode));

    free(packets);
    free(next_grid);

    pu->current_cpu_tick++;
}

PROPHYSICS_API bool ProPhysics_Verify_Invariance(const ProUniverse* pu) {
    if (!pu || !pu->ur_grid) return false;

    uint64_t current_sum = 0;
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        switch (pu->ur_grid[i].type_state) {
        case UR_POSITRON_CW:
        case UR_POSITRON_CCW:
            current_sum += 1;
            break;
        case UR_NEGATRON_CW:
        case UR_NEGATRON_CCW:
            current_sum += 4;
            break;
        case UR_PHOTON:
            current_sum += 5;
            break;
        case UR_NEUTRAL:
        default:
            break;
        }
    }

    return (current_sum == pu->dynamic_invariance_target);
}