# ProPhysics Alpha-Test-Runner

**Dateien:** `tools\run_alpha_tests.ps1` + `tools\run_alpha_tests.cmd`

**Version:** 3.2 (Etappe 23 + V&V-Anker)

**Zweck:** Führt die Alpha-Test-Suite sequenziell aus, schreibt pro Test
ein Log und liefert eine PASS/FAIL-Bilanz mit Exit-Code.

---

## 1. Was das Skript tut

Für eine gewählte Prioritätsstufe (Prio 1–8 oder `all`):

1. **Test für Test** starten (als separater Prozess)
2. **Ausgabe sammeln** (stdout + stderr, UTF-8)
3. **Ergebnis auswerten** per Regex auf PASS/FAIL-Marker
4. **Log schreiben** pro Test nach `<LogDir>\<timestamp>_<name>.log`
5. **Zusammenfassung** als Tabelle ausgeben
6. **Exit-Code** zurückgeben: `0` bei allen PASS, `1` sonst

Der Runner führt Tests **nicht** parallel aus. Sequenziell ist Absicht:
Test-Ausgaben sind nicht thread-safe, und einzelne Tests brauchen
>30 min (Hydrogen-48), was parallele Läufe unübersichtlich machen
würde.

---

## 2. Ablageort

Seit **Refactoring 22** liegt der Runner in `tools\`, nicht mehr in
`bin\`. Die EXEs bleiben in `bin\`; das Skript ist self-locating und
zeigt automatisch auf das Schwester-Verzeichnis `..\bin`.

```
H:\ProPhysics_SDK\ProPhysics\
├── bin\
│   ├── example_alpha_test.exe
│   ├── example_test_density.exe
│   ├── example_test_tensor.exe
│   ├── ProPhysics.dll
│   ├── pro_sdk_interface.dll
│   └── logs\                       <- Ziel der Logs (Default)
├── tools\
│   ├── run_alpha_tests.cmd         <- Wrapper für cmd.exe
│   └── run_alpha_tests.ps1         <- dieses Skript
└── docs\
    └── test\
        └── run_alpha_tests.md      <- diese Datei
```

**Self-Locating:** Das Skript findet seinen Ablageort über
`$PSScriptRoot`. Der Default `-ExeDir` ist `$PSScriptRoot\..\bin`.
Damit funktioniert der Aufruf aus jedem CWD.

---

## 3. Aufruf

### 3.1 Über `run_alpha_tests.cmd` (empfohlen für cmd.exe)

```cmd
cd tools
run_alpha_tests.cmd -Prio 8
run_alpha_tests.cmd -Prio all
```

### 3.2 Direkt über PowerShell

```powershell
cd tools
.\run_alpha_tests.ps1 -Prio 8
.\run_alpha_tests.ps1 -Prio all
```

### 3.3 Aus jedem anderen CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\tools\run_alpha_tests.cmd -Prio all
```

### 3.4 Interaktiv (ohne Argumente)

```powershell
.\run_alpha_tests.ps1
```

PowerShell fragt nach dem `-Prio`-Wert. Nützlich beim Debuggen.

---

