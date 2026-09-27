# ProPhysics write_build_info — BUILD_INFO.txt Generator

**Datei:** `build\main\write_build_info.ps1`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Erzeugt `BUILD_INFO.txt` im Repo-Root mit Zeitstempel,
Version + Etappe, Config (optional), Git-Metadaten (optional),
Freitext-Notiz (optional) und einer Liste aller Artefakte in `bin\`
und `lib\`.

---

## 1. Was das Skript tut

Ein Aufruf → eine Textdatei im Repo-Root. Kein Build, kein Kopieren,
keine externen Abhaengigkeiten ausser optional `git`.

Der Ablauf:

1. **Version + Etappe** aus `src\prophysics\header\ProPhysics_Version.h` lesen
2. **Artefakte** in `bin\` und `lib\` auflisten (Name + Groesse)
3. **Git-Info** sammeln (nur bei `-GitStamp`)
4. **Notiz** einbetten (nur bei `-GitNote`)
5. **BUILD_INFO.txt** schreiben (ueberschreibt alte Datei)

Fehlt `bin\` oder `lib\`, wird das im Text vermerkt statt abzubrechen.
Das Skript ist robust gegen halb-fertige Zustaende.

**Empfohlener Aufruf:** ueber `pro_run build -Mode info` (siehe
`docs\build\pro_run.md`) oder als Teil eines Builds ueber
`build.ps1` (siehe `docs\build\helper\build.md`). Direkte Aufrufe
bleiben gueltig.

---

## 2. Ablageort

```
<repo>\
├── build\
│   └── main\
│       ├── write_build_info.ps1    <- dieses Skript
│       ├── build.ps1
│       ├── export.ps1
│       └── Makefile.nmake
├── tools\
│   └── pro_run.ps1 / .cmd          <- zentraler Einstiegspunkt
├── bin\
├── lib\
├── src\
│   └── prophysics\
│       └── header\
│           └── ProPhysics_Version.h
└── BUILD_INFO.txt                  <- Ziel
```

Der Repo-Root wird **automatisch** aus dem Skript-Ablageort abgeleitet:

```powershell
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
```

Der Aufrufer kann ihn mit `-RepoRoot` ueberschreiben — das ist
nuetzlich, wenn das Skript z. B. einen entfernten Root beschreiben
soll.

---

## 3. Aufruf

### 3.1 Ueber `pro_run` (empfohlen)

```cmd
:: nur BUILD_INFO aktualisieren
pro_run build -Mode info

:: mit Git-Metadaten
pro_run build -Mode info -GitStamp -GitNote "Release 1.0.0"
```

### 3.2 Ueber `build.ps1` (Teil eines Builds)

```cmd
build.cmd -GitStamp -GitNote "Release 1.0.0"
```

`build.ps1` ruft `write_build_info.ps1` am Ende auf und reicht die
Parameter durch. Details siehe `docs\build\helper\build.md`.

### 3.3 Direkt (PowerShell)

```powershell
.\write_build_info.ps1
.\write_build_info.ps1 -RepoRoot "H:\ProPhysics_SDK\ProPhysics"
.\write_build_info.ps1 -Config release -GitStamp
.\write_build_info.ps1 -Version 1.23.0 -GitNote "Etappe 23 abgeschlossen."
.\write_build_info.ps1 -OutFile D:\tmp\BUILD_INFO.txt
```

### 3.4 Ueber das Master-Makefile

Der `info`-Target im Master-Makefile ruft das Skript **ohne**
Git-Optionen auf:

```cmd
cd build\main
nmake info
```

Ergebnis: minimale `BUILD_INFO.txt` ohne Git- und Notiz-Block.

---

## 4. Parameter

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-RepoRoot` | Pfad | `$PSScriptRoot\..\..` | Wurzelverzeichnis des Repos |
| `-Config` | `release` \| `debug` | *(leer)* | Build-Konfiguration (nur Anzeige) |
| `-Version` | String | aus Header | Versions-Override |
| `-OutFile` | Pfad | `<RepoRoot>\BUILD_INFO.txt` | Ausgabepfad |
| `-GitStamp` | Switch | aus | Git-Metadaten einbetten |
| `-GitNote` | String | `''` | Freitext-Notiz einbetten (mehrzeilig) |

