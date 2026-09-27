# D1 — `docs/build/ci.md`

**Datei:** `.github\workflows\ci.yml`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** GitHub-Actions-Workflow, der bei jedem Push und Pull
Request auf `main` die schnelle Regression ausführt: Build +
Prio 1, 6, 7 + `SU2-Wilson-Loop`. Laufzeit ~1,5 min.

---

## §1 — Was der Workflow tut

Ein **einziger Job** (`quick-regression`), der fünf Schritte in
fester Reihenfolge ausführt:

| # | Schritt | Dauer (typisch) |
|:-:|---|---:|
| 1 | Checkout + MSVC-Setup | ~5 s |
| 2 | Build (`-Mode all -Rebuild`) | ~6 s |
| 3 | Prio 1 (2D-Basis, 12 Tests) | ~5 s |
| 4 | Prio 6 (Spin-1/2, 1 Test) | < 1 s |
| 5 | Prio 7 (Dirac, 1 Test) | ~15 s |
| 6 | `SU2-Wilson-Loop` (1 Test aus Prio 8) | ~2 s |
| 7 | Log-Upload (auch bei Fehlschlag) | ~5 s |
| | **Gesamt** | **~1,5 min** |

**Nicht enthalten:** Die langen Tests (Prio 5 mit Hydrogen-48,
`Running-Coupling` aus Prio 8). Diese laufen im separaten Workflow
`Alpha-Nightly` (`docs\build\alpha-nightly.md`), der manuell
angestoßen wird.

**Trigger:**

| Auslöser | Bedingung |
|---|---|
| Push | Branch `main` |
| Pull Request | Ziel-Branch `main` |

**Kein Pfad-Filter.** Jeder Push auf `main` und jeder PR gegen
`main` triggert den Workflow — unabhängig davon, welche Dateien
sich geändert haben.

---

## §2 — Ablageort

```
<repo>\
└── .github\
    └── workflows\
        ├── ci.yml                <- diese Datei
        ├── web-docs.yml          <- siehe docs\build\web-docs-ci.md
        └── alpha-nightly.yml     <- siehe docs\build\alpha-nightly.md
```

**Konvention:** Eine YAML-Datei pro Workflow. Kein Wiederverwenden
über `workflow_call` (Phase 1).

---

## §3 — Trigger im Detail

```yaml
on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
```

### §3.1 — Push-Trigger

Jeder Push auf `main` startet den Workflow. Kein Pfad-Filter.

**Konsequenz:**

- Änderungen an `docs\**` triggern **nicht nur** den Web-Docs-Workflow,
  sondern auch die schnelle Regression.
- Änderungen an `src\**`, `build\**`, `tools\**` triggern die
  Regression — was gewollt ist.
- Änderungen an `.github\workflows\ci.yml` selbst triggern den
  Workflow ebenfalls (Reflexivität ist Standard bei GitHub Actions).

**Bewertung:** Der Workflow ist schnell (~1,5 min), ein
Pfad-Filter wäre möglich, aber nicht notwendig. Die schnelle
Regression bei jedem Push ist ein Feature, kein Bug — sie kostet
kaum CI-Minuten.

### §3.2 — Pull-Request-Trigger

Jeder PR gegen `main` startet den Workflow. Das ist der
**Review-Anker**: Wer einen PR stellt, sieht sofort, ob die
schnelle Regression grün ist.

**Wichtig:** Der Workflow läuft auf dem **Merge-Commit** des PRs,
nicht auf dem Branch-Kopf. Bei mehreren Commits im PR läuft er
mehrmals.

---

## §4 — Permissions

Der Workflow setzt **keine** expliziten `permissions`. Damit
gelten die Default-Permissions des Repos:

| Permission | Default (GitHub) |
|---|---|
| `contents` | `read` |
| `metadata` | `read` |
| `pages` | `none` |
| `id-token` | `none` |

**Konsequenz:** Der Workflow kann **nicht** in das Repo schreiben,
**nicht** auf Pages deployen. Nur lesen und Artefakte hochladen.

**Warum das reicht:** Der Workflow prüft nur. Er baut, testet und
lädt Logs hoch. Kein Push, kein Kommentar, kein Deploy.

---

## §5 — Job: `quick-regression`

```yaml
quick-regression:
  runs-on: windows-latest
```

### §5.1 — Warum Windows?

