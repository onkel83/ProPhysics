# ProPhysics pro_run — zentraler Einstiegspunkt

**Dateien:** `tools\pro_run.ps1` + `tools\pro_run.cmd`
**Version:** 1.1.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Ein einheitlicher Einstiegspunkt für Build, Export, Test
und Web-Docs. Dispatcht auf die bestehenden Skripte `build.ps1`,
`export.ps1`, `run_alpha_tests.ps1` und `prowb.exe`. Führt in der
Aktion `all` die Kette `build → test → export` aus.

---

## §1 — Was das Skript tut

`pro_run` ist ein dünner Dispatcher. Es baut nichts selbst, es kopiert
nichts selbst, es testet nichts selbst — es ruft die zuständigen
Skripte auf und reicht die Optionen durch.

| Aktion | Ruft auf | Zweck |
|---|---|---|
| `build` | `build\main\build.ps1` | Kernel/SDK/Tests bauen |
| `export` | `build\main\export.ps1` | Artefakte als ZIP exportieren |
| `test` | `tools\run_alpha_tests.ps1` | Alpha-Test-Suite ausführen |
| `web` | `build\prowb\prowb.exe` | Web-Docs bauen (ProWB) |
| `all` | build → test → export | Kompletter Durchlauf |
| `help` | interne Ausgabe | Übersicht oder Thema |

**Warum ein Dispatcher?**

- **Ein Befehl statt fünf.** Vorher musste man wissen, welches Skript
  in welchem Ordner liegt und welche Parameter es kennt.
- **Konsistente Parameter-Syntax.** `-Config`, `-Prio`, `-Export`,
  `-Rebuild` usw. werden an einer Stelle entgegengenommen und verteilt.
- **Automatisierbare Kette.** `all` ist die Grundlage für CI-Läufe.
- **Fehlschlag-Bruch.** In `all` bricht ein Fehler den Durchlauf ab —
  kein Export roter Tests.

**Direkte Aufrufe der Subskripte bleiben gültig.** Wer nur eine
Komponente braucht, kann weiterhin `build.cmd`, `export.cmd`,
`run_alpha_tests.cmd` oder `prowb.exe` direkt aufrufen. `pro_run`
ist der empfohlene Weg, kein Zwang.

**Neu in v1.1.0:** Aktion `web`. Dispatcht auf `build\prowb\prowb.exe`
und baut die Web-Docs aus `docs\web\manifest.txt` nach `out\web\`.
Details siehe §4.5.

---

## §2 — Ablageort

```
<repo>\
├── tools\
│   ├── pro_run.cmd              <- dieses Wrapper-Skript
│   ├── pro_run.ps1              <- dieses Skript
│   ├── run_alpha_tests.cmd
│   └── run_alpha_tests.ps1
├── build\
│   ├── main\
│   │   ├── build.ps1 / build.cmd
│   │   ├── export.ps1 / export.cmd
│   │   ├── write_build_info.ps1
│   │   └── Makefile.nmake
│   └── prowb\
│       └── Makefile.nmake
├── src\
│   └── prowb\
│       ├── prowb.c
│       └── header\prowb.h
├── docs\
│   └── web\
│       ├── manifest.txt
│       └── src\
├── bin\
├── lib\
└── out\
    └── web\                     <- Ziel des `web`-Schritts
```

Der Repo-Root wird aus dem Skript-Ablageort abgeleitet:

```powershell
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
```

`pro_run.ps1` liegt in `<repo>\tools\`, der Repo-Root ist eine Ebene
höher. Der Aufruf funktioniert damit aus **jedem CWD**.

---

## §3 — Aufruf

### §3.1 — Über `pro_run.cmd` (empfohlen in cmd.exe)

```cmd
pro_run all
pro_run build -Config debug -Rebuild
pro_run test -Prio 1-4
pro_run export -Export kit -OutDir D:\sdk-kit
pro_run web
pro_run help
```

Der `.cmd`-Wrapper setzt `chcp 65001` (UTF-8-Konsole) und reicht alle
Argumente an PowerShell weiter.

### §3.2 — Direkt über PowerShell

```powershell
.\pro_run.ps1 all
.\pro_run.ps1 build -Config debug -Rebuild
.\pro_run.ps1 test -Prio 1-4
.\pro_run.ps1 export -Export kit -OutDir D:\sdk-kit
.\pro_run.ps1 web -Rebuild
```

### §3.3 — Aus jedem CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\tools\pro_run.cmd build -Mode kernel
```

