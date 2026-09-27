# ProPhysics pro_run — zentraler Einstiegspunkt

**Dateien:** `tools\pro_run.ps1` + `tools\pro_run.cmd`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Ein einheitlicher Einstiegspunkt fuer Build, Export und Test.
Dispatcht auf die bestehenden Skripte `build.ps1`, `export.ps1` und
`run_alpha_tests.ps1`. Fuehrt in der Aktion `all` die Kette
`build → test → export` aus.

---

## 1. Was das Skript tut

`pro_run` ist ein duenner Dispatcher. Es baut nichts selbst, es kopiert
nichts selbst, es testet nichts selbst — es ruft die zustaendigen
Skripte auf und reicht die Optionen durch.

| Aktion | Ruft auf | Zweck |
|---|---|---|
| `build` | `build\main\build.ps1` | Kernel/SDK/Tests bauen |
| `export` | `build\main\export.ps1` | Artefakte als ZIP exportieren |
| `test` | `tools\run_alpha_tests.ps1` | Alpha-Test-Suite ausfuehren |
| `all` | alle drei, in Reihenfolge | Kompletter Durchlauf |
| `help` | interne Ausgabe | Uebersicht oder Thema |

**Warum ein Dispatcher?**

- **Ein Befehl statt vier.** Vorher musste man wissen, welches Skript
  in welchem Ordner liegt und welche Parameter es kennt.
- **Konsistente Parameter-Syntax.** `-Config`, `-Prio`, `-Export` usw.
  werden an einer Stelle entgegengenommen und verteilt.
- **Automatisierbare Kette.** `all` ist die Grundlage fuer CI-Laeufe.
- **Fehlschlag-Bruch.** In `all` bricht ein Fehler den Durchlauf ab —
  kein Export roter Tests.

**Direkte Aufrufe der Subskripte bleiben gueltig.** Wer nur eine
Komponente braucht, kann weiterhin `build.cmd`, `export.cmd` oder
`run_alpha_tests.cmd` direkt aufrufen. `pro_run` ist der empfohlene
Weg, kein Zwang.

---

## 2. Ablageort

```
<repo>\
├── tools\
│   ├── pro_run.cmd              <- dieses Wrapper-Skript
│   ├── pro_run.ps1              <- dieses Skript
│   ├── run_alpha_tests.cmd
│   └── run_alpha_tests.ps1
├── build\
│   └── main\
│       ├── build.ps1 / build.cmd
│       ├── export.ps1 / export.cmd
│       ├── write_build_info.ps1
│       └── Makefile.nmake
├── bin\
├── lib\
├── src\
├── docs\
└── out\
```

Der Repo-Root wird aus dem Skript-Ablageort abgeleitet:

```powershell
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
```

`pro_run.ps1` liegt in `<repo>\tools\`, der Repo-Root ist eine Ebene
hoeher. Der Aufruf funktioniert damit aus **jedem CWD**.

---

## 3. Aufruf

### 3.1 Ueber `pro_run.cmd` (empfohlen in cmd.exe)

```cmd
pro_run all
pro_run build -Config debug -Rebuild
pro_run test -Prio 1-4
pro_run export -Export kit -OutDir D:\sdk-kit
pro_run help
```

Der `.cmd`-Wrapper setzt `chcp 65001` (UTF-8-Konsole) und reicht alle
Argumente an PowerShell weiter.

### 3.2 Direkt ueber PowerShell

```powershell
.\pro_run.ps1 all
.\pro_run.ps1 build -Config debug -Rebuild
.\pro_run.ps1 test -Prio 1-4
.\pro_run.ps1 export -Export kit -OutDir D:\sdk-kit
```

### 3.3 Aus jedem CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\tools\pro_run.cmd build -Mode kernel
```

Funktioniert, weil alle Pfade ueber `$PSScriptRoot` bestimmt werden.

### 3.4 Wenn `tools\` im PATH liegt

```cmd
pro_run all
pro_run test -Prio 8
```

