# ProPhysics ProWB — NMAKE Build

**Datei:** `build\prowb\Makefile.nmake`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Bau des ProWB-Web-Docs-Builders (`bin\prowb\prowb.exe`)
aus den Quellen in `src\prowb\`.

---

## §1 — Was gebaut wird

Aus **zwei** `.c`-Dateien entsteht **eine** EXE:

| Artefakt | Pfad | Zweck |
|---|---|---|
| `prowb.exe` | `bin\prowb\prowb.exe` | Web-Docs-Builder (Standalone-Tool) |

**Keine DLL, keine Import-Lib.** ProWB ist ein Build-Werkzeug, kein
Runtime-Artefakt. Es wird nicht von anderen Modulen gelinkt.

**Keine Header-Kopien.** Die Header `prowb.h` und `md_parser.h`
bleiben in `src\prowb\header\` und werden vom Compiler über
`/I`-Pfade eingebunden.

**Ziel-Verzeichnis `bin\prowb\` ist separat** von `bin\` (wo die
Kernel-DLLs und Test-EXEs liegen). Grund: ProWB ist ein Build-Tool
und soll vom Kernel-`clean` unabhängig sein.

**Empfohlener Aufruf:** über `pro_run web …` in `tools\`. Direkte
`nmake`-Aufrufe bleiben gültig.

---

## §2 — Ablageort und Aufruf

```
<repo>\
├── build\
│   └── prowb\
│       └── Makefile.nmake        <- dieses Makefile
├── src\
│   └── prowb\
│       ├── prowb.c               <- Builder-Kern
│       ├── md_parser.c           <- GFM-Parser
│       └── header\
│           ├── prowb.h
│           └── md_parser.h
└── bin\
    └── prowb\
        └── prowb.exe             <- Ziel
```

**Aufruf** aus beliebigem CWD — die Pfade werden über `$(MAKEDIR)`
relativ zum Ablageort dieses Makefiles abgeleitet:

```cmd
cd build\prowb
nmake /NOLOGO /f Makefile.nmake CONFIG=release
```

**CWD-Unabhängigkeit:** `$(MAKEDIR)` wird von NMAKE auf das
Verzeichnis gesetzt, aus dem das Makefile geladen wurde. Ein Aufruf
aus einem anderen CWD funktioniert damit ohne vorheriges `cd`:

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\prowb\Makefile.nmake
```

**Empfohlen** ist der Aufruf über `build.cmd` oder `pro_run web`,
weil die Wrapper die Konsole auf UTF-8 stellen.

---

## §3 — Pfade und Symbole

| Symbol | Wert | Bemerkung |
|---|---|---|
| `CONFIG` | `release` \| `debug` | Default `release`, validiert |
| `ROOT` | `$(MAKEDIR)\..\..` | Repo-Root |
| `SRC_DIR` | `$(ROOT)\src\prowb` | Builder-Quellen |
| `HDR_DIR` | `$(ROOT)\src\prowb\header` | Builder-Header |
| `OBJ_DIR` | `$(MAKEDIR)\_obj` | temporäre Objektdateien |
| `BIN_DIR` | `$(ROOT)\bin\prowb` | Ziel-EXE |
| `EXE_TARGET` | `$(BIN_DIR)\prowb.exe` | Ziel-EXE vollständig |

Alle Pfade sind **relativ zu `$(MAKEDIR)`**. Damit ist das Makefile
verschiebbar, solange die Repo-Struktur intakt bleibt.

**Warum `bin\prowb\` und nicht `bin\`?** `bin\` ist flach, damit der
Windows-Loader Kernel-DLLs und Test-EXEs ohne `PATH`-Eintrag findet.
ProWB braucht diese Nachbarschaft nicht — es ist ein eigenständiges
Tool. Getrennte Verzeichnisse halten den Kernel-`clean` sauber.

---

## §4 — Config-Validierung

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
ungültiger Wert bricht den Build ab, bevor der erste `cl.exe`-Aufruf
startet.

| `CONFIG` | Compiler-Flags | Linker-Flags |
|---|---|---|
| `release` | `/O2 /Ob2 /Oi /GL /W4 /MP` | `/LTCG` |
| `debug` | `/Od /Zi /MDd /W4 /MP` | `/DEBUG` |

Details siehe §6 und §7.

**Kein `/arch:AVX2`.** ProWB ist IO-lastig (Datei lesen, parsen,
schreiben), nicht compute-lastig. Der Kernel nutzt `/arch:AVX2` für
die Q31-Arithmetik; ProWB braucht das nicht.

---

## §5 — Targets

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup` + EXE bauen |
| `setup` | `$(OBJ_DIR)`, `$(BIN_DIR)` anlegen (idempotent) |
| `help` | Übersicht der Targets + Optionen |
| `clean` | `$(OBJ_DIR)` rekursiv löschen + EXE entfernen |