Funktioniert, weil alle Pfade über `$PSScriptRoot` bestimmt werden.

### §3.4 — Wenn `tools\` im PATH liegt

```cmd
pro_run all
pro_run test -Prio 8
pro_run web
```

Voraussetzung: `<repo>\tools\` ist in `%PATH%` eingetragen. Das ist
nicht Standard, muss einmalig eingerichtet werden.

---

## §4 — Die Aktionen

`pro_run` kennt **sechs** Aktionen. Die Aktion steht immer an erster
Position und ist Pflicht.

### §4.1 — `build` — Bauen

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

**Rückgabe:** 0 bei Erfolg, `≠0` bei nmake-Fehler, 2 wenn `nmake`
nicht im PATH ist.

**Details:** `docs\build\helper\build.md`.

### §4.2 — `export` — Exportieren

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

**Rückgabe:** 0 bei Erfolg, `≠0` bei nmake-Fehler.

**Details:** `docs\build\helper\export.md`.

### §4.3 — `test` — Testen

Dispatcht auf `tools\run_alpha_tests.ps1`. Führt die Alpha-Test-Suite
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

**Rückgabe:** 0 bei Erfolg, 1 wenn mindestens ein Test fehlgeschlagen
ist, 2 bei Aufbaufehler.

**Details:** `docs\test\run_alpha_tests.md`.

### §4.4 — `all` — Kompletter Durchlauf

Führt `build → test → export` in dieser Reihenfolge aus.

**Regeln:**

- **Bricht bei Fehlschlag ab.** Wenn `build` fehlschlägt, wird
  `test` und `export` nicht mehr ausgeführt. Wenn `test` fehlschlägt,
  wird `export` nicht mehr ausgeführt.
- **Skip-Flags.** Mit `-NoBuild`, `-NoTest` oder `-NoExport` kannst du
  einzelne Schritte überspringen. Mit allen drei Skip-Flags wird nur
  der Kopf ausgegeben, kein Schritt läuft.
- **Parameter werden gereicht.** `-Config`, `-Rebuild`, `-Prio`,
  `-Export`, `-OutDir`, `-Version`, `-Test` usw. gehen an den
  jeweiligen Schritt.
- **Ein einziger Aufruf, alle Zwischenschritte mit Anzeige.**

**Reihenfolge und Kosten:**

| Schritt | Dauer | Skip-Flag |
|---|---|---|
| build | ~30 s (inkrementell) | `-NoBuild` |
| test | 2 s .. 74 min (Prio-abhängig) | `-NoTest` |
| export | ~5 s (+ ZIP) | `-NoExport` |

**Nicht enthalten:** Der `web`-Schritt ist **nicht** in `all`. Grund:
Web-Docs sind ein publizierbares Artefakt, kein Runtime-Bestandteil.
Wer sie mit-bauen will, ruft `pro_run web` separat auf oder ergänzt
`pro_run all && pro_run web`.

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

**Rückgabe:** 0 wenn alle Schritte OK, sonst der erste Fehler-Exit.

### §4.5 — `web` — Web-Docs bauen (neu in v1.1.0)

Dispatcht auf `build\prowb\prowb.exe`. Baut die Web-Docs aus
`docs\web\manifest.txt` nach `out\web\`.

**Ablauf:**

1. `nmake /NOLOGO /f Makefile.nmake CONFIG=release` in `build\prowb\`
   — baut `bin\prowb\prowb.exe`, falls nicht vorhanden.
2. `bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt <OutDir>`
   — erzeugt `index.html` + Assets.

**Relevante Parameter:**

| Parameter | Typ | Default | Wirkung |
|---|---|---|---|
| `-Rebuild` | Switch | aus | `nmake clean` + ProWB-Build neu |
| `-OutDir` | Pfad | `<repo>\out\web` | Ziel-Verzeichnis |

**Beispiele:**

```cmd
:: Standard-Build
pro_run web