ProPhysics ist ein C99-Kernel mit MSVC-Toolchain (`cl.exe`,
`link.exe`, `nmake.exe`). Die Test-EXEs sind x86-64-Binaries für
Windows. Es gibt keine Linux/macOS-Variante.

`windows-latest` hat Visual Studio vorinstalliert. Notwendig.

### §5.2 — Timeout

**Kein expliziter `timeout-minutes` gesetzt.** Das bedeutet: Der
GitHub-Default von **360 Minuten** (6 Stunden) greift.

**Bewertung:** Bei einer erwarteten Laufzeit von ~1,5 min ist das
großzügig. Ein `timeout-minutes: 15` würde reichen und einen
hängenden Build früher abbrechen. Aktuell aber nicht gesetzt.

### §5.3 — Kein Cache

Der Workflow nutzt **kein** `actions/cache`. Grund: Der Rebuild
dauert ~6 s. Caching wäre Overhead ohne Nutzen.

---

## §6 — Schritte im Detail

### §6.1 — Checkout

```yaml
- uses: actions/checkout@v4
```

Standard-Checkout. `fetch-depth` bleibt Default (1) — ProPhysics
braucht keine Historie für die schnelle Regression.

**Hinweis:** Im Gegensatz zum Web-Docs-Workflow wird hier **kein**
`-GitStamp` im Build verwendet. Die Regression testet den Code,
nicht die Build-Metadaten.

### §6.2 — Setup MSVC

```yaml
- name: Setup MSVC
  uses: ilammy/msvc-dev-cmd@v1
  with:
    arch: x64
```

Setzt die MSVC-Umgebung: `cl.exe`, `link.exe`, `nmake.exe` im
PATH. Notwendig, weil `windows-latest` zwar VS installiert hat,
aber die Developer-Umgebung nicht automatisch aktiviert.

**`arch: x64`:** Baut 64-Bit-Artefakte. Konsistent mit dem Kernel.

### §6.3 — Build

```yaml
- name: Build
  shell: cmd
  run: |
    cd build\main
    build.cmd -Mode all -Rebuild
```

**`-Mode all -Rebuild`:** Kompletter Rebuild in `release`-Konfiguration.

| Was | Effekt |
|---|---|
| `-Mode all` | Kernel + SDK + Tests + ProWB |
| `-Rebuild` | `clean` vor dem Build |

**Warum Rebuild und nicht inkrementell?** GitHub-Runner starten
immer mit einem frischen Checkout. Es gibt keinen vorherigen
Build-Zustand, der inkrementell genutzt werden könnte. `-Rebuild`
ist semantisch korrekt — `clean` findet nichts zu löschen, aber
der Aufruf ist idempotent.

**`shell: cmd`:** NMAKE versteht keine Unix-Shell. Muss `cmd` sein.

**`build.cmd` statt `pro_run`:** Der `.cmd`-Wrapper ist robuster in
CI (kein PowerShell-Startup, kein Encoding-Handling nötig). `pro_run`
bleibt der empfohlene lokale Einstiegspunkt.

**Was gebaut wird:**

- `bin\ProPhysics.dll`
- `bin\pro_sdk_interface.dll`
- `bin\example_alpha_test.exe`
- `bin\example_test_density.exe`
- `bin\example_test_tensor.exe`
- `bin\prowb\prowb.exe`
- `lib\ProPhysics.lib`
- `lib\pro_sdk_interface.lib`
- `BUILD_INFO.txt`

### §6.4 — Prio 1 (2D-Basis)

```yaml
- name: Prio 1 — 2D-Basis
  shell: cmd
  run: |
    cd tools
    run_alpha_tests.cmd -Prio 1
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
```

**Was läuft:** 12 Tests (Amp-Smoke, Born-Regel, Unitary-Tick,
Context-Perm, Wilson-Loop, Local-Gauge, Triangle-Corr, CHSH-Native,
CHSH-Collapse, CHSH-Graph, Density-Regression, Tensor-Regression).

**Explizite Exit-Code-Prüfung:** `if %ERRORLEVEL% neq 0 exit /b`
stellt sicher, dass der Job abbricht, wenn ein Test fehlschlägt.

