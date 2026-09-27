# ProWB — ProPhysics Web Builder

**Datei:** `src/prowb/README.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Zweck:** Kompakter C99-Builder, der die Markdown-Dokumentation aus
`docs\` in ein statisches Web-Portal (`out\web\index.html`) rendert.
Liest die Quellen über ein Manifest, **kopiert keine MD**, sondern
parst sie direkt von ihren kanonischen Pfaden.

---

## §0 — Wie dieses Dokument zu lesen ist

Dieses Dokument beschreibt den **ProWB-Builder** als eigenständiges
Werkzeug. Es ist **kein** Kernel-Modul und **kein** SDK-Bestandteil.
ProWB ist ein Build-Tool, das parallel zu Kernel/SDK/Tests gebaut
wird und die Web-Docs erzeugt.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was ProWB tut (Rolle im Build-System) |
| §2 | Verzeichnis-Layout |
| §3 | Öffentliche Builder-API |
| §4 | Manifest-Format |
| §5 | Sektionen |
| §6 | Templates und Views |
| §7 | CLI-Aufruf |
| §8 | Fehlerbehandlung und Exit-Codes |
| §9 | Design-Entscheidungen |
| §10 | Was ProWB nicht tut |
| §11 | Siehe auch |

**Verwandte Dokumente:**

- `docs\build\prowb\Makefile.md` — Build-Integration
- `docs\web\README.md` — Manifest-Pflege
- `docs\build\web-docs-ci.md` — CI-Workflow
- `docs\build\BUILD_SCRIPT.md` — Build-System-Übersicht
- `CHANGELOG.md` — Änderungshistorie (`1.23.11`)

**Konvention:** Öffentliche Symbole aus `prowb.h` haben Prefix
`prowb_`. Interne Helfer in `prowb.c` sind `static`. Öffentliche
Symbole aus `md_parser.h` haben Prefix `md_`.

---

## §1 — Was ProWB tut

ProWB ist ein **einzelnes C-Programm** (~800 LOC), das die
Markdown-Dokumentation von ProPhysics in ein statisches,
publikationsfähiges Web-Portal umwandelt.

**Aufgaben:**

1. **Manifest lesen** — `docs\web\manifest.txt`.
2. **MD-Dateien parsen** — **an ihren kanonischen Pfaden**, nicht
   kopiert.
3. **Templates expandieren** — `header.html`, `nav.html`,
   `footer.html`.
4. **Views rendern** — Sektions-Übersichten, Home, Settings.
5. **Assets kopieren** — CSS, JS.
6. **`out\web\index.html` schreiben** — Single-Page-App.

**Rolle im Build-System:**

```
src\prowb\       →  bin\prowb\prowb.exe   (Builder)
                        │
                        ▼
docs\web\         ──→  out\web\           (Portal)
```

ProWB läuft **nach** Kernel/SDK/Tests im Master-`all`. Es hängt
**nicht** von deren Artefakten ab.

**Was ProWB nicht ist:**

- Kein Markdown-Editor.
- Kein Static-Site-Generator für beliebige Projekte.
- Kein Server. Es erzeugt **statische Dateien**.

---

## §2 — Verzeichnis-Layout

```
src\prowb\
├── README.md              ← diese Datei
├── prowb.c                ← Builder-Kern
├── md_parser.c            ← GFM-Parser
└── header\
    ├── prowb.h            ← öffentliche Builder-API
    └── md_parser.h        ← Parser-API
```

**Build-Artefakt:** `bin\prowb\prowb.exe` (Standalone-EXE, keine
DLL, keine Import-Lib).

**Warum ein eigener `header\`-Ordner?** Konsistenz mit den anderen
Modulen (`src\prophysics\header\`, `src\sdk\header\`,
`src\test\header\`). Header bleiben am Pflegeort, werden nicht
kopiert.

---

## §3 — Öffentliche Builder-API

### §3.1 — `prowb_build()` (Legacy)

```c
int prowb_build(const char* src_dir,
                const char* template_dir,
                const char* out_dir);
