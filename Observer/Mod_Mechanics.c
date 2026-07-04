/* ==========================================================================
 * ProPhysics - Mechanics Diagnostic Core
 * File: Mod_Mechanics.c
 * Architecture: C99, Cache-Optimized Monolithic Evaluator Layer
 * Optimierung: Redundancy Elimination, O(1) Loop-Merging, Inline Bit-Masking
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

 // SDK-Schnittstellen
#include "ProPhysics.h"
#include "ProDiBatch.h"

// Lokale Konfigurations-Axiome
#include "Observer_Version.h"
#include "Observer_Config.h"
#include "../ProPhysics/ProPhysics_Types.h"
#include "../ProDiBatch/ProDiBatch_Exports.h"

extern ModuleTestControl global_mod_control[];

/* Interner Speicher für die Invarianten-Prüfung über Ticks hinweg */
static int64_t last_total_px = 0;
static int64_t last_total_py = 0;
static int64_t last_total_pz = 0;
static int64_t last_total_Lx = 0;
static int64_t last_total_Ly = 0;
static int64_t last_total_Lz = 0;

/* Historische Kinematik-Werte für Translation (v, a, j) */
static double last_com_x = 0.0;
static double last_com_y = 0.0;
static double last_com_z = 0.0;
static double last_v_x = 0.0;
static double last_v_y = 0.0;
static double last_v_z = 0.0;
static double last_a_x = 0.0;
static double last_a_y = 0.0;
static double last_a_z = 0.0;

/* Historische Rotations-Werte */
static double last_omega_x = 0.0;
static double last_omega_y = 0.0;
static double last_omega_z = 0.0;
static double last_alpha_x = 0.0;
static double last_alpha_y = 0.0;
static double last_alpha_z = 0.0;

/* Historische Kinetik-Werte */
static uint64_t last_energy_kin = 0;
static double last_center_pressure = 0.0;

static bool first_tick = true;

#define KINEMATIC_EPSILON 0.0001

/* Globale Akkumulatoren für den integrierten Formelprüfstand */
static double total_work_done = 0.0;
static double current_launch_angle = 0.0;
static double current_lever_delta = 0.0;

// Schnellzugriff auf Gitter-Bitmasken aus der Core-Engine
#define UNPACK_X(idx) ((int32_t)((idx) & 0x3FF))
#define UNPACK_Y(idx) ((int32_t)(((idx) >> 10) & 0x1FF))
#define UNPACK_Z(idx) ((int32_t)((idx) >> 19))

/* ==========================================================================
 * DIAGNOSE 1: Kinetik-Profiler (Kraftstoss, Arbeit, Leistung & Energie)
 * ========================================================================== */
static void check_kinetics_energy_work_power(const ProUniverse* universe, ProDiBatch_Engine* db_engine, uint64_t current_energy_kin) {
    if (first_tick) return;

    total_work_done = 0.0;
    double total_impulse_x = 0.0, total_impulse_y = 0.0, total_impulse_z = 0.0;
    uint64_t dynamic_nodes = 0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                int32_t x = UNPACK_X(idx);
                int32_t y = UNPACK_Y(idx);
                int32_t z = UNPACK_Z(idx);

                double P_c = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z, 1);
                double F_x = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + 1, y, z, 1);
                double F_y = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y + 1, z, 1);
                double F_z = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z + 1, 1);

                double vx = (double)universe->data_pool[slot].vx;
                double vy = (double)universe->data_pool[slot].vy;
                double vz = (double)universe->data_pool[slot].vz;

                total_impulse_x += F_x; total_impulse_y += F_y; total_impulse_z += F_z;
                total_work_done += (F_x * vx + F_y * vy + F_z * vz);

                dynamic_nodes += ((fabs(vx) > 0.0) || (fabs(vy) > 0.0) || (fabs(vz) > 0.0));
            }
        }
    }

    double power_eff = total_work_done;
    int64_t delta_E_kin = (int64_t)current_energy_kin - (int64_t)last_energy_kin;

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0) {
        ProDiBatch_Log(db_engine, "[KINETICS] E_kin: %llu (Delta: %lld) | Arbeit W: %.4f | Leistung P: %.4f",
            current_energy_kin, delta_E_kin, total_work_done, power_eff);
    }
