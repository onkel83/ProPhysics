# ProPhysics Web-Docs — Manifest und Pflege

**Datei:** `docs\web\README.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Zweck:** Anleitung zur Pflege des ProWB-Manifests und der
Web-Source-Struktur. Wer neue Doku ins Portal aufnimmt oder
Sektionen ändert, liest dieses Dokument.

---

## §0 — Wie dieses Dokument zu lesen ist

`docs\web\` ist die **Quelle** für das Web-Portal, das ProWB
generiert. Es enthält **keine** Markdown-Dokumentation — die liegt
an ihren kanonischen Pfaden in `docs\project\`, `docs\test\`,
`docs\build\`, `docs\physics\` und im Repo-Root. Das Manifest
**verweist** nur auf sie.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was hier passiert (Rolle im Build-System) |
| §2 | Verzeichnis-Layout |
| §3 | Manifest-Format |
| §4 | Einen neuen Doku-Eintrag hinzufügen |
| §5 | Eine neue Sektion hinzufügen |
| §6 | Templates und Views |
| §7 | Themes |
| §8 | Build-Integration |
| §9 | Fehlersuche |
| §10 | Siehe auch |

**Verwandte Dokumente:**

- `src\prowb\README.md` — Builder-Referenz
- `docs\build\prowb\Makefile.md` — ProWB-Makefile
- `docs\build\web-docs-ci.md` — CI-Workflow
- `docs\build\pro_run.md` — Aktion `web`
- `CHANGELOG.md` — Änderungshistorie (`1.23.11`)

**Prinzip:** **Keine MD-Kopien.** Die Doku bleibt am Pflegeort.
Das Manifest ist die einzige Stelle, an der die Zuordnung
Sektion → Dokument steht.

---

## §1 — Was hier passiert

`docs\web\` ist die **Web-Docs-Quelle**. ProWB liest:

1. Das **Manifest** (`manifest.txt`) — Liste aller Dokumente.
2. Die **Templates** (`src\parts\*.html`) — Kopf, Nav, Fuß.
3. Die **Views** (`src\views\*.html`) — Sektions-Übersichten.
4. Die **Assets** (`src\css\*.css`, `src\js\*.js`).

ProWB **schreibt** daraus:

- `out\web\index.html` — Single-Page-App.
- `out\web\assets\*.css` / `*.js` — kopierte Assets.
- `out\web\data\docs.js` — geparste MD-Inhalte als JSON-ish.

**Rolle im Build-System:**

```
docs\web\  ──→  ProWB  ──→  out\web\  ──→  CI  ──→  GitHub Pages
   │
   └─ Manifest verweist auf:
      README.md                (Root)
      CHANGELOG.md             (Root)
      TODO.md                  (Root)
      CONTRIBUTING.md          (Root)
      CONTRIBUTORS.md          (Root)
      COMMERCIAL.md            (Root)
      docs\project\*.md
      docs\test\*.md
      docs\build\*.md
      docs\physics\*.md
      src\prowb\*.md
```

**Die MD-Dateien selbst werden nicht kopiert.** Sie werden bei
jedem Build frisch gelesen und geparst. Eine Änderung an
`docs\project\Amp.md` wirkt sich beim nächsten `pro_run web`
automatisch aus.

---

## §2 — Verzeichnis-Layout

```
docs\web\
├── README.md              ← diese Datei
├── manifest.txt           ← zentrale Konfiguration
└── src\
    ├── parts\
    │   ├── header.html    ← <head>, Branding, Theme-Switch
    │   ├── nav.html       ← Navigation (Sektionen + Docs)
    │   └── footer.html    ← Copyright, Links
    ├── css\
    │   ├── 00_reset.css
    │   ├── 00_vars.css
    │   ├── 10_layout.css
    │   ├── 50_prophysics.css
    │   ├── 99_home.css
    │   ├── 99_theme_light.css
    │   ├── 99_theme_matrix.css
    │   └── 99_theme_prophysics.css
    ├── js\
    │   ├── 00_bridge.js   ← Daten-Bridge (data/docs.js → DOM)
    │   └── 10_app.js      ← Router, Theme-Switch, localStorage
    └── views\
        ├── 00_view_home.html
        ├── 01_view_overview.html
        ├── 02_view_physics.html
        ├── 03_view_modules.html
        ├── 04_view_api.html
        ├── 05_view_tests.html
        ├── 06_view_build.html
        ├── 96_view_lizenz.html
        ├── 97_view_datenschutz.html
        └── 99_view_settings.html
