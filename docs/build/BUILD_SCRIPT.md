# ProPhysics Build & Package — Übersicht

**Stand:** 2026-09-25 (Etappe 23)
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
- **Export-Funktionen** — kopieren Artefakte in `out\` für externe Weitergabe.
- **BUILD_INFO.txt** — Metadaten im Projekt-Root.

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
│   │   ├── *.c                 (13 Module)
│   │   └── header\*.h
│   ├── sdk\
│   │   ├── pro_sdk_interface.c
│   │   └── header\pro_sdk_interface.h
│   └── test\
│       ├── alpha_test_*.c      (18 Module)
│       ├── example_test_*.c    (2 Module)
│       └── header\alpha_test_common.h
│
├── tools\                      # Test-Runner (nicht Teil des Kernel-Builds)
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
    ├── exe\
    ├── sdk\
    └── src\<name>\
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
  (Test-Runner), liegen nicht in `bin\`, sondern in `tools\` (Refactoring 22).

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
- Wird aus seinem eigenen Ablageort ausgeführt (`cd build\<modul>`).
- Verwendet relative Pfade `..\..\src\<modul>\` und `..\..\bin\`.
- Legt seine Zwischendateien in `_obj\` ab.
- Hat ein `clean`-Target, das nur seine eigenen Artefakte entfernt.

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
| `rebuild` | `clean` + `all` |
| `rebuild_prophysics` | `clean_prophysics` + `prophysics` + `info` |
| `rebuild_sdk` | `clean_sdk` + `sdk` + `info` |
| `rebuild_test` | `clean_test` + `test` + `info` |
| `clean` | bin\ + lib\ + BUILD_INFO.txt weg |
| `clean_prophysics` | nur Kernel-Artefakte |
| `clean_sdk` | nur SDK-Artefakte |
| `clean_test` | nur Test-Artefakte |

**Setup:** Legt `bin\` und `lib\` an, falls sie nicht existieren.

**Doku:** `docs\build\main\Makefile.md`

### 3.3 PowerShell-Wrapper

Drei Skripte in `build\main\`, die nmake-Aufrufe kapseln und zusätzliche
Funktionen bieten.

| Skript | Rolle | Doku |
|---|---|---|
| `build.ps1` + `build.cmd` | Build-Wrapper mit Modus/Flags | `docs\build\helper\build.md` |
| `export.ps1` + `export.cmd` | Build + Export in `out\` | `docs\build\helper\export.md` |
| `write_build_info.ps1` | BUILD_INFO.txt-Generator | `docs\build\helper\write_build_info.md` |

### 3.4 Test-Runner

Liegt seit Refactoring 22 in `tools\`, nicht mehr in `bin\`. Self-Locating,
nimmt alle 43 Tests in den Prios 1–8.

| Datei | Rolle | Doku |
|---|---|---|
| `tools\run_alpha_tests.ps1` + `.cmd` | Test-Runner | `docs\test\run_alpha_tests.md` |

Der Runner ist self-locating: `-ExeDir` Default = `<repo>\bin`,
`-LogDir` Default = `<ExeDir>\logs`.

**Neu in Etappe 23:** Prio 8 enthält jetzt zwei Tests
(`SU2-Wilson-Loop` und `Running-Coupling`). Der zweite läuft ~23 min
und ist nicht CI-tauglich. Siehe `docs\test\run_alpha_tests.md` §16
für CI-Empfehlungen.

---

## 4. Typische Aufrufe

### 4.1 Vom Projekt-Root

```cmd
:: Kompletter Build + BUILD_INFO
build\main\build.cmd

:: Nur Kernel
build\main\build.cmd -Mode prophysics

:: Rebuild + Git-Info
build\main\build.cmd -Mode all -Rebuild -GitStamp -GitNote "Release v3.3"

:: Alles weg
build\main\build.cmd -Clean

:: Export als SDK-Paket
build\main\export.cmd sdk -Scope all -Clean
```

### 4.2 Test

```cmd
cd tools
run_alpha_tests.cmd -Prio 8         :: beide SU(2)-Tests (~24 min)
run_alpha_tests.cmd -Prio all       :: alle 43 Tests (~74 min)
```

### 4.3 Direkt im Sub-Ordner (Debug)

```cmd
cd build\prophysics
nmake /NOLOGO /f Makefile.nmake

