\# ProPhysics Test Build — NMAKE Build



\*\*Datei:\*\* `build\\test\\Makefile.nmake`

\*\*Version:\*\* 3.0 (Etappe 21)

\*\*Zweck:\*\* Bau der drei Test-EXEs aus den Quellen in `src\\test\\`. Linkt

gegen `ProPhysics.lib` und `pro\_sdk\_interface.lib`.



\---



\## 1. Was gebaut wird



Aus 17 `.c`-Dateien entstehen \*\*drei\*\* EXEs:



| Artefakt | Pfad | Quellen | Rolle |

|---|---|---|---|

| `example\_alpha\_test.exe` | `bin\\` | 15 Dateien (multi-file) | Alpha-Test-Suite (Prio 1–7, 41 Tests) |

| `example\_test\_density.exe` | `bin\\` | 1 Datei | Dichte-Trilogie-Regression (86 Checks) |

| `example\_test\_tensor.exe` | `bin\\` | 1 Datei | Tensor-/Fock-Regression (61 Checks) |



Alle drei EXEs werden \*\*direkt neben die DLLs\*\* in `bin\\` gelegt. Der

Windows-Loader findet `ProPhysics.dll` und `pro\_sdk\_interface.dll` dann

automatisch beim Start — kein `PATH`-Eintrag, kein Kopieren.



\*\*Keine\*\* Header-Kopien. Header bleiben am Pflegeort in

`src\\test\\header\\` und `src\\prophysics\\header\\`.



\---



\## 2. Ablageort und Aufruf



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── build\\

│   └── test\\

│       └── Makefile.nmake         <- wird HIER ausgeführt

├── src\\

│   ├── test\\

│   │   ├── alpha\_test\_\*.c          (15 Dateien)

│   │   ├── example\_test\_\*.c        (2 Dateien)

│   │   └── header\\

│   │       └── alpha\_test\_common.h

│   ├── prophysics\\

│   │   └── header\\\*.h              (Kernel-Header)

│   └── sdk\\

│       └── header\\pro\_sdk\_interface.h

├── bin\\

│   ├── ProPhysics.dll              (aus prophysics-Build)

│   ├── pro\_sdk\_interface.dll       (aus sdk-Build)

│   └── example\_\*.exe               <- Ziel

└── lib\\

&#x20;   ├── ProPhysics.lib              (Eingang)

&#x20;   └── pro\_sdk\_interface.lib       (Eingang)

```



\*\*Aufruf\*\* immer aus dem Ablageort des Makefiles:



```cmd

cd build\\test

nmake /NOLOGO /f Makefile.nmake

```



Alle Pfade sind relativ zu `build\\test\\`.



\---



\## 3. Verzeichnis-Layout (relativ zu `build\\test\\`)



| Symbol | Wert | Bemerkung |

|---|---|---|

| `SRC\_DIR` | `..\\..\\src\\test` | Quellen |

| `TEST\_HDR\_DIR` | `..\\..\\src\\test\\header` | `alpha\_test\_common.h` |

| `OBJ\_DIR` | `\_obj` | temporäre Objektdateien |

| `BIN\_DIR` | `..\\..\\bin` | Ziel-EXEs |

| `LIB\_DIR` | `..\\..\\lib` | Import-Libs (Eingang) |

| `CORE\_LIB` | `..\\..\\lib\\ProPhysics.lib` | Kernel-Lib |

| `SDK\_LIB` | `..\\..\\lib\\pro\_sdk\_interface.lib` | SDK-Lib |



\---



\## 4. Targets



| Target | Wirkung |

|---|---|

| `all` (Default) | `check\_deps` + `setup` + alle drei EXEs + `postclean` |

| `check\_deps` | prüft, dass `ProPhysics.lib` und `pro\_sdk\_interface.lib` existieren |

| `setup` | `\_obj\\` anlegen (idempotent) |

| `postclean` | räumt `\_obj\\\*.obj`, `\*.exp`, `\*.lib` ab |

| `clean` | `\_obj\\`, alle drei EXEs entfernen |



Es gibt \*\*keine\*\* separaten Targets pro Test — der `all`-Target baut

alle drei EXEs, weil sie dieselben Header und Libs brauchen.



\---



\## 5. Compiler-Flags im Detail



```

CFLAGS = /nologo /O2 /Ob2 /Oi /GL /W3 /MP \\

&#x20;        /I..\\..\\src\\test\\header \\

&#x20;        /I..\\..\\src\\prophysics\\header \\

&#x20;        /I..\\..\\src\\sdk\\header \\

&#x20;        /D\_CRT\_SECURE\_NO\_WARNINGS