Es gibt **keine** separaten Targets pro Modul. Beide `.c`-Dateien
werden in einem Lauf kompiliert und gelinkt.

---

## §6 — Compiler-Flags im Detail

Die Flags sind in zwei Teile getrennt: `CFLAGS_COMMON`
(config-unabhängig) und `CFLAGS` (config-spezifisch).

### §6.1 — Immer aktiv

```nmake
CFLAGS_COMMON = /nologo /W4 /MP /D_CRT_SECURE_NO_WARNINGS \
                /I"$(HDR_DIR)"
```

| Flag | Bedeutung |
|---|---|
| `/nologo` | kein Copyright-Banner |
| `/W4` | Warnstufe 4 (wie Kernel) |
| `/MP` | Multi-Prozessor-Kompilierung (bei 2 Dateien wirkungslos, aber konsistent) |
| `/D_CRT_SECURE_NO_WARNINGS` | unterdrückt MSVC-Warnungen über `fopen`, `strcpy` etc. |
| `/I"$(HDR_DIR)"` | Builder-Header (`prowb.h`, `md_parser.h`) |

### §6.2 — Config-spezifisch

| `CONFIG` | Flags |
|---|---|
| `release` | `/O2 /Ob2 /Oi /GL` |
| `debug` | `/Od /Zi /MDd` |

Erklärung:

| Flag | Bedeutung |
|---|---|
| `/O2` | maximale Optimierung |
| `/Ob2` | aggressives Inlining |
| `/Oi` | intrinsische Funktionen |
| `/GL` | Whole-Program-Optimization (Link-Time Code Generation) |
| `/Od` | Optimierung aus (Debug) |
| `/Zi` | Debug-Symbole erzeugen |
| `/MDd` | Debug-Runtime (DLL-Variante) |

### §6.3 — Warnstufe `/W4`

ProWB nutzt `/W4` wie der Kernel, nicht `/W3` wie SDK/Test. Grund:
Der Builder ist neu und soll warnungsfrei sein. Es gibt keine
Altlasten, die `/W4` brechen würden.

**Ziel:** 0 Warnungen bei `/W4` — wie der Kernel seit `1.23.10`.

### §6.4 — Kein `DLL_FLAGS`

ProWB exportiert nichts. Es ist eine EXE mit `main()`.

---

## §7 — Link

Der Link-Schritt verwendet eine eigene Variable `LFLAGS`, die von
`CONFIG` abhängt:

```nmake
$(LINKER) /nologo $(LFLAGS) \
    /OUT:$(EXE_TARGET) \
    $(OBJ_PROWB)
```

| Flag | CONFIG | Bedeutung |
|---|---|---|
| `/LTCG` | `release` | Link-Time Code Generation (vollendet `/GL`) |
| `/DEBUG` | `debug` | Debug-Informationen ins Link-Ergebnis |
| `/OUT:` | beide | Pfad der EXE |

**Keine Import-Libs.** ProWB ist eigenständig — keine Abhängigkeit
auf `ProPhysics.lib`, `pro_sdk_interface.lib` oder eine andere
ProPhysics-Bibliothek.

**Keine `.exp`-Datei.** EXEs mit `main()` erzeugen keine Export-Datei.

---

## §8 — Was nach dem Build wo liegt

```
<repo>\
├── bin\
│   └── prowb\
│       └── prowb.exe             <- final, sauber
└── build\
    └── prowb\
        └── _obj\                  <- leer (nur Verzeichnis)
```

**Kein Zwischenstand liegt außerhalb von `_obj\`.** Nach dem Link
werden alle `.obj`-Dateien gelöscht. Das `_obj\`-Verzeichnis selbst
bleibt (leer) bestehen.

**`bin\` bleibt unangetastet.** Kernel-DLLs und Test-EXEs werden
nicht berührt.

---

## §9 — Clean

```cmd
nmake /NOLOGO /f Makefile.nmake clean
```

Löscht:

- `$(OBJ_DIR)` komplett (rekursiv)
- `bin\prowb\prowb.exe`

**Nicht** angetastet werden:

- `bin\ProPhysics.dll` (gehört dem Kernel-Build)
- `bin\pro_sdk_interface.dll` (gehört dem SDK-Build)
- `bin\example_*.exe` (gehören dem Test-Build)
- `lib\*.lib` (gehören Kernel + SDK)
- `BUILD_INFO.txt` im Root
- `out\web\` (Output des ProWB-Builds, nicht Teil des Makefiles)

Sauberes Zusammenspiel mit `build\main\Makefile.nmake`, das sein
`clean_prowb`-Target auf dieses `clean` mappt.

**Warum `out\web\` nicht gelöscht wird:** Der Web-Docs-Output ist
ein **Ergebnis** des Builders, nicht des Builds. Ein `nmake clean`
soll die Builder-EXE entfernen, nicht das Portal. Wer das Portal
neu erzeugen will, ruft `pro_run web -Rebuild` auf — das löscht
`out\web\` vor dem Lauf.

---

## §10 — Abhängigkeits-Graph

```
setup
  │  mkdir $(OBJ_DIR)\, $(BIN_DIR)\
  ▼