**Hinweise:**

- `-RepoRoot` wird nicht validiert — fehlt der Ordner, wird die Datei
  trotzdem geschrieben (mit Hinweisen zu fehlenden Unterordnern).
- `-Config` wird nur in die Datei geschrieben, wenn er gesetzt ist.
  Ohne `-Config` entfaellt die `Config:`-Zeile komplett.
- `-Version` ueberschreibt den aus dem Header gelesenen Wert. Wird
  vor allem fuer Reproduzierbarkeits-Checks genutzt.
- `-OutFile` erlaubt einen abweichenden Pfad (z. B. fuer Build-
  Vergleiche). Das Zielverzeichnis wird angelegt, falls noetig.
- `-GitStamp` und `-GitNote` sind **unabhaengig**. Du kannst `-GitNote`
  ohne `-GitStamp` verwenden, und umgekehrt.
- Fehlt `git` oder ist der Ordner kein Git-Repo: kein Fehler, nur eine
  Hinweiszeile im Git-Block.

---

## 5. Beispiel-Ausgaben

### 5.1 Minimal (keine Optionen)

```
ProPhysics Build Information
============================

Erzeugt:  2026-09-27 16:15:03
Host:     DEV-WORKSTATION
User:     koehn
Version:  1.23.0
Etappe:   23

Artefakte:

  bin\
    ProPhysics.dll                   263.7 KB
    pro_sdk_interface.dll            138.2 KB
    example_alpha_test.exe           442.4 KB
    example_test_density.exe         177.7 KB
    example_test_tensor.exe          163.3 KB
    run_alpha_tests.cmd                0.1 KB
    run_alpha_tests.ps1               16.4 KB
    pro_run.cmd                        0.1 KB
    pro_run.ps1                       12.7 KB

  lib\
    ProPhysics.lib                    51.4 KB
    pro_sdk_interface.lib              2.4 KB

Quellen:
  src\prophysics\   Kernel (12 Module)
  src\sdk\          SDK Interface (1 Modul)
  src\test\         Alpha-Test (17 Module) + 2 Example-Tests

Hinweis:
  Diese Datei wird bei jedem erfolgreichen Build neu erzeugt.
  Inhalt und Format sind dokumentiert in docs\build\BUILD_SCRIPT.md.
```

### 5.2 Mit Config

```
...
Version:  1.23.0
Etappe:   23
Config:   debug

Artefakte:
...
```

### 5.3 Mit Git-Info

Aufruf mit `-GitStamp`:

```
ProPhysics Build Information
============================

Erzeugt:  2026-09-27 16:15:03
Host:     DEV-WORKSTATION
User:     koehn
Version:  1.23.0
Etappe:   23

Git:
  describe: v1.0.0-3-g1a2b3c4d
  branch:   main
  sha:      1a2b3c4d
  status:   clean

Artefakte:
...
```

### 5.4 Mit Notiz

Aufruf mit `-GitNote`:

```
ProPhysics Build Information
============================

Erzeugt:  2026-09-27 16:15:03
Host:     DEV-WORKSTATION
User:     koehn
Version:  1.23.0
Etappe:   23

Notiz:
  Release 1.0.0
  Kernel 1.23.0 / Etappe 23
  Alle 43 Tests gruen

Artefakte:
...
```

### 5.5 Mit Git-Info und Notiz

Aufruf mit `-GitStamp -GitNote "..."`:

```
ProPhysics Build Information
============================

Erzeugt:  2026-09-27 16:15:03
Host:     DEV-WORKSTATION
User:     koehn
Version:  1.23.0
Etappe:   23
Config:   release

Git:
  describe: v1.0.0-3-g1a2b3c4d
  branch:   main
  sha:      1a2b3c4d
  status:   clean

Notiz:
  Release 1.0.0
  Kernel 1.23.0 / Etappe 23
  Alle 43 Tests gruen

Artefakte:
...
```

