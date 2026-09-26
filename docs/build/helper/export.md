# ProPhysics Export — Artefakt-Export mit Dokumentation

**Dateien:** `build\main\export.ps1` + `build\main\export.cmd`
**Version:** 3.1 (Etappe 21)
**Zweck:** Baut (optional) via Master-Makefile und kopiert die
Artefakte plus passende Dokumentation in eine saubere Zielstruktur.

---

## 1. Was das Skript tut

Vier Aufgaben in einem Aufruf:

1. **Optional Build** — ruft `build\main\Makefile.nmake` mit dem
   passenden Target auf.
2. **Artefakte kopieren** — legt DLLs, LIBs, EXEs, Header in eine
   Zielstruktur unter `out\`.
3. **Dokumentation kopieren** — kopiert die für den Modus passenden
   MD-Dateien nach `<Paket>\docs\`.
4. **`README.md` + `BUILD_INFO.txt`** — erzeugt ein paket-spezifisches
   README und legt die Build-Metadaten bei.

Der Export-Modus (`exe` / `sdk` / `src`) bestimmt die Zielstruktur. Der
Scope (`prophysics` / `sdk` / `test` / `all`) bestimmt, welche
Komponenten einbezogen werden.

---

## 2. Ablageort

```
H:\ProPhysics_SDK\ProPhysics\
├── build\
│   └── main\
│       ├── export.ps1       <- dieses Skript
│       ├── export.cmd       <- Wrapper für cmd.exe
│       └── Makefile.nmake   <- Master-Build
├── bin\                     <- Quelle für exe/sdk-Export
├── lib\                     <- Quelle für sdk-Export
├── src\                     <- Quelle für src-Export
│   ├── prophysics\
│   ├── sdk\
│   └── test\
├── docs\                    <- Quelle für den Doku-Export
│   ├── build\
│   ├── project\
│   └── test\
├── BUILD_INFO.txt           <- Kopie landet im Paket
└── out\                     <- Ziel (Default)
    ├── exe\
    ├── sdk\
    └── src\<name>\
```

Der Repo-Root wird über `$PSScriptRoot\..\..` ermittelt. Aufruf
funktioniert aus jedem CWD.

---

## 3. Aufruf

### 3.1 Über `export.cmd`

```cmd
export.cmd exe
export.cmd sdk -Scope sdk
export.cmd src -Name v3.0-etappe21
```

### 3.2 Direkt über PowerShell

```powershell
.\export.ps1 exe
.\export.ps1 sdk -Rebuild -Clean
.\export.ps1 src -Scope all
```

### 3.3 Aus jedem CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\build\main\export.cmd sdk
```

---

## 4. Die drei Modi

### 4.1 `exe` — Runtime-Paket

**Zweck:** Weitergabe an Anwender, die nur die Tests ausführen wollen.

**Ziel:** `out\exe\` (oder `-OutRoot <pfad>\exe\`)

**Struktur:**

```
out\exe\
├── README.md                    ← generiert
├── BUILD_INFO.txt               ← Kopie aus Root
├── ProPhysics.dll
├── pro_sdk_interface.dll        (nur bei Scope ≥ sdk)
├── example_alpha_test.exe       (nur bei Scope ≥ test)
├── example_test_density.exe     (nur bei Scope ≥ test)
├── example_test_tensor.exe      (nur bei Scope ≥ test)
├── run_alpha_tests.cmd          (nur bei Scope ≥ test)
├── run_alpha_tests.ps1          (nur bei Scope ≥ test)
└── docs\
    ├── Project.md               ← docs\project\Project.md
    ├── ProPhysics_Testkatalog.md
    └── run_alpha_tests.md