:: kompletter Rebuild (clean + build)
pro_run web -Rebuild

:: in ein anderes Ziel
pro_run web -OutDir D:\docs-portal

:: Dry-Run (zeigt nur die Aufrufe)
pro_run web -DryRun
```

**Was passiert:**

- `bin\prowb\prowb.exe` wird gebaut, falls nötig.
- `out\web\` wird angelegt, falls nötig.
- `out\web\index.html` wird geschrieben (~1,2 MB bei 39 Docs).
- `out\web\assets\` enthält CSS + JS.
- `out\web\data\docs.js` enthält die geparsten MD-Inhalte.

**Was passiert nicht:**

- `out\web\` wird **nicht** geleert. Bei `-Rebuild` wird
  `bin\prowb\prowb.exe` neu gebaut, aber `out\web\` bleibt bis
  zum nächsten ProWB-Lauf. Wer das Portal sauber neu will, muss
  `out\web\` manuell löschen oder ein Folge-Patch nutzen.
- Kein Kernel-Build. Kein SDK-Build. Kein Test.

**Rückgabe:** 0 bei Erfolg, `≠0` bei nmake- oder ProWB-Fehler.

**Details:**

- `docs\build\prowb\Makefile.md` — ProWB-Makefile
- `src\prowb\README.md` — Builder-Referenz
- `docs\web\README.md` — Manifest-Pflege
- `docs\build\web-docs-ci.md` — CI-Workflow

### §4.6 — `help` — Übersicht

Gibt eine Übersicht aus. Ohne Thema: Gesamtübersicht mit
Aktionen und Beispielen. Mit Thema: spezifische Hilfe.

**Thema** wird über `-Test` übergeben (pragmatische Wiederverwendung
des Parameters, siehe §10.3).

**Beispiele:**

```cmd
pro_run help
pro_run help -Test build
pro_run help -Test export
pro_run help -Test test
pro_run help -Test web
pro_run help -Test all
```

**Rückgabe:** immer 0.

---

## §5 — Die Kette `all` im Detail

### §5.1 — Ablaufdiagramm

```
pro_run all
    │
    ├─ Aktion all erkannt
    │
    ├─ 1. Build (übersprungen wenn -NoBuild)
    │   ├─ Ruft build.ps1 -Mode <m> -Config <c> [-Rebuild]
    │   ├─ Bei Fehlschlag: exit 1
    │   └─ Bei OK: weiter
    │
    ├─ 2. Test (übersprungen wenn -NoTest)
    │   ├─ Ruft run_alpha_tests.ps1 -Prio <p>
    │   ├─ Bei Fehlschlag: exit <rc>
    │   └─ Bei OK: weiter
    │
    ├─ 3. Export (übersprungen wenn -NoExport)
    │   ├─ Ruft export.ps1 -Export <e>
    │   ├─ Bei Fehlschlag: exit <rc>
    │   └─ Bei OK: weiter
    │
    └─ Zusammenfassung + exit 0
