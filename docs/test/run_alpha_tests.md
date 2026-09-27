# ProPhysics Alpha-Test-Runner

**Dateien:** `tools\run_alpha_tests.ps1` + `tools\run_alpha_tests.cmd`
**Version:** 1.1.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Fuehrt die Alpha-Test-Suite sequenziell aus, schreibt pro Test
ein Log und liefert eine PASS/FAIL-Bilanz mit Exit-Code.

---

## 1. Was das Skript tut

Fuer eine gewaehlte Prioritaetsstufe (Prio 1–8, Range, Liste oder `all`):

1. **Test fuer Test** starten (als separater Prozess)
2. **Ausgabe sammeln** (stdout + stderr, UTF-8)
3. **Ergebnis auswerten** per Regex auf PASS/FAIL-Marker
4. **Log schreiben** pro Test nach `<LogDir>\<timestamp>_<name>.log`
5. **Zusammenfassung** als Tabelle ausgeben
6. **Exit-Code** zurueckgeben: `0` bei allen PASS, `1` sonst

Der Runner fuehrt Tests **nicht** parallel aus. Sequenziell ist Absicht:
Test-Ausgaben sind nicht thread-safe, und einzelne Tests brauchen
>30 min (Hydrogen-48), was parallele Laeufe unuebersichtlich machen
wuerde.

**Empfohlener Aufruf:** ueber `pro_run test …` in `tools\`. Der
Runner bleibt auch direkt aufrufbar. `pro_run` dispatcht auf dieses
Skript und reicht Parameter wie `-Prio`, `-Test`, `-LogDir` durch.

---

## 2. Ablageort

Der Runner liegt in `tools\`. Die EXEs bleiben in `bin\`; das Skript
ist self-locating und zeigt automatisch auf `<repo>\bin`.

```
<repo>\
├── bin\
│   ├── example_alpha_test.exe
│   ├── example_test_density.exe
│   ├── example_test_tensor.exe
│   ├── ProPhysics.dll
│   ├── pro_sdk_interface.dll
│   └── logs\                       <- Ziel der Logs (Default)
├── tools\
│   ├── pro_run.cmd / .ps1          <- zentraler Einstiegspunkt
│   ├── run_alpha_tests.cmd         <- Wrapper fuer cmd.exe
│   └── run_alpha_tests.ps1         <- dieses Skript
└── docs\
    └── test\
        └── run_alpha_tests.md      <- diese Datei
```

**Self-Locating:** Das Skript findet seinen Ablageort ueber
`$PSScriptRoot`. Der Default `-ExeDir` ist `$PSScriptRoot\..\bin`.
Damit funktioniert der Aufruf aus jedem CWD.

---

## 3. Aufruf

### 3.1 Ueber `pro_run` (empfohlen)

```cmd
pro_run test
pro_run test -Prio 1-4
pro_run test -Prio 8
pro_run test -Test Running-Coupling -LogDir C:\logs
```

Details siehe `docs\build\pro_run.md`. `pro_run test` dispatcht auf
`tools\run_alpha_tests.ps1` und reicht alle Parameter durch.

### 3.2 Ueber `run_alpha_tests.cmd` (direkt)

```cmd
cd tools
run_alpha_tests.cmd -Prio 8
run_alpha_tests.cmd -Prio all
run_alpha_tests.cmd -Prio 1-4
run_alpha_tests.cmd -Test Dirac
```

### 3.3 Direkt ueber PowerShell

```powershell
cd tools
.\run_alpha_tests.ps1 -Prio 8
.\run_alpha_tests.ps1 -Prio all
.\run_alpha_tests.ps1 -Test Running-Coupling
```

### 3.4 Aus jedem anderen CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\tools\run_alpha_tests.cmd -Prio all
```

### 3.5 Interaktiv (ohne Argumente)

```powershell
.\run_alpha_tests.ps1
```

PowerShell fragt nach `-Prio`. Nuetzlich beim Debuggen.

---

