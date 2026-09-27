# ProPhysics SDK Interface — NMAKE Build

**Datei:** `build\sdk\Makefile.sdk.nmake`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Bau der SDK-Interface-DLL und ihrer Import-Lib aus der
Quelle in `src\sdk\`. Linkt gegen `ProPhysics.lib`.

---

## 1. Was gebaut wird

Aus **einer** `.c`-Datei entsteht **eine** DLL und **eine** Import-Lib:

| Artefakt | Pfad | Zweck |
|---|---|---|
| `pro_sdk_interface.dll` | `bin\pro_sdk_interface.dll` | Laufzeit-Bibliothek |
| `pro_sdk_interface.lib` | `lib\pro_sdk_interface.lib` | Import-Lib fuer nachgelagerte Builds (Tests) |

Zusaetzlich linkt der Build gegen `lib\ProPhysics.lib` — die Import-Lib
des Kernels. Diese muss **vor** diesem Build existieren.

**Kein** Header-Export. Der SDK-Header `pro_sdk_interface.h` bleibt am
Pflegeort in `src\sdk\header\`.

**Empfohlener Aufruf:** ueber `pro_run build -Mode sdk …` oder
`build.ps1`. Direkte `nmake`-Aufrufe bleiben gueltig. Siehe
`docs\build\pro_run.md`.

---

## 2. Ablageort und Aufruf

```
<repo>\
├── build\
│   └── sdk\
│       └── Makefile.sdk.nmake    <- dieses Makefile
├── src\
│   ├── sdk\
│   │   ├── pro_sdk_interface.c   <- Quelle
│   │   └── header\
│   │       └── pro_sdk_interface.h
│   └── prophysics\
│       └── header\*.h            <- Kernel-Header (eingebunden)
├── bin\
│   ├── ProPhysics.dll
│   └── pro_sdk_interface.dll     <- Ziel
└── lib\
    ├── ProPhysics.lib            <- Eingang
    └── pro_sdk_interface.lib     <- Ziel
```

**Aufruf** aus beliebigem CWD — die Pfade werden ueber `$(MAKEDIR)`
relativ zum Ablageort dieses Makefiles abgeleitet:

```cmd
cd build\sdk
nmake /NOLOGO /f Makefile.sdk.nmake CONFIG=release
```

**CWD-Unabhaengigkeit:** `$(MAKEDIR)` wird von NMAKE auf das
Verzeichnis gesetzt, aus dem das Makefile geladen wurde. Ein Aufruf
aus einem anderen CWD funktioniert damit ohne vorheriges `cd`:

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\sdk\Makefile.sdk.nmake
```

**Empfohlen** ist der Aufruf ueber `build.cmd` oder `pro_run build`,
weil die Wrapper die Konsole auf UTF-8 stellen.

---

## 3. Pfade und Symbole

| Symbol | Wert | Bemerkung |
|---|---|---|
| `CONFIG` | `release` \| `debug` | Default `release`, validiert |
| `ROOT` | `$(MAKEDIR)\..\..` | Repo-Root |
| `SRC_DIR` | `$(ROOT)\src\sdk` | SDK-Quelle |
| `HDR_DIR` | `$(ROOT)\src\sdk\header` | SDK-Header |
| `CORE_HDR` | `$(ROOT)\src\prophysics\header` | Kernel-Header |
| `OBJ_DIR` | `$(MAKEDIR)\_obj` | temporaere Objektdateien |
| `BIN_DIR` | `$(ROOT)\bin` | Ziel-DLL |
| `LIB_DIR` | `$(ROOT)\lib` | Ziel-LIB |
| `DLL_TARGET` | `$(BIN_DIR)\pro_sdk_interface.dll` | Ziel-DLL vollstaendig |
| `LIB_TARGET` | `$(LIB_DIR)\pro_sdk_interface.lib` | Ziel-LIB vollstaendig |
| `CORE_LIB` | `$(LIB_DIR)\ProPhysics.lib` | Kernel-Import-Lib (Eingang) |

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
| `release` | `/O2 /Ob2 /Oi /GL /W3 /MP` | `/LTCG` |
| `debug` | `/Od /Zi /MDd /W3 /MP` | `/DEBUG` |