Voraussetzung: `<repo>\tools\` ist in `%PATH%` eingetragen. Das ist
nicht Standard, muss einmalig eingerichtet werden.

---

## 4. Die Aktionen

`pro_run` kennt fuenf Aktionen. Die Aktion steht immer an erster
Position und ist Pflicht.

### 4.1 `build` — Bauen

Dispatcht auf `build\main\build.ps1`. Baut Kernel, SDK und/oder Tests.

**Relevante Parameter:**

| Parameter | Typ | Default | Wirkung |
|---|---|---|---|
| `-Mode` | Choice | `all` | `all` \| `kernel` \| `sdk` \| `test` |
| `-Config` | Choice | `release` | `release` \| `debug` |
| `-Rebuild` | Switch | aus | clean + build |

**Beispiele:**

```cmd
pro_run build
pro_run build -Mode kernel -Rebuild
pro_run build -Config debug
pro_run build -Mode sdk -Config debug -Rebuild
```

**Rueckgabe:** 0 bei Erfolg, `≠0` bei nmake-Fehler, 2 wenn `nmake`
nicht im PATH ist.

**Details:** `docs\build\helper\build.md`.

### 4.2 `export` — Exportieren

Dispatcht auf `build\main\export.ps1`. Baut optional und packt die
Artefakte als ZIP-Archiv.

**Relevante Parameter:**

| Parameter | Typ | Default | Wirkung |
|---|---|---|---|
| `-Export` | Choice | `sdk` | `exe` \| `sdk` \| `kit` \| `all` |
| `-OutDir` | Pfad | `<repo>\out` | Zielverzeichnis |
| `-Version` | String | aus Header | ZIP-Version-Tag |

**Beispiele:**

```cmd
pro_run export
pro_run export -Export kit
pro_run export -Export all -OutDir D:\release
pro_run export -Version 1.23.0-rc1
```

**Rueckgabe:** 0 bei Erfolg, `≠0` bei nmake-Fehler.

**Details:** `docs\build\helper\export.md`.

### 4.3 `test` — Testen

Dispatcht auf `tools\run_alpha_tests.ps1`. Fuehrt die Alpha-Test-Suite
aus.

**Relevante Parameter:**

| Parameter | Typ | Default | Wirkung |
|---|---|---|---|
| `-Prio` | String | `all` | `all` \| `1`..`8` \| `1-4` \| `1,3,5` |
| `-Test` | String | (leer) | einzelner Test nach Name |
| `-ExeDir` | Pfad | `<repo>\bin` | EXE-Verzeichnis |
| `-DllDir` | Pfad | `<repo>\bin` | DLL-Verzeichnis |
| `-LogDir` | Pfad | `<ExeDir>\logs` | Log-Verzeichnis |

**Beispiele:**

```cmd
pro_run test
pro_run test -Prio 1-4
pro_run test -Prio 8
pro_run test -Test Running-Coupling
pro_run test -Test Running-Coupling -LogDir C:\logs
```

**Rueckgabe:** 0 bei Erfolg, 1 wenn mindestens ein Test fehlgeschlagen
ist, 2 bei Aufbaufehler.

**Details:** `docs\test\run_alpha_tests.md`.

### 4.4 `all` — Kompletter Durchlauf

Fuehrt `build → test → export` in dieser Reihenfolge aus.

**Regeln:**

- **Bricht bei Fehlschlag ab.** Wenn `build` fehlschlaegt, wird
  `test` und `export` nicht mehr ausgefuehrt. Wenn `test` fehlschlaegt,
  wird `export` nicht mehr ausgefuehrt.
- **Skip-Flags.** Mit `-NoBuild`, `-NoTest` oder `-NoExport` kannst du
  einzelne Schritte ueberspringen. Mit allen drei Skip-Flags wird nur
  der Kopf ausgegeben, kein Schritt laeuft.
- **Parameter werden gerecht.** `-Config`, `-Rebuild`, `-Prio`,
  `-Export`, `-OutDir`, `-Version`, `-Test` usw. gehen an den
  jeweiligen Schritt.
- **Ein einziger Aufruf, alle Zwischenschritte mit Anzeige.**

**Reihenfolge und Kosten:**

| Schritt | Dauer | Skip-Flag |
|---|---|---|
| build | ~30 s (inkrementell) | `-NoBuild` |
| test | 2 s .. 74 min (Prio-abhaengig) | `-NoTest` |
| export | ~5 s (+ ZIP) | `-NoExport` |

**Beispiele:**

```cmd
:: kompletter Durchlauf (Build, alle Tests, SDK-ZIP)
pro_run all

:: debug build, schnelle Regression, alle drei ZIPs
pro_run all -Config debug -Prio 1-4 -Export all

:: nur Tests, Build existiert
pro_run all -NoBuild -NoExport -Prio 8

:: nur Build + Export, keine Tests
pro_run all -NoTest -Export all