## 4. Parameter

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Prio` | String | `all` | `all` \| `N` \| `N-M` \| `N,M,K` (N in 1..8) |
| `-Test` | String | (leer) | Einzelner Test nach Name (statt Prio) |
| `-ExeDir` | Pfad | `$PSScriptRoot\..\bin` | Wo die EXEs liegen |
| `-DllDir` | Pfad | identisch mit `-ExeDir` | Wo die DLLs liegen |
| `-LogDir` | Pfad | `<ExeDir>\logs` | Ziel der Logs |

**`-Prio`** akzeptiert drei Syntax-Formen:

| Form | Beispiel | Bedeutung |
|---|---|---|
| Einzelwert | `-Prio 8` | nur Prio 8 |
| Range | `-Prio 1-4` | Prios 1,2,3,4 |
| Liste | `-Prio 1,3,5` | Prios 1,3,5 |
| `all` | `-Prio all` | alle 8 Prios, 43 Tests |

**`-Test`** waehlt einen einzelnen Test per Name. Wenn `-Test` gesetzt
ist, wird `-Prio` ignoriert. Der Name ist case-insensitive, `-` und `_`
sind austauschbar: `Running-Coupling`, `running_coupling` und
`RUNNING-COUPLING` sind aequivalent.

**`-ExeDir`** ist selten noetig. Es wird verwendet, wenn der Runner aus
einem Export-Paket heraus aufgerufen wird (z.B. `out\exe\`).

**`-DllDir`** ist relevant, wenn EXEs und DLLs in verschiedenen
Verzeichnissen liegen. In dem Fall haengt der Runner `-DllDir` an
`$env:PATH` an, damit der Windows-Loader `ProPhysics.dll` findet.

**`-LogDir`** wird angelegt, falls nicht vorhanden.

---

## 5. Prios im Ueberblick

| Prio | Thema | Tests | Typische Dauer |
|:-:|---|:-:|---:|
| 1 | 2D-Basis | 12 | ~5 s |
| 2 | Emergenz | 10 | ~6,5 min |
| 3 | Langlauf | 10 | ~1 min |
| 4 | 3D-Torus | 3 | ~1 min |
| 5 | Hydrogen + Shared-Ref + Tournament | 4 | ~41 min |
| 6 | Spin-1/2 | 1 | < 1 s |
| 7 | Dirac | 1 | ~15 s |
| 8 | SU(2)-Eichfeld + Running-Coupling | 2 | **~24 min** |
| **all** | **alle** | **43** | **~74 min** |

Die Laufzeit-Dominanz verteilt sich auf zwei Tests:
- **Hydrogen-48** (Prio 5, ~2 280 s = 51,6 %)
- **Running-Coupling** (Prio 8, ~1 400 s = 31,6 %)

Fuer schnelle Regressionen im CI sind die Prios 1–4, 6, 7 und der
`SU2-Wilson-Loop`-Test aus Prio 8 empfohlen (~1,5 min zusammen).

**Achtung:** Prio 8 enthaelt zwei Tests. Fuer CI kann mit
`-Test SU2-Wilson-Loop` gezielt nur der kurze Test gefahren werden
(siehe §16).

---

## 6. Test-Katalog (Prio-Mapping)

Der Katalog ist im Skript als PowerShell-Hashtable `$TestCatalog`
hinterlegt. Pro Eintrag: Name, EXE, Argumente, Timeout.

### Prio 1 — 2D-Basis (12)

| Test | EXE | Args | Timeout |
|---|---|---|---|
| Amp-Smoke | `example_alpha_test.exe` | `--test-amp` | 60 s |
| Born-Regel | `example_alpha_test.exe` | `--test-born` | 60 s |
| Unitary-Tick | `example_alpha_test.exe` | `--test-unitary` | 60 s |
| Context-Perm | `example_alpha_test.exe` | `--test-context` | 60 s |
| Wilson-Loop | `example_alpha_test.exe` | `--test-wilson` | 60 s |
| Local-Gauge | `example_alpha_test.exe` | `--test-gauge` | 60 s |
| Triangle-Corr | `example_alpha_test.exe` | `--test-triangle` | 120 s |
| CHSH-Native | `example_alpha_test.exe` | `--test-chsh` | 120 s |
| CHSH-Collapse | `example_alpha_test.exe` | `--test-chsh-collapse` | 120 s |
| CHSH-Graph | `example_alpha_test.exe` | `--test-chsh-graph` | 120 s |
| Density-Regression | `example_test_density.exe` | *(keine)* | 180 s |
| Tensor-Regression | `example_test_tensor.exe` | *(keine)* | 180 s |

### Prio 2 — Emergenz (10)

| Test | Args | Timeout |
|---|---|---|
| Superdet | `--test-superdet` | 300 s |
| Observer-CHSH | `--test-observer-chsh` | 300 s |
| CHSH-Diffusion | `--test-chsh-diffusion` | 600 s |
| CHSH-Wave | `--test-chsh-wave` | 600 s |
| Born-Emergent | `--test-born-emergent` | 900 s |
| Born-Local | `--test-born-local` | 900 s |
| Born-Equiv | `--test-born-equiv` | 600 s |
| QM-Basics | `--test-qm-basics` | 300 s |
| QM-Advanced | `--test-qm-advanced` | 600 s |
| QM-Emergent | `--test-qm-emergent` | 600 s |

### Prio 3 — Langlauf (10)

| Test | Args | Timeout |
|---|---|---|
| Amp-Invariant | `--test-amp-invariant` | 300 s |
| Amp-Inv-Colored | `--test-amp-invariant-colored` | 300 s |
| Amp-Inv-Bisect | `--test-amp-invariant-bisect` | 600 s |
| Edge-Transport | `--test-edge-transport` | 300 s |
| Edge-Trans-Colored | `--test-edge-transport-colored` | 300 s |
| Edge-Trans-Scaling | `--test-edge-transport-scaling` | 600 s |
| Wave-Packet | `--test-wave-packet` | 300 s |
| Soliton | `--test-soliton` | 1200 s |
| Lorentz | `--test-lorentz` | 600 s |
| No-Signaling | `--test-no-signaling` | 900 s |

### Prio 4 — 3D-Torus (3)

| Test | Args | Timeout |
|---|---|---|
| 3D-Smoke | `--test-3d-smoke` | 300 s |
| 3D-Invariance | `--test-3d-invariance` | 900 s |
| 3D-Dispersion | `--test-3d-dispersion` | 900 s |

### Prio 5 — Hydrogen + Shared-Ref (4)

| Test | Args | Timeout |
|---|---|---|
| Hydrogen | `--test-hydrogen` | 1800 s |
| Hydrogen-48 | `--test-hydrogen-48` | **7200 s** |
| Shared-Reference | `--test-shared-reference` | 600 s |
| Shared-Formula-Tourn | `--test-shared-formula-tournament` | 300 s |

### Prio 6 — Spin-1/2 (1)

| Test | Args | Timeout |
|---|---|---|
| Spin-Half | `--test-spin-half` | 120 s |

### Prio 7 — Dirac (1)

| Test | Args | Timeout |
|---|---|---|
| Dirac | `--test-dirac` | 300 s |

### Prio 8 — SU(2)-Eichfeld + Running-Coupling (2)

| Test | Args | Timeout | Bemerkung |
|---|---|---|---|
| SU2-Wilson-Loop | `--test-su2-wilson-loop` | 300 s | 18 + KA = 19 Einzelchecks |
| Running-Coupling | `--test-running-coupling` | **1800 s** | CI-untauglich |

**Empfehlung:** In normalen CI-Laeufen `Running-Coupling` auslassen,
indem `-Test SU2-Wilson-Loop` gefahren wird (~2 s). `Running-Coupling`
als Nightly-Job (manuell, siehe §16.2).

---

## 7. Test-Auswertung — wie PASS/FAIL erkannt wird

Der Runner wertet die Test-Ausgabe per Regex aus. Verschiedene Tests
nutzen unterschiedliche Marker. Die Erkennung laeuft in dieser
Reihenfolge:

### 7.1 Sammeltests mit Nummer

Muster: `Ergebnis: N PASS, M FAIL`

Verwendet von: Density-Regression, Tensor-Regression.

```
Ergebnis: 86 PASS, 0 FAIL   → PASS
Ergebnis: 85 PASS, 1 FAIL   → FAIL
```

### 7.2 No-Signaling

Muster: `Ergebnis: HELD` oder `Ergebnis: BROKEN`

### 7.3 Einzel-Check-Marker

Muster: zaehlt `[PASS]` und `[FAIL]` in der Ausgabe.

Wenn mindestens ein `[FAIL]` vorhanden ist: FAIL.
Sonst bei mindestens einem `[PASS]`: PASS.

Verwendet von: Density, Tensor.

### 7.4 Fuehrende Ergebniszeile

Muster: `Ergebnis: PASSED` oder `Ergebnis: FAILED`.

Verwendet von: CHSH-Native, Lorentz.

### 7.5 Direkt-Marker

Muster: `-> PASSED` oder `-> FAILED`.

Verwendet von: allen `*_impl`-Funktionen, Hydrogen, Shared-Reference,
SU2-Wilson-Loop, **Running-Coupling**.

### 7.6 Sondermarker ohne PASS/FAIL

| Muster | Bedeutung | Beispiel |
|---|---|---|
| `Klassische Schranke respektiert` | PASS | CHSH-Diffusion/Wave |
| `KEIN Bell-Bruch` | PASS | CHSH-Chaotic |
| `Signalverlust` | PASS | CHSH-Chaotic |
| `[Bisect] Fertig` | PASS | Amp-Inv-Bisect |
| `[Amp-Test] OK` | PASS | Amp-Smoke |
| `# Gesamt : PASS` | PASS | QM-Basics, QM-Advanced |
| `# Gesamt : FAIL` | FAIL | QM-Basics, QM-Advanced |