```

**Der `web`-Schritt ist nicht Teil von `all`.** Wer Web-Docs mit
publizieren will:

```cmd
pro_run all && pro_run web
```

### §5.2 — Parameter-Verteilung

| Parameter | Geht an |
|---|---|
| `-Mode`, `-Config`, `-Rebuild` | build-Schritt |
| `-Prio`, `-Test`, `-ExeDir`, `-DllDir`, `-LogDir` | test-Schritt |
| `-Export`, `-OutDir`, `-Version` | export-Schritt |
| `-DryRun` | alle Schritte |
| `-ShowVerbose` / `-v` | alle Schritte |
| `-NoBuild`, `-NoTest`, `-NoExport` | nur `all` |

**Hinweis:** `-Rebuild` wirkt **unterschiedlich** je Aktion:

- Bei `build` → `rebuild_<scope>` in nmake.
- Bei `export` → `rebuild_<scope>` in nmake.
- Bei `web` → `clean_prowb` + `prowb` (ProWB-Makefile).

### §5.3 — Dry-Run in `all`

Mit `-DryRun` zeigt `all` alle Aufrufe an, die stattfinden würden,
ohne sie auszuführen:

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

Nützlich, um vor einem CI-Lauf zu prüfen, welche Optionen an welchen
Schritt gehen.

### §5.4 — Dry-Run in `web`

```
[*] Web  (prowb.exe)
    [dry] nmake /NOLOGO /f Makefile.nmake CONFIG=release
    [dry] bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web
    OK  Web-Docs abgeschlossen.
```

---

## §6 — Parameter (vollständig)

### §6.1 — Aktions-spezifische Parameter

| Parameter | Typ | Default | Geht an | Beschreibung |
|---|---|---|---|---|
| `-Mode` | Choice | `all` | build | `all` \| `kernel` \| `sdk` \| `test` |
| `-Config` | Choice | `release` | build | `release` \| `debug` |
| `-Rebuild` | Switch | aus | build, export, web | clean + build |
| `-Export` | Choice | `sdk` | export | `exe` \| `sdk` \| `kit` \| `all` |
| `-OutDir` | Pfad | `<repo>\out` bzw. `<repo>\out\web` | export, web | Zielverzeichnis |
| `-Version` | String | aus Header | export | ZIP-Version-Tag |
| `-Prio` | String | `all` | test | `all` \| `1`..`8` \| `1-4` \| `1,3,5` |
| `-Test` | String | (leer) | test, help | Testname oder Hilfethema |
| `-ExeDir` | Pfad | `<repo>\bin` | test | EXE-Verzeichnis |
| `-DllDir` | Pfad | `<repo>\bin` | test | DLL-Verzeichnis |
| `-LogDir` | Pfad | `<ExeDir>\logs` | test | Log-Verzeichnis |

**Hinweis zu `-OutDir`:** Der Default ist aktions-abhängig.

- Bei `export` → `<repo>\out` (Wurzel, darunter entstehen
  `exe\`, `sdk\`, `kit\`).
- Bei `web` → `<repo>\out\web` (direkt das Ziel).

### §6.2 — Allgemeine Parameter

| Parameter | Typ | Default | Wirkung |
|---|---|---|---|
| `-NoBuild` | Switch | aus | build-Schritt in `all` überspringen |
| `-NoTest` | Switch | aus | test-Schritt in `all` überspringen |
| `-NoExport` | Switch | aus | export-Schritt in `all` überspringen |
| `-DryRun` | Switch | aus | nur auflisten, nichts ausführen |
| `-ShowVerbose` / `-v` | Switch | aus | ausführliche Ausgabe (an Subskripte) |

### §6.3 — Pflicht-Parameter

| Parameter | Typ | Position | Beschreibung |
|---|---|---|---|
| `-Aktion` | Choice | 0 | `build` \| `export` \| `test` \| `web` \| `all` \| `help` |

Die Aktion muss immer angegeben werden. Alle anderen Parameter sind
optional.

---

## §7 — Beispiele

### §7.1 — Kompletter Durchlauf

```cmd
pro_run all
```

Ergebnis: Build (inkrementell), alle 43 Tests, SDK-ZIP.

### §7.2 — Schnelle Regression

```cmd
pro_run all -Prio 1-4
```

Ergebnis: Build (inkrementell), Prios 1-4 (35 Tests, ~1 min), SDK-ZIP.

### §7.3 — Debug-Build komplett

```cmd
pro_run all -Config debug -Export all
```

Ergebnis: Debug-Build, alle Tests, alle drei ZIPs.

### §7.4 — Nur Tests, Build existiert

```cmd
pro_run all -NoBuild -NoExport -Prio 8
```

Ergebnis: kein Build, nur Prio-8-Tests (SU2 + Running-Coupling,
~30 min), kein Export.

### §7.5 — Nur Kernel bauen

```cmd
pro_run build -Mode kernel -Rebuild
```

Ergebnis: `bin\ProPhysics.dll` + `lib\ProPhysics.lib` neu.

### §7.6 — KIT exportieren

```cmd
pro_run export -Export kit -OutDir D:\sdk-kit
```

Ergebnis: `D:\sdk-kit\kit\` + `D:\sdk-kit\prophysics-kit-1.23.0.zip`.

### §7.7 — Einzelner Test

```cmd
pro_run test -Test Running-Coupling -LogDir C:\logs
```

Ergebnis: nur `Running-Coupling`, Logs in `C:\logs`.

### §7.8 — Web-Docs bauen

```cmd
pro_run web
```

Ergebnis: `out\web\index.html` + Assets. Voraussetzung:
`bin\prowb\prowb.exe` existiert oder wird gebaut.

### §7.9 — Web-Docs mit Rebuild

```cmd
pro_run web -Rebuild
```

Ergebnis: `bin\prowb\prowb.exe` wird neu gebaut, dann `out\web\`
neu erzeugt.

### §7.10 — Web-Docs in ein anderes Ziel

```cmd
pro_run web -OutDir D:\docs-portal
```

Ergebnis: `D:\docs-portal\index.html` + Assets.

### §7.11 — Dry-Run vor Release

```cmd
pro_run all -Config release -Export all -DryRun
```

Ergebnis: zeigt alle Aufrufe, kein Schreibzugriff.

### §7.12 — Dry-Run für Web-Docs

```cmd
pro_run web -DryRun
```

Ergebnis: zeigt die nmake- und prowb-Aufrufe, kein Schreibzugriff.

### §7.13 — Hilfe

```cmd
pro_run help
pro_run help -Test export
pro_run help -Test web
```

### §7.14 — Ausführliche Ausgabe

```cmd
pro_run build -v
pro_run all -v
pro_run web -v
```

Ergebnis: Subskripte erhalten `-Verbose`, mehr Diagnose.

---

## §8 — Konsolenausgabe

### §8.1 — Kopf

```