## 4. Parameter

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Prio` | Choice: `1`,`2`,`3`,`4`,`5`,`6`,`7`,`8`,`all` | *(mandatory)* | Prioritätsstufe |
| `-ExeDir` | Pfad | `$PSScriptRoot\..\bin` | Wo die EXEs liegen |
| `-LogDir` | Pfad | `<ExeDir>\logs` | Ziel der Logs |

**`-ExeDir`** ist selten nötig. Es wird nur verwendet, wenn du den
Runner aus einem anderen Verzeichnis heraus auf eine Kopie der EXEs
richten willst (z.B. Export-Paket in `out\exe\`).

**`-LogDir`** wird angelegt, falls nicht vorhanden.

---

## 5. Prios im Überblick

| Prio | Thema | Tests | Typische Dauer |
|:-:|---|:-:|---:|
| 1 | 2D-Basis | 12 | ~5 s |
| 2 | Emergenz | 10 | ~6,5 min |
| 3 | Langlauf | 10 | ~1 min |
| 4 | 3D-Torus | 3 | ~1 min |
| 5 | Hydrogen + Shared-Ref + Tournament | 4 | ~41 min |
| 6 | Spin-1/2 | 1 | < 1 s |
| 7 | Dirac | 1 | ~15 s |
| 8 | SU(2)-Eichfeld + Link-Dynamik + Running-Coupling | 2 | **~24 min** |
| **all** | **alle** | **43** | **~74 min** |

Die Laufzeit-Dominanz verteilt sich auf zwei Tests:
- **Hydrogen-48** (Prio 5, ~2 280 s = 51,6 %)
- **Running-Coupling** (Prio 8, ~1 400 s = 31,6 %)

Für schnelle Regressionen im CI sind die Prios 1–4, 6, 7 und der
`SU2-Wilson-Loop`-Test aus Prio 8 empfohlen (~1,5 min zusammen).

**Achtung:** Prio 8 enthält seit Etappe 23 **zwei** Tests. Für CI
sollte `Running-Coupling` explizit ausgenommen werden (siehe §16).

---

## 6. Test-Katalog (Prio-Mapping)

Der Katalog ist im Skript als PowerShell-Hashtable `$TestCatalog`
hinterlegt. Pro Eintrag: Name, EXE, Argumente, Timeout.

### Prio 1 — 2D-Basis (12)

| Test | EXE | Args |
|---|---|---|
| Amp-Smoke | `example_alpha_test.exe` | `--test-amp` |
| Born-Regel | `example_alpha_test.exe` | `--test-born` |
| Unitary-Tick | `example_alpha_test.exe` | `--test-unitary` |
| Context-Perm | `example_alpha_test.exe` | `--test-context` |
| Wilson-Loop | `example_alpha_test.exe` | `--test-wilson` |
| Local-Gauge | `example_alpha_test.exe` | `--test-gauge` |
| Triangle-Corr | `example_alpha_test.exe` | `--test-triangle` |
| CHSH-Native | `example_alpha_test.exe` | `--test-chsh` |
| CHSH-Collapse | `example_alpha_test.exe` | `--test-chsh-collapse` |
| CHSH-Graph | `example_alpha_test.exe` | `--test-chsh-graph` |
| Density-Regression | `example_test_density.exe` | *(keine)* |
| Tensor-Regression | `example_test_tensor.exe` | *(keine)* |

### Prio 2 — Emergenz (10)

| Test | Args |
|---|---|
| Superdet | `--test-superdet` |
| Observer-CHSH | `--test-observer-chsh` |
| CHSH-Diffusion | `--test-chsh-diffusion` |
| CHSH-Wave | `--test-chsh-wave` |
| Born-Emergent | `--test-born-emergent` |
| Born-Local | `--test-born-local` |
| Born-Equiv | `--test-born-equiv` |
| QM-Basics | `--test-qm-basics` |
| QM-Advanced | `--test-qm-advanced` |
| QM-Emergent | `--test-qm-emergent` |

### Prio 3 — Langlauf (10)

| Test | Args |
|---|---|
| Amp-Invariant | `--test-amp-invariant` |
| Amp-Inv-Colored | `--test-amp-invariant-colored` |
| Amp-Inv-Bisect | `--test-amp-invariant-bisect` |
| Edge-Transport | `--test-edge-transport` |
| Edge-Trans-Colored | `--test-edge-transport-colored` |
| Edge-Trans-Scaling | `--test-edge-transport-scaling` |
| Wave-Packet | `--test-wave-packet` |
| Soliton | `--test-soliton` |
| Lorentz | `--test-lorentz` |
| No-Signaling | `--test-no-signaling` |

### Prio 4 — 3D-Torus (3)

| Test | Args |
|---|---|
| 3D-Smoke | `--test-3d-smoke` |
| 3D-Invariance | `--test-3d-invariance` |
| 3D-Dispersion | `--test-3d-dispersion` |

### Prio 5 — Hydrogen + Shared-Ref (4)

| Test | Args |
|---|---|
| Hydrogen | `--test-hydrogen` |
| Hydrogen-48 | `--test-hydrogen-48` |
| Shared-Reference | `--test-shared-reference` |
| Shared-Formula-Tourn | `--test-shared-formula-tournament` |

### Prio 6 — Spin-1/2 (1)

| Test | Args |
|---|---|
| Spin-Half | `--test-spin-half` |

### Prio 7 — Dirac (1)

| Test | Args |
|---|---|
| Dirac | `--test-dirac` |

### Prio 8 — SU(2)-Eichfeld + Link-Dynamik + Running-Coupling (2)

| Test | Args | Timeout | Bemerkung |
|---|---|---|---|
| SU2-Wilson-Loop | `--test-su2-wilson-loop` | 300 s | 18 + KA = 19 Einzelchecks |
| Running-Coupling | `--test-running-coupling` | **2400 s** | 16 Werte + V&V-Anker; CI-untauglich |

**Empfehlung:** In normalen CI-Läufen `Running-Coupling` auslassen.
Nur `SU2-Wilson-Loop` fahren (~2 s). `Running-Coupling` als
Nightly-Job.

---

## 7. Test-Auswertung — wie PASS/FAIL erkannt wird

Der Runner wertet die Test-Ausgabe per Regex aus. Verschiedene Tests
nutzen unterschiedliche Marker. Die Erkennung läuft in dieser
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

Muster: zählt `[PASS]` und `[FAIL]` in der Ausgabe.

Wenn mindestens ein `[FAIL]` vorhanden ist: FAIL.
Sonst bei mindestens einem `[PASS]`: PASS.

Verwendet von: Density, Tensor.

### 7.4 Führende Ergebniszeile

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
| `[SR] -> PASSED` | PASS | Shared-Reference |
| `[SR] -> FAILED` | FAIL | Shared-Reference |
| `[Amp-Test] OK` | PASS | Amp-Smoke |
| `# Gesamt : PASS` | PASS | QM-Basics, QM-Advanced |
| `# Gesamt : FAIL` | FAIL | QM-Basics, QM-Advanced |
| **`[RC] -> PASSED`** | **PASS** | **Running-Coupling** |
| **`[RC] -> FAILED`** | **FAIL** | **Running-Coupling** |

