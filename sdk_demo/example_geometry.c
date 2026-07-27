/* ==========================================================================
 * ProPhysics SDK - Example: Parametric 3D Topology & Time-Series Exporter
 * File: example_geometry.c
 * Architecture: 3D Topological Injectors & ParaView PVD Animation Engine
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "ProPhysics.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 // =========================================================================
 // 1. 3D GEOMETRY HELPER FUNCTIONS & STRUCTURES
 // =========================================================================

typedef struct {
    int64_t x;
    int64_t y;
    int64_t z;
} Point3D;

/**
 * Konvertiert 3D-Gitterkoordinaten in einen flachen Index des ProPhysics-Substrats
 */
static inline uint64_t Grid3D_ToIndex(int64_t x, int64_t y, int64_t z, uint64_t width, uint64_t height) {
    return (uint64_t)((z * (int64_t)height + y) * (int64_t)width + x);
}

/**
 * Rekonstruiert 3D-Koordinaten aus einem flachen Knoteninstitution-Index
 */
static inline void Grid3D_IndexTo3D(uint64_t index, uint64_t width, uint64_t height, int64_t* out_x, int64_t* out_y, int64_t* out_z) {
    *out_x = (int64_t)(index % width);
    *out_y = (int64_t)((index / width) % height);
    *out_z = (int64_t)(index / (width * height));
}

// =========================================================================
// 2. PROCEDURAL 3D TOPOLOGY INJECTORS
// =========================================================================

/**
 * Injiziert eine 3D-Kugelschale (Sphärischer Resonator) in den relationalen Graph.
 */
void Geometry_InjectSphereShell(
    ProUniverse* pu,
    uint64_t width, uint64_t height,
    Point3D center,
    double radius,
    uint32_t ring_samples,
    uint8_t initial_state)
{
    if (!pu) return;
    uint64_t depth = pu->total_nodes / (width * height);

    uint32_t count = 0;
    for (uint32_t i = 0; i < ring_samples; i++) {
        double theta = (M_PI * i) / ring_samples; // Breitengrad (0..PI)
        for (uint32_t j = 0; j < ring_samples * 2; j++) {
            double phi = (2.0 * M_PI * j) / (ring_samples * 2); // Längengrad (0..2PI)

            int64_t x = center.x + (int64_t)round(radius * sin(theta) * cos(phi));
            int64_t y = center.y + (int64_t)round(radius * sin(theta) * sin(phi));
            int64_t z = center.z + (int64_t)round(radius * cos(theta));

            if (x < 0 || x >= (int64_t)width ||
                y < 0 || y >= (int64_t)height ||
                z < 0 || z >= (int64_t)depth) continue;

            uint64_t idx = Grid3D_ToIndex(x, y, z, width, height);
            pu->ur_grid[idx].type_state = initial_state;

            // Relationale Verknüpfung zum nächsten Längengrad-Knoten (Channel 0)
            uint32_t next_j = (j + 1) % (ring_samples * 2);
            int64_t nx = center.x + (int64_t)round(radius * sin(theta) * cos((2.0 * M_PI * next_j) / (ring_samples * 2)));
            int64_t ny = center.y + (int64_t)round(radius * sin(theta) * sin((2.0 * M_PI * next_j) / (ring_samples * 2)));

            if (nx >= 0 && nx < (int64_t)width && ny >= 0 && ny < (int64_t)height) {
                uint64_t next_idx = Grid3D_ToIndex(nx, ny, z, width, height);
                ProPhysics_Link_Nodes(pu, idx, next_idx, 0);
            }
            count++;
        }
    }
    printf("[Geometry 3D] Kugelschale injiziert: Zentrum (%lld,%lld,%lld) | Radius %.1f | Knoten: %u\n",
        (long long)center.x, (long long)center.y, (long long)center.z, radius, count);
}

/**
 * Injiziert ein non-orientierbares 3D-Möbiusband (Wheeler-Topologie mit 180° Phasendrehung)
 */