### 5.6 Ohne Repo

Wenn `-GitStamp` gesetzt ist, aber kein Repo / kein git gefunden wird:

```
...
Version:  1.23.0
Etappe:   23

Git:      nicht verfuegbar (kein Repo oder git fehlt)

Artefakte:
...
```

Kein Fehler, Exit-Code bleibt 0.

---

## 6. Format und Aufbau

### 6.1 Header-Block

| Feld | Quelle |
|---|---|
| `Erzeugt:` | `Get-Date -Format 'yyyy-MM-dd HH:mm:ss'` |
| `Host:` | `$env:COMPUTERNAME` |
| `User:` | `$env:USERNAME` |
| `Version:` | aus `ProPhysics_Version.h` (MAJOR.MINOR.PATCH), ueberschreibbar mit `-Version` |
| `Etappe:` | aus `ProPhysics_Version.h` (`PROPHYSICS_ETAPPE`) |
| `Config:` | nur wenn `-Config` gesetzt |

**Version-Fallback:** Ist `ProPhysics_Version.h` nicht lesbar, wird
`unknown` ausgegeben. Findet der Regex keine Werte, kommen `?` zum
Einsatz (`?.?.?`).

### 6.2 Git-Block (optional)

Nur wenn `-GitStamp` gesetzt. Vier Zeilen mit fixen Prefixen:

| Zeile | Quelle | Wertebereich |
|---|---|---|
| `describe:` | `git describe --tags --dirty --always --long` | Tag-Name + SHA, oder `n/a` |
| `branch:` | `git rev-parse --abbrev-ref HEAD` | Branch-Name, oder `n/a` |
| `sha:` | `git rev-parse --short HEAD` | 7-Zeichen-Hash, oder `n/a` |
| `status:` | `git status --porcelain` | `clean` oder `dirty` |

**Warum `--dirty` und `--long`?** `--long` erzwingt das Format
`<tag>-<count>-g<sha>`, auch wenn direkt auf einem Tag. `--dirty`
markiert uncommitted Changes mit `-dirty`. Zusammen: der Nutzer sieht
auf einen Blick, ob die Version ein sauberer Tag ist oder ein
Zwischenstand.

### 6.3 Notiz-Block (optional)

Nur wenn `-GitNote` gesetzt und nicht leer. Mehrzeilige Notizen
werden mit zwei Leerzeichen eingerueckt:

```
Notiz:
  Zeile 1
  Zeile 2
  Zeile 3
```

**Mehrzeilige Notiz** uebergibst du mit einem PowerShell-Here-String:

```powershell
$note = @"
Release 1.0.0
- Kernel 1.23.0 / Etappe 23
- Alle 43 Tests gruen
- Regression 43/43
"@
.\build.ps1 -GitStamp -GitNote $note
```

### 6.4 Artefakt-Block

Zwei Abschnitte: `bin\` und `lib\`.

**`bin\`-Filter:** `*.dll`, `*.exe`, `*.cmd`, `*.ps1`
**`lib\`-Filter:** `*.lib`

Format pro Datei:

```
    <Name>                              <Groesse> KB
```

Name linksbuendig auf 32 Zeichen, Groesse rechtsbuendig auf 10 Zeichen,
in KB gerundet auf eine Nachkommastelle.

**Sortierung:** nach Name, eindeutig (keine Duplikate trotz
mehrerer Filter).

**Fehlt der Ordner:** Hinweis `bin\  (Verzeichnis fehlt)`.
**Leerer Ordner:** Hinweis `bin\  (leer)`.

### 6.5 Quellen-Block

Statischer Text mit festen Zeilen:

```
Quellen:
  src\prophysics\   Kernel (12 Module)
  src\sdk\          SDK Interface (1 Modul)
  src\test\         Alpha-Test (17 Module) + 2 Example-Tests
```

**Nicht dynamisch.** Wenn du die Modul-Anzahl aenderst, passe den Text
im Skript an. Das ist Absicht: der Block dient als Orientierung, nicht
als Inventar.

### 6.6 Hinweis-Block

Statischer Footer:

```
Hinweis:
  Diese Datei wird bei jedem erfolgreichen Build neu erzeugt.
  Inhalt und Format sind dokumentiert in docs\build\BUILD_SCRIPT.md.