+---------------------+   +---------------------+
| prowb.c             |   | md_parser.c         |
+---------------------+   +---------------------+
          │                         │
          └────────────┬────────────┘
                       ▼
                  link.exe
                       │
                       ▼
              bin\prowb\prowb.exe
                       │
                       ▼
         delete $(OBJ_DIR)\*.obj
```

Beide Modulregeln hängen von **allen** Headern ab. Ein Header-Update
rebuildet damit beide Module. Das ist bewusst grob — bei zwei Dateien
spart das die `/showIncludes`-Verrenkung ohne nennenswerten Nachteil
(Build < 1 s).

---

## §11 — Integration in den Master

### §11.1 — Target `prowb` im Master-Makefile

```nmake
prowb: setup
	cd /d "$(BUILD_PROWB)" && $(SUB_NMAKE) Makefile.nmake CONFIG=$(CONFIG)
```

`BUILD_PROWB` ist im Master definiert als `$(ROOT)\build\prowb`.

### §11.2 — Position im `all`-Target

```
all: setup prophysics sdk test prowb info
```

**ProWB läuft nach `test`.**

Begründung:

- **Kernel/SDK/Test zuerst.** Die sind Runtime-relevant.
- **ProWB danach.** Es ist ein Build-Tool, kein Runtime-Artefakt.
- **Fehlschlag blockiert nicht.** Wenn ProWB fehlschlägt, sind
  Kernel/SDK/Tests bereits gebaut. Der Master loggt den Fehler,
  setzt `all` aber nicht zurück.

### §11.3 — Clean-Target `clean_prowb`

```nmake
clean_prowb:
	cd /d "$(BUILD_PROWB)" && $(SUB_NMAKE) Makefile.nmake clean
	@if exist "$(BIN_DIR)\prowb" rmdir /S /Q "$(BIN_DIR)\prowb"
```

`clean_prowb` ist **nicht** im `clean`-Target des Masters enthalten.
Grund: `clean` löscht Kernel/SDK/Tests, aber ProWB ist unabhängig.
Wer ProWB mit-cleanen will, ruft `nmake clean_prowb` separat auf.

**Alternativ:** `clean` könnte um `clean_prowb` erweitert werden.
Das ist eine Design-Entscheidung, die bei Bedarf geändert werden
kann. Aktueller Stand: getrennt.

### §11.4 — Rebuild-Target `rebuild_prowb`

```nmake
rebuild_prowb: clean_prowb prowb
```

Kein `info` am Ende (ProWB erzeugt keine `BUILD_INFO`-Änderung).

---

## §12 — Reihenfolge in der Build-Kette

```
build\prophysics  →  bin\ProPhysics.dll + lib\ProPhysics.lib
       │
       ▼
build\sdk         →  bin\pro_sdk_interface.dll + lib\pro_sdk_interface.lib
       │
       ▼
build\test        →  bin\example_*.exe
       │
       ▼
build\prowb       →  bin\prowb\prowb.exe       (dieses Makefile)
```

**ProWB hängt nicht von den anderen ab.** Es steht am Ende, weil es
im Master-`all` als letztes Target läuft. Bei `pro_run web` wird es
direkt gebaut, ohne die anderen Komponenten.

**Konsequenz:** Ein `nmake /f Makefile.nmake` in `build\prowb\`
funktioniert **alleine**, ohne dass Kernel/SDK/Tests existieren.

---

## §13 — Voraussetzungen

1. **`src\prowb\prowb.c` und `src\prowb\md_parser.c` müssen
   existieren.**
   Das Makefile listet sie in `OBJ_PROWB`. Fehlt eine, meldet
   NMAKE sie als unbekannte Regel.

2. **`src\prowb\header\prowb.h` und `src\prowb\header\md_parser.h`
   müssen existieren.**
   Die `/I`-Pfade referenzieren `$(HDR_DIR)`. Fehlt ein Header,
   bricht `cl.exe` mit `cannot open source file` ab.

3. **MSVC-Toolchain im PATH.**
   Wie bei den anderen Builds. Am einfachsten über den
   VS-Developer-Prompt oder `vcvars64.bat`.

4. **Keine externen Bibliotheken.**
   ProWB linkt gegen nichts außer `msvcrt` (implizit). Kein
   `ProPhysics.lib`, kein `pro_sdk_interface.lib`.

---

## §14 — Hilfe

```cmd
nmake /NOLOGO /f Makefile.nmake help
```

Gibt eine Übersicht der Targets und der `CONFIG`-Option aus.
Rein informativ.

**Ausgabe:**

```
ProWB Builder -- Targets:
  all                Builder bauen (Default)
  setup              Ausgabe-Verzeichnisse anlegen
  clean              Artefakte entfernen
  help               diese Uebersicht

