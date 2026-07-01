/* ==========================================================================
 * ProDiBatch - Static Unified Configuration Matrix
 * File: ProDiBatch_Config.h
 * Architecture: Zero-Allocation Compile-Time Boundaries
 * ========================================================================== */

#ifndef PRODIBATCH_CONFIG_H
#define PRODIBATCH_CONFIG_H

 // --- Geometrie des Kontrollraums ---
#define PRODIBATCH_VIEWPORTS_COUNT     4    // 4 parallele Überwachungsfenster
#define PRODIBATCH_VIEWPORT_WIDTH     10    // 10 Zellen Breite
#define PRODIBATCH_VIEWPORT_HEIGHT    10    // 10 Zellen Höhe
#define PRODIBATCH_CELL_CHAR_LEN       2    // Jede Zelle beansprucht exakt 2 ASCII-Zeichen (z.B. "++")

// --- Asynchrones Log-System ---
#define PRODIBATCH_LOG_LINES          12    // Anzahl der Zeilen im unteren Log-Fenster
#define PRODIBATCH_LOG_LINE_LEN      125    // Maximale Breite einer Log-Zeile

// --- Hardware-Zeitbasis ---
#define PRODIBATCH_REFRESH_MS         33    // ~30 FPS Aktualisierungsfrequenz gegen CPU-Bloat

#endif // PRODIBATCH_CONFIG_H