#endif

    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && fabs(total_work_done) > 0.1 && delta_E_kin == 0) {
        ProDiBatch_Log(db_engine, "[WARN][KINETICS] Arbeit verrichtet, aber keine kinetische Energieaenderung!");
    }
}

/* ==========================================================================
 * DIAGNOSE 2: Wurfdynamik-Profiler (Waagerechter vs. Schraeger Wurf)
 * ========================================================================== */
static void check_projectile_motion(ProDiBatch_Engine* db_engine, double vx, double vy, double vz, double az) {
    double v_horiz = sqrt(vx * vx + vy * vy);
    current_launch_angle = 0.0;

    if (v_horiz > KINEMATIC_EPSILON && fabs(az) > KINEMATIC_EPSILON) {
        if (fabs(vz) <= KINEMATIC_EPSILON) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0) {
                ProDiBatch_Log(db_engine, "[BALLISTICS] Typ: WAAGERECHTER WURF | v_horiz: %.4f", v_horiz);
            }
#endif
        }
        else {
            current_launch_angle = atan2(vz, v_horiz) * (180.0 / 3.1415926535);
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
            if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0) {
                ProDiBatch_Log(db_engine, "[BALLISTICS] Typ: SCHRAEGER WURF | Winkel: %.1f Grad", current_launch_angle);
            }
#endif
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 3: Rotationskinematik (Gleichfoermig, Gleichmaessig, Ansatz)
 * ========================================================================== */
static void check_rotational_kinematics(ProDiBatch_Engine* db_engine, int64_t Lx, int64_t Ly, int64_t Lz, double moment_of_inertia) {
    if (first_tick || moment_of_inertia <= KINEMATIC_EPSILON) return;

    double omega_x = (double)Lx / moment_of_inertia;
    double omega_y = (double)Ly / moment_of_inertia;
    double omega_z = (double)Lz / moment_of_inertia;

    double alpha_x = omega_x - last_omega_x; double alpha_y = omega_y - last_omega_y; double alpha_z = omega_z - last_omega_z;
    double rot_jerk_x = alpha_x - last_alpha_x; double rot_jerk_y = alpha_y - last_alpha_y; double rot_jerk_z = alpha_z - last_alpha_z;

    double abs_omega = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
    double abs_alpha = sqrt(alpha_x * alpha_x + alpha_y * alpha_y + alpha_z * alpha_z);
    double abs_jerk = sqrt(rot_jerk_x * rot_jerk_x + rot_jerk_y * rot_jerk_y + rot_jerk_z * rot_jerk_z);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && abs_omega > KINEMATIC_EPSILON) {
        if (abs_alpha <= KINEMATIC_EPSILON) {
            ProDiBatch_Log(db_engine, "[ROTATION] Typ: GLEICHFOERMIGE ROTATION | Omega: %.4f", abs_omega);
        }
        else if (abs_alpha > KINEMATIC_EPSILON && abs_jerk <= KINEMATIC_EPSILON) {
            ProDiBatch_Log(db_engine, "[ROTATION] Typ: GLEICHMAESSIG BESCHLEUNIGT | Alpha: %.4f", abs_alpha);
        }
        else if (abs_jerk > KINEMATIC_EPSILON) {
            ProDiBatch_Log(db_engine, "[ROTATION] Typ: UNGLEICHMAESSIG BESCHLEUNIGT | Ruck: %.4f", abs_jerk);
        }
    }
#endif
    last_omega_x = omega_x; last_omega_y = omega_y; last_omega_z = omega_z;
    last_alpha_x = alpha_x; last_alpha_y = alpha_y; last_alpha_z = alpha_z;
}