```

**Build-Ziel:** `out\web\` (nicht versioniert, siehe `.gitignore`).

**Prinzip:** Nur **Konfiguration** und **Layout**. Kein Inhalt.
Der Inhalt kommt aus den MD-Dateien über das Manifest.

**Anmerkung zu den View-Dateinamen:** Die Nummerierung (`00_`, `01_`,
… `99_`) ist Konvention, keine harte Regel. Sie hilft bei der
Sortierung im Explorer, hat aber für den Router keine Bedeutung.
Der Router matcht über die Sektionsnamen aus dem Manifest, nicht
über Dateinamen.

---

## §3 — Manifest-Format

Das Manifest ist die **einzige** Konfigurationsdatei für die
Zuordnung Sektion → Dokument. Es ist eine zeilenbasierte
UTF-8-Textdatei.

### §3.1 — Zeilen-Format

```
<Quellpfad>|<Sektion>|<Doc-ID>
```

**Drei Felder, getrennt durch `|`.**

| Feld | Position | Typ | Beispiel |
|---|:-:|---|---|
| `Quellpfad` | 1 | Pfad | `docs/project/Project.md` |
| `Sektion` | 2 | String | `Modules` |
| `Doc-ID` | 3 | String | `Amp` |

**Trenner ist `|`** (Pipe), **nicht** `:`. Grund: Windows-Pfade
können Laufwerksbuchstaben enthalten (`C:\...`), die mit `:` mit
dem Pfad kollidieren würden.

### §3.2 — Sektionen

ProWB kennt **sechs Sektionen**. Die Schreibweise ist
**case-sensitive**:

| Sektion | Zweck |
|---|---|
| `Overview` | Projekt, Architektur, Versionierung, Contributor |
| `Physics` | Physik-Übersicht, Config |
| `Modules` | Modul-Referenzen (12 Kernel-Module) |
| `API` | Kernel-API, SDK-API |
| `Tests` | Testkatalog, Runner, Baseline, Anleitung |
| `Build` | Build-System, Makefiles, Wrapper, ProWB, CI |

Die Sektion ist **im Manifest frei wählbar**, aber der Wert muss
einem der sechs in `prowb.c` hart kodierten Werte entsprechen
(siehe `src\prowb\README.md` §5). Ein unbekannter Sektionsname
führt zu einer Warnung und der Eintrag wird ignoriert.

### §3.3 — Pfad-Regeln

- **Relativ zum Repo-Root**, nicht zu `src_root`.
- **Forward slash** (`/`) als Trenner, auch unter Windows.
  **Nicht** Backslash.
- **Kein führender `./`**.
- **Keine Leerzeichen** im Pfad.
- **Kein abschließender `/`**.
- **Groß-/Kleinschreibung** wie auf der Platte.

**Beispiele für korrekte Pfade:**

```
README.md
CHANGELOG.md
docs/project/Project.md
docs/build/prowb/Makefile.md
src/prowb/README.md
```

**Beispiele für falsche Pfade:**

```
./docs/project/Project.md          (führender ./)
docs\project\Project.md            (Backslash)
docs/project/Project.md/           (abschließender /)
C:\ProPhysics\docs\...             (absoluter Pfad)
```

### §3.4 — Doc-ID

Die Doc-ID ist **eindeutig innerhalb der Sektion**. Sie dient als
URL-Fragment (z. B. `#/Modules/Amp`) und als Schlüssel für die
View-Zuordnung.

**Konventionen:**

- **Keine Leerzeichen.**
- **Unterstriche** für Worttrennung (`Makefile_Main`,
  `WriteBuildInfo`).
- **CamelCase** für zusammengesetzte Namen (`WritingTests`,
  `WebDocsCI`).
- **Kurz halten** — die Doc-ID wird in der URL sichtbar.

**Beispiele:**

| Datei | Doc-ID |
|---|---|
| `docs/project/Amp.md` | `Amp` |
| `docs/build/main/Makefile.md` | `Makefile_Main` |
| `docs/build/helper/write_build_info.md` | `WriteBuildInfo` |
| `docs/test/WRITING_TESTS.md` | `WritingTests` |
| `docs/build/web-docs-ci.md` | `WebDocsCI` |

### §3.5 — Kommentare und Leerzeilen

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

### §3.6 — Reihenfolge

Die Reihenfolge im Manifest bestimmt die Reihenfolge **innerhalb
einer Sektion**. Sektionen selbst werden in der Reihenfolge ihres
ersten Auftretens in der Nav gruppiert (typischerweise
Overview, Physics, Modules, API, Tests, Build).