Details siehe §6 und §7.

---

## 5. Targets

| Target | Wirkung |
|---|---|
| `all` (Default) | `check_core` + `setup` + DLL bauen |
| `check_core` | prueft, dass `$(CORE_LIB)` existiert |
| `setup` | `$(OBJ_DIR)`, `$(BIN_DIR)`, `$(LIB_DIR)` anlegen (idempotent) |
| `help` | Uebersicht der Targets + Optionen |
| `clean` | `$(OBJ_DIR)` rekursiv loeschen + SDK-DLL + SDK-LIB entfernen |

Es gibt keine separaten Targets pro Modul — es ist nur eine `.c`-Datei.

---

## 6. Compiler-Flags im Detail

Die Flags sind in zwei Teile getrennt: `CFLAGS_COMMON` (config-unabhaengig)
und `CFLAGS` (config-spezifisch).

### 6.1 Immer aktiv

```nmake
CFLAGS_COMMON = /nologo /W3 /MP /D_CRT_SECURE_NO_WARNINGS \
                /I"$(HDR_DIR)" /I"$(CORE_HDR)"
```

| Flag | Bedeutung |
|---|---|
| `/nologo` | kein Copyright-Banner |
| `/W3` | Warnstufe 3 (im Kernel: `/W4`) |
| `/MP` | Multi-Prozessor-Kompilierung (bei 1 Datei wirkungslos) |
| `/D_CRT_SECURE_NO_WARNINGS` | unterdrueckt MSVC-Warnungen ueber `fopen`, `strcpy` etc. |
| `/I"$(HDR_DIR)"` | SDK-eigener Header (`pro_sdk_interface.h`) |
| `/I"$(CORE_HDR)"` | Kernel-Header (`ProPhysics.h`, `Types`, `Config`) |

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

### 6.3 Warnstufe: `/W3` statt `/W4`

Der SDK-Build nutzt `/W3`, der Kernel `/W4`. Grund:
`pro_sdk_interface.c` enthaelt Test-/Analyse-Hilfen (BMP-Export,
ASCII-Renderer), die bewusst gegen einige `/W4`-Warnungen verstossen.
Der Kernel bleibt bei `/W4`.

### 6.4 DLL-Export

```nmake
DLL_FLAGS = /DPRO_SDK_EXPORTS
```

Das ist das SDK-Export-Macro. Es sorgt dafuer, dass `PRO_SDK_API` in
`pro_sdk_interface.h` zu `__declspec(dllexport)` expandiert.

---

## 7. Link

Der Link-Schritt verwendet eine eigene Variable `LFLAGS`, die von
`CONFIG` abhaengt:

```nmake
$(LINKER) /nologo /DLL $(LFLAGS) \
    /OUT:$(DLL_TARGET) \
    /IMPLIB:$(LIB_TARGET) \
    $(OBJ_SDK) $(CORE_LIB)
```

| Flag | CONFIG | Bedeutung |
|---|---|---|
| `/DLL` | beide | DLL statt EXE |
| `/LTCG` | `release` | Link-Time Code Generation (vollendet `/GL`) |
| `/DEBUG` | `debug` | Debug-Informationen ins Link-Ergebnis |
| `/OUT:` | beide | Pfad der DLL |
| `/IMPLIB:` | beide | Pfad der Import-Lib |
| Eingangsdateien | beide | eigene `.obj` **und** Kernel-Import-Lib |