### 7.7 Superdet-Sonderfall

Erkennt zwei S-Zeilen:

```
getrennte Quelle: S = 2.0073
geteilte Quelle:  S = 4.0000
```

Prüft: `|S_sep − 2.0| < 0.20` UND `|S_shr − 4.0| < 0.20`.

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
20260925_150634_Amp-Smoke.log
20260925_150634_Born-Regel.log
20260925_150634_Dirac.log
20260925_150634_SU2-Wilson-Loop.log
20260925_150634_Running-Coupling.log
...
```

Jede Log enthält:

1. Die komplette stdout des Test-Prozesses (UTF-8)
2. Bei stderr-Ausgabe: einen `--- STDERR ---`-Trenner und den stderr-Inhalt
3. Bei Timeout: einen `--- TIMEOUT nach Ns ---`-Hinweis

**Log-Namen normalisiert:** Sonderzeichen im Test-Namen (Leerzeichen,
Umlaute) werden zu `_`. Der Regex im Skript ist `[^\w\-]` → `_`.

---

## 9. Timeouts

Jeder Test hat einen individuellen Timeout:

| Kategorie | Timeout |
|---|---|
| Smoke-Tests | 60 s |
| Einzelkorrelation | 120 s |
| Statistik-Tests (klein) | 180–300 s |
| Statistik-Tests (groß) | 600–900 s |
| Langlauf | 900–1800 s |
| Soliton | 1200 s |
| **Hydrogen-48** | **7200 s** |
| Spin-Half | 120 s |
| Dirac | 300 s |
| SU2-Wilson-Loop | 300 s |
| **Running-Coupling** | **2400 s** |

Bei Timeout: Prozess wird via `Kill()` beendet, Log enthält die
bis dahin gesammelte Ausgabe, Ergebnis ist `TIMEOUT`. Timeout wird
als Fehler gewertet (Exit-Code 1).

**Warum 7200 s für Hydrogen-48?** Der Test dauert typisch ~2 280 s.
Der Timeout ist ~3,2× größer, damit auch ein langsamerer Rechner
(~1,5× langsamer) noch durchläuft.

**Warum 2400 s für Running-Coupling?** Der Test dauert typisch
~1 400 s (dim=64 dominiert). Der Timeout ist ~1,7× größer. Bei
einem langsamen Rechner kann er auf 2 000 s oder mehr laufen;
2400 s ist Reserve. Wenn du in Etappe 23b die Creutz-Ratio
hinzufügst, wird die Laufzeit weiter steigen — dann Timeout auf
3600 s erhöhen.

---

## 10. Konsolenausgabe

### 10.1 Start

```
============================================================
  ProPhysics Alpha-Test-Runner -- Prio all
