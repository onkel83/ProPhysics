# ProPhysics Build & Package — Übersicht

**Stand:** 2026-09-28 (Etappe 23, Kernel 1.23.0, ProWB + CI-Doku)
**Zweck:** Einstiegspunkt in das Build-System. Erklärt die Struktur,
die Komponenten und ihre Wechselwirkungen. Verweist auf die
Detail-Dokumente.

---

## 1. Was das Build-System tut

Das Build-System erzeugt aus dem Quellcode unter `src\` vier Artefakte:

| Artefakt | Pfad | Quelle |
|---|---|---|
| Kernel-DLL + Import-Lib | `bin\ProPhysics.dll`, `lib\ProPhysics.lib` | `src\prophysics\` |
| SDK-Interface | `bin\pro_sdk_interface.dll`, `lib\pro_sdk_interface.lib` | `src\sdk\` |
| Test-EXEs | `bin\example_alpha_test.exe` + 2 weitere | `src\test\` |
| ProWB-Builder | `bin\prowb\prowb.exe` | `src\prowb\` |

Zusätzlich:
- **Web-Docs** — ProWB rendert die MD-Doku aus `docs\` in ein
  statisches Portal (`out\web\index.html`).
- **Test-Runner** in `tools\` — startet die Alpha-Suite.
- **Export-Funktionen** — kopieren Artefakte in `out\` und packen sie
  als ZIP-Archive (`prophysics-<kind>-<version>.zip`).
- **BUILD_INFO.txt** — Metadaten im Projekt-Root.
- **Einheitlicher Einstiegspunkt `pro_run`** — dispatcht auf die
  bestehenden Build-, Export-, Test- und Web-Skripte.
- **CI-Workflows** in `.github\workflows\` — Standard-CI,
  Web-Docs-Deploy, Alpha-Nightly.

Die Build-Kette für den **Kernel-Strang** ist strikt sequenziell:

```
prophysics → sdk → test
```

Der SDK-Build linkt gegen `ProPhysics.lib`, der Test-Build gegen beide
Import-Libs. Kettenabhängigkeiten werden vom Master-Makefile erzwungen.

Der **ProWB-Build** hängt **nicht** von dieser Kette ab. Er läuft im
Master-`all` **nach** `test`, ist aber eigenständig baubar
(`pro_run web`).

---

## 2. Ordner-Layout

```
ProPhysics\
├── BUILD_INFO.txt              # wird bei jedem Build erzeugt
│
├── .github\
│   └── workflows\              # CI-Workflows (GitHub Actions)
│       ├── ci.yml
│       ├── web-docs.yml
│       └── alpha-nightly.yml
│
├── bin\                        # alle DLLs + EXEs (flach)
│   ├── ProPhysics.dll
│   ├── pro_sdk_interface.dll
│   ├── example_alpha_test.exe
│   ├── example_test_density.exe
│   ├── example_test_tensor.exe
│   ├── logs\                   # Test-Logs
│   └── prowb\                  # ProWB-Builder (separat)
│       └── prowb.exe
│
├── lib\                        # alle Import-Libs (flach)
│   ├── ProPhysics.lib
│   └── pro_sdk_interface.lib
│
├── src\                        # Quellcode
│   ├── prophysics\
│   │   ├── *.c                 (12 Module)
│   │   └── header\*.h
│   ├── sdk\
│   │   ├── pro_sdk_interface.c (1 Modul)
│   │   └── header\pro_sdk_interface.h
│   ├── test\
│   │   ├── alpha_test_*.c      (17 Module)
│   │   ├── example_test_*.c    (2 Module)
│   │   └── header\alpha_test_common.h
│   └── prowb\                  # Web-Docs-Builder
│       ├── prowb.c
│       ├── md_parser.c
│       ├── README.md
│       └── header\
│           ├── prowb.h
│           └── md_parser.h
│
├── tools\                      # Build-/Test-Werkzeuge
│   ├── pro_run.cmd             # zentraler Einstiegspunkt
│   ├── pro_run.ps1
│   ├── run_alpha_tests.cmd
│   └── run_alpha_tests.ps1
│
├── build\                      # Build-Infrastruktur
│   ├── main\                   # Master-Orchestrierung + Wrapper
│   ├── prophysics\             # Kernel-Makefile
│   ├── sdk\                    # SDK-Makefile
│   ├── test\                   # Test-Makefile
│   └── prowb\                  # ProWB-Makefile
│
├── docs\                       # Dokumentation
│   ├── build\                  # (diese Sektion)
│   ├── project\                # Roadmap/Projekt
│   ├── physics\                # Physik-Übersicht
│   ├── test\                   # Testkatalog + Runner-Doc
│   └── web\                    # Web-Docs-Quelle
│       ├── manifest.txt        # zentrale Konfiguration
│       ├── README.md           # Manifest-Pflege
│       └── src\                # Templates, CSS, JS, Views
│
├── python\
│   └── analysis.py
│
└── out\                        # Export-Ziel (runtime erzeugt)
    ├── exe\                                  # entpackter Inhalt
    ├── sdk\
    ├── kit\
    ├── web\                                  # Web-Docs (ProWB-Output)
    ├── prophysics-exe-<version>.zip          # ZIP-Archive
    ├── prophysics-sdk-<version>.zip
    └── prophysics-kit-<version>.zip