:: nur Export, sonst nichts
pro_run all -NoBuild -NoTest
```

**Rueckgabe:** 0 wenn alle Schritte OK, sonst der erste Fehler-Exit.

### 4.5 `help` — Uebersicht

Gibt eine Uebersicht aus. Ohne Thema: Gesamtuebersicht mit
Aktionen und Beispielen. Mit Thema: spezifische Hilfe.

**Thema** wird ueber `-Test` uebergeben (pragmatische Wiederverwendung
des Parameters, siehe §10.3).

**Beispiele:**

```cmd
pro_run help
pro_run help -Test build
pro_run help -Test export
pro_run help -Test test
pro_run help -Test all
```

**Rueckgabe:** immer 0.

---

## 5. Die Kette `all` im Detail

### 5.1 Ablaufdiagramm

```
pro_run all
    │
    ├─ Aktion all erkannt
    │
    ├─ 1. Build (uebersprungen wenn -NoBuild)
    │   ├─ Ruft build.ps1 -Mode <m> -Config <c> [-Rebuild]
    │   ├─ Bei Fehlschlag: exit 1
    │   └─ Bei OK: weiter
    │
    ├─ 2. Test (uebersprungen wenn -NoTest)
    │   ├─ Ruft run_alpha_tests.ps1 -Prio <p>
    │   ├─ Bei Fehlschlag: exit <rc>
    │   └─ Bei OK: weiter
    │
    ├─ 3. Export (uebersprungen wenn -NoExport)
    │   ├─ Ruft export.ps1 -Export <e>
    │   ├─ Bei Fehlschlag: exit <rc>
    │   └─ Bei OK: weiter
    │
    └─ Zusammenfassung + exit 0
```

### 5.2 Parameter-Verteilung

| Parameter | Geht an |
|---|---|
| `-Mode`, `-Config`, `-Rebuild` | build-Schritt |
| `-Prio`, `-Test`, `-ExeDir`, `-DllDir`, `-LogDir` | test-Schritt |
| `-Export`, `-OutDir`, `-Version` | export-Schritt |
| `-DryRun` | alle Schritte |
| `-ShowVerbose` / `-v` | alle Schritte |
| `-NoBuild`, `-NoTest`, `-NoExport` | nur `all` |

### 5.3 Dry-Run in `all`

Mit `-DryRun` zeigt `all` alle Aufrufe an, die stattfinden wuerden,
ohne sie auszufuehren:

```
[*] Build  (build.ps1)
    [dry] powershell -File "H:\...\build.ps1" "-Mode" "all" "-Config" "release" "-DryRun"
    OK  Build abgeschlossen.

[*] Test  (run_alpha_tests.ps1)
    [dry] powershell -File "H:\...\run_alpha_tests.ps1" "-Prio" "all" "-DryRun"
    OK  Test abgeschlossen.

[*] Export  (export.ps1)
    [dry] powershell -File "H:\...\export.ps1" "-Export" "sdk" "-DryRun"
    OK  Export abgeschlossen.
