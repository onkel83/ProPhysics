#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <stdio.h>

// Konfiguriert den Parser (optional für spätere Features)
typedef struct {
    int support_tables;
    int support_images;
} MDConfig;

// Die Hauptfunktion: Parsed eine MD Datei und schreibt HTML in den Stream
void md_parse_file(FILE *dest, const char *filepath, const char *view_id);

// Parsed eine einzelne Zeile (falls wir später Strings statt Dateien parsen wollen)
void md_parse_line(FILE *dest, const char *line);

#endif
