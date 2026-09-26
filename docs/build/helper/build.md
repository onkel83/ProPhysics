\# ProPhysics Build — Master-Wrapper



\*\*Dateien:\*\* `build\\main\\build.ps1` + `build\\main\\build.cmd`

\*\*Version:\*\* 3.0 (Etappe 21)

\*\*Zweck:\*\* Dünner Wrapper um `build\\main\\Makefile.nmake`, delegiert

BUILD\_INFO an `write\_build\_info.ps1`.



\---



\## 1. Was das Skript tut



Ein Aufruf → mehrere Aktionen, die sonst einzeln nötig wären:



1\. \*\*UTF-8-Konsole\*\* einstellen (`\[Console]::OutputEncoding`, `chcp 65001`)

2\. \*\*Git-Info\*\* sammeln (optional, nur bei `-GitStamp`)

3\. \*\*nmake\*\* mit dem aus Mode + Flags abgeleiteten Target aufrufen

4\. \*\*Signieren\*\* (optional, nur bei `-Sign`)

5\. \*\*BUILD\_INFO.txt\*\* über `write\_build\_info.ps1` schreiben

6\. \*\*Zusammenfassung\*\* ausgeben (Anzahl Dateien in `bin\\` und `lib\\`)



Der Aufruf ist idempotent: ein zweiter Lauf ohne Änderungen ist schnell

(nmake baut nichts Neues) und schreibt BUILD\_INFO neu.



\---



\## 2. Ablageort



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── build\\

│   └── main\\

│       ├── build.ps1        <- dieses Skript

│       ├── build.cmd        <- Wrapper für cmd.exe

│       ├── Makefile.nmake   <- Master-Build

│       └── write\_build\_info.ps1

├── bin\\

├── lib\\

└── BUILD\_INFO.txt           <- Ziel

```



Das Skript ermittelt den Repo-Root selbst:



```powershell

$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\\..')).Path

```



Aufruf funktioniert damit \*\*aus jedem CWD\*\*.



\---



\## 3. Aufruf



\### 3.1 Über `build.cmd` (empfohlen für cmd.exe)



```cmd

build.cmd

build.cmd -Mode sdk -Rebuild

build.cmd -Mode prophysics -Clean

```



Der `.cmd`-Wrapper setzt `chcp 65001` und reicht alle Argumente an

PowerShell weiter.



\### 3.2 Direkt über PowerShell



```powershell

.\\build.ps1

.\\build.ps1 -Mode sdk -Rebuild -GitStamp -GitNote "SDK v3.0"

```



\### 3.3 Aus jedem anderen CWD



```cmd

cd C:\\Temp

H:\\ProPhysics\_SDK\\ProPhysics\\build\\main\\build.cmd -Mode prophysics

```



Funktioniert, weil alle Pfade über `$PSScriptRoot` bestimmt werden.



\---



\## 4. Modi — Auswahl der Komponenten



`-Mode` bestimmt, welche Komponente gebaut wird. Die Kette ist

kumulativ, weil SDK und Tests auf dem Kernel aufbauen.



| `-Mode` | Was gebaut wird | nmake-Target |

|---|---|---|

| `prophysics` | nur Kernel (DLL + LIB) | `prophysics` |

| `sdk` | Kernel + SDK-Interface | `sdk` |

| `test` | Kernel + SDK + 3 Test-EXEs | `test` |

| `all` (Default) | identisch mit `test` | `all` |



Der Unterschied zwischen `test` und `all` ist aktuell keiner — beide

rufen dieselbe Kette auf. `all` ist die sprachlich klarere Variante,

`test` betont, dass die Tests mitkommen.



\*\*Kumulativ heißt:\*\* `-Mode sdk` baut den Kernel mit, wenn er nicht

schon da ist. `-Mode prophysics` baut \*\*nicht\*\* SDK und Tests.



\---



\## 5. Flags



\### 5.1 Build-Steuerung



| Flag | Wirkung |

|---|---|

| \*(kein Flag)\* | inkrementeller Build (nur was sich geändert hat) |

| `-Rebuild` | `nmake clean` + kompletter Build |

| `-Clean` | nur `nmake clean` — kein Build, keine BUILD\_INFO |

| `-NoBuild` | Build überspringen — nur BUILD\_INFO.txt neu |



\*\*`-Rebuild`\*\* ist der Standard für Release-Läufe: erst alles weg,

dann alles neu.



\*\*`-Clean`\*\* löscht `bin\\`, `lib\\` und `BUILD\_INFO.txt`. Nach einem

`-Clean`-Lauf ist der Zustand wie bei einem frischen Checkout.



\*\*`-NoBuild`\*\* ist nützlich, wenn du nur die BUILD\_INFO aktualisieren

willst (z.B. nach einem manuellen `nmake`-Lauf).



\### 5.2 Git-Metadaten



| Flag | Wirkung |

|---|---|

| `-GitStamp` | Git-Info (describe/branch/sha/status) in BUILD\_INFO.txt |

| `-GitNote "text"` | Freitext-Notiz in BUILD\_INFO.txt, unabhängig von `-GitStamp` |



\*\*`-GitStamp`\*\* ruft im Repo-Root auf:



```

