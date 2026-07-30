/**
 * @file example_Periode_Interactive.c
 * @brief ProPhysics SDK Demo: Interaktiver Element-Compiler mit 2D-Orbital-Visualisierer
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#define CLEAR_SCREEN() system("cls")
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#define CLEAR_SCREEN() printf("\033[H\033[J")
#endif

#include "ProPhysics.h"
#include "ProPhysics_Types.h"

#define DEMO_GRID_SIZE 8192
#define CANVAS_W       65
#define CANVAS_H       25
#define CENTER_X       32
#define CENTER_Y       12

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    uint8_t Z;               // Ordnungszahl
    uint8_t N;               // Neutronen
    char symbol[4];          // Chemisches Symbol
    char name[32];           // Name
    uint8_t shells[7];       // K, L, M, N, O, P, Q
} ElementDefinition;

/**
 * Erstellt die Element-Spezifikation für Z = 1..118.
 */
ElementDefinition ProPhysics_Get_Element(uint8_t Z) {
    ElementDefinition elem;
    memset(&elem, 0, sizeof(ElementDefinition));

    if (Z < 1) Z = 1;
    if (Z > 118) Z = 118;

    elem.Z = Z;
    elem.N = (Z <= 20) ? Z : (uint8_t)(Z * 1.35f);

    switch (Z) {
    case 1:  strcpy(elem.symbol, "H");  strcpy(elem.name, "Wasserstoff"); break;
    case 2:  strcpy(elem.symbol, "He"); strcpy(elem.name, "Helium"); break;
    case 3:  strcpy(elem.symbol, "Li"); strcpy(elem.name, "Lithium"); break;
    case 6:  strcpy(elem.symbol, "C");  strcpy(elem.name, "Kohlenstoff"); break;
    case 7:  strcpy(elem.symbol, "N");  strcpy(elem.name, "Stickstoff"); break;
    case 8:  strcpy(elem.symbol, "O");  strcpy(elem.name, "Sauerstoff"); break;
    case 10: strcpy(elem.symbol, "Ne"); strcpy(elem.name, "Neon"); break;
    case 11: strcpy(elem.symbol, "Na"); strcpy(elem.name, "Natrium"); break;
    case 14: strcpy(elem.symbol, "Si"); strcpy(elem.name, "Silizium"); break;
    case 26: strcpy(elem.symbol, "Fe"); strcpy(elem.name, "Eisen"); break;
    case 29: strcpy(elem.symbol, "Cu"); strcpy(elem.name, "Kupfer"); break;
    case 47: strcpy(elem.symbol, "Ag"); strcpy(elem.name, "Silber"); break;
    case 79: strcpy(elem.symbol, "Au"); strcpy(elem.name, "Gold"); break;
    case 92: strcpy(elem.symbol, "U");  strcpy(elem.name, "Uran"); break;
    default: snprintf(elem.symbol, sizeof(elem.symbol), "E%u", Z);
        snprintf(elem.name, sizeof(elem.name), "Element_%u", Z); break;
    }

    uint8_t max_capacities[7] = { 2, 8, 18, 32, 32, 18, 8 };
    uint8_t remaining = Z;

    for (int i = 0; i < 7 && remaining > 0; i++) {
        uint8_t fill = (remaining > max_capacities[i]) ? max_capacities[i] : remaining;
        elem.shells[i] = fill;
        remaining -= fill;
    }

    return elem;
}

/**
 * Rendert das Atom mit seinen Orbitalschalen in 2D ASCII Grafik.
 */
