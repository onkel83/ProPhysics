/* ==========================================================================
 * ProPhysics - Engine Kernel Execution (Pure "It from Bit" LGA Core)
 * Architektur: Lock-Free Gather (Pull) Engine, Pure Array Chunking
 * Optimierung: Zero-Branching Torus & SoA Cache-Density (Flat Flux Array)
 * ========================================================================== */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <intrin.h>
#include "ProPhysics.h"

#ifndef MAX_SPARSE_TRACKING_NODES
#define MAX_SPARSE_TRACKING_NODES 5000000 
#endif

 // ==========================================================================
 // BITSHIFT OPTIMIERUNG FÜR 1024x512x512 GITTER
 // ==========================================================================
#define X_MASK  0x3FF   // 1023 (10 Bits)
#define Y_MASK  0x1FF   // 511  (9 Bits)
#define Z_MASK  0x1FF   // 511  (9 Bits)
#define Y_SHIFT 10      // Start der Y Bits
#define Z_SHIFT 19      // Start der Z Bits (10 + 9)

static const int32_t DX[12] = { 1, -1,  1, -1,  1, -1,  1, -1,  0,  0,  0,  0 };
static const int32_t DY[12] = { 1, -1, -1,  1,  0,  0,  0,  0,  1, -1,  1, -1 };
static const int32_t DZ[12] = { 0,  0,  0,  0,  1, -1, -1,  1,  1, -1, -1,  1 };

static inline uint32_t ProPhysics_Get_Neighbor_Inline(int32_t x, int32_t y, int32_t z, int i) {
    // BRANCHLESS TORUS
    uint32_t nx = (x + DX[i]) & X_MASK;
    uint32_t ny = (y + DY[i]) & Y_MASK;
    uint32_t nz = (z + DZ[i]) & Z_MASK;

    return nx | (ny << Y_SHIFT) | (nz << Z_SHIFT);
}

// ==========================================================================
// MULTITHREADING INFRASTRUKTUR
// ==========================================================================
static volatile long current_tick_phase = 0;
static volatile long workers_done = 0;
static volatile bool engine_running = false;

#define NUM_WORKERS 10
static HANDLE worker_threads[NUM_WORKERS];
static uint32_t* tls_active_nodes[NUM_WORKERS];
static uint64_t tls_active_count[NUM_WORKERS];
static uint8_t* tls_bitset[NUM_WORKERS];
static ProUniverse* global_pu = NULL;
static bool threads_initialized = false;

