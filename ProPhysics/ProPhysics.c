/* ==========================================================================
 * ProPhysics - Engine Kernel Execution (Pure "It from Bit" LGA Core)
 * Architektur: Lock-Free Gather (Pull) Engine, Pure Array Chunking
 * Optimierung: Zero-Branching Torus & SoA Cache-Density (Flat Flux Array)
 * Instrumentierung: Forensisches Phasen- & Thread-Debugging
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

 // --- VORWÄRTSDEKLARATIONEN (Verhindert C4013 & C2371) ---
void ProPhysics_Update_Quantum_Flux(ProUniverse* universe, uint32_t node_idx);

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

    printf("[WORKER %d] Online und an Core gebunden. Engine-Status: %d\n", t_id, engine_running);

    uint32_t* fast_flux = global_pu->active_nodes_kinetic;

    while (engine_running) {
        // ====================================================================
        // PHASE 1: KOLLISION & POSTAUSGANG (SCATTER PREP) + QUANTUM FLUX
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

            // --- Synchronisation & Paralleler Quanten-Ablenkungs-Kernel ---
            global_pu->grid[idx].active_flux = fast_flux[idx];
            ProPhysics_Update_Quantum_Flux(global_pu, idx);
            fast_flux[idx] = global_pu->grid[idx].active_flux;

            uint32_t flux = fast_flux[idx];
            uint32_t move = flux & 0x0FFF;
            uint32_t mass = flux & 0x1000;

            if (move == 0 && mass == 0) {
                fast_flux[idx] = 0;
                global_pu->grid[idx].active_flux = 0;
                continue;
            }

            if (mass) {
                if (!(tls_bitset[t_id][idx >> 3] & (1 << (idx & 7)))) {
                    tls_bitset[t_id][idx >> 3] |= (1 << (idx & 7));
                    tls_active_nodes[t_id][tls_active_count[t_id]++] = idx;
                }
            }

            if (move) {
                // --- Polaritätsabhängiges Fusions-Gating (Korrektur: uint32_t) ---
                uint32_t island_idx = global_pu->grid[idx].state_island_idx;

                uint64_t local_charge = (island_idx != 0) ?
                    (global_pu->data_pool[island_idx].charge_spin & QUANTUM_MASK_POLARITY) : QUANTUM_POL_NEUTRAL;

                // Axiom: Fusion ist nur in einem aktiv polarisierten Feld erlaubt
                uint32_t fusion_allowed = (local_charge == QUANTUM_POL_PLUS) | (local_charge == QUANTUM_POL_MINUS);

                if (__builtin_popcountll(move) == 3 && mass == 0 && fusion_allowed) {
                    fast_flux[idx] = 0x1000;

                    // Neue Masse erbt den Feld-Katalysator
                    global_pu->grid[idx].state_island_idx = island_idx;

                    if (!(tls_bitset[t_id][idx >> 3] & (1 << (idx & 7)))) {
                        tls_bitset[t_id][idx >> 3] |= (1 << (idx & 7));
                        tls_active_nodes[t_id][tls_active_count[t_id]++] = idx;
                    }
                    continue;
                }

                // ============================================================
                // PAAR-STREUUNG (LEGACY LGA)
                // ============================================================
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
        // PHASE 2: GATHER
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
            global_pu->grid[idx].active_flux = fast_flux[idx];
        }

        _InterlockedIncrement(&workers_done);
        my_tick++;
    }
    return 0;
}

void ProPhysics_Init_Threads(ProUniverse* pu) {
    if (threads_initialized) return;
    printf("[INIT] Starte Thread-Infrastruktur für %d Worker...\n", NUM_WORKERS);
    global_pu = pu;
    engine_running = true;
    current_tick_phase = 0;

    uint64_t bitset_bytes = (PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX + 7) / 8;

    for (int i = 0; i < NUM_WORKERS; i++) {
        tls_active_nodes[i] = (uint32_t*)malloc(MAX_SPARSE_TRACKING_NODES * sizeof(uint32_t));
        tls_bitset[i] = (uint8_t*)malloc(bitset_bytes);
        memset(tls_bitset[i], 0, bitset_bytes);
        worker_threads[i] = CreateThread(NULL, 0, PhysicsWorker, (LPVOID)(uintptr_t)i, 0, NULL);
        if (worker_threads[i] == NULL) {
            printf("[CRITICAL] Fehler beim Spawnen von Worker %d!\n", i);
        }
    }
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << 1);
    threads_initialized = true;
    printf("[INIT] Alle Worker erfolgreich initialisiert und im Standby.\n");
}

void ProPhysics_Initialize(ProUniverse* pu) {
    if (!pu) return;
    printf("[INITIALIZE] Allokiere Gitterstrukturen...\n");
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
    printf("[INJECTOR] Flute das Vakuum mit Quantenrauschen...\n");
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

    printf("[INJECTOR] Stanze Planet A und Planet B in den RAM...\n");
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

    long last_seen_done = -1;
    uint64_t watchdog = 0;
    while (workers_done < NUM_WORKERS) {
        long current_done = workers_done;
        if (current_done != last_seen_done) {
            last_seen_done = current_done;
        }
        _mm_pause();
        if (++watchdog > 500000000ULL) {
            printf("[CRITICAL DEADLOCK] Hauptthread hängt in Phase 1! workers_done ist blockiert bei %ld.\n", workers_done);
            watchdog = 0;
            Sleep(1000);
        }
    }
    QueryPerformanceCounter(&t_p1);

    // ====================================================================
    // --- CONSOLIDATION STEP (MAIN THREAD SANCTUARY - NOW WITH MASS MIGRATION) ---
    // ====================================================================
    uint64_t total_nodes = (uint64_t)PROPHYSICS_X_MAX * PROPHYSICS_Y_MAX * PROPHYSICS_Z_MAX;
    memset(pu->node_active_bitset, 0, (total_nodes + 7) / 8);
    pu->active_count_next = 0;

    for (int i = 0; i < NUM_WORKERS; i++) {
        for (uint64_t j = 0; j < tls_active_count[i]; j++) {
            uint32_t n = tls_active_nodes[i][j];
            uint32_t original_n = n;

            // --- INERTIAL MASS MIGRATION ENGINE ---
            uint32_t flux = pu->active_nodes_kinetic[n];
            if (flux & 0x1000) { // Enthält dieser Netzknoten träge Ruhemasse?
                uint32_t island_idx = pu->grid[n].state_island_idx;
                if (island_idx != 0) {
                    QuantumStateIsland* island = &pu->data_pool[island_idx];
                    int64_t vx = island->vx;
                    int64_t vy = island->vy;
                    int64_t vz = island->vz;

                    int move_ch = -1;
                    int64_t threshold = 16; // Trägheits-Schranke (Je höher, desto träger/schwerer die Planeten)

                    // Prüfe, ob der akkumulierte Vektor eine der 12 FCC-Richtungen triggert
                    for (int ch = 0; ch < 12; ch++) {
                        int64_t proj = vx * DX[ch] + vy * DY[ch] + vz * DZ[ch];
                        if (proj >= threshold) {
                            move_ch = ch;
                            break; // Richtungs-Vektor für diesen Takt fixiert
                        }
                    }

                    if (move_ch != -1) {
                        int32_t x = n & X_MASK;
                        int32_t y = (n >> Y_SHIFT) & Y_MASK;
                        int32_t z = n >> Z_SHIFT;
                        uint32_t target_n = ProPhysics_Get_Neighbor_Inline(x, y, z, move_ch);

                        // Pauli-Exklusion: Masse darf nur wandern, wenn das Ziel frei von schwerer Masse ist
                        if (!(pu->active_nodes_kinetic[target_n] & 0x1000)) {
                            // 1. Alten Knoten evakuieren
                            pu->active_nodes_kinetic[n] &= ~0x1000;
                            pu->grid[n].active_flux &= ~0x1000;
                            pu->grid[n].state_island_idx = 0;

                            // 2. Auf Zielknoten einstanzen
                            pu->active_nodes_kinetic[target_n] |= 0x1000;
                            pu->grid[target_n].active_flux |= 0x1000;
                            pu->grid[target_n].state_island_idx = island_idx;

                            // 3. IMPULSERHALTUNG: Ziehe die verbrauchte Bewegungsenergie ab
                            island->vx -= DX[move_ch] * threshold;
                            island->vy -= DY[move_ch] * threshold;
                            island->vz -= DZ[move_ch] * threshold;

                            // Verschiebe die Zeiger-Referenz für das nachfolgende Bitset-Gating
                            n = target_n;
                        }
                    }
                }
            }

            // Transienten Bitset-Postausgang des Workers sauber zurücksetzen
            uint64_t orig_byte = original_n >> 3;
            tls_bitset[i][orig_byte] = 0;

            // Globales Horizont-Gating für Phase 2 (Gather) einbrennen
            uint64_t byte_idx = n >> 3;
            uint8_t mask = 1 << (n & 7);
            if (!(pu->node_active_bitset[byte_idx] & mask)) {
                pu->node_active_bitset[byte_idx] |= mask;
                if (pu->active_count_next < MAX_SPARSE_TRACKING_NODES)
                    pu->active_nodes_next[pu->active_count_next++] = n;
            }
        }
    }
    QueryPerformanceCounter(&t_cons);

    _InterlockedExchange(&workers_done, 0);
    _InterlockedExchange(&current_tick_phase, expected_tick * 10 + 0x2);
    while (workers_done < NUM_WORKERS) {
        _mm_pause();
    }
    QueryPerformanceCounter(&t_p2);

    _InterlockedExchange(&workers_done, 0);
    _InterlockedExchange(&current_tick_phase, expected_tick * 10 + 0x3);
    while (workers_done < NUM_WORKERS) {
        _mm_pause();
    }
    QueryPerformanceCounter(&t_p3);

    uint32_t* temp = pu->active_nodes_current;
    pu->active_nodes_current = pu->active_nodes_next;
    pu->active_nodes_next = temp;
    pu->active_count_current = pu->active_count_next;
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

void ProPhysics_Update_Quantum_Flux(ProUniverse* universe, uint32_t node_idx) {
    FCCNode* node = &universe->grid[node_idx];
    uint32_t island_idx = node->state_island_idx;

    uint64_t polarity = QUANTUM_POL_NEUTRAL;
    QuantumStateIsland* island = NULL;

    if (island_idx != 0) {
        island = &universe->data_pool[island_idx];
        polarity = island->charge_spin & QUANTUM_MASK_POLARITY;
    }

    uint32_t mass_bit = node->active_flux & 0x1000;
    uint32_t flux = node->active_flux & 0xFFF;
    uint32_t updated_flux = 0;

    int32_t x = node_idx & X_MASK;
    int32_t y = (node_idx >> Y_SHIFT) & Y_MASK;
    int32_t z = node_idx >> Z_SHIFT;

    // Wir verarbeiten die 12 Kanäle paarweise als 6 Raumachsen (Bijectivity-Gating)
    for (int p = 0; p < 6; p++) {
        int i = p * 2;
        int j = i + 1; // Der physikalische Gegenkanal (i ^ 1)

        uint32_t bit_i = (flux >> i) & 1U;
        uint32_t bit_j = (flux >> j) & 1U;

        uint32_t repel_i = 0;
        uint32_t repel_j = 0;

        // Achsen-Fluss I evaluieren
        if (bit_i) {
            uint32_t n_idx = ProPhysics_Get_Neighbor_Inline(x, y, z, i);
            uint32_t n_island = universe->grid[n_idx].state_island_idx;
            uint64_t n_pol = (n_island != 0) ? (universe->data_pool[n_island].charge_spin & QUANTUM_MASK_POLARITY) : QUANTUM_POL_NEUTRAL;

            repel_i = (polarity == n_pol) & (polarity != QUANTUM_POL_NEUTRAL);

            if ((polarity != n_pol) & ((polarity != QUANTUM_POL_NEUTRAL) | (n_pol != QUANTUM_POL_NEUTRAL))) {
                uint64_t next_charge = (polarity + n_pol) * ((polarity + n_pol) != 3);
                if (island_idx == 0 && n_island != 0) {
                    node->state_island_idx = n_island;
                    island_idx = n_island;
                    island = &universe->data_pool[island_idx];
                }
                if (island_idx != 0) {
                    island->charge_spin = (island->charge_spin & ~QUANTUM_MASK_POLARITY) | next_charge;
                    polarity = next_charge;
                }
            }
        }

        // Achsen-Fluss J evaluieren
        if (bit_j) {
            uint32_t n_idx = ProPhysics_Get_Neighbor_Inline(x, y, z, j);
            uint32_t n_island = universe->grid[n_idx].state_island_idx;
            uint64_t n_pol = (n_island != 0) ? (universe->data_pool[n_island].charge_spin & QUANTUM_MASK_POLARITY) : QUANTUM_POL_NEUTRAL;

            repel_j = (polarity == n_pol) & (polarity != QUANTUM_POL_NEUTRAL);

            if ((polarity != n_pol) & ((polarity != QUANTUM_POL_NEUTRAL) | (n_pol != QUANTUM_POL_NEUTRAL))) {
                uint64_t next_charge = (polarity + n_pol) * ((polarity + n_pol) != 3);
                if (island_idx == 0 && n_island != 0) {
                    node->state_island_idx = n_island;
                    island_idx = n_island;
                    island = &universe->data_pool[island_idx];
                }
                if (island_idx != 0) {
                    island->charge_spin = (island->charge_spin & ~QUANTUM_MASK_POLARITY) | next_charge;
                    polarity = next_charge;
                }
            }
        }

        // BIJEKTIVER TAUSCH: Löst das Bit-Clobbering-Problem vollständig
        // Wenn mindestens ein Kanal der Achse blockiert ist, zwingt er beide Kanäle zur Reflexion
        uint32_t do_swap = repel_i | repel_j;

        updated_flux |= (do_swap) ? ((bit_i << j) | (bit_j << i)) : ((bit_i << i) | (bit_j << j));
    }

    // --- SPIN-CHIRALITÄT ---
    if (island_idx != 0) {
        uint32_t spin_chiral = (uint32_t)((island->charge_spin & QUANTUM_MASK_SPIN_CHIRAL) >> 2);
        uint32_t is_spinning = (spin_chiral != QUANTUM_SPIN_NONE);
        uint32_t shift = is_spinning * ((spin_chiral == QUANTUM_SPIN_CW) ? 1 : 11);

        updated_flux = ((updated_flux << shift) | (updated_flux >> (12 - shift))) & 0xFFF;

        island->vx += (int64_t)((updated_flux & 0x1) - ((updated_flux >> 1) & 0x1));
        island->vy += (int64_t)(((updated_flux >> 2) & 0x1) - ((updated_flux >> 3) & 0x1));
        island->vz += (int64_t)(((updated_flux >> 4) & 0x1) - ((updated_flux >> 5) & 0x1));
    }

    node->active_flux = updated_flux | mass_bit;
}