#ifndef PROWB_H
#define PROWB_H

#ifdef __cplusplus
extern "C" {
#endif

    // --- DLL EXPORT / IMPORT LOGIK ---
#if defined(PROWB_STANDALONE) || defined(PROWB_STATIC)
#define PROWB_API
#elif defined(_WIN32) && defined(PROWB_BUILD_DLL)
#define PROWB_API __declspec(dllexport)
#elif defined(_WIN32)
#define PROWB_API __declspec(dllimport)
#else
#define PROWB_API
#endif

    // --- TYPEN DEFINITION ---
    typedef int PROWB_RESULT;

    /**
     * @brief Baut die Webseite für einen spezifischen WASM-Kern
     * @param src_dir Pfad zu den Web-Assets (Templates, CSS, JS)
     * @param out_base_dir Basis-Pfad für die Ausgabe (z.B. "./dist")
     * @param wasm_name Name des Kerns (z.B. "prokey") -> erstellt Ordner ./dist/prokey/
     */
    PROWB_API PROWB_RESULT prowb_build(const char* src_dir, const char* out_base_dir, const char* wasm_name);

    PROWB_API const char* prowb_version();

#ifdef __cplusplus
}
#endif

#endif // PROWB_H