```

---

## 7. Enkodierung

Die Datei wird als **UTF-8** geschrieben (mit BOM von PowerShells
`Out-File -Encoding utf8`). Damit sind Umlaute in Pfaden und Notizen
sicher.

Die Konsole stellt UTF-8 selbst ein:

```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding           = [System.Text.Encoding]::UTF8
```

Diese Zeilen sind **nicht** in einem `chcp`-Try-Block — sie wirken
nur auf die PowerShell-Ausgabe. Der Aufrufer (`build.ps1` oder
`pro_run`) setzt `chcp 65001` selbst.

---

## 8. Ueberschreiben-Verhalten

Die Datei wird **immer komplett ueberschrieben**. Der Ablauf:

```powershell
if (Test-Path -LiteralPath $OutFile) {
    Remove-Item -LiteralPath $OutFile -Force
}
$lines -join [Environment]::NewLine |
    Out-File -LiteralPath $OutFile -Encoding utf8
```

**Konsequenz:** Jeder Lauf erzeugt eine frische Datei mit neuem
Zeitstempel. Kein Anhaengen, kein Merge.

**Atomaritaet:** Die Loesch-dann-Schreib-Sequenz ist nicht atomar.
Bei Absturz zwischen den beiden Schritten ist die Datei weg, nicht
halb. Das ist fuer eine Info-Datei akzeptabel.

---

## 9. Interne Funktionen

### 9.1 `Get-ProVersion`

Liest vier Regex-Treffer aus `ProPhysics_Version.h`:

```powershell
VERSION_MAJOR      -> $maj
VERSION_MINOR      -> $min
VERSION_PATCH      -> $pat
PROPHYSICS_ETAPPE  -> $etappe
```

Liefert ein Hashtable `@{ Version = "1.23.0"; Etappe = "23" }` oder
`@{ Version = 'unknown'; Etappe = 'unknown' }` bei fehlender Datei.

### 9.2 `Get-GitInfo`

Prueft `git rev-parse --is-inside-work-tree`, dann vier Abfragen.
Jede Abfrage in einem eigenen Try/Catch-Safe-Block. Bei Fehler:
`$null` zurueck, damit der Aufrufer den Git-Block weglassen kann.

**Wichtig:** `Push-Location $RepoRoot` vor den Git-Aufrufen.
`Pop-Location` in `finally`. Damit laeuft der Aufruf unabhaengig vom
aktuellen CWD.

### 9.3 `Get-ArtifactLines`

Listet Dateien aus einem Ordner mit mehreren Filtern. Filter werden
zusammengefuehrt, sortiert, dedupliziert (durch `Sort-Object Name
-Unique`).

Rueckgabe: `System.Collections.Generic.List[string]` mit einer Zeile
pro Datei, plus Header-Zeile.

---

## 10. Wie BUILD_INFO.txt weiterverwendet wird

### 10.1 Release-Body in GitHub

Der Release-Workflow nutzt `BUILD_INFO.txt` als Body der
GitHub-Release-Beschreibung:

```yaml
- name: Build
  run: |
    .\build.ps1 -GitStamp -GitNote "${{ github.event.head_commit.message }}"

- name: Read body
  id: body
  run: |
    $body = Get-Content BUILD_INFO.txt -Raw
    "RELEASE_BODY<<EOF" | Out-File -Append $env:GITHUB_ENV
    $body                 | Out-File -Append $env:GITHUB_ENV
    "EOF"                 | Out-File -Append $env:GITHUB_ENV

- uses: softprops/action-gh-release@v2
  with:
    body: ${{ env.RELEASE_BODY }}
