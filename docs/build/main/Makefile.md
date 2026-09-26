# ProPhysics Master Build — NMAKE Orchestrierung

**Datei:** `build\main\Makefile.nmake`
**Version:** 3.0 (Etappe 23)
**Zweck:** Orchestriert die drei Sub-Makefiles (`prophysics`, `sdk`, `test`)
in der richtigen Reihenfolge, legt `bin\` und `lib\` an und ruft am Ende
`write_build_info.ps1` auf.

---

## 1. Was das Makefile tut

Ein Aufruf `nmake` aus `build\main\` heraus erzeugt den kompletten
Build-Zustand:

1. **Setup** — legt `bin\` und `lib\` an, falls sie fehlen (idempotent).
2. **Kernel bauen** — delegiert an `build\prophysics\Makefile.nmake`.
3. **SDK bauen** — delegiert an `build\sdk\Makefile.sdk.nmake`.
4. **Tests bauen** — delegiert an `build\test\Makefile.nmake`.
5. **`BUILD_INFO.txt`** — ruft `write_build_info.ps1` auf.

Es gibt **kein eigenes Compile-/Link-Target**. Das Master-Makefile
enthält keine Compiler-Aufrufe. Es ist reine Orchestrierung plus die
zwei Hilfsdienste Setup und Info.

Die Build-Kette ist strikt:

```
prophysics → sdk → test
```

Diese Reihenfolge ist im Makefile durch Target-Abhängigkeiten
erzwungen (`sdk: prophysics`, `test: sdk`).

---

## 2. Ablageort und Aufruf

```
H:\ProPhysics_SDK\ProPhysics\
└── build\
    └── main\
        ├── Makefile.nmake          <- dieses Makefile
        ├── build.ps1 / build.cmd
        ├── export.ps1 / export.cmd
        └── write_build_info.ps1
```

**Aufruf** aus dem Ablageort:

```cmd
cd build\main
nmake
```

**CWD-Unabhängigkeit:** Das Makefile nutzt `$(MAKEDIR)`, um den
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

**Empfohlen** ist aber der Aufruf aus `build\main\` heraus oder über
`build.cmd`, weil der Wrapper zusätzlich die Konsole auf UTF-8 stellt.

---

## 3. Pfade und Symbole

| Symbol | Wert | Bemerkung |
|---|---|---|
| `ROOT` | `$(MAKEDIR)\..\..` | Repo-Root |
| `BIN_DIR` | `$(ROOT)\bin` | Ziel aller DLLs + EXEs |
| `LIB_DIR` | `$(ROOT)\lib` | Ziel aller Import-Libs |
| `BUILD_PP` | `$(ROOT)\build\prophysics` | Kernel-Sub-Makefile |
| `BUILD_SDK` | `$(ROOT)\build\sdk` | SDK-Sub-Makefile |
| `BUILD_TST` | `$(ROOT)\build\test` | Test-Sub-Makefile |
| `INFO_FILE` | `$(ROOT)\BUILD_INFO.txt` | Ausgabedatei der Info |
| `INFO_SCRIPT` | `$(MAKEDIR)\write_build_info.ps1` | Generator-Skript |

Alle Pfade sind **relativ zu `$(MAKEDIR)`**. Damit ist das Makefile
verschiebbar, solange die Repo-Struktur intakt bleibt.

---

## 4. Targets

### 4.1 Standard-Build

| Target | Wirkung |
|---|---|
| `all` (Default) | `setup prophysics sdk test info` |
| `prophysics` | nur Kernel (+ Setup) |
| `sdk` | Kernel + SDK (+ Setup) |
| `test` | Kernel + SDK + Tests (+ Setup) |
| `info` | nur `BUILD_INFO.txt` |

`all` ist Default, wenn `nmake` ohne Argument aufgerufen wird.

### 4.2 Rebuild-Targets

| Target | Wirkung |
|---|---|
| `rebuild` | `clean all` — alles neu |
| `rebuild_prophysics` | `clean_prophysics prophysics info` |
| `rebuild_sdk` | `clean_sdk sdk info` |
| `rebuild_test` | `clean_test test info` |

Jedes Rebuild-Target ruft am Ende `info` auf, damit `BUILD_INFO.txt`
zum neuen Zustand passt.

### 4.3 Clean-Targets

| Target | Wirkung |
|---|---|
| `clean` | `clean_test clean_sdk clean_prophysics` + `bin\*`, `lib\*`, `BUILD_INFO.txt` löschen |
| `clean_prophysics` | Kernel-Sub-`clean` + `ProPhysics.dll` + `ProPhysics.lib` |
| `clean_sdk` | SDK-Sub-`clean` + `pro_sdk_interface.dll` + `.lib` |
| `clean_test` | Test-Sub-`clean` + die drei `example_*.exe` |

`clean` ist die Summe der drei Modul-Cleans plus eines finalen
Räumens von `bin\` und `lib\`. Damit werden auch eventuelle
Restdateien entfernt, die nicht von einem Sub-`clean` erfasst sind.

---

## 5. Setup-Target im Detail

```
setup:
	@if not exist "$(BIN_DIR)" ( mkdir "$(BIN_DIR)" & echo [MASTER] bin\ angelegt )
	@if not exist "$(LIB_DIR)" ( mkdir "$(LIB_DIR)" & echo [MASTER] lib\ angelegt )
