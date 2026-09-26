\# ProPhysics write\_build\_info — BUILD\_INFO.txt Generator



\*\*Datei:\*\* `build\\main\\write\_build\_info.ps1`

\*\*Version:\*\* 3.0 (Etappe 21)

\*\*Zweck:\*\* Erzeugt `BUILD\_INFO.txt` im Repo-Root mit Zeitstempel,

Version, Git-Metadaten (optional), Freitext-Notiz (optional) und einer

Liste aller Artefakte in `bin\\` und `lib\\`.



\---



\## 1. Was das Skript tut



Ein Aufruf → eine Textdatei im Repo-Root. Kein Build, kein Kopieren,

keine externen Abhängigkeiten außer optional `git`.



Der Ablauf:



1\. \*\*Version\*\* aus `src\\prophysics\\header\\ProPhysics\_Version.h` lesen

2\. \*\*Artefakte\*\* in `bin\\` und `lib\\` auflisten (Name + Größe)

3\. \*\*Git-Info\*\* sammeln (nur bei `-GitStamp`)

4\. \*\*Notiz\*\* einbetten (nur bei `-GitNote`)

5\. \*\*BUILD\_INFO.txt\*\* schreiben (überschreibt alte Datei)



Fehlt `bin\\` oder `lib\\`, wird das im Text vermerkt statt abzubrechen.

Das Skript ist robust gegen halb-fertige Zustände.



\---



\## 2. Ablageort



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── build\\

│   └── main\\

│       ├── write\_build\_info.ps1    <- dieses Skript

│       ├── build.ps1

│       ├── export.ps1

│       └── Makefile.nmake

├── bin\\

├── lib\\

├── src\\

│   └── prophysics\\

│       └── header\\

│           └── ProPhysics\_Version.h

└── BUILD\_INFO.txt                  <- Ziel

```



Der Repo-Root wird \*\*nicht\*\* automatisch ermittelt. Der Aufrufer

übergibt ihn explizit mit `-RepoRoot`. Das ist Absicht: das Skript

kann damit auch außerhalb des Repos liegen und einen entfernten Root

beschreiben.



\---



\## 3. Aufruf



\### 3.1 Direkt



```powershell

.\\write\_build\_info.ps1 -RepoRoot "H:\\ProPhysics\_SDK\\ProPhysics"

```



\### 3.2 Mit Git-Metadaten



```powershell

.\\write\_build\_info.ps1 -RepoRoot "H:\\ProPhysics\_SDK\\ProPhysics" `

&#x20;                      -GitStamp

```



\### 3.3 Mit Freitext-Notiz



```powershell

.\\write\_build\_info.ps1 -RepoRoot "H:\\ProPhysics\_SDK\\ProPhysics" `

&#x20;                      -GitNote "Etappe 21 abgeschlossen. Alle 41 Tests gruen."

```



\### 3.4 Über den Build-Wrapper



`build.ps1` ruft das Skript mit den passenden Flags auf:



```cmd

build.cmd -GitStamp -GitNote "Release v3.0-etappe21"

```



Damit landet die Notiz automatisch in `BUILD\_INFO.txt`.



\### 3.5 Über das Master-Makefile



Der `info`-Target im Master-Makefile ruft das Skript \*\*ohne\*\*

Git-Optionen auf:



```cmd

cd build\\main

nmake info

```



Ergebnis: minimale `BUILD\_INFO.txt` ohne Git- und Notiz-Block.



\---



\## 4. Parameter



| Parameter | Typ | Default | Beschreibung |

|---|---|---|---|

| `-RepoRoot` | Pfad | \*(mandatory)\* | Wurzelverzeichnis des Repos |

| `-GitStamp` | Switch | aus | Git-Metadaten einbetten |

| `-GitNote` | String | `''` | Freitext-Notiz einbetten (mehrzeilig) |



\*\*Hinweise:\*\*



\- `-RepoRoot` wird nicht validiert — fehlt der Ordner, wird die Datei

&#x20; trotzdem geschrieben (mit Hinweisen zu fehlenden Unterordnern).

\- `-GitStamp` und `-GitNote` sind \*\*unabhängig\*\*. Du kannst `-GitNote`

&#x20; ohne `-GitStamp` verwenden, und umgekehrt.

\- Fehlt `git` oder ist der Ordner kein Git-Repo: kein Fehler, nur eine

&#x20; Hinweiszeile im Git-Block.



\---



\## 5. Beispiel-Ausgaben



\### 5.1 Minimal



Aufruf ohne `-GitStamp`, ohne `-GitNote`:



```