```



| Flag | Bedeutung |

|---|---|

| `/nologo` | kein Copyright-Banner |

| `/O2` | maximale Optimierung |

| `/Ob2` | aggressives Inlining |

| `/Oi` | intrinsische Funktionen |

| `/GL` | Whole-Program-Optimization (Link-Time Code Generation) |

| `/W3` | Warnstufe 3 (Kernel: `/W4`, SDK: `/W3`) |

| `/MP` | Multi-Prozessor-Kompilierung (parallel über die 15 Dateien) |

| `/I..\\..\\src\\test\\header` | `alpha\_test\_common.h` |

| `/I..\\..\\src\\prophysics\\header` | Kernel-Header |

| `/I..\\..\\src\\sdk\\header` | SDK-Header |



\*\*Kein\*\* `DLL\_FLAGS`. Die EXEs exportieren nichts — sie sind Endpunkte.



\---



\## 6. Drei EXEs, zwei Build-Muster



\### 6.1 Single-File-EXEs (Density, Tensor)



`example\_test\_density.c` und `example\_test\_tensor.c` sind eigenständige

Programme mit `main()`.



```

cl.exe $(CFLAGS) $(SRC\_DIR)\\example\_test\_density.c \\

&#x20;   /Fo$(OBJ\_DIR)\\ /Fe$@ \\

&#x20;   /link $(SDK\_LIB) $(CORE\_LIB)

```



MSVC kompiliert und linkt in \*\*einem\*\* Aufruf. `/Fe` setzt den EXE-Namen,

`/Fo` legt die `.obj` in `\_obj\\`. Der `/link`-Abschnitt übergibt die

Import-Libs.



\### 6.2 Multi-File-EXE (Alpha-Test)



`example\_alpha\_test.exe` besteht aus 15 `.c`-Dateien:



```

cl.exe $(CFLAGS) $(ALPHA\_SOURCES) \\

&#x20;   /Fo$(OBJ\_DIR)\\ /Fe$@ \\

&#x20;   /link $(SDK\_LIB) $(CORE\_LIB)

```



MSVC kompiliert alle 15 Dateien — bei aktivem `/MP` parallel — und

linkt sie am Ende. Die OBJs landen \*\*alle\*\* in `\_obj\\`. Das ist die

einzige Stelle, an der `cl.exe` auch als Linker-Aufruf agiert.



\*\*Wichtig:\*\* `/Fo$(OBJ\_DIR)\\` mit \*\*abschließendem Backslash\*\*.

Ohne ihn hängt MSVC den Objektnamen direkt an den Pfad an

(`\_objProPhysics\_Core.obj` statt `\_obj\\ProPhysics\_Core.obj`).



\---



\## 7. Link



Zwei Import-Libs werden verlinkt:



```

/link $(SDK\_LIB) $(CORE\_LIB)

```



\- `$(SDK\_LIB)` = `..\\..\\lib\\pro\_sdk\_interface.lib`

\- `$(CORE\_LIB)` = `..\\..\\lib\\ProPhysics.lib`



\*\*Reihenfolge:\*\* SDK-Lib \*\*vor\*\* Kernel-Lib. MSVC löst Symbole von

links nach rechts auf. Wenn die SDK-Lib auf Kernel-Symbole verweist

(was sie tut — sie nutzt `ProPhysics\_\*`-Funktionen), muss die

Kernel-Lib rechts stehen.



Die Reihenfolge ist nicht optional. Vertauscht man sie, kommt

`unresolved external symbol`.



\---



\## 8. Post-Link-Aufräumen



```

postclean:

&#x20;   @if exist "$(OBJ\_DIR)\\\*.obj" del /Q "$(OBJ\_DIR)\\\*.obj" 2>nul

&#x20;   @if exist "$(OBJ\_DIR)\\\*.exp" del /Q "$(OBJ\_DIR)\\\*.exp" 2>nul

&#x20;   @if exist "$(OBJ\_DIR)\\\*.lib" del /Q "$(OBJ\_DIR)\\\*.lib" 2>nul

```



MSVC legt beim Linken zusätzliche Artefakte ab:



| Datei | Wo | Zweck |

|---|---|---|

| `.obj` | `\_obj\\` | Objektdateien |

| `.exp` | `\_obj\\` | Export-Datei (leer bei EXEs, aber trotzdem angelegt) |

| `.lib` | `\_obj\\` | Import-Lib (leer bei EXEs) |



Alle drei Typen werden gelöscht. Nach dem Build ist `\_obj\\` leer — nur

der Verzeichniseintrag bleibt.



\---



\## 9. Was nach dem Build wo liegt



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── bin\\

│   ├── ProPhysics.dll              (aus prophysics-Build)

│   ├── pro\_sdk\_interface.dll       (aus sdk-Build)

│   ├── example\_alpha\_test.exe      <- final

│   ├── example\_test\_density.exe    <- final

│   └── example\_test\_tensor.exe     <- final

├── lib\\

│   ├── ProPhysics.lib              (aus prophysics-Build)

│   └── pro\_sdk\_interface.lib       (aus sdk-Build)

└── build\\

&#x20;   └── test\\

&#x20;       └── \_obj\\                   <- leer (nur Verzeichnis)

```