void Render_Atom_2D(ProUniverse* pu, ElementDefinition* elem, uint32_t tick) {
    char canvas[CANVAS_H][CANVAS_W];

    // Canvas leeren
    for (int y = 0; y < CANVAS_H; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            canvas[y][x] = ' ';
        }
        canvas[y][CANVAS_W - 1] = '\0';
    }

    // Concentric Shell Radii (X/Y Korrektur für Konsolen-Aspekt-Verhältnis ~2:1)
    float radii_x[7] = { 4.0f, 7.0f, 10.0f, 13.0f, 16.0f, 19.0f, 22.0f };
    float radii_y[7] = { 2.0f, 3.5f,  5.0f,  6.5f,  8.0f,  9.5f, 11.0f };

    // 1. Schalen-Ringe zeichnen (nur belegte Schalen)
    for (int s = 0; s < 7; s++) {
        if (elem->shells[s] == 0) break;

        float rx = radii_x[s];
        float ry = radii_y[s];

        for (int angle_deg = 0; angle_deg < 360; angle_deg += 15) {
            float rad = (float)(angle_deg * M_PI / 180.0);
            int x = (int)(CENTER_X + rx * cosf(rad));
            int y = (int)(CENTER_Y + ry * sinf(rad));

            if (x >= 0 && x < CANVAS_W - 1 && y >= 0 && y < CANVAS_H) {
                if (canvas[y][x] == ' ') canvas[y][x] = '.';
            }
        }
    }

    // 2. Elektronen auf den Schalen rotieren lassen
    for (int s = 0; s < 7; s++) {
        uint8_t count = elem->shells[s];
        if (count == 0) break;

        float rx = radii_x[s];
        float ry = radii_y[s];

        for (uint8_t e = 0; e < count; e++) {
            // Phasenversatz pro Schale + Bewegung durch Ticks
            float base_angle = (float)(2.0 * M_PI * e / count);
            float rotation = (float)(tick * 0.15f * (s % 2 == 0 ? 1.0f : -1.0f));
            float angle = base_angle + rotation;

            int x = (int)(CENTER_X + rx * cosf(angle));
            int y = (int)(CENTER_Y + ry * sinf(angle));

            if (x >= 0 && x < CANVAS_W - 1 && y >= 0 && y < CANVAS_H) {
                canvas[y][x] = 'e';
            }
        }
    }

    // 3. Atomkern im Zentrum platzieren
    int sym_len = (int)strlen(elem->symbol);
    int start_x = CENTER_X - (sym_len / 2);
    for (int i = 0; i < sym_len; i++) {
        canvas[CENTER_Y][start_x + i] = elem->symbol[i];
    }

    CLEAR_SCREEN();
    printf("==================================================================\n");
    printf(" PROPHYSICS :: TOPOLOGISCHER ATOM-INSPEKTOR (Z=%u)                \n", elem->Z);
    printf("==================================================================\n");
    printf(" Element: %-16s | Symbol: %-3s | Tick: #%u\n", elem->name, elem->symbol, tick);
    printf(" Kern: %u Protonen, %u Neutronen | Elektronen Total: %u\n", elem->Z, elem->N, elem->Z);
    printf(" Schalen [K,L,M,N,O,P,Q]: [%u, %u, %u, %u, %u, %u, %u]\n",
        elem->shells[0], elem->shells[1], elem->shells[2], elem->shells[3],
        elem->shells[4], elem->shells[5], elem->shells[6]);
    printf("------------------------------------------------------------------\n\n");

    for (int y = 0; y < CANVAS_H; y++) {
        printf("  %s\n", canvas[y]);
    }

    printf("\n------------------------------------------------------------------\n");
    bool inv_ok = ProPhysics_Verify_Invariance(pu);
    printf(" Legende: [%s] Atomkern  |  [e] Rotierendes Schalen-Elektron  |  [.] Orbital\n", elem->symbol);
    printf(" Bit-Invarianz-Status: %s\n", inv_ok ? "PASSED [100% BIT CONSERVED]" : "FAILED");
    printf("==================================================================\n");
}

int main(void) {
    while (1) {
        CLEAR_SCREEN();
        printf("==================================================================\n");
        printf(" PROPHYSICS SDK :: ATOM- & ELEMENT-COMPILER (Z=1..118)            \n");
        printf("==================================================================\n\n");
        printf("Geben Sie eine Ordnungszahl ein (1-118, 0 zum Beenden): ");

        int z_input = 0;
        if (scanf("%d", &z_input) != 1 || z_input <= 0) {
            printf("\nProgramm beendet.\n");
            break;
        }

        if (z_input > 118) z_input = 118;

        ElementDefinition elem = ProPhysics_Get_Element((uint8_t)z_input);

        ProUniverse pu;
        ProPhysics_Initialize(&pu, DEMO_GRID_SIZE);

        // Soll-Invarianz für das Atom definieren
        pu.dynamic_invariance_target = (uint64_t)elem.Z * 5;

        // Eingabepuffer leeren
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        printf("\nWaehlen Sie den Visualisierungsmodus:\n");
        printf(" [1] Live-Animation der Orbitale (50 Ticks)\n");
        printf(" [2] Step-by-Step Inspektor (Enter-Taste)\n");
        printf("Auswahl (1-2): ");

        int mode = 1;
        if (scanf("%d", &mode) != 1) mode = 1;

        while ((c = getchar()) != '\n' && c != EOF);

        if (mode == 1) {
            for (uint32_t tick = 1; tick <= 50; tick++) {
                Render_Atom_2D(&pu, &elem, tick);
                ProPhysics_Execute_Tick(&pu);
                SLEEP_MS(100);
            }
        }
        else {
            uint32_t tick = 1;
            while (1) {
                Render_Atom_2D(&pu, &elem, tick);
                printf("\n [ENTER] Nächster Quanten-Tick | [q + ENTER] Zurück zur Elementauswahl: ");
                char input = (char)getchar();
                if (input == 'q' || input == 'Q') break;
                if (input != '\n') while ((c = getchar()) != '\n' && c != EOF);

                ProPhysics_Execute_Tick(&pu);
                tick++;
            }
        }

        ProPhysics_Free(&pu);
    }

    return EXIT_SUCCESS;
}