ProPhysics Build Information

============================



Erzeugt:  2026-09-24 16:15:03

Host:     DEV-WORKSTATION

User:     koehn

Version:  3.0.0



Artefakte:



&#x20; bin\\

&#x20;   ProPhysics.dll                   263.7 KB

&#x20;   pro\_sdk\_interface.dll            138.2 KB

&#x20;   example\_alpha\_test.exe           442.4 KB

&#x20;   example\_test\_density.exe         177.7 KB

&#x20;   example\_test\_tensor.exe          163.3 KB

&#x20;   run\_alpha\_tests.cmd                0.1 KB

&#x20;   run\_alpha\_tests.ps1               16.4 KB



&#x20; lib\\

&#x20;   ProPhysics.lib                    51.4 KB

&#x20;   pro\_sdk\_interface.lib              2.4 KB



Quellen:

&#x20; src\\prophysics\\   Kernel (11 Module)

&#x20; src\\sdk\\          SDK Interface

&#x20; src\\test\\         Alpha-Test (15 Module) + 2 Example-Tests



Hinweis:

&#x20; Diese Datei wird bei jedem erfolgreichen Build neu erzeugt.

&#x20; Inhalt und Format sind dokumentiert in docs\\build\\BUILD\_SCRIPT.md.

```



\### 5.2 Mit Git-Info



Aufruf mit `-GitStamp`:



```

ProPhysics Build Information

============================



Erzeugt:  2026-09-24 16:15:03

Host:     DEV-WORKSTATION

User:     koehn

Version:  3.0.0



Git:

&#x20; describe: v3.0-etappe21-0-g1a2b3c4d

&#x20; branch:   main

&#x20; sha:      1a2b3c4d

&#x20; status:   clean



Artefakte:

...

```



\### 5.3 Mit Notiz



Aufruf mit `-GitNote`:



```

ProPhysics Build Information

============================



Erzeugt:  2026-09-24 16:15:03

Host:     DEV-WORKSTATION

User:     koehn

Version:  3.0.0



Notiz:

&#x20; Release v3.0-etappe21

&#x20; Dirac mit alpha-Kopplung

&#x20; Alle 41 Tests gruen



Artefakte:

...

```



\### 5.4 Mit Git-Info und Notiz



Aufruf mit `-GitStamp -GitNote "..."`:



```

ProPhysics Build Information

============================



Erzeugt:  2026-09-24 16:15:03

Host:     DEV-WORKSTATION

User:     koehn

Version:  3.0.0



Git:

&#x20; describe: v3.0-etappe21-0-g1a2b3c4d

&#x20; branch:   main

&#x20; sha:      1a2b3c4d

&#x20; status:   clean



Notiz:

&#x20; Release v3.0-etappe21

&#x20; Dirac mit alpha-Kopplung

&#x20; Alle 41 Tests gruen



Artefakte:

...

```



\### 5.5 Ohne Repo



Wenn `-GitStamp` gesetzt ist, aber kein Repo / kein git gefunden wird:



```

...

Version:  3.0.0



Git:      nicht verfuegbar (kein Repo oder git fehlt)



Artefakte:

...

```



Kein Fehler, Exit-Code bleibt 0.



\---



\## 6. Format und Aufbau



\### 6.1 Header-Block



| Feld | Quelle |

|---|---|

| `Erzeugt:` | `Get-Date -Format 'yyyy-MM-dd HH:mm:ss'` |

| `Host:` | `$env:COMPUTERNAME` |

| `User:` | `$env:USERNAME` |

| `Version:` | aus `ProPhysics\_Version.h` (MAJOR.MINOR.PATCH) |



\*\*Version-Fallback:\*\* Ist `ProPhysics\_Version.h` nicht lesbar, wird

`unknown` ausgegeben. Findet der Regex keine Werte, kommen `?` zum

Einsatz (`?.?.?`).



\### 6.2 Git-Block (optional)



Nur wenn `-GitStamp` gesetzt. Vier Zeilen mit fixen Prefixen:



| Zeile | Quelle | Wertebereich |

|---|---|---|

| `describe:` | `git describe --tags --dirty --always --long` | Tag-Name + SHA, oder `n/a` |

| `branch:` | `git rev-parse --abbrev-ref HEAD` | Branch-Name, oder `n/a` |

| `sha:` | `git rev-parse --short HEAD` | 7-Zeichen-Hash, oder `n/a` |

| `status:` | `git status --porcelain` | `clean` oder `dirty` |



\*\*Warum `--dirty` und `--long`?\*\* `--long` erzwingt das Format

`<tag>-<count>-g<sha>`, auch wenn direkt auf einem Tag. `--dirty`

markiert uncommitted Changes mit `-dirty`. Zusammen: der Nutzer sieht

auf einen Blick, ob die Version ein sauberer Tag ist oder ein

Zwischenstand.



\### 6.3 Notiz-Block (optional)



Nur wenn `-GitNote` gesetzt und nicht leer. Mehrzeilige Notizen

werden mit zwei Leerzeichen eingerückt:



```

