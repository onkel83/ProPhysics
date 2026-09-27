# D2 — `docs/build/alpha-nightly.md`

**Datei:** `.github\workflows\alpha-nightly.yml`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** GitHub-Actions-Workflow, der die **langen** Alpha-Tests
ausführt: Prio 5 (Hydrogen-48, Shared-Reference, Tournament) und
`Running-Coupling` aus Prio 8. Manuell angestoßen, kein Cron.
Laufzeit ~23–64 min je nach Scope.

---

## §1 — Was der Workflow tut

Ein **einziger Job** (`long-tests`), der in Abhängigkeit vom
gewählten **Scope** einen oder zwei lange Testläufe ausführt:

| Scope | Was läuft | Laufzeit |
|---|---|---:|
| `all-long` (Default) | Prio 5 + `Running-Coupling` | ~64 min |
| `prio-5` | Nur Prio 5 (4 Tests) | ~41 min |
| `running-coupling` | Nur `Running-Coupling` | ~23 min |

**Trigger:** ausschließlich **manuell** über den Actions-Tab
(`workflow_dispatch`). **Kein Cron-Trigger.**

**Warum kein Cron?** GitHub deaktiviert `schedule`-Workflows in
Repos, die 60 Tage lang keine Aktivität zeigen — still und ohne
Warnung. Außerdem verbrauchen Nightly-Läufe ohne aktive Entwicklung
CI-Minuten ohne Nutzen. Manuell = du entscheidest.

**Läuft nicht in der Standard-CI.** `.github/workflows/ci.yml`
bleibt schnell (~1,5 min). Dieser Workflow ist eine Ergänzung für
den Fall, dass du die langen Tests prüfen willst.

---

## §2 — Ablageort

```
<repo>\
└── .github\
    └── workflows\
        ├── ci.yml                <- schnell, jeder Push (docs\build\ci.md)
        ├── web-docs.yml          <- Web-Docs-Deploy (docs\build\web-docs-ci.md)
        └── alpha-nightly.yml     <- diese Datei
```

**Konvention:** Eine YAML-Datei pro Workflow. Kein Wiederverwenden
über `workflow_call`.

---

## §3 — Trigger im Detail

```yaml
on:
  workflow_dispatch:
    inputs:
      scope:
        description: 'Welche langen Tests laufen'
        required: true
        default: 'all-long'
        type: choice
        options:
          - all-long
          - prio-5
          - running-coupling
```

### §3.1 — Nur `workflow_dispatch`

Der Workflow kann **nicht** durch Push, Pull Request oder Cron
ausgelöst werden. Er startet ausschließlich über den Actions-Tab.

**Konsequenz:** Kein unbeabsichtigter CI-Verbrauch. Kein Deaktivieren
nach 60 Tagen Inaktivität (das betrifft nur `schedule`).

### §3.2 — Scope-Auswahl

Beim manuellen Start wählt der Nutzer einen von drei Werten:

| Scope | Semantik |
|---|---|
| `all-long` | Beide langen Tests (Prio 5 + Running-Coupling) |
| `prio-5` | Nur die Prio-5-Tests (Hydrogen-48, Shared, Tournament) |
| `running-coupling` | Nur der Metropolis-Test aus Prio 8 |

Der Scope steuert, welche `if`-Bedingungen in den Steps greifen
(siehe §6).

### §3.3 — Kein Cron

**Bewusst nicht implementiert.** Ein `schedule`-Block würde bei
Inaktivität des Repos deaktiviert werden. Wer den Workflow
automatisiert auslösen will, kann das über die GitHub-API oder
einen externen Scheduler tun — aber nicht über GitHub Actions
selbst.

---

## §4 — Permissions

```yaml
permissions:
  contents: read
```

**Minimale Rechte:**

| Permission | Warum |
|---|---|
| `contents: read` | Checkout |

**Nicht gesetzt:** `pages`, `id-token`, `packages`, `actions`.
Der Workflow kann **nicht** in das Repo schreiben, **nicht** auf
Pages deployen, **nicht** an anderen Workflows teilnehmen.

**Konsequenz:** Der Workflow kann nur lesen und Artefakte
hochladen. Das ist für einen Testlauf ausreichend.

---

## §5 — Job: `long-tests`