============================================================
  pro_run  |  all
============================================================
  Repo:   H:\ProPhysics_SDK\ProPhysics
  Config: release
```

Bei `web`:

```

============================================================
  pro_run  |  web
============================================================
  Repo:   H:\ProPhysics_SDK\ProPhysics
  Out:    H:\ProPhysics_SDK\ProPhysics\out\web
```

### §8.2 — Während `all`

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

### §8.3 — Während `web`

```
[*] Web  (prowb.exe)

============================================================
  ProPhysics ProWB Build  |  CONFIG=release
============================================================
[CC] prowb.c
[CC] md_parser.c
[LD] bin\prowb\prowb.exe
    OK  Builder gebaut.

============================================================
  ProPhysics ProWB Build  |  Manifest
============================================================
[ProWB] Manifest geladen: 39 Einträge, 6 Sektionen
[ProWB]   Docs: 39 geparst, 0 fehlend
[ProWB] Build erfolgreich.
    OK  out\web\index.html erzeugt (1225907 Bytes).
```

### §8.4 — Zusammenfassung

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
| Grün | Erfolg |
| Gelb | Warnung (nicht kritisch) |
| Rot | Fehler |
| Grau | Dry-Run-Detail |

---

## §9 — Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | Erfolg (alle Schritte OK) |
| `1` | Ein Test ist fehlgeschlagen, oder Export-Fehler |
| `2` | Aufbaufehler: Subskript fehlt, `nmake` nicht im PATH |
| `≠0` | Sonstiger Fehler-Exit, wird durchgereicht |

**In `all`:** der erste fehlgeschlagene Schritt bestimmt den Exit.
Bei Erfolg aller Schritte: 0.

**In `web`:** der Exit-Code von `prowb.exe`. 0 = OK, 2 = Manifest-
Fehler, 3 = IO-Fehler (siehe `src\prowb\README.md` §8).

**Im `help`-Modus:** immer 0.

---

## §10 — Design-Entscheidungen

### §10.1 — Aufruf via `powershell.exe -File` statt `& $script`

Die Subskripte werden als frische PowerShell-Prozesse gestartet:

```powershell
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Args
```

**Grund:**

- **Execution-Policy-Robustheit.** `-ExecutionPolicy Bypass` wirkt auf
  den neuen Prozess. `& $script` würde die Policy des aufrufenden
  Prozesses erben.
- **Saubere Exit-Codes.** `$LASTEXITCODE` kommt garantiert vom
  Subskript-Prozess, nicht von einer in-process Exception.
- **Isolation.** Encoding-Einstellungen, `$ErrorActionPreference` und
  Variablen der Subskripte leaken nicht in den Dispatcher.

### §10.2 — `-ShowVerbose` statt `-Verbose`

`-Verbose` ist in PowerShell ein Common-Parameter. Wenn `pro_run.ps1`
ihn selbst als Parameter führt, kollidiert das mit
`[CmdletBinding()]`-Verhalten. Daher:

- Parametername: `-ShowVerbose`
- Alias: `-v`

Weitergereicht wird an Subskripte als `-Verbose`.

### §10.3 — `-Test` als Hilfethema-Alias

`pro_run help build` ist intuitiv, aber `build` wäre dann als
Positionsparameter zu interpretieren — Konflikt mit `-Aktion`. Die
Lösung:

- `pro_run help` → Gesamtübersicht
- `pro_run help -Test build` → Themen-Hilfe

Pragmatisch, aber nicht schön. Eine Alternative wäre ein
separater Positionsparameter `-Topic`, was aber einen weiteren
Konflikt mit `-Aktion` erzeugen würde. Die aktuelle Lösung
vermeidet die Konflikt-Erklärung auf Kosten eines leichten
Missbrauchs von `-Test`.

### §10.4 — Abbruch bei Fehlschlag in `all`

Ein Fehlschlag in `build` führt dazu, dass `test` und `export`
nicht laufen. Das ist bewusst: ein Export roter Tests wäre
irreführend.

Mit `-NoTest` kann man explizit nur build + export wollen.

### §10.5 — `web` nicht in `all`

`web` ist **nicht** Teil der `all`-Kette. Grund:

- **Kein Runtime-Artefakt.** Web-Docs sind publizierbar, aber nicht
  Teil des ausgelieferten Kernels.
- **Kein Test.** Der Web-Docs-Build fügt keine Tests zur Regression
  hinzu.
- **Unabhängigkeit.** Wer nur den Kernel weiterentwickelt, soll
  nicht gezwungen sein, die Web-Docs mit-zubauen.

**Konsequenz:** Für den vollen Durchlauf inkl. Web-Docs:

```cmd
pro_run all && pro_run web
```

oder in CI der separate Workflow
`.github\workflows\web-docs.yml`.

### §10.6 — Keine State-Übernahme

Jeder Subskript-Aufruf ist unabhängig. `pro_run all` ruft nicht
einen gemeinsamen Kontext auf, sondern drei Prozesse. Das ist
einfacher und robuster als ein Zustandstransfer zwischen den
Schritten.

Konsequenz: `-Config debug` wird vom build-Schritt konsumiert, aber
der Export-Schritt baut nicht neu — er nutzt die Artefakte, die
`build` in `bin\` und `lib\` hinterlassen hat. Der `Config:`-Eintrag
in BUILD_INFO.txt zeigt `debug`, wenn `build` ihn geschrieben hat.

---

## §11 — Zusammenspiel mit den Subskripten

| `pro_run ...` | Ruft auf |
|---|---|
| `pro_run build -Mode X -Config Y -Rebuild` | `build.ps1 -Mode X -Config Y -Rebuild` |
| `pro_run export -Export E -OutDir O -Version V` | `export.ps1 -Export E -OutDir O -Version V` |
| `pro_run test -Prio P` | `run_alpha_tests.ps1 -Prio P` |
| `pro_run test -Test T -LogDir L` | `run_alpha_tests.ps1 -Test T -LogDir L` |
| `pro_run web -Rebuild -OutDir O` | `nmake` in `build\prowb` + `prowb.exe --manifest … O` |
| `pro_run all -NoExport -Prio 1-4` | `build.ps1` + `run_alpha_tests.ps1 -Prio 1-4` |

**Die Subskripte bleiben eigenständig.** `pro_run` ist ein Wrapper,
kein Ersatz. Ein Aufruf ohne `pro_run` funktioniert weiterhin:

```cmd
build\main\build.cmd -Mode kernel -Rebuild
build\main\export.cmd kit -OutDir D:\sdk-kit
tools\run_alpha_tests.cmd -Prio 1-4
bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web
```

---

## §12 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `pro_run nicht gefunden` | `tools\` nicht im PATH oder falscher CWD | `tools\pro_run.cmd` direkt aufrufen |
| `-Aktion ist erforderlich` | kein erstes Argument | `pro_run all` oder `pro_run help` |
| `Unbekannte Aktion` | Tippfehler | `build`, `export`, `test`, `web`, `all`, `help` |
| `Build-Skript fehlt` | `build.ps1` verschoben | `build\main\` prüfen |
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausführen |
| `Umlaute kaputt` | `chcp` fehlt | `pro_run.cmd` statt `pro_run.ps1` |
| `Parameter wird nicht durchgereicht` | falsche Aktion | `-Prio` geht nur an `test`, `-Config` nur an `build`, `-OutDir` an `export` oder `web` |
| `test-Schritt bricht nach PRIO ab` | Timeout | Timeout in `run_alpha_tests.ps1` erhöhen |
| `exit 1 nach test` | ein Test FAILED | Log-Datei unter `bin\logs\` prüfen |
| `exit 2` | Aufbaufehler | Fehlermeldung oben lesen |
| `help -Test build` zeigt nichts | Thema-Schreibfehler | `build`, `export`, `test`, `web`, `all` |
| `all` ohne einen Schritt | alle Skip-Flags gesetzt | Warnung wird ausgegeben, exit 0 |
| `web` findet `prowb.exe` nicht | ProWB nicht gebaut | `pro_run web` baut automatisch |
| `web` findet `manifest.txt` nicht | Manifest fehlt | `docs\web\manifest.txt` prüfen |
| `web` erzeugt leeres `index.html` | Manifest leer oder Parser-Fehler | Manifest + `pro_run web -v` prüfen |
| `web` schreibt nicht nach `OutDir` | Pfad gesperrt | Ordner-Berechtigungen prüfen |
| `nmake CONFIG=debug` in `web` | nur `release` unterstützt | ProWB ist IO-lastig, nur `release` |
| `prowb.exe` gibt Exit 3 | IO-Fehler | `out\web\` beschreibbar? |

---

## §13 — Was dieses Skript nicht tut

- **Kein eigener Build.** Es ruft `build.ps1` auf.
- **Kein eigener Export.** Es ruft `export.ps1` auf.
- **Keine eigenen Tests.** Es ruft `run_alpha_tests.ps1` auf.
- **Keine eigene Web-Docs-Logik.** Es ruft `prowb.exe` auf.
- **Keine direkte nmake-Ansprache** außer für ProWB.
- **Keine parallele Ausführung.** Alle Schritte laufen sequenziell.
- **Kein State-Cache.** Jeder Aufruf ist unabhängig.
- **Kein Rollback.** Bei Fehlschlag werden Teilergebnisse nicht
  zurückgenommen.
- **Kein Deployment.** Der `web`-Schritt baut nur lokal. Deploy auf
  GitHub Pages läuft über die CI.

---

## §14 — Parameter-Referenz (kompakt)

```
pro_run <build|export|test|web|all|help> [optionen]

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

  web:
    -Rebuild                    clean + build ProWB
    -OutDir <pfad>              (Default: <repo>\out\web)

  all:
    -NoBuild                    build ueberspringen
    -NoTest                     test ueberspringen
    -NoExport                   export ueberspringen
    (+ alle Parameter der drei Subskripte)

  help:
    -Test <thema>               build | export | test | web | all