```

**Prinzipien:**

- **`bin\` und `lib\` sind flach.** Alle DLLs/EXEs nebeneinander, damit
  der Windows-Loader sie ohne `PATH`-Eintrag findet.
- **`bin\prowb\` ist separat.** ProWB ist ein Build-Tool, kein
  Runtime-Artefakt. Getrenntes Verzeichnis hält den Kernel-`clean`
  sauber.
- **`.c`-Dateien direkt, `.h`-Dateien in `header\`.** Innerhalb jedes
  `src\<modul>\`-Ordners.
- **Keine Header-Kopien.** Header bleiben am Pflegeort; andere Module
  binden sie über `/I`-Pfade ein.
- **Keine OBJ-Reste.** Alle Zwischendateien landen in lokalen `_obj\`-
  Ordnern und werden nach dem Link gelöscht.
- **`tools\` ist separat.** Skripte, die nicht zum Kernel-Build gehören
  (Test-Runner, `pro_run`), liegen nicht in `bin\`, sondern in `tools\`.
- **`docs\web\` ist Konfiguration + Layout.** Keine MD-Kopien; das
  Manifest verweist auf die kanonischen MD-Pfade.
- **`$(MAKEDIR)` statt CWD.** Alle Sub-Makefiles leiten ihre Pfade
  relativ zu ihrem eigenen Ablageort ab. Der Aufruf ist damit
  unabhängig vom aktuellen Arbeitsverzeichnis.
- **`.github\workflows\` gehört zum Build-System.** Die CI ist keine
  externe Komponente — sie ist die automatisierte Variante der
  lokalen Regression.

---

## 3. Die Build-Komponenten

### 3.1 Sub-Makefiles

Vier Module, jeweils ein eigenes Makefile:

| Datei | Baut | Doku |
|---|---|---|
| `build\prophysics\Makefile.nmake` | Kernel-DLL + Import-Lib | `docs\build\prophysics\Makefile.md` |
| `build\sdk\Makefile.sdk.nmake` | SDK-Interface-DLL + Import-Lib | `docs\build\sdk\Makefile.md` |
| `build\test\Makefile.nmake` | drei Test-EXEs | `docs\build\test\Makefile.md` |
| `build\prowb\Makefile.nmake` | ProWB-Builder-EXE | `docs\build\prowb\Makefile.md` |

Jedes Sub-Makefile:
- Leitet seine Pfade über `$(MAKEDIR)\..\..` ab und ist CWD-unabhängig.
- Akzeptiert `CONFIG=release|debug` (Default: `release`).
- Legt seine Zwischendateien in `$(MAKEDIR)\_obj\` ab.
- Hat ein `clean`-Target, das nur seine eigenen Artefakte entfernt.
- Hat ein `help`-Target, das Targets und Optionen auflistet.

### 3.2 Master-Makefile

`build\main\Makefile.nmake` — orchestriert die Sub-Makefiles.

**Targets:**

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup` + `prophysics` + `sdk` + `test` + `prowb` + `info` |
| `prophysics` | nur Kernel |
| `sdk` | Kernel + SDK (Kettenabhängigkeit) |
| `test` | Kernel + SDK + Tests |
| `prowb` | nur ProWB-Builder |
| `info` | nur `BUILD_INFO.txt` |
| `help` | Übersicht |
| `rebuild` | `clean` + `all` |
| `rebuild_prophysics` | `clean_prophysics` + `prophysics` + `info` |
| `rebuild_sdk` | `clean_sdk` + `sdk` + `info` |
| `rebuild_test` | `clean_test` + `test` + `info` |
| `rebuild_prowb` | `clean_prowb` + `prowb` |
| `clean` | bin\ + lib\ + BUILD_INFO.txt weg |
| `clean_prophysics` | nur Kernel-Artefakte |
| `clean_sdk` | nur SDK-Artefakte |
| `clean_test` | nur Test-Artefakte |
| `clean_prowb` | nur ProWB-Artefakte |