```

**Idempotent:** Existiert der Ordner, passiert nichts. Nur beim
ersten Lauf (oder nach `clean`) werden die Verzeichnisse angelegt.

**Warum überhaupt?** Die Sub-Makefiles legen ihre Zielordner
eigentlich selbst an. Das Master-Setup stellt trotzdem sicher, dass
`bin\` und `lib\` existieren, **bevor** der erste Sub-Build startet —
sonst kann ein `clean`-Lauf auf leerem Repo mit einer Fehlermeldung
abbrechen.

---

## 6. Sub-Makefile-Aufrufe

Jedes Sub-Makefile wird mit `cd /d` in seinen Ordner und dann mit
`nmake /NOLOGO /f <datei>` aufgerufen:

```
prophysics: setup
	cd /d "$(BUILD_PP)" && nmake /NOLOGO /f Makefile.nmake

sdk: prophysics
	cd /d "$(BUILD_SDK)" && nmake /NOLOGO /f Makefile.sdk.nmake

test: sdk
	cd /d "$(BUILD_TST)" && nmake /NOLOGO /f Makefile.nmake
```

**`/NOLOGO`** unterdrückt das NMAKE-Copyright-Banner. Fehlerausgaben
der Sub-Makefiles werden **nicht** unterdrückt.

**Kettenabhängigkeit:** `sdk: prophysics` heißt: Wer `nmake sdk`
aufruft, bekommt automatisch einen Kernel-Build mit, falls nötig.
Analog `test: sdk`. NMAKE führt jede Abhängigkeit **einmal** aus,
auch wenn mehrere Ziele sie referenzieren.

**`/d`-Flag bei `cd`:** Wechselt auch das Laufwerk. Wichtig, falls
das Repo auf einem anderen Laufwerk als das CWD liegt.

---

## 7. Info-Target im Detail

```
info:
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

**Keine Git-Optionen.** Das Master-Makefile ruft `info` ohne
`-GitStamp` und ohne `-GitNote` auf. Für Git-Metadaten oder Notizen
muss `build.ps1` oder `write_build_info.ps1` direkt verwendet werden
(siehe `docs\build\helper\write_build_info.md`).

**Warum separater Prozess?** Isolation. Encoding- und
Ausführungsrichtlinien-Einstellungen des Master-Builds werden nicht
an das Info-Skript vererbt, und umgekehrt.

---

## 8. Abhängigkeits-Graph

