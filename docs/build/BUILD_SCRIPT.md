# ProPhysics Build & Package — Übersicht

**Stand:** 2026-09-27 (Etappe 23, Kernel 1.23.0)
**Zweck:** Einstiegspunkt in das Build-System. Erklärt die Struktur,
die Komponenten und ihre Wechselwirkungen. Verweist auf die
Detail-Dokumente.

---

## 1. Was das Build-System tut

Das Build-System erzeugt aus dem Quellcode unter `src\` drei Artefakte:

| Artefakt | Pfad | Quelle |
|---|---|---|
| Kernel-DLL + Import-Lib | `bin\ProPhysics.dll`, `lib\ProPhysics.lib` | `src\prophysics\` |
| SDK-Interface | `bin\pro_sdk_interface.dll`, `lib\pro_sdk_interface.lib` | `src\sdk\` |
| Test-EXEs | `bin\example_alpha_test.exe` + 2 weitere | `src\test\` |

Zusätzlich:
- **Test-Runner** in `tools\` — startet die Alpha-Suite.
- **Export-Funktionen** — kopieren Artefakte in `out\` und packen sie
  als ZIP-Archive (`prophysics-<kind>-<version>.zip`).
- **BUILD_INFO.txt** — Metadaten im Projekt-Root.
- **Einheitlicher Einstiegspunkt `pro_run`** — dispatcht auf die
  bestehenden Build-, Export- und Test-Skripte.

Die Build-Kette ist strikt sequenziell:

```
prophysics → sdk → test
```

Der SDK-Build linkt gegen `ProPhysics.lib`, der Test-Build gegen beide
Import-Libs. Kettenabhängigkeiten werden vom Master-Makefile erzwungen.

---

## 2. Ordner-Layout

```
ProPhysics\
├── BUILD_INFO.txt              # wird bei jedem Build erzeugt
│
├── bin\                        # alle DLLs + EXEs (flach)
│   ├── ProPhysics.dll
│   ├── pro_sdk_interface.dll
│   ├── example_alpha_test.exe
│   ├── example_test_density.exe
│   ├── example_test_tensor.exe
│   └── logs\                   # Test-Logs
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
│   └── test\
│       ├── alpha_test_*.c      (17 Module)
│       ├── example_test_*.c    (2 Module)
│       └── header\alpha_test_common.h
│
├── tools\                      # Build-/Test-Werkzeuge (nicht Teil des Kernel-Builds)
│   ├── pro_run.cmd             # zentraler Einstiegspunkt
│   ├── pro_run.ps1
│   ├── run_alpha_tests.cmd
│   └── run_alpha_tests.ps1
│
├── build\                      # Build-Infrastruktur
│   ├── main\                   # Master-Orchestrierung + Wrapper
│   ├── prophysics\             # Kernel-Makefile
│   ├── sdk\                    # SDK-Makefile
│   └── test\                   # Test-Makefile
│
├── docs\                       # Dokumentation
│   ├── build\                  # (diese Sektion)
│   ├── project\                # Roadmap/Projekt
│   └── test\                   # Testkatalog + Runner-Doc
│
├── python\
│   └── analysis.py
│
└── out\                        # Export-Ziel (runtime erzeugt)
    ├── exe\                                  # entpackter Inhalt
    ├── sdk\
    ├── kit\
    ├── prophysics-exe-<version>.zip          # ZIP-Archive
    ├── prophysics-sdk-<version>.zip
    └── prophysics-kit-<version>.zip