```

**Zweck:** Baut ein Portal aus einem **festen Verzeichnis-Layout**.

**Erwartetes Layout:**

```
<src_dir>/
├── pages/*.md
├── parts/header.html
├── parts/nav.html
├── parts/footer.html
├── css/*.css
└── js/*.js
```

**Status:** Bleibt erhalten (R5). Für neue Aufrufer **nicht mehr
empfohlen**, weil die MD-Dateien an einen zweiten Ort kopiert
werden müssten.

**Rückgabe:** `0` bei Erfolg, `≠0` bei Fehler.

### §3.2 — `prowb_build_from_manifest()` (neu, empfohlen)

```c
int prowb_build_from_manifest(const char* src_root,
                              const char* manifest_path,
                              const char* out_dir);
```

**Zweck:** Baut das Portal aus einem Manifest. Die MD-Quellen
werden **nicht kopiert** — sie werden direkt vom Manifest-Pfad
gelesen.

**Parameter:**

| Parameter | Bedeutung | Typischer Wert |
|---|---|---|
| `src_root` | Wurzel für Templates, Views, Assets | `docs\web\src` |
| `manifest_path` | Manifest-Datei | `docs\web\manifest.txt` |
| `out_dir` | Ziel-Verzeichnis | `out\web` |

**Additiv:** Der Legacy-Aufruf bleibt funktional. ProWB wählt zur
Laufzeit: Bei gesetztem `--manifest`-Flag wird die
Manifest-Variante verwendet.

**Rückgabe:** `0` bei Erfolg, `≠0` bei Fehler.

---

## §4 — Manifest-Format

Das Manifest ist eine **zeilenbasierte UTF-8-Textdatei**. Pro Zeile
beschreibt sie **einen** Doku-Eintrag.

### §4.1 — Zeilen-Format

```
<Quellpfad>|<Sektion>|<Doc-ID>
```

| Feld | Position | Typ | Beispiel |
|---|:-:|---|---|
| `Quellpfad` | 1 | Pfad | `docs/project/Project.md` |
| `Sektion` | 2 | String | `Modules` |
| `Doc-ID` | 3 | String | `Amp` |

**Trenner ist `|`** (Pipe). Felder dürfen **keine** `|`-Zeichen
enthalten.

### §4.2 — Pfad-Regeln

- **Relativ zum Repo-Root**, nicht zu `src_root`.
- **Forward slash** (`/`) als Trenner, auch unter Windows.
- **Kein führender `./`**.
- **Keine Leerzeichen** im Pfad.
- **Kein abschließender `/`**.
- **Groß-/Kleinschreibung** wie auf der Platte.

### §4.3 — Sektion

Der Sektionsname ist **case-sensitive** und muss einer der sechs
festen Werte entsprechen (siehe §5):

```
Overview
Physics
Modules
API
Tests
Build
```

Ein unbekannter Sektionsname führt zu einer Warnung auf `stderr`
und der Eintrag wird **ignoriert**.

### §4.4 — Doc-ID

Die Doc-ID ist **eindeutig innerhalb der Sektion**. Sie dient als
URL-Fragment (z. B. `#/Modules/Amp`) und als Schlüssel für die
View-Zuordnung.

**Konventionen:**

- Keine Leerzeichen.
- Unterstriche für Worttrennung (`Makefile_Main`, `WriteBuildInfo`).
- CamelCase für zusammengesetzte Namen (`WritingTests`,
  `WebDocsCI`).
- Kurz halten.

### §4.5 — Kommentare und Leerzeilen

```text
# --- Modules ---
docs/project/Amp.md                             |Modules|Amp
docs/project/Core.md                            |Modules|Core

# --- API ---
docs/project/ProPhysics_API.md                  |API|ProPhysics_API
```

- Zeilen, die mit `#` **am Zeilenanfang** beginnen, werden
  ignoriert.
- Leerzeilen werden ignoriert.
- **Kein** Inline-Kommentar.
- **Kein** Block-Kommentar.

### §4.6 — Reihenfolge

Die Reihenfolge im Manifest bestimmt die Reihenfolge **innerhalb
einer Sektion** in der Navigation. Sektionen selbst werden in der
Reihenfolge ihres ersten Auftretens in der Nav gruppiert
(typischerweise Overview, Physics, Modules, API, Tests, Build).

### §4.7 — Detail-Doku

Die vollständige Pflege-Anleitung steht in `docs\web\README.md`.

---

## §5 — Sektionen

ProWB kennt **sechs Sektionen**. Sie sind im C-Code hart kodiert
(siehe §9.4). Die Schreibweise ist **case-sensitive**:

| Sektion | Zweck | Beispiel-Docs |
|---|---|---|
| `Overview` | Projekt, Architektur, Versionierung, Contributor | `Project.md`, `ARCHITECTURE.md`, `CHANGELOG.md`, `TODO.md`, `CONTRIBUTING.md`, `COMMERCIAL.md` |
| `Physics` | Physik-Übersicht, Config | `docs/physics/README.md`, `CONFIG.md` |
| `Modules` | Modul-Referenzen (12 Kernel-Module) | `Amp.md` … `Tensor.md` |
| `API` | Kernel-API, SDK-API | `ProPhysics_API.md`, `SDK_API.md` |
| `Tests` | Testkatalog, Runner, Baseline, Anleitung | `ProPhysics_Testkatalog.md`, `run_alpha_tests.md`, `BASELINE.md`, `WRITING_TESTS.md` |
| `Build` | Build-System, Makefiles, Wrapper, ProWB, CI | `BUILD_SCRIPT.md`, `pro_run.md`, `Makefile.md`, Helper-Docs, ProWB-Docs |

**Die Sektionen sind flach** (eine Ebene). Es gibt keine
Unter-Sektionen.

**Utility-Bereiche** wie Lizenz, Datenschutz und Settings sind
**keine** Manifest-Sektionen. Sie werden vom Router direkt
angesteuert (Views `96_view_lizenz.html`, `97_view_datenschutz.html`,
`99_view_settings.html`) und stehen nicht im Manifest.

---

## §6 — Templates und Views

ProWB liest Templates, Views und Assets aus `docs\web\src\`:

```
docs\web\src\
├── parts\
│   ├── header.html    ← <head>, Branding, Theme-Switch
│   ├── nav.html       ← Navigation (Sektionen + Docs)
│   └── footer.html    ← Copyright, Links
├── css\
│   └── *.css          ← Reset, Vars, Layout, Themes
├── js\
│   ├── 00_bridge.js   ← Daten-Bridge
│   └── 10_app.js      ← Router, Theme-Switch
└── views\
    └── *.html         ← Sektions-Übersichten + Utility-Views
```

Die **Struktur und Platzhalter-Konventionen** der Templates sind
nicht Gegenstand dieses Dokuments. Sie werden in der Datei
`docs\web\README.md` beschrieben, sobald die Templates final
festgelegt sind.

**Verbindlich für ProWB:**

- Templates bleiben unter `docs\web\src\parts\`.
- Views bleiben unter `docs\web\src\views\`.
- Assets bleiben unter `docs\web\src\css\` und `docs\web\src\js\`.
- ProWB kopiert Assets nach `out\web\assets\` und expandiert die
  Views in `out\web\index.html`.

---

## §7 — CLI-Aufruf

### §7.1 — Manifest-Modus (empfohlen)

```cmd
bin\prowb\prowb.exe --manifest <src_root> <manifest_path> <out_dir>
```

**Beispiel:**

```cmd
bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web
```

### §7.2 — Legacy-Modus

```cmd
bin\prowb\prowb.exe <src_dir> <template_dir> <out_dir>
```

**Beispiel:**

```cmd
bin\prowb\prowb.exe docs\web\src docs\web\src out\web
```

**Hinweis:** Legacy-Modus erwartet ein anderes Layout
(`pages/`, `parts/` direkt in `src_dir`). Wird intern nicht mehr
verwendet.

### §7.3 — Über `pro_run`

```cmd
pro_run web
pro_run web -Rebuild
pro_run web -OutDir D:\docs-portal
```

Details siehe `docs\build\pro_run.md` §4.6.

---

## §8 — Fehlerbehandlung und Exit-Codes

| Situation | Verhalten | Exit |
|---|---|---|
| Manifest nicht gefunden | `fprintf(stderr, ...)` | `2` |
| Manifest-Zeile fehlerhaft | Warnung, Zeile übersprungen | `0` |
| Sektion unbekannt | Warnung, Zeile übersprungen | `0` |
| MD-Quelle fehlt | Warnung auf stderr, Sektion ohne Eintrag | `0` |
| Template fehlt | Warnung, Template durch leeren String ersetzt | `0` |
| View fehlt | Warnung, View wird übersprungen | `0` |
| `out_dir` nicht schreibbar | `fprintf(stderr, ...)` | `3` |
| Unbekannter Platzhalter | Bleibt wörtlich stehen, Warnung | `0` |
| Parser-Fehler in MD | Warnung, MD mit Roh-Text eingebettet | `0` |

**Exit-Code-Übersicht:**

| Code | Bedeutung |
|:-:|---|
| `0` | Erfolg (auch mit Warnungen) |
| `1` | Parser-interner Fehler (unerwartet) |
| `2` | IO-Fehler (Manifest, Templates, Output) |
| `3` | Manifest-Format-Fehler (kritisch) |

**Keine Exception, kein `abort`.** Der Builder ist defensiv
geschrieben und produziert immer eine `index.html` — notfalls
mit leeren Sektionen.

---

## §9 — Design-Entscheidungen

### §9.1 — Warum C und nicht Python/Node?

| Grund | Erklärung |
|---|---|
| **Keine externen Abhängigkeiten** | Das Repo bleibt self-contained. Kein `pip install`, kein `npm install`. |
| **Build-Zeit < 1 s** | Kein Interpreter-Startup, keine Modul-Imports. |
| **Cross-Compile-fähig** | Ein `cl.exe`-Aufruf, fertig. Kein Node-Build-Step. |
| **Konsistenz** | ProPhysics ist C99. Ein Python-Build-Tool wäre ein Stilbruch. |
| **CI-freundlich** | Keine Runtime-Abhängigkeit auf dem CI-Runner außer MSVC. |

### §9.2 — Warum Manifest statt Konvention?

Konvention (`pages/*.md`) hätte eine **zweite Kopie** der Doku
erfordert — oder einen Symlink-Baum. Beides ist schlecht:

- **Kopie:** Läuft auseinander. Zwei Quellen der Wahrheit.
- **Symlink:** Funktioniert nicht auf Windows ohne Admin-Rechte.

Das Manifest erlaubt, die MD-Dateien **am Pflegeort** zu lassen.
Ein Eintrag pro Datei, ein Blick auf `manifest.txt` — fertig.

### §9.3 — Warum drei Felder statt vier?

Das Manifest kennt **nur drei** Felder: Quellpfad, Sektion,
Doc-ID. Der **Anzeigename** (Titel) wird beim Rendern aus dem
MD-Inhalt abgeleitet (erste `#`-Überschrift oder Dateiname). Ein
separates Titel-Feld wäre eine zweite Quelle der Wahrheit — der
Titel im Manifest könnte vom MD-Header abweichen.

**Konsequenz:** Wer den Anzeigenamen im Portal ändern will, ändert
die `#`-Überschrift im MD. Kein Manifest-Edit nötig.

**Kein Icon-Feld.** Icons pro Sektion sind in `nav.html` hinterlegt,
nicht pro Dokument. Das hält das Manifest schlank und vermeidet
UTF-8-Probleme in Textdateien.

### §9.4 — Warum die Sektionen hart kodiert?

Weil es **sechs Sektionen** gibt und sie stabil sind. Ein
dynamisches System (Sektionen aus Manifest ableiten) wäre in C
unverhältnismäßig aufwendig. Bei einer 7. Sektion wird die
`prowb.c`-Konstante erweitert — 3 Zeilen.

### §9.5 — Warum ein eigener Parser?

`md_parser.c` ist eigenständig (keine externe Bibliothek wie
`cmark`). Grund: **Self-contained**. Der Parser deckt genau die
GFM-Teilmenge ab, die ProPhysics verwendet:

- Überschriften (`#` bis `######`)
- Fett / Kursiv (`**`, `*`)
- Code-Blöcke (``` mit Sprache)
- Inline-Code
- Listen (ungeordnet / geordnet)
- Tabellen (GFM-Style mit `|---|`)
- Links (`[text](url)`)
- Blockquotes (`>`)

Nicht abgedeckt: Fußnoten, Definitions-Listen, HTML-Inline.
Reicht für die ProPhysics-Doku.

---

## §10 — Was ProWB nicht tut

- **Kein Markdown-Editor.** ProWB liest MD, es schreibt kein MD.
- **Keine MD-Kopien.** Die Quellen bleiben an ihren Pflegeorten.
- **Kein Watch-Modus.** Kein Auto-Rebuild bei Dateiänderung.
- **Kein Server.** Kein `http://localhost`, kein Hot-Reload.
- **Kein PDF-Export.** Nur HTML-Ausgabe.
- **Keine Suche.** Keine Volltextsuche, kein Index.
- **Keine Cross-Refs.** Phase 2 (siehe `TODO.md` §5.9).
- **Kein Deployment.** Das macht der CI-Workflow
  (`.github\workflows\web-docs.yml`).
- **Keine i18n.** Nur Deutsch.

---

## §11 — Siehe auch

| Thema | Datei |
|---|---|
| ProWB-Makefile | `docs\build\prowb\Makefile.md` |
| Manifest-Pflege | `docs\web\README.md` |
| CI-Workflow | `docs\build\web-docs-ci.md` |
| `pro_run web` | `docs\build\pro_run.md` |
| Build-Übersicht | `docs\build\BUILD_SCRIPT.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Changelog | `CHANGELOG.md` (`1.23.11`) |
| Projekt-Roadmap | `docs\project\Project.md` |
| Quelldatei | `src\prowb\prowb.c` |
| Parser | `src\prowb\md_parser.c` |

---

**Ende src/prowb/README.md v1.0.**