### 7.7 Superdet-Sonderfall

Erkennt zwei S-Zeilen:

```
getrennte Quelle: S = 2.0073
geteilte Quelle:  S = 4.0000
```

Prueft: `|S_sep − 2.0| < 0.20` UND `|S_shr − 4.0| < 0.20`.

### 7.8 Fallback

Weder Marker noch Ergebniszeile erkannt:

- `ExitCode != 0` → FAIL
- `ExitCode == 0` → UNKNOWN (wird als Fehler gewertet)

---

## 8. Log-Verzeichnis

Standard: `<ExeDir>\logs\`

Format pro Datei: `<timestamp>_<name>.log`

Beispiel:

```
20260928_150634_Amp-Smoke.log
20260928_150634_Born-Regel.log
20260928_150634_Dirac.log
20260928_150634_SU2-Wilson-Loop.log
20260928_150634_Running-Coupling.log
...
```

Jede Log enthaelt:

1. Die komplette stdout des Test-Prozesses (UTF-8)
2. Bei stderr-Ausgabe: einen `--- STDERR ---`-Trenner und den stderr-Inhalt
3. Bei Timeout: einen `--- TIMEOUT nach Ns ---`-Hinweis

**Log-Namen normalisiert:** Sonderzeichen im Test-Namen (Leerzeichen,
Umlaute) werden zu `_`. Der Regex im Skript ist `[^\w\-]` → `_`.

**CI-Upload:** Bei GitHub-Actions-Läufen werden die Logs als Artefakt
hochgeladen. Details siehe die CI-Dokumente (§16, §20).

---

## 9. Timeouts

Jeder Test hat einen individuellen Timeout. Zusammenfassung der
Extremwerte:

| Kategorie | Timeout-Bereich |
|---|---|
| Smoke-Tests | 60 s |
| Einzelkorrelation | 120 s |
| Statistik-Tests (klein) | 180–300 s |
| Statistik-Tests (gross) | 600–900 s |
| Langlauf | 900–1800 s |
| Soliton | 1200 s |
| Hydrogen-48 | **7200 s** |
| Spin-Half | 120 s |
| Dirac | 300 s |
| SU2-Wilson-Loop | 300 s |
| Running-Coupling | **1800 s** |

Bei Timeout: Prozess wird via `Kill()` beendet, Log enthaelt die
bis dahin gesammelte Ausgabe, Ergebnis ist `TIMEOUT`. Timeout wird
als Fehler gewertet (Exit-Code 1).

**Warum 7200 s fuer Hydrogen-48?** Der Test dauert typisch ~2 280 s.
Der Timeout ist ~3,2× groesser, damit auch ein langsamerer Rechner
(~1,5× langsamer) noch durchlaeuft.

**Warum 1800 s fuer Running-Coupling?** Der Test dauert typisch
~1 400 s (dim=64 dominiert). Der Timeout ist ~1,3× groesser. Bei
einem langsamen Rechner kann er an die 1700 s laufen; 1800 s ist
knapp. Wenn er in deiner Umgebung TIMEOUT-t, Timeout im Skript auf
2400 s erhoehen.

---

## 10. Konsolenausgabe

### 10.1 Start

```
============================================================
  ProPhysics Alpha-Test-Runner -- all