git describe --tags --dirty --always --long

git rev-parse --abbrev-ref HEAD

git rev-parse --short HEAD

git status --porcelain

```



Bei fehlendem git oder nicht-Repo: kein Fehler, nur Hinweiszeile.



\*\*`-GitNote`\*\* funktioniert ohne git. Mehrzeilig möglich

(PowerShell-Here-String). Beispiel-Ausgabe:



```

Notiz:

&#x20; Release v3.0-etappe21

&#x20; Dirac mit alpha-Kopplung

&#x20; Alle 41 Tests gruen

```



\### 5.3 Signieren



| Flag | Wirkung |

|---|---|

| `-Sign` | DLLs in `bin\\` via `signtool` signieren |



Ruft für jede DLL in `bin\\`:



```

signtool sign /a /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 <dll>

```



\- `/a` — automatisches Zertifikat (Standard im User-Store)

\- `/fd SHA256` — Datei-Hash

\- `/tr` + `/td` — RFC-3161-Timestamp (DigiCert)



Fehlt `signtool.exe` im PATH: Warnung, Exit bleibt 0. Fehlende Signatur

ist kein Build-Fehler.



\### 5.4 Dry-Run



| Flag | Wirkung |

|---|---|

| `-DryRun` | nur auflisten, nichts schreiben |



Unterdrückt:

\- `nmake`-Aufrufe (zeigt stattdessen `\[dry] nmake …`)

\- `signtool` (nur Hinweis)

\- `write\_build\_info.ps1` (nur Hinweis)



Nützlich vor einem großen Build, um zu sehen, welche Aktionen

stattfinden würden.



\---



\## 6. Beispiel-Aufrufe



\### 6.1 Standard-Build (Kernel + SDK + Tests + Info)



```cmd

cd build\\main

build.cmd

```



Ergebnis: `bin\\` enthält alle DLLs und EXEs, `lib\\` enthält beide Libs,

`BUILD\_INFO.txt` im Repo-Root.



\### 6.2 Nur Kernel, kompletter Rebuild



```cmd

build.cmd -Mode prophysics -Rebuild

```



Ergebnis: `bin\\ProPhysics.dll` und `lib\\ProPhysics.lib` neu gebaut.

SDK- und Test-Artefakte bleiben unangetastet.



\### 6.3 SDK-Build mit Git-Metadaten



```cmd

build.cmd -Mode sdk -GitStamp -GitNote "SDK-Interface v3.0"

```



Ergebnis: Kernel + SDK-Interface. BUILD\_INFO.txt enthält

Git-Block + Notiz.



\### 6.4 Alles sauber weg



```cmd

build.cmd -Clean

```



Ergebnis: `bin\\` und `lib\\` sind leer, `BUILD\_INFO.txt` entfernt.



\### 6.5 Nur BUILD\_INFO aktualisieren



```cmd

build.cmd -NoBuild

