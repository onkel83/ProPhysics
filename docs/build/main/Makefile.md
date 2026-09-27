# ProPhysics Master Build — NMAKE Orchestrierung

**Datei:** `build\main\Makefile.nmake`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Orchestriert die drei Sub-Makefiles (`prophysics`, `sdk`, `test`)
in der richtigen Reihenfolge, legt `bin\` und `lib\` an und ruft am Ende
`write_build_info.ps1` auf.

---

## 1. Was das Makefile tut

Ein Aufruf `nmake` erzeugt den kompletten Build-Zustand:

1. **Setup** — legt `bin\` und `lib\` an, falls sie fehlen (idempotent).
2. **Kernel bauen** — delegiert an `build\prophysics\Makefile.nmake`.
3. **SDK bauen** — delegiert an `build\sdk\Makefile.sdk.nmake`.
4. **Tests bauen** — delegiert an `build\test\Makefile.nmake`.
5. **`BUILD_INFO.txt`** — ruft `write_build_info.ps1` auf.

Es gibt **kein eigenes Compile-/Link-Target**. Das Master-Makefile
enthaelt keine Compiler-Aufrufe. Es ist reine Orchestrierung plus die
zwei Hilfsdienste Setup und Info.

Die Build-Kette ist strikt:

```
prophysics → sdk → test
```

Diese Reihenfolge ist im Makefile durch Target-Abhaengigkeiten
erzwungen (`sdk: prophysics`, `test: sdk`).

**Empfohlener Aufruf:** ueber `pro_run build …` oder `build.ps1`. Das
Master-Makefile ist die unterste Orchestrierungs-Schicht; die Wrapper
fuegen UTF-8-Konsole, Parameter-Uebersetzung und optionale Schritte
(Signing, Git) hinzu. Direkte `nmake`-Aufrufe bleiben gueltig.

---

## 2. Ablageort und Aufruf

```
<repo>\
├── build\
│   └── main\
│       ├── Makefile.nmake          <- dieses Makefile
│       ├── build.ps1 / build.cmd
│       ├── export.ps1 / export.cmd
│       └── write_build_info.ps1
├── tools\
│   └── pro_run.ps1 / .cmd
├── build\
│   ├── prophysics\Makefile.nmake
│   ├── sdk\Makefile.sdk.nmake
│   └── test\Makefile.nmake
```

**Aufruf aus dem Ablageort:**

```cmd
cd build\main
nmake
```

**CWD-Unabhaengigkeit:** Das Makefile nutzt `$(MAKEDIR)`, um den
Repo-Root zu bestimmen:

```
ROOT = $(MAKEDIR)\..\..
```

`$(MAKEDIR)` wird von NMAKE auf das Verzeichnis gesetzt, aus dem das
Makefile geladen wurde — nicht auf das aktuelle Arbeitsverzeichnis.
Dadurch funktioniert der Aufruf mit `-f` auch aus anderen
Verzeichnissen:

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\main\Makefile.nmake
```

**Empfohlen** ist der Aufruf aus `build\main\` heraus, ueber
`build.cmd`, oder ueber `pro_run build`, weil die Wrapper zusaetzlich
die Konsole auf UTF-8 stellen.

---

## 3. Pfade und Symbole

| Symbol | Wert | Bemerkung |
|---|---|---|
| `CONFIG` | `release` oder `debug` | Default `release`, validiert |
| `ROOT` | `$(MAKEDIR)\..\..` | Repo-Root |
| `BIN_DIR` | `$(ROOT)\bin` | Ziel aller DLLs + EXEs |
| `LIB_DIR` | `$(ROOT)\lib` | Ziel aller Import-Libs |
| `BUILD_PP` | `$(ROOT)\build\prophysics` | Kernel-Sub-Makefile |
| `BUILD_SDK` | `$(ROOT)\build\sdk` | SDK-Sub-Makefile |
| `BUILD_TST` | `$(ROOT)\build\test` | Test-Sub-Makefile |
| `INFO_FILE` | `$(ROOT)\BUILD_INFO.txt` | Ausgabedatei der Info |
| `INFO_SCRIPT` | `$(MAKEDIR)\write_build_info.ps1` | Generator-Skript |
| `SUB_NMAKE` | `nmake /NOLOGO /f` | NMAKE-Aufruf-Muster |

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

Der Wert von `CONFIG` wird an alle Sub-Makefiles durchgereicht
(`$(SUB_NMAKE) Makefile.nmake CONFIG=$(CONFIG)`). Ist `CONFIG` weder
`release` noch `debug`, bricht NMAKE mit `!ERROR` ab, **bevor** der
erste Sub-Build startet.

| `CONFIG` | Sub-Makefile-Verhalten |
|---|---|
| `release` | Kernel `/O2 /GL /LTCG`, SDK `/O2 /GL /LTCG`, Test `/O2 /GL /LTCG` |
| `debug` | `/Od /Zi /MDd` + `/DEBUG` in allen Sub-Makefiles |

Details siehe jeweilige Sub-Makefile-Doku.

---

## 5. Targets

### 5.1 Standard-Build

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup prophysics sdk test info` |
| `prophysics` | nur Kernel (+ Setup) |
| `sdk` | Kernel + SDK (+ Setup) |
| `test` | Kernel + SDK + Tests (+ Setup) |
| `info` | nur `BUILD_INFO.txt` |
| `help` | Uebersicht der Targets + Optionen |

