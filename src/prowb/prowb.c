#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "prowb.h"
#include "md_parser.h"

/* ============================================================
 *  OS ABSTRAKTION & DIRECTORY UTILS
 * ============================================================ */

typedef struct {
    char** files;
    int count;
} FileList;

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(path) _mkdir(path)

FileList get_file_list_os(const char* dirpath, const char* ext) {
    FileList list = { NULL, 0 };
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char searchPath[2048];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", dirpath);

    hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return list;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (ext && !strstr(findData.cFileName, ext)) continue;
        list.files = realloc(list.files, sizeof(char*) * (list.count + 1));
        if (list.files) {
            list.files[list.count] = _strdup(findData.cFileName);
            list.count++;
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    return list;
}
#else
#include <dirent.h>
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)

FileList get_file_list_os(const char* dirpath, const char* ext) {
    FileList list = { NULL, 0 };
    DIR* d = opendir(dirpath);
    if (!d) return list;
    struct dirent* dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        if (ext && !strstr(dir->d_name, ext)) continue;
        list.files = realloc(list.files, sizeof(char*) * (list.count + 1));
        if (list.files) {
            list.files[list.count] = strdup(dir->d_name);
            list.count++;
        }
    }
    closedir(d);
    return list;
}
#endif

void free_file_list(FileList list) {
    for (int i = 0; i < list.count; i++) free(list.files[i]);
    if (list.files) free(list.files);
}

int compare_filenames(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

/* ============================================================
 *  STANDARD HELPER
 * ============================================================ */

#define BUFFER_SIZE 8192

void _append_raw(FILE* dest, const char* filepath) {
    if (!dest || !filepath) return;
    FILE* src = fopen(filepath, "r");
    if (!src) return;

    char buffer[BUFFER_SIZE];
    size_t n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, src)) > 0) {
        fwrite(buffer, 1, n, dest);
    }
    fprintf(dest, "\n");
    fclose(src);
}

/* ============================================================
 *  SICHERHEITS-FUNKTIONEN
 * ============================================================ */

void _sanitize_for_id(const char* src, char* dest, size_t max_len) {
    if (!src || !dest || max_len == 0) return;
    size_t i = 0;
    while (*src && i < max_len - 1) {
        if (isalnum((unsigned char)*src) || *src == '_') {
            dest[i++] = *src;
        }
        else {
            dest[i++] = '_';
        }
        src++;
    }
    dest[i] = '\0';
}

void _escape_html_string(const char* src, char* dest, size_t max_len) {
    if (!src || !dest || max_len == 0) return;
    size_t i = 0;
    while (*src && i < max_len - 6) {
        switch (*src) {
        case '<': strcpy(&dest[i], "&lt;");   i += 4; break;
        case '>': strcpy(&dest[i], "&gt;");   i += 4; break;
        case '&': strcpy(&dest[i], "&amp;");  i += 5; break;
        case '"': strcpy(&dest[i], "&quot;"); i += 6; break;
        case '\'':strcpy(&dest[i], "&#39;");  i += 5; break;
        default:  dest[i++] = *src; break;
        }
        src++;
    }
    dest[i] = '\0';
}

/* ============================================================
 *  NAV GENERATOR (Legacy — view-basiert)
 * ============================================================ */

void _print_nav_button(FILE* dest, const char* filename, int is_doc) {
    if (!dest || !filename) return;

    char raw_name[256];
    strncpy(raw_name, filename, 255);
    raw_name[255] = '\0';

    char* dot = strrchr(raw_name, '.');
    if (dot) *dot = '\0';

    char* clean_name = raw_name;
    if (strlen(clean_name) >= 3 &&
        isdigit((unsigned char)clean_name[0]) &&
        isdigit((unsigned char)clean_name[1]) &&
        clean_name[2] == '_') {
        clean_name += 3;
    }

    char safe_id[256];
    _sanitize_for_id(clean_name, safe_id, sizeof(safe_id));

    char js_id[512];
    if (is_doc) snprintf(js_id, sizeof(js_id), "doc_%s", safe_id);
    else        snprintf(js_id, sizeof(js_id), "%s", safe_id);

    char raw_label[256];
    char* start = clean_name;
    if (strncmp(start, "view_", 5) == 0) start += 5;

    int i = 0;
    for (; *start && i < 254; start++) {
        if (*start == '_') raw_label[i] = ' ';
        else raw_label[i] = (char)toupper((unsigned char)*start);
        i++;
    }
    raw_label[i] = '\0';

    char safe_label[1024];
    _escape_html_string(raw_label, safe_label, sizeof(safe_label));

    fprintf(dest, "<button class=\"nav-btn\" onclick=\"app.navigate('%s')\">%s</button>\n",
        js_id, safe_label);
}