```



Ergebnis: kein nmake-Aufruf, aber `BUILD\_INFO.txt` wird neu geschrieben

(mit aktuellem Zeitstempel und Dateiliste).



\### 6.6 Release-Build



```cmd

build.cmd -Mode all -Rebuild -GitStamp -Sign `

&#x20;         -GitNote "Release v3.0-etappe21: Dirac mit alpha-Kopplung."

```



Ergebnis: kompletter Neu-Build, Git-Metadaten, signierte DLLs,

BUILD\_INFO mit Notiz.



\### 6.7 Dry-Run vor Release



```cmd

build.cmd -Mode all -Rebuild -GitStamp -Sign -DryRun

```



Prüft in einem Durchgang: nmake-Target, signtool-Verfügbarkeit,

Git-Erreichbarkeit. Kein Schreibzugriff.



\---



\## 7. Was in BUILD\_INFO.txt landet



Das Skript delegiert das an `write\_build\_info.ps1`. Ausgabe-Beispiel

ohne Git/Notiz:



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



&#x20; lib\\

&#x20;   ProPhysics.lib                    51.4 KB

&#x20;   pro\_sdk\_interface.lib              2.4 KB

...

```



Mit `-GitStamp` und `-GitNote`:



```

...

Version:  3.0.0



Git:

&#x20; describe: v3.0-etappe21-0-g1a2b3c4d

&#x20; branch:   main

&#x20; sha:      1a2b3c4d

&#x20; status:   clean



Notiz:

&#x20; Release v3.0-etappe21: Dirac mit alpha-Kopplung.



Artefakte:

...

```



Details zum Format siehe `docs\\build\\helper\\write\_build\_info.md`.



\---



\## 8. Ausgabe auf der Konsole



```

============================================================

&#x20; ProPhysics Build  |  Modus: all

============================================================

&#x20; Repo:   H:\\ProPhysics\_SDK\\ProPhysics

&#x20; Build:  H:\\ProPhysics\_SDK\\ProPhysics\\build\\main



\[\*] nmake all

...

&#x20;   OK  nmake all abgeschlossen.

\[\*] BUILD\_INFO.txt schreiben...

&#x20;   OK  H:\\...\\BUILD\_INFO.txt



\------------------------------------------------------------

&#x20; Zusammenfassung

\------------------------------------------------------------

&#x20; bin\\:   2 DLL(s), 3 EXE(s)

&#x20; lib\\:   2 LIB(s)

&#x20; Root:   BUILD\_INFO.txt



```



Farbcodierung:



| Farbe | Bedeutung |

|---|---|

| Cyan | Schritt-Markierung |

| Grün | Erfolg |

| Gelb | Warnung (nicht kritisch) |

| Rot | Fehler |

| Grau | Dry-Run-Detail |



\---



\## 9. Exit-Codes



| Code | Bedeutung |

|---|---|

| `0` | Erfolg |

| `2` | `nmake` nicht im PATH |

| `≠0` | `nmake`-Exit-Code (Build-Fehler) |



Bei Fehler in `nmake` wird die Ausgabe nicht unterdrückt — der Nutzer

sieht die volle Compiler-/Linker-Meldung.



\---



\## 10. Interner Ablauf



```

build.ps1

&#x20;   │

&#x20;   ├─ UTF-8 einstellen (Konsole + chcp 65001)

&#x20;   │

&#x20;   ├─ -GitStamp?  ─→ Git-Vorschau ausgeben (nur Info)

&#x20;   │

&#x20;   ├─ -Clean und nicht -Rebuild?

&#x20;   │   ├─ ja:  nur nmake clean\_<scope>, dann Info überspringen

&#x20;   │   └─ nein: weiter

&#x20;   │

&#x20;   ├─ -NoBuild?

&#x20;   │   ├─ ja:  Build überspringen

&#x20;   │   └─ nein: nmake <target> aufrufen

&#x20;   │

&#x20;   ├─ -Sign und nicht skipBuild?

&#x20;   │   └─ signtool über bin\\\*.dll

&#x20;   │