============================================================
  ExeDir: <repo>\bin
  DllDir: <repo>\bin
  LogDir: <repo>\bin\logs
  Tests:  43
```

### 10.2 Pro Test eine Zeile

```
[ 1/43] Amp-Smoke              ... PASS         0,5s
[ 2/43] Born-Regel             ... PASS         0,1s
[ 3/43] Unitary-Tick           ... PASS         0,1s
...
[42/43] SU2-Wilson-Loop        ... PASS         1,1s
[43/43] Running-Coupling       ... PASS     1.398,3s
```

Farbcodierung:

| Ergebnis | Farbe |
|---|---|
| `PASS` | Gruen |
| `FAIL` | Rot |
| `TIMEOUT` | Gelb |
| `MISSING` | Magenta |
| `UNKNOWN` | Grau |

### 10.3 Zusammenfassung

```
============================================================
  Zusammenfassung
============================================================

Test                     Ergebnis      Sek. Log
----                     --------      ---- ---
Amp-Smoke                PASS           0,5 20260928_150634_Amp-Smoke.log
Born-Regel               PASS           0,1 20260928_150634_Born-Regel.log
...
SU2-Wilson-Loop          PASS           1,1 20260928_150634_SU2-Wilson-Loop.log
Running-Coupling         PASS       1.398,3 20260928_150634_Running-Coupling.log

PASS: 43   FAIL: 0   TIMEOUT: 0   MISSING: 0   UNKNOWN: 0   | Gesamt: 43
Dauer gesamt: 4420,6s

Alle Tests erfolgreich.
```

### 10.4 Fehlerfall

Wenn ein Test fehlschlaegt:

```
PASS: 42   FAIL: 1   TIMEOUT: 0   MISSING: 0   UNKNOWN: 0   | Gesamt: 43
Dauer gesamt: 4421,0s

