\# ProPhysics SDK Interface — NMAKE Build



\*\*Datei:\*\* `build\\sdk\\Makefile.sdk.nmake`

\*\*Version:\*\* 3.0 (Etappe 21)

\*\*Zweck:\*\* Bau der SDK-Interface-DLL und ihrer Import-Lib aus der

Quelle in `src\\sdk\\`. Linkt gegen `ProPhysics.lib`.



\---



\## 1. Was gebaut wird



Aus \*\*einer\*\* `.c`-Datei entsteht \*\*eine\*\* DLL und \*\*eine\*\* Import-Lib:



| Artefakt | Pfad | Zweck |

|---|---|---|

| `pro\_sdk\_interface.dll` | `bin\\pro\_sdk\_interface.dll` | Laufzeit-Bibliothek |

| `pro\_sdk\_interface.lib` | `lib\\pro\_sdk\_interface.lib` | Import-Lib für nachgelagerte Builds (Tests) |



Zusätzlich linkt der Build gegen `lib\\ProPhysics.lib` — die Import-Lib

des Kernels. Diese muss \*\*vor\*\* diesem Build existieren.



\*\*Kein\*\* Header-Export. Der SDK-Header `pro\_sdk\_interface.h` bleibt am

Pflegeort in `src\\sdk\\header\\`.



\---



\## 2. Ablageort und Aufruf



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── build\\

│   └── sdk\\

│       └── Makefile.sdk.nmake    <- wird HIER ausgeführt

├── src\\

│   ├── sdk\\

│   │   ├── pro\_sdk\_interface.c   <- Quelle

│   │   └── header\\

│   │       └── pro\_sdk\_interface.h

│   └── prophysics\\

│       └── header\\\*.h            <- Kernel-Header (eingebunden)

├── bin\\

│   ├── ProPhysics.dll

│   └── pro\_sdk\_interface.dll     <- Ziel

└── lib\\

&#x20;   ├── ProPhysics.lib            <- Eingang

&#x20;   └── pro\_sdk\_interface.lib     <- Ziel

```



\*\*Aufruf\*\* immer aus dem Ablageort des Makefiles:



```cmd

cd build\\sdk

nmake /NOLOGO /f Makefile.sdk.nmake

```



Alle Pfade sind relativ zu `build\\sdk\\`.



\---



\## 3. Verzeichnis-Layout (relativ zu `build\\sdk\\`)



| Symbol | Wert | Bemerkung |

|---|---|---|

| `SRC\_DIR` | `..\\..\\src\\sdk` | Quelle |

| `HDR\_DIR` | `..\\..\\src\\sdk\\header` | SDK-Header |

| `OBJ\_DIR` | `\_obj` | temporäre Objektdateien |

| `BIN\_DIR` | `..\\..\\bin` | Ziel-DLL |

| `LIB\_DIR` | `..\\..\\lib` | Ziel-LIB |

| `CORE\_LIB` | `..\\..\\lib\\ProPhysics.lib` | Kernel-Import-Lib (Eingang) |



\---



\## 4. Targets



| Target | Wirkung |

|---|---|

| `all` (Default) | `check\_core` + `setup` + DLL bauen |

| `check\_core` | prüft, dass `..\\..\\lib\\ProPhysics.lib` existiert |

| `setup` | `\_obj\\`, `bin\\`, `lib\\` anlegen (idempotent) |

| `clean` | `\_obj\\`, SDK-DLL, SDK-LIB entfernen |



Es gibt keine separaten Targets pro Modul — es ist nur eine `.c`-Datei.



\---



\## 5. Compiler-Flags im Detail



```

CFLAGS = /nologo /O2 /Ob2 /Oi /GL /W3 /MP \\

&#x20;        /I..\\..\\src\\sdk\\header \\

&#x20;        /I..\\..\\src\\prophysics\\header \\

&#x20;        /D\_CRT\_SECURE\_NO\_WARNINGS

```



| Flag | Bedeutung |

|---|---|

| `/nologo` | kein Copyright-Banner |

| `/O2` | maximale Optimierung |

| `/Ob2` | aggressives Inlining |

| `/Oi` | intrinsische Funktionen |

| `/GL` | Whole-Program-Optimization (Link-Time Code Generation) |

| `/W3` | Warnstufe 3 (im Kernel: `/W4`) |

| `/MP` | Multi-Prozessor-Kompilierung (bei 1 Datei wirkungslos) |

| `/I..\\..\\src\\sdk\\header` | SDK-eigener Header (`pro\_sdk\_interface.h`) |

| `/I..\\..\\src\\prophysics\\header` | Kernel-Header (`ProPhysics.h`, `Types`, `Config`) |

| `/D\_CRT\_SECURE\_NO\_WARNINGS` | unterdrückt MSVC-Warnungen über `fopen`, `strcpy` etc. |



Zusätzlich:

```