```

**Enthält nicht:** Header, LIB-Dateien, Test-Quellcode, Python.

**Build:** Ruft `nmake <target>` im Master auf, wo `<target>` aus
`Scope` abgeleitet wird.

### 4.2 `sdk` — SDK-Paket

**Zweck:** Weitergabe an externe Entwickler, die gegen den Kernel
programmieren wollen.

**Ziel:** `out\sdk\` (oder `-OutRoot <pfad>\sdk\`)

**Struktur:**

```
out\sdk\
├── README.md                    ← generiert
├── BUILD_INFO.txt               ← Kopie aus Root
└── libs\
    ├── ProPhysics.dll
    ├── ProPhysics.lib
    ├── pro_sdk_interface.dll          (nur bei Scope ≥ sdk)
    ├── pro_sdk_interface.lib          (nur bei Scope ≥ sdk)
    ├── src\
    │   └── header\
    │       ├── prophysics\
    │       │   ├── ProPhysics.h
    │       │   ├── ProPhysics_Types.h
    │       │   ├── ProPhysics_Config.h
    │       │   ├── ProPhysics_Exports.h
    │       │   ├── ProPhysics_Version.h
    │       │   └── ProPhysics_Internal.h
    │       └── sdk\
    │           └── pro_sdk_interface.h
    └── docs\
        ├── Project.md
        ├── ProPhysics_Testkatalog.md
        ├── BUILD_SCRIPT.md
        ├── prophysics-Makefile.md
        └── sdk-Makefile.md
```

**Header-Layout:** Header werden in Unterordnern (`prophysics\`, `sdk\`)
gespiegelt, damit `#include`-Pfade konsistent bleiben:

```c
#include "prophysics/ProPhysics.h"
#include "sdk/pro_sdk_interface.h"
```

**Enthält nicht:** Test-Quellcode, EXE-Dateien, Python.

### 4.3 `src` — Quellcode-Snapshot

**Zweck:** Weitergabe an externe Tester oder Archivierung eines
bestimmten Etappen-Stands.

**Ziel:** `out\src\<Name>\` — `<Name>` default = Version aus
`ProPhysics_Version.h` (z.B. `v3.0.0`).

**Struktur:**

```
out\src\v3.0.0\
├── README.md                    ← generiert
├── BUILD_INFO.txt               ← Kopie aus Root
├── prophysics\
│   ├── ProPhysics_Core.c
│   ├── ProPhysics_Amp.c
│   ├── ProPhysics_Dirac.c
│   ├── ... (11 Module)
│   └── header\
│       ├── ProPhysics.h
│       ├── ProPhysics_Internal.h
│       └── ... (6 Header)
├── sdk\                              (nur bei Scope ≥ sdk)
│   ├── pro_sdk_interface.c
│   └── header\
│       └── pro_sdk_interface.h
├── test\                             (nur bei Scope ≥ test)
│   ├── alpha_test_main.c
│   ├── alpha_test_dirac.c
│   ├── ... (15 Module)
│   └── header\
│       └── alpha_test_common.h
└── docs\                             ← vollständige Doku-Mirror
    ├── build\
    │   ├── BUILD_SCRIPT.md
    │   ├── main\Makefile.md
    │   ├── prophysics\Makefile.md
    │   ├── sdk\Makefile.md
    │   ├── test\Makefile.md
    │   └── helper\
    │       ├── build.md
    │       ├── export.md
    │       └── write_build_info.md
    ├── project\
    │   └── Project.md
    └── test\
        ├── ProPhysics_Testkatalog.md
        └── run_alpha_tests.md
