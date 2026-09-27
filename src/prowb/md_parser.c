#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "md_parser.h"

#define LINE_SIZE 8192 // Puffer vergrößert für Sicherheit gegen lange Zeilen
#define MAX_TABLE_COLS 32 // Hard-Limit für Tabellenspalten

// --- HILFSFUNKTIONEN ---
void _print_escaped(FILE* dest, const char* src) {
    if (!src || !dest) return;
    while (*src) {
        switch (*src) {
        case '<': fprintf(dest, "&lt;"); break;
        case '>': fprintf(dest, "&gt;"); break;
        case '&': fprintf(dest, "&amp;"); break;
        case '"': fprintf(dest, "&quot;"); break;
        default: fputc(*src, dest);
        }
        src++;
    }
}

void _parse_inline(FILE* dest, const char* src);

// --- TABELLEN PARSER (GFM Alignment Support & Bounds Checked) ---
void _parse_table_row(FILE* dest, char* line, int is_header, int* aligns) {
    if (!line || !dest) return;
    fprintf(dest, "<tr>");
    char* ptr = line;
    if (*ptr == '|') ptr++;

    int col = 0;
    char* end;
    while ((end = strchr(ptr, '|')) != NULL && col < MAX_TABLE_COLS) {
        *end = '\0'; // Zerteile String sicher

        char* cell = ptr;
        while (*cell == ' ' || *cell == '\t') cell++; // Trim Left

        const char* align_str = "";
        if (!is_header && aligns) {
            if (aligns[col] == 1) align_str = " style=\"text-align:center;\"";
            else if (aligns[col] == 2) align_str = " style=\"text-align:right;\"";
        }

        fprintf(dest, is_header ? "<th scope=\"col\">" : "<td%s>", align_str);
        _parse_inline(dest, cell);
        fprintf(dest, is_header ? "</th>" : "</td>");

        ptr = end + 1;
        col++;
    }
    fprintf(dest, "</tr>\n");
}

// --- INLINE PARSER (Overrun & Underrun Protected) ---
void _parse_inline(FILE* dest, const char* src) {
    if (!src || !dest) return;

    while (*src) {
        // 1. INLINE CODE
        if (src[0] == '`') {
            const char* end = strchr(src + 1, '`');
            if (end) {
                int len = (int)(end - (src + 1));
                if (len > 0 && len < LINE_SIZE) { // Bounds Check
                    char* temp = malloc(len + 1);
                    if (temp) { // OOM Check
                        strncpy(temp, src + 1, len); temp[len] = '\0';
                        fprintf(dest, "<code>");
                        _print_escaped(dest, temp);
                        fprintf(dest, "</code>");
                        free(temp);
                        src = end + 1; continue;
                    }
                }
            }
        }

        // 2. BILDER
        if (src[0] == '!' && src[1] == '[') {
            const char* mid = strstr(src + 2, "](");
            if (mid) {
                const char* url_end = strchr(mid + 2, ')');
                if (url_end) {
                    int url_len = (int)(url_end - (mid + 2));
                    int alt_len = (int)(mid - (src + 2));
                    // Check against negative lengths (Underrun) and insane sizes (Overrun)
                    if (url_len >= 0 && url_len < LINE_SIZE && alt_len >= 0 && alt_len < LINE_SIZE) {
                        fprintf(dest, "<img src=\"%.*s\" alt=\"%.*s\" class=\"md-img\">",
                            url_len, mid + 2, alt_len, src + 2);
                        src = url_end + 1; continue;
                    }
                }
            }
        }

        // 3. LINKS
        if (src[0] == '[') {
            const char* mid = strstr(src + 1, "](");
            if (mid) {
                const char* url_end = strchr(mid + 2, ')');
                if (url_end) {
                    int txt_len = (int)(mid - (src + 1));
                    int url_len = (int)(url_end - (mid + 2));
                    if (txt_len > 0 && txt_len < LINE_SIZE && url_len >= 0 && url_len < LINE_SIZE) {
                        char* link_txt = malloc(txt_len + 1);
                        if (link_txt) {
                            strncpy(link_txt, src + 1, txt_len); link_txt[txt_len] = '\0';
                            fprintf(dest, "<a href=\"%.*s\">", url_len, mid + 2);
                            _parse_inline(dest, link_txt);
                            fprintf(dest, "</a>");
                            free(link_txt);
                            src = url_end + 1; continue;
                        }
                    }
                }
            }
        }

        // 4. FORMATIERUNG (Bold & Italic)
        if ((src[0] == '*' && src[1] == '*') || (src[0] == '_' && src[1] == '_')) {
            char token[3] = { src[0], src[1], '\0' };
            const char* end = strstr(src + 2, token);
            if (end) {
                int len = (int)(end - (src + 2));
                if (len > 0 && len < LINE_SIZE) {
                    char* inner = malloc(len + 1);
                    if (inner) {
                        strncpy(inner, src + 2, len); inner[len] = '\0';
                        fprintf(dest, "<strong>");
                        _parse_inline(dest, inner);
                        fprintf(dest, "</strong>");
                        free(inner);
                        src = end + 2; continue;
                    }
                }
            }
        }

        if (src[0] == '*' || src[0] == '_') {
            const char* end = strchr(src + 1, src[0]);
            if (end && (end[1] != src[0])) {
                int len = (int)(end - (src + 1));
                if (len > 0 && len < LINE_SIZE) {
                    char* inner = malloc(len + 1);
                    if (inner) {
                        strncpy(inner, src + 1, len); inner[len] = '\0';
                        fprintf(dest, "<em>");
                        _parse_inline(dest, inner);
                        fprintf(dest, "</em>");
                        free(inner);
                        src = end + 1; continue;
                    }
                }
            }
        }

        // 5. STRIKETHROUGH
        if (src[0] == '~' && src[1] == '~') {
            const char* end = strstr(src + 2, "~~");
            if (end) {
                int len = (int)(end - (src + 2));
                if (len > 0 && len < LINE_SIZE) {
                    char* inner = malloc(len + 1);
                    if (inner) {
                        strncpy(inner, src + 2, len); inner[len] = '\0';
                        fprintf(dest, "<del>");
                        _parse_inline(dest, inner);
                        fprintf(dest, "</del>");
                        free(inner);
                        src = end + 2; continue;
                    }
                }
            }
        }

        // Fallback: Einzelnes Zeichen ausgeben
        fputc(*src, dest);
        src++;
    }
}

