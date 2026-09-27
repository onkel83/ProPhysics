#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "md_parser.h"

/* ============================================================
 *  MSVC-KOMPATIBILITÄT
 * ============================================================ */
#ifdef _WIN32
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#ifndef strcasecmp
#define strcasecmp  _stricmp
#endif
#endif

 /* ============================================================
  *  HARTE LIMITS
  * ============================================================ */
#define LINE_BUF           8192
#define MAX_TABLE_COLS     32
#define MAX_LINES          65536
#define MAX_FILE_BYTES     (4 * 1024 * 1024)
#define MAX_HEADINGS       512
#define MAX_REFDEFS        256
#define MAX_SLUG_LEN       128
#define MAX_FULLID_LEN     256

  /* Rekursionsschutz für _parse_inline: verschachtelte Marker
   * (Bold-in-Link-in-Emphasis ...) können sonst den Stack sprengen. */
#define MAX_INLINE_DEPTH   32

   /* Kleinere Arbeitsbuffer für Inline-Pfade -- die 8K-Puffer waren
    * der zweite Hauptgrund für Stack-Overflow bei Rekursion. */
#define INLINE_URL_BUF     1024
#define INLINE_ALT_BUF     512
#define INLINE_TEXT_BUF    1024

    /* ============================================================
     *  STATE
     * ============================================================ */

typedef struct {
    char base[MAX_SLUG_LEN];
    char full[MAX_FULLID_LEN];
} HeadingEntry;

typedef struct {
    char label[256];
    char url[1024];
    char title[256];
} RefDef;

typedef struct {
    RefDef        refdefs[MAX_REFDEFS];
    int           refdef_count;

    HeadingEntry  headings[MAX_HEADINGS];
    int           heading_count;

    const MdConfig* cfg;
    char            view_id[128];
} MdState;

/* ============================================================
 *  KLEINE HELFER
 * ============================================================ */

static void _trim_inplace(char* s) {
    if (!s) return;
    char* p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    int len = (int)strlen(s);
    while (len > 0) {
        char c = s[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') s[--len] = '\0';
        else break;
    }
}

static void _print_escaped(FILE* dest, const char* src) {
    if (!dest || !src) return;
    while (*src) {
        switch (*src) {
        case '<': fputs("&lt;", dest); break;
        case '>': fputs("&gt;", dest); break;
        case '&': fputs("&amp;", dest); break;
        case '"': fputs("&quot;", dest); break;
        default:  fputc(*src, dest);
        }
        src++;
    }
}

static void _print_escaped_attr(FILE* dest, const char* src) {
    if (!dest || !src) return;
    while (*src) {
        switch (*src) {
        case '<': fputs("&lt;", dest); break;
        case '>': fputs("&gt;", dest); break;
        case '&': fputs("&amp;", dest); break;
        case '"': fputs("&quot;", dest); break;
        case '\'':fputs("&#39;", dest); break;
        default:  fputc(*src, dest);
        }
        src++;
    }
}

/* ============================================================
 *  CONFIG-LOADER
 * ============================================================ */

void md_config_init(MdConfig* cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(*cfg));
}

int md_config_load_emoji(MdConfig* cfg, const char* path) {
    if (!cfg || !path) return 0;
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        _trim_inplace(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        if (cfg->emoji_count >= MD_MAX_EMOJIS) break;

        char* sep = strchr(line, '|');
        if (!sep) continue;
        *sep = '\0';

        char* code = line;
        char* repl = sep + 1;
        _trim_inplace(code);
        _trim_inplace(repl);

        size_t clen = strlen(code);
        if (clen < 3 || code[0] != ':' || code[clen - 1] != ':') continue;
        code[clen - 1] = '\0';
        code++;

        if (*code == '\0' || *repl == '\0') continue;

        MdEmojiEntry* e = &cfg->emojis[cfg->emoji_count];
        strncpy(e->shortcode, code, sizeof(e->shortcode) - 1);
        e->shortcode[sizeof(e->shortcode) - 1] = '\0';
        strncpy(e->replacement, repl, sizeof(e->replacement) - 1);
        e->replacement[sizeof(e->replacement) - 1] = '\0';
        cfg->emoji_count++;
    }
    fclose(f);
    return 1;
}

