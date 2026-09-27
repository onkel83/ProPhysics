# ProPhysics Test Build — NMAKE Build

**Datei:** `build\test\Makefile.nmake`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Bau der drei Test-EXEs aus den Quellen in `src\test\`. Linkt
gegen `ProPhysics.lib` und `pro_sdk_interface.lib`.

---

## 1. Was gebaut wird

Aus 19 `.c`-Dateien (17 Alpha + 2 Example) entstehen **drei** EXEs:

| Artefakt | Pfad | Quellen | Rolle |
|---|---|---|---|
| `example_alpha_test.exe` | `bin\` | 17 Dateien (multi-file) | Alpha-Test-Suite (Prio 1–8, 43 Tests) |
| `example_test_density.exe` | `bin\` | 1 Datei | Dichte-Trilogie-Regression (86 Checks) |
| `example_test_tensor.exe` | `bin\` | 1 Datei | Tensor-/Fock-Regression (61 Checks) |

Alle drei EXEs werden **direkt neben die DLLs** in `bin\` gelegt. Der
Windows-Loader findet `ProPhysics.dll` und `pro_sdk_interface.dll` dann
automatisch beim Start — kein `PATH`-Eintrag, kein Kopieren.

**Keine** Header-Kopien. Header bleiben am Pflegeort in
`src\test\header\` und `src\prophysics\header\`.

**Empfohlener Aufruf:** ueber `pro_run build -Mode test …` oder
`build.ps1`. Direkte `nmake`-Aufrufe bleiben gueltig. Siehe
`docs\build\pro_run.md`.

---

## 2. Ablageort und Aufruf

```
<repo>\
├── build\
│   └── test\
│       └── Makefile.nmake         <- dieses Makefile
├── src\
│   ├── test\
│   │   ├── alpha_test_*.c          (17 Dateien)
│   │   ├── example_test_*.c        (2 Dateien)
│   │   └── header\
│   │       └── alpha_test_common.h
│   ├── prophysics\
│   │   └── header\*.h              (Kernel-Header)
│   └── sdk\
│       └── header\pro_sdk_interface.h
├── bin\
│   ├── ProPhysics.dll              (aus prophysics-Build)
│   ├── pro_sdk_interface.dll       (aus sdk-Build)
│   └── example_*.exe               <- Ziel
└── lib\
    ├── ProPhysics.lib              (Eingang)
    └── pro_sdk_interface.lib       (Eingang)
```

**Aufruf** aus beliebigem CWD — die Pfade werden ueber `$(MAKEDIR)`
relativ zum Ablageort dieses Makefiles abgeleitet:

```cmd
cd build\test
nmake /NOLOGO /f Makefile.nmake CONFIG=release
```

**CWD-Unabhaengigkeit:** `$(MAKEDIR)` wird von NMAKE auf das
Verzeichnis gesetzt, aus dem das Makefile geladen wurde. Ein Aufruf
aus einem anderen CWD funktioniert damit ohne vorheriges `cd`:

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\test\Makefile.nmake
```

**Empfohlen** ist der Aufruf ueber `build.cmd` oder `pro_run build`,
weil die Wrapper die Konsole auf UTF-8 stellen.

---

## 3. Pfade und Symbole