```

**Besonderheit:** Der `src`-Modus kopiert **nur** — er baut nicht. Das
ist Absicht: ein Quellcode-Export ist unabhängig vom Build-Zustand.

**Nicht kopiert:** `ProPhysics.legacy` (falls vorhanden), `_obj\`,
`.vs\`, `.git\`.

---

## 5. Dokumenten-Export — was wohin

### 5.1 Mapping pro Modus

| Datei in `docs\` | `exe` | `sdk` | `src` |
|---|:-:|:-:|:-:|
| `project\Project.md` | ✅ als `docs\Project.md` | ✅ als `docs\Project.md` | ✅ volle Mirror |
| `test\ProPhysics_Testkatalog.md` | ✅ | ✅ | ✅ |
| `test\run_alpha_tests.md` | ✅ | – | ✅ |
| `build\BUILD_SCRIPT.md` | – | ✅ | ✅ |
| `build\prophysics\Makefile.md` | – | ✅ als `docs\prophysics-Makefile.md` | ✅ volle Struktur |
| `build\sdk\Makefile.md` | – | ✅ als `docs\sdk-Makefile.md` | ✅ |
| `build\test\Makefile.md` | – | – | ✅ |
| `build\main\Makefile.md` | – | – | ✅ |
| `build\helper\*.md` | – | – | ✅ |

**Rationale:**

- **`exe`:** Anwender braucht Test-Doku (welche Prios? wie aufrufen?)
  und Projekt-Übersicht. Keine Build-Interna.
- **`sdk`:** Entwickler braucht Build-Doku (Makefile-Details für
  Kernel + SDK) plus Testkatalog (was der Kernel kann) plus
  Projekt-Übersicht.
- **`src`:** Vollständige Mirror. Wer den Code hat, soll auch alle
  Docs haben — insbesondere die Helper- und Master-Makefile-Doku.

### 5.2 Was passiert, wenn eine Datei fehlt

Der `Copy-Docs`-Helper prüft jede Quelldatei. Fehlt eine, wird sie
mit `skip: <pfad>` gemeldet und die anderen werden trotzdem kopiert.
Der Export bricht **nicht** ab.

Beispiel:

```
[*] Export exe -> H:\...\out\exe
    OK  README.md
    OK  BUILD_INFO.txt
    --  skip: build\main\Makefile.md
    OK  Docs (2 Dateien)
```

Fehlt `docs\` komplett, kommt nur:

```
    --  docs\ nicht gefunden — kein Dokumenten-Export
```

---

## 6. README.md — paket-spezifischer Inhalt

Jedes Paket bekommt eine generierte `README.md` im Wurzelverzeichnis.
Der Inhalt hängt vom Modus ab.

### 6.1 `exe`-README

Enthält:

- **Header** — Version, Zeitstempel, Host, Scope.
- **Paketinhalt** — Tabelle aller Dateien mit Rolle.
- **Schnellstart** — `run_alpha_tests.cmd -Prio all` und Einzeltest-Beispiel.
- **Prios-Tabelle** — alle 7 Prios mit Test-Anzahl und Dauer.
- **Doku-Verweise** — Liste der mitgelieferten MD-Dateien.

### 6.2 `sdk`-README

Enthält:

- **Header** — Version, Zeitstempel, Host, Scope.
- **Paketinhalt** — Ordnerstruktur mit Erklärung.
- **Einbindung** — Include-Pfade, Library-Pfade, Link-Reihenfolge
  (`pro_sdk_interface.lib` **vor** `ProPhysics.lib`), MSVC-Beispiel.
- **DLL-Weitergabe** — welche DLLs neben der EXE liegen müssen.
- **Doku-Verweise**.

### 6.3 `src`-README

Enthält:

- **Header** — Version, Zeitstempel, Host, Scope, Paket-Name.
- **Paketinhalt** — Ordnerstruktur.
- **Build-Voraussetzungen** — VS-Developer-Prompt, Windows SDK.
- **Build-Anleitung** — `build.cmd`, `nmake`-Direktaufruf, Tests.
- **Doku-Verweise** — Verweis auf `docs\`.

### 6.4 Beispiel-Ausgabe (`exe`)

```markdown
# ProPhysics Package — EXE

**Version:** v3.0.0
**Erzeugt:** 2026-09-24 17:42:11 auf DEV-WORKSTATION
**Scope:** all

## Was ist in diesem Paket?

Ein lauffaehiges Runtime-Paket der ProPhysics-Alpha-Tests.
Alle DLLs und EXEs liegen **flach nebeneinander**, so
dass Windows die DLLs beim Start der EXEs automatisch findet.

| Datei | Rolle |
|---|---|
| `ProPhysics.dll` | Kernel-Bibliothek |
| `pro_sdk_interface.dll` | SDK-Interface |
| `example_alpha_test.exe` | Alpha-Test-Suite (41 Tests) |
| `example_test_density.exe` | Dichte-Regression (86 Checks) |
| `example_test_tensor.exe` | Tensor-/Fock-Regression (61 Checks) |
| `run_alpha_tests.cmd` / `.ps1` | Test-Runner |

## Schnellstart

```cmd
run_alpha_tests.cmd -Prio all
```

## Prioritaeten