```

---

## §15 — Beispiele für typische Szenarien

### §15.1 — Lokaler Entwickler-Workflow

```cmd
:: Schnellregression: Kernel bauen, Prios 1-4, SDK exportieren
pro_run all -Prio 1-4

:: Bei Aenderung an Kernel-Code:
pro_run build -Mode kernel -Rebuild
pro_run all -NoBuild -NoExport -Prio 1
```

### §15.2 — Release-Workflow

```cmd
:: kompletter Rebuild, alle Tests, alle Pakete
pro_run all -Rebuild -Config release -Export all -Version 1.23.0
```

### §15.3 — CI-Workflow

```cmd
:: in CI: Artefakte schon gebaut, nur Tests + alle ZIPs
pro_run all -NoBuild -Prio all -Export all -OutDir %CI_ARTIFACT_DIR%
```

### §15.4 — Debug-Workflow

```cmd
:: nur Kernel im Debug-Modus
pro_run build -Mode kernel -Config debug -Rebuild

:: Debug-ZIP
pro_run export -Export sdk -Version 1.23.0-debug
```

### §15.5 — Nach einem manuellen nmake

```cmd
:: Info auffrischen, nichts bauen
pro_run build -Mode info
```

### §15.6 — Web-Docs bauen

```cmd
:: Standard
pro_run web