```yaml
long-tests:
  name: Alpha-Nightly (${{ inputs.scope }})
  runs-on: windows-latest
  timeout-minutes: 120
```

### §5.1 — Warum Windows?

ProPhysics ist ein C99-Kernel mit MSVC-Toolchain. Die Test-EXEs
sind Windows-Binaries. `windows-latest` hat Visual Studio
vorinstalliert.

### §5.2 — Timeout 120 min

**Explizit gesetzt.** Der GitHub-Default wäre 360 min (6 Stunden).
120 min sind ausreichend:

| Scope | Erwartete Laufzeit | Timeout-Reserve |
|---|---:|---:|
| `all-long` | ~64 min | +56 min |
| `prio-5` | ~41 min | +79 min |
| `running-coupling` | ~23 min | +97 min |

Die Reserve deckt einen ~2× langsameren Runner ab. Falls dein
Rechner zu Hause deutlich schneller oder langsamer ist als
`windows-latest`, bleibt die Reserve bestehen.

**Bei Timeout:** Der Job bricht ab, Logs werden trotzdem
hochgeladen (`if: always()` beim Upload-Step).

### §5.3 — Kein Cache

Kein `actions/cache`. Der Rebuild dauert ~6 s. Caching wäre
Overhead ohne Nutzen.

---

## §6 — Schritte im Detail

### §6.1 — Checkout

```yaml
- name: Checkout
  uses: actions/checkout@v4
```

Standard-Checkout. `fetch-depth` bleibt Default (1) — die langen
Tests brauchen keine Historie.

### §6.2 — Setup MSVC

```yaml
- name: Setup MSVC
  uses: ilammy/msvc-dev-cmd@v1
  with:
    arch: x64
```

Setzt die MSVC-Umgebung. Notwendig, weil `windows-latest` VS
installiert hat, aber die Developer-Umgebung nicht automatisch
aktiviert.

**`arch: x64`:** Baut 64-Bit-Artefakte. Konsistent mit dem Kernel.

### §6.3 — Build (kompletter Rebuild)

```yaml
- name: Build (all, Rebuild)
  shell: cmd
  run: |
    cd build\main
    call build.cmd -Mode all -Rebuild
```

**`call build.cmd`:** Wichtig! In `cmd.exe` beendet ein
`.cmd`-Aufruf ohne `call` die umgebende Batch-Datei. GitHub
Actions führt jeden Step als eigene `.cmd`-Datei aus — ohne `call`
würde der Step nach dem ersten `.cmd`-Aufruf enden.

**`-Mode all -Rebuild`:** Kompletter Rebuild — Kernel + SDK +
Tests + ProWB. Konsistent mit der Standard-CI.

**`release`-Konfiguration** (Default). Die langen Tests laufen in
`release`, weil `debug` ~5–10× langsamer wäre und keine
zusätzliche Aussagekraft bringt.

### §6.4 — Verify Build Output

```yaml
- name: Verify build output
  shell: cmd
  run: |
    if not exist "bin\example_alpha_test.exe" (
      echo [FEHLER] example_alpha_test.exe fehlt
      exit /b 1
    )
    if not exist "bin\ProPhysics.dll" (
      echo [FEHLER] ProPhysics.dll fehlt
      exit /b 1
    )
    echo [OK] Build-Output vorhanden
```

**Doppelte Absicherung.** Wenn `build.cmd` ohne Fehler durchläuft,
aber die EXE trotzdem fehlt, bricht der Job hier ab.

**Prüft nur die zwei kritischen Dateien:**

| Datei | Warum |
|---|---|
| `bin\example_alpha_test.exe` | Enthält die Test-Suite; ohne sie laufen Prio 5 und Running-Coupling nicht |
| `bin\ProPhysics.dll` | Kernel-DLL; ohne sie startet die EXE nicht |

Andere Artefakte (SDK-DLL, Import-Libs, ProWB) werden nicht
geprüft — sie werden für die langen Tests nicht gebraucht.

### §6.5 — Prio 5 (nur bei `all-long` oder `prio-5`)

```yaml
- name: Prio 5 (Hydrogen + Shared + Tournament)
  if: inputs.scope == 'all-long' || inputs.scope == 'prio-5'
  shell: cmd
  run: |
    cd tools
    call run_alpha_tests.cmd -Prio 5
```