| Prio | Thema | Tests | Typische Dauer |
|:-:|---|---:|---:|
| 1 | 2D-Basis | 12 | ~2 s |
| 2 | Emergenz | 10 | ~4,5 min |
| 3 | Langlauf | 10 | ~45 s |
| 4 | 3D-Torus | 3 | ~45 s |
| 5 | Hydrogen + Shared-Ref | 4 | ~36 min |
| 6 | Spin-1/2 | 1 | < 1 s |
| 7 | Dirac | 1 | ~15 s |
| `all` | alle | 41 | ~43 min |

## Dokumentation

| Datei | Inhalt |
|---|---|
| `docs\run_alpha_tests.md` | Test-Runner-Bedienung, Prios, Logs |
| `docs\ProPhysics_Testkatalog.md` | alle 41 Tests mit Kriterien |
| `docs\Project.md` | Ontologie und Roadmap |
| `BUILD_INFO.txt` | Version, Artefakt-Liste, Zeitstempel |
```

---

## 7. Parameter

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Mode` | `exe` \| `sdk` \| `src` | *(mandatory)* | Export-Modus |
| `-Scope` | `prophysics` \| `sdk` \| `test` \| `all` | `all` | Was einbezogen wird |
| `-OutRoot` | Pfad | `<repo>\out` | Wurzelverzeichnis |
| `-Name` | String | Version aus `ProPhysics_Version.h` | Nur bei `-Mode src`: Unterordner-Name |
| `-NoBuild` | Switch | aus | nmake überspringen |
| `-Rebuild` | Switch | aus | nmake clean + Build vor Export |
| `-Clean` | Switch | aus | Zielordner vorher rekursiv löschen |
| `-DryRun` | Switch | aus | nur auflisten, nichts kopieren |

**Hinweise:**