Optionen:  CONFIG=release|debug   (Default: release)
```

---

## §15 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `CONFIG muss "release" oder "debug" sein` | Tippfehler | Groß-/Kleinschreibung prüfen |
| `prowb.h: No such file` | Header fehlt | `src\prowb\header\` prüfen |
| `md_parser.h: No such file` | Header fehlt | dito |
| `unresolved external symbol md_parse` | `md_parser.c` nicht in `OBJ_PROWB` | Makefile prüfen |
| `LNK2019: _main` | keine `main()` in `prowb.c` | `main()` prüfen |
| `fatal error U1073: ... nicht gefunden` | Datei fehlt | Pfade prüfen |
| `bin\prowb\` wird nicht angelegt | `setup` nicht gelaufen | `nmake setup` |
| `cl.exe` gibt Warnung `C4100` (unreferenzierter Parameter) | Warnung bei `/W4` | Parameter nutzen oder `(void)`-Cast |
| `cl.exe` gibt Warnung `C4189` (unbenutzte Variable) | Warnung bei `/W4` | Variable entfernen |
| EXE startet nicht | `msvcrt.dll` fehlt | unwahrscheinlich — Teil von Windows |

**Ziel:** 0 Warnungen bei `/W4`. Wenn Warnungen auftreten, sind sie
neue Regressions und sollten gefixt werden.

---

## §16 — Was dieses Makefile nicht tut

- **Kein Header-Export.** Header bleiben in `src\prowb\header\`.
- **Kein `BUILD_INFO.txt`.** Das macht der Master-Build.
- **Kein Signing.** Das macht `build.ps1 -Sign` (falls gewünscht).
- **Kein Web-Docs-Build.** Das macht die EXE selbst
  (`prowb.exe --manifest …`). Das Makefile baut nur die EXE.
- **Kein Deployment.** Kein Push, kein Upload. Das ist Aufgabe des
  CI-Workflows.
- **Kein `out\web\`-Clean.** Das Portal wird vom Makefile nicht
  angefasst.
- **Kein Test-Lauf.** ProWB hat keine Tests im Kernel-Sinne.

---

## §17 — Parameter-Referenz

Das Makefile hat **einen Parameter** (`CONFIG`) plus Targets.

```
nmake [Target] [CONFIG=release|debug]
```

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup` + EXE bauen |
| `setup` | Ausgabe-Verzeichnisse anlegen |
| `help` | Übersicht |
| `clean` | Artefakte entfernen |

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `CONFIG` | Choice | `release` | `release` \| `debug` |

---

## §18 — Zusammenspiel mit `pro_run web`

`pro_run web` ruft **zwei** Schritte auf:

1. `nmake /f Makefile.nmake CONFIG=release` in `build\prowb\`.
2. `bin\prowb\prowb.exe --manifest docs\web\src docs\web\manifest.txt out\web`.

**Der Makefile-Build ist Schritt 1.** Er baut nur die EXE. Schritt 2
ist der eigentliche Web-Docs-Build — der läuft **außerhalb** des
Makefiles, direkt in `pro_run.ps1`.

**Warum zwei Schritte?** Trennung von Bau und Ausführung:

- **Bauen:** `nmake` (Build-System-Konventionen, `CONFIG`,
  Cache, Clean).
- **Ausführen:** direkter EXE-Aufruf (Manifest, OutDir,
  Fehlerausgabe).

Ein einzelner `nmake`-Aufruf, der beides macht, würde die
Manifest-Pfade in das Makefile ziehen. Das ist unschön.

**Vgl.:** Der Master-Build trennt auch `build` und `test`. Der
`test`-Schritt ist **nicht** im Makefile — er läuft über den
Runner.

---

## §19 — Siehe auch

| Thema | Datei |
|---|---|
| ProWB-README | `src\prowb\README.md` |
| Manifest-Pflege | `docs\web\README.md` |
| `pro_run web` | `docs\build\pro_run.md` |
| Master-Makefile | `docs\build\main\Makefile.md` |
| Build-Übersicht | `docs\build\BUILD_SCRIPT.md` |
| CI-Workflow | `docs\build\web-docs-ci.md` |
| Changelog | `CHANGELOG.md` (`1.23.11`) |
| Quelldatei | `src\prowb\prowb.c` |
| Header | `src\prowb\header\prowb.h` |

---

**Ende ProWB-Makefile-Dokumentation v1.0.**