/* ==========================================================================
 * DIAGNOSE 4: Krummlinige Bahnen & Zentralbeschleunigung
 * ========================================================================== */
static void check_centripetal_mechanics(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t curved_path_nodes = 0;
    double avg_centripetal_accel = 0.0;

    for (uint64_t i = 0; i < universe->active_count_current; i += 5) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                double vx = (double)universe->data_pool[slot].vx;
                double vy = (double)universe->data_pool[slot].vy;
                double vz = (double)universe->data_pool[slot].vz;
                double abs_v = sqrt(vx * vx + vy * vy + vz * vz);

                if (abs_v > 0.1) {
                    int32_t x = UNPACK_X(idx);
                    int32_t y = UNPACK_Y(idx);
                    int32_t z = UNPACK_Z(idx);

                    double P_c = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z, 1);
                    double ax = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + 1, y, z, 1);
                    double ay = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y + 1, z, 1);
                    double az = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z + 1, 1);

                    double dot_product = vx * ax + vy * ay + vz * az;
                    double abs_a = sqrt(ax * ax + ay * ay + az * az);

                    if (abs_a > 0.05 && fabs(dot_product) / (abs_v * abs_a) < 0.3) {
                        avg_centripetal_accel += abs_a;
                        curved_path_nodes++;
                    }
                }
            }
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && curved_path_nodes > 0) {
        ProDiBatch_Log(db_engine, "[CURVED_PATH] Umfangsbewegung aktiv! Kreis-Nodes: %llu | Zentral-a: %.4f", curved_path_nodes, avg_centripetal_accel / (double)curved_path_nodes);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 5: Translations-Profiler (Kinematische Ableitungen)
 * ========================================================================== */
static void check_translational_kinematics(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double current_com_x, double current_com_y, double current_com_z) {
    if (first_tick) return;
    (void)universe;

    double v_eff_x = current_com_x - last_com_x; double v_eff_y = current_com_y - last_com_y; double v_eff_z = current_com_z - last_com_z;
    double a_eff_x = v_eff_x - last_v_x; double a_eff_y = v_eff_y - last_v_y; double a_eff_z = v_eff_z - last_v_z;
    double j_eff_x = a_eff_x - last_a_x; double j_eff_y = a_eff_y - last_a_y; double j_eff_z = a_eff_z - last_a_z;

    double abs_v = sqrt(v_eff_x * v_eff_x + v_eff_y * v_eff_y + v_eff_z * v_eff_z);
    double abs_a = sqrt(a_eff_x * a_eff_x + a_eff_y * a_eff_y + a_eff_z * a_eff_z);
    double abs_j = sqrt(j_eff_x * j_eff_x + j_eff_y * j_eff_y + j_eff_z * j_eff_z);

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && abs_v > KINEMATIC_EPSILON) {
        if (abs_a <= KINEMATIC_EPSILON) ProDiBatch_Log(db_engine, "[KINEMATICS] Typ: Gleichfoermige Translation | v: %.4f", abs_v);
        else if (abs_a > KINEMATIC_EPSILON && abs_j <= KINEMATIC_EPSILON) ProDiBatch_Log(db_engine, "[KINEMATICS] Typ: Gleichmaessig beschleunigt | a: %.4f", abs_a);
        else if (abs_j > KINEMATIC_EPSILON) ProDiBatch_Log(db_engine, "[KINEMATICS] Typ: Ungleichmaessig beschleunigt | Ruck: %.4f", abs_j);
    }
#endif
    check_projectile_motion(db_engine, v_eff_x, v_eff_y, v_eff_z, a_eff_z);
    last_v_x = v_eff_x; last_v_y = v_eff_y; last_v_z = v_eff_z;
    last_a_x = a_eff_x; last_a_y = a_eff_y; last_a_z = a_eff_z;
}

/* ==========================================================================
 * DIAGNOSE 6: Standfestigkeit & Kippstabilität
 * ========================================================================== */