- `-Name` wird nur im `src`-Modus ausgewertet. In `exe`- und
  `sdk`-Modus ist der Zielordner immer `out\exe\` bzw. `out\sdk\`.
- `-NoBuild` funktioniert in `exe`- und `sdk`-Modus. In `src`-Modus
  ist es wirkungslos, weil dort ohnehin nicht gebaut wird.

---

## 8. Scope × Mode — Auswirkung

| Scope | `exe` | `sdk` | `src` |
|---|---|---|---|
| `prophysics` | nur `ProPhysics.dll` | Kernel (DLL + LIB + Header) | nur `prophysics\` |
| `sdk` | + `pro_sdk_interface.dll` | + SDK (DLL + LIB + Header) | + `sdk\` |
| `test` | + 3 EXEs + Runner | + SDK-Header | + `test\` |
| `all` | identisch mit `test` | identisch mit `test` | alle drei |

**Kumulativ:** `-Scope sdk` schließt `prophysics` ein. `-Scope test`
schließt `sdk` und `prophysics` ein. Der Scope nach oben ist immer
vollständig.

**Der Scope beeinflusst die Docs nicht.** Die Dokumenten-Auswahl hängt
nur vom Modus ab.

---

## 9. Beispiel-Aufrufe

### 9.1 Komplettes Runtime-Paket

```cmd
export.cmd exe
```

Ergebnis: `nmake all` läuft, dann alle DLLs und EXEs nach `out\exe\`,
plus `README.md`, `BUILD_INFO.txt`, `docs\`.

### 9.2 SDK-Paket aus bereits gebauten Artefakten

```cmd
export.cmd sdk -NoBuild
```

Ergebnis: kein nmake-Aufruf, nur Kopieren. Voraussetzung: `bin\` und
`lib\` enthalten die aktuellen DLLs und Libs.

### 9.3 SDK-Paket mit Rebuild

```cmd
export.cmd sdk -Scope all -Rebuild -Clean
```

Ergebnis: kompletter Neu-Build, Zielordner geleert, SDK-Layout erstellt.

### 9.4 Quellcode-Export mit eigenem Namen

```cmd
export.cmd src -Scope sdk -Name v3.0-etappe21
```

Ergebnis: `out\src\v3.0-etappe21\` mit `prophysics\`, `sdk\`, `docs\`.

### 9.5 Quellcode-Export mit automatischem Namen

```cmd
export.cmd src
```

Ergebnis: `out\src\v3.0.0\` — Version aus `ProPhysics_Version.h`.

### 9.6 In ein anderes Zielverzeichnis

```cmd
export.cmd exe -OutRoot D:\dist
```

Ergebnis: `D:\dist\exe\`.

### 9.7 Dry-Run

```cmd
export.cmd sdk -Rebuild -DryRun
```

Ergebnis: zeigt, welches nmake-Target aufgerufen würde und welche
Dateien kopiert würden — ohne Schreibzugriff.

---

## 10. Zielverzeichnis-Bestimmung

| `-Mode` | `-OutRoot` (Default `<repo>\out`) | Ergebnis |
|---|---|---|
| `exe` | `<repo>\out` | `<repo>\out\exe\` |
| `sdk` | `<repo>\out` | `<repo>\out\sdk\` |
| `src` | `<repo>\out` | `<repo>\out\src\<Name>\` |
| `exe` | `D:\dist` | `D:\dist\exe\` |
| `sdk` | `D:\dist` | `D:\dist\sdk\` |
| `src` | `D:\dist` | `D:\dist\src\<Name>\` |

Der Zielordner wird **angelegt**, falls nicht vorhanden. Mit `-Clean`
wird er vorher rekursiv geleert.

---

## 11. Interner Ablauf

```
export.ps1
    │
    ├─ UTF-8 einstellen
    │
    ├─ -Name leer und -Mode src? → Version aus Header lesen
    │
    ├─ Zielordner bestimmen (out\exe\ | out\sdk\ | out\src\<name>\)
    │
    ├─ -Mode src?
    │   ├─ ja:  kein Build, direkt zu Export
    │   └─ nein: weiter
    │
    ├─ nmake-Target aus Scope + -Rebuild ableiten
    │   ├─ -Rebuild gesetzt → rebuild_<scope>
    │   └─ sonst → <scope> (prophysics | sdk | test | all)
    │
    ├─ nmake aufrufen (falls nicht -NoBuild)
    │
    ├─ Export-Funktion je nach Mode:
    │   ├─ exe: DLLs + EXEs flach
    │   │       + Copy-BuildInfo
    │   │       + Copy-Docs (exe)
    │   │       + New-PackageReadme (exe)
    │   ├─ sdk: libs\ + libs\src\header\
    │   │       + Copy-BuildInfo
    │   │       + Copy-Docs (sdk) → libs\docs\
    │   │       + New-PackageReadme (sdk)
    │   └─ src: Copy-Tree von src\<gebiet>\
    │           + Copy-BuildInfo
    │           + Copy-Docs (src) → docs\ (volle Mirror)
    │           + New-PackageReadme (src)
    │
    └─ Zusammenfassung (Anzahl Dateien im Zielordner)