============================================================
  ExeDir: H:\ProPhysics_SDK\Test\bin
  LogDir: H:\ProPhysics_SDK\Test\bin\logs
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
| `PASS` | Grün |
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
Amp-Smoke                PASS           0,5 20260925_150634_Amp-Smoke.log
Born-Regel               PASS           0,1 20260925_150634_Born-Regel.log
...
SU2-Wilson-Loop          PASS           1,1 20260925_150634_SU2-Wilson-Loop.log
Running-Coupling         PASS       1.398,3 20260925_150634_Running-Coupling.log

PASS: 43   FAIL: 0   TIMEOUT: 0   MISSING: 0   UNKNOWN: 0   | Gesamt: 43
Dauer gesamt: 4420,6s

Alle Tests erfolgreich.
```

### 10.4 Fehlerfall

Wenn ein Test fehlschlägt:

```
PASS: 42   FAIL: 1   TIMEOUT: 0   MISSING: 0   UNKNOWN: 0   | Gesamt: 43
Dauer gesamt: 4421,0s

Nicht-bestandene Tests:
  - Running-Coupling       FAIL     H:\...\logs\20260925_150634_Running-Coupling.log
```

Die Zeile wird in Rot ausgegeben, Exit-Code ist 1.

---

## 11. Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | alle Tests PASS |
| `1` | mindestens ein FAIL, TIMEOUT, MISSING oder UNKNOWN |

Exit-Code `1` reicht nicht zwischen FAIL und TIMEOUT — die
Unterscheidung steht in der Konsolenausgabe und im Log-Verzeichnis.

---

## 12. Was im Log landet — Beispiele

### 12.1 SU2-Wilson-Loop (T1–T18 + KA)

`20260925_150634_SU2-Wilson-Loop.log` (gekürzt):

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
[SU2]   E(0) = 4.130587644
[SU2]   E(100) = 4.140653074
[SU2]   rel. Drift = 2.4368e-03 (Schwelle 1e-2)
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

**Wichtig:** T16 = 2,44e-03 (nach Backward-Staple-Fix). Vorher
8,06e-03.

### 12.2 Running-Coupling (Etappe 23)

`20260925_150634_Running-Coupling.log` (gekürzt):

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

## 13. Wie ein einzelner Test manuell läuft

Ohne den Runner, für Debug-Zwecke:

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

---

## 14. Aufruf-Matrix

| CWD | Aufruf | `ExeDir` | `LogDir` |
|---|---|---|---|
| `tools\` | `run_alpha_tests.cmd -Prio 8` | `<repo>\bin` | `<repo>\bin\logs` |
| `tools\` | `run_alpha_tests.cmd -Prio all -LogDir H:\temp\logs` | `<repo>\bin` | `H:\temp\logs` |
| Repo-Root | `tools\run_alpha_tests.cmd -Prio 8` | `<repo>\bin` | `<repo>\bin\logs` |
| `C:\Temp` | `H:\...\tools\run_alpha_tests.cmd -Prio all` | `<repo>\bin` | `<repo>\bin\logs` |
| `tools\` | `run_alpha_tests.cmd -Prio 8 -ExeDir ..\bin -LogDir ..\bin\logs` | `<repo>\bin` | `<repo>\bin\logs` |

Default `ExeDir = $PSScriptRoot\..\bin` sorgt dafür, dass der Aufruf
**immer** auf die richtige EXE-Lage zeigt, unabhängig vom CWD.

---

## 15. Integration mit `build.ps1`

Der Build-Wrapper ruft den Runner **nicht** auf. Das ist Absicht:

- **Build ≠ Test.** Der Build produziert die EXEs, der Runner führt
  sie aus.
- **Der Nutzer entscheidet.** Nach einem Build kann er `-Prio 6`
  laufen lassen, ohne die vollen 74 Minuten zu investieren.

Nach einem Build:

```cmd
cd build\main
build.cmd -Mode all -Rebuild
cd ..\..\tools
run_alpha_tests.cmd -Prio 8
```

---

## 16. Integration mit CI / GitHub Actions

Seit Etappe 23 hat Prio 8 einen **langen** Test (Running-Coupling).
Der läuft ~23 min und ist damit **nicht CI-tauglich**. Zwei
Empfehlungen:

### 16.1 Standard-CI (Prios 1–4, 6, 7 + SU2-Wilson-Loop)

Aktuell läuft `Prio 8` immer beide Tests. Für CI brauchst du einen
Modus, der nur `SU2-Wilson-Loop` fährt. Zwei Möglichkeiten:

**Variante A (empfohlen): Der Runner bekommt einen optionalen
Parameter `-Exclude <Testname>`.**

```powershell
# In run_alpha_tests.ps1:
param(
    [Parameter(Mandatory=$false)]
    [string[]]$Exclude = @()
)