void _process_nav_template(FILE* dest, const char* src_dir) {
    if (!dest || !src_dir) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/parts/nav.html", src_dir);
    FILE* src = fopen(path, "r");
    if (!src) return;

    char line[1024];
    while (fgets(line, sizeof(line), src)) {
        if (strstr(line, "{{NAV_VIEWS}}")) {
            char view_path[512]; snprintf(view_path, sizeof(view_path), "%s/views", src_dir);
            FileList list = get_file_list_os(view_path, ".html");
            if (list.count > 0) {
                qsort(list.files, list.count, sizeof(char*), compare_filenames);
                for (int i = 0; i < list.count; i++) _print_nav_button(dest, list.files[i], 0);
            }
            free_file_list(list);
            continue;
        }
        if (strstr(line, "{{NAV_DOCS}}")) {
            char doc_path[512]; snprintf(doc_path, sizeof(doc_path), "%s/docs", src_dir);
            FileList list = get_file_list_os(doc_path, ".md");
            if (list.count > 0) {
                qsort(list.files, list.count, sizeof(char*), compare_filenames);
                for (int i = 0; i < list.count; i++) _print_nav_button(dest, list.files[i], 1);
            }
            free_file_list(list);
            continue;
        }
        fputs(line, dest);
    }
    fclose(src);
}

/* ============================================================
 *  VERARBEITUNG (Legacy)
 * ============================================================ */

void _process_dir(FILE* dest, const char* base, const char* sub, const char* ext,
    const char* pre, const char* post) {
    if (!dest || !base || !sub) return;

    char dirpath[512]; snprintf(dirpath, sizeof(dirpath), "%s/%s", base, sub);
    FileList list = get_file_list_os(dirpath, ext);
    if (list.count == 0) return;

    qsort(list.files, list.count, sizeof(char*), compare_filenames);
    for (int i = 0; i < list.count; i++) {
        if (!list.files[i]) continue;
        char path[1024]; snprintf(path, sizeof(path), "%s/%s", dirpath, list.files[i]);
        if (pre)  fprintf(dest, "%s\n", pre);
        _append_raw(dest, path);
        if (post) fprintf(dest, "%s\n", post);
    }
    free_file_list(list);
}

void _process_docs(FILE* dest, const char* base, const MdConfig* cfg) {
    if (!dest || !base) return;

    char dirpath[512]; snprintf(dirpath, sizeof(dirpath), "%s/docs", base);
    FileList list = get_file_list_os(dirpath, ".md");
    if (list.count == 0) return;

    qsort(list.files, list.count, sizeof(char*), compare_filenames);
    for (int i = 0; i < list.count; i++) {
        if (!list.files[i]) continue;

        char path[1024]; snprintf(path, sizeof(path), "%s/%s", dirpath, list.files[i]);

        char raw_name[256];
        strncpy(raw_name, list.files[i], 255);
        raw_name[255] = '\0';

        char* dot = strrchr(raw_name, '.');
        if (dot) *dot = 0;

        char* clean_name = raw_name;
        if (strlen(clean_name) >= 3 &&
            isdigit((unsigned char)clean_name[0]) &&
            isdigit((unsigned char)clean_name[1]) &&
            clean_name[2] == '_') {
            clean_name += 3;
        }

        char safe_id[256];
        _sanitize_for_id(clean_name, safe_id, sizeof(safe_id));

        char vid[512];
        snprintf(vid, sizeof(vid), "doc_%s", safe_id);

        md_parse_file(dest, path, vid, cfg);
    }
    free_file_list(list);
}

/* ============================================================
 *  LEGACY API
 * ============================================================ */