```

---

## 12. Was exportiert wird — im Detail

### 12.1 `exe`-Modus

| Quelle | Ziel | Bedingung |
|---|---|---|
| `bin\ProPhysics.dll` | `out\exe\` | immer |
| `bin\pro_sdk_interface.dll` | `out\exe\` | Scope ≥ sdk |
| `bin\example_*.exe` | `out\exe\` | Scope ≥ test |
| `bin\run_alpha_tests.cmd` | `out\exe\` | Scope ≥ test |
| `bin\run_alpha_tests.ps1` | `out\exe\` | Scope ≥ test |
| `BUILD_INFO.txt` | `out\exe\` | immer (falls vorhanden) |
| `docs\project\Project.md` | `out\exe\docs\` | immer |
| `docs\test\ProPhysics_Testkatalog.md` | `out\exe\docs\` | immer |
| `docs\test\run_alpha_tests.md` | `out\exe\docs\` | immer |
| generiert | `out\exe\README.md` | immer |

### 12.2 `sdk`-Modus

| Quelle | Ziel | Bedingung |
|---|---|---|
| `bin\ProPhysics.dll` | `out\sdk\libs\` | immer |
| `lib\ProPhysics.lib` | `out\sdk\libs\` | immer |
| `bin\pro_sdk_interface.dll` | `out\sdk\libs\` | Scope ≥ sdk |
| `lib\pro_sdk_interface.lib` | `out\sdk\libs\` | Scope ≥ sdk |
| `src\prophysics\header\*.h` | `out\sdk\libs\src\header\prophysics\` | immer |
| `src\sdk\header\*.h` | `out\sdk\libs\src\header\sdk\` | Scope ≥ sdk |
| `BUILD_INFO.txt` | `out\sdk\` | immer (falls vorhanden) |
| `docs\project\Project.md` | `out\sdk\libs\docs\` | immer |
| `docs\test\ProPhysics_Testkatalog.md` | `out\sdk\libs\docs\` | immer |
| `docs\build\BUILD_SCRIPT.md` | `out\sdk\libs\docs\` | immer |
| `docs\build\prophysics\Makefile.md` | `out\sdk\libs\docs\prophysics-Makefile.md` | immer |
| `docs\build\sdk\Makefile.md` | `out\sdk\libs\docs\sdk-Makefile.md` | immer |
| generiert | `out\sdk\README.md` | immer |

### 12.3 `src`-Modus

| Quelle | Ziel | Bedingung |
|---|---|---|
| `src\prophysics\` (rekursiv) | `out\src\<name>\prophysics\` | immer |
| `src\sdk\` (rekursiv) | `out\src\<name>\sdk\` | Scope ≥ sdk |
| `src\test\` (rekursiv) | `out\src\<name>\test\` | Scope ≥ test |
| `BUILD_INFO.txt` | `out\src\<name>\` | immer |
| `docs\` (rekursiv) | `out\src\<name>\docs\` | immer |
| generiert | `out\src\<name>\README.md` | immer |

`Copy-Tree` spiegelt die **komplette** Quellstruktur. `.c`-Dateien
landen direkt im Zielordner, `.h`-Dateien in `header\`.

---

## 13. Konsolenausgabe

### 13.1 Start

```
============================================================
  ProPhysics Export  |  Mode: sdk  Scope: all
============================================================
  Repo:    H:\ProPhysics_SDK\ProPhysics
  Ziel:    H:\ProPhysics_SDK\ProPhysics\out\sdk
```

### 13.2 Während des Exports

```
[*] Baue (all)...
...
    OK  Build abgeschlossen.
[*] Export sdk -> H:\...\out\sdk
    OK  BUILD_INFO.txt
    OK  Docs (5 Dateien)
    OK  README.md
    OK  Fertig.
```

### 13.3 Zusammenfassung

```
------------------------------------------------------------
  Zusammenfassung
------------------------------------------------------------
  14 Datei(en) in H:\...\out\sdk
```

Farbcodierung wie bei `build.ps1`:

| Farbe | Bedeutung |
|---|---|
| Cyan | Schritt-Markierung |
| Grün | Erfolg |
| Gelb | Warnung (nicht kritisch) |
| Rot | Fehler |
| Grau | Dry-Run-Detail |

---

## 14. Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | Erfolg |
| `2` | `nmake` nicht im PATH (nur bei Build-Modi) |
| `≠0` | `nmake`-Exit-Code |

Im `src`-Modus gibt es keinen `nmake`-Aufruf, also auch keinen
`2`-Exit-Code.

Fehlende Quelldateien (Docs, Header) brechen nicht ab — sie werden
mit `skip:` geloggt, und der Export läuft weiter.

---

## 15. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausführen |
| `Quelle fehlt: ...\bin` | Kernel nicht gebaut | ohne `-NoBuild` neu laufen |
| `Quelle fehlt: ...\lib` | Kernel + SDK nicht gebaut | `nmake` in `build\prophysics` und `build\sdk` |
| `example_*.exe` fehlen im Ziel | Scope zu niedrig | `-Scope test` oder `-Scope all` |
| `pro_sdk_interface.dll` fehlt | Scope zu niedrig | `-Scope sdk` oder höher |
| `docs\ nicht gefunden` | Doku nie angelegt | `docs\project\`, `docs\test\` prüfen |
| `skip: build\main\Makefile.md` | Doc-Datei fehlt | Doku nachliefern |
| `BUILD_INFO.txt fehlt` | Build nie gelaufen | `build.cmd` laufen lassen |
| Umlaute in Ausgabe kaputt | PowerShell umging `chcp` | `export.cmd` statt `export.ps1` |
| `-Name` wirkungslos | falscher Mode | `-Name` wirkt nur bei `-Mode src` |
| `ProPhysics.legacy` im src-Export | Datei liegt in `src\prophysics\` | manuell löschen oder Filter ergänzen |

---

## 16. Was dieses Skript nicht tut

- **Kein Zip.** Die Zielordner werden nicht archiviert.
- **Kein Signing.** DLLs bleiben unsigniert — für Signatur
  `build.ps1 -Sign` verwenden, dann exportieren.
- **Kein Git-Push.** Kein Upload irgendwohin.
- **Kein Test-Lauf.** Der Test-Runner bleibt separat.
- **Kein Diff.** Es wird blind kopiert, kein Vergleich mit vorherigem
  Export.
- **Kein Docs-Rendering.** Markdown bleibt Markdown, keine PDF/HTML-
  Konvertierung.
- **Kein Lizenz-File.** Falls eine LICENSE im Root liegt, wird sie
  **nicht** automatisch mitkopiert.

---

## 17. Parameter-Referenz (kompakt)

```
export.cmd <exe|sdk|src>
           [-Scope <prophysics|sdk|test|all>]
           [-OutRoot <pfad>] [-Name <string>]
           [-NoBuild] [-Rebuild] [-Clean] [-DryRun]