`all` ist Default, wenn `nmake` ohne Argument aufgerufen wird.

### 5.2 Rebuild-Targets

| Target | Wirkung |
|---|---|
| `rebuild` | `clean all` — alles neu |
| `rebuild_prophysics` | `clean_prophysics prophysics info` |
| `rebuild_sdk` | `clean_sdk sdk info` |
| `rebuild_test` | `clean_test test info` |

Jedes Rebuild-Target ruft am Ende `info` auf, damit `BUILD_INFO.txt`
zum neuen Zustand passt.

### 5.3 Clean-Targets

| Target | Wirkung |
|---|---|
| `clean` | `clean_test clean_sdk clean_prophysics` + Wildcard-Loeschung in `bin\`, `lib\` + `BUILD_INFO.txt` |
| `clean_prophysics` | Kernel-Sub-`clean` + `ProPhysics*.dll` + `ProPhysics*.lib` |
| `clean_sdk` | SDK-Sub-`clean` + `pro_sdk_interface*.dll` + `pro_sdk_interface*.lib` |
| `clean_test` | Test-Sub-`clean` + `example_*_test.exe` + `example_test_*.exe` |

`clean` ist die Summe der drei Modul-Cleans plus einer finalen
Wildcard-Raeumung von `bin\` und `lib\`. Damit werden auch eventuelle
Restdateien entfernt, die nicht von einem Sub-`clean` erfasst sind.

**Alle Loeschungen laufen ueber Wildcards.** Umbenennungen von
Artefakten (z.B. `ProPhysics_v2.dll`) hinterlassen damit keine Waisen
im `bin\` oder `lib\`.

---

## 6. Setup-Target im Detail

```nmake
setup:
	@if not exist "$(BIN_DIR)" ( \
	    mkdir "$(BIN_DIR)" & \
	    echo [MASTER] bin\ angelegt \
	)
	@if not exist "$(LIB_DIR)" ( \
	    mkdir "$(LIB_DIR)" & \
	    echo [MASTER] lib\ angelegt \
	)
```

**Idempotent:** Existiert der Ordner, passiert nichts. Nur beim
ersten Lauf (oder nach `clean`) werden die Verzeichnisse angelegt.

**Warum ueberhaupt?** Die Sub-Makefiles legen ihre Zielordner
eigentlich selbst an. Das Master-Setup stellt trotzdem sicher, dass
`bin\` und `lib\` existieren, **bevor** der erste Sub-Build startet —
sonst kann ein `clean`-Lauf auf leerem Repo mit einer Fehlermeldung
abbrechen.

---

## 7. Sub-Makefile-Aufrufe

Jedes Sub-Makefile wird mit `cd /d` in seinen Ordner und dann mit
`$(SUB_NMAKE)` aufgerufen. `CONFIG` wird durchgereicht:

```nmake
prophysics: setup
	cd /d "$(BUILD_PP)" && $(SUB_NMAKE) Makefile.nmake CONFIG=$(CONFIG)

sdk: prophysics
	cd /d "$(BUILD_SDK)" && $(SUB_NMAKE) Makefile.sdk.nmake CONFIG=$(CONFIG)

test: sdk
	cd /d "$(BUILD_TST)" && $(SUB_NMAKE) Makefile.nmake CONFIG=$(CONFIG)