**Beispiel:** Wenn im Manifest `docs/project/Amp.md` vor
`docs/project/Core.md` steht, erscheint im Portal Amp über Core.

### §3.7 — Aktuelle Anzahl

- **44 Einträge** über **6 Sektionen**.
- Sektion `Overview`: 11 Einträge
- Sektion `Physics`: 2 Einträge
- Sektion `Modules`: 12 Einträge
- Sektion `API`: 2 Einträge
- Sektion `Tests`: 4 Einträge
- Sektion `Build`: 13 Einträge

**Nicht im Manifest:** Utility-Bereiche Lizenz, Datenschutz,
Settings. Diese sind **keine** Manifest-Sektionen und werden vom
Router direkt über die Views `96_view_lizenz.html`,
`97_view_datenschutz.html` und `99_view_settings.html`
angesteuert.

---

## §4 — Einen neuen Doku-Eintrag hinzufügen

**Szenario:** Du hast ein neues Modul-Doc
`docs\project\MyModule.md` geschrieben und willst es ins Portal
aufnehmen.

### §4.1 — Schritt 1: Manifest öffnen

Öffne `docs\web\manifest.txt`.

### §4.2 — Schritt 2: Zeile hinzufügen

Füge an der gewünschten Stelle (Reihenfolge!) eine Zeile hinzu:

```
docs/project/MyModule.md                        |Modules|MyModule
```

| Feld | Wert |
|---|---|
| `Quellpfad` | `docs/project/MyModule.md` (Forward slashes!) |
| `Sektion` | `Modules` (weil es ein Modul ist) |
| `Doc-ID` | `MyModule` |

**Ausrichtung:** Die Pipe-Zeichen sind im aktuellen Manifest
auf eine feste Spalte ausgerichtet (Spalte 49). Diese Ausrichtung
ist **nicht** funktional erforderlich, aber sie macht die Datei
lesbarer. Wer sie beibehält, hält die Konvention ein.

**Länge des Pfads beachten:** Bei kürzeren Pfaden (`README.md`)
wird der Pfad nicht auf Spalte 49 aufgefüllt, sondern endet nach
dem letzten Zeichen; dann folgt **ein** Leerzeichen, dann `|`.

### §4.3 — Schritt 3: Bauen

```cmd
pro_run web
```

### §4.4 — Schritt 4: Prüfen

- `out\web\index.html` im Browser öffnen.
- Über die Sektion `Modules` navigieren.
- Der Eintrag `MyModule` sollte als Karte erscheinen.
- Der MD-Inhalt sollte korrekt gerendert sein.

### §4.5 — Fertig

**Keine andere Datei muss angepasst werden.** Kein C-Code, kein
Template, kein Build-Skript. Das Manifest ist die einzige Stelle.

**Ausnahme:** Wenn die Doc-ID eines Eintrags sich ändert (nicht der
Pfad, nicht die Sektion), muss der Eintrag **neu geschrieben**
werden. Die Doc-ID ist Teil des URL-Schemas.

---

## §5 — Eine neue Sektion hinzufügen

**Szenario:** Du willst eine 7. Sektion `Research` einführen.

**Hinweis:** Sektionen sind in `prowb.c` hart kodiert. Eine neue
Sektion erfordert **C-Änderung**. Das ist ein bewusster Trade-off
(siehe `src\prowb\README.md` §9.4).

### §5.1 — Schritt 1: Manifest erweitern

Füge Zeilen mit `Research` in der Sektions-Spalte hinzu:

```
docs/research/Papers.md                         |Research|Papers
docs/research/Experiments.md                    |Research|Experiments
```

### §5.2 — Schritt 2: Sektion in `prowb.c` ergänzen

In `src\prowb\prowb.c` die Sektions-Konstante erweitern:

```c
typedef enum {
    PROWB_SEC_OVERVIEW = 0,
    PROWB_SEC_PHYSICS,
    PROWB_SEC_MODULES,
    PROWB_SEC_API,
    PROWB_SEC_TESTS,
    PROWB_SEC_BUILD,
    PROWB_SEC_RESEARCH,       /* NEU */
    PROWB_SEC_COUNT
} ProWbSection;
```

Und im Sektions-Array:

```c
static const ProWbSectionInfo g_sections[] = {
    { "Overview", "Overview", "📋" },
    { "Physics",  "Physics",  "⚛️" },
    { "Modules",  "Modules",  "🧩" },
    { "API",      "API",      "🔌" },
    { "Tests",    "Tests",    "🧪" },
    { "Build",    "Build",    "🏗️" },
    { "Research", "Research", "🔬" },   /* NEU */
};
```