DWORD WINAPI PhysicsWorker(LPVOID lpParam) {
    int t_id = (int)(uintptr_t)lpParam;

    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << (t_id + 2));
    long my_tick = 1;

    // L3-CACHE OPTIMIERUNG: Wir nutzen active_nodes_kinetic als flaches, reines uint32_t Array!
    // Dadurch schaufelt die CPU nicht mehr die riesigen 64-Byte FCCNode-Strukturen durch den Bus.
    uint32_t* fast_flux = global_pu->active_nodes_kinetic;

    while (engine_running) {
        // ====================================================================
        // PHASE 1: KOLLISION & POSTAUSGANG (SCATTER PREP)
        // ====================================================================
        while (current_tick_phase != (my_tick * 10 + 1)) {
            if (!engine_running) return 0;
            _mm_pause();
        }

        uint64_t total_tasks = global_pu->active_count_current;
        uint64_t chunk_size = total_tasks / NUM_WORKERS;
        uint64_t start = t_id * chunk_size;
        uint64_t end = (t_id == NUM_WORKERS - 1) ? total_tasks : start + chunk_size;

        tls_active_count[t_id] = 0;

        for (uint64_t a = start; a < end; a++) {
            uint32_t idx = global_pu->active_nodes_current[a];
            uint32_t flux = fast_flux[idx]; // <- Ultraschneller Flat-Array Zugriff
            uint32_t move = flux & 0x0FFF;
            uint32_t mass = flux & 0x1000;

            if (move == 0 && mass == 0) {
                fast_flux[idx] = 0;
                global_pu->grid[idx].active_flux = 0; // Legacy-Sync für Observer
                continue;
            }

            if (mass) {
                if (!(tls_bitset[t_id][idx >> 3] & (1 << (idx & 7)))) {
                    tls_bitset[t_id][idx >> 3] |= (1 << (idx & 7));
                    tls_active_nodes[t_id][tls_active_count[t_id]++] = idx;
                }
            }

            if (move) {
                if (__builtin_popcountll(move) == 3 && mass == 0) {
                    fast_flux[idx] = 0x1000;
                    if (!(tls_bitset[t_id][idx >> 3] & (1 << (idx & 7)))) {
                        tls_bitset[t_id][idx >> 3] |= (1 << (idx & 7));
                        tls_active_nodes[t_id][tls_active_count[t_id]++] = idx;
                    }
                    continue;
                }

                bool pair01 = (move & 0x0003) == 0x0003;
                bool pair23 = (move & 0x000C) == 0x000C;
                bool pair45 = (move & 0x0030) == 0x0030;
                bool pair67 = (move & 0x00C0) == 0x00C0;
                bool pair89 = (move & 0x0300) == 0x0300;
                bool pairAB = (move & 0x0C00) == 0x0C00;

                if (pair01 && !(move & 0x0030)) move ^= 0x0033;
                else if (pair45 && !(move & 0x00C0)) move ^= 0x00F0;
                else if (pair67 && !(move & 0x0300)) move ^= 0x03C0;
                else if (pair89 && !(move & 0x0C00)) move ^= 0x0F00;
                else if (pairAB && !(move & 0x000C)) move ^= 0x0C0C;
                else if (pair23 && !(move & 0x0003)) move ^= 0x000F;

                fast_flux[idx] = mass | (move << 13);

                int32_t x = idx & X_MASK;
                int32_t y = (idx >> Y_SHIFT) & Y_MASK;
                int32_t z = idx >> Z_SHIFT;

                for (int i = 0; i < 12; i++) {
                    if (move & (1U << i)) {
                        uint32_t n = ProPhysics_Get_Neighbor_Inline(x, y, z, i);
                        if (!(tls_bitset[t_id][n >> 3] & (1 << (n & 7)))) {
                            tls_bitset[t_id][n >> 3] |= (1 << (n & 7));
                            if (tls_active_count[t_id] < MAX_SPARSE_TRACKING_NODES) {
                                tls_active_nodes[t_id][tls_active_count[t_id]++] = n;
                            }
                        }
                    }
                }
            }
            else {
                fast_flux[idx] = mass;
            }
        }

        _InterlockedIncrement(&workers_done);

        // ====================================================================
        // PHASE 2: GATHER (Mit 16-fach verbesserter Cache-Dichte!)
        // ====================================================================
        while (current_tick_phase != (my_tick * 10 + 2)) {
            if (!engine_running) return 0;
            _mm_pause();
        }

        uint64_t total_tasks_next = global_pu->active_count_next;
        uint64_t chunk_size_next = total_tasks_next / NUM_WORKERS;
        uint64_t start_next = t_id * chunk_size_next;
        uint64_t end_next = (t_id == NUM_WORKERS - 1) ? total_tasks_next : start_next + chunk_size_next;

        for (uint64_t a = start_next; a < end_next; a++) {
            uint32_t n = global_pu->active_nodes_next[a];
            uint32_t flux = fast_flux[n];
            uint32_t incoming = 0;

            int32_t x = n & X_MASK;
            int32_t y = (n >> Y_SHIFT) & Y_MASK;
            int32_t z = n >> Z_SHIFT;

            for (int i = 0; i < 12; i++) {
                uint32_t neighbor = ProPhysics_Get_Neighbor_Inline(x, y, z, i ^ 1);
                // Durch den Zugriff auf das Flat-Array sinkt die L3-Miss Rate dramatisch ab
                if (fast_flux[neighbor] & (1U << (i + 13))) {
                    incoming |= (1U << i);
                }
            }
            fast_flux[n] = (flux & 0xFFFFE000) | (flux & 0x1000) | incoming;
        }

        _InterlockedIncrement(&workers_done);

        // ====================================================================
        // PHASE 3: CLEANUP & LEGACY SYNC
        // ====================================================================
        while (current_tick_phase != (my_tick * 10 + 3)) {
            if (!engine_running) return 0;
            _mm_pause();
        }

        for (uint64_t a = start; a < end; a++) {
            uint32_t idx = global_pu->active_nodes_current[a];
            fast_flux[idx] &= 0x00001FFF;
            global_pu->grid[idx].active_flux = fast_flux[idx]; // Wir schreiben es für das Teleskop in main.c zurück!
        }

        _InterlockedIncrement(&workers_done);
        my_tick++;
    }
    return 0;
}

void ProPhysics_Init_Threads(ProUniverse* pu) {
    if (threads_initialized) return;
    global_pu = pu;
    engine_running = true;
    current_tick_phase = 0;

    uint64_t bitset_bytes = (PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX + 7) / 8;

    for (int i = 0; i < NUM_WORKERS; i++) {
        tls_active_nodes[i] = (uint32_t*)malloc(MAX_SPARSE_TRACKING_NODES * sizeof(uint32_t));
        tls_bitset[i] = (uint8_t*)malloc(bitset_bytes);
        memset(tls_bitset[i], 0, bitset_bytes);
        worker_threads[i] = CreateThread(NULL, 0, PhysicsWorker, (LPVOID)(uintptr_t)i, 0, NULL);
    }
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << 1);
    threads_initialized = true;
}