:: Kompletter Rebuild
pro_run web -Rebuild

:: In anderes Ziel
pro_run web -OutDir D:\docs-portal
```

### §15.7 — Web-Docs + Build in einem Aufruf

```cmd
:: erst Kernel/SDK/Tests, dann Web-Docs
pro_run all -Prio 1-4 && pro_run web
```

### §15.8 — Web-Docs debuggen

```cmd
:: ausfuehrliche Ausgabe, zeigt stderr-Warnungen von ProWB
pro_run web -v
```

---

## §16 — Siehe auch

| Thema | Datei |
|---|---|
| Build-System-Übersicht | `docs\build\BUILD_SCRIPT.md` |
| Build-Wrapper | `docs\build\helper\build.md` |
| Export-Wrapper | `docs\build\helper\export.md` |
| BUILD_INFO.txt | `docs\build\helper\write_build_info.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Kernel-Build | `docs\build\prophysics\Makefile.md` |
| SDK-Build | `docs\build\sdk\Makefile.md` |
| Test-Build | `docs\build\test\Makefile.md` |
| **ProWB-Makefile** | `docs\build\prowb\Makefile.md` |
| **ProWB-README** | `src\prowb\README.md` |
| **Web-Docs-Manifest** | `docs\web\README.md` |
| **Web-Docs-CI** | `docs\build\web-docs-ci.md` |
| Test-Runner | `docs\test\run_alpha_tests.md` |
| Test-Übersicht | `docs\test\ProPhysics_Testkatalog.md` |
| Changelog | `CHANGELOG.md` (`1.23.11`) |

---

**Ende pro_run-Dokumentation (v1.1.0).**