**Was läuft:** 4 Tests.

| Test | Dauer | Thema |
|---|---:|---|
| Hydrogen (dim=32) | ~170 s | Coulomb-Lokalisierung |
| **Hydrogen-48 (dim=64)** | **~2 281 s** | Coulomb, imaginaere Zeit |
| Shared-Reference | ~4 s | U4 Union-Find |
| Shared-Formula-Tournament | ~1 s | Klassen-Kopplung |

**Gesamt:** ~2 456 s (~41 min).

**`call run_alpha_tests.cmd`:** Wieder mit `call`, damit der Step
nach dem Aufruf nicht abbricht.

**Exit-Code:** `run_alpha_tests.cmd` liefert `0` bei allen PASS,
`1` sonst. Der `.cmd`-Wrapper setzt `chcp 65001` und reicht die
Argumente an PowerShell weiter.

**Wichtig:** Es gibt **kein** explizites `if %ERRORLEVEL% neq 0`-
Statement wie in der Standard-CI. Der `.cmd`-Wrapper propagiert
den Exit-Code korrekt — GitHub Actions wertet ihn aus.

### §6.6 — Running-Coupling (nur bei `all-long` oder `running-coupling`)

```yaml
- name: Running-Coupling (SU(2)-Metropolis)
  if: inputs.scope == 'all-long' || inputs.scope == 'running-coupling'
  shell: cmd
  run: |
    cd tools
    call run_alpha_tests.cmd -Test Running-Coupling
```

**Was läuft:** 1 Test (`Running-Coupling`, Metropolis-Sampling).

**Dauer:** ~1 398 s (~23 min).

**`-Test Running-Coupling`:** Selektiert den Test unabhängig von
der Prio-Struktur. Wichtig, weil Prio 8 auch `SU2-Wilson-Loop`
enthält — der aber bereits in der Standard-CI läuft und hier
nicht dupliziert werden soll.

**Was der Test prüft:**

- Metropolis-Sampler produziert Boltzmann-Verteilung.
- `u_plaq(β, dim)` ist monoton fallend in β.
- Fehlerbalken skaliert wie 1/√N.
- Skalen-Unabhängigkeit zwischen `dim ∈ {16, 32, 64}`.
- **V&V-Anker:** ⟨P⟩(β=2, dim=64) = 0,43346 ± 0,00005 vs.
  Referenz I₂(2)/I₁(2) = 0,43313, Abweichung **0,08 %**.

### §6.7 — Log-Upload

```yaml
- name: Upload logs
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: alpha-nightly-logs-${{ inputs.scope }}-${{ github.run_id }}
    path: bin/logs/
    retention-days: 30
    if-no-files-found: warn
```

**`if: always()`:** Lädt die Logs auch bei Fehlschlag oder Timeout
hoch. Kritisch für die Diagnose.

**Artefakt-Name:** `alpha-nightly-logs-<scope>-<run_id>`.

- `<scope>` = `all-long`, `prio-5` oder `running-coupling`.
- `<run_id>` = eindeutige GitHub-Lauf-ID.

Damit sind mehrere Läufe unterscheidbar. Anders als in der
Standard-CI (dort `test-logs` ohne Suffix).

**Retention: 30 Tage.** Länger als die Standard-CI (7 Tage), weil
Nightly-Läufe seltener sind und die Diagnose wertvoller ist.