PROWB_API const char* prowb_version() { return "4.2.1-Anchor-GFM"; }

PROWB_API PROWB_RESULT prowb_build(const char* src_dir, const char* out_base, const char* wasm_name) {
    if (!src_dir || !out_base || !wasm_name) return 1;

    char target_dir[512];
    char index_path[1024];

    char safe_wasm_name[256];
    _sanitize_for_id(wasm_name, safe_wasm_name, sizeof(safe_wasm_name));

    snprintf(target_dir, sizeof(target_dir), "%s/%s", out_base, safe_wasm_name);
    MKDIR(out_base);
    MKDIR(target_dir);
    snprintf(index_path, sizeof(index_path), "%s/index.html", target_dir);

    printf("[ProWB] Build Start (legacy): %s -> %s\n", safe_wasm_name, index_path);
    FILE* out = fopen(index_path, "w");
    if (!out) return 1;

    /* STATIC statt stack -- MdConfig ist ~780 KB gross */
    static MdConfig cfg;
    md_config_init(&cfg);
    {
        char p[512];
        snprintf(p, sizeof(p), "%s/config/emoji.txt", src_dir); md_config_load_emoji(&cfg, p);
        snprintf(p, sizeof(p), "%s/config/html_whitelist.txt", src_dir); md_config_load_whitelist(&cfg, p);
        snprintf(p, sizeof(p), "%s/config/crosslinks.txt", src_dir); md_config_load_crosslinks(&cfg, p);
    }

    char tmp[1024];

    snprintf(tmp, sizeof(tmp), "%s/parts/header.html", src_dir); _append_raw(out, tmp);
    _process_dir(out, src_dir, "css", ".css", "<style>", "</style>");
    _process_nav_template(out, src_dir);
    _process_dir(out, src_dir, "views", ".html", "", "");
    _process_docs(out, src_dir, &cfg);

    fprintf(out, "<script>const PRO_WASM_CORE = '%s';</script>\n", safe_wasm_name);

    _process_dir(out, src_dir, "js", ".js", "<script>", "</script>");
    snprintf(tmp, sizeof(tmp), "%s/parts/footer.html", src_dir); _append_raw(out, tmp);

    fclose(out);
    printf("[ProWB] Build erfolgreich.\n");
    return 0;
}

/* ============================================================
 *  NEU: MANIFEST-DRIVEN BUILDER (ProPhysics)
 * ============================================================ */

#define MAX_SECTIONS          32
#define MAX_MANIFEST_ENTRIES 1024

typedef struct {
    char source_path[512];
    char section[64];
    char doc_id[128];
} ManifestEntry;

typedef struct {
    ManifestEntry entries[MAX_MANIFEST_ENTRIES];
    int           count;
    char          sections[MAX_SECTIONS][64];
    int           section_count;
} Manifest;

/* --- kleine Helfer --- */

static void _trim_inplace_b(char* s) {
    if (!s) return;
    char* p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
        s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

static int _manifest_has_section(const Manifest* m, const char* sec) {
    for (int i = 0; i < m->section_count; i++)
        if (strcmp(m->sections[i], sec) == 0) return 1;
    return 0;
}

static void _manifest_add_section(Manifest* m, const char* sec) {
    if (m->section_count >= MAX_SECTIONS) return;
    if (_manifest_has_section(m, sec)) return;
    strncpy(m->sections[m->section_count], sec, 63);
    m->sections[m->section_count][63] = '\0';
    m->section_count++;
}

static int _load_manifest(const char* path, Manifest* m) {
    if (!path || !m) return 0;
    memset(m, 0, sizeof(Manifest));

    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[ProWB] Manifest nicht gefunden: %s\n", path);
        return 0;
    }

    char line[2048];
    while (fgets(line, sizeof(line), f) && m->count < MAX_MANIFEST_ENTRIES) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\0' || *p == '\r' || *p == '\n') continue;

        char* p1 = strchr(p, '|');
        if (!p1) continue;
        char* p2 = strchr(p1 + 1, '|');
        if (!p2) continue;

        *p1 = '\0';
        *p2 = '\0';

        char* src = p;
        char* sec = p1 + 1;
        char* id = p2 + 1;

        _trim_inplace_b(src);
        _trim_inplace_b(sec);
        _trim_inplace_b(id);

        if (*src == '\0' || *sec == '\0' || *id == '\0') continue;

        ManifestEntry* e = &m->entries[m->count];
        strncpy(e->source_path, src, sizeof(e->source_path) - 1);
        e->source_path[sizeof(e->source_path) - 1] = '\0';
        strncpy(e->section, sec, sizeof(e->section) - 1);
        e->section[sizeof(e->section) - 1] = '\0';
        strncpy(e->doc_id, id, sizeof(e->doc_id) - 1);
        e->doc_id[sizeof(e->doc_id) - 1] = '\0';

        _manifest_add_section(m, e->section);
        m->count++;
    }
    fclose(f);
    return 1;
}

