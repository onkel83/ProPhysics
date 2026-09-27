# ProPhysics Build — Master-Wrapper

**Dateien:** `build\main\build.ps1` + `build\main\build.cmd`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Duenner Wrapper um `build\main\Makefile.nmake`, delegiert
BUILD_INFO an `write_build_info.ps1`.

---

## 1. Was das Skript tut

Ein Aufruf → mehrere Aktionen, die sonst einzeln noetig waeren:

1. **UTF-8-Konsole** einstellen (`[Console]::OutputEncoding`, `chcp 65001`)
2. **Git-Info** sammeln (optional, nur bei `-GitStamp`)
3. **nmake** mit dem aus Mode + Config + Flags abgeleiteten Target aufrufen
4. **Signieren** (optional, nur bei `-Sign`)
5. **BUILD_INFO.txt** ueber `write_build_info.ps1` schreiben —
   *ausser* das Makefile-Target hat das schon erledigt
6. **Zusammenfassung** ausgeben (Anzahl Dateien in `bin\` und `lib\`)

Der Aufruf ist idempotent: ein zweiter Lauf ohne Aenderungen ist schnell
(nmake baut nichts Neues) und schreibt BUILD_INFO neu.

**Empfohlener Aufruf:** ueber `pro_run build …` in `tools\`. Siehe
`docs\build\pro_run.md`. Direkte Aufrufe ueber `build.cmd` bleiben
gueltig und sind nuetzlich, wenn nur die Build-Komponente ohne
Dispatch-Schicht gebraucht wird.

---

## 2. Ablageort

```
<repo>\
├── build\
│   └── main\
│       ├── build.ps1        <- dieses Skript
│       ├── build.cmd        <- Wrapper fuer cmd.exe
│       ├── Makefile.nmake   <- Master-Build
│       └── write_build_info.ps1
├── tools\
│   └── pro_run.ps1 / .cmd   <- zentraler Einstiegspunkt
├── bin\
├── lib\
└── BUILD_INFO.txt           <- Ziel
```

Das Skript ermittelt den Repo-Root selbst:

```powershell
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
```

Aufruf funktioniert damit **aus jedem CWD**.

---

## 3. Aufruf

### 3.1 Ueber `pro_run` (empfohlen)

```cmd
:: kompletter Durchlauf inkl. Tests + Export
pro_run all

:: nur Build mit Debug-Config
pro_run build -Config debug -Rebuild

:: nur Kernel
pro_run build -Mode kernel

:: nur BUILD_INFO aktualisieren
pro_run build -Mode info
```

Details siehe `docs\build\pro_run.md`.

### 3.2 Ueber `build.cmd` (direkt)

```cmd
build.cmd
build.cmd -Mode sdk -Config debug -Rebuild
build.cmd -Mode kernel -Clean
build.cmd -Mode info -GitStamp
```

Der `.cmd`-Wrapper setzt `chcp 65001` und reicht alle Argumente an
PowerShell weiter.

### 3.3 Direkt ueber PowerShell

```powershell
.\build.ps1
.\build.ps1 -Mode sdk -Config release -Rebuild -GitStamp -GitNote "SDK v1.0.0"
```

### 3.4 Aus jedem anderen CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\build\main\build.cmd -Mode kernel
```

Funktioniert, weil alle Pfade ueber `$PSScriptRoot` bestimmt werden.

---

## 4. Modi — Auswahl der Komponenten

`-Mode` bestimmt, welche Komponente gebaut wird. Die Kette ist
kumulativ, weil SDK und Tests auf dem Kernel aufbauen.

| `-Mode` | Was gebaut wird | nmake-Target |
|---|---|---|
| `kernel` / `prophysics` | nur Kernel (DLL + LIB) | `prophysics` |
| `sdk` | Kernel + SDK-Interface | `sdk` |
| `test` | Kernel + SDK + 3 Test-EXEs | `test` |
| `all` (Default) | identisch mit `test` | `all` |
| `info` | nur BUILD_INFO.txt | `info` |

`kernel` ist Alias fuer `prophysics`. Beide Schreibweisen sind zulaessig;
`kernel` passt zur `pro_run`-Vokabular, `prophysics` ist die historische
Bezeichnung.

Der Unterschied zwischen `test` und `all` ist aktuell keiner — beide
rufen dieselbe Kette auf. `all` ist die sprachlich klarere Variante,
`test` betont, dass die Tests mitkommen.

**Kumulativ heisst:** `-Mode sdk` baut den Kernel mit, wenn er nicht
schon da ist. `-Mode kernel` baut **nicht** SDK und Tests.

---

## 5. Flags

### 5.1 Build-Steuerung

| Flag | Wirkung |
|---|---|
| *(kein Flag)* | inkrementeller Build (nur was sich geaendert hat) |
| `-Config release\|debug` | Build-Konfiguration (Default: `release`) |
| `-Rebuild` | `nmake clean` + kompletter Build |
| `-Clean` | nur `nmake clean` — kein Build, keine BUILD_INFO |
| `-NoBuild` | Build ueberspringen — nur BUILD_INFO.txt neu |

**`-Config`** wird als `CONFIG=<wert>` an alle nmake-Ebenen durchgereicht.
Die Sub-Makefiles kennen zwei Zustaende:

| Config | Kernel-Flags | Linker-Flags |
|---|---|---|
| `release` | `/O2 /Ob2 /Oi /GL /MP /arch:AVX2` | `/LTCG` |
| `debug` | `/Od /Zi /MDd /MP` | `/DEBUG` |

Ungueltige Werte brechen den Build mit `!ERROR` ab.

**`-Rebuild`** ist der Standard fuer Release-Laeufe: erst alles weg,
dann alles neu.

**`-Clean`** loescht `bin\`, `lib\` und `BUILD_INFO.txt`. Nach einem
`-Clean`-Lauf ist der Zustand wie bei einem frischen Checkout.

**`-NoBuild`** ist nuetzlich, wenn du nur die BUILD_INFO aktualisieren
willst (z.B. nach einem manuellen `nmake`-Lauf oder mit `-Mode info`).

### 5.2 Git-Metadaten

| Flag | Wirkung |
|---|---|
| `-GitStamp` | Git-Info (describe/branch/sha/status) in BUILD_INFO.txt |
| `-GitNote "text"` | Freitext-Notiz in BUILD_INFO.txt, unabhaengig von `-GitStamp` |

**`-GitStamp`** ruft im Repo-Root auf:

```
git describe --tags --dirty --always --long
git rev-parse --abbrev-ref HEAD
git rev-parse --short HEAD
git status --porcelain
```

Bei fehlendem git oder nicht-Repo: kein Fehler, nur Hinweiszeile.

**`-GitNote`** funktioniert ohne git. Mehrzeilig moeglich
(PowerShell-Here-String). Beispiel-Ausgabe:

```
Notiz:
  Release 1.0.0
  Kernel 1.23.0 / Etappe 23
  Alle 43 Tests gruen
```

### 5.3 Signieren

| Flag | Wirkung |
|---|---|
| `-Sign` | DLLs in `bin\` via `signtool` signieren |

Ruft fuer jede DLL in `bin\`:

```
signtool sign /a /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 <dll>
```

- `/a` — automatisches Zertifikat (Standard im User-Store)
- `/fd SHA256` — Datei-Hash
- `/tr` + `/td` — RFC-3161-Timestamp (DigiCert)

Fehlt `signtool.exe` im PATH: Warnung, Exit bleibt 0. Fehlende Signatur
ist kein Build-Fehler.

### 5.4 Dry-Run

| Flag | Wirkung |
|---|---|
| `-DryRun` | nur auflisten, nichts schreiben |

Unterdrueckt:
- `nmake`-Aufrufe (zeigt stattdessen `[dry] nmake …`)
- `signtool` (nur Hinweis)
- `write_build_info.ps1` (nur Hinweis)

Nuetzlich vor einem grossen Build, um zu sehen, welche Aktionen
stattfinden wuerden.

---

## 6. Beispiel-Aufrufe

### 6.1 Standard-Build (Kernel + SDK + Tests + Info)

```cmd
cd build\main
build.cmd
```

Ergebnis: `bin\` enthaelt alle DLLs und EXEs, `lib\` enthaelt beide Libs,
`BUILD_INFO.txt` im Repo-Root.

### 6.2 Nur Kernel, kompletter Rebuild

```cmd
build.cmd -Mode kernel -Rebuild
```

Ergebnis: `bin\ProPhysics.dll` und `lib\ProPhysics.lib` neu gebaut.
SDK- und Test-Artefakte bleiben unangetastet.

### 6.3 SDK-Build in Debug mit Git-Metadaten

```cmd
build.cmd -Mode sdk -Config debug -GitStamp -GitNote "SDK-Interface 1.0.0"
```

Ergebnis: Kernel + SDK-Interface in Debug. BUILD_INFO.txt enthaelt
`Config: debug`, Git-Block + Notiz.

### 6.4 Alles sauber weg

```cmd
build.cmd -Clean
```

Ergebnis: `bin\` und `lib\` sind leer, `BUILD_INFO.txt` entfernt.

### 6.5 Nur BUILD_INFO aktualisieren

```cmd
build.cmd -Mode info
:: oder
build.cmd -NoBuild
```

Ergebnis: kein nmake-Build, aber `BUILD_INFO.txt` wird neu geschrieben
(mit aktuellem Zeitstempel und Dateiliste).

### 6.6 Release-Build

```cmd
build.cmd -Mode all -Config release -Rebuild -GitStamp -Sign ^
          -GitNote "Release 1.0.0: Kernel 1.23.0 / Etappe 23."
```

Ergebnis: kompletter Neu-Build, Git-Metadaten, signierte DLLs,
BUILD_INFO mit Notiz.

### 6.7 Dry-Run vor Release

```cmd
build.cmd -Mode all -Rebuild -GitStamp -Sign -DryRun
```

Prueft in einem Durchgang: nmake-Target, signtool-Verfuegbarkeit,
Git-Erreichbarkeit. Kein Schreibzugriff.

---

## 7. Was in BUILD_INFO.txt landet

Das Skript delegiert das an `write_build_info.ps1`. Ausgabe-Beispiel
ohne Git/Notiz:

```
ProPhysics Build Information
============================

Erzeugt:  2026-09-27 16:15:03
Host:     DEV-WORKSTATION
User:     koehn
Version:  1.23.0
Etappe:   23
Config:   release

Artefakte:

  bin\
    ProPhysics.dll                   263.7 KB
    pro_sdk_interface.dll            138.2 KB
    example_alpha_test.exe           442.4 KB
    example_test_density.exe         177.7 KB
    example_test_tensor.exe          163.3 KB

  lib\
    ProPhysics.lib                    51.4 KB
    pro_sdk_interface.lib              2.4 KB
...
```

Mit `-GitStamp` und `-GitNote`:

```
...
Version:  1.23.0
Etappe:   23
Config:   release

Git:
  describe: v1.0.0-0-g1a2b3c4d
  branch:   main
  sha:      1a2b3c4d
  status:   clean

Notiz:
  Release 1.0.0: Kernel 1.23.0 / Etappe 23.

Artefakte:
...
```

Die `Etappe:`-Zeile wird aus `PROPHYSICS_ETAPPE` in
`ProPhysics_Version.h` gelesen. Die `Config:`-Zeile wird nur
geschrieben, wenn `-Config` uebergeben wurde.

Details zum Format siehe `docs\build\helper\write_build_info.md`.

---

## 8. Ausgabe auf der Konsole

```
============================================================
  ProPhysics Build  |  Modus: all  Config: release
============================================================
  Repo:   H:\ProPhysics_SDK\ProPhysics
  Build:  H:\ProPhysics_SDK\ProPhysics\build\main

[*] nmake all CONFIG=release
...
    OK  nmake all abgeschlossen.
[*] BUILD_INFO.txt bereits durch Makefile-Target 'all' erzeugt

------------------------------------------------------------
  Zusammenfassung
------------------------------------------------------------
  bin\:   2 DLL(s), 3 EXE(s)
  lib\:   2 LIB(s)
  Root:   BUILD_INFO.txt

```

Farbcodierung:

| Farbe | Bedeutung |
|---|---|
| Cyan | Schritt-Markierung |
| Gruen | Erfolg |
| Gelb | Warnung (nicht kritisch) |
| Rot | Fehler |
| Grau | Dry-Run-Detail |

---

## 9. Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | Erfolg |
| `2` | `nmake` nicht im PATH |
| `≠0` | `nmake`-Exit-Code (Build-Fehler) |

Bei Fehler in `nmake` wird die Ausgabe nicht unterdrueckt — der Nutzer
sieht die volle Compiler-/Linker-Meldung.

---

## 10. Interner Ablauf

```
build.ps1
    │
    ├─ UTF-8 einstellen (Konsole + chcp 65001)
    │
    ├─ -GitStamp?  ─→ Git-Vorschau ausgeben (nur Info)
    │
    ├─ Mode = info?
    │   └─ ja: nur write_build_info.ps1 aufrufen, kein nmake
    │
    ├─ -Clean und nicht -Rebuild?
    │   ├─ ja:  nur nmake clean_<scope>, dann Info ueberspringen
    │   └─ nein: weiter
    │
    ├─ -NoBuild?
    │   ├─ ja:  Build ueberspringen
    │   └─ nein: nmake <target> CONFIG=<config> aufrufen
    │
    ├─ -Sign und nicht skipBuild?
    │   └─ signtool ueber bin\*.dll
    │
    ├─ BUILD_INFO:
    │   │   Wenn Target in {all, rebuild, rebuild_*} → Makefile hat
    │   │     info schon ausgefuehrt, kein zweiter Aufruf.
    │   │   Sonst (prophysics, sdk, test) → write_build_info.ps1 aufrufen.
    │   └─ write_build_info.ps1 mit -RepoRoot, -Config,
    │      optional -GitStamp, -GitNote
    │
    └─ Zusammenfassung ausgeben
```

**Drei Sonderfaelle:**

1. `-Clean` ohne `-Rebuild`: nur Clean, keine BUILD_INFO
2. `-NoBuild`: kein nmake, aber BUILD_INFO wird geschrieben
3. `-Mode info`: nur BUILD_INFO, kein nmake

**Doppelaufruf-Vermeidung:** Der Master-Makefile ruft in den Targets
`all`, `rebuild`, `rebuild_prophysics`, `rebuild_sdk`, `rebuild_test`
jeweils den `info:`-Target selbst auf. `build.ps1` erkennt das und
ruft `write_build_info.ps1` in diesen Faellen **nicht** erneut auf.

---

## 11. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausfuehren |
| `CONFIG muss "release" oder "debug" sein` | Tippfehler bei `-Config` | Gross-/Kleinschreibung pruefen |
| `write_build_info.ps1 nicht gefunden` | Skript umbenannt/geloescht | Datei in `build\main\` pruefen |
| `BUILD_INFO.txt fehlt` nach Lauf | `-Clean` gesetzt | ohne `-Clean` neu laufen |
| `signtool.exe nicht gefunden` | Windows SDK fehlt | Warnung ist normal, Exit bleibt 0 |
| Git-Vorschau zeigt `nicht verfuegbar` | kein Repo / kein git | `-GitStamp` weglassen |
| Umlaute kaputt in Konsole | PowerShell umging `chcp` | `build.cmd` statt `build.ps1` verwenden |
| nmake-Target unbekannt | `-Mode`-Wert falsch | `ValidateSet` in `build.ps1` zeigt erlaubte Werte |
| BUILD_INFO.txt erscheint doppelt geschrieben | Bug vor v1.0.0 | Update auf aktuelle `build.ps1` |

---

## 12. Was dieses Skript nicht tut

- **Keine Direktaufrufe der Sub-Makefiles.** Es nutzt ausschliesslich
  `build\main\Makefile.nmake` und dessen Targets.
- **Kein Paketieren.** Artefakte bleiben in `bin\` und `lib\`. Fuer
  Export-Pakete siehe `export.ps1`.
- **Kein Zip.** Keine Archivierung im Build-Schritt. ZIP-Erzeugung
  liegt in `export.ps1`.
- **Kein Deployment.** Kein Push in Repos oder Verzeichnisse.
- **Keine Test-Ausfuehrung.** Die Tests laufen ueber
  `tools\run_alpha_tests.ps1` oder `pro_run test`.
- **Kein Rebuild der `.cmd`-Wrapper.** Die sind statisch.

---

## 13. Parameter-Referenz (kompakt)

```
build.cmd [-Mode <kernel|prophysics|sdk|test|all|info>]
          [-Config <release|debug>]
          [-Rebuild] [-Clean] [-NoBuild]
          [-GitStamp] [-GitNote "text"]
          [-Sign] [-DryRun]
```

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Mode` | Choice | `all` | Was gebaut wird (`kernel` = `prophysics`) |
| `-Config` | Choice | `release` | Build-Konfiguration |
| `-Rebuild` | Switch | aus | clean + Build |
| `-Clean` | Switch | aus | nur clean |
| `-NoBuild` | Switch | aus | nur BUILD_INFO |
| `-GitStamp` | Switch | aus | Git-Metadaten in BUILD_INFO |
| `-GitNote` | String | `''` | Freitext in BUILD_INFO |
| `-Sign` | Switch | aus | DLLs signieren |
| `-DryRun` | Switch | aus | nur auflisten |

---

## 14. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des gesamten Build-Systems
- `docs\build\helper\export.md` — Export-Skript
- `docs\build\helper\write_build_info.md` — BUILD_INFO.txt-Format
- `docs\test\run_alpha_tests.md` — Test-Runner
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\sdk\Makefile.md` — SDK-Build
- `docs\build\test\Makefile.md` — Test-Build

---

**Ende Build-Wrapper-Dokumentation (v1.0.0).**