**Warum explizit?** In `cmd.exe` wird der Exit-Code des letzten
Befehls **nicht** automatisch zum Exit-Code des Steps. Ohne die
Prüfung würde der Job bei einem fehlgeschlagenen Test **weiterlaufen**
und am Ende „grün" sein — obwohl ein Test FAIL war.

**Das ist kritisch.** Ohne diese Zeile wäre die CI wertlos.

### §6.5 — Prio 6 (Spin-1/2)

```yaml
- name: Prio 6 — Spin-1/2
  shell: cmd
  run: |
    cd tools
    run_alpha_tests.cmd -Prio 6
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
```

**Was läuft:** 1 Test (`Spin-Half-Emergence`).

**Warum Prio 6 mit drin?** Spin-1/2 ist eine **Schicht-2-Erweiterung**
(seit Etappe 19). Der Test prüft die SU(2)-Algebra auf zwei
Basiszuständen. Er ist mit < 1 s sehr schnell und deckt eine
wichtige Struktur ab.

### §6.6 — Prio 7 (Dirac)

```yaml
- name: Prio 7 — Dirac
  shell: cmd
  run: |
    cd tools
    run_alpha_tests.cmd -Prio 7
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
```

**Was läuft:** 1 Test (`Dirac-Struktur`).

**Warum Prio 7 mit drin?** Dirac ist die zweite Schicht-2-Erweiterung
(seit Etappe 21). Der Test prüft Clifford-Algebra und Massenterm.
Dauer ~15 s. Der Test ist der einzige in seiner Prio.

### §6.7 — SU(2)-Wilson-Loop

```yaml
- name: SU(2)-Wilson-Loop
  shell: cmd
  run: |
    cd tools
    run_alpha_tests.cmd -Test SU2-Wilson-Loop
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
```

**Was läuft:** 1 Test (`SU2-Wilson-Loop`, T1–T18 + Kernel-Algebra).

**Warum nur dieser Test aus Prio 8?** Prio 8 enthält zwei Tests:

- `SU2-Wilson-Loop` (~2 s) — Leapfrog-Dynamik, Eichinvarianz,
  R7-Konformität.
- `Running-Coupling` (~23 min) — Metropolis-Sampling.

Nur der kurze Test läuft in der schnellen Regression. Der lange
läuft im `Alpha-Nightly`-Workflow.

**`-Test`-Parameter:** `run_alpha_tests.cmd -Test SU2-Wilson-Loop`
selektiert nur diesen Test — unabhängig von der Prio-Struktur.

### §6.8 — Log-Upload

```yaml
- name: Upload logs
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: test-logs
    path: bin/logs/
    retention-days: 7
```

**`if: always()`:** Lädt die Logs **auch bei Fehlschlag** hoch.
Kritisch für die Diagnose.

**Artefakt-Name:** `test-logs`. Kein Suffix — bei mehreren
parallelen Runs kann das zu Konflikten führen (siehe §9).

**Retention:** 7 Tage. Kürzer als beim Web-Docs-Workflow (30 Tage),
weil der Workflow häufiger läuft (jeder Push).

**Was hochgeladen wird:** Alle Dateien aus `bin\logs\`. Format:
`<timestamp>_<Testname>.log`. Details siehe
`docs\test\run_alpha_tests.md` §8.

---

## §7 — Was der Workflow **nicht** prüft

| Prio | Thema | Warum nicht? |
|:-:|---|---|
| 2 | Emergenz (10 Tests) | ~6,5 min — zu lang für Standard-CI |
| 3 | Langlauf (10 Tests) | ~1 min — grenzwertig, aber verzichtbar |
| 4 | 3D-Torus (3 Tests) | ~1 min — dito |
| 5 | Hydrogen + Shared (4 Tests) | ~41 min — Nightly-Territorium |
| 8 | `Running-Coupling` | ~23 min — Nightly-Territorium |

**Begründung:** Die schnelle Regression deckt die „Schicht-1 +
Schicht-2"-Basis ab. Alles, was länger als ~30 s dauert, läuft
im Nightly-Workflow.

**Empfehlung (nicht umgesetzt):** Prio 3 und 4 wären Kandidaten
für die Standard-CI. Sie sind schnell (~2 min zusammen) und decken
Langlauf-Stabilität und 3D-Torus ab. Aktuell sind sie nicht drin.

---

## §8 — Konsolenausgabe

### §8.1 — Erwartete Ausgabe (Erfolg)

```
Run cd build\main
  ...
  [MASTER] === ProPhysics Kernel (CONFIG=release) ===
  [KERNEL] ...
  [MASTER] === SDK Interface (CONFIG=release) ===
  [SDK] ...
  [MASTER] === Tests (CONFIG=release) ===
  [TEST] ...
  [MASTER] === ProWB Builder (CONFIG=release) ===
  [PROWB] ...
  [MASTER] === BUILD_INFO ===
      OK  BUILD_INFO.txt erzeugt.