```

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Mode` | Choice | *(mandatory)* | `exe` \| `sdk` \| `src` |
| `-Scope` | Choice | `all` | Was einbezogen wird |
| `-OutRoot` | Pfad | `<repo>\out` | Wurzelverzeichnis |
| `-Name` | String | Version | Nur `-Mode src` |
| `-NoBuild` | Switch | aus | nmake überspringen |
| `-Rebuild` | Switch | aus | clean + Build |
| `-Clean` | Switch | aus | Zielordner leeren |
| `-DryRun` | Switch | aus | nur auflisten |

---

## 18. Beispiele für typische Szenarien

### 18.1 Release-Paket für Anwender

```cmd
export.cmd exe -Scope all -Rebuild -Clean
```

Liefert `out\exe\` mit:

- Allem, was zum Ausführen der Tests nötig ist.
- `README.md` mit Schnellstart und Prios-Tabelle.
- `docs\` mit Testkatalog und Runner-Doku.
- `BUILD_INFO.txt` als Versionsnachweis.

Anwender bekommt einen Ordner, kopiert ihn irgendwohin, führt
`run_alpha_tests.cmd` aus. Fertig.

### 18.2 SDK an externen Entwickler

```cmd
export.cmd sdk -Scope sdk -Rebuild -Clean -OutRoot D:\dist
```

Liefert `D:\dist\sdk\` mit:

- `libs\` mit Header, LIBs und DLLs.
- `libs\docs\` mit Build-Doku.
- `README.md` mit Einbindungs-Anleitung (Include-Pfade, Link-Reihenfolge).

Der Empfänger kann gegen `pro_sdk_interface.lib` linken und die Header
unter `libs\src\header\prophysics\` und `libs\src\header\sdk\` finden.

### 18.3 Quellcode-Snapshot einer Etappe

```cmd
export.cmd src -Scope all -Name v3.0-etappe21
```

Liefert `out\src\v3.0-etappe21\` mit:

- Komplettem Quellcode.
- Voller Doku-Mirror unter `docs\`.
- `README.md` mit Build-Anleitung.

Nützlich für Archivierung, Reviews, oder Weitergabe an Testpersonen.

### 18.4 CI-Export

```cmd
export.cmd sdk -Scope all -NoBuild -Clean -OutRoot %CI_ARTIFACT_DIR%
```

Im CI ist der Build schon gelaufen. Der Export kopiert nur die
Artefakte plus Docs ins Artefakt-Verzeichnis.

---

## 19. Siehe auch

- `docs\build\BUILD_SCRIPT.md` — Übersicht des Build-Systems
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\write_build_info.md` — BUILD_INFO.txt
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\sdk\Makefile.md` — SDK-Build
- `docs\build\test\Makefile.md` — Test-Build
- `docs\test\run_alpha_tests.md` — Test-Runner
- `docs\test\ProPhysics_Testkatalog.md` — Test-Übersicht

---

**Ende Export-Wrapper-Dokumentation.**