int md_config_load_whitelist(MdConfig* cfg, const char* path) {
    if (!cfg || !path) return 0;
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        _trim_inplace(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        if (cfg->allowed_tag_count >= MD_MAX_TAGS) break;

        char* colon = strchr(line, ':');
        char tagbuf[MD_MAX_TAG_LEN] = { 0 };
        char attrbuf[MD_MAX_TAG_ATTRS] = { 0 };

        if (colon) {
            *colon = '\0';
            strncpy(tagbuf, line, sizeof(tagbuf) - 1);
            strncpy(attrbuf, colon + 1, sizeof(attrbuf) - 1);
            _trim_inplace(tagbuf);
            _trim_inplace(attrbuf);
        }
        else {
            strncpy(tagbuf, line, sizeof(tagbuf) - 1);
        }

        int ok = (*tagbuf != '\0');
        for (char* p = tagbuf; *p; p++) {
            if (!(isalnum((unsigned char)*p) || *p == '-')) { ok = 0; break; }
        }
        if (!ok) continue;

        if (strcmp(tagbuf, "script") == 0 ||
            strcmp(tagbuf, "style") == 0 ||
            strcmp(tagbuf, "iframe") == 0 ||
            strcmp(tagbuf, "object") == 0 ||
            strcmp(tagbuf, "embed") == 0) continue;

        char safe_attrs[MD_MAX_TAG_ATTRS] = { 0 };
        size_t sa = 0;
        if (attrbuf[0]) {
            char tmp[MD_MAX_TAG_ATTRS];
            strncpy(tmp, attrbuf, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = '\0';
            char* tok = strtok(tmp, ",");
            int first = 1;
            while (tok) {
                _trim_inplace(tok);
                int bad = 0;
                if (tok[0] == '\0') bad = 1;
                else if (strncmp(tok, "on", 2) == 0) bad = 1;
                else if (strcmp(tok, "style") == 0) bad = 1;
                else if (strcmp(tok, "formaction") == 0) bad = 1;
                else if (strcmp(tok, "srcdoc") == 0) bad = 1;
                else {
                    for (char* q = tok; *q; q++) {
                        if (!(isalnum((unsigned char)*q) || *q == '-' || *q == '_')) { bad = 1; break; }
                    }
                }
                if (!bad) {
                    size_t need = strlen(tok);
                    if (sa + need + 2 < sizeof(safe_attrs)) {
                        if (!first) safe_attrs[sa++] = ',';
                        memcpy(safe_attrs + sa, tok, need);
                        sa += need;
                        safe_attrs[sa] = '\0';
                        first = 0;
                    }
                }
                tok = strtok(NULL, ",");
            }
        }

        MdAllowedTag* e = &cfg->allowed_tags[cfg->allowed_tag_count];
        strncpy(e->tag, tagbuf, sizeof(e->tag) - 1);
        e->tag[sizeof(e->tag) - 1] = '\0';
        strncpy(e->attrs, safe_attrs, sizeof(e->attrs) - 1);
        e->attrs[sizeof(e->attrs) - 1] = '\0';
        cfg->allowed_tag_count++;
    }
    fclose(f);
    return 1;
}

int md_config_load_crosslinks(MdConfig* cfg, const char* path) {
    if (!cfg || !path) return 0;
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        _trim_inplace(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        if (cfg->crosslink_count >= MD_MAX_CROSSLINKS) break;

        char* p1 = strchr(line, '|');
        if (!p1) continue;
        char* p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        *p1 = '\0';
        *p2 = '\0';

        char* sec = line;
        char* name = p1 + 1;
        char* ort = p2 + 1;
        _trim_inplace(sec);
        _trim_inplace(name);
        _trim_inplace(ort);

        if (*sec == '\0' || *name == '\0' || *ort == '\0') continue;

        MdCrossLink* e = &cfg->crosslinks[cfg->crosslink_count];
        strncpy(e->section, sec, sizeof(e->section) - 1);  e->section[sizeof(e->section) - 1] = '\0';
        strncpy(e->doc_id, name, sizeof(e->doc_id) - 1);   e->doc_id[sizeof(e->doc_id) - 1] = '\0';
        strncpy(e->path, ort, sizeof(e->path) - 1);     e->path[sizeof(e->path) - 1] = '\0';
        cfg->crosslink_count++;
    }
    fclose(f);
    return 1;
}

/* ============================================================
 *  CONFIG-LOOKUP
 * ============================================================ */

static const char* _emoji_lookup(const MdConfig* cfg, const char* name) {
    if (!cfg || !name) return NULL;
    for (int i = 0; i < cfg->emoji_count; i++)
        if (strcmp(cfg->emojis[i].shortcode, name) == 0)
            return cfg->emojis[i].replacement;
    return NULL;
}

static const MdAllowedTag* _tag_lookup(const MdConfig* cfg, const char* name) {
    if (!cfg || !name) return NULL;
    for (int i = 0; i < cfg->allowed_tag_count; i++)
        if (strcmp(cfg->allowed_tags[i].tag, name) == 0)
            return &cfg->allowed_tags[i];
    return NULL;
}

static const MdCrossLink* _crosslink_lookup_path(const MdConfig* cfg, const char* path) {
    if (!cfg || !path) return NULL;
    for (int i = 0; i < cfg->crosslink_count; i++)
        if (strcmp(cfg->crosslinks[i].path, path) == 0)
            return &cfg->crosslinks[i];
    return NULL;
}

/* ============================================================
 *  SLUGIFY (GitHub-kompatibel, UTF-8-safe)
 * ============================================================ */
static void _slugify(const char* src, char* out, size_t out_max) {
    if (!src || !out || out_max == 0) return;
    size_t i = 0;
    int last_dash = 0;

    while (*src && i + 1 < out_max) {
        unsigned char c = (unsigned char)*src;
        if (c >= 0x80) {
            out[i++] = (char)c;
            last_dash = 0;
        }
        else if (isalnum(c)) {
            out[i++] = (char)tolower(c);
            last_dash = 0;
        }
        else if (c == '_') {
            out[i++] = '_';
            last_dash = 0;
        }
        else if (c == ' ' || c == '-') {
            if (!last_dash && i > 0) { out[i++] = '-'; last_dash = 1; }
        }
        src++;
    }
    while (i > 0 && out[i - 1] == '-') i--;
    out[i] = '\0';
}

/* ============================================================
 *  HEADING-REGISTRY
 * ============================================================ */

static int _heading_lookup_base(const MdState* st, const char* base) {
    for (int i = 0; i < st->heading_count; i++)
        if (strcmp(st->headings[i].base, base) == 0) return 1;
    return 0;
}

static int _heading_count_base(const MdState* st, const char* base) {
    int n = 0;
    for (int i = 0; i < st->heading_count; i++)
        if (strcmp(st->headings[i].base, base) == 0) n++;
    return n;
}

static void _heading_add(MdState* st, const char* base) {
    if (!st || st->heading_count >= MAX_HEADINGS) return;
    HeadingEntry* e = &st->headings[st->heading_count];
    strncpy(e->base, base, sizeof(e->base) - 1);
    e->base[sizeof(e->base) - 1] = '\0';

    int prior = _heading_count_base(st, base);
    if (prior == 0) {
        snprintf(e->full, sizeof(e->full), "%s-%s", st->view_id, base);
    }
    else {
        snprintf(e->full, sizeof(e->full), "%s-%s-%d", st->view_id, base, prior);
    }
    st->heading_count++;
}

static const HeadingEntry* _heading_find_first(const MdState* st, const char* base) {
    for (int i = 0; i < st->heading_count; i++)
        if (strcmp(st->headings[i].base, base) == 0) return &st->headings[i];
    return NULL;
}

/* ============================================================
 *  REFDEFS
 * ============================================================ */

static void _normalize_label(const char* src, char* out, size_t out_max) {
    size_t i = 0;
    while (*src && i + 1 < out_max) {
        unsigned char c = (unsigned char)*src;
        if (c >= 0x80) out[i++] = (char)tolower(c);
        else if (isalnum(c)) out[i++] = (char)tolower(c);
        else if (c == ' ' || c == '\t') out[i++] = ' ';
        src++;
    }
    while (i > 0 && out[i - 1] == ' ') i--;
    out[i] = '\0';
}

static const RefDef* _refdef_lookup(const MdState* st, const char* label) {
    if (!st || !label) return NULL;
    char norm[256];
    _normalize_label(label, norm, sizeof(norm));
    for (int i = 0; i < st->refdef_count; i++) {
        char cmp[256];
        _normalize_label(st->refdefs[i].label, cmp, sizeof(cmp));
        if (strcmp(cmp, norm) == 0) return &st->refdefs[i];
    }
    return NULL;
}

/* ============================================================
 *  INLINE PARSER
 * ============================================================ */

static void _parse_inline_depth(FILE* dest, const char* src, MdState* st, int depth);

static void _parse_inline(FILE* dest, const char* src, MdState* st) {
    _parse_inline_depth(dest, src, st, 0);
}

static void _emit_href(FILE* dest, const char* url, MdState* st) {
    if (!url) { fputs("href=\"\"", dest); return; }

    if (url[0] == '#') {
        const char* slug = url + 1;
        if (st && _heading_lookup_base(st, slug)) {
            const HeadingEntry* h = _heading_find_first(st, slug);
            if (h) { fputs("href=\"#", dest); _print_escaped_attr(dest, h->full); fputs("\"", dest); return; }
        }
        fputs("href=\"#", dest); _print_escaped_attr(dest, slug); fputs("\"", dest);
        return;
    }

    if (strncmp(url, "http://", 7) == 0 ||
        strncmp(url, "https://", 8) == 0 ||
        strncmp(url, "mailto:", 7) == 0 ||
        strncmp(url, "//", 2) == 0 ||
        strncmp(url, "ftp://", 6) == 0) {
        fputs("href=\"", dest); _print_escaped_attr(dest, url); fputs("\"", dest);
        return;
    }

    const char* hash = strchr(url, '#');
    size_t path_len = hash ? (size_t)(hash - url) : strlen(url);

    char pathbuf[MD_MAX_PATH_LEN];
    if (path_len >= sizeof(pathbuf)) path_len = sizeof(pathbuf) - 1;
    memcpy(pathbuf, url, path_len);
    pathbuf[path_len] = '\0';

    const MdCrossLink* cl = (st && st->cfg) ? _crosslink_lookup_path(st->cfg, pathbuf) : NULL;

    if (cl) {
        fputs("href=\"#doc_", dest);
        _print_escaped_attr(dest, cl->section);
        fputc('_', dest);
        _print_escaped_attr(dest, cl->doc_id);
        if (hash && hash[1]) {
            fputc('-', dest);
            _print_escaped_attr(dest, hash + 1);
        }
        fputs("\"", dest);
        return;
    }

    fputs("href=\"", dest); _print_escaped_attr(dest, url); fputs("\"", dest);
}

static void _emit_text_span(FILE* dest, const char* start, size_t len, MdState* st, int depth) {
    if (len == 0) return;
    char tmp[INLINE_TEXT_BUF];
    if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
    memcpy(tmp, start, len);
    tmp[len] = '\0';
    _parse_inline_depth(dest, tmp, st, depth + 1);
}

static int _try_html_tag(FILE* dest, const char* src, size_t* pos, MdState* st) {
    if (!st || !st->cfg || !st->cfg->allowed_tag_count) return 0;
    size_t i = *pos;
    if (src[i] != '<') return 0;

    size_t j = i + 1;
    int closing = 0;
    if (src[j] == '/') { closing = 1; j++; }

    char tagname[MD_MAX_TAG_LEN];
    size_t tn = 0;
    while (src[j] && (isalnum((unsigned char)src[j]) || src[j] == '-') && tn + 1 < sizeof(tagname)) {
        tagname[tn++] = (char)tolower((unsigned char)src[j]);
        j++;
    }
    tagname[tn] = '\0';
    if (tn == 0) return 0;

    const MdAllowedTag* tag = _tag_lookup(st->cfg, tagname);
    if (!tag) return 0;

    if (closing) {
        if (src[j] != '>') return 0;
        fprintf(dest, "</%s>", tag->tag);
        *pos = j + 1;
        return 1;
    }

    char attrs_out[MD_MAX_TAG_ATTRS * 2] = { 0 };
    size_t ao = 0;
    int self_closing = 0;

    while (src[j] && src[j] != '>') {
        while (src[j] == ' ' || src[j] == '\t') j++;
        if (src[j] == '>') break;
        if (src[j] == '/' && src[j + 1] == '>') { self_closing = 1; j++; break; }

        char an[64];
        size_t anl = 0;
        while (src[j] && (isalnum((unsigned char)src[j]) || src[j] == '-' || src[j] == '_') && anl + 1 < sizeof(an)) {
            an[anl++] = (char)tolower((unsigned char)src[j]);
            j++;
        }
        an[anl] = '\0';
        if (anl == 0) return 0;

        if (strncmp(an, "on", 2) == 0) return 0;
        if (strcmp(an, "style") == 0) return 0;

        int allowed = 0;
        if (tag->attrs[0]) {
            char tmp_attrs[MD_MAX_TAG_ATTRS];
            strncpy(tmp_attrs, tag->attrs, sizeof(tmp_attrs) - 1);
            tmp_attrs[sizeof(tmp_attrs) - 1] = '\0';
            char* tok = strtok(tmp_attrs, ",");
            while (tok) {
                _trim_inplace(tok);
                if (strcmp(tok, an) == 0) { allowed = 1; break; }
                tok = strtok(NULL, ",");
            }
        }
        if (!allowed) return 0;

        while (src[j] == ' ' || src[j] == '\t') j++;
        if (src[j] != '=') return 0;
        j++;
        while (src[j] == ' ' || src[j] == '\t') j++;

        char quote = 0;
        if (src[j] == '"' || src[j] == '\'') { quote = src[j]; j++; }

        char av[512];
        size_t avl = 0;
        while (src[j] && avl + 1 < sizeof(av)) {
            if (quote && src[j] == quote) { j++; break; }
            if (!quote && (src[j] == ' ' || src[j] == '>' || src[j] == '/')) break;
            av[avl++] = src[j++];
        }
        av[avl] = '\0';

        if (strncasecmp(av, "javascript:", 11) == 0) return 0;
        if (strncasecmp(av, "vbscript:", 9) == 0) return 0;
        if (strncasecmp(av, "data:", 5) == 0) return 0;

        if (ao + strlen(an) + avl + 8 < sizeof(attrs_out)) {
            ao += (size_t)snprintf(attrs_out + ao, sizeof(attrs_out) - ao,
                " %s=\"", an);
            for (size_t k = 0; k < avl && ao + 6 < sizeof(attrs_out); k++) {
                char c = av[k];
                if (c == '"') { memcpy(attrs_out + ao, "&quot;", 6); ao += 6; }
                else if (c == '&') { memcpy(attrs_out + ao, "&amp;", 5); ao += 5; }
                else if (c == '<') { memcpy(attrs_out + ao, "&lt;", 4); ao += 4; }
                else if (c == '>') { memcpy(attrs_out + ao, "&gt;", 4); ao += 4; }
                else { attrs_out[ao++] = c; }
            }
            if (ao < sizeof(attrs_out) - 2) { attrs_out[ao++] = '"'; attrs_out[ao] = '\0'; }
        }
    }

    if (src[j] != '>') return 0;

    fprintf(dest, "<%s%s%s>", tag->tag, attrs_out, self_closing ? " /" : "");
    *pos = j + 1;
    return 1;
}

static void _parse_inline_depth(FILE* dest, const char* src, MdState* st, int depth) {
    if (!src || !dest) return;

    /* Rekursionstiefe begrenzt -- bei Überschreitung wird der Inhalt
     * als Plain-Text ausgegeben. Verhindert Stack-Overflow bei
     * pathologisch verschachtelten Markern. */
    if (depth > MAX_INLINE_DEPTH) {
        _print_escaped(dest, src);
        return;
    }

    size_t i = 0;
    size_t n = strlen(src);

    while (i < n) {
        unsigned char c = (unsigned char)src[i];

        /* 1. CODE SPAN */
        if (c == '`') {
            size_t j = i + 1;
            while (j < n && src[j] == '`') j++;
            int ticks = (int)(j - i);
            size_t k = j;
            while (k < n) {
                if (src[k] == '`') {
                    int m = 0;
                    while (k + m < n && src[k + m] == '`') m++;
                    if (m == ticks) {
                        fputs("<code>", dest);
                        for (size_t p = j; p < k; p++) {
                            switch ((unsigned char)src[p]) {
                            case '<': fputs("&lt;", dest); break;
                            case '>': fputs("&gt;", dest); break;
                            case '&': fputs("&amp;", dest); break;
                            default:  fputc(src[p], dest);
                            }
                        }
                        fputs("</code>", dest);
                        i = k + ticks;
                        goto next_char;
                    }
                    k += m;
                }
                else k++;
            }
            fputc('`', dest); i++; continue;
        }

        /* 2. IMAGE  ![alt](url "title") */
        if (c == '!' && i + 1 < n && src[i + 1] == '[') {
            const char* mid = strstr(src + i + 2, "](");
            if (mid) {
                const char* url_start = mid + 2;
                const char* url_end = strchr(url_start, ')');
                if (url_end) {
                    size_t alt_len = (size_t)(mid - (src + i + 2));
                    size_t url_len = (size_t)(url_end - url_start);
                    if (url_len < INLINE_URL_BUF && alt_len < INLINE_ALT_BUF) {
                        char altbuf[INLINE_ALT_BUF], urlbuf[INLINE_URL_BUF];
                        memcpy(altbuf, src + i + 2, alt_len); altbuf[alt_len] = '\0';
                        memcpy(urlbuf, url_start, url_len);   urlbuf[url_len] = '\0';

                        char* title = NULL;
                        char* sp = strchr(urlbuf, ' ');
                        if (sp) {
                            *sp = '\0'; title = sp + 1;
                            size_t tl = strlen(title);
                            if (tl >= 2 && ((title[0] == '"' && title[tl - 1] == '"') ||
                                (title[0] == '\'' && title[tl - 1] == '\''))) {
                                title[tl - 1] = '\0'; title++;
                            }
                        }

                        fputs("<img src=\"", dest);
                        _print_escaped_attr(dest, urlbuf);
                        fputs("\" alt=\"", dest);
                        _print_escaped_attr(dest, altbuf);
                        fputc('"', dest);
                        if (title && *title) {
                            fputs(" title=\"", dest);
                            _print_escaped_attr(dest, title);
                            fputc('"', dest);
                        }
                        fputs(" class=\"md-img\">", dest);
                        i = (size_t)(url_end - src) + 1;
                        continue;
                    }
                }
            }
        }

        /* 3. LINK */
        if (c == '[') {
            /* A: [text](url) */
            const char* mid = strstr(src + i + 1, "](");
            if (mid && (mid - (src + i + 1)) < INLINE_TEXT_BUF) {
                const char* url_start = mid + 2;
                const char* url_end = strchr(url_start, ')');
                if (url_end) {
                    size_t txt_len = (size_t)(mid - (src + i + 1));
                    size_t url_len = (size_t)(url_end - url_start);
                    if (url_len < INLINE_URL_BUF && txt_len > 0) {
                        char urlbuf[INLINE_URL_BUF];
                        memcpy(urlbuf, url_start, url_len); urlbuf[url_len] = '\0';
                        char* title = NULL;
                        char* sp = strchr(urlbuf, ' ');
                        if (sp) {
                            *sp = '\0'; title = sp + 1;
                            size_t tl = strlen(title);
                            if (tl >= 2 && ((title[0] == '"' && title[tl - 1] == '"') ||
                                (title[0] == '\'' && title[tl - 1] == '\''))) {
                                title[tl - 1] = '\0'; title++;
                            }
                        }

                        fputs("<a ", dest);
                        _emit_href(dest, urlbuf, st);
                        if (title && *title) {
                            fputs(" title=\"", dest);
                            _print_escaped_attr(dest, title);
                            fputc('"', dest);
                        }
                        fputc('>', dest);
                        _emit_text_span(dest, src + i + 1, txt_len, st, depth);
                        fputs("</a>", dest);
                        i = (size_t)(url_end - src) + 1;
                        continue;
                    }
                }
            }

            /* B: [text][ref] */
            const char* mid_ref = strstr(src + i + 1, "][");
            if (mid_ref) {
                const char* ref_end = strchr(mid_ref + 2, ']');
                if (ref_end) {
                    size_t txt_len = (size_t)(mid_ref - (src + i + 1));
                    size_t ref_len = (size_t)(ref_end - (mid_ref + 2));
                    char refbuf[256];
                    if (ref_len < sizeof(refbuf)) {
                        memcpy(refbuf, mid_ref + 2, ref_len); refbuf[ref_len] = '\0';
                        const RefDef* rd = _refdef_lookup(st, refbuf);
                        if (rd) {
                            fputs("<a ", dest);
                            _emit_href(dest, rd->url, st);
                            if (rd->title[0]) {
                                fputs(" title=\"", dest);
                                _print_escaped_attr(dest, rd->title);
                                fputc('"', dest);
                            }
                            fputc('>', dest);
                            _emit_text_span(dest, src + i + 1, txt_len, st, depth);
                            fputs("</a>", dest);
                            i = (size_t)(ref_end - src) + 1;
                            continue;
                        }
                    }
                }
            }

            /* C: [ref] */
            const char* close = strchr(src + i + 1, ']');
            if (close) {
                size_t ref_len = (size_t)(close - (src + i + 1));
                char refbuf[256];
                if (ref_len > 0 && ref_len < sizeof(refbuf)) {
                    memcpy(refbuf, src + i + 1, ref_len); refbuf[ref_len] = '\0';
                    const RefDef* rd = _refdef_lookup(st, refbuf);
                    if (rd) {
                        fputs("<a ", dest);
                        _emit_href(dest, rd->url, st);
                        fputc('>', dest);
                        _emit_text_span(dest, src + i + 1, ref_len, st, depth);
                        fputs("</a>", dest);
                        i = (size_t)(close - src) + 1;
                        continue;
                    }
                }
            }
        }

        /* 4. BOLD */
        if ((c == '*' && i + 1 < n && src[i + 1] == '*') ||
            (c == '_' && i + 1 < n && src[i + 1] == '_')) {
            char tok[3] = { (char)c, (char)c, 0 };
            const char* end = strstr(src + i + 2, tok);
            if (end) {
                size_t len = (size_t)(end - (src + i + 2));
                if (len > 0 && len < INLINE_TEXT_BUF) {
                    fputs("<strong>", dest);
                    _emit_text_span(dest, src + i + 2, len, st, depth);
                    fputs("</strong>", dest);
                    i = (size_t)(end - src) + 2;
                    continue;
                }
            }
        }

        /* 5. ITALIC */
        if (c == '*' || c == '_') {
            const char* end = strchr(src + i + 1, c);
            if (end && end > src + i + 1) {
                if (!(end + 1 < src + n && end[1] == (char)c)) {
                    size_t len = (size_t)(end - (src + i + 1));
                    if (len > 0 && len < INLINE_TEXT_BUF) {
                        fputs("<em>", dest);
                        _emit_text_span(dest, src + i + 1, len, st, depth);
                        fputs("</em>", dest);
                        i = (size_t)(end - src) + 1;
                        continue;
                    }
                }
            }
        }

        /* 6. STRIKETHROUGH */
        if (c == '~' && i + 1 < n && src[i + 1] == '~') {
            const char* end = strstr(src + i + 2, "~~");
            if (end) {
                size_t len = (size_t)(end - (src + i + 2));
                if (len > 0 && len < INLINE_TEXT_BUF) {
                    fputs("<del>", dest);
                    _emit_text_span(dest, src + i + 2, len, st, depth);
                    fputs("</del>", dest);
                    i = (size_t)(end - src) + 2;
                    continue;
                }
            }
        }

        /* 7. AUTOLINK / WHITELIST-TAG */
        if (c == '<') {
            if (st && st->cfg && _try_html_tag(dest, src, &i, st)) continue;

            const char* gt = strchr(src + i + 1, '>');
            if (gt && (gt - (src + i + 1)) < 512) {
                size_t alen = (size_t)(gt - (src + i + 1));
                char abuf[512];
                memcpy(abuf, src + i + 1, alen); abuf[alen] = '\0';
                if (strncmp(abuf, "http://", 7) == 0 ||
                    strncmp(abuf, "https://", 8) == 0 ||
                    strncmp(abuf, "mailto:", 7) == 0) {
                    fputs("<a href=\"", dest);
                    _print_escaped_attr(dest, abuf);
                    fputs("\">", dest);
                    _print_escaped(dest, abuf);
                    fputs("</a>", dest);
                    i = (size_t)(gt - src) + 1;
                    continue;
                }
            }
        }

        /* 8. EMOJI */
        if (c == ':' && st && st->cfg && st->cfg->emoji_count) {
            size_t j = i + 1;
            while (j < n && (isalnum((unsigned char)src[j]) || src[j] == '_' ||
                src[j] == '+' || src[j] == '-')) j++;
            if (j > i + 1 && j < n && src[j] == ':') {
                size_t slen = j - (i + 1);
                if (slen < MD_MAX_EMOJI_LEN) {
                    char code[MD_MAX_EMOJI_LEN];
                    memcpy(code, src + i + 1, slen); code[slen] = '\0';
                    const char* repl = _emoji_lookup(st->cfg, code);
                    if (repl) {
                        fputs(repl, dest);
                        i = j + 1;
                        continue;
                    }
                }
            }
        }

        /* 9. BACKSLASH-ESCAPE */
        if (c == '\\' && i + 1 < n) {
            char nx = src[i + 1];
            if (strchr("\\`*_{}[]()#+-.!~|>", nx)) {
                switch ((unsigned char)nx) {
                case '<': fputs("&lt;", dest); break;
                case '>': fputs("&gt;", dest); break;
                case '&': fputs("&amp;", dest); break;
                case '"': fputs("&quot;", dest); break;
                default:  fputc(nx, dest);
                }
                i += 2; continue;
            }
        }

        /* 10. ENTITY */
        if (c == '&') {
            size_t j = i + 1;
            if (j < n && src[j] == '#') {
                j++;
                if (j < n && (src[j] == 'x' || src[j] == 'X')) {
                    j++;
                    while (j < n && isxdigit((unsigned char)src[j])) j++;
                }
                else {
                    while (j < n && isdigit((unsigned char)src[j])) j++;
                }
                if (j < n && src[j] == ';') {
                    fwrite(src + i, 1, j - i + 1, dest);
                    i = j + 1; continue;
                }
            }
            else {
                while (j < n && isalpha((unsigned char)src[j])) j++;
                if (j > i + 1 && j < n && src[j] == ';') {
                    fwrite(src + i, 1, j - i + 1, dest);
                    i = j + 1; continue;
                }
            }
            fputs("&amp;", dest); i++; continue;
        }

        /* 11. Literal */
        switch (c) {
        case '<': fputs("&lt;", dest); break;
        case '>': fputs("&gt;", dest); break;
        default:  fputc((char)c, dest);
        }
        i++;
        continue;

    next_char:;
    }
}

/* ============================================================
 *  TABELLEN
 * ============================================================ */

static void _parse_table_row(FILE* dest, char* line, int is_header,
    int* aligns, MdState* st) {
    if (!line || !dest) return;
    fprintf(dest, "<tr>");
    char* ptr = line;
    if (*ptr == '|') ptr++;

    int col = 0;
    char* end;
    while ((end = strchr(ptr, '|')) != NULL && col < MAX_TABLE_COLS) {
        *end = '\0';

        char* cell = ptr;
        while (*cell == ' ' || *cell == '\t') cell++;

        int cl = (int)strlen(cell);
        while (cl > 0 && (cell[cl - 1] == ' ' || cell[cl - 1] == '\t')) cell[--cl] = '\0';

        const char* align_str = "";
        if (!is_header && aligns) {
            if (aligns[col] == 1)      align_str = " style=\"text-align:center;\"";
            else if (aligns[col] == 2) align_str = " style=\"text-align:right;\"";
        }

        if (is_header) {
            fprintf(dest, "<th scope=\"col\">");
            _parse_inline(dest, cell, st);
            fprintf(dest, "</th>");
        }
        else {
            fprintf(dest, "<td%s>", align_str);
            _parse_inline(dest, cell, st);
            fprintf(dest, "</td>");
        }

        ptr = end + 1;
        col++;
    }
    fprintf(dest, "</tr>\n");
}

/* ============================================================
 *  PREPASS — Refdefs und Heading-Slugs
 * ============================================================ */

static void _prepass_refdefs(char** lines, int count, char* skip, MdState* st) {
    for (int li = 0; li < count; li++) {
        if (skip[li]) continue;
        char* p = lines[li];
        while (*p == ' ' || *p == '\t') p++;
        if (*p != '[') continue;
        char* close = strchr(p, ']');
        if (!close || close[1] != ':') continue;

        if (st->refdef_count >= MAX_REFDEFS) return;

        size_t lab_len = (size_t)(close - (p + 1));
        if (lab_len == 0 || lab_len >= 256) continue;

        char* url = close + 2;
        while (*url == ' ' || *url == '\t') url++;
        if (*url == '\0') continue;

        char* title = NULL;
        size_t url_len = strlen(url);
        if (url_len >= 3) {
            char* q = url + url_len - 1;
            if (*q == '"' || *q == '\'') {
                char* prev = q - 1;
                while (prev > url && *prev != (char)*q) prev--;
                if (prev > url && prev > url + 1) {
                    *prev = '\0';
                    title = prev + 1;
                    while (*q == (char)q[0]) { (void)q; break; }
                    url[strcspn(url, " \t")] = '\0';
                }
            }
        }
        char* sp = strpbrk(url, " \t");
        if (sp) {
            *sp = '\0';
            title = sp + 1;
            while (*title == ' ' || *title == '\t') title++;
            size_t tl = strlen(title);
            if (tl >= 2 && ((title[0] == '"' && title[tl - 1] == '"') ||
                (title[0] == '\'' && title[tl - 1] == '\''))) {
                title[tl - 1] = '\0'; title++;
            }
        }

        RefDef* rd = &st->refdefs[st->refdef_count];
        memcpy(rd->label, p + 1, lab_len); rd->label[lab_len] = '\0';
        strncpy(rd->url, url, sizeof(rd->url) - 1);
        rd->url[sizeof(rd->url) - 1] = '\0';
        if (title) {
            strncpy(rd->title, title, sizeof(rd->title) - 1);
            rd->title[sizeof(rd->title) - 1] = '\0';
        }
        else {
            rd->title[0] = '\0';
        }
        st->refdef_count++;
        skip[li] = 1;
    }
}

static int _detect_atx(char* line, char** content_ptr) {
    char* p = line;
    while (*p == ' ' && (p - line) < 3) p++;
    int lvl = 0;
    while (*p == '#' && lvl < 6) { lvl++; p++; }
    if (lvl == 0) return 0;
    if (*p == ' ' || *p == '\t') {
        while (*p == ' ' || *p == '\t') p++;
        *content_ptr = p;
        return lvl;
    }
    return 0;
}

static void _prepass_headings(char** lines, int count, char* skip, MdState* st) {
    for (int li = 0; li < count; li++) {
        if (skip[li]) continue;

        char* content = NULL;
        int lvl = _detect_atx(lines[li], &content);
        if (lvl > 0 && content) {
            int cl = (int)strlen(content);
            while (cl > 0 && content[cl - 1] == '#') content[--cl] = '\0';
            while (cl > 0 && (content[cl - 1] == ' ' || content[cl - 1] == '\t')) content[--cl] = '\0';
            if (cl == 0) continue;

            char slug[MAX_SLUG_LEN];
            _slugify(content, slug, sizeof(slug));
            if (slug[0]) _heading_add(st, slug);
            continue;
        }

        if (li + 1 < count && !skip[li + 1]) {
            char* cur = lines[li];
            char* nx = lines[li + 1];
            if (cur[0] != '\0') {
                char* q = nx;
                while (*q == ' ' && (q - nx) < 3) q++;
                char mc = *q;
                if (mc == '=' || mc == '-') {
                    int all = 1;
                    char* r = q;
                    while (*r == mc) r++;
                    while (*r == ' ' || *r == '\t') r++;
                    if (*r != '\0') all = 0;
                    if (all && (r - q) >= 1) {
                        char slug[MAX_SLUG_LEN];
                        _slugify(cur, slug, sizeof(slug));
                        if (slug[0]) _heading_add(st, slug);
                        skip[li + 1] = 1;
                    }
                }
            }
        }
    }
}

/* ============================================================
 *  MAIN PASS
 * ============================================================ */

typedef struct {
    int in_list;
    int in_code;
    int in_quote;
    int in_table;
    int in_p;
    int in_details;
    int table_align[MAX_TABLE_COLS];
} BlockState;

static void _close_paragraph(FILE* dest, BlockState* bs) {
    if (bs->in_p) { fputs("</p>\n", dest); bs->in_p = 0; }
}
static void _close_list(FILE* dest, BlockState* bs) {
    if (bs->in_list) {
        fputs(bs->in_list == 1 ? "</ul>\n" : "</ol>\n", dest);
        bs->in_list = 0;
    }
}
static void _close_quote(FILE* dest, BlockState* bs) {
    if (bs->in_quote) { fputs("</blockquote>\n", dest); bs->in_quote = 0; }
}
static void _close_table(FILE* dest, BlockState* bs) {
    if (bs->in_table) { fputs("</tbody>\n</table>\n</div>\n", dest); bs->in_table = 0; }
}
static void _close_all(FILE* dest, BlockState* bs) {
    _close_paragraph(dest, bs);
    _close_list(dest, bs);
    _close_quote(dest, bs);
    _close_table(dest, bs);
}

static void _main_pass(FILE* dest, char** lines, int count, char* skip, MdState* st) {
    BlockState bs;
    memset(&bs, 0, sizeof(bs));
    int heading_cursor = 0;

    for (int li = 0; li < count; li++) {
        if (skip[li]) continue;
        char* line = lines[li];
        int len = (int)strlen(line);

        /* 1. FENCED CODE */
        if (strncmp(line, "```", 3) == 0 || strncmp(line, "~~~", 3) == 0) {
            const char* fence = line;
            _close_all(dest, &bs);
            if (bs.in_code) {
                fputs("</code></pre>\n", dest);
                bs.in_code = 0;
            }
            else {
                char lang[64] = { 0 };
                const char* lptr = fence + 3;
                while (*lptr == ' ' || *lptr == '\t') lptr++;
                if (*lptr) {
                    strncpy(lang, lptr, sizeof(lang) - 1);
                    char* sp = strpbrk(lang, " \t\r\n");
                    if (sp) *sp = '\0';
                }
                if (lang[0]) fprintf(dest, "<pre><code class=\"language-%s\">", lang);
                else fputs("<pre><code>", dest);
                bs.in_code = 1;
            }
            continue;
        }

        if (bs.in_code) {
            _print_escaped(dest, line);
            fputc('\n', dest);
            continue;
        }

        /* 2. LEERZEILE */
        if (len == 0) {
            _close_all(dest, &bs);
            continue;
        }

        /* 3. TABELLE */
        if (line[0] == '|') {
            _close_paragraph(dest, &bs);
            _close_list(dest, &bs);
            if (bs.in_table == 0) {
                fputs("<div class=\"table-responsive\">\n<table>\n<thead>\n", dest);
                _parse_table_row(dest, line, 1, NULL, st);
                bs.in_table = 1;
            }
            else if (bs.in_table == 1 && strstr(line, "---")) {
                char* ptr = line; if (*ptr == '|') ptr++;
                int col = 0; char* end;
                while ((end = strchr(ptr, '|')) != NULL && col < MAX_TABLE_COLS) {
                    *end = '\0';
                    char* cell = ptr;
                    while (*cell == ' ' || *cell == '\t') cell++;
                    int cl = (int)strlen(cell);
                    while (cl > 0 && (cell[cl - 1] == ' ' || cell[cl - 1] == '\t')) cell[--cl] = '\0';
                    if (cl > 0) {
                        int left = (cell[0] == ':');
                        int right = (cell[cl - 1] == ':');
                        if (left && right) bs.table_align[col] = 1;
                        else if (right)    bs.table_align[col] = 2;
                        else               bs.table_align[col] = 0;
                    }
                    ptr = end + 1; col++;
                }
                fputs("</thead>\n<tbody>\n", dest);
                bs.in_table = 2;
            }
            else {
                _parse_table_row(dest, line, 0, bs.table_align, st);
            }
            continue;
        }
        else if (bs.in_table != 0) {
            _close_table(dest, &bs);
        }

        /* 4. THEMATIC BREAK */
        {
            char* p = line;
            while (*p == ' ' && (p - line) < 3) p++;
            char tc = *p;
            if (tc == '-' || tc == '*' || tc == '_') {
                int cnt = 0;
                char* q = p;
                int only = 1;
                while (*q) {
                    if (*q == tc) cnt++;
                    else if (*q != ' ' && *q != '\t') { only = 0; break; }
                    q++;
                }
                if (only && cnt >= 3) {
                    _close_all(dest, &bs);
                    fputs("<hr>\n", dest);
                    continue;
                }
            }
        }

        /* 5. ATX HEADING */
        {
            char* content = NULL;
            int lvl = _detect_atx(line, &content);
            if (lvl > 0 && content) {
                _close_all(dest, &bs);
                int cl = (int)strlen(content);
                while (cl > 0 && content[cl - 1] == '#') content[--cl] = '\0';
                while (cl > 0 && (content[cl - 1] == ' ' || content[cl - 1] == '\t')) content[--cl] = '\0';

                if (heading_cursor < st->heading_count) {
                    const HeadingEntry* h = &st->headings[heading_cursor++];
                    fprintf(dest, "<h%d id=\"%s\">", lvl, h->full);
                    _parse_inline(dest, content, st);
                    fprintf(dest, "</h%d>\n", lvl);
                }
                else {
                    fprintf(dest, "<h%d>", lvl);
                    _parse_inline(dest, content, st);
                    fprintf(dest, "</h%d>\n", lvl);
                }
                continue;
            }
        }

        /* 6. SETEXT HEADING */
        if (li + 1 < count && !skip[li + 1]) {
            char* nx = lines[li + 1];
            char* q = nx;
            while (*q == ' ' && (q - nx) < 3) q++;
            char mc = *q;
            if (mc == '=' || mc == '-') {
                char* r = q;
                while (*r == mc) r++;
                while (*r == ' ' || *r == '\t') r++;
                if (*r == '\0' && line[0] != '\0') {
                    int lvl = (mc == '=') ? 1 : 2;
                    _close_all(dest, &bs);

                    if (heading_cursor < st->heading_count) {
                        const HeadingEntry* h = &st->headings[heading_cursor++];
                        fprintf(dest, "<h%d id=\"%s\">", lvl, h->full);
                        _parse_inline(dest, line, st);
                        fprintf(dest, "</h%d>\n", lvl);
                    }
                    else {
                        fprintf(dest, "<h%d>", lvl);
                        _parse_inline(dest, line, st);
                        fprintf(dest, "</h%d>\n", lvl);
                    }
                    li++;
                    continue;
                }
            }
        }

        /* 7. BLOCKQUOTE */
        if (line[0] == '>') {
            _close_paragraph(dest, &bs);
            _close_list(dest, &bs);
            if (!bs.in_quote) { fputs("<blockquote>\n", dest); bs.in_quote = 1; }
            char* content = line + 1;
            while (*content == ' ') content++;
            _parse_inline(dest, content, st);
            fputs("<br>\n", dest);
            continue;
        }
        else if (bs.in_quote) {
            _close_quote(dest, &bs);
        }

        /* 8. LISTEN */
        {
            char* lp = line;
            while (*lp == ' ' || *lp == '\t') lp++;

            int is_ul = (strncmp(lp, "- ", 2) == 0 || strncmp(lp, "* ", 2) == 0 || strncmp(lp, "+ ", 2) == 0);
            int is_ol = 0;
            char* ol_end = NULL;
            if (!is_ul && isdigit((unsigned char)*lp)) {
                char* ol = lp;
                while (isdigit((unsigned char)*ol)) ol++;
                if ((*ol == '.' || *ol == ')') && ol[1] == ' ') {
                    is_ol = 1;
                    ol_end = ol;
                }
            }

            if (is_ul || is_ol) {
                _close_paragraph(dest, &bs);
                int target = is_ul ? 1 : 2;
                if (bs.in_list && bs.in_list != target) {
                    fputs(bs.in_list == 1 ? "</ul>\n" : "</ol>\n", dest);
                    bs.in_list = 0;
                }
                if (!bs.in_list) {
                    fputs(target == 1 ? "<ul>\n" : "<ol>\n", dest);
                    bs.in_list = target;
                }

                char* content = is_ul ? (lp + 2) : (ol_end + 2);
                int is_task = 0, is_checked = 0;
                if (strlen(content) >= 4) {
                    if (strncmp(content, "[ ] ", 4) == 0) { is_task = 1; content += 4; }
                    else if (strncmp(content, "[x] ", 4) == 0 || strncmp(content, "[X] ", 4) == 0) {
                        is_task = 1; is_checked = 1; content += 4;
                    }
                }

                if (is_task)
                    fputs("<li class=\"task-list-item\" style=\"list-style-type: none;\">", dest);
                else
                    fputs("<li>", dest);
                if (is_task)
                    fprintf(dest, "<input type=\"checkbox\" disabled%s> ",
                        is_checked ? " checked" : "");
                _parse_inline(dest, content, st);
                fputs("</li>\n", dest);
                continue;
            }
        }
        if (bs.in_list) {
            _close_list(dest, &bs);
        }

        /* 9. HTML-BLOCK (details/summary) */
        {
            const char* p = line;
            while (*p == ' ' || *p == '\t') p++;
            if (strncmp(p, "<details", 8) == 0) {
                _close_all(dest, &bs);
                if (st && st->cfg && strstr(line, "open")) {
                    fputs("<details open>\n", dest);
                }
                else {
                    fputs("<details>\n", dest);
                }
                bs.in_details = 1;
                continue;
            }
            if (strncmp(p, "</details>", 10) == 0) {
                _close_all(dest, &bs);
                fputs("</details>\n", dest);
                bs.in_details = 0;
                continue;
            }
        }

        /* 10. ABSATZ */
        if (!bs.in_p) {
            fputs("<p>", dest);
            bs.in_p = 1;
        }
        else {
            fputc(' ', dest);
        }
        _parse_inline(dest, line, st);
        fputc('\n', dest);
    }

    if (bs.in_code) fputs("</code></pre>\n", dest);
    _close_all(dest, &bs);
}

/* ============================================================
 *  ÖFFENTLICHE API
 * ============================================================ */

static char* _read_file(const char* path, size_t* out_len) {
    if (!path) return NULL;
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > MAX_FILE_BYTES) { fclose(f); return NULL; }
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    *out_len = n;
    return buf;
}