**Setup:** Legt `bin\`, `lib\` und `bin\prowb\` an, falls sie nicht
existieren.

**CONFIG-Weitergabe:** `CONFIG=release|debug` wird an alle Sub-Makefiles
durchgereicht.

**Position von `prowb` im `all`-Target:** **Nach** `test`. ProWB ist
ein Build-Tool, kein Runtime-Artefakt. Fehlschlag blockiert nicht die
Kernel-Kette.

**Doku:** `docs\build\main\Makefile.md`

### 3.3 PowerShell-Wrapper

Drei Skripte in `build\main\`, die nmake-Aufrufe kapseln und zusätzliche
Funktionen bieten. Sie werden im Normalfall **nicht mehr direkt**
aufgerufen, sondern über `pro_run` dispatcht.

| Skript | Rolle | Doku |
|---|---|---|
| `build.ps1` + `build.cmd` | Build-Wrapper mit Modus/Flags | `docs\build\helper\build.md` |
| `export.ps1` + `export.cmd` | Build + Export + ZIP | `docs\build\helper\export.md` |
| `write_build_info.ps1` | BUILD_INFO.txt-Generator | `docs\build\helper\write_build_info.md` |

### 3.4 Test-Runner

Liegt in `tools\`. Self-Locating, nimmt alle 43 Tests in den Prios 1–8.

| Datei | Rolle | Doku |
|---|---|---|
| `tools\run_alpha_tests.ps1` + `.cmd` | Test-Runner | `docs\test\run_alpha_tests.md` |

Der Runner ist self-locating: `-ExeDir` Default = `<repo>\bin`,
`-DllDir` Default = `<repo>\bin`, `-LogDir` Default = `<ExeDir>\logs`.

**Prio 8 enthält zwei Tests** (`SU2-Wilson-Loop` und `Running-Coupling`).
Der zweite läuft ~23 min und ist nicht CI-tauglich. Siehe
`docs\test\run_alpha_tests.md` §16 für CI-Empfehlungen.

### 3.5 `pro_run` — zentraler Einstiegspunkt

`tools\pro_run.ps1` + `tools\pro_run.cmd`. Dispatcht auf die Skripte
aus §3.3, §3.4 und §3.6.

| Aktion | Ruft auf |
|---|---|
| `pro_run build` | `build\main\build.ps1` |
| `pro_run export` | `build\main\export.ps1` |
| `pro_run test` | `tools\run_alpha_tests.ps1` |
| `pro_run web` | `build\prowb\prowb.exe` (via nmake) |
| `pro_run all` | build → test → export (Abbruch bei Fehlschlag) |
| `pro_run help` | Übersicht / `pro_run help <aktion>` |

**Doku:** `docs\build\pro_run.md`

### 3.6 ProWB-Build

`build\prowb\Makefile.nmake` — baut `bin\prowb\prowb.exe`.

**Targets:**

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup` + EXE |
| `setup` | `bin\prowb\` anlegen |
| `help` | Übersicht |
| `clean` | Artefakte entfernen |

**CONFIG:** `release|debug`.

**Position im Master:** Nach `test` im `all`-Target. Eigenständig
baubar über `nmake /f Makefile.nmake` in `build\prowb\`.

**Doku:** `docs\build\prowb\Makefile.md`

**Builder-Referenz:** `src\prowb\README.md`

### 3.7 CI-Workflows (neu in v1.2)

Drei GitHub-Actions-Workflows in `.github\workflows\`. Sie sind die
**automatisierte Variante** der lokalen Regression — kein Ersatz,
sondern eine Ergänzung.

| Datei | Trigger | Laufzeit | Was läuft | Doku |
|---|---|---:|---|---|
| `ci.yml` | Push + PR auf `main` | ~1,5 min | Build + Prio 1, 6, 7, `SU2-Wilson-Loop` | `docs\build\ci.md` |
| `web-docs.yml` | Push auf `main` (nur `docs/**`, `src/prowb/**`, `build/prowb/**`) | ~1 min | ProWB-Build + Web-Docs + Deploy auf GitHub Pages | `docs\build\web-docs-ci.md` |
| `alpha-nightly.yml` | **manuell** (`workflow_dispatch`) | ~23–64 min | Prio 5 + `Running-Coupling` (Scope-abhängig) | `docs\build\alpha-nightly.md` |

**Aufteilung:**

- **Standard-CI** deckt die **schnelle Regression** ab (~1,5 min).
- **Web-Docs-CI** deckt **Doku-Deploy** ab (~1 min).
- **Alpha-Nightly** deckt **die langen Tests** ab (~64 min, manuell).

**Warum drei Workflows statt einem?** Jeder hat einen anderen Zweck,
andere Trigger und andere Laufzeiten. Eine Zusammenlegung würde
entweder die Standard-CI verlangsamen oder den Nightly-Lauf
unnötig häufig auslösen.

**Warum kein Cron-Trigger für den Nightly?** GitHub deaktiviert
`schedule`-Workflows nach 60 Tagen Inaktivität. Manuell = der Nutzer
entscheidet.

---

## 4. Typische Aufrufe

### 4.1 Über `pro_run` (empfohlen)

```cmd
:: Kompletter Durchlauf: bauen, testen, als SDK-ZIP exportieren
pro_run all

:: Nur Kernel bauen, Debug-Konfiguration, vorher clean
pro_run build -Mode kernel -Config debug -Rebuild

:: SDK + Kit als ZIP nach D:\sdk-kit
pro_run export -Export all -OutDir D:\sdk-kit -Version 1.23.0

:: Tests Prio 1-4 (schnelle Regression)
pro_run test -Prio 1-4

:: Einzelner Test mit eigenem Log-Verzeichnis
pro_run test -Test Running-Coupling -LogDir C:\logs

:: Web-Docs bauen
pro_run web

:: Web-Docs mit Rebuild
pro_run web -Rebuild

:: Alles inkl. Web-Docs
pro_run all && pro_run web

:: Nur Test, ohne vorher zu bauen (Build existiert bereits)
pro_run all -NoBuild -NoExport -Prio 8

:: Hilfe
pro_run help
pro_run help export
pro_run help web
```

### 4.2 Direkte Aufrufe (Legacy / Debug)

Die bestehenden Wrapper bleiben erhalten und rufen dieselben Skripte
auf. Nützlich, wenn nur eine einzelne Komponente gebaut werden soll,
ohne die `pro_run`-Dispatch-Schicht.

```cmd
:: Build direkt
build\main\build.cmd
build\main\build.cmd -Mode prophysics -Config debug
build\main\build.cmd -Mode all -Rebuild -GitStamp -GitNote "Release 1.23.0"
build\main\build.cmd -Clean

:: Export direkt
build\main\export.cmd sdk -Scope all -Clean
build\main\export.cmd kit -Version 1.23.0

:: Test direkt
tools\run_alpha_tests.cmd -Prio 8
tools\run_alpha_tests.cmd -Prio all
tools\run_alpha_tests.cmd -Prio 1,3,5

:: Web-Docs direkt
bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web
```

### 4.3 Direkt im Sub-Ordner (nur nmake)

```cmd
cd build\prophysics
nmake /NOLOGO /f Makefile.nmake CONFIG=release

cd ..\sdk
nmake /NOLOGO /f Makefile.sdk.nmake CONFIG=release

cd ..\test
nmake /NOLOGO /f Makefile.nmake CONFIG=release

cd ..\prowb
nmake /NOLOGO /f Makefile.nmake CONFIG=release
```

### 4.4 Über GitHub Actions

```cmd
:: Workflow manuell starten (Actions-Tab im Browser)
:: -> Alpha-Nightly -> Run workflow -> Scope wählen
```

**Standard-CI** läuft automatisch bei Push und PR auf `main`.
**Web-Docs-CI** läuft automatisch bei Push auf `main` mit
Doku-Änderungen. **Alpha-Nightly** wird manuell gestartet.

---

## 5. Reihenfolge und Kettenabhängigkeiten

```
build\prophysics
    ├── liest:  src\prophysics\*.c, src\prophysics\header\*.h
    ├── schreibt: bin\ProPhysics.dll, lib\ProPhysics.lib
    │
    ▼
build\sdk
    ├── liest:  src\sdk\pro_sdk_interface.c, src\sdk\header\*.h
    │           src\prophysics\header\*.h
    │           lib\ProPhysics.lib
    ├── schreibt: bin\pro_sdk_interface.dll, lib\pro_sdk_interface.lib
    │
    ▼
build\test
    ├── liest:  src\test\*.c, src\test\header\*.h
    │           src\prophysics\header\*.h, src\sdk\header\*.h
    │           lib\ProPhysics.lib, lib\pro_sdk_interface.lib
    ├── schreibt: bin\example_*.exe
    │
    ▼ (unabhängig, parallel möglich)
build\prowb
    ├── liest:  src\prowb\*.c, src\prowb\header\*.h
    ├── schreibt: bin\prowb\prowb.exe
```

**Kettenprüfung:** SDK- und Test-Build haben Vorabprüfungen (`check_core`,
`check_deps`), die mit klarer Meldung abbrechen, wenn die Vorgängerlibs
fehlen.

**ProWB ist unabhängig.** Der ProWB-Build hat keine
Kettenabhängigkeit. Er kann parallel zu Kernel/SDK/Tests gebaut werden.

Der Master-Build drückt die Kernel-Kette durch Makefile-Abhängigkeiten
aus: `test: sdk`, `sdk: prophysics`. Ein `nmake sdk` baut also den
Kernel automatisch mit.

---

## 6. Was die Build-Skripte **nicht** tun

- **Kein Signing** im Sub-Makefile. Nur `build.ps1 -Sign` ruft `signtool`.
- **Kein Header-Export.** Header bleiben am Pflegeort.
- **Kein Deployment** aus den Makefiles. Der Web-Docs-Deploy läuft
  über die CI (`web-docs.yml`).
- **Keine Test-Ausführung.** Nur `run_alpha_tests.ps1` / `pro_run test`.
- **Keine inkrementelle Header-Analyse.** Ein Header-Update rebuildet
  alle Module eines Sub-Makefiles (bewusst grob).
- **Kein MD-Kopieren.** ProWB liest die Doku von ihren Pflegeorten;
  keine Kopien in `docs\web\`.
- **Kein Auto-Nightly.** Die langen Tests laufen nur manuell
  (`alpha-nightly.yml`) oder lokal (`pro_run test -Prio all`).

---

## 7. Konfiguration

### 7.1 Compiler-Flags

Pro Sub-Makefile definiert. Alle Sub-Makefiles kennen
`CONFIG=release|debug`:

| Modul | Release | Debug |
|---|---|---|
| Kernel | `/W4 /O2 /Ob2 /Oi /GL /MP /arch:AVX2` + `/LTCG` | `/W4 /Od /Zi /MDd /MP /arch:AVX2` + `/DEBUG` |
| SDK | `/W3 /O2 /Ob2 /Oi /GL /MP` + `/LTCG` | `/W3 /Od /Zi /MDd /MP` + `/DEBUG` |
| Test | `/W3 /O2 /Ob2 /Oi /GL /MP /arch:AVX2` + `/LTCG` | `/W3 /Od /Zi /MDd /MP /arch:AVX2` + `/DEBUG` |
| ProWB | `/W4 /O2 /Ob2 /Oi /GL /MP` + `/LTCG` | `/W4 /Od /Zi /MDd /MP` + `/DEBUG` |

**Hinweis:** ProWB nutzt `/W4` wie der Kernel, aber **kein**
`/arch:AVX2` — ProWB ist IO-lastig, nicht compute-lastig.

### 7.2 Pfade

Alle Sub-Makefiles verwenden **`$(MAKEDIR)`**-basierte Pfade relativ zu
ihrem eigenen Ablageort. Der Aufruf ist damit unabhängig vom CWD —
egal ob über den Master, über `pro_run`, oder direkt aufgerufen.

Der Master-Makefile leitet seinen Repo-Root ebenfalls über `$(MAKEDIR)`
ab und wechselt beim Sub-Aufruf mit `cd /d` in das jeweilige
Modulverzeichnis.

### 7.3 `dim`-Regel

R1: `dim` muss Zweierpotenz sein. Gilt zur Laufzeit (Gitter-Konfiguration),
nicht beim Build. Aktuell verwendet: `dim ∈ {16, 32, 64, 128}`.

### 7.4 Q31 vs. Q30 (Kernel)

Der Kernel arbeitet mit **Q31** für Amplituden (Skala `INT32_MAX`) und
**Q30** für SU(2)-Links (Skala `2^30`, um int64-Overflow in der
Quaternion-Multiplikation zu vermeiden). Diese Dualität ist stabil und
in `ProPhysics_Config.h` dokumentiert.

### 7.5 `CONFIG`-Support

`CONFIG=release|debug` ist der einzige Build-Parameter, der durch alle
Makefile-Ebenen gereicht wird:

- Master: `nmake /f Makefile.nmake all CONFIG=debug`
- Sub: `nmake /f Makefile.nmake CONFIG=debug`
- PowerShell-Wrapper: `build.ps1 -Config debug` → ruft nmake mit `CONFIG=debug`

Ungültige Werte brechen den Build mit `!ERROR` ab.

### 7.6 CI-Umgebung

Die CI-Workflows nutzen:

| Aspekt | Wert |
|---|---|
| Runner | `windows-latest` |
| MSVC-Setup | `ilammy/msvc-dev-cmd@v1` mit `arch: x64` |
| Konfiguration | `release` (Default) |
| Timeout | 120 min (Nightly), 360 min (Default, Standard-CI) |
| Cache | keiner |

Details siehe `docs\build\ci.md`, `docs\build\alpha-nightly.md`,
`docs\build\web-docs-ci.md`.

---

## 8. Was jeder Build-Schritt erzeugt

### 8.1 Nach `nmake all` (Master)

| Pfad | Was |
|---|---|
| `bin\ProPhysics.dll` | Kernel-DLL |
| `bin\pro_sdk_interface.dll` | SDK-Interface-DLL |
| `bin\example_alpha_test.exe` | Haupt-Test-Suite |
| `bin\example_test_density.exe` | Dichte-Regression |
| `bin\example_test_tensor.exe` | Tensor-/Fock-Regression |
| `bin\prowb\prowb.exe` | ProWB-Web-Docs-Builder |
| `lib\ProPhysics.lib` | Kernel-Import-Lib |
| `lib\pro_sdk_interface.lib` | SDK-Import-Lib |
| `BUILD_INFO.txt` | Metadaten (Zeitstempel, Version, Etappe, Dateiliste) |

**Hinweis:** `bin\prowb\prowb.exe` liegt **nicht** direkt in `bin\`,
sondern in `bin\prowb\`. Grund: ProWB ist ein Build-Tool, kein
Runtime-Artefakt.

### 8.2 Nach `pro_run export` / `export.ps1` (Modi)

| Modus | Ordner | ZIP |
|---|---|---|
| `exe` | `out\exe\` — DLLs + EXEs flach, mit Runner aus `tools\` | `out\prophysics-exe-<version>.zip` |
| `sdk` | `out\sdk\libs\` — DLLs + LIBs + Header | `out\prophysics-sdk-<version>.zip` |
| `kit` | `out\kit\` — SDK + Beispiele + erweiterte Docs | `out\prophysics-kit-<version>.zip` |
| `all` | alle drei nacheinander | alle drei ZIPs |

`<version>` ist `MAJOR.MINOR.PATCH` aus `ProPhysics_Version.h`, ohne
`v`-Präfix. Aktuell: `1.23.0`.

Details siehe `docs\build\helper\export.md`.

### 8.3 Nach `pro_run web` / `prowb.exe`

ProWB erzeugt:

| Pfad | Was |
|---|---|
| `out\web\index.html` | Single-Page-App (Manifest-Übersicht) |
| `out\web\assets\*.css` | 8 CSS-Dateien (kopiert) |
| `out\web\assets\*.js` | 2 JS-Dateien (kopiert) |
| `out\web\data\docs.js` | Geparste MD-Inhalte (JSON-ish) |

**Größe:** ~1,2 MB bei 44 Docs.

**Enthält nicht:** Keine MD-Dateien, keine Kopien. Alle Inhalte sind
im `data\docs.js` eingebettet.

**Deployment:** `out\web\` wird von der CI
(`.github\workflows\web-docs.yml`) auf GitHub Pages deployt.

Details siehe `docs\build\prowb\Makefile.md`, `src\prowb\README.md`,
`docs\web\README.md`, `docs\build\web-docs-ci.md`.

### 8.4 Nach CI-Läufen

| Workflow | Artefakt | Retention |
|---|---|---|
| `ci.yml` | `test-logs` (aus `bin\logs\`) | 7 Tage |
| `web-docs.yml` | Pages-Artefakt (nicht herunterladbar) | (Pages) |
| `alpha-nightly.yml` | `alpha-nightly-logs-<scope>-<run_id>` | 30 Tage |

Alle Artefakte sind im Actions-Tab unter „Summary" → „Artifacts"
erreichbar. Details siehe die jeweilige CI-Doku.

---

## 9. Fehlersuche — Erste Anlaufstellen

| Symptom | Wahrscheinliche Ursache | Erster Blick |
|---|---|---|
| `pro_run nicht gefunden` | `tools\` nicht im PATH oder falscher CWD | `tools\pro_run.cmd` direkt aufrufen |
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` aufrufen |
| `ProPhysics.lib fehlt` | Kernel nicht gebaut | `pro_run build -Mode kernel` |
| `pro_sdk_interface.lib fehlt` | SDK nicht gebaut | `pro_run build -Mode sdk` |
| `*.h not found` | Header am falschen Ort | `src\<modul>\header\` prüfen |
| `unresolved external symbol ProPhysics_...` | Kernel-Lib fehlt oder Link-Reihenfolge falsch | SDK-Lib **vor** Kernel-Lib in Link-Zeile |
| `unresolved external symbol ProPhysics_SU2_...` | `SU2_Dynamics.c` nicht im `OBJ_KERNEL` | `build\prophysics\Makefile.nmake` prüfen |
| `unresolved external symbol ProPhysics_SU2_Link_Plaquette_Sum` | Neue Funktion nicht im Kernel-`.obj` | `pro_run build -Mode kernel -Rebuild` |
| Test-EXE startet nicht | DLL nicht in `bin\` | Kernel + SDK zuerst bauen |
| Test-EXE „DLL nicht gefunden" | EXE und DLL in verschiedenen Ordnern | beide müssen in `bin\` liegen |
| Umlaute kaputt in Konsole | `chcp` fehlt | `*.cmd`-Wrapper statt direkt aufrufen |
| `BUILD_INFO.txt fehlt` | `-Clean` gesetzt | `build.cmd -NoBuild` erneut laufen |
| `CONFIG muss "release" oder "debug" sein` | Tippfehler bei `-Config` | Groß-/Kleinschreibung prüfen |
| Runner findet Test-EXE nicht | `-ExeDir` falsch | Default ist `<repo>\bin`, prüfen |
| Runner lädt falsche DLL | `-DllDir != -ExeDir` | beide auf `<repo>\bin` zeigen lassen |
| `Running-Coupling TIMEOUT` | Rechner zu langsam | Timeout in `run_alpha_tests.ps1` erhöhen |
| `ZIP konnte nicht erzeugt werden` | `OutDir` voll oder gesperrt | `Compress-Archive`-Fehler prüfen |
| `prowb.exe: No such file` | ProWB nicht gebaut | `pro_run web` baut automatisch |
| `Manifest nicht gefunden` | `docs\web\manifest.txt` fehlt | Pfad prüfen |
| `out\web\index.html` leer | Manifest leer oder Parser-Fehler | `pro_run web -v` prüfen |
| Web-Docs-Icons zeigen □ | UTF-8-Problem in `manifest.txt` | Datei als UTF-8 speichern |
| `prowb.exe` Exit 3 | IO-Fehler (Output-Verzeichnis) | `out\web\` beschreibbar? |
| CI-Web-Docs schlägt fehl | Pages-Source nicht auf „GitHub Actions" | Repo-Settings prüfen |
| Standard-CI FAIL, lokal OK | MSVC-Version unterschiedlich | `windows-latest`-Version im Log prüfen |
| Alpha-Nightly taucht nicht auf | Workflow-Datei nicht committed | `.github\workflows\alpha-nightly.yml` prüfen |
| Alpha-Nightly TIMEOUT | Runner zu langsam | `timeout-minutes` erhöhen |

**Detail-Diagnose** pro Sub-Makefile: siehe jeweilige Doku-Seite.
**Detail-Diagnose** pro CI-Workflow: siehe `ci.md`, `alpha-nightly.md`,
`web-docs-ci.md`.

---

## 10. Dokumentations-Struktur

```
docs\build\
├── BUILD_SCRIPT.md                # diese Datei
├── pro_run.md                     # zentraler Einstiegspunkt
├── ci.md                          # Standard-CI (ci.yml)
├── alpha-nightly.md               # Manueller Nightly (alpha-nightly.yml)
├── web-docs-ci.md                 # Web-Docs-CI (web-docs.yml)
├── main\
│   └── Makefile.md                # Master-Makefile
├── prophysics\
│   └── Makefile.md                # Kernel-Build
├── sdk\
│   └── Makefile.md                # SDK-Interface-Build
├── test\
│   └── Makefile.md                # Test-Build
├── prowb\
│   └── Makefile.md                # ProWB-Makefile
└── helper\
    ├── build.md                   # build.ps1 / build.cmd
    ├── export.md                  # export.ps1 / export.cmd
    └── write_build_info.md        # write_build_info.ps1
```

Test-Dokumentation separat:

```
docs\test\
├── ProPhysics_Testkatalog.md      # alle 43 Tests
├── run_alpha_tests.md             # Test-Runner
├── BASELINE.md                    # Test-Baseline
└── WRITING_TESTS.md               # Anleitung zum Test-Schreiben
```

Web-Docs-Dokumentation separat:

```
docs\web\
├── manifest.txt                   # zentrale Konfiguration
├── README.md                      # Manifest-Pflege
└── src\                           # Templates, CSS, JS, Views
```

ProWB-Builder-Dokumentation:

```
src\prowb\
├── README.md                      # Builder-Referenz
├── prowb.c
├── md_parser.c
└── header\
    ├── prowb.h
    └── md_parser.h
```

Projekt-Roadmap:

```
docs\project\
└── Project.md                     # Ontologie + Roadmap
```

**Empfohlene Lesereihenfolge für neue Mitwirkende:**

1. Diese Datei (Überblick).
2. `docs\project\Project.md` — was das Projekt ist.
3. `docs\build\pro_run.md` — wie man den Build bedient.
4. `docs\build\<modul>\Makefile.md` — je nach Modul-Interesse.
5. `docs\build\helper\build.md` — Details zu `build.ps1`.
6. `docs\test\ProPhysics_Testkatalog.md` — was getestet wird.
7. `docs\test\run_alpha_tests.md` — wie man die Tests fährt.
8. `docs\build\ci.md` — was in der CI läuft.
9. `src\prowb\README.md` — wie die Web-Docs entstehen.
10. `docs\web\README.md` — wie man die Web-Docs pflegt.

---

## 11. Was passiert nach dem Build

Nach erfolgreichem `pro_run all` (Default) oder `build.cmd`
(Default-Modus `all`):

1. `bin\` enthält 2 DLLs + 3 EXEs.
2. `bin\prowb\` enthält `prowb.exe`.
3. `lib\` enthält 2 LIBs.
4. `BUILD_INFO.txt` im Root ist aktuell (mit Version `1.23.0`,
   Etappe `23`).
5. Test-Runner und `pro_run` sind einsatzbereit in `tools\`.
6. `out\` enthält die angeforderten ZIP-Pakete.

Nach erfolgreichem `pro_run web`:

7. `out\web\` enthält das generierte Portal (`index.html` + Assets).
8. Das Portal ist lokal im Browser öffenbar (Doppelklick).
9. Die CI deployt es auf GitHub Pages (bei Push auf `main`).

Nach einem Push auf `main`:

10. Standard-CI läuft (~1,5 min, `.github\workflows\ci.yml`).
11. Web-Docs-CI läuft, wenn Doku geändert wurde (~1 min).
12. GitHub Pages aktualisiert sich (Live-URL
    `https://onkel83.github.io/prophysics/`).

Nach manuellem Start von Alpha-Nightly:

13. Prio 5 + `Running-Coupling` laufen (~64 min).
14. Logs werden als Artefakt hochgeladen (30 Tage).

**Nächster Schritt:**

```cmd
pro_run test -Prio all
```

**Erwartung:** 43/43 PASS, ~4 421 s (~74 min).
- Prio 5 (Hydrogen-48): ~2 281 s
- Prio 8 (Running-Coupling): ~1 398 s
- Alle anderen 41 Tests: ~742 s

**Für die Web-Docs:**

```cmd
pro_run web
```

**Erwartung:** `out\web\index.html` ~1,2 MB, 44 Docs geparst.

---

## 12. Was in Etappe 22 bis 23 dazugekommen ist

### 12.1 — Etappe 22: SU(2)-Eichfeld (kinematisch)

- **`ProEdge`** — um 4 × int32 (`su2_a_re/ai`, `su2_b_re/bi`) erweitert.
  `sizeof(ProEdge)` wächst von 12 auf 24 Bytes.
- **Neue Kernel-Datei** `ProPhysics_SU2.c` (Kernel-Modul 4/12).
- **Neue public API:** `Set_Edge_SU2`, `Set_Edge_SU2_AxisAngle`,
  `Get_Edge_SU2`, `Wilson_Loop_SU2`, `Wilson_Loop_SU2_Trace`,
  `Apply_Local_SU2_Gauge`, `Verify_SU2_Quaternion`.
- **Neue `ProUniverse`-Felder:** `su2_active`, `su2_gauge_basis`,
  `su2_coupling_q15`.
- **Master-Makefile** — `ProPhysics_SU2.obj` in `OBJ_KERNEL`.
- **Test-Makefile** — `alpha_test_su2.c` in `ALPHA_SOURCES`.
- **Test-Runner** — Prio 8, Timeout ~300 s.
- **Refactoring 22 (parallel):**
  - Kernel-Tick (`ProPhysics_Tick`) in `ProPhysics_Core.c`.
  - Cache-Aligned-Alloc (`pro_aligned_calloc`) für 8 Hot-Arrays.
  - Zentrale Helfer in `Internal.h`.
  - Konstanten zentral in `ProPhysics_Config.h`.
  - Test-Runner von `bin\` nach `tools\`.
  - EPR-Debug-Ring statt printf im Kernel-Tick.

### 12.2 — Etappe 22b: SU(2)-Link-Dynamik (Leapfrog)

- **`ProEdge`** — um 4 × int32 (`su2_E_a_re/ai`, `su2_E_b_re/bi`) erweitert.
  `sizeof(ProEdge)` wächst von 24 auf 40 Bytes.
- **Neue Kernel-Datei** `ProPhysics_SU2_Dynamics.c` (Kernel-Modul 5/12).
- **Neue public API:** `Enable_SU2_Dynamics`, `Disable_SU2_Dynamics`,
  `Is_SU2_Dynamics_Active`, `Set_SU2_Yang_Mills`, `Apply_SU2_Tick`,
  `SU2_Plaquette_Action`, `SU2_Total_Energy`, `SU2_Link_Plaquette_Sum`.
- **Neue `ProUniverse`-Felder:** `su2_dynamics_active`, `su2_yang_mills_q15`.
- **Neue Config-Konstanten:** `PRO_SU2_YM_DEFAULT_Q15`,
  `PRO_SU2_LEAPFROG_DT_Q15`.
- **`Internal.h`** — `pro_su2_mul/conj/norm_sq` zentral; neue
  `pro_su2_exp_apply`.
- **Master-Makefile** — `ProPhysics_SU2_Dynamics.obj` in `OBJ_KERNEL`.

### 12.3 — Etappe 23: SU(2)-Metropolis / Wilson-Action-Validierung

- **Neue Kernel-Funktion:** `ProPhysics_SU2_Link_Plaquette_Sum` (read-only).
  Kein Struct-Change, kein neues Modul, keine Config-Erweiterung.
- **Neuer Test:** `alpha_test_running_coupling.c` (Test-Modul 17/17).
- **Test-Makefile** — `alpha_test_running_coupling.c` in `ALPHA_SOURCES`.
- **Test-Runner** — Prio 8 enthält beide Tests; Timeout für
  Running-Coupling auf 1800 s.
- **Backward-Staple-Fix in 22b** (in Etappe 23 verifiziert):
  T16 von 8,06e-03 auf 2,44e-03, T15 von 2,02e-08 auf 1,80e-08.
- **V&V-Anker (NEU):** Erste absolute Validierung gegen externe
  Lattice-QCD-Physik. ⟨P⟩(β=2) = 0,43346 vs. Referenz I₂(2)/I₁(2) =
  0,43313, Abweichung **0,08 %**.

### 12.4 — Skript-Vereinheitlichung (Etappe 23, Konsolidierung)

- **`pro_run.ps1` + `pro_run.cmd`** — neuer zentraler Einstiegspunkt in
  `tools\`. Dispatcht auf `build.ps1`, `export.ps1`, `run_alpha_tests.ps1`.
  Aktion `all` führt build → test → export aus.
- **`-Config <release|debug>`** für alle Sub-Makefiles und Wrapper.
- **`$(MAKEDIR)`-basierte Pfade** in allen Sub-Makefiles (CWD-unabhängig).
- **`export.ps1`** um Export-Typ `kit` erweitert, ZIP-Erzeugung
  (`prophysics-<kind>-<version>.zip`), `-Version` Parameter.
- **`run_alpha_tests.ps1`** akzeptiert `-Prio` mit Range/Liste,
  `-Test <name>`, `-DllDir`.
- **`write_build_info.ps1`** um Etappe-Zeile und Config-Feld erweitert.
- **Konsistenz-Fixes:** Kernel-Modul-Zahl überall **12**, Test-Modul-Zahl
  überall **17**, Test-Anzahl überall **43**.

### 12.5 — ProWB / Web-Docs Integration (Patch `1.23.11`)

- **`src\prowb\`** — neuer Builder (C99), bestehend aus `prowb.c`,
  `md_parser.c` und zwei Headern in `header\`.
- **`build\prowb\Makefile.nmake`** — baut `bin\prowb\prowb.exe`.
  `CONFIG=release|debug`. Eigenständig baubar.
- **Master-Makefile** — Target `prowb` ergänzt. Position im `all`-Target:
  **nach** `test`. Fehlschlag blockiert nicht die Kernel-Kette.
- **`pro_run web`** — neue Aktion. Ruft `nmake` in `build\prowb\` und
  dann `prowb.exe --manifest`.
- **`docs\web\`** — Web-Docs-Quelle (Manifest + Templates + Assets).
  Keine MD-Kopien; das Manifest verweist auf die kanonischen Pfade.
- **`.github\workflows\web-docs.yml`** — CI-Workflow: baut ProWB +
  Web-Docs, deployt auf GitHub Pages.
- **Neue Doku:** `src\prowb\README.md`, `docs\build\prowb\Makefile.md`,
  `docs\web\README.md`, `docs\build\web-docs-ci.md`.

### 12.6 — CI-Workflows und CI-Doku (Patch `1.23.12`)

- **`.github\workflows\ci.yml`** — Standard-CI: Build + Prio 1, 6, 7,
  `SU2-Wilson-Loop`. Läuft auf jedem Push und PR auf `main` (~1,5 min).
- **`.github\workflows\web-docs.yml`** — Web-Docs-CI (siehe §12.5).
- **`.github\workflows\alpha-nightly.yml`** — Manueller Nightly-Workflow
  für Prio 5 + `Running-Coupling` (~64 min). `workflow_dispatch`,
  Scope-Auswahl (`all-long`, `prio-5`, `running-coupling`).
- **Neue Doku:** `docs\build\ci.md`, `docs\build\alpha-nightly.md`.
  `docs\build\web-docs-ci.md` existiert bereits (§12.5).
- **`docs\build\BUILD_SCRIPT.md`** (diese Datei) — §3.7 (CI-Workflows)
  und §12.6 (dieser Abschnitt) neu.
- **`docs\web\manifest.txt`** — Sektion `Build` um `docs/build/ci.md`
  und `docs/build/alpha-nightly.md` erweitert (Einträge von 44 auf 46).

Die Kernel-Build-Kette selbst bleibt unverändert. CI-Workflows sind
**keine Build-Komponenten** im engeren Sinn, sondern eine
automatisierte Variante der lokalen Regression.

---

## 13. Siehe auch

| Thema | Datei |
|---|---|
| Zentraler Einstiegspunkt | `docs\build\pro_run.md` |
| Standard-CI | `docs\build\ci.md` |
| Manueller Nightly | `docs\build\alpha-nightly.md` |
| Web-Docs-CI | `docs\build\web-docs-ci.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Kernel-Build | `docs\build\prophysics\Makefile.md` |
| SDK-Build | `docs\build\sdk\Makefile.md` |
| Test-Build | `docs\build\test\Makefile.md` |
| ProWB-Build | `docs\build\prowb\Makefile.md` |
| build.ps1 / build.cmd | `docs\build\helper\build.md` |
| export.ps1 / export.cmd | `docs\build\helper\export.md` |
| write_build_info.ps1 | `docs\build\helper\write_build_info.md` |
| ProWB-README | `src\prowb\README.md` |
| Web-Docs-Manifest | `docs\web\README.md` |
| Testkatalog | `docs\test\ProPhysics_Testkatalog.md` |
| Test-Runner | `docs\test\run_alpha_tests.md` |
| Test-Baseline | `docs\test\BASELINE.md` |
| Projekt-Roadmap | `docs\project\Project.md` |
| Changelog | `CHANGELOG.md` (`1.23.12`) |

---

**Ende Build-System-Übersicht v1.2.**