void ProPhysics_Initialize(ProUniverse* pu) {
    if (!pu) return;
    uint64_t total_nodes = (uint64_t)PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX;
    uint64_t topo_bytes = total_nodes * sizeof(FCCNode);
    uint64_t bitset_bytes = (total_nodes + 7) / 8;

    if (!pu->grid) pu->grid = (FCCNode*)malloc(topo_bytes);
    if (!pu->active_nodes_kinetic) pu->active_nodes_kinetic = (uint32_t*)malloc(total_nodes * sizeof(uint32_t));
    if (!pu->node_active_bitset) pu->node_active_bitset = (uint8_t*)malloc(bitset_bytes);
    if (!pu->active_nodes_current) pu->active_nodes_current = (uint32_t*)malloc(MAX_SPARSE_TRACKING_NODES * sizeof(uint32_t));
    if (!pu->active_nodes_next) pu->active_nodes_next = (uint32_t*)malloc(MAX_SPARSE_TRACKING_NODES * sizeof(uint32_t));

    memset(pu->grid, 0, topo_bytes);
    memset(pu->active_nodes_kinetic, 0, total_nodes * sizeof(uint32_t));
    memset(pu->node_active_bitset, 0, bitset_bytes);

    pu->active_count_current = 0;
    pu->current_cpu_tick = 0;
    ProPhysics_Init_Threads(pu);
}

void ProPhysics_Inject_Elements(ProUniverse* pu) {
    uint64_t total_nodes = (uint64_t)PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX;
    uint32_t* fast_flux = pu->active_nodes_kinetic;
    srand(1337);

    for (uint64_t i = 0; i < total_nodes; i++) {
        if ((rand() % 1000) < 5) {
            fast_flux[i] = (1U << (rand() % 12));
            pu->grid[i].active_flux = fast_flux[i];
            if (pu->active_count_current < MAX_SPARSE_TRACKING_NODES) {
                pu->active_nodes_current[pu->active_count_current++] = (uint32_t)i;
                pu->node_active_bitset[i >> 3] |= (1 << (i & 7));
            }
        }
    }

    int radius = 10;
    int cx_A = 256, cx_B = 768;
    int cy = 256, cz = 256;

    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            for (int z = -radius; z <= radius; z++) {
                if (x * x + y * y + z * z <= radius * radius) {
                    uint32_t idx_A = FCC_INDEX(cx_A + x, cy + y, cz + z);
                    fast_flux[idx_A] |= 0x1000;
                    pu->grid[idx_A].active_flux = fast_flux[idx_A];
                    if (!(pu->node_active_bitset[idx_A >> 3] & (1 << (idx_A & 7)))) {
                        pu->node_active_bitset[idx_A >> 3] |= (1 << (idx_A & 7));
                        if (pu->active_count_current < MAX_SPARSE_TRACKING_NODES)
                            pu->active_nodes_current[pu->active_count_current++] = idx_A;
                    }

                    uint32_t idx_B = FCC_INDEX(cx_B + x, cy + y, cz + z);
                    fast_flux[idx_B] |= 0x1000;
                    pu->grid[idx_B].active_flux = fast_flux[idx_B];
                    if (!(pu->node_active_bitset[idx_B >> 3] & (1 << (idx_B & 7)))) {
                        pu->node_active_bitset[idx_B >> 3] |= (1 << (idx_B & 7));
                        if (pu->active_count_current < MAX_SPARSE_TRACKING_NODES)
                            pu->active_nodes_current[pu->active_count_current++] = idx_B;
                    }
                }
            }
        }
    }
}