`bin\\` ist flach: DLLs und EXEs nebeneinander. Die EXE findet ihre

DLLs beim Start automatisch, weil beide im selben Verzeichnis liegen.



\---



\## 10. Clean



```cmd

nmake /NOLOGO /f Makefile.nmake clean

```



Löscht:

\- `\_obj\\` komplett (rekursiv)

\- `bin\\example\_alpha\_test.exe`

\- `bin\\example\_test\_density.exe`

\- `bin\\example\_test\_tensor.exe`



\*\*Nicht\*\* angetastet werden:

\- `bin\\ProPhysics.dll`

\- `bin\\pro\_sdk\_interface.dll`

\- `lib\\\*.lib`

\- `BUILD\_INFO.txt` im Root



Sauberes Zusammenspiel mit `build\\main\\Makefile.nmake`, das sein

`clean\_test`-Target auf dieses `clean` mapped.



\---



\## 11. Abhängigkeits-Graph



```

check\_deps       (Prüfung: beide Import-Libs existieren)

&#x20;     │

&#x20;     ▼

setup            (mkdir \_obj\\)

&#x20;     │

&#x20;     ├──> example\_test\_density.c   ──> example\_test\_density.exe

&#x20;     │

&#x20;     ├──> example\_test\_tensor.c    ──> example\_test\_tensor.exe

&#x20;     │

&#x20;     └──> alpha\_test\_main.c    ┐

&#x20;          alpha\_test\_common.c  │

&#x20;          alpha\_test\_basic.c   │

&#x20;          ... (15 Dateien)     ├──> example\_alpha\_test.exe

&#x20;          alpha\_test\_dirac.c   │

&#x20;                                ┘

&#x20;     │

&#x20;     ▼

postclean        (delete \_obj\\\*.obj, \*.exp, \*.lib)

```



Alle drei EXEs hängen von allen Headern ab. Header-Änderung rebuildet

alles. Der `postclean`-Schritt läuft erst nach dem letzten Link.



\---



\## 12. Reihenfolge in der Build-Kette



```

build\\prophysics   →  bin\\ProPhysics.dll + lib\\ProPhysics.lib

&#x20;       │

&#x20;       ▼

build\\sdk          →  bin\\pro\_sdk\_interface.dll + lib\\pro\_sdk\_interface.lib

&#x20;       │

&#x20;       ▼

build\\test         →  bin\\example\_\*.exe       (dieses Makefile)

```



Der `check\_deps`-Schritt bricht ab, wenn eine der beiden Import-Libs

fehlt. Damit kann der Test-Build nicht versehentlich ohne die

Vorgänger laufen.



\---



\## 13. Wichtige Voraussetzungen



1\. \*\*Beide Import-Libs müssen existieren.\*\*

&#x20;  `..\\..\\lib\\ProPhysics.lib` und `..\\..\\lib\\pro\_sdk\_interface.lib`.

&#x20;  Sonst bricht `check\_deps` ab mit:

&#x20;  ```

&#x20;  \[FEHLER] ..\\..\\lib\\ProPhysics.lib fehlt. Zuerst build\\prophysics bauen.

&#x20;  NMAKE : fatal error U1077: ... Rückgabe-Code "0x1"

&#x20;  ```



2\. \*\*`alpha\_test\_common.h` muss in `src\\test\\header\\` liegen.\*\*

&#x20;  Sie wird von allen `alpha\_test\_\*.c` inkludiert.



3\. \*\*Alle 15 Alpha-Test-Quellen müssen vorhanden sein.\*\*

&#x20;  Das Makefile listet sie explizit im `ALPHA\_SOURCES`-Block. Fehlt eine,

&#x20;  meldet NMAKE sie als unbekannte Regel — oder `cl.exe` bricht mit

&#x20;  `cannot open source file` ab.



4\. \*\*MSVC-Toolchain im PATH.\*\* Wie bei den beiden anderen Builds.



5\. \*\*Kernel- und SDK-Header\*\* müssen an ihren Pflegeorten liegen

&#x20;  (`src\\prophysics\\header\\`, `src\\sdk\\header\\`).



\---



\## 14. Fehlersuche