Notiz:

&#x20; Zeile 1

&#x20; Zeile 2

&#x20; Zeile 3

```



\*\*Mehrzeilige Notiz\*\* übergibst du mit einem PowerShell-Here-String:



```powershell

$note = @"

Release v3.0-etappe21

\- Dirac mit alpha-Kopplung

\- Alle 41 Tests gruen

\- Regression 41/41

"@

.\\build.ps1 -GitStamp -GitNote $note

```



\### 6.4 Artefakt-Block



Zwei Abschnitte: `bin\\` und `lib\\`.



\*\*`bin\\`-Filter:\*\* `\*.dll`, `\*.exe`, `\*.cmd`, `\*.ps1`

\*\*`lib\\`-Filter:\*\* `\*.lib`



Format pro Datei:



```

&#x20;   <Name>                              <Größe> KB

```



Name linksbündig auf 32 Zeichen, Größe rechtsbündig auf 10 Zeichen,

in KB gerundet auf eine Nachkommastelle.



\*\*Sortierung:\*\* nach Name, eindeutig (keine Duplikate trotz

mehrerer Filter).



\*\*Fehlt der Ordner:\*\* Hinweis `bin\\  (Verzeichnis fehlt)`.

\*\*Leerer Ordner:\*\* Hinweis `bin\\  (leer)`.



\### 6.5 Quellen-Block



Statischer Text mit festen Zeilen:



```

Quellen:

&#x20; src\\prophysics\\   Kernel (11 Module)

&#x20; src\\sdk\\          SDK Interface

&#x20; src\\test\\         Alpha-Test (15 Module) + 2 Example-Tests

```



\*\*Nicht dynamisch.\*\* Wenn du die Modul-Anzahl änderst, passe den Text

im Skript an. Das ist Absicht: der Block dient als Orientierung, nicht

als Inventar.



\### 6.6 Hinweis-Block



Statischer Footer:



```

Hinweis:

&#x20; Diese Datei wird bei jedem erfolgreichen Build neu erzeugt.

&#x20; Inhalt und Format sind dokumentiert in docs\\build\\BUILD\_SCRIPT.md.

```



\---



\## 7. Enkodierung



Die Datei wird als \*\*UTF-8\*\* geschrieben (mit BOM von PowerShells

`Out-File -Encoding utf8`). Damit sind Umlaute in Pfaden und Notizen

sicher.



Die Konsole stellt UTF-8 selbst ein:



```powershell

\[Console]::OutputEncoding = \[System.Text.Encoding]::UTF8

$OutputEncoding           = \[System.Text.Encoding]::UTF8

```



Diese Zeilen sind \*\*nicht\*\* in einem `chcp`-Try-Block — sie wirken

nur auf die PowerShell-Ausgabe. Der Aufrufer (`build.ps1`) setzt

`chcp 65001` selbst.



\---



\## 8. Überschreiben-Verhalten



Die Datei wird \*\*immer komplett überschrieben\*\*. Der Ablauf:



```powershell

if (Test-Path -LiteralPath $outFile) {

&#x20;   Remove-Item -LiteralPath $outFile -Force

}

$lines -join \[Environment]::NewLine |

&#x20;   Out-File -LiteralPath $outFile -Encoding utf8

```



\*\*Konsequenz:\*\* Jeder Lauf erzeugt eine frische Datei mit neuem

Zeitstempel. Kein Anhängen, kein Merge.



\*\*Atomicität:\*\* Die Lösch-dann-Schreib-Sequenz ist nicht atomar.

Bei Absturz zwischen den beiden Schritten ist die Datei weg, nicht

halb. Das ist für eine Info-Datei akzeptabel.



\---



\## 9. Interne Funktionen



\### 9.1 `Get-ProVersion`



Liest drei Regex-Treffer aus `ProPhysics\_Version.h`:



```powershell

VERSION\_MAJOR   -> $maj

VERSION\_MINOR   -> $min

VERSION\_PATCH   -> $pat