```

Die Notiz (`-GitNote`) wandert so in die Release-Beschreibung.

### 10.2 Manuelle Pruefung

`BUILD_INFO.txt` ist eine flache Textdatei. Du kannst sie mit jedem
Editor oeffnen und z. B. per `grep` durchsuchen:

```cmd
findstr /C:"describe:" BUILD_INFO.txt
findstr /C:"Version:"  BUILD_INFO.txt
findstr /C:"Etappe:"   BUILD_INFO.txt
findstr /C:"Config:"   BUILD_INFO.txt
```

### 10.3 Verifikation des Build-Zustands

Die Datei listet alle Dateien mit Groesse. Wenn eine Datei fehlt oder
auffaellig klein ist, weisst du sofort, dass etwas schiefgelaufen ist.

Beispiel: `example_alpha_test.exe` ist normalerweise ~440 KB. Steht
dort `0.1 KB`, ist der Link-Schritt fehlgeschlagen und die Datei ist
ein Null-Byt-Placeholder.

### 10.4 Version + Etappe als Release-Anker

Die Zeilen `Version:` und `Etappe:` machen BUILD_INFO.txt zu einem
stabilen Anker fuer Release-Notizen und externe Referenzen. Ein
Verweis `Kernel 1.23.0 / Etappe 23` laesst sich direkt aus der Datei
extrahieren.

---

## 11. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `Version: unknown` | `ProPhysics_Version.h` fehlt | Header am Pfad pruefen |
| `Version: ?.?.?` | Header existiert, Regex passt nicht | Header-Format pruefen |
| `Etappe: unknown` | `PROPHYSICS_ETAPPE` fehlt im Header | Header pruefen |
| `Config:` fehlt | `-Config` nicht uebergeben | Parameter setzen, oder ignorieren |
| `Git: nicht verfuegbar` | kein Repo / kein git | `-GitStamp` weglassen |
| `describe: n/a` | kein Tag im Repo | `git tag <name>` anlegen |
| `status: dirty` | uncommitted Changes | committen, dann neu laufen |
| `bin\ (Verzeichnis fehlt)` | nichts gebaut | `pro_run build` laufen lassen |
| `bin\ (leer)` | Build fehlgeschlagen | Build-Log pruefen |
| Umlaute in der Datei kaputt | Editor interpretiert falsch | UTF-8 im Editor waehlen |
| Datei nicht gefunden | falscher `-RepoRoot` | Pfad pruefen |
| Falscher Zielordner | `-OutFile` mit falschem Pfad | `Split-Path` pruefen |

---

## 12. Was dieses Skript nicht tut

- **Kein Build.** Es liest nur den aktuellen Zustand.
- **Kein Zip.** Keine Archivierung.
- **Kein Validieren.** Fehlende Dateien werden vermerkt, aber nicht
  als Fehler behandelt.
- **Kein Rekursieren.** Es listet nur die oberste Ebene von `bin\`
  und `lib\`.
- **Keine Groessenlimit-Pruefung.** 1-KB-EXE wird nicht als verdaechtig
  markiert.
- **Kein Editieren der Version.** Die Version kommt aus dem Header,
  nicht aus dem Skript (ausser `-Version` wird explizit gesetzt).

---

## 13. Parameter-Referenz (kompakt)

```
write_build_info.ps1 [-RepoRoot <pfad>]
                     [-Config <release|debug>]
                     [-Version <string>]
                     [-OutFile <pfad>]
                     [-GitStamp]
                     [-GitNote "<text>"]