void Geometry_InjectMobiusStrip(
    ProUniverse* pu,
    uint64_t width, uint64_t height,
    Point3D center,
    double major_radius,
    double strip_width,
    uint32_t u_steps, uint32_t v_steps,
    uint8_t initial_state)
{
    if (!pu) return;
    uint64_t depth = pu->total_nodes / (width * height);

    uint64_t prev_row_start = 0;
    uint64_t first_row_start = 0;

    for (uint32_t u_idx = 0; u_idx < u_steps; u_idx++) {
        double u = (2.0 * M_PI * u_idx) / u_steps;

        for (uint32_t v_idx = 0; v_idx < v_steps; v_idx++) {
            double v = -strip_width / 2.0 + (strip_width * v_idx) / (v_steps - 1);

            // Parametrische 3D-Möbius-Transformation
            double x_rel = (major_radius + v * cos(u / 2.0)) * cos(u);
            double y_rel = (major_radius + v * cos(u / 2.0)) * sin(u);
            double z_rel = v * sin(u / 2.0);

            int64_t x = center.x + (int64_t)round(x_rel);
            int64_t y = center.y + (int64_t)round(y_rel);
            int64_t z = center.z + (int64_t)round(z_rel);

            if (x < 0 || x >= (int64_t)width ||
                y < 0 || y >= (int64_t)height ||
                z < 0 || z >= (int64_t)depth) continue;

            uint64_t current_idx = Grid3D_ToIndex(x, y, z, width, height);
            pu->ur_grid[current_idx].type_state = initial_state;

            // Längsverknüpfung entlang des Bands (Channel 0)
            if (u_idx > 0) {
                // Verknüpfe mit vorherigem u_step
                double prev_u = (2.0 * M_PI * (u_idx - 1)) / u_steps;
                double px = center.x + (major_radius + v * cos(prev_u / 2.0)) * cos(prev_u);
                double py = center.y + (major_radius + v * cos(prev_u / 2.0)) * sin(prev_u);
                double pz = center.z + v * sin(prev_u / 2.0);

                uint64_t prev_idx = Grid3D_ToIndex((int64_t)round(px), (int64_t)round(py), (int64_t)round(pz), width, height);
                ProPhysics_Link_Nodes(pu, prev_idx, current_idx, 0);
            }
        }
    }
    printf("[Geometry 3D] Möbius-Band injiziert: Radius %.1f | Schritte: %ux%u\n",
        major_radius, u_steps, v_steps);
}

// =========================================================================
// 3. PARAVIEW VTK & PVD TIME-SERIES EXPORTER
// =========================================================================

/**
 * Exportiert den aktuellen Simulations-Snapshot als ASCII VTK-Datei (Single Frame)
 */
bool Geometry_ExportVTK_Frame(const ProUniverse* pu, uint64_t width, uint64_t height, const char* filename) {
    if (!pu || !filename) return false;

    FILE* fp = fopen(filename, "w");
    if (!fp) return false;

    uint64_t active_count = 0;
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        if (pu->ur_grid[i].type_state > UR_NEUTRAL) active_count++;
    }

    fprintf(fp, "# vtk DataFile Version 3.0\nProPhysics 3D Snapshot\nASCII\nDATASET POLYDATA\n");
    fprintf(fp, "POINTS %llu float\n", (unsigned long long)active_count);

    uint64_t* node_to_point = (uint64_t*)malloc(pu->total_nodes * sizeof(uint64_t));
    memset(node_to_point, 0xFF, pu->total_nodes * sizeof(uint64_t));

    uint64_t pt_id = 0;
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        if (pu->ur_grid[i].type_state > UR_NEUTRAL) {
            int64_t x, y, z;
            Grid3D_IndexTo3D(i, width, height, &x, &y, &z);
            fprintf(fp, "%lld.0 %lld.0 %lld.0\n", (long long)x, (long long)y, (long long)z);
            node_to_point[i] = pt_id++;
        }
    }

    // Topologische Pointer-Lines exportieren
    uint64_t valid_links = 0;
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        if (pu->ur_grid[i].type_state > UR_NEUTRAL) {
            uint64_t target = pu->reg_source[i].channels[0];
            if (target < pu->total_nodes && node_to_point[target] != (uint64_t)-1) {
                valid_links++;
            }
        }
    }

    fprintf(fp, "\nLINES %llu %llu\n", (unsigned long long)valid_links, (unsigned long long)(valid_links * 3));
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        if (pu->ur_grid[i].type_state > UR_NEUTRAL) {
            uint64_t target = pu->reg_source[i].channels[0];
            if (target < pu->total_nodes && node_to_point[target] != (uint64_t)-1) {
                fprintf(fp, "2 %llu %llu\n", (unsigned long long)node_to_point[i], (unsigned long long)node_to_point[target]);
            }
        }
    }

    // Ur-States & Helizität als Farbfelder
    fprintf(fp, "\nPOINT_DATA %llu\n", (unsigned long long)active_count);
    fprintf(fp, "SCALARS UrState unsigned_char 1\nLOOKUP_TABLE default\n");
    for (uint64_t i = 0; i < pu->total_nodes; i++) {
        if (pu->ur_grid[i].type_state > UR_NEUTRAL) {
            fprintf(fp, "%u\n", pu->ur_grid[i].type_state);
        }
    }

    free(node_to_point);
    fclose(fp);
    return true;
}