```



Liefert `"$maj.$min.$pat"` oder `'unknown'` bei fehlender Datei.



\### 9.2 `Get-GitInfo`



Prüft `git rev-parse --is-inside-work-tree`, dann vier Abfragen.

Jede Abfrage in einem eigenen Try/Catch-Safe-Block. Bei Fehler:

`$null` zurück, damit der Aufrufer den Git-Block weglassen kann.



\*\*Wichtig:\*\* `Push-Location $RepoRoot` vor den Git-Aufrufen.

`Pop-Location` in `finally`. Damit läuft der Aufruf unabhängig vom

aktuellen CWD.



\### 9.3 `Get-ArtifactLines`



Listet Dateien aus einem Ordner mit mehreren Filtern. Filter werden

zusammengeführt, sortiert, dedupliziert (durch `Sort-Object Name

\-Unique`).



Rückgabe: `System.Collections.Generic.List\[string]` mit einer Zeile

pro Datei, plus Header-Zeile.



\---



\## 10. Wie BUILD\_INFO.txt weiterverwendet wird



\### 10.1 Release-Body in GitHub



Der Release-Workflow nutzt `BUILD\_INFO.txt` als Body der

GitHub-Release-Beschreibung:



```yaml

\- name: Build

&#x20; run: |

&#x20;   .\\build.ps1 -GitStamp -GitNote "${{ github.event.head\_commit.message }}"



\- name: Read body

&#x20; id: body

&#x20; run: |

&#x20;   $body = Get-Content BUILD\_INFO.txt -Raw

&#x20;   "RELEASE\_BODY<<EOF" | Out-File -Append $env:GITHUB\_ENV

&#x20;   $body                 | Out-File -Append $env:GITHUB\_ENV

&#x20;   "EOF"                 | Out-File -Append $env:GITHUB\_ENV



\- uses: softprops/action-gh-release@v2

&#x20; with:

&#x20;   body: ${{ env.RELEASE\_BODY }}

```



Die Notiz (`-GitNote`) wandert so in die Release-Beschreibung.



\### 10.2 Manuelle Prüfung



`BUILD\_INFO.txt` ist eine flache Textdatei. Du kannst sie mit jedem

Editor öffnen und z.B. per `grep` durchsuchen:



```cmd

findstr /C:"describe:" BUILD\_INFO.txt

findstr /C:"Version:" BUILD\_INFO.txt

```



\### 10.3 Verifikation des Build-Zustands



Die Datei listet alle Dateien mit Größe. Wenn eine Datei fehlt oder

auffällig klein ist, weißt du sofort, dass etwas schiefgelaufen ist.



Beispiel: `example\_alpha\_test.exe` ist normalerweise \~440 KB. Steht

dort `0.1 KB`, ist der Link-Schritt fehlgeschlagen und die Datei ist

ein Null-Byt-Placeholder.



\---



\## 11. Fehlersuche



| Symptom | Ursache | Fix |

|---|---|---|

| `Version: unknown` | `ProPhysics\_Version.h` fehlt | Header am Pfad prüfen |

| `Version: ?.?.?` | Header existiert, Regex passt nicht | Header-Format prüfen |

| `Git: nicht verfuegbar` | kein Repo / kein git | `-GitStamp` weglassen |

| `describe: n/a` | kein Tag im Repo | `git tag <name>` anlegen |

| `status: dirty` | uncommitted Changes | committen, dann neu laufen |

| `bin\\ (Verzeichnis fehlt)` | nichts gebaut | `build.ps1` laufen lassen |

| `bin\\ (leer)` | Build fehlgeschlagen | Build-Log prüfen |

| Umlaute in der Datei kaputt | Editor interpretiert falsch | UTF-8 im Editor wählen |

| Datei nicht gefunden | falscher `-RepoRoot` | Pfad prüfen |



\---



\## 12. Was dieses Skript nicht tut



\- \*\*Kein Build.\*\* Es liest nur den aktuellen Zustand.

\- \*\*Kein Zip.\*\* Keine Archivierung.

\- \*\*Kein Validieren.\*\* Fehlende Dateien werden vermerkt, aber nicht

&#x20; als Fehler behandelt.

\- \*\*Kein Rekursieren.\*\* Es listet nur die oberste Ebene von `bin\\`

&#x20; und `lib\\`.

\- \*\*Keine Größenlimit-Prüfung.\*\* 1-KB-EXE wird nicht als verdächtig

&#x20; markiert.

\- \*\*Kein Editieren der Version.\*\* Die Version kommt aus dem Header,

&#x20; nicht aus dem Skript.



\---



\## 13. Parameter-Referenz (kompakt)



```