void md_parse_file(FILE* dest, const char* filepath,
    const char* view_id, const MdConfig* cfg) {
    if (!dest || !filepath || !view_id) return;

    size_t blen = 0;
    char* buf = _read_file(filepath, &blen);
    if (!buf) {
        fprintf(stderr, "[md_parser] Kann Datei nicht lesen: %s\n", filepath);
        return;
    }

    int cap = 256, count = 0;
    char** lines = malloc((size_t)cap * sizeof(char*));
    if (!lines) { free(buf); return; }
    char* p = buf;
    lines[count++] = p;
    while (*p) {
        if (*p == '\n') {
            *p = '\0';
            if (p > buf && *(p - 1) == '\r') *(p - 1) = '\0';
            if (count >= cap) {
                cap *= 2;
                if (cap > MAX_LINES) cap = MAX_LINES;
                char** tmp = realloc(lines, (size_t)cap * sizeof(char*));
                if (!tmp) { free(lines); free(buf); return; }
                lines = tmp;
            }
            lines[count++] = p + 1;
            if (count >= MAX_LINES) break;
        }
        p++;
    }

    char* skip = calloc((size_t)count, 1);
    if (!skip) { free(lines); free(buf); return; }

    /* STATIC statt stack: MdState ist ~580 KB gross. Bei Standard-
     * 1MB-Windows-Stack ist ein lokales MdState der Hauptgrund fuer
     * STATUS_STACK_OVERFLOW (0xC00000FD). */
    static MdState st;
    memset(&st, 0, sizeof(st));
    st.cfg = cfg;
    strncpy(st.view_id, view_id, sizeof(st.view_id) - 1);
    st.view_id[sizeof(st.view_id) - 1] = '\0';

    _prepass_refdefs(lines, count, skip, &st);
    _prepass_headings(lines, count, skip, &st);

    fprintf(dest, "<section id=\"%s\" class=\"view-content\" style=\"display:none;\">\n", view_id);
    fputs("<div class=\"markdown-body\">\n", dest);

    _main_pass(dest, lines, count, skip, &st);

    fputs("</div>\n</section>\n", dest);

    free(skip);
    free(lines);
    free(buf);
}