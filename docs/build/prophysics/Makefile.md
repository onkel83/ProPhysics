# ProPhysics Kernel — NMAKE Build

**Datei:** `build\prophysics\Makefile.nmake`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Bau der ProPhysics Kernel-DLL und ihrer Import-Lib aus den
Kernel-Quellen in `src\prophysics\`.

---

## 1. Was gebaut wird

Aus 12 `.c`-Modulen entsteht **eine** DLL und **eine** Import-Lib:

| Artefakt | Pfad | Zweck |
|---|---|---|
| `ProPhysics.dll` | `bin\ProPhysics.dll` | Laufzeit-Bibliothek |
| `ProPhysics.lib` | `lib\ProPhysics.lib` | Import-Lib fuer nachgelagerte Builds (SDK, Tests) |

**Kein** Header-Export. Die Header bleiben am Pflegeort in
`src\prophysics\header\` und werden von den anderen Builds ueber den
`/I`-Pfad eingebunden. Das verhindert doppelte Header-Kopien, die
auseinanderlaufen koennen.

**Empfohlener Aufruf:** ueber `pro_run build -Mode kernel …` oder
`build.ps1`. Direkte `nmake`-Aufrufe bleiben gueltig. Siehe
`docs\build\pro_run.md`.

---

## 2. Ablageort und Aufruf

```
<repo>\
├── build\
│   └── prophysics\
│       └── Makefile.nmake     <- dieses Makefile
├── src\
│   └── prophysics\
│       ├── *.c                <- 12 Kernel-Quellen
│       └── header\*.h         <- Header
├── bin\                       <- Ziel DLL
└── lib\                       <- Ziel LIB
```

**Aufruf** aus beliebigem CWD — die Pfade werden ueber `$(MAKEDIR)`
relativ zum Ablageort dieses Makefiles abgeleitet:

```cmd
cd build\prophysics
nmake /NOLOGO /f Makefile.nmake CONFIG=release
```

**CWD-Unabhaengigkeit:** `$(MAKEDIR)` wird von NMAKE auf das
Verzeichnis gesetzt, aus dem das Makefile geladen wurde. Ein Aufruf
aus einem anderen CWD funktioniert damit ohne vorheriges `cd`:

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\prophysics\Makefile.nmake
```

**Empfohlen** ist aber der Aufruf ueber `build.cmd` oder `pro_run build`,
weil die Wrapper die Konsole auf UTF-8 stellen.

---

## 3. Pfade und Symbole

| Symbol | Wert | Bemerkung |
|---|---|---|
| `CONFIG` | `release` \| `debug` | Default `release`, validiert |
| `ROOT` | `$(MAKEDIR)\..\..` | Repo-Root |
| `SRC_DIR` | `$(ROOT)\src\prophysics` | Kernel-Quellen |
| `HDR_DIR` | `$(ROOT)\src\prophysics\header` | Kernel-Header |
| `SDK_HDR` | `$(ROOT)\src\sdk\header` | SDK-Header (falls referenziert) |
| `OBJ_DIR` | `$(MAKEDIR)\_obj` | temporaere Objektdateien |
| `BIN_DIR` | `$(ROOT)\bin` | Ziel-DLL |
| `LIB_DIR` | `$(ROOT)\lib` | Ziel-LIB |
| `DLL_TARGET` | `$(BIN_DIR)\ProPhysics.dll` | Ziel-DLL vollstaendig |
| `LIB_TARGET` | `$(LIB_DIR)\ProPhysics.lib` | Ziel-LIB vollstaendig |

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
| `release` | `/O2 /Ob2 /Oi /GL /W4 /MP /arch:AVX2` | `/LTCG` |
| `debug` | `/Od /Zi /MDd /W4 /MP /arch:AVX2` | `/DEBUG` |

Details siehe §5 und §6.

---

## 5. Targets

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup` + DLL bauen |
| `setup` | `$(OBJ_DIR)`, `$(BIN_DIR)`, `$(LIB_DIR)` anlegen (idempotent) |
| `help` | Uebersicht der Targets + Optionen |
| `clean` | `$(OBJ_DIR)` rekusiv loeschen + DLL + LIB entfernen |

Es gibt keine separaten Targets pro Modul. Alle 12 `.c`-Dateien werden
in einem Lauf kompiliert und gelinkt.

---

## 6. Compiler-Flags im Detail

Die Flags sind in zwei Teile getrennt: `CFLAGS_COMMON` (config-unabhaengig)
und `CFLAGS` (config-spezifisch).

### 6.1 Immer aktiv

```nmake
CFLAGS_COMMON = /nologo /W4 /MP /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS \
                /I"$(HDR_DIR)" /I"$(SDK_HDR)"