```

**Prinzipien:**

- **`bin\` und `lib\` sind flach.** Alle DLLs/EXEs nebeneinander, damit
  der Windows-Loader sie ohne `PATH`-Eintrag findet.
- **`.c`-Dateien direkt, `.h`-Dateien in `header\`.** Innerhalb jedes
  `src\<modul>\`-Ordners.
- **Keine Header-Kopien.** Header bleiben am Pflegeort; andere Module
  binden sie über `/I`-Pfade ein.
- **Keine OBJ-Reste.** Alle Zwischendateien landen in lokalen `_obj\`-
  Ordnern und werden nach dem Link gelöscht.
- **`tools\` ist separat.** Skripte, die nicht zum Kernel-Build gehören
  (Test-Runner, `pro_run`), liegen nicht in `bin\`, sondern in `tools\`.
- **`$(MAKEDIR)` statt CWD.** Alle Sub-Makefiles leiten ihre Pfade
  relativ zu ihrem eigenen Ablageort ab. Der Aufruf ist damit
  unabhängig vom aktuellen Arbeitsverzeichnis.

---

## 3. Die Build-Komponenten

### 3.1 Sub-Makefiles

Drei Module, jeweils ein eigenes Makefile:

| Datei | Baut | Doku |
|---|---|---|
| `build\prophysics\Makefile.nmake` | Kernel-DLL + Import-Lib | `docs\build\prophysics\Makefile.md` |
| `build\sdk\Makefile.sdk.nmake` | SDK-Interface-DLL + Import-Lib | `docs\build\sdk\Makefile.md` |
| `build\test\Makefile.nmake` | drei Test-EXEs | `docs\build\test\Makefile.md` |

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
| `all` (Default) | `setup` + `prophysics` + `sdk` + `test` + `info` |
| `prophysics` | nur Kernel |
| `sdk` | Kernel + SDK (Kettenabhängigkeit) |
| `test` | Kernel + SDK + Tests |
| `info` | nur `BUILD_INFO.txt` |
| `help` | Übersicht |
| `rebuild` | `clean` + `all` |
| `rebuild_prophysics` | `clean_prophysics` + `prophysics` + `info` |
| `rebuild_sdk` | `clean_sdk` + `sdk` + `info` |
| `rebuild_test` | `clean_test` + `test` + `info` |
| `clean` | bin\ + lib\ + BUILD_INFO.txt weg |
| `clean_prophysics` | nur Kernel-Artefakte |
| `clean_sdk` | nur SDK-Artefakte |
| `clean_test` | nur Test-Artefakte |

**Setup:** Legt `bin\` und `lib\` an, falls sie nicht existieren.

**CONFIG-Weitergabe:** `CONFIG=release|debug` wird an alle Sub-Makefiles
durchgereicht.

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
aus §3.3 und §3.4.

| Aktion | Ruft auf |
|---|---|
| `pro_run build` | `build\main\build.ps1` |
| `pro_run export` | `build\main\export.ps1` |
| `pro_run test` | `tools\run_alpha_tests.ps1` |
| `pro_run all` | build → test → export (Abbruch bei Fehlschlag) |
| `pro_run help` | Übersicht / `pro_run help <aktion>` |

**Doku:** `docs\build\pro_run.md`

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

:: Nur Test, ohne vorher zu bauen (Build existiert bereits)
pro_run all -NoBuild -NoExport -Prio 8

:: Hilfe
pro_run help
pro_run help export
```

### 4.2 Direkte Aufrufe (Legacy / Debug)

Die bestehenden Wrapper bleiben erhalten und rufen dieselben Skripte
auf. Nuetzlich, wenn nur eine einzelne Komponente gebaut werden soll,
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
```

### 4.3 Direkt im Sub-Ordner (nur nmake)

```cmd
cd build\prophysics
nmake /NOLOGO /f Makefile.nmake CONFIG=release

cd ..\sdk
nmake /NOLOGO /f Makefile.sdk.nmake CONFIG=release

cd ..\test
nmake /NOLOGO /f Makefile.nmake CONFIG=release
```

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
```

**Kettenprüfung:** SDK- und Test-Build haben Vorabprüfungen (`check_core`,
`check_deps`), die mit klarer Meldung abbrechen, wenn die Vorgängerlibs
fehlen.

Der Master-Build drückt die Kette durch Makefile-Abhängigkeiten aus:
`test: sdk`, `sdk: prophysics`. Ein `nmake sdk` baut also den Kernel
automatisch mit.

---

## 6. Was die Build-Skripte **nicht** tun

- **Kein Signing** im Sub-Makefile. Nur `build.ps1 -Sign` ruft `signtool`.
- **Kein Header-Export.** Header bleiben am Pflegeort.
- **Kein Deployment.** Kein Push in Repos oder Verzeichnisse.
- **Keine Test-Ausführung.** Nur `run_alpha_tests.ps1` / `pro_run test`.
- **Keine inkrementelle Header-Analyse.** Ein Header-Update rebuildet
  alle Module eines Sub-Makefiles (bewusst grob).