DLL\_FLAGS = /DPRO\_SDK\_EXPORTS

```



Das ist das SDK-Export-Macro. Es sorgt dafür, dass `PRO\_SDK\_API` in

`pro\_sdk\_interface.h` zu `\_\_declspec(dllexport)` expandiert.



\*\*Hinweis zur Warnstufe:\*\* Der SDK-Build nutzt `/W3` statt `/W4`. Grund:

`pro\_sdk\_interface.c` enthält Test-/Analyse-Hilfen (BMP-Export,

ASCII-Renderer), die bewusst gegen einige `/W4`-Warnungen verstoßen.

Der Kernel bleibt bei `/W4`.



\---



\## 6. Link



```

LINKER = link.exe

```



Der Link-Schritt:



```

link.exe /nologo /DLL /LTCG \\

&#x20;   /OUT:..\\..\\bin\\pro\_sdk\_interface.dll \\

&#x20;   /IMPLIB:..\\..\\lib\\pro\_sdk\_interface.lib \\

&#x20;   \_obj\\pro\_sdk\_interface.obj ..\\..\\lib\\ProPhysics.lib

```



| Flag | Bedeutung |

|---|---|

| `/DLL` | DLL statt EXE |

| `/LTCG` | Link-Time Code Generation (vollendet `/GL`) |

| `/OUT:` | Pfad der DLL |

| `/IMPLIB:` | Pfad der Import-Lib |

| Eingangsdateien | die eigene `.obj` \*\*und\*\* die Kernel-Import-Lib |



MSVC legt zusätzlich eine `.exp`-Datei ab. Sie landet üblicherweise

neben der DLL (in `bin\\`), nicht in `lib\\`. Das Makefile lässt sie

stehen — sie wird nur gebraucht, wenn eine weitere DLL gegen dieselben

Exporte gelinkt wird. Soll sie weg, ergänze im Post-Link-Schritt:



```makefile

@if exist "$(BIN\_DIR)\\pro\_sdk\_interface.exp" del /Q "$(BIN\_DIR)\\pro\_sdk\_interface.exp" 2>nul

```



\---



\## 7. Was nach dem Build wo liegt



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── bin\\

│   ├── ProPhysics.dll              (aus prophysics-Build)

│   ├── pro\_sdk\_interface.dll       <- final, sauber

│   └── pro\_sdk\_interface.exp       (optional, siehe §6)

├── lib\\

│   ├── ProPhysics.lib              (aus prophysics-Build)

│   └── pro\_sdk\_interface.lib       <- final, sauber

└── build\\

&#x20;   └── sdk\\

&#x20;       └── \_obj\\                   <- leer (nur Verzeichnis)

```



`bin\\` ist flach — alle DLLs liegen nebeneinander. Das ist wichtig,

weil der Windows-Loader die Kernel-DLL sucht, wenn eine EXE gegen

die SDK-DLL gelinkt ist und beide im selben Verzeichnis liegen.



\---



\## 8. Clean



```cmd

nmake /NOLOGO /f Makefile.sdk.nmake clean

```



Löscht:

\- `\_obj\\` komplett (rekursiv)

\- `bin\\pro\_sdk\_interface.dll`

\- `lib\\pro\_sdk\_interface.lib`



\*\*Nicht\*\* angetastet werden:

\- `bin\\ProPhysics.dll` (gehört dem Kernel-Build)

\- `bin\\example\_\*.exe` (gehört dem Test-Build)

\- `lib\\ProPhysics.lib`

\- `BUILD\_INFO.txt` im Root



Sauberes Zusammenspiel mit `build\\main\\Makefile.nmake`, das sein

`clean\_sdk`-Target auf dieses `clean` mapped.



\---



\## 9. Abhängigkeits-Graph



```

check\_core       (Prüfung: lib\\ProPhysics.lib existiert)

&#x20;     │

&#x20;     ▼

setup            (mkdir \_obj\\, bin\\, lib\\)

&#x20;     │

&#x20;     ▼

+--------------------------+

| pro\_sdk\_interface.c      |

+--------------------------+

&#x20;     │

&#x20;     ▼

link.exe → bin\\pro\_sdk\_interface.dll

&#x20;          lib\\pro\_sdk\_interface.lib

&#x20;     │

&#x20;     ▼

delete \_obj\\\*.obj, \_obj\\\*.exp

```



Nur ein Modul. Die Übersetzung ist trivial, der eigentliche Schritt ist

der Link gegen die Kernel-Import-Lib.



\---



\## 10. Reihenfolge in der Build-Kette



```

build\\prophysics   →  bin\\ProPhysics.dll + lib\\ProPhysics.lib

&#x20;       │

&#x20;       ▼

build\\sdk          →  bin\\pro\_sdk\_interface.dll       (dieses Makefile)

&#x20;                     lib\\pro\_sdk\_interface.lib

&#x20;       │

&#x20;       ▼

build\\test         →  bin\\example\_\*.exe

```



