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
     * @brief Legacy-Builder: flacher src_dir mit docs/-Unterordner.
     *        Bleibt für andere Projekte erhalten (kein Breaking-Change).
     */
    PROWB_API PROWB_RESULT prowb_build(const char* src_dir,
        const char* out_base,
        const char* wasm_name);

    /**
     * @brief Manifest-getriebener Builder (ProPhysics).
     *
     * Liest die Doku-Quellen aus einem Manifest und baut die komplette
     * index.html. Es werden KEINE MD-Dateien kopiert — der Builder liest
     * sie direkt von ihren kanonischen Pfaden.
     *
     * Zusätzlich werden drei Config-Dateien aus dem Unterordner
     * "config/" neben dem Manifest geladen, sofern vorhanden:
     *   config/emoji.txt          — Shortcode-Mapping
     *   config/html_whitelist.txt — Erlaubte HTML-Tags
     *   config/crosslinks.txt     — Thema|Name|Ort-Auflösung
     *
     * Manifest-Format (Pipe-getrennt, '#' als Kommentar):
     *     <Quellpfad>|<Sektion>|<Doc-ID>
     *
     * Template-Platzhalter in parts/nav.html:
     *     {{NAV_DOCS}}  -> Sektions-Buttons aus Manifest
     *     {{NAV_VIEWS}} -> Legacy: alle view_*.html als Buttons
     *
     * Template-Platzhalter in views/*.html:
     *     {{SECTION_CARDS:<Sektionsname>}} -> Doc-Karten aus Manifest
     *
     * @param src_dir       Pfad zu docs/web/src (parts, css, js, views)
     * @param manifest_path Pfad zu docs/web/manifest.txt
     * @param out_dir       Ausgabe-Verzeichnis (z.B. "out/web")
     * @return 0 bei Erfolg, !=0 bei Fehler
     */
    PROWB_API PROWB_RESULT prowb_build_from_manifest(const char* src_dir,
        const char* manifest_path,
        const char* out_dir);

    PROWB_API const char* prowb_version();

#ifdef __cplusplus
}
#endif

#endif // PROWB_H