/* --- Sektions-Nav-Button --- */

static const char* _section_icon(const char* section) {
    if (strcmp(section, "Overview") == 0) return "\xF0\x9F\x93\x8B";              /* 📋 */
    if (strcmp(section, "Physics") == 0) return "\xE2\x9A\x9B\xEF\xB8\x8F";     /* ⚛️ */
    if (strcmp(section, "Modules") == 0) return "\xF0\x9F\xA7\xA9";              /* 🧩 */
    if (strcmp(section, "API") == 0) return "\xF0\x9F\x94\x8C";              /* 🔌 */
    if (strcmp(section, "Tests") == 0) return "\xF0\x9F\xA7\xAA";              /* 🧪 */
    if (strcmp(section, "Build") == 0) return "\xF0\x9F\x8F\x97\xEF\xB8\x8F"; /* 🏗️ */
    return "\xF0\x9F\x93\x84";                                                     /* 📄 */
}

static void _print_section_nav_button(FILE* dest, const char* section) {
    if (!dest || !section) return;

    char view_id[128];
    int i;
    for (i = 0; section[i] && i < 127; i++) {
        unsigned char c = (unsigned char)section[i];
        if (isalnum(c)) view_id[i] = (char)tolower(c);
        else            view_id[i] = '_';
    }
    view_id[i] = '\0';

    char safe_id[256];
    _sanitize_for_id(view_id, safe_id, sizeof(safe_id));

    char label[256];
    int j;
    for (j = 0; section[j] && j < 255; j++)
        label[j] = (char)toupper((unsigned char)section[j]);
    label[j] = '\0';

    fprintf(dest,
        "<button class=\"nav-btn\" onclick=\"app.navigate('view_%s')\">%s %s</button>\n",
        safe_id, _section_icon(section), label);
}

/* --- Doku-Karten --- */

static void _print_section_cards(FILE* dest, const char* section, const Manifest* m) {
    if (!dest || !section || !m) return;

    for (int i = 0; i < m->count; i++) {
        if (strcmp(m->entries[i].section, section) != 0) continue;

        char doc_view_id[384];
        snprintf(doc_view_id, sizeof(doc_view_id),
            "doc_%s_%s", m->entries[i].section, m->entries[i].doc_id);

        char safe_view_id[512];
        _sanitize_for_id(doc_view_id, safe_view_id, sizeof(safe_view_id));

        char label[128];
        strncpy(label, m->entries[i].doc_id, 127);
        label[127] = '\0';
        for (char* p = label; *p; p++)
            if (*p == '_') *p = ' ';

        char safe_label[512];
        _escape_html_string(label, safe_label, sizeof(safe_label));

        fprintf(dest,
            "        <div class=\"product-box\">\n"
            "            <div class=\"box-icon\">\xF0\x9F\x93\x84</div>\n"
            "            <h3>%s</h3>\n"
            "            <div class=\"dashboard-actions\">\n"
            "                <button class=\"box-btn\" onclick=\"app.navigate('%s')\">OPEN</button>\n"
            "            </div>\n"
            "        </div>\n",
            safe_label, safe_view_id);
    }
}

/* --- Nav-Template mit Manifest --- */