```

Nuetzlich, um vor einem CI-Lauf zu pruefen, welche Optionen an welchen
Schritt gehen.

---

## 6. Parameter (vollstaendig)

### 6.1 Aktions-spezifische Parameter

| Parameter | Typ | Default | Geht an | Beschreibung |
|---|---|---|---|---|
| `-Mode` | Choice | `all` | build | `all` \| `kernel` \| `sdk` \| `test` |
| `-Config` | Choice | `release` | build | `release` \| `debug` |
| `-Rebuild` | Switch | aus | build | clean + build |
| `-Export` | Choice | `sdk` | export | `exe` \| `sdk` \| `kit` \| `all` |
| `-OutDir` | Pfad | `<repo>\out` | export | Zielverzeichnis |
| `-Version` | String | aus Header | export | ZIP-Version-Tag |
| `-Prio` | String | `all` | test | `all` \| `1`..`8` \| `1-4` \| `1,3,5` |
| `-Test` | String | (leer) | test, help | Testname oder Hilfethema |
| `-ExeDir` | Pfad | `<repo>\bin` | test | EXE-Verzeichnis |
| `-DllDir` | Pfad | `<repo>\bin` | test | DLL-Verzeichnis |
| `-LogDir` | Pfad | `<ExeDir>\logs` | test | Log-Verzeichnis |

### 6.2 Allgemeine Parameter

| Parameter | Typ | Default | Wirkung |
|---|---|---|---|
| `-NoBuild` | Switch | aus | build-Schritt in `all` ueberspringen |
| `-NoTest` | Switch | aus | test-Schritt in `all` ueberspringen |
| `-NoExport` | Switch | aus | export-Schritt in `all` ueberspringen |
| `-DryRun` | Switch | aus | nur auflisten, nichts ausfuehren |
| `-ShowVerbose` / `-v` | Switch | aus | ausfuehrliche Ausgabe (an Subskripte) |

### 6.3 Pflicht-Parameter

| Parameter | Typ | Position | Beschreibung |
|---|---|---|---|
| `-Aktion` | Choice | 0 | `build` \| `export` \| `test` \| `all` \| `help` |

Die Aktion muss immer angegeben werden. Alle anderen Parameter sind
optional.

---

## 7. Beispiele

### 7.1 Kompletter Durchlauf

```cmd
pro_run all
```

Ergebnis: Build (inkrementell), alle 43 Tests, SDK-ZIP.

### 7.2 Schnelle Regression

```cmd
pro_run all -Prio 1-4
```

Ergebnis: Build (inkrementell), Prios 1-4 (35 Tests, ~1 min), SDK-ZIP.

### 7.3 Debug-Build komplett

```cmd
pro_run all -Config debug -Export all
```

Ergebnis: Debug-Build, alle Tests, alle drei ZIPs.

### 7.4 Nur Tests, Build existiert

```cmd
pro_run all -NoBuild -NoExport -Prio 8
```

Ergebnis: kein Build, nur Prio-8-Tests (SU2 + Running-Coupling,
~30 min), kein Export.

### 7.5 Nur Kernel bauen

```cmd
pro_run build -Mode kernel -Rebuild
```

Ergebnis: `bin\ProPhysics.dll` + `lib\ProPhysics.lib` neu.

### 7.6 KIT exportieren

```cmd
pro_run export -Export kit -OutDir D:\sdk-kit
```

Ergebnis: `D:\sdk-kit\kit\` + `D:\sdk-kit\prophysics-kit-1.23.0.zip`.

### 7.7 Einzelner Test

```cmd
pro_run test -Test Running-Coupling -LogDir C:\logs
```

Ergebnis: nur `Running-Coupling`, Logs in `C:\logs`.

### 7.8 Dry-Run vor Release

```cmd
pro_run all -Config release -Export all -DryRun
```

Ergebnis: zeigt alle Aufrufe, kein Schreibzugriff.

### 7.9 Hilfe

```cmd
pro_run help
pro_run help -Test export
```

### 7.10 Ausfuehrliche Ausgabe

```cmd
pro_run build -v
pro_run all -v
```

Ergebnis: Subskripte erhalten `-Verbose`, mehr Diagnose.

---

## 8. Konsolenausgabe

### 8.1 Kopf

```

============================================================
  pro_run  |  all
============================================================
  Repo:   H:\ProPhysics_SDK\ProPhysics
  Config: release
```

### 8.2 Waehrend `all`

```
[*] Build  (build.ps1)

============================================================
  ProPhysics Build  |  Modus: all  Config: release
============================================================
...
    OK  Build abgeschlossen.
    OK  Build abgeschlossen.

[*] Test  (run_alpha_tests.ps1)

============================================================
  ProPhysics Alpha-Test-Runner -- all
============================================================
...
PASS: 43   FAIL: 0   ...

[*] Export  (export.ps1)

============================================================
  ProPhysics Export  |  Export: sdk  Scope: all
============================================================
...
```

### 8.3 Zusammenfassung

```
------------------------------------------------------------
  pro_run all: OK  (4521.3s)
------------------------------------------------------------
```

Bei Fehler:

```
------------------------------------------------------------
  pro_run all: FAILED (exit 1, 123.4s)