| Symbol | Wert | Bemerkung |
|---|---|---|
| `CONFIG` | `release` \| `debug` | Default `release`, validiert |
| `ROOT` | `$(MAKEDIR)\..\..` | Repo-Root |
| `SRC_DIR` | `$(ROOT)\src\test` | Test-Quellen |
| `TEST_HDR_DIR` | `$(ROOT)\src\test\header` | `alpha_test_common.h` |
| `CORE_HDR_DIR` | `$(ROOT)\src\prophysics\header` | Kernel-Header |
| `SDK_HDR_DIR` | `$(ROOT)\src\sdk\header` | SDK-Header |
| `OBJ_DIR` | `$(MAKEDIR)\_obj` | temporaere Objektdateien |
| `BIN_DIR` | `$(ROOT)\bin` | Ziel-EXEs |
| `LIB_DIR` | `$(ROOT)\lib` | Import-Libs (Eingang) |
| `CORE_LIB` | `$(LIB_DIR)\ProPhysics.lib` | Kernel-Lib |
| `SDK_LIB` | `$(LIB_DIR)\pro_sdk_interface.lib` | SDK-Lib |
| `EXE_ALPHA` | `$(BIN_DIR)\example_alpha_test.exe` | Ziel Alpha |
| `EXE_DENSITY` | `$(BIN_DIR)\example_test_density.exe` | Ziel Density |
| `EXE_TENSOR` | `$(BIN_DIR)\example_test_tensor.exe` | Ziel Tensor |

Alle Pfade sind **relativ zu `$(MAKEDIR)`**. Damit ist das Makefile
verschiebbar, solange die Repo-Struktur intakt bleibt.

---

## 4. Config-Validierung

```nmake
!IFNDEF CONFIG
CONFIG = release
!ENDIF
!IF "$(CONFIG)" != "release"
!IF "$(CONFIG)" != "debug"
!ERROR CONFIG muss "release" oder "debug" sein (erhalten: "$(CONFIG)")
!ENDIF
!ENDIF
```

Der Wert von `CONFIG` steuert die Compiler- und Linker-Flags. Ein
ungueltiger Wert bricht den Build ab, bevor der erste `cl.exe`-Aufruf
startet.

| `CONFIG` | Compiler-Flags | Linker-Flags |
|---|---|---|
| `release` | `/O2 /Ob2 /Oi /GL /W3 /MP /arch:AVX2` | `/LTCG` |
| `debug` | `/Od /Zi /MDd /W3 /MP /arch:AVX2` | `/DEBUG` |

Details siehe §6 und §7.

---

## 5. Targets

| Target | Wirkung |
|---|---|
| `all` (Default) | `check_deps` + `setup` + alle drei EXEs + `postclean` |
| `check_deps` | prueft, dass `$(CORE_LIB)` und `$(SDK_LIB)` existieren |
| `setup` | `$(OBJ_DIR)` und `$(BIN_DIR)` anlegen (idempotent) |
| `help` | Uebersicht der Targets + Optionen |
| `postclean` | raeumt `$(OBJ_DIR)\*.obj`, `*.exp`, `*.lib` ab |
| `clean` | `$(OBJ_DIR)`, alle drei EXEs entfernen |

Es gibt **keine** separaten Targets pro Test — der `all`-Target baut
alle drei EXEs, weil sie dieselben Header und Libs brauchen.

---

## 6. Compiler-Flags im Detail

Die Flags sind in zwei Teile getrennt: `CFLAGS_COMMON` (config-unabhaengig)
und `CFLAGS` (config-spezifisch).

### 6.1 Immer aktiv

```nmake
CFLAGS_COMMON = /nologo /W3 /MP /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS \
                /I"$(TEST_HDR_DIR)" \
                /I"$(CORE_HDR_DIR)" \
                /I"$(SDK_HDR_DIR)"
```

| Flag | Bedeutung |
|---|---|
| `/nologo` | kein Copyright-Banner |
| `/W3` | Warnstufe 3 (Kernel: `/W4`, SDK: `/W3`) |
| `/MP` | Multi-Prozessor-Kompilierung (parallel ueber die 17 Dateien) |
| `/arch:AVX2` | SSE/AVX fuer die Q31-Arithmetik (Konsistenz mit Kernel) |
| `/D_CRT_SECURE_NO_WARNINGS` | unterdrueckt MSVC-Warnungen ueber `fopen`, `strcpy` etc. |
| `/I"$(TEST_HDR_DIR)"` | `alpha_test_common.h` |
| `/I"$(CORE_HDR_DIR)"` | Kernel-Header |
| `/I"$(SDK_HDR_DIR)"` | SDK-Header |