```

| Flag | Bedeutung |
|---|---|
| `/nologo` | kein Copyright-Banner |
| `/W4` | Warnstufe 4 |
| `/MP` | Multi-Prozessor-Kompilierung (paralleler Build) |
| `/arch:AVX2` | SSE/AVX fuer die Q31-Arithmetik (Cache-Line-Align erfolgt im Kernel ueber `pro_aligned_calloc`) |
| `/D_CRT_SECURE_NO_WARNINGS` | unterdrueckt MSVC-Warnungen ueber `fopen`, `strcpy` etc. |
| `/I"$(HDR_DIR)"` | Kernel-Header (`ProPhysics.h`, `ProPhysics_Internal.h`, ...) |
| `/I"$(SDK_HDR)"` | SDK-Header (`pro_sdk_interface.h`, falls referenziert) |

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

### 6.3 DLL-Export

```nmake
DLL_FLAGS = /DPROPHYSICS_EXPORTS
```

Das ist das Export-Macro aus `ProPhysics_Exports.h`. Es sorgt dafuer,
dass `PROPHYSICS_API` zu `__declspec(dllexport)` expandiert.

---

## 7. Link

Der Link-Schritt verwendet eine eigene Variable `LFLAGS`, die von
`CONFIG` abhaengt:

```nmake
$(LINKER) /nologo /DLL $(LFLAGS) \
    /OUT:$(DLL_TARGET) \
    /IMPLIB:$(LIB_TARGET) \
    $(OBJ_KERNEL)
```

| Flag | CONFIG | Bedeutung |
|---|---|---|
| `/DLL` | beide | DLL statt EXE |
| `/LTCG` | `release` | Link-Time Code Generation (vollendet `/GL`) |
| `/DEBUG` | `debug` | Debug-Informationen ins Link-Ergebnis |
| `/OUT:` | beide | Pfad der DLL |
| `/IMPLIB:` | beide | Pfad der Import-Lib |

MSVC legt zusaetzlich eine `.exp`-Datei ab. Das Makefile loescht sie
zusammen mit den `.obj`-Dateien im `$(OBJ_DIR)`-Ordner direkt nach dem
Link.

---

## 8. Was nach dem Build wo liegt

```
<repo>\
├── bin\
│   └── ProPhysics.dll           <- final, sauber
├── lib\
│   └── ProPhysics.lib           <- final, sauber
└── build\
    └── prophysics\
        └── _obj\                <- leer (nur Verzeichnis)
```

**Kein Zwischenstand liegt ausserhalb von `_obj\`.** `bin\` ist flach
und enthaelt alle DLLs und EXEs nebeneinander (die SDK-DLL und die
Test-EXEs kommen aus ihren jeweiligen Sub-Makefiles dazu).

---

## 9. Clean

```cmd
nmake /NOLOGO /f Makefile.nmake clean
```

Loescht:

- `$(OBJ_DIR)` komplett (rekursiv)
- `bin\ProPhysics.dll`
- `lib\ProPhysics.lib`

**Nicht** angetastet werden:

- `bin\pro_sdk_interface.dll` (gehoert dem SDK-Build)
- `bin\example_*.exe` (gehoert dem Test-Build)
- `lib\pro_sdk_interface.lib`
- `BUILD_INFO.txt` im Root

Sauberes Zusammenspiel mit `build\main\Makefile.nmake`, das sein
`clean_prophysics`-Target auf dieses `clean` mappt.

---

## 10. Abhaengigkeits-Graph

```
setup
  │  mkdir $(OBJ_DIR)\, $(BIN_DIR)\, $(LIB_DIR)\
  ▼
+------+------+------+------+------+------+------+------+------+------+------+------+
| Core | Amp  | Dirac| SU2  |SU2_D | Gauge| EPR  | Obs  | Tens | Fock | Dens |Shrd |
+------+------+------+------+------+------+------+------+------+------+------+------+
        │
        ▼
   link.exe → bin\ProPhysics.dll + lib\ProPhysics.lib
        │
        ▼
   delete $(OBJ_DIR)\*.obj, $(OBJ_DIR)\*.exp
```

Jede Modulregel haengt von **allen** Headern ab. Ein Header-Update
rebuildet damit alle 12 Module. Das ist bewusst grob — bei einem
Projekt dieser Groesse spart das die `/showIncludes`-Verrenkung ohne
nennenswerten Nachteil (Build < 5 s).

---

## 11. Reihenfolge in der Build-Kette

```
build\prophysics   (dieses Makefile)   →  bin\ProPhysics.dll + lib\ProPhysics.lib
       │
       ▼
build\sdk                              →  bin\pro_sdk_interface.dll
                                          lib\pro_sdk_interface.lib
       │
       ▼