------------------------------------------------------------
```

Farbcodierung:

| Farbe | Bedeutung |
|---|---|
| Cyan | Schritt-Markierung, Kopf |
| Gruen | Erfolg |
| Gelb | Warnung (nicht kritisch) |
| Rot | Fehler |
| Grau | Dry-Run-Detail |

---

## 9. Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | Erfolg (alle Schritte OK) |
| `1` | Ein Test ist fehlgeschlagen, oder Export-Fehler |
| `2` | Aufbaufehler: Subskript fehlt, `nmake` nicht im PATH |
| `≠0` | Sonstiger Fehler-Exit, wird durchgereicht |

**In `all`:** der erste fehlgeschlagene Schritt bestimmt den Exit.
Bei Erfolg aller Schritte: 0.

**Im `help`-Modus:** immer 0.

---

## 10. Design-Entscheidungen

### 10.1 Aufruf via `powershell.exe -File` statt `& $script`

Die Subskripte werden als frische PowerShell-Prozesse gestartet:

```powershell
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Args
```

**Grund:**

- **Execution-Policy-Robustheit.** `-ExecutionPolicy Bypass` wirkt auf
  den neuen Prozess. `& $script` wuerde die Policy des aufrufenden
  Prozesses erben.
- **Saubere Exit-Codes.** `$LASTEXITCODE` kommt garantiert vom
  Subskript-Prozess, nicht von einer in-process Exception.
- **Isolation.** Encoding-Einstellungen, `$ErrorActionPreference` und
  Variablen der Subskripte leaken nicht in den Dispatcher.

### 10.2 `-ShowVerbose` statt `-Verbose`

`-Verbose` ist in PowerShell ein Common-Parameter. Wenn `pro_run.ps1`
ihn selbst als Parameter fuehrt, kollidiert das mit
`[CmdletBinding()]`-Verhalten. Daher:

- Parametername: `-ShowVerbose`
- Alias: `-v`

Weitergereicht wird an Subskripte als `-Verbose`.

### 10.3 `-Test` als Hilfethema-Alias

`pro_run help build` ist intuitiv, aber `build` waere dann als
Positionsparameter zu interpretieren — Konflikt mit `-Aktion`. Die
Loesung:

- `pro_run help` → Gesamtuebersicht
- `pro_run help -Test build` → Themen-Hilfe

Pragmatisch, aber nicht schoen. Eine Alternative waere ein
separater Positionsparameter `-Topic`, was aber einen weiteren
Konflikt mit `-Aktion` erzeugen wuerde. Die aktuelle Loesung
vermeidet die Konflikt-Erklaerung auf Kosten eines leichten
Missbrauchs von `-Test`.

### 10.4 Abbruch bei Fehlschlag in `all`

Ein Fehlschlag in `build` fuehrt dazu, dass `test` und `export`
nicht laufen. Das ist bewusst: ein Export roter Tests waere
irrefuehrend.

Mit `-NoTest` kann man explizit nur build + export wollen.

### 10.5 Keine State-Uebernahme

Jeder Subskript-Aufruf ist unabhaengig. `pro_run all` ruft nicht
einen gemeinsamen Kontext auf, sondern drei Prozesse. Das ist
einfacher und robuster als ein Zustandstransfer zwischen den
Schritten.

Konsequenz: `-Config debug` wird vom build-Schritt konsumiert, aber
der Export-Schritt baut nicht neu — er nutzt die Artefakte, die
`build` in `bin\` und `lib\` hinterlassen hat. Der `Config:`-Eintrag
in BUILD_INFO.txt zeigt `debug`, wenn `build` ihn geschrieben hat.

---

## 11. Zusammenspiel mit den Subskripten

| `pro_run ...` | Ruft auf |
|---|---|
| `pro_run build -Mode X -Config Y -Rebuild` | `build.ps1 -Mode X -Config Y -Rebuild` |
| `pro_run export -Export E -OutDir O -Version V` | `export.ps1 -Export E -OutDir O -Version V` |
| `pro_run test -Prio P` | `run_alpha_tests.ps1 -Prio P` |
| `pro_run test -Test T -LogDir L` | `run_alpha_tests.ps1 -Test T -LogDir L` |
| `pro_run all -NoExport -Prio 1-4` | `build.ps1` + `run_alpha_tests.ps1 -Prio 1-4` |

**Die Subskripte bleiben eigenstaendig.** `pro_run` ist ein Wrapper,
kein Ersatz. Ein Aufruf ohne `pro_run` funktioniert weiterhin:

```cmd
build\main\build.cmd -Mode kernel -Rebuild
build\main\export.cmd kit -OutDir D:\sdk-kit
tools\run_alpha_tests.cmd -Prio 1-4
```

---

## 12. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `pro_run nicht gefunden` | `tools\` nicht im PATH oder falscher CWD | `tools\pro_run.cmd` direkt aufrufen |
| `-Aktion ist erforderlich` | kein erstes Argument | `pro_run all` oder `pro_run help` |
| `Unbekannte Aktion` | Tippfehler | `build`, `export`, `test`, `all`, `help` |
| `Build-Skript fehlt` | `build.ps1` verschoben | `build\main\` pruefen |
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausfuehren |
| `Umlaute kaputt` | `chcp` fehlt | `pro_run.cmd` statt `pro_run.ps1` |
| `Parameter wird nicht durchgereicht` | falsche Aktion | `-Prio` geht nur an `test`, `-Config` nur an `build` |
| `test-Schritt bricht nach PRIO ab` | Timeout | Timeout in `run_alpha_tests.ps1` erhoehen |
| `exit 1 nach test` | ein Test FAILED | Log-Datei unter `bin\logs\` pruefen |
| `exit 2` | Aufbaufehler | Fehlermeldung oben lesen |
| `help -Test build` zeigt nichts | Thema-Schreibfehler | `build`, `export`, `test`, `all` |
| `all` ohne einen Schritt | alle Skip-Flags gesetzt | Warnung wird ausgegeben, exit 0 |

---

## 13. Was dieses Skript nicht tut

- **Kein eigener Build.** Es ruft `build.ps1` auf.
- **Kein eigener Export.** Es ruft `export.ps1` auf.
- **Keine eigenen Tests.** Es ruft `run_alpha_tests.ps1` auf.
- **Keine direkte nmake-Ansprache.** Der Dispatcher kennt nur die
  PowerShell-Skripte.
- **Keine parallele Ausfuehrung.** Alle Schritte laufen sequenziell.
- **Kein State-Cache.** Jeder Aufruf ist unabhaengig.
- **Kein Rollback.** Bei Fehlschlag werden Teilergebnisse nicht
  zurueckgenommen.

---

## 14. Parameter-Referenz (kompakt)

```
pro_run <build|export|test|all|help> [optionen]

  Allgemein:
    -DryRun                     nur auflisten
    -ShowVerbose / -v           ausfuehrliche Ausgabe

  build:
    -Mode <all|kernel|sdk|test> (Default: all)
    -Config <release|debug>     (Default: release)
    -Rebuild                    clean + build

  export:
    -Export <exe|sdk|kit|all>   (Default: sdk)
    -OutDir <pfad>              (Default: <repo>\out)
    -Version <tag>              (Default: aus Header)

  test:
    -Prio <all|1..8|1-4|1,3,5>  (Default: all)
    -Test <name>                statt Prio
    -ExeDir <pfad>              (Default: <repo>\bin)
    -DllDir <pfad>              (Default: <repo>\bin)
    -LogDir <pfad>              (Default: <ExeDir>\logs)

  all:
    -NoBuild                    build ueberspringen
    -NoTest                     test ueberspringen
    -NoExport                   export ueberspringen
    (+ alle Parameter der drei Subskripte)

  help:
    -Test <thema>               build | export | test | all