**Wichtig:** Die Groß-/Kleinschreibung muss exakt der Sektions-Spalte
im Manifest entsprechen (`Research`, nicht `research`).

### §5.3 — Schritt 3: View anlegen

Kopiere `docs\web\src\views\03_view_modules.html` nach
`docs\web\src\views\07_view_research.html` und passe die
Karten-Generierung an, falls die View einen Sektions-spezifischen
Platzhalter hat.

### §5.4 — Schritt 4: Nav-Template prüfen

`nav.html` enthält die Navigations-Struktur. Die Sektionen-Buttons
sind dort hart kodiert (siehe `nav.html`-Kommentar). Ein neuer
Button muss dort ergänzt werden.

### §5.5 — Schritt 5: Bauen und prüfen

```cmd
pro_run web -Rebuild
```

Die neue Sektion sollte in der Nav erscheinen.

### §5.6 — Schritt 6: Doku nachziehen

- `src\prowb\README.md` §5 — Sektionstabelle erweitern.
- `docs\web\README.md` (diese Datei) §3.2 — Tabelle erweitern.
- `CHANGELOG.md` — Eintrag für die Änderung.

---

## §6 — Templates und Views

### §6.1 — `parts\header.html`

Enthält:

- `<head>` mit Meta-Tags, CSS-Links.
- Branding: `⚛️ ProPhysics // KERNEL DOCS`.
- Version: `KERNEL: 1.23.0 / ETAPPE 23`.
- Theme-Umschalter-Button.

**Nicht ändern:** Die CSS-Links referenzieren `assets/00_reset.css`
etc. — die Reihenfolge ist wichtig (Reset vor Vars vor Layout).

### §6.2 — `parts\nav.html`

Enthält die Navigations-Struktur. Die Sektionen-Buttons sind dort
hart kodiert. Die Dokument-Links **innerhalb** einer Sektion werden
zur Render-Zeit aus dem Manifest generiert.

**Wichtig:** Die Klasse `nav-docs` (oder das Pendant in deiner
Template-Version) wird vom Router selektiert. Wer das Template
ändert, muss die Selektor-Konvention beibehalten.

### §6.3 — `parts\footer.html`

Enthält:

- Copyright: `© 2026 Sascha Alexander Köhne // BrainAI`.
- Links: Lizenz, Datenschutz, Repository.
- Build-Hinweis: `Generated by ProWB`.

**Keine Platzhalter.**

### §6.4 — `views\*.html`

Sektions-Übersichtsseiten. Die Dokument-Karten werden vom Router
zur Laufzeit aus dem Manifest generiert. Die View selbst enthält
das Container-Element und ggf. eine Intro-Text-Passage.

**Detail-Konventionen:** Siehe `docs\web\src\views\` und die
Kommentare im jeweiligen Template. Die genauen CSS-Klassen und
Platzhalter sind in der finalen Template-Version festgelegt.

### §6.5 — `views\00_view_home.html`

Hero + Kurzfassung aus `README.md` (Root) + Sektions-Karten.

### §6.6 — Utility-Views

| View | Zweck |
|---|---|
| `96_view_lizenz.html` | Dual-Licensing (frei für privat, kommerziell nur mit Lizenz) |
| `97_view_datenschutz.html` | DSGVO-Hinweis (Netlify/Pages, statisch, keine Personendaten) |
| `99_view_settings.html` | Theme-Umschaltung, Reset, Links zu Lizenz/Datenschutz |

**Diese Views haben keinen Manifest-Bezug.** Sie werden vom
Router direkt angesteuert (`#/settings`, `#/lizenz`,
`#/datenschutz`).

---

## §7 — Themes

ProWB unterstützt **vier** Themes:

| Theme | Datei | Beschreibung |
|---|---|---|
| `default` | `99_theme_prophysics.css` | Dark Industrial, Petroleum |
| `light` | `99_theme_light.css` | Hell, hoher Kontrast |
| `matrix` | `99_theme_matrix.css` | Grün auf Schwarz |
| `proedc` | `99_theme_prophysics.css` | Alias für `default` |

**Default:** `proedc` (siehe `TODO.md` §5.0).

**Umschaltung:** Client-seitig via `localStorage` in
`src\js\10_app.js`. Kein Server-Roundtrip. Kein Reload.

**Persistenz:** `localStorage.getItem('prophysics-theme')`. Bei
fehlendem Eintrag greift der Default `proedc`.

**CSS-Klassen:** `body.theme-default`, `body.theme-light`,
`body.theme-matrix`. Die CSS-Dateien setzen Variablen auf
`body.theme-<name>`. Der `10_app.js`-Router setzt die Klasse.