# In der Hauptschleife:
foreach ($name in $selected) {
    if ($Exclude -contains $name) { continue }
    ...
}
```

Dann in CI:

```yaml
- name: Alpha-Suite (Prios 1-4, 6, 7, SU2)
  shell: pwsh
  working-directory: tools
  run: |
    $prios = @('1','2','3','4','6','7','8')
    $failed = @()
    foreach ($p in $prios) {
        & cmd /c "run_alpha_tests.cmd -Prio $p -ExeDir ..\bin -LogDir ..\bin\logs -Exclude Running-Coupling"
        if ($LASTEXITCODE -ne 0) { $failed += $p }
    }
    if ($failed.Count -gt 0) {
        Write-Host "Failed: $($failed -join ', ')" -ForegroundColor Red
        exit 1
    }
```

**Variante B (einfacher, ohne Skriptänderung): Prio 8 in CI
überspringen, nur Prio 1–4, 6, 7.**

```yaml
- name: Alpha-Suite (Prios 1-4, 6, 7)
  shell: pwsh
  working-directory: tools
  run: |
    $prios = @('1','2','3','4','6','7')
    $failed = @()
    foreach ($p in $prios) {
        & cmd /c "run_alpha_tests.cmd -Prio $p -ExeDir ..\bin -LogDir ..\bin\logs"
        if ($LASTEXITCODE -ne 0) { $failed += $p }
    }
    if ($failed.Count -gt 0) {
        Write-Host "Failed: $($failed -join ', ')" -ForegroundColor Red
        exit 1
    }
```

Dann ist `SU2-Wilson-Loop` (2 s) nicht in CI, sondern nur im
Nightly-Job. Das ist akzeptabel, weil SU2-Wilson-Loop alle
SU(2)-Kinematik-Checks enthält, die sich nicht mit jedem Commit
ändern.

### 16.2 Nightly-Job (Prio 5 + Running-Coupling)

```yaml
- name: Nightly Alpha-Suite
  shell: pwsh
  working-directory: tools
  run: |
    & cmd /c "run_alpha_tests.cmd -Prio 5 -ExeDir ..\bin -LogDir ..\bin\logs"
    if ($LASTEXITCODE -ne 0) { exit 1 }
    & cmd /c "run_alpha_tests.cmd -Prio 8 -ExeDir ..\bin -LogDir ..\bin\logs"
    if ($LASTEXITCODE -ne 0) { exit 1 }