static void check_stability_and_tipping(const ProUniverse* universe, ProDiBatch_Engine* db_engine, double com_x, double com_y) {
    int min_x = PROPHYSICS_X_MAX; int max_x = 0; int min_y = PROPHYSICS_Y_MAX; int max_y = 0; uint64_t base_nodes = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            int32_t x = UNPACK_X(idx);
            int32_t y = UNPACK_Y(idx);
            int32_t z = UNPACK_Z(idx);

            if (z <= 1) {
                if (x < min_x) min_x = x; if (x > max_x) max_x = x;
                if (y < min_y) min_y = y; if (y > max_y) max_y = y;
                base_nodes++;
            }
        }
    }
    if (base_nodes > 0 && min_x < max_x && min_y < max_y) {
        if (com_x - min_x < 0.0 || max_x - com_x < 0.0 || com_y - min_y < 0.0 || max_y - com_y < 0.0) {
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_WARN
            if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0) {
                ProDiBatch_Log(db_engine, "[WARN][STABILITY] KIPPMOMENT ERREICHT! Schwerpunkt ausserhalb.");
            }
#endif
        }
    }
}

/* ==========================================================================
 * DIAGNOSE 7: Das Hebelgesetz & Einfache Maschinen (M1 * s1 = M2 * s2)
 * ========================================================================== */
static void check_lever_and_machinery(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int center_x = PROPHYSICS_X_MAX / 2;
    int64_t mass_left = 0, coord_x_left = 0;
    int64_t mass_right = 0, coord_x_right = 0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            int32_t x = UNPACK_X(idx);
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                uint64_t mass = universe->data_pool[slot].mass_accumulator;
                if (x < center_x) { mass_left += (int64_t)mass; coord_x_left += (int64_t)x * (int64_t)mass; }
                else { mass_right += (int64_t)mass; coord_x_right += (int64_t)x * (int64_t)mass; }
            }
        }
    }

    if (mass_left > 0 && mass_right > 0) {
        double com_left = (double)coord_x_left / (double)mass_left;
        double com_right = (double)coord_x_right / (double)mass_right;
        double global_barycenter_x = (double)(coord_x_left + coord_x_right) / (double)(mass_left + mass_right);

        current_lever_delta = ((double)mass_left * (global_barycenter_x - com_left)) - ((double)mass_right * (com_right - global_barycenter_x));

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0) {
            ProDiBatch_Log(db_engine, "[LEVER] Lastarm L: %.2f | Kraftarm R: %.2f | Hebel-Delta: %.4f",
                global_barycenter_x - com_left, com_right - global_barycenter_x, current_lever_delta);
        }
#endif
    }
}

/* ==========================================================================
 * DIAGNOSE 8: Schwingungen & Wellenlehre (Druckfluktuationen)
 * ========================================================================== */
static void check_waves_and_oscillations(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int center_x = PROPHYSICS_X_MAX / 2; int center_y = PROPHYSICS_Y_MAX / 2; int center_z = PROPHYSICS_Z_MAX / 2;
    double current_center_pressure = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, center_x, center_y, center_z, 4);

    if (!first_tick) {
        double pressure_delta = current_center_pressure - last_center_pressure;
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
        if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && fabs(pressure_delta) > 0.005) {
            ProDiBatch_Log(db_engine, "[WAVES] Longitudinalpuls detektiert! P_Center: %.4f", current_center_pressure);
        }
#endif
    }
    last_center_pressure = current_center_pressure;
}

/* ==========================================================================
 * DIAGNOSE 9: Strömungsmechanik & Hydrodynamischer Widerstand
 * ========================================================================== */
