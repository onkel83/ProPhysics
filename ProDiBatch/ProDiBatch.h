/* ==========================================================================
 * ProDiBatch - Public Engine Interface
 * File: ProDiBatch.h
 * Architecture: C99, Callback-Driven Hardware UI Abstraction
 * Optimierung: Cache-Line-Aligned Telemetry zur Vermeidung von False Sharing
 * ========================================================================== */

#ifndef PRODIBATCH_H
#define PRODIBATCH_H

#include "ProDiBatch_Config.h"
#include "ProDiBatch_Exports.h"
#include "ProDiBatch_Version.h"
#include <stdint.h>
#include <stdbool.h>

 // --- Der universelle funktionale Rendering-Vertrag ---
typedef void (*ProDiBatch_CellRenderCallback)(
    void* user_context, // Anonymisierter Zeiger auf das Triebwerk (z.B. ProUniverse)
    int view_idx,       // Index des aktuellen Viewports (0 bis 3)
    int local_x,        // Lokale Zell-Koordinate X (0 bis 9)
    int local_y,        // Lokale Zell-Koordinate Y (0 bis 9)
    char* out_chars     // Ziel-Buffer für exakt 2 Zeichen (muss im Callback befüllt werden)
    );

// Thread-sicherer Ring-Buffer für die asynchronen Log-Kerne
typedef struct {
    char lines[PRODIBATCH_LOG_LINES][PRODIBATCH_LOG_LINE_LEN];
    volatile long write_index;
} ProDiBatch_LogField;

// Zentrales Zustandsregister des UI-Triebwerks (Cache-Line Aligned)
#ifdef _MSC_VER
#define ALIGN_64 __declspec(align(64))
#else
#define ALIGN_64 __attribute__((aligned(64)))
#endif

typedef struct {
    void* user_context;
    ProDiBatch_CellRenderCallback cell_renderer;

    // --- TELEMETRIE-METRIKEN MIT CACHE-LINE-ISOLIERUNG (64-BYTE ALIGNED) ---
    // Verhindert False Sharing zwischen den Physik-Workern und dem UI-Thread vollständig!
    ALIGN_64 volatile uint64_t metric_sim_tick;
    ALIGN_64 volatile uint64_t metric_active_nodes;
    ALIGN_64 volatile uint64_t metric_target_invariance;

    ALIGN_64 ProDiBatch_LogField log_buffer;
    volatile bool is_running;
    void* thread_handle; // Maps intern auf Win32 HANDLE
} ProDiBatch_Engine;

// --- Öffentliche API-Prototypen ---
PRODIBATCH_API bool ProDiBatch_Initialize(ProDiBatch_Engine* engine, void* user_context, ProDiBatch_CellRenderCallback callback);
PRODIBATCH_API bool ProDiBatch_Start(ProDiBatch_Engine* engine);
PRODIBATCH_API void ProDiBatch_Stop(ProDiBatch_Engine* engine);
PRODIBATCH_API void ProDiBatch_Log(ProDiBatch_Engine* engine, const char* format, ...);

#endif // PRODIBATCH_H