#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "prowb.h"
#include "md_parser.h"

// --- OS ABSTRAKTION & DIRECTORY UTILS ---

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

// --- STANDARD HELPER ---

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

// --- SICHERHEITS-FUNKTIONEN (XSS & INJECTION PROTECTION) ---

// Zwingt Strings dazu, nur aus Alphanumerischen Zeichen + Underscore zu bestehen (für IDs und JS-Calls)
void _sanitize_for_id(const char* src, char* dest, size_t max_len) {
    if (!src || !dest || max_len == 0) return;
    size_t i = 0;
    while (*src && i < max_len - 1) {
        if (isalnum((unsigned char)*src) || *src == '_') {
            dest[i++] = *src;
        }
        else {
            dest[i++] = '_'; // Gefährliche Zeichen maskieren
        }
        src++;
    }
    dest[i] = '\0';
}

// HTML-Escaping für die Anzeige von Texten im Browser
void _escape_html_string(const char* src, char* dest, size_t max_len) {
    if (!src || !dest || max_len == 0) return;
    size_t i = 0;
    while (*src && i < max_len - 6) { // -6 lässt Platz für &quot; + \0
        switch (*src) {
        case '<': strcpy(&dest[i], "&lt;"); i += 4; break;
        case '>': strcpy(&dest[i], "&gt;"); i += 4; break;
        case '&': strcpy(&dest[i], "&amp;"); i += 5; break;
        case '"': strcpy(&dest[i], "&quot;"); i += 6; break;
        case '\'': strcpy(&dest[i], "&#39;"); i += 5; break;
        default: dest[i++] = *src; break;
        }
        src++;
    }
    dest[i] = '\0';
}

// --- NAV GENERATOR (Dynamic Buttons) ---

void _print_nav_button(FILE* dest, const char* filename, int is_doc) {
    if (!dest || !filename) return;

    char raw_name[256];
    strncpy(raw_name, filename, 255);
    raw_name[255] = '\0'; // Buffer Overrun Protection

    char* dot = strrchr(raw_name, '.');
    if (dot) *dot = '\0';

    char* clean_name = raw_name;
    // Buffer Underrun Protection: Prüfe Länge, bevor auf Index 0,1,2 zugegriffen wird
    if (strlen(clean_name) >= 3 &&
        isdigit((unsigned char)clean_name[0]) &&
        isdigit((unsigned char)clean_name[1]) &&
        clean_name[2] == '_') {
        clean_name += 3;
    }

    // 1. Sanitize for JS and ID Execution (Verhindert XSS via onclick='...')
    char safe_id[256];
    _sanitize_for_id(clean_name, safe_id, sizeof(safe_id));

    char js_id[512];
    if (is_doc) {
        snprintf(js_id, sizeof(js_id), "doc_%s", safe_id);
    }
    else {
        snprintf(js_id, sizeof(js_id), "%s", safe_id);
    }

    // 2. Generate Label safely
    char raw_label[256];
    char* start = clean_name;
    if (strncmp(start, "view_", 5) == 0) start += 5;

    int i = 0;
    // Hard limit against buffer overrun during array generation
    for (; *start && i < 254; start++) {
        if (*start == '_') raw_label[i] = ' ';
        else raw_label[i] = toupper((unsigned char)*start);
        i++;
    }
    raw_label[i] = '\0';

    // 3. HTML Escape the Label
    char safe_label[1024];
    _escape_html_string(raw_label, safe_label, sizeof(safe_label));

    fprintf(dest, "<button class=\"nav-btn\" onclick=\"app.navigate('%s')\">%s</button>\n", js_id, safe_label);
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

// --- VERARBEITUNG ---

void _process_dir(FILE* dest, const char* base, const char* sub, const char* ext, const char* pre, const char* post) {
    if (!dest || !base || !sub) return;

    char dirpath[512]; snprintf(dirpath, sizeof(dirpath), "%s/%s", base, sub);
    FileList list = get_file_list_os(dirpath, ext);
    if (list.count == 0) return;

    qsort(list.files, list.count, sizeof(char*), compare_filenames);
    for (int i = 0; i < list.count; i++) {
        if (!list.files[i]) continue;
        char path[1024]; snprintf(path, sizeof(path), "%s/%s", dirpath, list.files[i]);
        if (pre) fprintf(dest, "%s\n", pre);
        _append_raw(dest, path);
        if (post) fprintf(dest, "%s\n", post);
    }
    free_file_list(list);
}

void _process_docs(FILE* dest, const char* base) {
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
        raw_name[255] = '\0'; // Protection

        char* dot = strrchr(raw_name, '.');
        if (dot) *dot = 0;

        char* clean_name = raw_name;
        // Underrun protection
        if (strlen(clean_name) >= 3 && isdigit((unsigned char)clean_name[0]) && isdigit((unsigned char)clean_name[1]) && clean_name[2] == '_') {
            clean_name += 3;
        }

        // Sanitize for ID mapping
        char safe_id[256];
        _sanitize_for_id(clean_name, safe_id, sizeof(safe_id));

        char vid[512];
        snprintf(vid, sizeof(vid), "doc_%s", safe_id);

        md_parse_file(dest, path, vid);
    }
    free_file_list(list);
}

// --- API ---

PROWB_API const char* prowb_version() { return "4.0.0-GFM-Industrial-SECURE"; }

PROWB_API PROWB_RESULT prowb_build(const char* src_dir, const char* out_base, const char* wasm_name) {
    if (!src_dir || !out_base || !wasm_name) return 1;

    char target_dir[512];
    char index_path[1024];

    // Sanitize wasm_name to prevent directory traversal
    char safe_wasm_name[256];
    _sanitize_for_id(wasm_name, safe_wasm_name, sizeof(safe_wasm_name));

    snprintf(target_dir, sizeof(target_dir), "%s/%s", out_base, safe_wasm_name);
    MKDIR(out_base);
    MKDIR(target_dir);
    snprintf(index_path, sizeof(index_path), "%s/index.html", target_dir);

    printf("[ProWB] Build Start: %s -> %s\n", safe_wasm_name, index_path);
    FILE* out = fopen(index_path, "w");
    if (!out) return 1;

    char tmp[1024];

    snprintf(tmp, sizeof(tmp), "%s/parts/header.html", src_dir); _append_raw(out, tmp);
    _process_dir(out, src_dir, "css", ".css", "<style>", "</style>");
    _process_nav_template(out, src_dir);
    _process_dir(out, src_dir, "views", ".html", "", "");
    _process_docs(out, src_dir);

    fprintf(out, "<script>const PRO_WASM_CORE = '%s';</script>\n", safe_wasm_name);

    _process_dir(out, src_dir, "js", ".js", "<script>", "</script>");
    snprintf(tmp, sizeof(tmp), "%s/parts/footer.html", src_dir); _append_raw(out, tmp);

    fclose(out);
    printf("[ProWB] Build erfolgreich.\n");
    return 0;
}
#ifdef PROWB_STANDALONE
int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <src_dir> <out_base> <wasm_name>\n", argv[0]);
        return 1;
    }
    return prowb_build(argv[1], argv[2], argv[3]);
}
#endif