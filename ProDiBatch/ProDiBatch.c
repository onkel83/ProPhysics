/* ==========================================================================
 * ProDiBatch - Core Engine Runtime Execution
 * File: ProDiBatch.c
 * Architecture: Win32 High-Performance Low-CPU ASCII Renderer
 * ========================================================================== */

#include "ProDiBatch.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>

 // Interne Struktur für den Double-Buffer-Vergleich im RAM gegen Bildschirm-Flimmern
#define SCREEN_ROWS 40
#define SCREEN_COLS 120
static char back_buffer[SCREEN_ROWS][SCREEN_COLS];
static char front_buffer[SCREEN_ROWS][SCREEN_COLS];

// Hilfsfunktion: Setzt den Win32-Cursor blitzschnell auf eine punktuelle Position
static void ProDiBatch_SetCursor(int x, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hConsole, coord);
}

// Der asynchrone UI-Thread-Kernel
static DWORD WINAPI ProDiBatch_ThreadProc(LPVOID lpParam) {
    ProDiBatch_Engine* engine = (ProDiBatch_Engine*)lpParam;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Initialen Bildschirm-Buffer löschen und steril vorbereiten
    memset(back_buffer, ' ', sizeof(back_buffer));
    memset(front_buffer, 0, sizeof(front_buffer));

    // Konsolenfenster-Cursor hart ausschalten gegen Tearing
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    while (engine->is_running) {
        int current_row = 0;
        char temp_row[256];

        // 1. HEADER & METRIKEN BATCHEN
        sprintf(back_buffer[current_row++], "====================================================================================================================");
        sprintf(back_buffer[current_row++], "                                         PRODIBATCH CONTROL ROOM ENGINE v%d.%d.%d                                      ",
            PRODIBATCH_VERSION_MAJOR, PRODIBATCH_VERSION_MINOR, PRODIBATCH_VERSION_PATCH);
        sprintf(back_buffer[current_row++], "====================================================================================================================");
        sprintf(back_buffer[current_row++], "  [RUNTIME] Simulations-Tick: #%-10llu | Aktive Zellen: %-10llu | Invarianz-Soll: %-10llu Bits",
            (unsigned long long)engine->metric_sim_tick,
            (unsigned long long)engine->metric_active_nodes,
            (unsigned long long)engine->metric_target_invariance);
        sprintf(back_buffer[current_row++], "--------------------------------------------------------------------------------------------------------------------");
        sprintf(back_buffer[current_row++], "   VIEWPORT 0 (XY-CUT)  |   VIEWPORT 1 (XZ-CUT)  |   VIEWPORT 2 (YZ-CUT)  |   VIEWPORT 3 (DIAG-ORBIT)           ");
        sprintf(back_buffer[current_row++], "------------------------+------------------------+------------------------+-----------------------------------------");

        // 2. LINE-INTERLEAVING: Baut die Zeilen aller 4 Viewports nebeneinander im RAM zusammen
        for (int y = PRODIBATCH_VIEWPORT_HEIGHT - 1; y >= 0; y--) {
            int bp = 0;
            temp_row[bp++] = ' '; temp_row[bp++] = ' '; // Linker Randabstand

            for (int v = 0; v < PRODIBATCH_VIEWPORTS_COUNT; v++) {
                for (int x = 0; x < PRODIBATCH_VIEWPORT_WIDTH; x++) {
                    char cell_token[PRODIBATCH_CELL_CHAR_LEN + 1] = { ' ', ' ', '\0' };

                    // Kausaler Callback-Aufruf in das anonyme physikalische Modell (FIX: user_context)
                    if (engine->cell_renderer) {
                        engine->cell_renderer(engine->user_context, v, x, y, cell_token);
                    }
                    temp_row[bp++] = cell_token[0];
                    temp_row[bp++] = cell_token[1];
                }
                if (v < PRODIBATCH_VIEWPORTS_COUNT - 1) {
                    temp_row[bp++] = ' '; temp_row[bp++] = '|'; temp_row[bp++] = ' '; temp_row[bp++] = ' ';
                }
            }
            temp_row[bp] = '\0';
            strcpy(back_buffer[current_row++], temp_row);
        }

        sprintf(back_buffer[current_row++], "------------------------+------------------------+------------------------+-----------------------------------------");
        sprintf(back_buffer[current_row++], "                                         ASYNCHRONOUS ENGINE LIVE LOG BUFFER                                        ");
        sprintf(back_buffer[current_row++], "--------------------------------------------------------------------------------------------------------------------");

        // 3. LOG-ZEILEN SCANNTEN
        long head = engine->log_buffer.write_index;
        int log_start_idx = (head >= PRODIBATCH_LOG_LINES) ? (head % PRODIBATCH_LOG_LINES) : 0;

        for (int l = 0; l < PRODIBATCH_LOG_LINES; l++) {
            int actual_idx = (log_start_idx + l) % PRODIBATCH_LOG_LINES;
            if (engine->log_buffer.lines[actual_idx][0] == '\0') {
                sprintf(back_buffer[current_row++], " ");
            }
            else {
                sprintf(back_buffer[current_row++], " [LOG] %s", engine->log_buffer.lines[actual_idx]);
            }
        }
        sprintf(back_buffer[current_row++], "====================================================================================================================");

        // 4. UNBIEGSAMES INPLACE-VALUE UPDATE
        for (int r = 0; r < current_row; r++) {
            int len = (int)strlen(back_buffer[r]);
            for (int c = 0; c < len; c++) {
                if (back_buffer[r][c] != front_buffer[r][c]) {
                    front_buffer[r][c] = back_buffer[r][c];
                    ProDiBatch_SetCursor(c, r);
                    putchar(front_buffer[r][c]);
                }
            }
            int front_len = (int)strlen(front_buffer[r]);
            if (front_len > len) {
                ProDiBatch_SetCursor(len, r);
                for (int k = len; k < front_len; k++) {
                    putchar(' ');
                    front_buffer[r][k] = ' ';
                }
            }
        }

        Sleep(PRODIBATCH_REFRESH_MS);
    }
    return 0;
}