Run cd tools
  ...
  [ 1/12] Amp-Smoke              ... PASS         0,5s
  [ 2/12] Born-Regel             ... PASS         0,1s
  ...
  PASS: 12   FAIL: 0   | Gesamt: 12

Run cd tools
  [ 1/ 1] Spin-Half              ... PASS         0,3s
  PASS: 1   FAIL: 0   | Gesamt: 1

Run cd tools
  [ 1/ 1] Dirac                  ... PASS        14,8s
  PASS: 1   FAIL: 0   | Gesamt: 1

Run cd tools
  [ 1/ 1] SU2-Wilson-Loop        ... PASS         1,1s
  PASS: 1   FAIL: 0   | Gesamt: 1
```

### §8.2 — Bei Fehlschlag

Der Job stoppt beim ersten fehlgeschlagenen Schritt. Die
**nachfolgenden Schritte werden nicht ausgeführt**. Der Log-Upload
läuft trotzdem (`if: always()`).

**Beispiel:**

```
Run cd tools
  [ 1/12] Amp-Smoke              ... PASS         0,5s
  [ 2/12] Born-Regel             ... FAIL         0,2s
  ...
  PASS: 11   FAIL: 1   | Gesamt: 12
  Nicht-bestandene Tests:
    - Born-Regel    FAIL    ...
  ##[error]Process completed with exit code 1.
```

**Was kommt danach nicht:** Prio 6, Prio 7, SU2-Wilson-Loop werden
übersprungen. **Was trotzdem läuft:** Log-Upload.

---

## §9 — Bekannte Einschränkungen

### §9.1 — Kein `timeout-minutes`

Der Job hat keinen expliziten Timeout. Bei einem hängenden Build
läuft er bis zum GitHub-Default (360 min). Ein
`timeout-minutes: 15` wäre ausreichend.

### §9.2 — Keine `concurrency`-Kontrolle

Bei mehreren schnellen Pushes auf `main` starten mehrere Runs
parallel. Jeder läuft ~1,5 min. Kein Problem bei normaler Nutzung,
aber bei schnellen Merge-Serien kann die Queue wachsen.

**Option (nicht umgesetzt):**

```yaml
concurrency:
  group: ci-${{ github.ref }}
  cancel-in-progress: true
```

### §9.3 — Artefakt-Name ohne Suffix

Der Artefakt-Name `test-logs` ist bei mehreren parallelen Runs
identisch. GitHub Actions hängt automatisch den `run_id` an, aber
im UI erscheinen mehrere Artefakte mit ähnlichem Namen.

### §9.4 — Kein Cache

Kein `actions/cache` für Build-Artefakte. Bei ~6 s Rebuild-Zeit
ist das akzeptabel.

### §9.5 — Kein Pfad-Filter

Jeder Push auf `main` triggert den Workflow — auch reine
Doku-Änderungen. Das kostet CI-Minuten (public Repos: kostenlos,
private: kostenpflichtig).

**Option (nicht umgesetzt):**

```yaml
on:
  push:
    branches: [ main ]
    paths:
      - 'src/**'
      - 'build/**'
      - 'tools/**'
      - '.github/workflows/ci.yml'