**Neues Theme hinzufügen:**

1. `docs\web\src\css\99_theme_<name>.css` anlegen.
2. `header.html` — CSS-Link ergänzen.
3. `10_app.js` — Theme in die Liste aufnehmen.
4. `99_view_settings.html` — Button ergänzen.

---

## §8 — Build-Integration

### §8.1 — Manuell

```cmd
pro_run web
pro_run web -Rebuild
```

`-Rebuild` löscht `out\web\` vor dem Lauf.

### §8.2 — Über nmake (nur die EXE)

```cmd
cd build\prowb
nmake /NOLOGO /f Makefile.nmake CONFIG=release
```

Baut **nur** die Builder-EXE. Der Web-Docs-Build läuft separat
(siehe §8.3).

### §8.3 — Über die EXE direkt

```cmd
bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web
```

Die Argumente:

| Argument | Bedeutung |
|---|---|
| `--manifest` | Schaltet auf Manifest-Modus |
| `docs\web\src` | Wurzel für Templates/Views/Assets |
| `docs\web\manifest.txt` | Manifest-Pfad |
| `out\web` | Ziel-Verzeichnis |

### §8.4 — CI

GitHub Actions: `.github\workflows\web-docs.yml`. Trigger:

- Push auf `main` mit Änderungen in `docs/**`, `src/prowb/**`,
  `build/prowb/**`.
- Manuell via Actions-Tab (`workflow_dispatch`).

**Ergebnis:** Deploy auf GitHub Pages.

**Live-URL:** `https://onkel83.github.io/prophysics/`.

Details siehe `docs\build\web-docs-ci.md`.

---

## §9 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| Sektion fehlt in Nav | Manifest-Zeile mit unbekannter `Sektion` | Gegen §3.2 prüfen (case-sensitive!) |
| Sektion erscheint leer | Alle Einträge haben falsche `Sektion` | Manifest prüfen |
| Doku-Link führt zu 404 | Pfad falsch oder Datei fehlt | Pfad im Manifest prüfen, `pro_run web` stderr lesen |
| Doc-ID nicht eindeutig in Sektion | Zwei Einträge mit gleicher `Sektion`+`Doc-ID` | Doc-ID umbenennen |
| `[ProWB] Docs: 0 geparst, 44 fehlend` | Working Directory falsch | `pro_run web` nutzen (setzt Pfade korrekt) |
| Backslash im Pfad | Windows-Konvention verwendet | Forward slash `/` nutzen |
| Leerzeichen im Pfad | Datei umbenennen oder Manifest-Eintrag anpassen | Pfad korrigieren |
| `docs\project\CHANGELOG.md` existiert nicht | Datei liegt im Root | Pfad zu `CHANGELOG.md` ändern |
| `docs\todo\ToDo.md` existiert nicht | Verzeichnis existiert nicht | Pfad zu `TODO.md` (Root) ändern |
| Theme-Umschaltung geht nicht | `localStorage` blockiert | Inkognito-Modus prüfen |
| Theme persistiert nicht | localStorage geleert | `settings`-View öffnen, Theme neu wählen |
| `out\web\` leer | Build gescheitert | Build-Log prüfen (`pro_run web -v`) |
| MD-Inhalt wird roh angezeigt | Parser-Fehler | `md_parser.c` prüfen, MD-Struktur vereinfachen |
| Tabellen rendern nicht | MD-Tabelle nicht GFM-konform | Format prüfen (`\|---\|`-Trenner) |
| Cross-Ref `[link](other.md)` tot | Phase-1-Limitierung | siehe `src\prowb\README.md` §9.3 |
| Build dauert > 5 s | Unerwartet | Bei 44 kleinen MD-Dateien normal ~1 s |

**Diagnose-Tipp:** `pro_run web -v` zeigt den vollen ProWB-Output
inklusive stderr-Warnungen.

---

## §10 — Siehe auch

| Thema | Datei |
|---|---|
| ProWB-README | `src\prowb\README.md` |
| ProWB-Makefile | `docs\build\prowb\Makefile.md` |
| CI-Workflow | `docs\build\web-docs-ci.md` |
| `pro_run web` | `docs\build\pro_run.md` |
| Build-Übersicht | `docs\build\BUILD_SCRIPT.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Projekt-Roadmap | `docs\project\Project.md` |
| Changelog | `CHANGELOG.md` (`1.23.11`) |
| ProWB-Quellen | `src\prowb\prowb.c` |
| Manifest | `docs\web\manifest.txt` |

---

**Ende docs/web/README.md v1.0.**