```

**`SUB_NMAKE = nmake /NOLOGO /f`** ist eine zentrale Variable, damit
alle Sub-Aufrufe denselben NMAKE-Stil nutzen.

**`/NOLOGO`** unterdrueckt das NMAKE-Copyright-Banner. Fehlerausgaben
der Sub-Makefiles werden **nicht** unterdrueckt.

**Kettenabhaengigkeit:** `sdk: prophysics` heisst: Wer `nmake sdk`
aufruft, bekommt automatisch einen Kernel-Build mit, falls noetig.
Analog `test: sdk`. NMAKE fuehrt jede Abhaengigkeit **einmal** aus,
auch wenn mehrere Ziele sie referenzieren.

**`/d`-Flag bei `cd`:** Wechselt auch das Laufwerk. Wichtig, falls
das Repo auf einem anderen Laufwerk als das CWD liegt.

**`CONFIG`-Weitergabe:** Alle drei Sub-Makefiles kennen `CONFIG`.
Ungueltige Werte brechen im Sub-Makefile mit `!ERROR` ab, bevor der
erste Compiler-Aufruf startet.

---

## 8. Hilfe-Target

```nmake
help:
	@echo.
	@echo ProPhysics Master-Build -- Targets:
	@echo   all                Kernel + SDK + Tests + BUILD_INFO
	@echo   prophysics         nur Kernel
	@echo   sdk                Kernel + SDK
	@echo   test               Kernel + SDK + Tests
	@echo   info               nur BUILD_INFO.txt
	@echo   rebuild[_xxx]      clean + build
	@echo   clean[_xxx]        Artefakte entfernen
	@echo   help               diese Uebersicht
	@echo.
	@echo Optionen:  CONFIG=release^|debug   (Default: release)
	@echo.
```

Rein informativ. Wird von `pro_run help` und `nmake help` genutzt.

---

## 9. Info-Target im Detail

```nmake
info:
	@echo.
	@echo [MASTER] === BUILD_INFO ===
	@powershell -NoProfile -ExecutionPolicy Bypass -File "$(INFO_SCRIPT)" -RepoRoot "$(ROOT)"
```

Ruft `write_build_info.ps1` in einem **frischen PowerShell-Prozess**
auf. Parameter:

| Parameter | Wert |
|---|---|
| `-NoProfile` | keine User-Profile (Reproduzierbarkeit) |
| `-ExecutionPolicy Bypass` | keine Policy-Blockade |
| `-File` | Skript-Pfad |
| `-RepoRoot` | `$(ROOT)` |

**Kein `-Config`.** Das Master-Makefile uebergibt die Build-
Konfiguration nicht an das Info-Skript, weil `CONFIG` pro Build-Schritt
variieren kann und die Info-Datei den letzten Zustand dokumentiert.
Fuer eine explizite `Config:`-Zeile in `BUILD_INFO.txt` muss
`build.ps1 -Config <wert>` verwendet werden.

**Keine Git-Optionen.** Fuer `-GitStamp` und `-GitNote` muss
`build.ps1` oder `write_build_info.ps1` direkt verwendet werden (siehe
`docs\build\helper\write_build_info.md`).

**Warum separater Prozess?** Isolation. Encoding- und
Ausfuehrungsrichtlinien-Einstellungen des Master-Builds werden nicht
an das Info-Skript vererbt, und umgekehrt.

**Doppelaufruf-Vermeidung:** Der `info:`-Target wird von `all`,
`rebuild`, `rebuild_prophysics`, `rebuild_sdk` und `rebuild_test`
jeweils referenziert. `build.ps1` erkennt das und ruft das Info-Skript
in diesen Faellen nicht ein zweites Mal auf.

---

## 10. Abhaengigkeits-Graph

```
all
 │
 ├─> setup           (mkdir bin\, lib\)
 │
 ├─> prophysics ─> setup
 │        │
 │        └─> cd build\prophysics && nmake /f Makefile.nmake CONFIG=<c>
 │
 ├─> sdk ──────> prophysics
 │        │
 │        └─> cd build\sdk && nmake /f Makefile.sdk.nmake CONFIG=<c>
 │
 ├─> test ─────> sdk
 │        │
 │        └─> cd build\test && nmake /f Makefile.nmake CONFIG=<c>
 │
 └─> info
          └─> powershell write_build_info.ps1 -RepoRoot <ROOT>
