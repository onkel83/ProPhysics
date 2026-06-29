/* ==========================================================================

ProPhysics - Standalone Console Bootstrapper (Pure Wheeler Emergence)

File: main.c

Experiment: Le-Sage Gravitation + Materie-Teleskop & Erhaltungs-Beweis

========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "ProPhysics.h"

#ifdef _MSC_VER
#include <intrin.h>
#define ZAEHLE_BITS(x) __popcnt64(x)
#else
#define ZAEHLE_BITS(x) __builtin_popcountll(x)
#endif

void Print_Cosmic_Slice(ProUniverse* pu, int center_x, int center_y, int center_z, int radius) {
	printf("\n[KOSMISCHES TELESKOP] - 2D Schnitt durch Planet A (Z = %d)\n", center_z);
	printf("Legende: [##] = Materie-Kern (Ruhemasse), [..] = Photonen-Strahlung, [  ] = Vakuum\n");
    printf("------------------------------------------------------------------------------------------\n");

    for (int y = center_y + (radius / 2); y >= center_y - (radius / 2); y--) {
        for (int x = center_x - radius; x <= center_x + radius; x++) {
            int px = x < 0 ? PROPHYSICS_X_MAX + x : (x >= PROPHYSICS_X_MAX ? x - PROPHYSICS_X_MAX : x);
            int py = y < 0 ? PROPHYSICS_Y_MAX + y : (y >= PROPHYSICS_Y_MAX ? y - PROPHYSICS_Y_MAX : y);

            uint32_t idx = FCC_INDEX(px, py, center_z);
            // Das Teleskop liest jetzt direkt aus dem schnellen Flat-Array!
            uint32_t flux = pu->active_nodes_kinetic[idx];

            if (flux & 0x1000) {
                printf("##"); // Massive Materie
            }
            else if (flux & 0x0FFF) {
                printf(".."); // Vorbeifliegendes Licht
            }
            else {
                printf("  "); // Pures Vakuum
            }
        }
        printf("\n");
    }
    printf("------------------------------------------------------------------------------------------\n");


}

int main(void) {
    // [Hier würde die Initialisierung der Universe-Struktur folgen...]

    // Output-Drosselung
    if (tick == 1 || tick % 20 == 0 || tick == 500) {

        uint64_t photonen_count = 0;
        uint64_t materie_count = 0;

        for (uint64_t i = 0; i < universe.active_count_current; i++) {
            uint32_t idx = universe.active_nodes_current[i];
            // Auslesen direkt aus dem Flat-Array!
            uint32_t flux = universe.active_nodes_kinetic[idx];

            uint32_t moving_flux = flux & 0x0FFF;
            photonen_count += ZAEHLE_BITS(moving_flux);
            if (flux & 0x1000) materie_count++;
        }
    }

    printf("-------------------------------------------------------------------------------------------------------\n");

    uint64_t finale_photonen = 0;
    uint64_t finale_materie = 0;
    for (uint64_t i = 0; i < universe.active_count_current; i++) {
        // Finale Auswertung liest ebenfalls aus dem Flat-Array!
        uint32_t flux = universe.active_nodes_kinetic[universe.active_nodes_current[i]];
        finale_photonen += ZAEHLE_BITS(flux & 0x0FFF);
        if (flux & 0x1000) finale_materie++;
    }

    printf("\n[EMERGENZ-ANALYSE NACH 500 TICKS]\n");
    Print_Cosmic_Slice(&universe, 256, 256, 256, 30);

    // Wir geben nur noch das Array frei, das Grid existiert nicht mehr.
    free(universe.active_nodes_kinetic);
    return 0;


}