build\test                             →  bin\example_*.exe
```

Der Kernel ist die unterste Ebene. Seine Artefakte werden vom SDK-Build
(Link gegen `ProPhysics.lib`) und vom Test-Build (Link gegen beide
Libs) benoetigt.

---

## 12. Wichtige Voraussetzungen

1. **`ProPhysics_Internal.h` muss in `$(HDR_DIR)` liegen.**
   Sie wird von jeder `.c`-Datei inkludiert und ist im `HEADERS_ALL`
   aufgefuehrt. Fehlt sie, bricht der Build mit klarer Meldung ab.

2. **Alle 12 `.c`-Dateien muessen vorhanden sein.**
   Das Makefile listet sie explizit in `OBJ_KERNEL`:

   ```
   ProPhysics_Core.c
   ProPhysics_Amp.c
   ProPhysics_Dirac.c
   ProPhysics_SU2.c
   ProPhysics_SU2_Dynamics.c
   ProPhysics_Gauge.c
   ProPhysics_EPR.c
   ProPhysics_Observer.c
   ProPhysics_Tensor.c
   ProPhysics_Fock.c
   ProPhysics_Density.c
   ProPhysics_Shared.c
   ```

   Fehlt eine, meldet NMAKE sie als unbekannte Regel.

3. **MSVC-Toolchain im PATH.** `nmake.exe` und `cl.exe` muessen
   erreichbar sein. Am einfachsten ueber den *VS Developer Command
   Prompt* (oder `vcvars64.bat`).

4. **`/arch:AVX2` erfordert AVX2-faehige Hardware.** Falls der Build
   auf aelteren CPUs laufen soll, muss `/arch:AVX2` entfernt werden.

5. **Zweierpotenz-Regel gilt nicht fuer dieses Makefile.** Die Regel
   betrifft `grid_dim` zur Laufzeit, nicht den Build. Hier ist sie
   irrelevant.

---

## 13. Hilfe

```cmd
nmake /NOLOGO /f Makefile.nmake help
```

Gibt eine Uebersicht der Targets und der `CONFIG`-Option aus.
Rein informativ.

---

## 14. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `CONFIG muss "release" oder "debug" sein` | Tippfehler | Gross-/Kleinschreibung pruefen |
| `ProPhysics_Core.c: No such file` | `$(MAKEDIR)` falsch expandiert | Aufruf via `nmake /f` oder `build.cmd` |
| `ProPhysics_Internal.h: No such file` | Header liegt woanders | Header nach `src\prophysics\header\` verschieben |
| `unresolved external symbol` beim Link | fehlende `.c`-Datei im `OBJ_KERNEL` | `.c`-Datei in `OBJ_KERNEL`-Liste ergaenzen |
| `LNK2019: _main` | `/DLL` fehlt | Link-Zeile pruefen |
| Warnung `C4189 phase_q16_signed` | tote Variable in `ProPhysics_Gauge.c` | kosmetisch, kann entfernt werden |
| `fatal error U1077` beim Link | Linker-Ausgabe unterdrueckt | `nmake` ohne `/NOLOGO` laufen lassen |
| `/arch:AVX2` unrecognized | aeltere MSVC-Version | MSVC 2017+ verwenden oder Flag entfernen |

---

## 15. Was dieses Makefile nicht tut

- **Keine Header-Kopien.** Header bleiben in `src\prophysics\header\`.
- **Kein Build_INFO.** Das schreibt der Master-Build in `build\main\`.
- **Kein Signing.** Das macht `build.ps1 -Sign`.
- **Keine Unit-Tests.** Die laufen ueber `tools\run_alpha_tests.ps1`
  oder `pro_run test`.
- **Kein inkrementeller Header-Check.** Aenderung an einem Header
  rebuildet alle 12 Module.
- **Kein SDK- oder Test-Build.** Das machen die jeweiligen
  Sub-Makefiles.

---

## 16. Parameter-Referenz

Das Makefile hat **einen Parameter** (`CONFIG`) plus Targets.

```
nmake [Target] [CONFIG=release|debug]
```

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup` + DLL bauen |
| `setup` | Ausgabe-Verzeichnisse anlegen |
| `help` | Uebersicht |
| `clean` | Artefakte entfernen |

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `CONFIG` | Choice | `release` | `release` \| `debug` |

---

## 17. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\sdk\Makefile.md` — SDK-Interface-Build
- `docs\build\test\Makefile.md` — Test-Build
- `src\prophysics\header\ProPhysics.h` — oeffentliche API
- `src\prophysics\header\ProPhysics_Internal.h` — interne Modul-Schnittstellen

---

**Ende Makefile-Dokumentation (ProPhysics Kernel, v1.0.0).**