static void check_fluid_dynamics_and_drag(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    uint64_t drag_events = 0;
    double avg_drag_force = 0.0;

    for (uint64_t i = 0; i < universe->active_count_current; i += 10) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                double vx = (double)universe->data_pool[slot].vx;
                double vy = (double)universe->data_pool[slot].vy;
                double vz = (double)universe->data_pool[slot].vz;
                double abs_v = sqrt(vx * vx + vy * vy + vz * vz);

                if (abs_v > 0.5) {
                    int32_t x = UNPACK_X(idx);
                    int32_t y = UNPACK_Y(idx);
                    int32_t z = UNPACK_Z(idx);

                    int32_t front_x = x + (vx > 0 ? 1 : (vx < 0 ? -1 : 0));
                    int32_t front_y = y + (vy > 0 ? 1 : (vy < 0 ? -1 : 0));
                    int32_t front_z = z + (vz > 0 ? 1 : (vz < 0 ? -1 : 0));

                    double P_front = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, front_x, front_y, front_z, 1);
                    if (P_front > 1.05) {
                        avg_drag_force += (P_front - 1.0);
                        drag_events++;
                    }
                }
            }
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && drag_events > 0) {
        ProDiBatch_Log(db_engine, "[FLUIDS] Vakuum-Viskositaet aktiv. Mittlerer Staudruck: %.4f", avg_drag_force / (double)drag_events);
    }
#endif
}

/* ==========================================================================
 * DIAGNOSE 10: Elastomechanik & Festigkeit (Bounding-Box-Strain)
 * ========================================================================== */
static void check_elastomechanics_and_deformation(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int min_x = PROPHYSICS_X_MAX, max_x = 0;
    int min_y = PROPHYSICS_Y_MAX, max_y = 0;
    int min_z = PROPHYSICS_Z_MAX, max_z = 0;
    uint64_t total_matter_nodes = 0;

    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            int32_t x = UNPACK_X(idx);
            int32_t y = UNPACK_Y(idx);
            int32_t z = UNPACK_Z(idx);

            if (x < min_x) min_x = x; if (x > max_x) max_x = x;
            if (y < min_y) min_y = y; if (y > max_y) max_y = y;
            if (z < min_z) min_z = z; if (z > max_z) max_z = z;
            total_matter_nodes++;
        }
    }

#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && total_matter_nodes > 5) {
        double size_x = (double)(max_x - min_x);
        double size_y = (double)(max_y - min_y);
        double size_z = (double)(max_z - min_z);
        ProDiBatch_Log(db_engine, "[ELASTO] Materie-Huelle Volumen: %.2f", size_x * size_y * size_z);
    }
#endif
}

static void check_inclined_plane_and_wedges(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    double total_slope_force_x = 0.0; double total_normal_force_y = 0.0; uint64_t sampled_nodes = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            int32_t x = UNPACK_X(idx); int32_t y = UNPACK_Y(idx); int32_t z = UNPACK_Z(idx);
            double P_c = ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y, z, 1);
            double F_x = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x + 1, y, z, 1);
            double F_y = P_c - ProPhysics_Query_Local_Pressure((ProUniverse*)universe, x, y + 1, z, 1);
            if (fabs(F_x) > 0.1 && fabs(F_y) > 0.1) { total_slope_force_x += F_x; total_normal_force_y += F_y; sampled_nodes++; }
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && sampled_nodes > 0) {
        ProDiBatch_Log(db_engine, "[PLANE/SCREW] Kraftaufspaltung (tan alpha): %.4f", total_slope_force_x / total_normal_force_y);
    }
#endif
}

static void check_pulleys_and_tackles(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    int center_x = PROPHYSICS_X_MAX / 2; int64_t momentum_in = 0; int64_t momentum_out = 0;
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                int32_t x = UNPACK_X(idx);
                int64_t pz = (int64_t)universe->data_pool[slot].vz * (int64_t)universe->data_pool[slot].mass_accumulator;
                if (x < center_x) momentum_in += pz; else momentum_out += pz;
            }
        }
    }
#if OBSERVER_CURRENT_LOG_LEVEL >= OBSERVER_LOG_INFO
    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 && momentum_in != 0 && momentum_out != 0) {
        ProDiBatch_Log(db_engine, "[PULLEY/TACKLE] Uebersetzung: %.4f", (double)labs(momentum_out) / (double)labs(momentum_in));
    }