```

| Parameter | Typ | Pflicht | Beschreibung |
|---|---|---|---|
| `-RepoRoot` | Pfad | nein | Wurzelverzeichnis (Default: `$PSScriptRoot\..\..`) |
| `-Config` | Choice | nein | Build-Konfiguration |
| `-Version` | String | nein | Versions-Override |
| `-OutFile` | Pfad | nein | Ausgabepfad |
| `-GitStamp` | Switch | nein | Git-Metadaten einbetten |
| `-GitNote` | String | nein | Freitext (mehrzeilig moeglich) |

---

## 14. Zusammenspiel mit `build.ps1`

`build.ps1` ruft `write_build_info.ps1` am Ende jedes erfolgreichen
Builds auf. Die Parameter werden durchgereicht:

```powershell
$psArgs = @(
    '-NoProfile'
    '-ExecutionPolicy','Bypass'
    '-File', $InfoScript
    '-RepoRoot', $RepoRoot
    '-Config', $Config
)
if ($GitStamp) { $psArgs += '-GitStamp' }
if ($GitNote -and $GitNote.Trim().Length -gt 0) {
    $psArgs += '-GitNote'
    $psArgs += $GitNote
}
& powershell.exe @psArgs
```

**Doppelaufruf-Vermeidung:** Der Master-Makefile ruft in den Targets
`all`, `rebuild`, `rebuild_prophysics`, `rebuild_sdk`, `rebuild_test`
jeweils den `info:`-Target selbst auf. `build.ps1` erkennt das und
ruft `write_build_info.ps1` in diesen Faellen nicht erneut auf.

**Warum ein separater `powershell.exe`-Aufruf?** Isolation. Das Skript
laeuft in einem frischen Prozess mit eigenen Encoding-Einstellungen.
Damit ist es unmoeglich, dass der aufrufende Build-Prozess die
Konfiguration des Skripts veraendert (oder umgekehrt).

---

## 15. Zusammenspiel mit Master-Makefile

Der `info`-Target im Master-Makefile ruft das Skript mit minimalen
Argumenten auf:

```makefile
info:
	@echo.
	@echo [MASTER] === BUILD_INFO ===
	@powershell -NoProfile -ExecutionPolicy Bypass -File "$(INFO_SCRIPT)" -RepoRoot "$(ROOT)"
```

Ergebnis: eine `BUILD_INFO.txt` ohne Git- und Notiz-Block. Fuer die
volle Variante `build.ps1` verwenden.

**`$(INFO_SCRIPT)`** und **`$(ROOT)`** sind im Master-Makefile
definiert:

```makefile
ROOT        = $(MAKEDIR)\..\..
INFO_SCRIPT = $(MAKEDIR)\write_build_info.ps1
```

`$(MAKEDIR)` expandiert NMAKE auf das Verzeichnis des Makefiles. Der
Aufruf ist damit unabhaengig vom CWD.

---

## 16. Beispiele fuer typische Szenarien

### 16.1 Build mit Git-Info und Release-Notiz

```cmd
cd build\main
build.cmd -Mode all -Config release -Rebuild -GitStamp ^
          -GitNote "Release 1.0.0: Kernel 1.23.0 / Etappe 23"
```

Ergebnis: `BUILD_INFO.txt` enthaelt `Version: 1.23.0`,
`Etappe: 23`, `Config: release`, Git-Block und Notiz.

### 16.2 Nur Info aktualisieren

```cmd
pro_run build -Mode info -GitStamp
:: oder
build.cmd -NoBuild -GitStamp
```

Ergebnis: kein nmake-Build, nur die Info-Datei wird aktualisiert.

### 16.3 Minimal-Invocation fuer CI

```cmd
nmake /NOLOGO /f Makefile.nmake info
```

Ergebnis: `BUILD_INFO.txt` ohne Git/Notiz/Config.

### 16.4 Notiz ohne Git

```cmd
build.cmd -NoBuild -GitNote "Manual check of bin\ contents"
```

Ergebnis: `BUILD_INFO.txt` mit Notiz, ohne Git-Block.

### 16.5 Debug-Build dokumentieren

```cmd
build.cmd -Mode kernel -Config debug
```

Ergebnis: `BUILD_INFO.txt` enthaelt `Config: debug`.

### 16.6 Versions-Override fuer externe Referenz

```powershell
.\write_build_info.ps1 -Version 1.23.0-rc1 -OutFile D:\tmp\BUILD_INFO.txt
```

Ergebnis: Datei zeigt `Version: 1.23.0-rc1`, unabhaengig vom
Header-Stand.

---

## 17. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\export.md` — Export-Wrapper
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\sdk\Makefile.md` — SDK-Build
- `docs\build\test\Makefile.md` — Test-Build
- `docs\test\run_alpha_tests.md` — Test-Runner
- `docs\test\ProPhysics_Testkatalog.md` — Test-Übersicht

---

**Ende BUILD_INFO.txt-Dokumentation (v1.0.0).**