**`if-no-files-found: warn`:** Wenn `bin\logs\` leer ist, gibt
es eine Warnung statt eines Fehlers. Tritt auf, wenn der Test
gar nicht gestartet ist — dann ist aber schon ein anderer Step
fehlgeschlagen.

---

## §7 — Was der Workflow prüft

Der Workflow prüft **genau die zwei Tests**, die zu lang für die
Standard-CI sind:

| Prio | Test | Dauer | Warum Nightly? |
|:-:|---|---:|---|
| 5 | Hydrogen-48 | ~2 281 s | > 30 min, dominiert Prio 5 |
| 8 | Running-Coupling | ~1 398 s | > 20 min, dominiert Prio 8 |

Die anderen Prio-5-Tests (`Hydrogen`, `Shared-Reference`,
`Shared-Formula-Tournament`) laufen automatisch mit, wenn `prio-5`
oder `all-long` gewählt ist. Sie sind zusammen ~3 min — vernachlässigbar.

**Was nicht geprüft wird:** Prio 2, 3, 4, 6, 7, 8 (außer
`Running-Coupling`), `SU2-Wilson-Loop`. Alle diese laufen in der
Standard-CI oder sind optional.

**Empfehlung:** Wenn du einen vollständigen Prio-All-Lauf brauchst
(z. B. vor einem Release), nutze den lokalen Lauf:

```cmd
cd tools
pro_run test -Prio all
```

~74 min. Der Nightly-Workflow ist für den Fall gedacht, dass du
**nur** die langen Tests regelmäßig prüfen willst, ohne die
komplette Suite zu fahren.

---

## §8 — Konsolenausgabe

### §8.1 — Erwartete Ausgabe (Erfolg, Scope `all-long`)

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
  [ 1/ 4] Hydrogen                ... PASS       170,5s
  [ 2/ 4] Hydrogen-48             ... PASS      2281,0s
  [ 3/ 4] Shared-Reference        ... PASS         4,3s
  [ 4/ 4] Shared-Formula-Tourn    ... PASS         0,6s
  PASS: 4   FAIL: 0   | Gesamt: 4

Run cd tools
  [ 1/ 1] Running-Coupling       ... PASS      1398,3s
  PASS: 1   FAIL: 0   | Gesamt: 1
```

### §8.2 — Bei Fehlschlag

Der Job stoppt beim ersten fehlgeschlagenen Step. Die
nachfolgenden Steps werden nicht ausgeführt. Der Log-Upload läuft
trotzdem (`if: always()`).

**Beispiel** (Prio 5 schlägt fehl, Running-Coupling wird
übersprungen):

```
Run cd tools
  [ 1/ 4] Hydrogen                ... PASS       170,5s
  [ 2/ 4] Hydrogen-48             ... FAIL      2281,0s
  ...
  PASS: 3   FAIL: 1   | Gesamt: 4
  Nicht-bestandene Tests:
    - Hydrogen-48    FAIL    ...
  ##[error]Process completed with exit code 1.

Run cd tools
  ##[error]This step was skipped.
```

**Was trotzdem läuft:** Log-Upload.

### §8.3 — Bei Timeout

Wenn der Job die 120-Minuten-Grenze erreicht, wird er zwangsweise
beendet. Der Log-Upload läuft trotzdem (durch `if: always()`).

**Wahrscheinliche Ursache:** Ein Runner, der deutlich langsamer
ist als erwartet. Oder ein Test, der in einer Endlosschleife hängt
(was aktuell nicht bekannt ist).

---

## §9 — Bekannte Einschränkungen

### §9.1 — Kein Cron

Kein automatischer Nachtlauf. Der Workflow muss manuell gestartet
werden. Das ist Absicht (§3.3).

### §9.2 — Keine `concurrency`-Kontrolle

Zwei parallele Starts sind möglich. Bei ~1–2 Starts pro Woche
kein Problem.

### §9.3 — Kein Pfad-Filter

Nicht relevant, weil es keinen Auto-Trigger gibt.

### §9.4 — Kein Cache

Der Rebuild dauert ~6 s. Kein Cache nötig.

### §9.5 — Kein `pro_run`

Der Workflow nutzt `run_alpha_tests.cmd` direkt statt
`pro_run test`. Grund: In CI sind die `.cmd`-Wrapper robuster
(kein PowerShell-Startup). `pro_run` bleibt der empfohlene
**lokale** Einstiegspunkt.

### §9.6 — Kein Artefakt-Upload der Build-Artefakte

Nur die Logs werden hochgeladen. Die gebauten DLLs/EXEs werden
nach dem Job verworfen. Wer die Artefakte braucht, baut lokal.

---

## §10 — Vergleich mit den anderen Workflows

| Workflow | Trigger | Laufzeit | Prios | Artefakt | Retention |
|---|---|---:|---|---|---:|
| `ci.yml` | Push + PR auf `main` | ~1,5 min | 1, 6, 7, `SU2-Wilson-Loop` | `test-logs` | 7 Tage |
| `web-docs.yml` | Push (nur `docs/**`, `src/prowb/**`, `build/prowb/**`) | ~1 min | — | Pages-Artefakt | (Pages) |
| `alpha-nightly.yml` | manuell | ~64 min | 5, `Running-Coupling` | `alpha-nightly-logs-*` | 30 Tage |