// --- MAIN PARSER ---
void md_parse_file(FILE* dest, const char* filepath, const char* view_id) {
    if (!filepath || !dest || !view_id) return;

    FILE* src = fopen(filepath, "r");
    if (!src) return;

    fprintf(dest, "<section id=\"%s\" class=\"view-content\" style=\"display:none;\">\n", view_id);
    fprintf(dest, "<div class=\"markdown-body\">\n");

    char line[LINE_SIZE];

    int in_list = 0;
    int in_code = 0;
    int in_quote = 0;
    int in_table = 0;
    int in_p = 0;
    int table_align[MAX_TABLE_COLS] = { 0 };

    // fgets schützt nativ vor Buffer Overruns, da es maximal LINE_SIZE-1 Bytes liest
    while (fgets(line, sizeof(line), src)) {
        line[strcspn(line, "\n")] = 0; // Sicheres Entfernen von Newline
        int len = strlen(line);

        // 1. CODE BLOCKS
        if (strncmp(line, "```", 3) == 0) {
            if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }
            if (in_list) { fprintf(dest, in_list == 1 ? "</ul>\n" : "</ol>\n"); in_list = 0; }
            if (in_code) { fprintf(dest, "</code></pre>\n"); in_code = 0; }
            else {
                char lang[64] = { 0 };
                char* lptr = line + 3;
                while (*lptr == ' ' || *lptr == '\t') lptr++;
                if (*lptr) {
                    // Verhindert Overrun des lang arrays
                    strncpy(lang, lptr, sizeof(lang) - 1);
                    lang[sizeof(lang) - 1] = '\0';
                    char* crlf = strpbrk(lang, "\r\n ");
                    if (crlf) *crlf = '\0';
                }
                if (strlen(lang) > 0) fprintf(dest, "<pre><code class=\"language-%s\">", lang);
                else fprintf(dest, "<pre><code>");
                in_code = 1;
            }
            continue;
        }

        if (in_code) {
            _print_escaped(dest, line);
            fputc('\n', dest);
            continue;
        }

        // Leere Zeilen schließen alle Blöcke sicher ab
        if (len == 0) {
            if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }
            if (in_list) { fprintf(dest, in_list == 1 ? "</ul>\n" : "</ol>\n"); in_list = 0; }
            if (in_quote) { fprintf(dest, "</blockquote>\n"); in_quote = 0; }
            if (in_table) { fprintf(dest, "</tbody>\n</table>\n</div>\n"); in_table = 0; }
            continue;
        }

        // 2. TABELLEN
        if (line[0] == '|') {
            if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }
            if (in_list) { fprintf(dest, in_list == 1 ? "</ul>\n" : "</ol>\n"); in_list = 0; }

            if (in_table == 0) {
                fprintf(dest, "<div class=\"table-responsive\">\n<table>\n<thead>\n");
                _parse_table_row(dest, line, 1, NULL);
                in_table = 1;
            }
            else if (in_table == 1 && strstr(line, "---")) {
                char* ptr = line; if (*ptr == '|') ptr++;
                int col = 0; char* end;
                while ((end = strchr(ptr, '|')) != NULL && col < MAX_TABLE_COLS) {
                    *end = '\0'; char* cell = ptr;
                    while (*cell == ' ' || *cell == '\t') cell++;
                    int clen = strlen(cell);
                    while (clen > 0 && (cell[clen - 1] == ' ' || cell[clen - 1] == '\t')) { cell[clen - 1] = '\0'; clen--; }
                    if (clen > 0) {
                        int left = (cell[0] == ':'); int right = (cell[clen - 1] == ':');
                        if (left && right) table_align[col] = 1;
                        else if (right) table_align[col] = 2;
                        else table_align[col] = 0;
                    }
                    ptr = end + 1; col++;
                }
                fprintf(dest, "</thead>\n<tbody>\n");
                in_table = 2;
            }
            else {
                _parse_table_row(dest, line, 0, table_align);
            }
            continue;
        }
        else if (in_table != 0) {
            fprintf(dest, "</tbody>\n</table>\n</div>\n");
            in_table = 0;
        }

        // 3. HORIZONTALE LINIE
        if (strncmp(line, "---", 3) == 0) {
            if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }
            fprintf(dest, "<hr>\n");
            continue;
        }

        // 4. ÜBERSCHRIFTEN (GFM & Bounds Safe)
        char* h_ptr = line;
        while (*h_ptr == ' ' || *h_ptr == '\t') h_ptr++;
        if (h_ptr[0] == '#') {
            int lvl = 0;
            while (h_ptr[lvl] == '#' && lvl < 6) lvl++;

            // Bounds-Check: Stellen sicher, dass wir nicht über das Ende des Strings lesen
            if ((int)strlen(h_ptr) > lvl && h_ptr[lvl] == ' ') {
                if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }
                if (in_list) { fprintf(dest, in_list == 1 ? "</ul>\n" : "</ol>\n"); in_list = 0; }

                char* content = h_ptr + lvl;
                while (*content == ' ') content++;

                fprintf(dest, "<h%d>", lvl);
                _parse_inline(dest, content);
                fprintf(dest, "</h%d>\n", lvl);
                continue;
            }
        }

        // 5. BLOCKQUOTES
        if (line[0] == '>') {
            if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }
            if (!in_quote) { fprintf(dest, "<blockquote>\n"); in_quote = 1; }
            char* content = line + 1; while (*content == ' ') content++;
            _parse_inline(dest, content);
            fprintf(dest, "<br>\n");
            continue;
        }
        else if (in_quote) {
            fprintf(dest, "</blockquote>\n");
            in_quote = 0;
        }

        // 6. GFM LISTEN (Nummeriert & Unnummeriert / Bounds Protected)
        char* list_ptr = line;
        while (*list_ptr == ' ' || *list_ptr == '\t') list_ptr++;

        int list_len = strlen(list_ptr);
        int is_ul = (list_len >= 2 && (strncmp(list_ptr, "- ", 2) == 0 || strncmp(list_ptr, "* ", 2) == 0 || strncmp(list_ptr, "+ ", 2) == 0));

        int is_ol = 0;
        char* ol_start = list_ptr;
        if (!is_ul && isdigit((unsigned char)*ol_start)) {
            char* ol_ptr = ol_start;
            while (isdigit((unsigned char)*ol_ptr)) ol_ptr++;
            // Längenprüfung gegen Buffer Underrun / Out of bounds read
            if ((int)strlen(ol_ptr) >= 2 && (*ol_ptr == '.' || *ol_ptr == ')') && ol_ptr[1] == ' ') {
                is_ol = 1;
                list_ptr = ol_ptr;
            }
        }

        if (is_ul || is_ol) {
            if (in_p) { fprintf(dest, "</p>\n"); in_p = 0; }

            int target_list = is_ul ? 1 : 2;

            if (in_list && in_list != target_list) {
                fprintf(dest, in_list == 1 ? "</ul>\n" : "</ol>\n");
                in_list = 0;
            }

            if (!in_list) {
                fprintf(dest, target_list == 1 ? "<ul>\n" : "<ol>\n");
                in_list = target_list;
            }

            char* content = list_ptr + 2;
            int is_task = 0; int is_checked = 0;

            // Task Listenprüfung mit Bounds
            if (strlen(content) >= 4) {
                if (strncmp(content, "[ ] ", 4) == 0) { is_task = 1; content += 4; }
                else if (strncmp(content, "[x] ", 4) == 0 || strncmp(content, "[X] ", 4) == 0) { is_task = 1; is_checked = 1; content += 4; }
            }

            fprintf(dest, "<li%s>", is_task ? " class=\"task-list-item\" style=\"list-style-type: none;\"" : "");
            if (is_task) {
                fprintf(dest, "<input type=\"checkbox\" disabled %s> ", is_checked ? "checked" : "");
            }
            _parse_inline(dest, content);
            fprintf(dest, "</li>\n");
            continue;
        }

        // 7. ABSÄTZE (Mehrzeilig)
        if (!in_p) {
            fprintf(dest, "<p>");
            in_p = 1;
        }
        else {
            fprintf(dest, " ");
        }
        _parse_inline(dest, line);
        fprintf(dest, "\n");
    }

    // Unclosed Blocks am Ende des Files sicher aufräumen
    if (in_p) fprintf(dest, "</p>\n");
    if (in_list) fprintf(dest, in_list == 1 ? "</ul>\n" : "</ol>\n");
    if (in_quote) fprintf(dest, "</blockquote>\n");
    if (in_table) fprintf(dest, "</tbody>\n</table>\n</div>\n");
    if (in_code) fprintf(dest, "</code></pre>\n");

    fprintf(dest, "</div>\n</section>\n");
    fclose(src);
}