#endif
}

static void check_newton_first_law(const ProUniverse* universe, ProDiBatch_Engine* db_engine, int64_t current_px, int64_t current_py, int64_t current_pz) {
    if (first_tick) return;

    if (global_mod_control[MOD_INDEX_MECHANICS].active_test_id == 0 &&
        (current_px != last_total_px || current_py != last_total_py || current_pz != last_total_pz)) {
        ProDiBatch_Log(db_engine, "[WARN][INVARIANCE] Impulserhaltung gestoert!");
    }
}

/* ==========================================================================
 * DIAGNOSE 11: RUNTIME MECHANICAL FORMULA VALIDATOR LAB
 * ========================================================================== */
static void execute_interactive_mechanics_lab(const ProUniverse* universe, ProDiBatch_Engine* db_engine, uint64_t current_energy_kin) {
    uint32_t test_id = global_mod_control[MOD_INDEX_MECHANICS].active_test_id;
    double expected_input = global_mod_control[MOD_INDEX_MECHANICS].target_intensity;

    if (test_id == 0) return;
    (void)universe;

    ProDiBatch_Log(db_engine, "[LAB-MECHANICS] === MECHANIK VALIDATOR IM LIVE-BETRIEB (TEST %u) ===", test_id);

    if (test_id == 1) {
        int64_t actual_delta_E = (int64_t)current_energy_kin - (int64_t)last_energy_kin;
        double calculation_error = total_work_done - (double)actual_delta_E;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Arbeit-Energie-Theorem");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Druck-Arbeit W_ist: %.4f | Realer E_kin-Wechsel: %lld", total_work_done, actual_delta_E);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Absoluter Abweichungsfehler: %.5f (%s)",
            calculation_error, (fabs(calculation_error) < 0.1) ? "ENERGIEERHALTUNG OK" : "PHYSIKDRIFT");
    }
    else if (test_id == 2) {
        double target_angle = expected_input;
        double angle_error = current_launch_angle - target_angle;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Ballistische Trajektorien-Analyse");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Soll-Winkel: %.2f Grad | Gemessener Ist-Flugwinkel: %.2f Grad", target_angle, current_launch_angle);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Winkel-Abweichung: %.4f Grad", angle_error);
    }
    else if (test_id == 3) {
        double expected_delta = expected_input;
        double error_lever = current_lever_delta - expected_delta;

        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Modell: Drehmoment-Gleichgewicht am Zentralbaryzentrum");
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Erwartetes Drehmoment-Delta: %.4f | Reales Drehmoment-Delta: %.4f", expected_delta, current_lever_delta);
        ProDiBatch_Log(db_engine, "[FORMULA_CHECK] Hebelgesetz-Konvergenzfehler: %.5f", error_lever);
    }
}

/* ==========================================================================
 * Haupt-Einstiegspunkt für das Mechanik-Modul
 * ========================================================================== */