```
all
 │
 ├─> setup           (mkdir bin\, lib\)
 │
 ├─> prophysics ─> setup
 │        │
 │        └─> cd build\prophysics && nmake
 │
 ├─> sdk ──────> prophysics
 │        │
 │        └─> cd build\sdk && nmake /f Makefile.sdk.nmake
 │
 ├─> test ─────> sdk
 │        │
 │        └─> cd build\test && nmake
 │
 └─> info
          └─> powershell write_build_info.ps1 -RepoRoot <ROOT>
```

NMAKE dedupliziert Target-Aufrufe: `setup` läuft einmal, obwohl
`all` und `prophysics` es referenzieren.

---

## 9. Reihenfolge und Kettenprüfung

Die Sub-Makefiles haben **eigene** Vorabprüfungen:

| Sub-Makefile | Prüft |
|---|---|
| `build\prophysics` | nichts (unterste Ebene) |
| `build\sdk` | `check_core` — `ProPhysics.lib` existiert |
| `build\test` | `check_deps` — beide Import-Libs existieren |

Das Master-Makefile verlässt sich auf die Target-Abhängigkeiten
(`sdk: prophysics`, `test: sdk`), um die richtige Reihenfolge zu
erzwingen. Bei einem direkten Sub-Aufruf außerhalb des Masters
greifen die `check_*`-Prüfungen und brechen mit klarer Meldung ab.

---

## 10. Ausgabe

Nach erfolgreichem `nmake all`:

```
H:\ProPhysics_SDK\ProPhysics\
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
    ├── prophysics\_obj\      (leer)
    ├── sdk\_obj\             (leer)
    └── test\_obj\            (leer)
```

Keine Zwischenstände außerhalb der jeweiligen `_obj\`-Ordner.

---

## 11. Konsolenausgabe

Beispiel für `nmake rebuild`:

```
[MASTER] Clean Tests...
[MASTER] Clean SDK...
[MASTER] Clean ProPhysics...
[MASTER] Alle Artefakte entfernt.