static void _process_nav_template_manifest(FILE* dest, const char* src_dir, const Manifest* m) {
    if (!dest || !src_dir) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/parts/nav.html", src_dir);
    FILE* src = fopen(path, "r");
    if (!src) return;

    char line[2048];
    while (fgets(line, sizeof(line), src)) {
        if (strstr(line, "{{NAV_VIEWS}}")) {
            char view_path[512];
            snprintf(view_path, sizeof(view_path), "%s/views", src_dir);
            FileList list = get_file_list_os(view_path, ".html");
            if (list.count > 0) {
                qsort(list.files, list.count, sizeof(char*), compare_filenames);
                for (int i = 0; i < list.count; i++)
                    _print_nav_button(dest, list.files[i], 0);
            }
            free_file_list(list);
            continue;
        }
        if (strstr(line, "{{NAV_DOCS}}")) {
            if (m) {
                for (int i = 0; i < m->section_count; i++)
                    _print_section_nav_button(dest, m->sections[i]);
            }
            continue;
        }
        fputs(line, dest);
    }
    fclose(src);
}

/* --- View-Datei mit {{SECTION_CARDS:...}} --- */

static void _process_view_file_manifest(FILE* dest, const char* path, const Manifest* m) {
    if (!dest || !path) return;

    FILE* src = fopen(path, "r");
    if (!src) return;

    const char* TAG = "{{SECTION_CARDS:";
    const size_t TAG_LEN = 16;

    char line[4096];
    while (fgets(line, sizeof(line), src)) {
        char* ph = strstr(line, TAG);
        if (ph) {
            /* FIX: schliessende Klammer ist DOPPELT: }} */
            char* end = strstr(ph, "}}");
            if (end && end > ph + TAG_LEN) {
                size_t len = (size_t)(end - (ph + TAG_LEN));
                if (len > 0 && len < 64) {
                    char section[64];
                    strncpy(section, ph + TAG_LEN, len);
                    section[len] = '\0';
                    _trim_inplace_b(section);

                    *ph = '\0';
                    fputs(line, dest);
                    _print_section_cards(dest, section, m);
                    fputs(end + 2, dest);   /* FIX: +2 statt +1 */
                    continue;
                }
            }
        }
        fputs(line, dest);
    }
    fclose(src);
}

static void _process_views_dir_manifest(FILE* dest, const char* base, const Manifest* m) {
    if (!dest || !base) return;

    char dirpath[512];
    snprintf(dirpath, sizeof(dirpath), "%s/views", base);
    FileList list = get_file_list_os(dirpath, ".html");
    if (list.count == 0) return;

    qsort(list.files, list.count, sizeof(char*), compare_filenames);
    for (int i = 0; i < list.count; i++) {
        if (!list.files[i]) continue;
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirpath, list.files[i]);
        _process_view_file_manifest(dest, path, m);
    }
    free_file_list(list);
}

/* --- Doku aus Manifest parsen --- */

static void _process_docs_from_manifest(FILE* dest, const Manifest* m, const MdConfig* cfg) {
    if (!dest || !m) return;

    int ok = 0, missing = 0;
    for (int i = 0; i < m->count; i++) {
        char vid[512];
        snprintf(vid, sizeof(vid), "doc_%s_%s",
            m->entries[i].section, m->entries[i].doc_id);

        char safe_vid[512];
        _sanitize_for_id(vid, safe_vid, sizeof(safe_vid));

        FILE* probe = fopen(m->entries[i].source_path, "r");
        if (!probe) {
            fprintf(stderr, "[ProWB]   ! missing: %s\n", m->entries[i].source_path);
            missing++;
            continue;
        }
        fclose(probe);

        md_parse_file(dest, m->entries[i].source_path, safe_vid, cfg);
        ok++;
    }
    printf("[ProWB]   Docs: %d geparst, %d fehlend\n", ok, missing);
}

/* --- Config-Verzeichnis aus Manifest-Pfad ableiten --- */

static void _config_dir_from_manifest(const char* manifest_path,
    char* out, size_t out_max) {
    if (!manifest_path || !out || out_max == 0) return;
    const char* slash = strrchr(manifest_path, '/');
#ifdef _WIN32
    const char* slash2 = strrchr(manifest_path, '\\');
    if (slash2 && (!slash || slash2 > slash)) slash = slash2;
#endif
    if (slash) {
        size_t len = (size_t)(slash - manifest_path);
        if (len >= out_max) len = out_max - 1;
        memcpy(out, manifest_path, len);
        out[len] = '\0';
    }
    else {
        strncpy(out, ".", out_max - 1);
        out[out_max - 1] = '\0';
    }
}