void ProPhysics_Execute_Tick(ProUniverse* pu) {
    if (!pu || !pu->grid || pu->active_count_current == 0) return;
    pu->current_cpu_tick++;

    long expected_tick = (long)pu->current_cpu_tick;
    LARGE_INTEGER freq, t_start, t_p1, t_cons, t_p2, t_p3;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t_start);

    _InterlockedExchange(&workers_done, 0);
    _InterlockedExchange(&current_tick_phase, expected_tick * 10 + 1);
    while (workers_done < NUM_WORKERS) _mm_pause();
    QueryPerformanceCounter(&t_p1);

    uint64_t total_nodes = (uint64_t)PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX;
    memset(pu->node_active_bitset, 0, (total_nodes + 7) / 8);
    pu->active_count_next = 0;
    for (int i = 0; i < NUM_WORKERS; i++) {
        for (uint64_t j = 0; j < tls_active_count[i]; j++) {
            uint32_t n = tls_active_nodes[i][j];
            uint64_t byte_idx = n >> 3;
            uint8_t mask = 1 << (n & 7);
            tls_bitset[i][byte_idx] = 0;
            if (!(pu->node_active_bitset[byte_idx] & mask)) {
                pu->node_active_bitset[byte_idx] |= mask;
                if (pu->active_count_next < MAX_SPARSE_TRACKING_NODES)
                    pu->active_nodes_next[pu->active_count_next++] = n;
            }
        }
    }
    QueryPerformanceCounter(&t_cons);

    _InterlockedExchange(&workers_done, 0);
    _InterlockedExchange(&current_tick_phase, expected_tick * 10 + 2);
    while (workers_done < NUM_WORKERS) _mm_pause();
    QueryPerformanceCounter(&t_p2);

    _InterlockedExchange(&workers_done, 0);
    _InterlockedExchange(&current_tick_phase, expected_tick * 10 + 3);
    while (workers_done < NUM_WORKERS) _mm_pause();
    QueryPerformanceCounter(&t_p3);

    uint32_t* temp = pu->active_nodes_current;
    pu->active_nodes_current = pu->active_nodes_next;
    pu->active_nodes_next = temp;
    pu->active_count_current = pu->active_count_next;

    if (expected_tick == 1 || expected_tick == 20) {
        printf("\n[DEBUG TICK %ld] Ph1: %.2fms | Cons: %.2fms | Ph2: %.2fms | Ph3: %.2fms\n",
            expected_tick, (double)(t_p1.QuadPart - t_start.QuadPart) * 1000.0 / freq.QuadPart,
            (double)(t_cons.QuadPart - t_p1.QuadPart) * 1000.0 / freq.QuadPart,
            (double)(t_p2.QuadPart - t_cons.QuadPart) * 1000.0 / freq.QuadPart,
            (double)(t_p3.QuadPart - t_p2.QuadPart) * 1000.0 / freq.QuadPart);
    }
}

void ProPhysics_Update_Observer(ProUniverse* pu, uint64_t expected_initial_bits) {
    if (!pu || !pu->grid || expected_initial_bits == 0) return;

    double com_x = 0, com_y = 0, com_z = 0;
    int64_t net_momentum = 0;
    uint64_t total_bits = 0;
    uint32_t* fast_flux = pu->active_nodes_kinetic;

    for (uint64_t i = 0; i < pu->active_count_current; i++) {
        uint32_t idx = pu->active_nodes_current[i];
        uint32_t flux = fast_flux[idx] & 0x1FFF;

        int32_t x = idx & X_MASK;
        int32_t y = (idx >> Y_SHIFT) & Y_MASK;
        int32_t z = idx >> Z_SHIFT;

        for (int ch = 0; ch < 12; ch++) {
            if (flux & (1U << ch)) {
                com_x += x; com_y += y; com_z += z;
                net_momentum += DX[ch];
                total_bits++;
            }
        }

        if (flux & 0x1000) {
            com_x += x * 3;
            com_y += y * 3;
            com_z += z * 3;
            total_bits += 3;
        }
    }

    if (total_bits > 0) {
        pu->observer_field_vx = com_x / total_bits;
        pu->observer_field_vy = com_y / total_bits;
        pu->observer_field_vz = com_z / total_bits;
    }
    pu->global_entropy_index = (double)net_momentum;
}

bool ProPhysics_Verify_Invariance(const ProUniverse* pu, uint64_t expected_initial_bits) {
    uint64_t current_bit_sum = 0;
    uint32_t* fast_flux = pu->active_nodes_kinetic;

    for (uint64_t a = 0; a < pu->active_count_current; a++) {
        uint32_t idx = pu->active_nodes_current[a];
        uint32_t flux = fast_flux[idx];

        current_bit_sum += (uint64_t)__builtin_popcountll(flux & 0x0FFF);
        if (flux & 0x1000) current_bit_sum += 3;
    }

    return (current_bit_sum == expected_initial_bits);
}

void ProPhysics_Reset(ProUniverse* pu, uint64_t* initial_bit_tracker) {
    if (!pu || pu->is_hardened || !initial_bit_tracker) return;

    ProPhysics_Initialize(pu);
    ProPhysics_Inject_Elements(pu);

    *initial_bit_tracker = 0;
    uint64_t total_nodes = (uint64_t)PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX;
    uint32_t* fast_flux = pu->active_nodes_kinetic;

    for (uint64_t i = 0; i < total_nodes; i++) {
        uint32_t flux = fast_flux[i];
        if (flux) {
            *initial_bit_tracker += (uint64_t)__builtin_popcountll(flux & 0x0FFF);
            if (flux & 0x1000) *initial_bit_tracker += 3;
        }
    }
    pu->dynamic_invariance_target = *initial_bit_tracker;
}

double ProPhysics_Query_Anisotropy(const ProUniverse* pu, int32_t x, int32_t y, int32_t z) { (void)pu; (void)x; (void)y; (void)z; return 0.0; }