---

## 7. Konfiguration

### 7.1 Compiler-Flags

Pro Sub-Makefile definiert. Alle Sub-Makefiles kennen `CONFIG=release|debug`:

| Modul | Release | Debug |
|---|---|---|
| Kernel | `/W4 /O2 /Ob2 /Oi /GL /MP /arch:AVX2` + `/LTCG` | `/W4 /Od /Zi /MDd /MP` + `/DEBUG` |
| SDK | `/W3 /O2 /Ob2 /Oi /GL /MP` + `/LTCG` | `/W3 /Od /Zi /MDd /MP` + `/DEBUG` |
| Test | `/W3 /O2 /Ob2 /Oi /GL /MP /arch:AVX2` + `/LTCG` | `/W3 /Od /Zi /MDd /MP /arch:AVX2` + `/DEBUG` |

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
| `lib\ProPhysics.lib` | Kernel-Import-Lib |
| `lib\pro_sdk_interface.lib` | SDK-Import-Lib |
| `BUILD_INFO.txt` | Metadaten (Zeitstempel, Version, Etappe, Dateiliste) |

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

**Detail-Diagnose** pro Sub-Makefile: siehe jeweilige Doku-Seite.

---

## 10. Dokumentations-Struktur

```
docs\build\
├── BUILD_SCRIPT.md                # diese Datei
├── pro_run.md                     # NEU: zentraler Einstiegspunkt
├── main\
│   └── Makefile.md                # Master-Makefile
├── prophysics\
│   └── Makefile.md                # Kernel-Build
├── sdk\
│   └── Makefile.md                # SDK-Interface-Build
├── test\
│   └── Makefile.md                # Test-Build
└── helper\
    ├── build.md                   # build.ps1 / build.cmd
    ├── export.md                  # export.ps1 / export.cmd
    └── write_build_info.md        # write_build_info.ps1
```

Test-Dokumentation separat:

```
docs\test\
├── ProPhysics_Testkatalog.md      # alle 43 Tests
└── run_alpha_tests.md             # Test-Runner
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

---

## 11. Was passiert nach dem Build

Nach erfolgreichem `pro_run all` (Default) oder `build.cmd` (Default-Modus `all`):

1. `bin\` enthält 2 DLLs + 3 EXEs.
2. `lib\` enthält 2 LIBs.
3. `BUILD_INFO.txt` im Root ist aktuell (mit Version `1.23.0`, Etappe `23`).
4. Test-Runner und `pro_run` sind einsatzbereit in `tools\`.
5. `out\` enthält die angeforderten ZIP-Pakete.

**Nächster Schritt:**

```cmd
pro_run test -Prio all
```

**Erwartung:** 43/43 PASS, ~4 421 s (~74 min).
- Prio 5 (Hydrogen-48): ~2 281 s
- Prio 8 (Running-Coupling): ~1 398 s
- Alle anderen 41 Tests: ~742 s

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

Die Build-Kette selbst bleibt unverändert (`prophysics → sdk → test`).
Nur die Sub-Makefiles bekommen zusätzliche Einträge und die Wrapper
eine neue Dispatch-Schicht.

---

## 13. Siehe auch

| Thema | Datei |
|---|---|
| Zentraler Einstiegspunkt | `docs\build\pro_run.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Kernel-Build | `docs\build\prophysics\Makefile.md` |
| SDK-Build | `docs\build\sdk\Makefile.md` |
| Test-Build | `docs\build\test\Makefile.md` |
| build.ps1 / build.cmd | `docs\build\helper\build.md` |
| export.ps1 / export.cmd | `docs\build\helper\export.md` |
| write_build_info.ps1 | `docs\build\helper\write_build_info.md` |
| Testkatalog | `docs\test\ProPhysics_Testkatalog.md` |
| Test-Runner | `docs\test\run_alpha_tests.md` |
| Projekt-Roadmap | `docs\project\Project.md` |

---

**Ende Build-System-Übersicht v1.0.**