```

Laufzeit: ~65 min. Einmal pro Nacht akzeptabel.

### 16.3 Was jeder Testlauf sicherstellt

| Prio | Was geprüft wird |
|---|---|
| 1 | Grundmechanik (Born, Unitariät, Gauge U(1), CHSH) |
| 2 | Emergenz (Born, Lorentz, Interferenz, CHSH-Varianten) |
| 3 | Langlauf (Invarianten, Transport, Soliton) |
| 4 | 3D-Torus (Smoke, Invariance, Bloch) |
| 5 | Wasserstoff + U4 Shared-Reference |
| 6 | Spin-1/2 aus SU(2) |
| 7 | Dirac-Struktur |
| 8 | SU(2)-Eichfeld (kinematisch + dynamisch + V&V-Anker) |

---

## 17. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `FEHLER: ExeDir nicht gefunden` | `-ExeDir` zeigt auf falschen Pfad | Pfad prüfen oder ohne `-ExeDir` aufrufen |
| alle Tests `MISSING` | `ExeDir` enthält keine EXEs | erst `build.cmd` laufen |
| `Dirac FAIL` | Kernel nicht neu gebaut | `nmake` in `build\prophysics` |
| `SU2-Wilson-Loop FAIL` | Kernel nicht neu gebaut oder `ProPhysics_SU2_Dynamics.c` fehlt in `OBJ_KERNEL` | `build\prophysics\Makefile.nmake` prüfen, dann `nmake rebuild` |
| `Running-Coupling FAIL` | `Link_Plaquette_Sum` fehlt oder `alpha_test_running_coupling.c` nicht in Test-Makefile | `build\prophysics` und `build\test` prüfen |
| `Running-Coupling TIMEOUT` | Rechner sehr langsam; oder Creutz-Ratio dazugekommen | Timeout auf 3600 s erhöhen (Prio 8) |
| `Hydrogen-48 TIMEOUT` | Rechner sehr langsam | Timeout im Skript erhöhen (Prio 5) |
| `UNKNOWN`-Ergebnis | unbekannter Test-Marker | Ausgabe im Log prüfen, Regex erweitern |
| Logs leer | EXE stürzte beim Start | DLL fehlt in `bin\` |
| Umlaute in Logs kaputt | EXE ohne UTF-8 | Konsolen-Encoding prüfen |
| `cmd` fragt nach Parameter | interaktiv gestartet | `-Prio <wert>` übergeben |

---

## 18. Was das Skript nicht tut

- **Kein Build.** Setzt voraus, dass die EXEs in `bin\` liegen.
- **Keine parallele Ausführung.** Alle Tests laufen sequenziell.
- **Kein Retry.** Ein fehlgeschlagener Test wird nicht wiederholt.
- **Keine Trend-Analyse.** Kein Vergleich mit vorherigen Läufen.
- **Kein Parsen von Zahlen.** Nur PASS/FAIL-Marker werden erkannt.
- **Kein Push in Datenbank.** Logs bleiben im Dateisystem.
- **Kein Exclude-Mechanismus** (in v3.2 noch nicht eingebaut).

---

## 19. Parameter-Referenz (kompakt)

```
run_alpha_tests.cmd -Prio <1|2|3|4|5|6|7|8|all>
                    [-ExeDir <pfad>] [-LogDir <pfad>]
```

| Parameter | Typ | Default | Pflicht |
|---|---|---|---|
| `-Prio` | Choice | — | ja |
| `-ExeDir` | Pfad | `$PSScriptRoot\..\bin` | nein |
| `-LogDir` | Pfad | `<ExeDir>\logs` | nein |
| `-Exclude` | String[] | `@()` | nein (geplant für v3.3) |

---

## 20. Siehe auch

- `docs\test\ProPhysics_Testkatalog.md` — Test-Übersicht mit Kriterien
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\export.md` — Export-Wrapper
- `docs\build\BUILD_SCRIPT.md` — Übersicht des Build-Systems
- `src\test\alpha_test_main.c` — CLI-Parser der EXE
- `src\test\alpha_test_running_coupling.c` — Running-Coupling-Test

---

**Ende Test-Runner-Dokumentation v3.2.**