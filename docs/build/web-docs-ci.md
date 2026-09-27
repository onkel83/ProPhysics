# ProPhysics Web-Docs — CI-Workflow

**Datei:** `.github\workflows\web-docs.yml`
**Version:** 1.0.1 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** GitHub-Actions-Workflow, der den ProWB-Builder baut und
die Web-Docs aus `docs\web\` nach GitHub Pages deployt.

---

## §1 — Was der Workflow tut

Der Workflow hat **zwei Jobs**:

| Job | Läuft auf | Zweck |
|---|---|---|
| `build` | `windows-latest` | Baut ProWB, generiert Web-Docs, lädt Pages-Artefakt hoch |
| `deploy` | `ubuntu-latest` | Nimmt das Artefakt, deployt es auf GitHub Pages |

**Reihenfolge:** `deploy` wartet auf `build` (`needs: build`). Wenn
`build` scheitert, läuft `deploy` nicht.

**Trigger:**

| Auslöser | Bedingung |
|---|---|
| Push | Branch `main`, Änderungen an `docs/**`, `src/prowb/**`, `build/prowb/**` oder am Workflow selbst |
| Manuell | Actions-Tab → „Web-Docs" → „Run workflow" |

**Ergebnis:** Live-URL `https://onkel83.github.io/prophysics/`.

**Abgrenzung zu den anderen Workflows:**

| Workflow | Zweck | Laufzeit |
|---|---|---|
| `ci.yml` | Schnelle Regression (Prio 1, 6, 7, `SU2-Wilson-Loop`) | ~1,5 min |
| **`web-docs.yml`** (dieser) | Web-Docs-Build + Deploy auf GitHub Pages | ~1 min |
| `alpha-nightly.yml` | Lange Tests (Prio 5 + `Running-Coupling`), manuell | ~64 min |

Details siehe `docs\build\ci.md` und `docs\build\alpha-nightly.md`.

---

## §2 — Ablageort

```
<repo>\
└── .github\
    └── workflows\
        ├── ci.yml                <- schnell, jeder Push (docs\build\ci.md)
        ├── web-docs.yml          <- diese Datei
        └── alpha-nightly.yml     <- manuell (docs\build\alpha-nightly.md)
```

**Konvention:** Eine YAML-Datei pro Workflow. Kein Wiederverwenden
über `workflow_call` (Phase 1).

---

## §3 — Trigger im Detail

```yaml
on:
  push:
    branches: [main]
    paths:
      - 'docs/**'
      - 'src/prowb/**'
      - 'build/prowb/**'
      - '.github/workflows/web-docs.yml'
  workflow_dispatch:
```

### §3.1 — Push-Trigger

| Pfad-Muster | Warum |
|---|---|
| `docs/**` | Änderung an Doku oder Web-Source (Manifest, Templates, CSS, JS) |
| `src/prowb/**` | Änderung am Builder |
| `build/prowb/**` | Änderung am ProWB-Makefile |
| `.github/workflows/web-docs.yml` | Änderung am Workflow selbst |

**Nicht getriggert:** Änderungen an Kernel-Code (`src/prophysics/`),
SDK, Tests, oder Build-Skripten anderer Module. Die haben mit
den Web-Docs nichts zu tun.

**Konsequenz:** Der Web-Docs-Build läuft **nur** wenn er relevant
ist. Kein unnötiger CI-Verbrauch.

**Vergleich mit `ci.yml`:** Der Standard-CI-Workflow hat **keinen**
Pfad-Filter — er läuft bei jedem Push auf `main`. Der Web-Docs-Workflow
ist selektiver (siehe `docs\build\ci.md` §9.5 für Diskussion).

### §3.2 — Manueller Trigger

`workflow_dispatch` erlaubt das Starten über den Actions-Tab ohne
Push. Nützlich für:

- Test-Lauf nach Settings-Änderung.
- Re-Deploy nach Pages-Konfigurationsänderung.
- Debug bei fehlgeschlagenem Auto-Lauf.

**Vergleich mit `alpha-nightly.yml`:** Der Nightly-Workflow ist
**ausschließlich** manuell. Der Web-Docs-Workflow läuft normalerweise
automatisch und manuell nur im Ausnahmefall.

---

## §4 — Permissions

```yaml
permissions:
  contents: read
  pages: write
  id-token: write
```

| Permission | Warum |
|---|---|
| `contents: read` | Repo lesen (Checkout) |
| `pages: write` | Pages-Artefakt schreiben und deployen |
| `id-token: write` | OIDC-Token für `deploy-pages` |

**Default-Permissions** werden überschrieben. Das ist **notwendig**,
weil GitHub seit 2023 für Pages-Deploy die explizite
`pages: write`-Permission verlangt.

**Sicherheit:** Kein `contents: write` — der Workflow darf **nicht**
in das Repo committen. Er deployt nur nach Pages.

**Vergleich mit `ci.yml`:** Der Standard-CI-Workflow setzt **keine**
expliziten Permissions — es reichen die Defaults (`contents: read`).
Der Web-Docs-Workflow braucht die zusätzlichen `pages`/`id-token`-
Permissions.

**Vergleich mit `alpha-nightly.yml`:** Der Nightly-Workflow setzt
explizit `contents: read`, weil er keine Pages braucht.

---

## §5 — Concurrency

```yaml
concurrency:
  group: pages
  cancel-in-progress: false
```

| Feld | Wert | Bedeutung |
|---|---|---|
| `group` | `pages` | Eine Gruppe für alle Pages-Deploys |
| `cancel-in-progress` | `false` | Laufende Deploys **nicht** abbrechen |

**Warum `cancel-in-progress: false`?** Ein abgebrochener
Pages-Deploy kann den Pages-Zustand in einen inkonsistenten
Zustand bringen. GitHub empfiehlt explizit `false` für
Pages-Deploys.

**Konsequenz:** Bei mehreren schnellen Pushes läuft nur **ein**
Deploy gleichzeitig. Nachfolgende warten.

**Vergleich mit `ci.yml`:** Die Standard-CI hat **keine**
`concurrency`-Kontrolle. Bei schnellen Pushes laufen mehrere
Test-Läufe parallel — das ist akzeptabel (siehe `docs\build\ci.md`
§9.2).

**Vergleich mit `alpha-nightly.yml`:** Der Nightly-Workflow hat
ebenfalls keine `concurrency`-Kontrolle. Bei 1–2 Starts pro Woche
unkritisch.

---

## §6 — Job 1: `build`

```yaml
build:
  name: Build ProWB + Web-Docs
  runs-on: windows-latest
```

### §6.1 — Warum Windows?

ProWB ist eine C99-Windows-EXE. Der Build benötigt `cl.exe`,
`link.exe`, `nmake.exe` aus dem MSVC-Toolchain.
`windows-latest` hat Visual Studio vorinstalliert.

**Alternative wäre:** Linux-Runner mit MinGW-Cross-Compile. Das
ist komplexer und liefert eine Linux-EXE, die auf Windows nicht
läuft. Bleibt bei Windows.

### §6.2 — Schritte

#### §6.2.1 — Checkout

```yaml
- name: Checkout
  uses: actions/checkout@v4
```

Standard-Checkout. `fetch-depth` bleibt Default (1) — ProWB
braucht keine Historie.

#### §6.2.2 — Setup MSVC

```yaml
- name: Setup MSVC
  uses: ilammy/msvc-dev-cmd@v1
  with:
    arch: x64
```

Setzt die MSVC-Umgebung: `cl.exe`, `link.exe`, `nmake.exe` im
PATH. Notwendig, weil `windows-latest` zwar VS installiert hat,
aber die Developer-Umgebung nicht automatisch aktiviert.

**`arch: x64`:** Baut 64-Bit-EXE. Konsistent mit dem Kernel.

#### §6.2.3 — Build ProWB Builder

```yaml
- name: Build ProWB Builder
  shell: cmd
  run: |
    cd build\prowb
    nmake /f Makefile.nmake CONFIG=release
```

Baut `bin\prowb\prowb.exe` aus `src\prowb\*.c`.

**`shell: cmd`:** NMAKE versteht keine Unix-Shell. Muss `cmd` sein.

**`CONFIG=release`:** Kein Debug-Build in CI.

#### §6.2.4 — Verify Builder

```yaml
- name: Verify builder exists
  shell: cmd
  run: |
    if not exist "bin\prowb\prowb.exe" (
      echo [FEHLER] bin\prowb\prowb.exe wurde nicht gebaut
      exit /b 1
    )
    echo [OK] bin\prowb\prowb.exe existiert
```

Doppelte Absicherung: Wenn NMAKE ohne Fehler durchläuft, aber
die EXE trotzdem fehlt, bricht der Job hier ab.

**Warum nicht nur den Exit-Code prüfen?** NMAKE kann bei
bestimmten Fehlern einen Null-Exit liefern. Explizite
Datei-Prüfung ist robuster.

#### §6.2.5 — Generate Web-Docs

```yaml
- name: Generate Web-Docs
  shell: cmd
  run: |
    bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web
```

Ruft ProWB im Manifest-Modus auf. Erzeugt `out\web\index.html`
plus Assets.

**Working Directory:** Repo-Root (GitHub Actions Default).
Pfade sind relativ zum Repo-Root.

#### §6.2.6 — Verify Output

```yaml
- name: Verify output exists
  shell: cmd
  run: |
    if not exist "out\web\index.html" (
      echo [FEHLER] out\web\index.html wurde nicht erzeugt
      exit /b 1
    )
    for %%F in (out\web\index.html) do set SIZE=%%~zF
    echo [OK] out\web\index.html erzeugt (%%~zF Bytes)
```

Prüft, dass `index.html` existiert und gibt die Größe aus.

**Erwartete Größe:** ~1,2 MB (44 MD-Dateien eingebettet).

**Warnung bei zu kleiner Datei:** Wenn die Größe < 100 KB ist,
deutet das auf einen Parser-Fehler hin. Aktuell nicht als Fehler
behandelt — kann später verschärft werden.

#### §6.2.7 — Upload Pages Artifact

```yaml
- name: Upload Pages artifact
  uses: actions/upload-pages-artifact@v3
  with:
    path: out/web
```

Packt `out\web\` in ein Pages-Artefakt und lädt es hoch. Der
`deploy`-Job nutzt es.

**Nicht `upload-artifact`:** `upload-pages-artifact` ist die
spezialisierte Version für Pages. Sie setzt die richtigen
Metadaten für `deploy-pages`.

**Vergleich mit den anderen Workflows:**

| Workflow | Artefakt-Typ | Retention |
|---|---|---|
| `ci.yml` | `upload-artifact` (`test-logs`) | 7 Tage |
| `web-docs.yml` | `upload-pages-artifact` | (Pages) |
| `alpha-nightly.yml` | `upload-artifact` (`alpha-nightly-logs-*`) | 30 Tage |

---

## §7 — Job 2: `deploy`

```yaml
deploy:
  name: Deploy to GitHub Pages
  needs: build
  runs-on: ubuntu-latest

  environment:
    name: github-pages
    url: ${{ steps.deployment.outputs.page_url }}

  steps:
    - name: Deploy
      id: deployment
      uses: actions/deploy-pages@v4
```

### §7.1 — Warum Ubuntu?

`deploy-pages` ist plattformunabhängig. Ubuntu-Runner sind
schneller und billiger als Windows-Runner. Der Deploy braucht
keinen MSVC.

### §7.2 — Environment

Das `environment: github-pages` ist **vorgeschrieben** von
GitHub. Es:

- Verlinkt den Job mit der Pages-Umgebung.
- Aktiviert den Schutzmechanismus (bei privaten Repos).
- Setzt `steps.deployment.outputs.page_url` für die Live-URL.

### §7.3 — Ein Schritt

Der Deploy-Job hat **einen** Schritt. `actions/deploy-pages@v4`
nimmt das Artefakt aus §6.2.7 und deployt es.

**Kein Checkout nötig.** Der Job hat keinen Repo-Zugriff.

---

## §8 — Voraussetzungen (einmalig)

### §8.1 — GitHub Pages auf „GitHub Actions" umstellen

**Repo → Settings → Pages → Build and deployment:**

| Feld | Wert |
|---|---|
| **Source** | `GitHub Actions` |

**Nicht** „Deploy from branch". Der alte Modus läuft nicht
parallel zu Actions-Deploys.

Nach der Umstellung ist die Seite erreichbar unter:

```
https://onkel83.github.io/prophysics/
```

### §8.2 — Repo-Inhalt prüfen

Der Workflow braucht folgende Dateien **committed**:

**Builder-Quellen:**
- `src/prowb/prowb.c`
- `src/prowb/md_parser.c`
- `src/prowb/header/prowb.h`
- `src/prowb/header/md_parser.h`

**Build-Skript:**
- `build/prowb/Makefile.nmake`

**Web-Source:**
- `docs/web/manifest.txt`
- `docs/web/src/parts/*.html`
- `docs/web/src/css/*.css`
- `docs/web/src/js/*.js`
- `docs/web/src/views/*.html`

**Prüfen:**

```cmd
cd C:\Users\koehn\source\repos\ProPhysics
git ls-files src\prowb build\prowb docs\web
```

Wenn Dateien fehlen:

```cmd
git add src\prowb build\prowb docs\web
git commit -m "ProWB: Web-Docs Builder + Content"
git push
```

### §8.3 — `.gitignore` prüfen

`out/` und `bin/prowb/` dürfen **nicht** versioniert werden.

```text
out/
bin/prowb/
```

**Achtung:** `bin/` ist **nicht** pauschal ignoriert (weil
`bin/ProPhysics.dll` etc. versioniert sind). Nur `bin/prowb/`
ausschließen.

---

## §9 — Was beim Push passiert

1. **Trigger:** Push auf `main` mit Änderung in `docs/**`
   (oder manuell).
2. **Job `build`** startet auf `windows-latest` (~40 s):
   - Checkout.
   - MSVC-Setup (~5 s).
   - ProWB-Build (~10 s).
   - Verify (~1 s).
   - Web-Docs generieren (~5 s).
   - Verify (~1 s).
   - Artefakt-Upload (~5 s).
3. **Job `deploy`** startet auf `ubuntu-latest` (~10 s):
   - `deploy-pages` lädt das Artefakt.
   - Deploy auf Pages-Infrastruktur.
4. **Live** unter `https://onkel83.github.io/prophysics/`.

**Gesamtzeit:** ~1 Minute bei warmem Cache. Beim ersten Lauf
(MSVC-Setup cold) ~2 Minuten.

**Parallel möglich:** Wenn derselbe Push auch Kernel-Code ändert,
läuft `ci.yml` parallel (~1,5 min). Beide Workflows sind unabhängig.

---

## §10 — Verifikation nach dem Push

### §10.1 — Actions-Tab

- Workflow „Web-Docs" läuft.
- Beide Jobs (`build`, `deploy`) grün.
- `deploy`-Job zeigt die Live-URL.

### §10.2 — Console-Output des `build`-Jobs

Erwartete Zeilen:

```
[CC] prowb.c
[CC] md_parser.c
[LD] ..\..\bin\prowb\prowb.exe
[OK] bin\prowb\prowb.exe existiert
[ProWB] Manifest geladen: 44 Einträge, 6 Sektionen
[ProWB]   Docs: 44 geparst, 0 fehlend
[ProWB] Build erfolgreich.
[OK] out\web\index.html erzeugt (1225907 Bytes)
```

**Bei Warnungen:** Zeilen wie

```
[ProWB] Warnung: unbekannter Platzhalter {{XYZ}}
[ProWB] Warnung: MD-Quelle fehlt: docs\project\Missing.md
```

sind **kein** Fehler. Der Build läuft durch. Wer sie beheben
will, prüft Manifest und Templates.

**Vergleich mit `ci.yml`:** Der Standard-CI-Workflow hat
ähnliche Verify-Steps, gibt aber PASS/FAIL-Zeilen pro Test aus.
Der Web-Docs-Workflow gibt Dateigrößen und Parser-Statistiken aus.

**Vergleich mit `alpha-nightly.yml`:** Der Nightly-Workflow
gibt die vollen Test-Ergebnisse (Prio 5 + `Running-Coupling`)
aus.

### §10.3 — Live-URL

`https://onkel83.github.io/prophysics/`:

- Home-Seite mit Hero + Sektions-Karten.
- Nav mit 6 Sektionen.
- Alle 44 Docs erreichbar.
- Theme-Umschaltung funktioniert.

**Bei 404:** Erster Deploy braucht manchmal 1–2 Minuten, bis die
Pages-Infrastruktur propagiert. Warten, dann neu laden.

---

## §11 — Badge im README

Optional ein Status-Badge:

```markdown
![Web-Docs](https://github.com/onkel83/prophysics/actions/workflows/web-docs.yml/badge.svg)
```

**Position:** Unter dem Titel in `README.md`, neben anderen
Badges (falls vorhanden).

**Vergleich mit anderen Badges:**

```markdown
![CI](https://github.com/onkel83/prophysics/actions/workflows/ci.yml/badge.svg)
![Web-Docs](https://github.com/onkel83/prophysics/actions/workflows/web-docs.yml/badge.svg)
```

**Nightly-Badge:** Nicht empfohlen, weil der Workflow manuell
ausgelöst wird — ein grauer Badge würde fälschlich „nicht
gelaufen" anzeigen.

---

## §12 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake not found` | `ilammy/msvc-dev-cmd` fehlt oder falsche Arch | Schritt prüfen, `arch: x64` |
| `fatal error U1073: prowb.h not found` | `src/prowb/header/` fehlt im Repo | committen |
| `fatal error U1073: Makefile.nmake not found` | `build/prowb/` fehlt | committen |
| `Manifest nicht gefunden` | `docs/web/manifest.txt` nicht committed | committen |
| `Docs: 0 geparst, 44 fehlend` | Working Directory falsch | Action ruft aus Repo-Root auf — prüfen |
| `out\web\index.html fehlt` | ProWB-Build gescheitert | Build-Log prüfen |
| Pages-Deploy schlägt fehl | Pages-Source nicht auf „GitHub Actions" | Settings → Pages prüfen |
| 404 auf Live-URL | Erster Deploy braucht 1–2 Min | warten, dann neu laden |
| Workflow läuft nicht | Trigger-Pfade stimmen nicht | `paths`-Filter prüfen |
| Nur `build` läuft, `deploy` nicht | `build` hat Fehler | Logs prüfen |
| Deploy-Job hängt | Concurrency-Lock | vorherigen Lauf abwarten |
| Unerwartet viele Trigger | Push auf `docs/**` bei jedem Doc-Edit | gewollt |

**Abgrenzung zu den anderen Workflows:**

- **`ci.yml` schlägt fehl, Web-Docs läuft?** Normal. Beide haben
  unterschiedliche Trigger. Ein Kernel-Fehler blockiert die
  Web-Docs nicht.
- **Web-Docs schlägt fehl, `ci.yml` läuft?** Normal. Ein
  Manifest-Fehler blockiert die Standard-CI nicht.
- **Beide schlagen fehl?** Meist ein Build-Fehler in
  `src/prowb/` oder ein fehlender Commit.

**Diagnose:** Im Actions-Tab auf den Job klicken, dann auf den
Schritt. GitHub zeigt die volle Ausgabe.

---

## §13 — Was der Workflow nicht tut

- **Kein Kernel-Build.** Kein `nmake all` im Master.
- **Keine Tests.** Prio 1/6/7 laufen in `ci.yml`
  (`docs\build\ci.md`). Prio 5 + `Running-Coupling` laufen in
  `alpha-nightly.yml` (`docs\build\alpha-nightly.md`).
- **Kein SDK-Build.** SDK ist unabhängig.
- **Kein Deployment außerhalb Pages.** Kein FTP, kein S3.
- **Kein Signing.** Pages-Inhalt ist nicht signiert.
- **Keine MD-Kopien.** Die Doku bleibt an ihren Pfaden.
- **Kein Nightly-Trigger.** Der Nightly-Workflow wird manuell
  gestartet, nicht durch Web-Docs-Deploys.

**Konsequenz:** Der Web-Docs-Workflow ist **unabhängig** von den
anderen beiden. Ein fehlgeschlagener Web-Docs-Build blockiert
weder die Standard-CI noch den Nightly-Job.

---

## §14 — Siehe auch

| Thema | Datei |
|---|---|
| Standard-CI | `docs\build\ci.md` |
| Alpha-Nightly | `docs\build\alpha-nightly.md` |
| ProWB-README | `src\prowb\README.md` |
| ProWB-Makefile | `docs\build\prowb\Makefile.md` |
| Manifest-Pflege | `docs\web\README.md` |
| `pro_run web` | `docs\build\pro_run.md` |
| Build-Übersicht | `docs\build\BUILD_SCRIPT.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Test-Runner | `docs\test\run_alpha_tests.md` |
| Changelog | `CHANGELOG.md` (`1.23.11`) |
| Workflow-Datei | `.github\workflows\web-docs.yml` |

---

## §15 — Workflow-Übersicht

Drei unabhängige GitHub-Actions-Workflows. Jeder mit eigenem
Zweck, eigenem Trigger und eigener Laufzeit.

| Aspekt | `ci.yml` | `web-docs.yml` | `alpha-nightly.yml` |
|---|---|---|---|
| **Zweck** | Schnelle Regression | Doku-Deploy | Lange Tests |
| **Trigger** | Push + PR auf `main` | Push auf `main` (Pfad-Filter) + manuell | nur manuell |
| **Runner** | `windows-latest` | `windows-latest` + `ubuntu-latest` | `windows-latest` |
| **Dauer** | ~1,5 min | ~1 min | ~64 min |
| **Permissions** | (Defaults) | `contents: read` + `pages: write` + `id-token: write` | `contents: read` |
| **Concurrency** | (keine) | `pages` (cancel: false) | (keine) |
| **Was läuft** | Build + Prio 1, 6, 7, `SU2-Wilson-Loop` | ProWB-Build + Web-Docs + Deploy | Prio 5 + `Running-Coupling` |
| **Artefakt** | `test-logs` | Pages-Artefakt | `alpha-nightly-logs-<scope>-<run_id>` |
| **Retention** | 7 Tage | (Pages) | 30 Tage |
| **Doku** | `docs\build\ci.md` | `docs\build\web-docs-ci.md` (diese Datei) | `docs\build\alpha-nightly.md` |

**Gemeinsamkeiten:**

- Alle nutzen `windows-latest` (außer der Deploy-Job).
- Alle nutzen `ilammy/msvc-dev-cmd@v1` mit `arch: x64`.
- Alle nutzen `actions/checkout@v4`.

**Unterschiede:**

- Nur `web-docs.yml` hat einen zweiten Job (`deploy`).
- Nur `web-docs.yml` hat `concurrency`-Kontrolle.
- Nur `web-docs.yml` schreibt außerhalb der Artefakte
  (Pages-Deploy).
- Nur `alpha-nightly.yml` ist rein manuell.
- Nur `ci.yml` läuft auch auf Pull Requests.

**Überlappung:**

Wenn ein Push sowohl Kernel-Code als auch Doku ändert, laufen
`ci.yml` und `web-docs.yml` **parallel**. Das ist gewollt — die
beiden Workflows sind unabhängig.

Der Nightly-Workflow läuft **nie** automatisch. Er muss manuell
gestartet werden.

---

**Ende Web-Docs-CI-Dokumentation v1.0.1.**