### 6.2 Config-spezifisch

| `CONFIG` | Flags |
|---|---|
| `release` | `/O2 /Ob2 /Oi /GL` |
| `debug` | `/Od /Zi /MDd` |

Erklaerung:

| Flag | Bedeutung |
|---|---|
| `/O2` | maximale Optimierung |
| `/Ob2` | aggressives Inlining |
| `/Oi` | intrinsische Funktionen |
| `/GL` | Whole-Program-Optimization (Link-Time Code Generation) |
| `/Od` | Optimierung aus (Debug) |
| `/Zi` | Debug-Symbole erzeugen |
| `/MDd` | Debug-Runtime (DLL-Variante) |

**Kein** `DLL_FLAGS`. Die EXEs exportieren nichts — sie sind Endpunkte.

---

## 7. Drei EXEs, zwei Build-Muster

### 7.1 Single-File-EXEs (Density, Tensor)

`example_test_density.c` und `example_test_tensor.c` sind eigenstaendige
Programme mit `main()`.

```
cl.exe $(CFLAGS) "$(SRC_DIR)\example_test_density.c" \
    /Fo"$(OBJ_DIR)\\" /Fe$@ \
    /link $(LFLAGS) $(SDK_LIB) $(CORE_LIB)
```

MSVC kompiliert und linkt in **einem** Aufruf. `/Fe` setzt den EXE-Namen,
`/Fo` legt die `.obj` in `$(OBJ_DIR)`. Der `/link`-Abschnitt uebergibt
die Import-Libs.

### 7.2 Multi-File-EXE (Alpha-Test)

`example_alpha_test.exe` besteht aus **17 `.c`-Dateien**:

```
cl.exe $(CFLAGS) $(ALPHA_SOURCES) \
    /Fo"$(OBJ_DIR)\\" /Fe$@ \
    /link $(LFLAGS) $(SDK_LIB) $(CORE_LIB)
```

MSVC kompiliert alle 17 Dateien — bei aktivem `/MP` parallel — und
linkt sie am Ende. Die OBJs landen **alle** in `$(OBJ_DIR)`. Das ist die
einzige Stelle, an der `cl.exe` auch als Linker-Aufruf agiert.

**Wichtig:** `/Fo"$(OBJ_DIR)\\"` mit **abschliessendem Backslash** und
Quotes. Ohne Backslash haengt MSVC den Objektnamen direkt an den Pfad an
(`_objProPhysics_Core.obj` statt `_obj\ProPhysics_Core.obj`). Ohne Quotes
bricht der Aufruf bei Pfaden mit Leerzeichen.

---

## 8. Link

Zwei Import-Libs werden verlinkt:

```
/link $(LFLAGS) $(SDK_LIB) $(CORE_LIB)
```

- `$(SDK_LIB)` = `$(LIB_DIR)\pro_sdk_interface.lib`
- `$(CORE_LIB)` = `$(LIB_DIR)\ProPhysics.lib`

**Reihenfolge:** SDK-Lib **vor** Kernel-Lib. MSVC loest Symbole von
links nach rechts auf. Wenn die SDK-Lib auf Kernel-Symbole verweist
(was sie tut — sie nutzt `ProPhysics_*`-Funktionen), muss die
Kernel-Lib rechts stehen.

Die Reihenfolge ist nicht optional. Vertauscht man sie, kommt
`unresolved external symbol`.

**Linker-Flags** werden ueber `$(LFLAGS)` gesteuert:

| `CONFIG` | `$(LFLAGS)` |
|---|---|
| `release` | `/LTCG` (vollendet `/GL`) |
| `debug` | `/DEBUG` |

---

## 9. Post-Link-Aufraeumen