&#x20;   ├─ write\_build\_info.ps1 aufrufen

&#x20;   │   ├─ mit -GitStamp wenn gesetzt

&#x20;   │   └─ mit -GitNote wenn gesetzt

&#x20;   │

&#x20;   └─ Zusammenfassung ausgeben

```



\*\*Zwei Sonderfälle:\*\*



1\. `-Clean` ohne `-Rebuild`: nur Clean, keine BUILD\_INFO

2\. `-NoBuild`: kein nmake, aber BUILD\_INFO wird geschrieben



\---



\## 11. Fehlersuche



| Symptom | Ursache | Fix |

|---|---|---|

| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausführen |

| `write\_build\_info.ps1 nicht gefunden` | Skript umbenannt/gelöscht | Datei in `build\\main\\` prüfen |

| `BUILD\_INFO.txt fehlt` nach Lauf | `-Clean` gesetzt | ohne `-Clean` neu laufen |

| `signtool.exe nicht gefunden` | Windows SDK fehlt | Warnung ist normal, Exit bleibt 0 |

| Git-Vorschau zeigt `nicht verfuegbar` | kein Repo / kein git | `-GitStamp` weglassen |

| Umlaute kaputt in Konsole | PowerShell umging `chcp` | `build.cmd` statt `build.ps1` verwenden |

| nmake-Target unbekannt | `-Mode`-Wert falsch | `ValidateSet` in `build.ps1` zeigt erlaubte Werte |



\---



\## 12. Was dieses Skript nicht tut



\- \*\*Keine Direktaufrufe der Sub-Makefiles.\*\* Es nutzt ausschließlich

&#x20; `build\\main\\Makefile.nmake` und dessen Targets.

\- \*\*Kein Paketieren.\*\* Artefakte bleiben in `bin\\` und `lib\\`. Für

&#x20; Export-Pakete siehe `export.ps1`.

\- \*\*Kein Zip.\*\* Keine Archivierung.

\- \*\*Kein Deployment.\*\* Kein Push in Repos oder Verzeichnisse.

\- \*\*Keine Test-Ausführung.\*\* Die Tests laufen über

&#x20; `bin\\run\_alpha\_tests.ps1`.

\- \*\*Kein Rebuild der `.cmd`-Wrapper.\*\* Die sind statisch.



\---



\## 13. Parameter-Referenz (kompakt)



```

build.cmd \[-Mode <prophysics|sdk|test|all>]

&#x20;         \[-Rebuild] \[-Clean] \[-NoBuild]

&#x20;         \[-GitStamp] \[-GitNote "text"]

&#x20;         \[-Sign] \[-DryRun]

```



| Parameter | Typ | Default | Beschreibung |

|---|---|---|---|

| `-Mode` | Choice | `all` | Was gebaut wird |

| `-Rebuild` | Switch | aus | clean + Build |

| `-Clean` | Switch | aus | nur clean |

| `-NoBuild` | Switch | aus | nur BUILD\_INFO |

| `-GitStamp` | Switch | aus | Git-Metadaten in BUILD\_INFO |

| `-GitNote` | String | `''` | Freitext in BUILD\_INFO |

| `-Sign` | Switch | aus | DLLs signieren |

| `-DryRun` | Switch | aus | nur auflisten |



\---



\## 14. Siehe auch



\- `docs\\build\\BUILD\_SCRIPT.md` — Übersicht des gesamten Build-Systems

\- `docs\\build\\helper\\export.md` — Export-Skript

\- `docs\\build\\helper\\write\_build\_info.md` — BUILD\_INFO.txt-Format

\- `docs\\build\\helper\\run\_alpha\_tests.md` — Test-Runner

\- `docs\\build\\main\\Makefile.md` — Master-Makefile

\- `docs\\build\\prophysics\\Makefile.md` — Kernel-Build

\- `docs\\build\\sdk\\Makefile.md` — SDK-Build

\- `docs\\build\\test\\Makefile.md` — Test-Build



\---



\*\*Ende Build-Wrapper-Dokumentation.\*\*