MSVC legt zusaetzlich eine `.exp`-Datei ab. Sie landet ueblicherweise
neben der DLL (in `bin\`), nicht in `lib\`. Das Makefile laesst sie
stehen — sie wird nur gebraucht, wenn eine weitere DLL gegen dieselben
Exporte gelinkt wird. Soll sie weg, ergaenze im Post-Link-Schritt:

```nmake
@if exist "$(BIN_DIR)\pro_sdk_interface.exp" del /Q "$(BIN_DIR)\pro_sdk_interface.exp" 2>nul
```

---

## 8. Was nach dem Build wo liegt

```
<repo>\
├── bin\
│   ├── ProPhysics.dll              (aus prophysics-Build)
│   ├── pro_sdk_interface.dll       <- final, sauber
│   └── pro_sdk_interface.exp       (optional, siehe §7)
├── lib\
│   ├── ProPhysics.lib              (aus prophysics-Build)
│   └── pro_sdk_interface.lib       <- final, sauber
└── build\
    └── sdk\
        └── _obj\                   <- leer (nur Verzeichnis)
```

`bin\` ist flach — alle DLLs liegen nebeneinander. Das ist wichtig,
weil der Windows-Loader die Kernel-DLL sucht, wenn eine EXE gegen
die SDK-DLL gelinkt ist und beide im selben Verzeichnis liegen.

---

## 9. Clean

```cmd
nmake /NOLOGO /f Makefile.sdk.nmake clean
```

Loescht:

- `$(OBJ_DIR)` komplett (rekursiv)
- `bin\pro_sdk_interface.dll`
- `lib\pro_sdk_interface.lib`

**Nicht** angetastet werden:

- `bin\ProPhysics.dll` (gehoert dem Kernel-Build)
- `bin\example_*.exe` (gehoert dem Test-Build)
- `lib\ProPhysics.lib`
- `BUILD_INFO.txt` im Root

Sauberes Zusammenspiel mit `build\main\Makefile.nmake`, das sein
`clean_sdk`-Target auf dieses `clean` mappt.

---

## 10. Abhaengigkeits-Graph

```
check_core       (Pruefung: $(CORE_LIB) existiert)
      │
      ▼
setup            (mkdir $(OBJ_DIR)\, $(BIN_DIR)\, $(LIB_DIR)\)
      │
      ▼
+--------------------------+
| pro_sdk_interface.c      |
+--------------------------+
      │
      ▼
link.exe → bin\pro_sdk_interface.dll
           lib\pro_sdk_interface.lib
      │
      ▼
delete $(OBJ_DIR)\*.obj, $(OBJ_DIR)\*.exp
```

Nur ein Modul. Die Uebersetzung ist trivial, der eigentliche Schritt ist
der Link gegen die Kernel-Import-Lib.

---

## 11. Reihenfolge in der Build-Kette

```
build\prophysics   →  bin\ProPhysics.dll + lib\ProPhysics.lib
       │
       ▼
build\sdk          →  bin\pro_sdk_interface.dll       (dieses Makefile)
                      lib\pro_sdk_interface.lib
       │
       ▼
build\test         →  bin\example_*.exe
```

Der `check_core`-Schritt bricht mit klarer Meldung ab, wenn
`ProPhysics.lib` fehlt. Damit kann `nmake /f Makefile.sdk.nmake`
nicht versehentlich alleine laufen — es muss immer der Kernel-Build
vorausgegangen sein.

**Hinweis:** Der Master-Build drueckt diese Kette durch
Target-Abhaengigkeiten aus (`sdk: prophysics`), aber das Sub-Makefile
schuetzt sich zusaetzlich selbst mit `check_core`.

---

## 12. Wichtige Voraussetzungen

1. **`ProPhysics.lib` muss existieren.**
   Der `check_core`-Schritt prueft `$(CORE_LIB)`. Fehlt sie, bricht
   der Build ab mit:

   ```
   [FEHLER] <repo>\lib\ProPhysics.lib fehlt. Zuerst build\prophysics bauen.
   NMAKE : fatal error U1077: ... Rueckgabe-Code "0x1"
   ```

2. **Kernel-Header muessen in `$(CORE_HDR)` liegen.**
   Die `/I`-Pfade referenzieren dieses Verzeichnis. `pro_sdk_interface.c`
   inkludiert `ProPhysics.h` und `ProPhysics_Types.h` von dort.

3. **SDK-Header muss in `$(HDR_DIR)` liegen.**
   `pro_sdk_interface.h` — sonst bricht der Compiler mit
   „cannot open source file".

4. **MSVC-Toolchain im PATH.** Wie beim Kernel-Build.

---

## 13. Hilfe

```cmd
nmake /NOLOGO /f Makefile.sdk.nmake help
```

Gibt eine Uebersicht der Targets und der `CONFIG`-Option aus.
Rein informativ.

---

## 14. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `CONFIG muss "release" oder "debug" sein` | Tippfehler | Gross-/Kleinschreibung pruefen |
| `[FEHLER] ProPhysics.lib fehlt` | Kernel nicht gebaut | `pro_run build -Mode kernel` |
| `pro_sdk_interface.h: No such file` | SDK-Header fehlt | Header nach `src\sdk\header\` legen |
| `ProPhysics.h: No such file` | Kernel-Header fehlt | Header nach `src\prophysics\header\` legen |
| `unresolved external symbol ProPhysics_...` | Kernel-Export fehlt | `ProPhysics.h` pruefen, Kernel neu bauen |
| `LNK2019: _main` | `/DLL` fehlt | Link-Zeile pruefen |
| `fatal error U1077` beim Link | Linker-Ausgabe unterdrueckt | `nmake` ohne `/NOLOGO` laufen lassen |
| `.exp`-Datei in `bin\` stoert | MSVC-Detail | optional in Post-Link-Schritt loeschen (§7) |

---

## 15. Was dieses Makefile nicht tut

- **Keine Header-Kopien.** Alle Header bleiben am Pflegeort.
- **Kein Build_INFO.** Das schreibt der Master-Build in `build\main\`.
- **Kein Signing.** Das macht `build.ps1 -Sign`.
- **Keine Tests.** Die laufen ueber `tools\run_alpha_tests.ps1`
  oder `pro_run test`.
- **Kein Kernel-Build.** Der Kernel muss vorher separat gebaut sein.
- **Kein ZIP, kein Deployment.** Nur `export.ps1` packt Artefakte.

---

## 16. Parameter-Referenz

Das Makefile hat **einen Parameter** (`CONFIG`) plus Targets.

```
nmake [Target] [CONFIG=release|debug]
```

| Target | Wirkung |
|---|---|
| `all` (Default) | `check_core` + `setup` + DLL bauen |
| `check_core` | prueft, dass `ProPhysics.lib` existiert |
| `setup` | Ausgabe-Verzeichnisse anlegen |
| `help` | Uebersicht |
| `clean` | Artefakte entfernen |

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `CONFIG` | Choice | `release` | `release` \| `debug` |

---

## 17. Export-Layout des SDK

`pro_run export -Export sdk` erzeugt aus den SDK-Artefakten folgendes
Ausgabe-Layout:

```
out\sdk\
├── README.md
├── BUILD_INFO.txt
└── libs\
    ├── ProPhysics.dll
    ├── ProPhysics.lib
    ├── pro_sdk_interface.dll
    ├── pro_sdk_interface.lib
    ├── src\
    │   └── header\
    │       ├── prophysics\
    │       │   ├── ProPhysics.h
    │       │   ├── ProPhysics_Types.h
    │       │   └── ...
    │       └── sdk\
    │           └── pro_sdk_interface.h
    └── docs\
        └── ...
```

Das ist die Struktur, die an externe Empfaenger weitergereicht wird.
Header landen in Unterordnern, damit `#include`-Pfade konsistent
bleiben (`#include "prophysics/ProPhysics.h"`).

**ZIP:** Zusaetzlich wird `out\prophysics-sdk-<version>.zip` erzeugt.

Details siehe `docs\build\helper\export.md`.

---

## 18. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\test\Makefile.md` — Test-Build
- `docs\build\helper\export.md` — Export-Wrapper und SDK-Layout
- `src\sdk\header\pro_sdk_interface.h` — SDK-API
- `src\prophysics\header\ProPhysics.h` — Kernel-API (SDK-Nutzer lesen beide)

---

**Ende Makefile-Dokumentation (SDK Interface, v1.0.0).**