```nmake
postclean:
	@if exist "$(OBJ_DIR)\*.obj" del /Q "$(OBJ_DIR)\*.obj" 2>nul
	@if exist "$(OBJ_DIR)\*.exp" del /Q "$(OBJ_DIR)\*.exp" 2>nul
	@if exist "$(OBJ_DIR)\*.lib" del /Q "$(OBJ_DIR)\*.lib" 2>nul
	@echo [CLEANUP] OBJ/EXP/LIB-Reste entfernt.
```

MSVC legt beim Linken zusaetzliche Artefakte ab:

| Datei | Wo | Zweck |
|---|---|---|
| `.obj` | `$(OBJ_DIR)` | Objektdateien |
| `.exp` | `$(OBJ_DIR)` | Export-Datei (leer bei EXEs, aber trotzdem angelegt) |
| `.lib` | `$(OBJ_DIR)` | Import-Lib (leer bei EXEs) |

Alle drei Typen werden geloescht. Nach dem Build ist `$(OBJ_DIR)` leer —
nur der Verzeichniseintrag bleibt.

`postclean` laeuft nur nach dem erfolgreichen `all`-Target, nicht bei
einem `clean`.

---

## 10. Was nach dem Build wo liegt

```
<repo>\
├── bin\
│   ├── ProPhysics.dll              (aus prophysics-Build)
│   ├── pro_sdk_interface.dll       (aus sdk-Build)
│   ├── example_alpha_test.exe      <- final
│   ├── example_test_density.exe    <- final
│   └── example_test_tensor.exe     <- final
├── lib\
│   ├── ProPhysics.lib              (aus prophysics-Build)
│   └── pro_sdk_interface.lib       (aus sdk-Build)
└── build\
    └── test\
        └── _obj\                   <- leer (nur Verzeichnis)
```

`bin\` ist flach: DLLs und EXEs nebeneinander. Die EXE findet ihre
DLLs beim Start automatisch, weil beide im selben Verzeichnis liegen.

---

## 11. Clean

```cmd
nmake /NOLOGO /f Makefile.nmake clean
```

Loescht:

- `$(OBJ_DIR)` komplett (rekursiv)
- `bin\example_alpha_test.exe`
- `bin\example_test_density.exe`
- `bin\example_test_tensor.exe`

**Nicht** angetastet werden:

- `bin\ProPhysics.dll`
- `bin\pro_sdk_interface.dll`
- `lib\*.lib`
- `BUILD_INFO.txt` im Root

Sauberes Zusammenspiel mit `build\main\Makefile.nmake`, das sein
`clean_test`-Target auf dieses `clean` mappt.

---

## 12. Abhaengigkeits-Graph

```
check_deps       (Pruefung: beide Import-Libs existieren)
      │
      ▼
setup            (mkdir $(OBJ_DIR)\, $(BIN_DIR)\)
      │
      ├──> example_test_density.c   ──> example_test_density.exe
      │
      ├──> example_test_tensor.c    ──> example_test_tensor.exe
      │
      └──> alpha_test_main.c    ┐
           alpha_test_common.c  │
           alpha_test_basic.c   │
           ... (17 Dateien)     ├──> example_alpha_test.exe
           alpha_test_su2.c     │
           alpha_test_running_  │
           coupling.c           │
                                ┘
      │
      ▼
postclean        (delete $(OBJ_DIR)\*.obj, *.exp, *.lib)
```

Alle drei EXEs haengen von allen Headern ab. Header-Aenderung rebuildet
alles. Der `postclean`-Schritt laeuft erst nach dem letzten Link.

---

## 13. Reihenfolge in der Build-Kette

```
build\prophysics   →  bin\ProPhysics.dll + lib\ProPhysics.lib
       │
       ▼
build\sdk          →  bin\pro_sdk_interface.dll + lib\pro_sdk_interface.lib
       │
       ▼