**Keine Überlappung mit `ci.yml`.** Der Nightly-Workflow prüft
genau die Tests, die die Standard-CI auslässt. Sie ergänzen sich.

**Überlappung mit `web-docs.yml`:** Keine. Der Web-Docs-Workflow
baut nur ProWB + Web-Docs, kein Kernel, keine Tests.

---

## §11 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| Workflow taucht nicht im Actions-Tab auf | Datei nicht committed | `.github/workflows/alpha-nightly.yml` prüfen |
| `Run workflow`-Button fehlt | Branch hat die Datei nicht | Push auf `main` |
| Build schlägt fehl | Kompilierfehler | Log im Actions-Tab lesen |
| `[FEHLER] example_alpha_test.exe fehlt` | Build unvollständig | Vorheriger Build-Step prüfen |
| Prio 5 FAIL | Regression in Hydrogen/Shared | `bin\logs\*_Hydrogen-48.log` herunterladen |
| Running-Coupling FAIL | Regression im Metropolis | `bin\logs\*_Running-Coupling.log` herunterladen |
| Job TIMEOUT | Runner zu langsam oder Test hängt | `timeout-minutes` erhöhen oder Test prüfen |
| Artefakt-Name kollidiert | Zwei parallele Starts | `run_id` unterscheidet automatisch |
| Upload fehlgeschlagen | `bin\logs\` leer | Test-Lauf ist gar nicht gestartet |

**Diagnose:** Actions-Tab → `Alpha-Nightly` → letzter Lauf →
Job `long-tests` → Steps aufklappen. Artefakt unter „Summary" →
„Artifacts".

---

## §12 — Lokale Reproduktion

Der Workflow ist 1:1 lokal reproduzierbar:

```cmd
:: 1. Rebuild wie in CI
cd build\main
build.cmd -Mode all -Rebuild

:: 2. Scope auswaehlen

:: Scope all-long
cd ..\..\tools
run_alpha_tests.cmd -Prio 5
run_alpha_tests.cmd -Test Running-Coupling

:: Scope prio-5
cd ..\..\tools
run_alpha_tests.cmd -Prio 5

:: Scope running-coupling
cd ..\..\tools
run_alpha_tests.cmd -Test Running-Coupling
```

**Laufzeit lokal:**

- Auf einem Entwicklungsrechner (~gleich schnell wie
  `windows-latest`): identisch zur CI-Schätzung.
- Auf einem langsameren Rechner: Faktor 1,5–2 möglich.
- Auf einem schnelleren Rechner: Faktor 0,7–0,8 möglich.

**Unterschied zur CI:**

- Der Workflow nutzt `call build.cmd` (Batch-Kontext), lokal
  reicht `build.cmd`.
- Die CI hat einen harten Timeout (120 min), lokal nicht.

---

## §13 — Was der Workflow nicht tut

- **Kein automatischer Nachtlauf.** Manuell per `workflow_dispatch`.
- **Kein Pfad-Filter.** Nicht relevant ohne Auto-Trigger.
- **Kein vollständiger Prio-All-Lauf.** Nur die langen Tests.
- **Kein Signing, kein Export, kein Deploy.**
- **Kein Web-Docs-Build.** Läuft in `web-docs.yml`.
- **Kein Cache.**
- **Kein PR-Kommentar.** Kein Bot-Kommentar mit Testergebnis.
- **Kein Push von Artefakten ins Repo.** Nur Log-Upload.

---

## §14 — Siehe auch

| Thema | Datei |
|---|---|
| Standard-CI | `docs\build\ci.md` |
| Web-Docs-CI | `docs\build\web-docs-ci.md` |
| Test-Runner | `docs\test\run_alpha_tests.md` |
| Testkatalog | `docs\test\ProPhysics_Testkatalog.md` |
| Test-Baseline | `docs\test\BASELINE.md` |
| Build-System | `docs\build\BUILD_SCRIPT.md` |
| `pro_run` | `docs\build\pro_run.md` |
| Beitragen (Regeln) | `CONTRIBUTING.md` §7 |
| Workflow-Datei | `.github\workflows\alpha-nightly.yml` |

---

**Ende Alpha-Nightly-Dokumentation v1.0.0.**