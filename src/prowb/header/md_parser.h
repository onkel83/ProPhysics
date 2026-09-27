#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <stdio.h>

/* ============================================================
 *  KONFIGURATION — Limits (statisch, für Stack-Resistenz)
 * ============================================================ */
#define MD_MAX_TAGS          128
#define MD_MAX_TAG_LEN        32
#define MD_MAX_TAG_ATTRS     256

#define MD_MAX_EMOJIS        512
#define MD_MAX_EMOJI_LEN      32
#define MD_MAX_EMOJI_REPL     16

#define MD_MAX_CROSSLINKS    1024
#define MD_MAX_SECTION_LEN    64
#define MD_MAX_DOCID_LEN     128
#define MD_MAX_PATH_LEN      512

 /* ============================================================
  *  KONFIGURATION — Strukturen
  * ============================================================ */

typedef struct {
    char tag[MD_MAX_TAG_LEN];
    char attrs[MD_MAX_TAG_ATTRS];   /* komma-separiert, "" = keine */
} MdAllowedTag;

typedef struct {
    char shortcode[MD_MAX_EMOJI_LEN];   /* ohne ':' */
    char replacement[MD_MAX_EMOJI_REPL];
} MdEmojiEntry;

typedef struct {
    char section[MD_MAX_SECTION_LEN];   /* Thema */
    char doc_id[MD_MAX_DOCID_LEN];      /* Name  */
    char path[MD_MAX_PATH_LEN];         /* Ort   */
} MdCrossLink;

typedef struct {
    MdAllowedTag  allowed_tags[MD_MAX_TAGS];
    int           allowed_tag_count;

    MdEmojiEntry  emojis[MD_MAX_EMOJIS];
    int           emoji_count;

    MdCrossLink   crosslinks[MD_MAX_CROSSLINKS];
    int           crosslink_count;
} MdConfig;

/* ============================================================
 *  CONFIG-API
 * ============================================================ */

 /**
  * Initialisiert eine Config auf "leer" (keine Tags, keine Emojis,
  * keine Crosslinks). Danach per *_load_* befüllen.
  */
void md_config_init(MdConfig* cfg);

/**
 * Lädt eine Config-Datei. Rückgabe: 1 bei Erfolg, 0 bei Fehler.
 * Fehlende Datei ist KEIN Fehler — dann bleibt Config unverändert,
 * Rückgabe 0, aber der Parser arbeitet mit dem, was da ist.
 */
int md_config_load_emoji(MdConfig* cfg, const char* path);
int md_config_load_whitelist(MdConfig* cfg, const char* path);
int md_config_load_crosslinks(MdConfig* cfg, const char* path);

/* ============================================================
 *  PARSER-API
 * ============================================================ */

 /**
  * Parst eine MD-Datei und schreibt HTML in den Stream.
  *
  * Anker / Sprungmarken:
  *   - Jede Überschrift bekommt id="<view_id>-<slug>" (GitHub-kompatibel).
  *   - Kollidierende Slugs werden durchnummeriert: "<view_id>-<slug>-1", …
  *   - Interne Links [x](#slug) werden auf "#<view_id>-<slug>" umgeschrieben,
  *     sofern der Slug in dieser Datei existiert.
  *   - Cross-Doc-Links [x](path/to.md[#slug]) werden über die Crosslink-
  *     Tabelle auf "#doc_<Section>_<DocID>[-<slug>]" gemappt.
  *
  * @param dest      Ausgabe-Stream (z.B. eine geöffnete index.html)
  * @param filepath  Pfad zur MD-Datei
  * @param view_id   Eindeutige Section-ID dieser Datei (z.B. doc_Modules_SU2)
  * @param cfg       Config (darf NULL sein → leere Config)
  */
void md_parse_file(FILE* dest, const char* filepath,
    const char* view_id, const MdConfig* cfg);

#endif /* MD_PARSER_H */