/* ============================================================
 *  MANIFEST-API
 * ============================================================ */

PROWB_API PROWB_RESULT prowb_build_from_manifest(const char* src_dir,
    const char* manifest_path,
    const char* out_dir) {
    if (!src_dir || !manifest_path || !out_dir) return 1;

    /* 1. Manifest laden */
    Manifest m;
    if (!_load_manifest(manifest_path, &m)) return 2;
    printf("[ProWB] Manifest geladen: %d Einträge, %d Sektionen\n",
        m.count, m.section_count);

    /* 2. Configs laden (config/ neben dem Manifest)
     *
     * WICHTIG: STATIC -- MdConfig ist ~780 KB gross und darf NICHT
     * auf den Stack (sonst STATUS_STACK_OVERFLOW beim ersten Aufruf).
     * prowb ist single-threaded, static ist daher unproblematisch.
     */
    static MdConfig cfg;
    md_config_init(&cfg);
    {
        char cfgdir[512];
        _config_dir_from_manifest(manifest_path, cfgdir, sizeof(cfgdir));
        char p[1024];

        snprintf(p, sizeof(p), "%s/config/emoji.txt", cfgdir);
        md_config_load_emoji(&cfg, p);
        snprintf(p, sizeof(p), "%s/config/html_whitelist.txt", cfgdir);
        md_config_load_whitelist(&cfg, p);
        snprintf(p, sizeof(p), "%s/config/crosslinks.txt", cfgdir);
        md_config_load_crosslinks(&cfg, p);

        printf("[ProWB] Configs: %d Emojis, %d Tags, %d Crosslinks\n",
            cfg.emoji_count, cfg.allowed_tag_count, cfg.crosslink_count);
    }

    /* 3. Ausgabe öffnen */
    MKDIR(out_dir);
    char index_path[1024];
    snprintf(index_path, sizeof(index_path), "%s/index.html", out_dir);

    printf("[ProWB] Build Start (manifest) -> %s\n", index_path);
    FILE* out = fopen(index_path, "w");
    if (!out) return 1;

    char tmp[1024];

    /* 4. Header */
    snprintf(tmp, sizeof(tmp), "%s/parts/header.html", src_dir);
    _append_raw(out, tmp);

    /* 5. CSS */
    _process_dir(out, src_dir, "css", ".css", "<style>", "</style>");

    /* 6. Nav */
    _process_nav_template_manifest(out, src_dir, &m);

    /* 7. Views */
    _process_views_dir_manifest(out, src_dir, &m);

    /* 8. Docs */
    _process_docs_from_manifest(out, &m, &cfg);

    /* 9. Runtime-Konstante */
    fprintf(out, "<script>const PRO_WASM_CORE = 'prophysics';</script>\n");

    /* 10. JS */
    _process_dir(out, src_dir, "js", ".js", "<script>", "</script>");

    /* 11. Footer */
    snprintf(tmp, sizeof(tmp), "%s/parts/footer.html", src_dir);
    _append_raw(out, tmp);

    fclose(out);
    printf("[ProWB] Build erfolgreich.\n");
    return 0;
}

/* ============================================================
 *  STANDALONE ENTRY
 * ============================================================ */

#ifdef PROWB_STANDALONE
int main(int argc, char* argv[]) {
    if (argc >= 2 && strcmp(argv[1], "--manifest") == 0) {
        if (argc < 5) {
            printf("Usage: %s --manifest <src_dir> <manifest.txt> <out_dir>\n", argv[0]);
            return 1;
        }
        return prowb_build_from_manifest(argv[2], argv[3], argv[4]);
    }
    if (argc < 4) {
        printf("Usage:\n");
        printf("  %s <src_dir> <out_base> <wasm_name>                 (legacy)\n", argv[0]);
        printf("  %s --manifest <src_dir> <manifest.txt> <out_dir>    (ProPhysics)\n", argv[0]);
        return 1;
    }
    return prowb_build(argv[1], argv[2], argv[3]);
}
#endif