Nicht-bestandene Tests:
  - Running-Coupling       FAIL     <repo>\bin\logs\20260928_150634_Running-Coupling.log
```

Die Zeile wird in Rot ausgegeben, Exit-Code ist 1.

---

## 11. Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | alle Tests PASS |
| `1` | mindestens ein FAIL, TIMEOUT, MISSING oder UNKNOWN |
| `2` | Aufbaufehler: `-Prio` ungueltig, `-Test` nicht gefunden, `ExeDir`/`DllDir` fehlt |

Exit-Code `1` reicht nicht zwischen FAIL und TIMEOUT — die
Unterscheidung steht in der Konsolenausgabe und im Log-Verzeichnis.

---

## 12. Was im Log landet — Beispiele

### 12.1 SU2-Wilson-Loop (T1–T18 + KA)

`20260928_104315_SU2-Wilson-Loop.log` (gekuerzt):

```
========================================================================
  Etappe 22: SU(2)-Eichfeld
========================================================================

[SU2] T1: Quaternion-Unit
[SU2]   max |norm^2/2^60 - 1| = 6.9297e-10 (Schwelle 1e-9)
[SU2]   T1 PASS
...
[SU2] T15: Dynamik erhaelt Link-Norm
[SU2]   max |norm^2/2^60 - 1| = 1.8030e-08 (Schwelle 1e-5)
[SU2]   T15 PASS
[SU2] T16: Dynamik — Energieerhaltung (100 Ticks)
[SU2]   E(0) = 4.006588065
[SU2]   E(100) = 4.000930941
[SU2]   rel. Drift = 1.4120e-03 (Schwelle 1e-2)
[SU2]   T16 PASS
[SU2] T17: R7 — Dynamics aus, Link unveraendert
[SU2]   Link unveraendert: ja
[SU2]   T17 PASS
[SU2] T18: Dynamics aendert Links bei aktivem Flag
[SU2]   Link geaendert nach Tick: ja
[SU2]   T18 PASS

[SU2] Ergebnis: 18 / 18
[SU2] Kernel-Algebra-Check max_err = 4.4488e-10 (Schwelle 1e-9)
[SU2] Kernel-Algebra PASS
[SU2] -> PASSED (SU(2)-Eichfeld implementiert)
```

**Wichtig:** T16 = 1,41e-03 (nach Forward-Plaquette-Konjugations-
Fix in Patch 1.23.10). Vorher 2,44e-03 (nach Backward-Staple-Fix),
vorher 8,06e-03 (Original).

### 12.2 Running-Coupling (Etappe 23)

`20260928_150634_Running-Coupling.log` (gekuerzt):

```
========================================================================
  Etappe 23 (II): Running Coupling / Beta-Funktion
========================================================================

[RC] Metropolis-Sampling auf SU(2)-Links.
[RC] Observable: u_plaq = <S_plaq> / (2 * N_plaq).
[RC] Sweep: dim in {16, 32, 64} x beta in {0.5, 1, 2, 4}.
[RC] Thermalisierung: 200 Sweeps, Messung: 300 Sweeps, Bins: 30.

[RC] --------------------------------------------------------------
[RC] dim = 64  (N = 262144 Knoten)
[RC] --------------------------------------------------------------
[RC]   beta     eps     accept    u_plaq       u_err
[RC]   -------------------------------------------------------
[RC]   0.50    1.519   0.741     0.438134   0.000025
[RC]   1.00    0.675   0.666     0.379875   0.000021
[RC]   2.00    0.450   0.558     0.283270   0.000027
[RC]   4.00    0.300   0.478     0.170344   0.000019

...