/**
 * Erzeugt die ParaView Master Index-Datei (.pvd) für flüssige 3D-Zeitserien-Animationen
 */
bool Geometry_WritePVDIndex(const char* pvd_filename, const char* frame_prefix, uint32_t total_frames) {
    FILE* fp = fopen(pvd_filename, "w");
    if (!fp) return false;

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n");
    fprintf(fp, "  <Collection>\n");

    for (uint32_t frame = 0; frame < total_frames; frame++) {
        fprintf(fp, "    <DataSet timestep=\"%u\" group=\"\" part=\"0\" file=\"%s_%04u.vtk\"/>\n",
            frame, frame_prefix, frame);
    }

    fprintf(fp, "  </Collection>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
    printf("[TimeSeries] ParaView Master-Index '%s' mit %u Frames erzeugt.\n", pvd_filename, total_frames);
    return true;
}

// =========================================================================
// 4. MAIN EXECUTABLE (3D TOPOLOGY BENCHMARK & TIME-SERIES)
// =========================================================================

int main(void) {
    printf("================================================================================\n");
    printf("     PROPHYSICS SDK: 3D TOPOLOGY & WHEELER TIME-SERIES BENCHMARK\n");
    printf("================================================================================\n");

    ProUniverse pu;
    uint64_t width = 200;
    uint64_t height = 200;
    uint64_t depth = 200;
    uint64_t total_nodes = width * height * depth; // 8 Millionen 3D-Gitterknoten

    ProPhysics_Initialize(&pu, total_nodes);

    printf("[*] Substrat allokiert (%llu Knoten). Injiziere 3D-Topologien...\n\n", (unsigned long long)total_nodes);

    // 1. Injiziere Sphärische Resonanz-Schale (Ur-Photon / Positron-Zustand)
    Point3D sphere_center = { 50, 100, 100 };
    Geometry_InjectSphereShell(&pu, width, height, sphere_center, 30.0, 32, UR_POSITRON_CW);

    // 2. Injiziere 3D-Möbius-Band (Verdrehte Raum-Topologie)
    Point3D mobius_center = { 150, 100, 100 };
    Geometry_InjectMobiusStrip(&pu, width, height, mobius_center, 25.0, 10.0, 64, 16, UR_PHOTON);

    printf("\n[*] Starte Simulation & Zeitserien-Export (60 Ticks)...\n");
    printf("--------------------------------------------------------------------------------\n");

    const uint32_t total_ticks = 60;
    char frame_name[256];

    for (uint32_t tick = 0; tick < total_ticks; tick++) {
        // Kern-Physikschritt nach den 5 Ur-Regeln ausführen
        ProPhysics_Execute_Tick(&pu);

        // Frame exportieren
        snprintf(frame_name, sizeof(frame_name), "frame_%04u.vtk", tick);
        Geometry_ExportVTK_Frame(&pu, width, height, frame_name);

        if (tick % 10 == 0 || tick == total_ticks - 1) {
            printf("  -> Tick #%02u verarbeitet | Frame '%s' geschrieben.\n", tick, frame_name);
        }
    }

    // ParaView .pvd Masterdatei schreiben
    Geometry_WritePVDIndex("simulation_3d.pvd", "frame", total_ticks);

    printf("--------------------------------------------------------------------------------\n");
    printf("[*] Testlauf erfolgreich beendet. ParaView-Datei 'simulation_3d.pvd' laden.\n");
    ProPhysics_Free(&pu);
    return 0;
}