// Initialisiert das sterile Zustandsregister von ProDiBatch
PRODIBATCH_API bool ProDiBatch_Initialize(ProDiBatch_Engine* engine, void* user_context, ProDiBatch_CellRenderCallback callback) {
    if (!engine || !callback) return false;
    memset(engine, 0, sizeof(ProDiBatch_Engine));
    engine->user_context = user_context; // (FIX: user_context)
    engine->cell_renderer = callback;
    engine->is_running = false;
    engine->thread_handle = NULL;
    return true;
}

// Startet den asynchronen Win32 UI-Thread
PRODIBATCH_API bool ProDiBatch_Start(ProDiBatch_Engine* engine) {
    if (!engine || engine->is_running) return false;
    engine->is_running = true;
    system("cls");
    engine->thread_handle = CreateThread(NULL, 0, ProDiBatch_ThreadProc, engine, 0, NULL);
    return (engine->thread_handle != NULL);
}

// Fährt das UI-System geordnet herunter
PRODIBATCH_API void ProDiBatch_Stop(ProDiBatch_Engine* engine) {
    if (!engine || !engine->is_running) return;
    engine->is_running = false;
    WaitForSingleObject((HANDLE)engine->thread_handle, INFINITE);
    CloseHandle((HANDLE)engine->thread_handle);
    engine->thread_handle = NULL;
    system("cls");
}

// Thread-sicherer Lock-Free Logger
PRODIBATCH_API void ProDiBatch_Log(ProDiBatch_Engine* engine, const char* format, ...) {
    if (!engine) return;

    long current_idx = _InterlockedIncrement(&engine->log_buffer.write_index) - 1;
    int target_line = current_idx % PRODIBATCH_LOG_LINES;

    va_list args;
    va_start(args, format);
    vsnprintf(engine->log_buffer.lines[target_line], PRODIBATCH_LOG_LINE_LEN - 1, format, args);
    va_end(args);

    engine->log_buffer.lines[target_line][PRODIBATCH_LOG_LINE_LEN - 1] = '\0';
}