```

NMAKE dedupliziert Target-Aufrufe: `setup` laeuft einmal, obwohl
`all` und `prophysics` es referenzieren.

---

## 11. Reihenfolge und Kettenpruefung

Die Sub-Makefiles haben **eigene** Vorabpruefungen:

| Sub-Makefile | Prueft |
|---|---|
| `build\prophysics` | nichts (unterste Ebene) |
| `build\sdk` | `check_core` — `ProPhysics.lib` existiert |
| `build\test` | `check_deps` — beide Import-Libs existieren |

Das Master-Makefile verlaesst sich auf die Target-Abhaengigkeiten
(`sdk: prophysics`, `test: sdk`), um die richtige Reihenfolge zu
erzwingen. Bei einem direkten Sub-Aufruf ausserhalb des Masters
greifen die `check_*`-Pruefungen und brechen mit klarer Meldung ab.

---

## 12. Ausgabe

Nach erfolgreichem `nmake all`:

```
<repo>\
├── bin\
│   ├── ProPhysics.dll
│   ├── pro_sdk_interface.dll
│   ├── example_alpha_test.exe
│   ├── example_test_density.exe
│   └── example_test_tensor.exe
├── lib\
│   ├── ProPhysics.lib
│   └── pro_sdk_interface.lib
├── BUILD_INFO.txt
└── build\
    ├── prophysics\_obj\      (nach Link geloescht)
    ├── sdk\_obj\             (nach Link geloescht)
    └── test\_obj\            (nach Link geloescht)
```

Keine Zwischenstaende ausserhalb der jeweiligen `_obj\`-Ordner.
Die `_obj\`-Ordner selbst werden von den Sub-Makefiles nach dem
Link aufgeraeumt; nur das leere Verzeichnis bleibt.

---

## 13. Konsolenausgabe

Beispiel fuer `nmake rebuild CONFIG=debug`:

```
[MASTER] Clean Tests...
[MASTER] Clean SDK...
[MASTER] Clean ProPhysics...
[MASTER] Alle Artefakte entfernt.

[MASTER] === ProPhysics Kernel (CONFIG=debug) ===
[KERNEL] ...
[MASTER] === SDK Interface (CONFIG=debug) ===
[SDK] ...
[MASTER] === Tests (CONFIG=debug) ===
[TEST] ...
[MASTER] === BUILD_INFO ===
    OK  H:\...\BUILD_INFO.txt
