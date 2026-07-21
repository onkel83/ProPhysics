/* ==========================================================================
 * ProPhysics - Pure Topological Physics Kernel Implementation
 * File: ProPhysics.c
 * Architecture: C99, Graph-based Pointer Registers (Zero-Coordinate Core)
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ProPhysics.h"

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

PROPHYSICS_API void ProPhysics_Execute_Tick(ProUniverse* pu) {
    if (!pu || !pu->ur_grid || !pu->reg_source) return;

    // --- 1. FIELD INDUCTION PHASE ---
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        uint8_t state = pu->ur_grid[i].type_state;

        if (state == UR_POSITRON_CW || state == UR_NEGATRON_CW) {
            pu->ur_grid[i].field_helicity = 1;
        }
        else if (state == UR_POSITRON_CCW || state == UR_NEGATRON_CCW) {
            pu->ur_grid[i].field_helicity = 2;
        }

        uint64_t next_node = pu->reg_source[i].channels[0];
        if (next_node < pu->total_nodes && pu->ur_grid[i].field_helicity > 0) {
            pu->ur_grid[next_node].field_helicity = pu->ur_grid[i].field_helicity;
        }
    }

    // --- 2. SHARED-ORBITAL & LORENTZ ROUTING PHASE ---
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        ProRegister* reg = &pu->reg_source[i];
        uint8_t state = pu->ur_grid[i].type_state;

        uint64_t core_a = reg->channels[1];
        uint64_t core_b = reg->channels[2];

        if (core_a != i && core_b != i && core_a != core_b) {
            if (pu->ur_grid[core_a].type_state != UR_NEUTRAL &&
                pu->ur_grid[core_b].type_state != UR_NEUTRAL) {
                reg->channels[0] = (reg->channels[0] == core_a) ? core_b : core_a;
            }
        }

        if (pu->ur_grid[i].field_helicity == 1 && state != UR_NEUTRAL) {
            if (reg->channels[3] != i) {
                uint64_t alt_target = reg->channels[3];
                reg->channels[3] = reg->channels[0];
                reg->channels[0] = alt_target;
            }
        }
    }

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