```

---

## §10 — Vergleich mit anderen Workflows

| Workflow | Trigger | Laufzeit | Prios | Artefakt | Retention |
|---|---|---:|---|---|---:|
| `ci.yml` | Push + PR auf `main` | ~1,5 min | 1, 6, 7, `SU2-Wilson-Loop` | `test-logs` | 7 Tage |
| `web-docs.yml` | Push auf `main` (nur `docs/**`, `src/prowb/**`, `build/prowb/**`) | ~1 min | — | Pages-Artefakt | (Pages) |
| `alpha-nightly.yml` | manuell | ~64 min | 5, `Running-Coupling` | `alpha-nightly-logs-*` | 30 Tage |

**Überlappung:** `ci.yml` und `web-docs.yml` laufen bei einem
Push, der sowohl Code als auch Doku ändert, **parallel**. Beide
bauen den Kernel (oder Teile davon) unabhängig.

**Bewertung:** Die Überlappung ist akzeptabel. Beide Workflows
sind unabhängig wartbar, und die doppelte Build-Zeit (~12 s
zusammen) ist minimal.

---

## §11 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake not found` | `ilammy/msvc-dev-cmd` fehlt oder falsche Arch | `arch: x64` prüfen |
| Build schlägt fehl | Kompilierfehler | Siehe Log im Actions-Tab |
| Prio 1 FAIL | Kernel-Regression | `bin\logs\*_Amp-Smoke.log` etc. herunterladen |
| Prio 6 FAIL | Spin-1/2-Regression | `bin\logs\*_Spin-Half.log` herunterladen |
| Prio 7 FAIL | Dirac-Regression | `bin\logs\*_Dirac.log` herunterladen |
| SU2-Wilson-Loop FAIL | Leapfrog-Regression oder `OBJ_KERNEL` unvollständig | `build\prophysics\Makefile.nmake` prüfen |
| Job grün, obwohl Test FAIL | `if %ERRORLEVEL%` fehlt in einem Step | Step prüfen — alle müssen die Zeile haben |
| Artefakt-Upload fehlgeschlagen | `bin\logs\` leer | Test-Lauf ist gar nicht gestartet |
| Workflow läuft nicht | Trigger-Bedingung prüft nur `main` | Push auf `main` |
| Workflow läuft zu oft | Jeder Push triggert | Pfad-Filter hinzufügen (§9.5) |

**Diagnose:** Im Actions-Tab auf den Job klicken, dann auf den
Step. GitHub zeigt die volle Ausgabe. Für PASS/FAIL-Details die
hochgeladenen Logs herunterladen.

---

## §12 — Lokale Reproduktion

Der Workflow ist 1:1 lokal reproduzierbar:

```cmd
:: 1. Rebuild wie in CI
cd build\main
build.cmd -Mode all -Rebuild

:: 2. Prios wie in CI
cd ..\..\tools
run_alpha_tests.cmd -Prio 1
run_alpha_tests.cmd -Prio 6
run_alpha_tests.cmd -Prio 7
run_alpha_tests.cmd -Test SU2-Wilson-Loop
```

**Unterschied zur CI:**

- Die CI nutzt `windows-latest` — potenziell andere MSVC-Version
  als lokal.
- Die CI prüft den **Merge-Commit** bei PRs, nicht den Branch-Kopf.
- Lokal gibt es keine `if %ERRORLEVEL%`-Prüfung — der Exit-Code
  ist direkt sichtbar.

**Empfehlung:** Vor jedem Push auf `main` lokal genau diese vier
Tests laufen lassen (`CONTRIBUTING.md` §7.4).

---

## §13 — Was der Workflow nicht tut

- **Kein vollständiger Prio-All-Lauf.** Prio 2, 3, 4, 5, 8 (außer
  `SU2-Wilson-Loop`) fehlen. Siehe §7.
- **Kein Signing.** Kein `build.ps1 -Sign`.
- **Kein Export.** Keine ZIP-Archive.
- **Kein Web-Docs-Build.** Läuft separat in `web-docs.yml`.
- **Kein Deployment.** Kein Push, kein Pages.
- **Kein Kommentar im PR.** Kein Bot-Kommentar mit Testergebnis.
- **Kein Cache.** Kein `actions/cache`.
- **Kein Pfad-Filter.** Jeder Push triggert.

---

## §14 — Siehe auch

| Thema | Datei |
|---|---|
| Web-Docs-CI | `docs\build\web-docs-ci.md` |
| Alpha-Nightly | `docs\build\alpha-nightly.md` |
| Test-Runner | `docs\test\run_alpha_tests.md` |
| Testkatalog | `docs\test\ProPhysics_Testkatalog.md` |
| Build-System | `docs\build\BUILD_SCRIPT.md` |
| `pro_run` | `docs\build\pro_run.md` |
| Beitragen (Regeln) | `CONTRIBUTING.md` §7 |
| Workflow-Datei | `.github\workflows\ci.yml` |

---

**Ende CI-Dokumentation v1.0.0.**