```

Der Master prefixiert seine eigenen Zeilen mit `[MASTER]`. Die
Sub-Makefiles haben eigene Prefixe (`[KERNEL]`, `[SDK]`, `[TEST]`),
sodass die Ausgabe pro Build-Stufe lesbar bleibt.

---

## 14. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausfuehren |
| `CONFIG muss "release" oder "debug" sein` | Tippfehler | Gross-/Kleinschreibung pruefen |
| `[FEHLER] ProPhysics.lib fehlt` | Kernel-Build fehlgeschlagen | Sub-Build-Log ansehen, `build\prophysics` pruefen |
| `[FEHLER] pro_sdk_interface.lib fehlt` | SDK-Build fehlgeschlagen | `build\sdk` pruefen |
| `write_build_info.ps1 nicht gefunden` | Skript umbenannt | `build\main\write_build_info.ps1` pruefen |
| `BUILD_INFO.txt fehlt` nach `clean` | `clean` entfernt sie absichtlich | `nmake info` separat laufen |
| Umlaute kaputt | Master direkt aufgerufen | `build.cmd` oder `pro_run` verwenden |
| `cd /d ... && nmake` schlaegt fehl | Sub-Makefile fehlt | Ordner + Dateinamen in `build\<modul>\` pruefen |
| `fatal error U1077` bei Info | PowerShell-Skript-Fehler | Skript einzeln aufrufen: `powershell -File write_build_info.ps1 -RepoRoot .` |
| Waisen im `bin\` nach Umbenennung | Wildcard deckt nicht alle Faelle | Wildcards in `clean_*` erweitern |

---

## 15. Was dieses Makefile nicht tut

- **Kein Compile und Link.** Kein `cl.exe`-, kein `link.exe`-Aufruf.
- **Kein Signing.** Nur `build.ps1 -Sign` signiert DLLs.
- **Kein Export.** Nur `export.ps1` erzeugt `out\` und ZIPs.
- **Kein Git-Stamp und keine Notiz.** Dafuer `build.ps1` mit
  `-GitStamp`/`-GitNote` verwenden.
- **Kein `-Config` an Info.** `BUILD_INFO.txt` bekommt keine
  `Config:`-Zeile, wenn `nmake` direkt aufgerufen wird. Fuer die volle
  Variante `build.ps1 -Config <wert>` verwenden.
- **Keine Test-Ausfuehrung.** Nur `tools\run_alpha_tests.ps1` oder
  `pro_run test`.
- **Kein ZIP, kein Deployment.**
- **Keine inkrementelle Logik.** Es delegiert nur an die Sub-Makefiles;
  die entscheiden selbst, was neu gebaut wird.

---

## 16. Zusammenspiel mit `build.ps1`

`build.ps1` nutzt **dasselbe** Master-Makefile als Backend. Der
Wrapper:

1. setzt UTF-8 (`chcp 65001`, `OutputEncoding`),
2. uebersetzt `-Mode` in ein nmake-Target
   (`prophysics`/`sdk`/`test`/`all`/`info`),
3. uebersetzt `-Rebuild` in `rebuild_<scope>`,
4. uebergibt `CONFIG=$(Config)`,
5. ruft `nmake <target> CONFIG=<config>` auf,
6. signiert optional mit `signtool`,
7. ruft `write_build_info.ps1` **mit** Git- und Config-Flags auf,
   falls gesetzt und falls der nmake-Target nicht bereits `info:`
   ausgefuehrt hat,
8. gibt eine Zusammenfassung aus.

Das Master-Makefile bleibt damit die **unterste** Orchestrierungs-Schicht.
Der Wrapper fuegt Bedienkomfort und optionale Schritte (Signing, Git,
Config-Dokumentation) hinzu, aendert aber nichts an der
Build-Reihenfolge.

---

## 17. Beispiel-Aufrufe

### 17.1 Voller Build (Default, release)

```cmd
cd build\main
nmake
```

Ergebnis: Kernel + SDK + Tests + `BUILD_INFO.txt` in `release`.

### 17.2 Voller Build in Debug

```cmd
nmake all CONFIG=debug
```

Ergebnis: alle Sub-Makefiles bauen mit `/Od /Zi /MDd` + `/DEBUG`.

### 17.3 Nur Kernel

```cmd
nmake prophysics
```

`BUILD_INFO.txt` wird **nicht** aktualisiert — dafuer `nmake info`
separat.

### 17.4 Rebuild + Info

```cmd
nmake rebuild_test
```

`clean_test test info` — Kernel und SDK bleiben unangetastet.

### 17.5 Nur Info aktualisieren

```cmd
nmake info
```

Kein Build. `BUILD_INFO.txt` bekommt einen neuen Zeitstempel und
die aktuelle Dateiliste.

### 17.6 Alles weg

```cmd
nmake clean
```

`bin\` und `lib\` sind leer, `BUILD_INFO.txt` entfernt.

### 17.7 Aus einem anderen CWD

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\main\Makefile.nmake ^
      prophysics CONFIG=debug
```

Funktioniert dank `$(MAKEDIR)`.

### 17.8 Hilfe

```cmd
nmake help
```

---

## 18. Parameter-Referenz

Das Makefile hat **einen Parameter** (`CONFIG`) plus Targets.

```
nmake [Target] [CONFIG=release|debug]
```

| Target | Kette |
|---|---|
| `all` (Default) | `setup prophysics sdk test info` |
| `prophysics` | `setup` + Kernel |
| `sdk` | Kernel + SDK |
| `test` | Kernel + SDK + Tests |
| `info` | nur Info |
| `help` | Uebersicht |
| `rebuild` | `clean all` |
| `rebuild_prophysics` | `clean_prophysics prophysics info` |
| `rebuild_sdk` | `clean_sdk sdk info` |
| `rebuild_test` | `clean_test test info` |
| `clean` | alles weg |
| `clean_prophysics` | nur Kernel weg |
| `clean_sdk` | nur SDK weg |
| `clean_test` | nur Tests weg |

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `CONFIG` | Choice | `release` | Build-Konfiguration. Wird an alle Sub-Makefiles durchgereicht |

---

## 19. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\prophysics\Makefile.md` — Kernel-Sub-Build
- `docs\build\sdk\Makefile.md` — SDK-Sub-Build
- `docs\build\test\Makefile.md` — Test-Sub-Build
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\export.md` — Export-Wrapper
- `docs\build\helper\write_build_info.md` — `BUILD_INFO.txt`-Format

---

**Ende Master-Makefile-Dokumentation (v1.0.0).**