write\_build\_info.ps1 -RepoRoot <pfad>

&#x20;                    \[-GitStamp]

&#x20;                    \[-GitNote "<text>"]

```



| Parameter | Typ | Pflicht | Beschreibung |

|---|---|---|---|

| `-RepoRoot` | Pfad | ja | Wurzelverzeichnis |

| `-GitStamp` | Switch | nein | Git-Metadaten einbetten |

| `-GitNote` | String | nein | Freitext (mehrzeilig möglich) |



\---



\## 14. Zusammenspiel mit `build.ps1`



`build.ps1` ruft `write\_build\_info.ps1` am Ende jedes erfolgreichen

Builds auf. Die Parameter werden durchgereicht:



```powershell

$psArgs = @(

&#x20;   '-NoProfile'

&#x20;   '-ExecutionPolicy','Bypass'

&#x20;   '-File', $InfoScript

&#x20;   '-RepoRoot', $RepoRoot

)

if ($GitStamp) { $psArgs += '-GitStamp' }

if ($GitNote -and $GitNote.Trim().Length -gt 0) {

&#x20;   $psArgs += '-GitNote'

&#x20;   $psArgs += $GitNote

}



\& powershell.exe @psArgs

```



\*\*Warum ein separater `powershell.exe`-Aufruf?\*\* Isolation. Das Skript

läuft in einem frischen Prozess mit eigenen Encoding-Einstellungen.

Damit ist es unmöglich, dass der aufrufende Build-Prozess die

Konfiguration des Skripts verändert (oder umgekehrt).



\---



\## 15. Zusammenspiel mit Master-Makefile



Der `info`-Target im Master-Makefile ruft das Skript mit minimalen

Argumenten auf:



```makefile

info:

&#x09;@echo.

&#x09;@echo \[MASTER] === BUILD\_INFO ===

&#x09;@powershell -NoProfile -ExecutionPolicy Bypass -File "$(INFO\_SCRIPT)" -RepoRoot "$(ROOT)"

```



Ergebnis: eine `BUILD\_INFO.txt` ohne Git- und Notiz-Block. Für die

volle Variante `build.ps1` verwenden.



\*\*`$(INFO\_SCRIPT)`\*\* und \*\*`$(ROOT)`\*\* sind im Master-Makefile

definiert:



```makefile

ROOT        = $(MAKEDIR)\\..\\..

INFO\_SCRIPT = $(MAKEDIR)\\write\_build\_info.ps1

```



`$(MAKEDIR)` expandiert NMAKE auf das Verzeichnis des Makefiles. Der

Aufruf ist damit unabhängig vom CWD.



\---



\## 16. Beispiele für typische Szenarien



\### 16.1 Build mit Git-Info und Release-Notiz



```cmd

cd build\\main

build.cmd -Mode all -Rebuild -GitStamp -GitNote "Release v3.0-etappe21"

```



Ergebnis: `BUILD\_INFO.txt` enthält Git-Block und Notiz.



\### 16.2 Nachträgliche Info-Aktualisierung



```cmd

build.cmd -NoBuild -GitStamp

```



Ergebnis: kein nmake-Aufruf, nur die Info-Datei wird aktualisiert.



\### 16.3 Minimal-Invocation für CI



```cmd

nmake /NOLOGO /f Makefile.nmake info

```



Ergebnis: `BUILD\_INFO.txt` ohne Git/Notiz.



\### 16.4 Notiz ohne Git



```cmd

build.cmd -NoBuild -GitNote "Manual check of bin\\ contents"

```



Ergebnis: `BUILD\_INFO.txt` mit Notiz, ohne Git-Block.



\---



\## 17. Siehe auch



\- `docs\\build\\BUILD\_SCRIPT.md` — Übersicht des Build-Systems

\- `docs\\build\\helper\\build.md` — Build-Wrapper

\- `docs\\build\\helper\\export.md` — Export-Wrapper

\- `docs\\build\\helper\\run\_alpha\_tests.md` — Test-Runner

\- `docs\\build\\main\\Makefile.md` — Master-Makefile

\- `docs\\build\\prophysics\\Makefile.md` — Kernel-Build

\- `docs\\build\\sdk\\Makefile.md` — SDK-Build

\- `docs\\build\\test\\Makefile.md` — Test-Build



\---



\*\*Ende BUILD\_INFO.txt-Dokumentation.\*\*