cd ..\sdk
nmake /NOLOGO /f Makefile.sdk.nmake

cd ..\test
nmake /NOLOGO /f Makefile.nmake
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
- **Kein Zip.** Nur `export.ps1 -Zip` (geplant) archiviert.
- **Kein Header-Export.** Header bleiben am Pflegeort.
- **Kein Deployment.** Kein Push in Repos oder Verzeichnisse.
- **Keine Test-Ausführung.** Nur `run_alpha_tests.ps1`.
- **Keine inkrementelle Header-Analyse.** Ein Header-Update rebuildet
  alle Module eines Sub-Makefiles (bewusst grob).

---

## 7. Konfiguration

### 7.1 Compiler-Flags

Pro Sub-Makefile definiert. Kernel nutzt `/W4 /GL /LTCG /arch:AVX2`, SDK
und Tests `/W3` (bewusst milder wegen Test-Harness), Tests ebenfalls
`/arch:AVX2`.

### 7.2 Pfade

Alle Sub-Makefiles verwenden ausschließlich **relative Pfade** zu ihrem
eigenen Ablageort. Damit ist der Aufruf unabhängig vom CWD.

**Ausnahme:** Der Master-Makefile nutzt `$(MAKEDIR)` für den Repo-Root
und wechselt mit `cd /d` in die Sub-Ordner, bevor er deren nmake aufruft.

### 7.3 `dim`-Regel

R1: `dim` muss Zweierpotenz sein. Gilt zur Laufzeit (Gitter-Konfiguration),
nicht beim Build. Aktuell verwendet: `dim ∈ {16, 32, 64, 128}`.

### 7.4 Q31 vs. Q30 (Kernel)

Der Kernel arbeitet mit **Q31** für Amplituden (Skala `INT32_MAX`) und
**Q30** für SU(2)-Links (Skala `2^30`, um int64-Overflow in der
Quaternion-Multiplikation zu vermeiden). Diese Dualität ist stabil und
in `ProPhysics_Config.h` dokumentiert.

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
| `BUILD_INFO.txt` | Metadaten (Zeitstempel, Version, Dateiliste) |

### 8.2 Nach `export.ps1` (Modi)

| Modus | Ziel |
|---|---|
| `exe` | `out\exe\` — DLLs + EXEs flach, mit Runner aus `tools\` |
| `sdk` | `out\sdk\libs\` — DLLs + LIBs + Header |
| `src` | `out\src\<name>\` — Quellcode-Snapshot inkl. `tools\` |

Details siehe `docs\build\helper\export.md`.

---

## 9. Fehlersuche — Erste Anlaufstellen

| Symptom | Wahrscheinliche Ursache | Erster Blick |
|---|---|---|
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` aufrufen |
| `ProPhysics.lib fehlt` | Kernel nicht gebaut | `build\prophysics` ausführen |
| `pro_sdk_interface.lib fehlt` | SDK nicht gebaut | `build\sdk` ausführen |
| `*.h not found` | Header am falschen Ort | `src\<modul>\header\` prüfen |
| `unresolved external symbol ProPhysics_...` | Kernel-Lib fehlt oder Link-Reihenfolge falsch | SDK-Lib **vor** Kernel-Lib in Link-Zeile |
| `unresolved external symbol ProPhysics_SU2_...` | SU2_Dynamics.c nicht im `OBJ_KERNEL` | `build\prophysics\Makefile.nmake` prüfen |
| `unresolved external symbol ProPhysics_SU2_Link_Plaquette_Sum` | Neue Funktion nicht in `ProPhysics_SU2_Dynamics.c` oder nicht ins `.obj` eingebunden | Kernel-Neubau: `nmake rebuild_prophysics` |
| Test-EXE startet nicht | DLL nicht in `bin\` | Kernel + SDK zuerst bauen |
| Test-EXE „DLL nicht gefunden" | EXE und DLL in verschiedenen Ordnern | beide müssen in `bin\` liegen |
| Umlaute kaputt in Konsole | `chcp` fehlt | `*.cmd`-Wrapper statt direkt aufrufen |
| `BUILD_INFO.txt fehlt` | `-Clean` gesetzt | `build.cmd -NoBuild` erneut laufen |
| Runner findet Test-EXE nicht | `-ExeDir` falsch | Default ist `<repo>\bin`, prüfen |
| `Running-Coupling TIMEOUT` | Rechner zu langsam oder Creutz-Ratio dazugekommen | Timeout auf 3600 s erhöhen (`run_alpha_tests.ps1`) |

**Detail-Diagnose** pro Sub-Makefile: siehe jeweilige Doku-Seite.

---

## 10. Dokumentations-Struktur

```
docs\build\
├── BUILD_SCRIPT.md                # diese Datei
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
3. `docs\build\<modul>\Makefile.md` — je nachdem, welches Modul dich interessiert.
4. `docs\build\helper\build.md` — wie man den Build benutzt.
5. `docs\test\ProPhysics_Testkatalog.md` — was getestet wird.
6. `docs\test\run_alpha_tests.md` — wie man die Tests fährt.