void observer_mod_mechanics_evaluate(const ProUniverse* universe, ProDiBatch_Engine* db_engine) {
    if (!universe || !db_engine) return;

    uint64_t total_mass = 0;
    int64_t com_x_sum = 0; int64_t com_y_sum = 0; int64_t com_z_sum = 0;
    int64_t current_px = 0; int64_t current_py = 0; int64_t current_pz = 0;
    double moment_of_inertia = 0.0;
    uint64_t current_energy_kin = 0;

    // --- ONE SWEEP TO RULE THEM ALL ---
    // Konsolidiert alle globalen Lineartranslationen und Impulserhaltungen in einem Durchlauf
    for (uint64_t i = 0; i < universe->active_count_current; i++) {
        uint32_t idx = universe->active_nodes_current[i];
        if (universe->active_nodes_kinetic[idx] & 0x1000) {
            uint32_t slot = universe->grid[idx].state_island_idx;
            if (slot != 0) {
                uint64_t mass = universe->data_pool[slot].mass_accumulator;
                total_mass += mass;

                com_x_sum += (int64_t)UNPACK_X(idx) * (int64_t)mass;
                com_y_sum += (int64_t)UNPACK_Y(idx) * (int64_t)mass;
                com_z_sum += (int64_t)UNPACK_Z(idx) * (int64_t)mass;

                int64_t vx = universe->data_pool[slot].vx;
                int64_t vy = universe->data_pool[slot].vy;
                int64_t vz = universe->data_pool[slot].vz;

                current_px += vx * (int64_t)mass;
                current_py += vy * (int64_t)mass;
                current_pz += vz * (int64_t)mass;

                current_energy_kin += (uint64_t)(vx * vx + vy * vy + vz * vz) * mass;
            }
        }
    }

    if (total_mass > 0) {
        double current_com_x = (double)com_x_sum / (double)total_mass;
        double current_com_y = (double)com_y_sum / (double)total_mass;
        double current_com_z = (double)com_z_sum / (double)total_mass;

        int64_t current_Lx = 0; int64_t current_Ly = 0; int64_t current_Lz = 0;

        for (uint64_t i = 0; i < universe->active_count_current; i++) {
            uint32_t idx = universe->active_nodes_current[i];
            if (universe->active_nodes_kinetic[idx] & 0x1000) {
                uint32_t slot = universe->grid[idx].state_island_idx;
                if (slot != 0) {
                    uint64_t mass = universe->data_pool[slot].mass_accumulator;
                    double rx = (double)UNPACK_X(idx) - current_com_x;
                    double ry = (double)UNPACK_Y(idx) - current_com_y;
                    double rz = (double)UNPACK_Z(idx) - current_com_z;

                    moment_of_inertia += (double)mass * (rx * rx + ry * ry + rz * rz);

                    current_Lx += (int64_t)(ry * ((double)universe->data_pool[slot].vz * (double)mass) - rz * ((double)universe->data_pool[slot].vy * (double)mass));
                    current_Ly += (int64_t)(rz * ((double)universe->data_pool[slot].vx * (double)mass) - rx * ((double)universe->data_pool[slot].vz * (double)mass));
                    current_Lz += (int64_t)(rx * ((double)universe->data_pool[slot].vy * (double)mass) - ry * ((double)universe->data_pool[slot].vx * (double)mass));
                }
            }
        }

        check_kinetics_energy_work_power(universe, db_engine, current_energy_kin);
        check_translational_kinematics(universe, db_engine, current_com_x, current_com_y, current_com_z);
        check_rotational_kinematics(db_engine, current_Lx, current_Ly, current_Lz, moment_of_inertia);
        check_centripetal_mechanics(universe, db_engine);
        check_stability_and_tipping(universe, db_engine, current_com_x, current_com_y);

        last_com_x = current_com_x; last_com_y = current_com_y; last_com_z = current_com_z;
        last_total_Lx = current_Lx; last_total_Ly = current_Ly; last_total_Lz = current_Lz;
    }

    check_lever_and_machinery(universe, db_engine);
    check_waves_and_oscillations(universe, db_engine);
    check_fluid_dynamics_and_drag(universe, db_engine);
    check_elastomechanics_and_deformation(universe, db_engine);
    check_inclined_plane_and_wedges(universe, db_engine);
    check_pulleys_and_tackles(universe, db_engine);

    // Übergabe der vorab akkumulierten Impulswerte an Newtons Invarianz-Prüfer
    check_newton_first_law(universe, db_engine, current_px, current_py, current_pz);

    execute_interactive_mechanics_lab(universe, db_engine, current_energy_kin);

    // --- REVOLUTIONÄR: DIE ZWEITE $O(N)$-SCHLEIFE WURDE RESTLOS ELIMINIERT ---
    // Übergabe erfolgt direkt ohne redundanten Hauptspeicher-Sweep
    last_energy_kin = current_energy_kin;
    last_total_px = current_px;
    last_total_py = current_py;
    last_total_pz = current_pz;

    first_tick = false;
}