```

---

## 15. Beispiele fuer typische Szenarien

### 15.1 Lokaler Entwickler-Workflow

```cmd
:: Schnellregression: Kernel bauen, Prios 1-4, SDK exportieren
pro_run all -Prio 1-4

:: Bei Aenderung an Kernel-Code:
pro_run build -Mode kernel -Rebuild
pro_run all -NoBuild -NoExport -Prio 1
```

### 15.2 Release-Workflow

```cmd
:: kompletter Rebuild, alle Tests, alle Pakete
pro_run all -Rebuild -Config release -Export all -Version 1.23.0
```

### 15.3 CI-Workflow

```cmd
:: in CI: Artefakte schon gebaut, nur Tests + alle ZIPs
pro_run all -NoBuild -Prio all -Export all -OutDir %CI_ARTIFACT_DIR%
```

### 15.4 Debug-Workflow

```cmd
:: nur Kernel im Debug-Modus
pro_run build -Mode kernel -Config debug -Rebuild

:: Debug-ZIP
pro_run export -Export sdk -Version 1.23.0-debug
```

### 15.5 Nach einem manuellen nmake

```cmd
:: Info auffrischen, nichts bauen
pro_run build -Mode info
```

---

## 16. Siehe auch

- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\export.md` — Export-Wrapper
- `docs\build\helper\write_build_info.md` — BUILD_INFO.txt
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\sdk\Makefile.md` — SDK-Build
- `docs\build\test\Makefile.md` — Test-Build
- `docs\test\run_alpha_tests.md` — Test-Runner
- `docs\test\ProPhysics_Testkatalog.md` — Test-Übersicht

---

**Ende pro_run-Dokumentation (v1.0.0).**