[RC] -> PASSED (Rohdaten, weitere Analyse extern)
```

Der Runner erkennt entweder `[RC] -> PASSED` oder
`-> PASSED (Rohdaten, ...)`.

---

## 13. Wie ein einzelner Test manuell laeuft

Ohne den Runner, fuer Debug-Zwecke:

```cmd
cd bin
.\example_alpha_test.exe --test-dirac
.\example_alpha_test.exe --test-su2-wilson-loop
.\example_alpha_test.exe --test-running-coupling
```

Die Ausgabe ist direkt sichtbar. Der Runner macht dasselbe, leitet
aber stdout/stderr in eine Datei um und wertet den Marker aus.

**Hinweis:** `--test-running-coupling` dauert ~23 min. Nicht
versehentlich im Debug-Modus starten.

Alternativ per Runner:

```cmd
cd tools
run_alpha_tests.cmd -Test Dirac
run_alpha_tests.cmd -Test Running-Coupling
```

Oder ueber `pro_run`:

```cmd
cd tools
pro_run test -Test Dirac
pro_run test -Test Running-Coupling -LogDir C:\logs
```

---

## 14. Aufruf-Matrix

| CWD | Aufruf | `ExeDir` | `DllDir` | `LogDir` |
|---|---|---|---|---|
| `tools\` | `pro_run test -Prio 8` | `<repo>\bin` | `<repo>\bin` | `<repo>\bin\logs` |
| `tools\` | `run_alpha_tests.cmd -Prio 8` | `<repo>\bin` | `<repo>\bin` | `<repo>\bin\logs` |
| `tools\` | `run_alpha_tests.cmd -Prio all -LogDir H:\temp\logs` | `<repo>\bin` | `<repo>\bin` | `H:\temp\logs` |
| `tools\` | `run_alpha_tests.cmd -Test Dirac` | `<repo>\bin` | `<repo>\bin` | `<repo>\bin\logs` |
| Repo-Root | `tools\run_alpha_tests.cmd -Prio 8` | `<repo>\bin` | `<repo>\bin` | `<repo>\bin\logs` |
| `C:\Temp` | `H:\...\tools\run_alpha_tests.cmd -Prio all` | `<repo>\bin` | `<repo>\bin` | `<repo>\bin\logs` |
| Export | `out\exe\run_alpha_tests.cmd -Prio all` | `out\exe` | `out\exe` | `out\exe\logs` |

Default `ExeDir = $PSScriptRoot\..\bin` sorgt dafuer, dass der Aufruf
**immer** auf die richtige EXE-Lage zeigt, unabhaengig vom CWD.

---

## 15. Integration mit `build.ps1` / `pro_run`

Der Build-Wrapper ruft den Runner **nicht** auf. Das ist Absicht:

- **Build ≠ Test.** Der Build produziert die EXEs, der Runner fuehrt
  sie aus.
- **Der Nutzer entscheidet.** Nach einem Build kann er `-Prio 6`
  laufen lassen, ohne die vollen 74 Minuten zu investieren.

**Empfohlen:** ueber `pro_run all`, das build → test → export in
einem Aufruf macht:

```cmd
pro_run all
pro_run all -Prio 1-4
pro_run all -NoBuild -Prio 8
```

Oder klassisch:

```cmd
cd build\main
build.cmd -Mode all -Rebuild
cd ..\..\tools
run_alpha_tests.cmd -Prio 8
```

---

## 16. Integration mit CI / GitHub Actions

ProPhysics hat **drei** GitHub-Actions-Workflows. Sie sind
unabhängig, haben unterschiedliche Trigger und Laufzeiten. Der
Test-Runner wird von **zwei** von ihnen genutzt.

### 16.1 Standard-CI — `ci.yml`

**Trigger:** Push auf `main` und Pull Requests gegen `main`.
**Laufzeit:** ~1,5 min.

**Was läuft:**

| Prio | Test | Dauer |
|:-:|---|---:|
| 1 | 2D-Basis (12 Tests) | ~5 s |
| 6 | Spin-1/2 (1 Test) | < 1 s |
| 7 | Dirac (1 Test) | ~15 s |
| 8 | **nur** `SU2-Wilson-Loop` (nicht Running-Coupling) | ~2 s |

**Warum genau diese vier?**

- Prio 1 deckt die Basis-Physik ab (Born, Unitariät, Gauge U(1), CHSH).
- Prio 6 und 7 sind schnell und decken zwei Schicht-2-Erweiterungen ab.
- `SU2-Wilson-Loop` ist der einzige schnelle Test aus Prio 8.

**Aufruf aus dem Workflow:**

```cmd
cd tools
run_alpha_tests.cmd -Prio 1
run_alpha_tests.cmd -Prio 6
run_alpha_tests.cmd -Prio 7
run_alpha_tests.cmd -Test SU2-Wilson-Loop
```

Nach jedem Aufruf prüft der Workflow explizit den Exit-Code
(`if %ERRORLEVEL% neq 0 exit /b`) — sonst wäre die CI wertlos.

**Artefakt:** `bin\logs\` wird als `test-logs` hochgeladen
(Retention 7 Tage). Auch bei Fehlschlag (`if: always()`).

**Detaillierte Doku:** `docs\build\ci.md`.

### 16.2 Alpha-Nightly — `alpha-nightly.yml`

**Trigger:** Ausschließlich manuell (`workflow_dispatch`).
**Laufzeit:** ~64 min (Scope `all-long`).

**Was läuft (Scope-abhängig):**

| Scope | Was läuft | Dauer |
|---|---|---:|
| `all-long` (Default) | Prio 5 + `Running-Coupling` | ~64 min |
| `prio-5` | Nur Prio 5 (Hydrogen, Hydrogen-48, Shared-Reference, Tournament) | ~41 min |
| `running-coupling` | Nur der Metropolis-Test | ~23 min |

**Warum manuell und nicht per Cron?**

- GitHub deaktiviert `schedule`-Workflows nach 60 Tagen Inaktivität
  im Repo — still und ohne Warnung.
- Bei 1–2 Läufen pro Woche ist eine automatische Ausführung
  unnötig.
- Manuell = der Nutzer entscheidet, wann der ~64-min-Lauf läuft.

**Aufruf aus dem Workflow:**

```cmd
cd tools
run_alpha_tests.cmd -Prio 5              :: Scope prio-5
run_alpha_tests.cmd -Test Running-Coupling   :: Scope running-coupling
```

**Artefakt:** `bin\logs\` wird als
`alpha-nightly-logs-<scope>-<run_id>` hochgeladen (Retention 30 Tage).
Auch bei Fehlschlag.

**Detaillierte Doku:** `docs\build\alpha-nightly.md`.

### 16.3 Web-Docs-CI — `web-docs.yml`

**Trigger:** Push auf `main` mit Änderungen an `docs/**`,
`src/prowb/**` oder `build/prowb/**`. Manuell via Actions-Tab.
**Laufzeit:** ~1 min.

**Was läuft:** ProWB-Build und Web-Docs-Generierung, dann Deploy
auf GitHub Pages. **Kein Test-Runner.**

**Detaillierte Doku:** `docs\build\web-docs-ci.md`.

### 16.4 Was jeder Testlauf sicherstellt

| Prio | Was geprüft wird | Wo läuft's |
|---|---|---|
| 1 | Grundmechanik (Born, Unitariät, Gauge U(1), CHSH) | Standard-CI |
| 2 | Emergenz (Born, Lorentz, Interferenz, CHSH-Varianten) | — (lokal) |
| 3 | Langlauf (Invarianten, Transport, Soliton) | — (lokal) |
| 4 | 3D-Torus (Smoke, Invariance, Bloch) | — (lokal) |
| 5 | Wasserstoff + U4 Shared-Reference | Alpha-Nightly (manuell) |
| 6 | Spin-1/2 aus SU(2) | Standard-CI |
| 7 | Dirac-Struktur | Standard-CI |
| 8 | SU(2)-Eichfeld (kinematisch + dynamisch) | Standard-CI (`SU2-Wilson-Loop`) |
| 8 | SU(2)-Metropolis (Running-Coupling) | Alpha-Nightly (manuell) |

**Prio 2, 3, 4 laufen aktuell nicht automatisch.** Sie sind für
lokale Regressionen vorgesehen. Bei Bedarf können sie in die
Standard-CI aufgenommen werden (siehe `docs\build\ci.md` §7 für
Diskussion).

### 16.5 Lokale Reproduktion der CI

Der Standard-CI-Workflow ist 1:1 lokal reproduzierbar:

```cmd
cd build\main
build.cmd -Mode all -Rebuild

cd ..\..\tools
run_alpha_tests.cmd -Prio 1
run_alpha_tests.cmd -Prio 6
run_alpha_tests.cmd -Prio 7
run_alpha_tests.cmd -Test SU2-Wilson-Loop
```

Das ist dieselbe Sequenz wie in §7.4 (`CONTRIBUTING.md`). Vor jedem
Push auf `main` empfohlen.

Für den Nightly-Workflow:

```cmd
cd build\main
build.cmd -Mode all -Rebuild

cd ..\..\tools
run_alpha_tests.cmd -Prio 5
run_alpha_tests.cmd -Test Running-Coupling
```

### 16.6 Vollständiger Prio-All-Lauf vor einem Release

Vor einem Release oder einem Push auf `main` (Regel R6) ist der
komplette Lauf Pflicht:

```cmd
cd tools
pro_run test -Prio all
```

~74 min. Kein CI-Workflow führt diesen Lauf automatisch aus. Er
muss lokal oder über einen manuell gestarteten Nightly-Lauf
erledigt werden.

**Regel:** Kein Push auf `main` ohne 43/43 PASS (siehe
`CONTRIBUTING.md` §7.4).

---

## 17. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `FEHLER: ExeDir nicht gefunden` | `-ExeDir` zeigt auf falschen Pfad | Pfad pruefen oder ohne `-ExeDir` aufrufen |
| `FEHLER: DllDir nicht gefunden` | `-DllDir` falsch | Pfad pruefen |
| `ProPhysics.dll fehlt in <DllDir>` | Build nicht gelaufen | `pro_run build` |
| `Test nicht im Katalog: <name>` | `-Test` Tippfehler | Liste der verfuegbaren Tests wird ausgegeben |
| `Ungueltige Prio-Angabe` | `-Prio` Syntax falsch | `1`, `1-4`, `1,3,5` oder `all` |
| alle Tests `MISSING` | `ExeDir` enthaelt keine EXEs | erst `pro_run build` laufen |
| `Dirac FAIL` | Kernel nicht neu gebaut | `pro_run build -Mode kernel` |
| `SU2-Wilson-Loop FAIL` | Kernel nicht neu gebaut oder `ProPhysics_SU2_Dynamics.c` fehlt in `OBJ_KERNEL` | `build\prophysics\Makefile.nmake` pruefen, dann `-Rebuild` |
| `Running-Coupling FAIL` | `SU2_Link_Plaquette_Sum` fehlt | `pro_run build -Mode kernel -Rebuild` |
| `Running-Coupling TIMEOUT` | Rechner sehr langsam | Timeout auf 2400 s erhoehen (Prio 8) |
| `Hydrogen-48 TIMEOUT` | Rechner sehr langsam | Timeout im Skript erhoehen (Prio 5) |
| `UNKNOWN`-Ergebnis | unbekannter Test-Marker | Ausgabe im Log pruefen, Regex erweitern |
| Logs leer | EXE stuerzte beim Start | DLL fehlt in `bin\` |
| Umlaute in Logs kaputt | EXE ohne UTF-8 | Konsolen-Encoding pruefen |
| `Workflow läuft nicht` (CI) | Trigger-Bedingung prüft nur `main` | Push auf `main` |
| `Standard-CI grün, lokal FAIL` (oder umgekehrt) | MSVC-Version unterschiedlich | `windows-latest`-MSVC-Version im Log prüfen |
| `Artefakt-Upload fehlgeschlagen` (CI) | `bin\logs\` leer | Test-Lauf ist gar nicht gestartet |
| `Alpha-Nightly taucht nicht auf` | Workflow-Datei nicht committed | `.github\workflows\alpha-nightly.yml` prüfen |

**Diagnose in CI:** Actions-Tab → Workflow → Job → Steps. Für
PASS/FAIL-Details die hochgeladenen Logs herunterladen (Artifacts
unter „Summary").

---

## 18. Was das Skript nicht tut

- **Kein Build.** Setzt voraus, dass die EXEs in `bin\` liegen.
- **Keine parallele Ausfuehrung.** Alle Tests laufen sequenziell.
- **Kein Retry.** Ein fehlgeschlagener Test wird nicht wiederholt.
- **Keine Trend-Analyse.** Kein Vergleich mit vorherigen Laeufen.
- **Kein Parsen von Zahlen.** Nur PASS/FAIL-Marker werden erkannt.
- **Kein Push in Datenbank.** Logs bleiben im Dateisystem.
- **Kein Auto-Nightly.** Der Nightly-Workflow wird manuell
  gestartet (`docs\build\alpha-nightly.md`).
- **Kein Deploy.** Der Web-Docs-Workflow
  (`docs\build\web-docs-ci.md`) deployt separat.

---

## 19. Parameter-Referenz (kompakt)

```
run_alpha_tests.cmd [-Prio <all|N|N-M|N,M,K>]
                    [-Test <name>]
                    [-ExeDir <pfad>]
                    [-DllDir <pfad>]
                    [-LogDir <pfad>]
```

| Parameter | Typ | Default | Pflicht |
|---|---|---|---|
| `-Prio` | String | `all` | nein |
| `-Test` | String | (leer) | nein |
| `-ExeDir` | Pfad | `$PSScriptRoot\..\bin` | nein |
| `-DllDir` | Pfad | identisch mit `-ExeDir` | nein |
| `-LogDir` | Pfad | `<ExeDir>\logs` | nein |

**Über `pro_run`:** `pro_run test` akzeptiert dieselben Parameter
und dispatcht auf dieses Skript. Details siehe
`docs\build\pro_run.md`.

---

## 20. Siehe auch

| Thema | Datei |
|---|---|
| Zentraler Einstiegspunkt | `docs\build\pro_run.md` |
| Test-Uebersicht mit Kriterien | `docs\test\ProPhysics_Testkatalog.md` |
| Test-Baseline (Kurzfassung) | `docs\test\BASELINE.md` |
| Tests schreiben | `docs\test\WRITING_TESTS.md` |
| **Standard-CI (`ci.yml`)** | **`docs\build\ci.md`** |
| **Alpha-Nightly (`alpha-nightly.yml`)** | **`docs\build\alpha-nightly.md`** |
| **Web-Docs-CI (`web-docs.yml`)** | **`docs\build\web-docs-ci.md`** |
| Build-Wrapper | `docs\build\helper\build.md` |
| Export-Wrapper | `docs\build\helper\export.md` |
| Build-System-Übersicht | `docs\build\BUILD_SCRIPT.md` |
| CLI-Parser der EXE | `src\test\alpha_test_main.c` |
| Running-Coupling-Test | `src\test\alpha_test_running_coupling.c` |

---

**Ende Test-Runner-Dokumentation (v1.1.0).**