build\test         →  bin\example_*.exe       (dieses Makefile)
```

Der `check_deps`-Schritt bricht ab, wenn eine der beiden Import-Libs
fehlt. Damit kann der Test-Build nicht versehentlich ohne die
Vorgaenger laufen.

---

## 14. Wichtige Voraussetzungen

1. **Beide Import-Libs muessen existieren.**
   `$(LIB_DIR)\ProPhysics.lib` und `$(LIB_DIR)\pro_sdk_interface.lib`.
   Sonst bricht `check_deps` ab mit:

   ```
   [FEHLER] <repo>\lib\ProPhysics.lib fehlt. Zuerst build\prophysics bauen.
   NMAKE : fatal error U1077: ... Rueckgabe-Code "0x1"
   ```

2. **`alpha_test_common.h` muss in `$(TEST_HDR_DIR)` liegen.**
   Sie wird von allen `alpha_test_*.c` inkludiert.

3. **Alle 17 Alpha-Test-Quellen muessen vorhanden sein.**
   Das Makefile listet sie explizit im `ALPHA_SOURCES`-Block. Fehlt eine,
   meldet NMAKE sie als unbekannte Regel — oder `cl.exe` bricht mit
   `cannot open source file` ab.

4. **MSVC-Toolchain im PATH.** Wie bei den beiden anderen Builds.

5. **Kernel- und SDK-Header** muessen an ihren Pflegeorten liegen
   (`src\prophysics\header\`, `src\sdk\header\`).

6. **`/arch:AVX2` erfordert AVX2-faehige Hardware.** Falls der Build
   auf aelteren CPUs laufen soll, muss `/arch:AVX2` entfernt werden.

---

## 15. Hilfe

```cmd
nmake /NOLOGO /f Makefile.nmake help
```

Gibt eine Uebersicht der Targets und der `CONFIG`-Option aus.
Rein informativ.

---

## 16. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `CONFIG muss "release" oder "debug" sein` | Tippfehler | Gross-/Kleinschreibung pruefen |
| `[FEHLER] ProPhysics.lib fehlt` | Kernel nicht gebaut | `pro_run build -Mode kernel` |
| `[FEHLER] pro_sdk_interface.lib fehlt` | SDK nicht gebaut | `pro_run build -Mode sdk` |
| `alpha_test_common.h: No such file` | Header fehlt | Header nach `src\test\header\` |
| `unresolved external symbol ProPhysics_...` | Link-Reihenfolge | SDK-Lib muss **vor** Kernel-Lib stehen |
| `example_*.obj in CWD` statt `$(OBJ_DIR)` | fehlender Backslash in `/Fo` | `/Fo"$(OBJ_DIR)\\"` pruefen |
| `LNK2019: _main` | keine `main()` in einer Datei | `alpha_test_main.c` oder `example_test_*.c` pruefen |
| `fatal error U1077` beim Link | Linker-Ausgabe unterdrueckt | `nmake` ohne `/NOLOGO` |
| Zeitueberschreitung bei `example_alpha_test.exe` | Hydrogen-48 laeuft ~35 min | Timeout in `run_alpha_tests.ps1` pruefen (Prio 5: 7200 s) |
| EXE findet DLL nicht | DLL nicht in `bin\` | Kernel + SDK zuerst bauen |
| `/arch:AVX2` unrecognized | aeltere MSVC-Version | MSVC 2017+ verwenden oder Flag entfernen |

---

## 17. Was dieses Makefile nicht tut

- **Keine Header-Kopien.** Alle Header bleiben am Pflegeort.
- **Kein Build_INFO.** Das schreibt der Master-Build in `build\main\`.
- **Kein Signing.** Das macht `build.ps1 -Sign`.
- **Keine Test-Ausfuehrung.** Der Build erzeugt EXEs, fuehrt sie aber
  nicht aus. Die Test-Laeufe macht `tools\run_alpha_tests.ps1` oder
  `pro_run test`.
- **Keine separaten Targets pro Test.** Alle drei EXEs werden in
  einem Lauf gebaut.
- **Kein ZIP, kein Deployment.** Nur `export.ps1` packt Artefakte.

---

## 18. Parameter-Referenz

Das Makefile hat **einen Parameter** (`CONFIG`) plus Targets.

```
nmake [Target] [CONFIG=release|debug]
```

| Target | Wirkung |
|---|---|
| `all` (Default) | `check_deps` + `setup` + drei EXEs + `postclean` |
| `check_deps` | prueft, dass beide Import-Libs existieren |
| `setup` | Ausgabe-Verzeichnisse anlegen |
| `help` | Uebersicht |
| `postclean` | OBJ/EXP/LIB-Reste entfernen |
| `clean` | `$(OBJ_DIR)` + drei EXEs entfernen |

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `CONFIG` | Choice | `release` | `release` \| `debug` |

---

## 19. Die drei EXEs im Detail

### 19.1 `example_alpha_test.exe`

Die Haupt-Test-Suite. Ruft je nach CLI-Flag einen einzelnen Test auf:

```cmd
example_alpha_test.exe --test-spin-half
example_alpha_test.exe --test-dirac
```

Verfuegbare Flags:

| Prio | Tests | CLI-Flags |
|---|---|---|
| 1 | 12 | `--test-amp`, `--test-born`, `--test-unitary`, `--test-context`, `--test-wilson`, `--test-gauge`, `--test-triangle`, `--test-chsh`, `--test-chsh-collapse`, `--test-chsh-graph`, + 2 Example-Tests |
| 2 | 10 | `--test-superdet`, `--test-observer-chsh`, `--test-chsh-diffusion`, `--test-chsh-wave`, `--test-born-emergent`, `--test-born-local`, `--test-born-equiv`, `--test-qm-basics`, `--test-qm-advanced`, `--test-qm-emergent` |
| 3 | 10 | `--test-amp-invariant`, `--test-amp-invariant-colored`, `--test-amp-invariant-bisect`, `--test-edge-transport`, `--test-edge-transport-colored`, `--test-edge-transport-scaling`, `--test-wave-packet`, `--test-soliton`, `--test-lorentz`, `--test-no-signaling` |
| 4 | 3 | `--test-3d-smoke`, `--test-3d-invariance`, `--test-3d-dispersion` |
| 5 | 4 | `--test-hydrogen`, `--test-hydrogen-48`, `--test-shared-reference`, `--test-shared-formula-tournament` |
| 6 | 1 | `--test-spin-half` |
| 7 | 1 | `--test-dirac` |
| 8 | 2 | `--test-su2-wilson-loop`, `--test-running-coupling` |

Ohne Flag laeuft der Datenmodus (CSV-Ausgabe), der fuer die Alpha-Tests
nicht genutzt wird.

**Gesamt:** 43 Tests.

### 19.2 `example_test_density.exe`

Dichte-Trilogie-Regression: 8x8-Knoten-Dichte, 64x64-Tensor-Dichte,
256x256-Fock-Dichte mit Lindblad-Kanaelen. 86 Checks. Kein CLI-Argument.
Exit-Code 0 bei allen PASS.

### 19.3 `example_test_tensor.exe`

Tensor-/Fock-Regression: Popcount, Vakuum, Basis-Zustaende, Erzeuger/
Vernichter, Jordan-Wigner-Vorzeichen, Antikommutatoren, Hopping,
Tensor-Fock-Roundtrip. 61 Checks. Kein CLI-Argument.

---

## 20. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\sdk\Makefile.md` — SDK-Interface-Build
- `docs\test\ProPhysics_Testkatalog.md` — Test-Uebersicht
- `docs\test\run_alpha_tests.md` — Test-Runner
- `tools\run_alpha_tests.ps1` — Test-Runner-Skript

---

**Ende Makefile-Dokumentation (Test Build, v1.0.0).**