Der `check\_core`-Schritt bricht mit klarer Meldung ab, wenn

`ProPhysics.lib` fehlt. Damit kann `nmake /f Makefile.sdk.nmake`

nicht versehentlich alleine laufen — es muss immer der Kernel-Build

vorausgegangen sein.



\---



\## 11. Wichtige Voraussetzungen



1\. \*\*`ProPhysics.lib` muss existieren.\*\*

&#x20;  Der `check\_core`-Schritt prüft `..\\..\\lib\\ProPhysics.lib`.

&#x20;  Fehlt sie, bricht der Build ab mit:

&#x20;  ```

&#x20;  \[FEHLER] ..\\..\\lib\\ProPhysics.lib fehlt. Zuerst build\\prophysics bauen.

&#x20;  NMAKE : fatal error U1077: ... Rückgabe-Code "0x1"

&#x20;  ```



2\. \*\*Kernel-Header müssen in `src\\prophysics\\header\\` liegen.\*\*

&#x20;  Die `/I`-Pfade referenzieren dieses Verzeichnis. `pro\_sdk\_interface.c`

&#x20;  inkludiert `ProPhysics.h` und `ProPhysics\_Types.h` von dort.



3\. \*\*SDK-Header muss in `src\\sdk\\header\\` liegen.\*\*

&#x20;  `pro\_sdk\_interface.h` — sonst bricht der Compiler mit

&#x20;  „cannot open source file".



4\. \*\*MSVC-Toolchain im PATH.\*\* Wie beim Kernel-Build.



\---



\## 12. Fehlersuche



| Symptom | Ursache | Fix |

|---|---|---|

| `\[FEHLER] ProPhysics.lib fehlt` | Kernel nicht gebaut | `cd build\\prophysics \&\& nmake` |

| `pro\_sdk\_interface.h: No such file` | SDK-Header fehlt | Header nach `src\\sdk\\header\\` legen |

| `ProPhysics.h: No such file` | Kernel-Header fehlt | Header nach `src\\prophysics\\header\\` legen |

| `unresolved external symbol ProPhysics\_...` | Kernel-Export fehlt | `ProPhysics.h` prüfen, Kernel neu bauen |

| `LNK2019: \_main` | `/DLL` fehlt | Link-Zeile prüfen |

| `fatal error U1077` beim Link | Linker-Ausgabe unterdrückt | `nmake` ohne `/NOLOGO` laufen lassen |

| `.exp`-Datei in `bin\\` stört | MSVC-Detail | optional in Post-Link-Schritt löschen (§6) |



\---



\## 13. Was dieses Makefile nicht tut



\- \*\*Keine Header-Kopien.\*\* Alle Header bleiben am Pflegeort.

\- \*\*Kein Build\_INFO.\*\* Das schreibt der Master-Build in `build\\main\\`.

\- \*\*Kein Signing.\*\* Das macht `build.ps1 -Sign`.

\- \*\*Keine Tests.\*\* Die laufen über `bin\\run\_alpha\_tests.ps1`.

\- \*\*Kein Debug-Build.\*\* Aktuell nur Release (`/O2`).

\- \*\*Kein Kernel-Build.\*\* Der Kernel muss vorher separat gebaut sein.



\---



\## 14. Export-Layout des SDK



Der `build\\main\\export.ps1 -Mode sdk` erzeugt aus den SDK-Artefakten

folgendes Ausgabe-Layout:



```

out\\sdk\\

└── libs\\

&#x20;   ├── ProPhysics.dll

&#x20;   ├── ProPhysics.lib

&#x20;   ├── pro\_sdk\_interface.dll

&#x20;   ├── pro\_sdk\_interface.lib

&#x20;   └── src\\

&#x20;       └── header\\

&#x20;           ├── prophysics\\

&#x20;           │   ├── ProPhysics.h

&#x20;           │   ├── ProPhysics\_Types.h

&#x20;           │   └── ...

&#x20;           └── sdk\\

&#x20;               └── pro\_sdk\_interface.h

```



Das ist die Struktur, die an externe Empfänger weitergereicht wird.

Header landen in Unterordnern, damit `#include`-Pfade konsistent

bleiben (`#include "prophysics/ProPhysics.h"`).



\---



\## 15. Siehe auch



\- `docs\\build\\BUILD\_SCRIPT.md` — Master-Build, Export, Signing

\- `docs\\build\\prophysics\\Makefile.md` — Kernel-Build

\- `docs\\build\\test\\Makefile.md` — Test-Build

\- `src\\sdk\\header\\pro\_sdk\_interface.h` — SDK-API

\- `src\\prophysics\\header\\ProPhysics.h` — Kernel-API (SDK-Nutzer lesen beide)



\---



\*\*Ende Makefile-Dokumentation (SDK Interface).\*\*