| Symptom | Ursache | Fix |

|---|---|---|

| `\[FEHLER] ProPhysics.lib fehlt` | Kernel nicht gebaut | `cd build\\prophysics \&\& nmake` |

| `\[FEHLER] pro\_sdk\_interface.lib fehlt` | SDK nicht gebaut | `cd build\\sdk \&\& nmake /f Makefile.sdk.nmake` |

| `alpha\_test\_common.h: No such file` | Header fehlt | Header nach `src\\test\\header\\` |

| `unresolved external symbol ProPhysics\_...` | Link-Reihenfolge | SDK-Lib muss \*\*vor\*\* Kernel-Lib stehen |

| `example\_\*.obj in CWD` statt `\_obj\\` | fehlender Backslash in `/Fo` | `/Fo$(OBJ\_DIR)\\` prüfen |

| `LNK2019: \_main` | keine `main()` in einer Datei | `alpha\_test\_main.c` oder `example\_test\_\*.c` prüfen |

| `fatal error U1077` beim Link | Linker-Ausgabe unterdrückt | `nmake` ohne `/NOLOGO` |

| Zeitüberschreitung bei `example\_alpha\_test.exe` | Hydrogen-48 läuft \~35 min | Timeout in `run\_alpha\_tests.ps1` prüfen (Prio 5: 7200 s) |

| EXE findet DLL nicht | DLL nicht in `bin\\` | Kernel + SDK zuerst bauen |



\---



\## 15. Was dieses Makefile nicht tut



\- \*\*Keine Header-Kopien.\*\* Alle Header bleiben am Pflegeort.

\- \*\*Kein Build\_INFO.\*\* Das schreibt der Master-Build in `build\\main\\`.

\- \*\*Kein Signing.\*\* Das macht `build.ps1 -Sign`.

\- \*\*Keine Test-Ausführung.\*\* Der Build erzeugt EXEs, führt sie aber

&#x20; nicht aus. Die Test-Läufe macht `bin\\run\_alpha\_tests.ps1`.

\- \*\*Kein Debug-Build.\*\* Aktuell nur Release (`/O2`).

\- \*\*Keine separaten Targets pro Test.\*\* Alle drei EXEs werden in

&#x20; einem Lauf gebaut.



\---



\## 16. Die drei EXEs im Detail



\### 16.1 `example\_alpha\_test.exe`



Die Haupt-Test-Suite. Ruft je nach CLI-Flag einen einzelnen Test auf:



```cmd

example\_alpha\_test.exe --test-spin-half

example\_alpha\_test.exe --test-dirac

```



Verfügbare Flags:



| Prio | Tests | CLI-Flags |

|---|---|---|

| 1 | 12 | `--test-amp`, `--test-born`, `--test-unitary`, ... |

| 2 | 10 | `--test-superdet`, `--test-observer-chsh`, ... |

| 3 | 10 | `--test-amp-invariant`, `--test-edge-transport`, ... |

| 4 | 3 | `--test-3d-smoke`, `--test-3d-invariance`, `--test-3d-dispersion` |

| 5 | 4 | `--test-hydrogen`, `--test-hydrogen-48`, `--test-shared-reference`, `--test-shared-formula-tournament` |

| 6 | 1 | `--test-spin-half` |

| 7 | 1 | `--test-dirac` |



Ohne Flag läuft der Datenmodus (CSV-Ausgabe), der für die Alpha-Tests

nicht genutzt wird.



\### 16.2 `example\_test\_density.exe`



Dichte-Trilogie-Regression: 8×8-Knoten-Dichte, 64×64-Tensor-Dichte,

256×256-Fock-Dichte mit Lindblad-Kanälen. 86 Checks. Kein CLI-Argument.

Exit-Code 0 bei allen PASS.



\### 16.3 `example\_test\_tensor.exe`



Tensor-/Fock-Regression: Popcount, Vakuum, Basis-Zustände, Erzeuger/

Vernichter, Jordan-Wigner-Vorzeichen, Antikommutatoren, Hopping,

Tensor-Fock-Roundtrip. 61 Checks. Kein CLI-Argument.



\---



\## 17. Siehe auch



\- `docs\\build\\BUILD\_SCRIPT.md` — Master-Build, Export, Signing

\- `docs\\build\\prophysics\\Makefile.md` — Kernel-Build

\- `docs\\build\\sdk\\Makefile.md` — SDK-Interface-Build

\- `docs\\test\\ProPhysics\_Testkatalog.md` — Test-Übersicht

\- `bin\\run\_alpha\_tests.ps1` — Test-Runner



\---



\*\*Ende Makefile-Dokumentation (Test Build).\*\*