[MASTER] === ProPhysics Kernel ===
[KERNEL] ...
[MASTER] === SDK Interface ===
[SDK] ...
[MASTER] === Tests ===
[TEST] ...
[MASTER] === BUILD_INFO ===
...
```

Der Master prefixiert seine eigenen Zeilen mit `[MASTER]`. Die
Sub-Makefiles haben eigene Prefixe (`[KERNEL]`, `[SDK]`, `[TEST]`),
sodass die Ausgabe pro Build-Stufe lesbar bleibt.

---

## 12. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausführen |
| `[FEHLER] ProPhysics.lib fehlt` | Kernel-Build fehlgeschlagen | Sub-Build-Log ansehen, `build\prophysics` prüfen |
| `[FEHLER] pro_sdk_interface.lib fehlt` | SDK-Build fehlgeschlagen | `build\sdk` prüfen |
| `write_build_info.ps1 nicht gefunden` | Skript umbenannt | `build\main\write_build_info.ps1` prüfen |
| `BUILD_INFO.txt fehlt` nach `-Clean` | `clean` entfernt sie absichtlich | `nmake info` separat laufen |
| Umlaute kaputt | Master direkt aufgerufen | `build.cmd` statt `nmake` verwenden |
| `cd /d ... && nmake` schlägt fehl | Sub-Makefile fehlt | Ordner + Dateinamen in `build\<modul>\` prüfen |
| `fatal error U1077` bei Info | PowerShell-Skript-Fehler | Skript einzeln aufrufen: `powershell -File write_build_info.ps1 -RepoRoot .` |

---

## 13. Was dieses Makefile nicht tut

- **Kein Compile und Link.** Kein `cl.exe`-, kein `link.exe`-Aufruf.
- **Kein Signing.** Nur `build.ps1 -Sign` signiert DLLs.
- **Kein Export.** Nur `export.ps1` erzeugt `out\`.
- **Kein Git-Stamp und keine Notiz.** Dafür `build.ps1` mit
  `-GitStamp`/`-GitNote` verwenden.
- **Keine Test-Ausführung.** Nur `tools\run_alpha_tests.ps1`.
- **Kein Zip, kein Deployment.**
- **Keine inkrementelle Logik.** Es delegiert nur an die Sub-Makefiles;
  die entscheiden selbst, was neu gebaut wird.

---

## 14. Zusammenspiel mit `build.ps1`

`build.ps1` nutzt **dasselbe** Master-Makefile als Backend. Der
Wrapper:

1. setzt UTF-8 (`chcp 65001`, `OutputEncoding`),
2. übersetzt `-Mode` in ein nmake-Target (`prophysics`/`sdk`/`test`/`all`),
3. übersetzt `-Rebuild` in `rebuild_<scope>`,
4. ruft `nmake <target>` auf,
5. signiert optional mit `signtool`,
6. ruft `write_build_info.ps1` **mit** Git-Flags auf, falls gesetzt,
7. gibt eine Zusammenfassung aus.

Das Master-Makefile bleibt damit die **unterste** Orchestrierungs-Schicht.
Der Wrapper fügt Bedienkomfort und optionale Schritte (Signing, Git)
hinzu, ändert aber nichts an der Build-Reihenfolge.

---

## 15. Beispiel-Aufrufe

### 15.1 Voller Build (Default)

```cmd
cd build\main
nmake
```

Ergebnis: Kernel + SDK + Tests + `BUILD_INFO.txt`.

### 15.2 Nur Kernel

```cmd
nmake prophysics
```

`BUILD_INFO.txt` wird **nicht** aktualisiert — dafür `nmake info`
separat.

### 15.3 Rebuild + Info

```cmd
nmake rebuild_test
```

`clean_test test info` — Kernel und SDK bleiben unangetastet.

### 15.4 Nur Info aktualisieren

```cmd
nmake info
```

Kein Build. `BUILD_INFO.txt` bekommt einen neuen Zeitstempel und
die aktuelle Dateiliste.

### 15.5 Alles weg

```cmd
nmake clean
```

`bin\` und `lib\` sind leer, `BUILD_INFO.txt` entfernt.

### 15.6 Aus einem anderen CWD

```cmd
cd C:\Temp
nmake /NOLOGO /f H:\ProPhysics_SDK\ProPhysics\build\main\Makefile.nmake prophysics
```

Funktioniert dank `$(MAKEDIR)`.

---

## 16. Parameter-Referenz

Das Makefile hat **keine Parameter** im Sinne von `-Flags`. Alles
läuft über Targets.

```
nmake [Target]
```

| Target | Kette |
|---|---|
| `all` (Default) | `setup prophysics sdk test info` |
| `prophysics` | `setup` + Kernel |
| `sdk` | Kernel + SDK |
| `test` | Kernel + SDK + Tests |
| `info` | nur Info |
| `rebuild` | `clean all` |
| `rebuild_prophysics` | `clean_prophysics prophysics info` |
| `rebuild_sdk` | `clean_sdk sdk info` |
| `rebuild_test` | `clean_test test info` |
| `clean` | alles weg |
| `clean_prophysics` | nur Kernel weg |
| `clean_sdk` | nur SDK weg |
| `clean_test` | nur Tests weg |

---

## 17. Siehe auch

- `docs\build\BUILD_SCRIPT.md` — Übersicht des Build-Systems
- `docs\build\prophysics\Makefile.md` — Kernel-Sub-Build
- `docs\build\sdk\Makefile.md` — SDK-Sub-Build
- `docs\build\test\Makefile.md` — Test-Sub-Build
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\export.md` — Export-Wrapper
- `docs\build\helper\write_build_info.md` — `BUILD_INFO.txt`-Format

---

**Ende Master-Makefile-Dokumentation.**