---

## 11. Was passiert nach dem Build

Nach erfolgreichem `build.cmd` (Default-Modus `all`):

1. `bin\` enthält 2 DLLs + 3 EXEs.
2. `lib\` enthält 2 LIBs.
3. `BUILD_INFO.txt` im Root ist aktuell.
4. Test-Runner sind einsatzbereit in `tools\`.

**Nächster Schritt:**

```cmd
cd tools
run_alpha_tests.cmd -Prio all
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
- **Neue Kernel-Datei** `ProPhysics_SU2.c` (12. Modul).
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
- **Neue Kernel-Datei** `ProPhysics_SU2_Dynamics.c` (13. Modul).
- **Neue public API:** `Enable_SU2_Dynamics`, `Disable_SU2_Dynamics`,
  `Is_SU2_Dynamics_Active`, `Set_SU2_Yang_Mills`, `Apply_SU2_Tick`,
  `SU2_Plaquette_Action`, `SU2_Total_Energy`, `SU2_Link_Plaquette_Sum`.
- **Neue `ProUniverse`-Felder:** `su2_dynamics_active`, `su2_yang_mills_q15`.
- **Neue Config-Konstanten:** `PRO_SU2_YM_DEFAULT_Q15`,
  `PRO_SU2_LEAPFROG_DT_Q15`.
- **`Internal.h`** — `pro_su2_mul/conj/norm_sq` zentral; neue
  `pro_su2_exp_apply`.
- **Master-Makefile** — `ProPhysics_SU2_Dynamics.obj` in `OBJ_KERNEL`.
- **Test-Makefile** — keine neue Datei, aber `alpha_test_running_coupling.c`
  als Vorbereitung für Etappe 23 (bereits registriert, ggf. inaktiv).
- **Test-Runner** — Prio 8 um T15–T18 erweitert; neuer Test
  `--test-running-coupling` vorgesehen.

### 12.3 — Etappe 23: SU(2)-Metropolis / Wilson-Action-Validierung

- **Neue Kernel-Funktion:** `ProPhysics_SU2_Link_Plaquette_Sum` (read-only).
  Kein Struct-Change, kein neues Modul, keine Config-Erweiterung.
- **Neuer Test:** `alpha_test_running_coupling.c` (18. Test-Modul).
- **Test-Makefile** — `alpha_test_running_coupling.c` in `ALPHA_SOURCES`.
- **Test-Runner** — Prio 8 enthält beide Tests; Timeout für
  Running-Coupling auf 2400 s.
- **Backward-Staple-Fix in 22b** (in Etappe 23 verifiziert):
  T16 von 8,06e-03 auf 2,44e-03, T15 von 2,02e-08 auf 1,80e-08.
- **V&V-Anker (NEU):** Erste absolute Validierung gegen externe
  Lattice-QCD-Physik. ⟨P⟩(β=2) = 0,43346 vs. Referenz I₂(2)/I₁(2) =
  0,43313, Abweichung **0,08 %**.

Die Build-Kette selbst bleibt unverändert (`prophysics → sdk → test`).
Nur die Sub-Makefiles bekommen zusätzliche Einträge.

---

## 13. Siehe auch

| Thema | Datei |
|---|---|
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

**Ende Build-System-Übersicht v3.3.**