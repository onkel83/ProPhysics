\# ProPhysics Kernel — NMAKE Build



\*\*Datei:\*\* `build\\prophysics\\Makefile.nmake`

\*\*Version:\*\* 3.0 (Etappe 21)

\*\*Zweck:\*\* Bau der ProPhysics Kernel-DLL und ihrer Import-Lib aus den

Kernel-Quellen in `src\\prophysics\\`.



\---



\## 1. Was gebaut wird



Aus 11 `.c`-Modulen entsteht \*\*eine\*\* DLL und \*\*eine\*\* Import-Lib:



| Artefakt | Pfad | Zweck |

|---|---|---|

| `ProPhysics.dll` | `bin\\ProPhysics.dll` | Laufzeit-Bibliothek |

| `ProPhysics.lib` | `lib\\ProPhysics.lib` | Import-Lib für nachgelagerte Builds (SDK, Tests) |



\*\*Kein\*\* Header-Export. Die Header bleiben am Pflegeort in

`src\\prophysics\\header\\` und werden von den anderen Builds über den

`/I`-Pfad eingebunden. Das verhindert doppelte Header-Kopien, die

auseinanderlaufen können.



\---



\## 2. Ablageort und Aufruf



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── build\\

│   └── prophysics\\

│       └── Makefile.nmake     <- wird HIER ausgeführt

├── src\\

│   └── prophysics\\

│       ├── \*.c                <- Quellen

│       └── header\\\*.h         <- Header

├── bin\\                       <- Ziel DLL

└── lib\\                       <- Ziel LIB

```



\*\*Aufruf\*\* immer aus dem Ablageort des Makefiles:



```cmd

cd build\\prophysics

nmake /NOLOGO /f Makefile.nmake

```



Alle Pfade im Makefile sind \*\*relativ zu `build\\prophysics\\`\*\* und

funktionieren unabhängig vom Arbeitsverzeichnis der aufrufenden Shell.

Ein Aufruf aus dem Repo-Root ist mit `-f` möglich, aber nur wenn du

vorher in `build\\prophysics\\` wechselst — das Makefile nutzt keine

`$(MAKEDIR)`-Expansion für die Pfad-Basis.



\---



\## 3. Verzeichnis-Layout (relativ zu `build\\prophysics\\`)



| Symbol | Wert | Bemerkung |

|---|---|---|

| `SRC\_DIR` | `..\\..\\src\\prophysics` | Quellen |

| `HDR\_DIR` | `..\\..\\src\\prophysics\\header` | Header |

| `OBJ\_DIR` | `\_obj` | temporäre Objektdateien |

| `BIN\_DIR` | `..\\..\\bin` | Ziel-DLL |

| `LIB\_DIR` | `..\\..\\lib` | Ziel-LIB |



\---



\## 4. Targets



| Target | Wirkung |

|---|---|

| `all` (Default) | `setup` + DLL bauen |

| `setup` | `\_obj\\`, `bin\\`, `lib\\` anlegen (idempotent) |

| `clean` | `\_obj\\`, DLL, LIB entfernen |



Es gibt keine separaten Targets pro Modul. Alle 11 `.c`-Dateien werden

in einem Lauf kompiliert und gelinkt.



\---



\## 5. Compiler-Flags im Detail



```

CFLAGS = /nologo /O2 /Ob2 /Oi /GL /W4 /MP \\

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

| `/W4` | Warnstufe 4 |

| `/MP` | Multi-Prozessor-Kompilierung (paralleler Build) |

| `/I..\\..\\src\\prophysics\\header` | Kernel-Header (ProPhysics.h, Types, Internal) |

| `/I..\\..\\src\\sdk\\header` | SDK-Header (für `pro\_sdk\_interface.h`, falls referenziert) |

| `/D\_CRT\_SECURE\_NO\_WARNINGS` | unterdrückt MSVC-Warnungen über `fopen`, `strcpy` etc. |



Zusätzlich:

```

DLL\_FLAGS = /DPROPHYSICS\_EXPORTS

```

Das ist das Export-Macro aus `ProPhysics\_Exports.h`. Es sorgt dafür,

dass `PROPHYSICS\_API` zu `\_\_declspec(dllexport)` expandiert.



\---



\## 6. Link



```

LINKER = link.exe

```



Der Link-Schritt:



```

link.exe /nologo /DLL /LTCG \\

&#x20;   /OUT:..\\..\\bin\\ProPhysics.dll \\

&#x20;   /IMPLIB:..\\..\\lib\\ProPhysics.lib \\

&#x20;   \_obj\\\*.obj

```



| Flag | Bedeutung |

|---|---|

| `/DLL` | DLL statt EXE |

| `/LTCG` | Link-Time Code Generation (vollendet `/GL`) |

| `/OUT:` | Pfad der DLL |

| `/IMPLIB:` | Pfad der Import-Lib |



MSVC legt zusätzlich eine `.exp`-Datei ab — üblicherweise neben der DLL.

Das Makefile löscht sie zusammen mit den `.obj`-Dateien im `\_obj\\`-Ordner.



\---



\## 7. Was nach dem Build wo liegt



```

H:\\ProPhysics\_SDK\\ProPhysics\\

├── bin\\

│   └── ProPhysics.dll           <- final, sauber

├── lib\\

│   └── ProPhysics.lib           <- final, sauber

└── build\\

&#x20;   └── prophysics\\

&#x20;       └── \_obj\\                <- leer (nur Verzeichnis)

```



\*\*Kein Zwischenstand liegt außerhalb von `\_obj\\`.\*\* Die frühere

Variante mit einem `..\\bin\\ProPhysics\\`-Unterordner ist entfallen;

`bin\\` ist jetzt flach und enthält alle DLLs und EXEs nebeneinander.



\---



\## 8. Clean



```cmd

nmake /NOLOGO /f Makefile.nmake clean

```



Löscht:

\- `\_obj\\` komplett (rekursiv)

\- `bin\\ProPhysics.dll`

\- `lib\\ProPhysics.lib`



\*\*Nicht\*\* angetastet werden:

\- `bin\\pro\_sdk\_interface.dll` (gehört dem SDK-Build)

\- `bin\\example\_\*.exe` (gehört dem Test-Build)

\- `lib\\pro\_sdk\_interface.lib`

\- `BUILD\_INFO.txt` im Root



Sauberes Zusammenspiel mit `build\\main\\Makefile.nmake`, das sein

`clean\_prophysics`-Target auf dieses `clean` mapped.



\---



\## 9. Abhängigkeits-Graph



```

setup

&#x20; │  mkdir \_obj\\, bin\\, lib\\

&#x20; ▼

+------+------+------+------+------+------+------+------+------+------+------+

| Core | Amp  | Dirac| Gauge| EPR  | Obs  | Tens | Fock | Dens | Shared      |

+------+------+------+------+------+------+------+------+------+------+------+

&#x20;       │

&#x20;       ▼

&#x20;  link.exe → bin\\ProPhysics.dll + lib\\ProPhysics.lib

&#x20;       │

&#x20;       ▼

&#x20;  delete \_obj\\\*.obj, \_obj\\\*.exp

```



Jede Modulregel hängt von \*\*allen\*\* Headern ab. Ein Header-Update

rebuildet damit alles. Das ist bewusst grob — bei einem Projekt dieser

Größe spart das die `/showIncludes`-Verrenkung ohne nennenswerten

Nachteil (Build < 5 s).



\---



\## 10. Reihenfolge in der Build-Kette



```

build\\prophysics   (dieses Makefile)   →  bin\\ProPhysics.dll + lib\\ProPhysics.lib

&#x20;       │

&#x20;       ▼

build\\sdk                              →  bin\\pro\_sdk\_interface.dll

&#x20;                                         lib\\pro\_sdk\_interface.lib

&#x20;       │

&#x20;       ▼

build\\test                             →  bin\\example\_\*.exe

```



Der Kernel ist die unterste Ebene. Seine Artefakte werden vom SDK-Build

(Link gegen `ProPhysics.lib`) und vom Test-Build (Link gegen beide

Libs) benötigt.



\---



\## 11. Wichtige Voraussetzungen



1\. \*\*`ProPhysics\_Internal.h` muss in `src\\prophysics\\header\\` liegen.\*\*

&#x20;  Sie wird von jeder `.c`-Datei inkludiert und ist im `HEADERS\_ALL`

&#x20;  aufgeführt. Fehlt sie, bricht der Build mit klarer Meldung ab.



2\. \*\*Alle 11 `.c`-Dateien müssen vorhanden sein.\*\*

&#x20;  Das Makefile listet sie explizit:

&#x20;  ```

&#x20;  ProPhysics\_Core.c

&#x20;  ProPhysics\_Amp.c

&#x20;  ProPhysics\_Dirac.c

&#x20;  ProPhysics\_Gauge.c

&#x20;  ProPhysics\_EPR.c

&#x20;  ProPhysics\_Observer.c

&#x20;  ProPhysics\_Tensor.c

&#x20;  ProPhysics\_Fock.c

&#x20;  ProPhysics\_Density.c

&#x20;  ProPhysics\_Shared.c

&#x20;  ```

&#x20;  Fehlt eine, meldet NMAKE sie als unbekannte Regel.



3\. \*\*MSVC-Toolchain im PATH.\*\* `nmake.exe` und `cl.exe` müssen

&#x20;  erreichbar sein. Am einfachsten über den \*VS Developer Command

&#x20;  Prompt\* (oder `vcvars64.bat`).



4\. \*\*Zweierpotenz-Regel gilt nicht für dieses Makefile.\*\* Die Regel

&#x20;  betrifft `grid\_dim` zur Laufzeit, nicht den Build. Hier ist sie

&#x20;  irrelevant.



\---



\## 12. Fehlersuche



| Symptom | Ursache | Fix |

|---|---|---|

| `ProPhysics\_Core.c: No such file` | falscher CWD | `cd build\\prophysics` vor dem Aufruf |

| `ProPhysics\_Internal.h: No such file` | Header liegt woanders | Header nach `src\\prophysics\\header\\` verschieben |

| `unresolved external symbol` beim Link | fehlende `.c`-Datei im `OBJ\_KERNEL` | `.c`-Datei in `OBJ\_KERNEL`-Liste ergänzen |

| `LNK2019: \_main` | `/DLL` fehlt | Link-Zeile prüfen |

| Warnung `C4189 phase\_q16\_signed` | tote Variable in `ProPhysics\_Gauge.c` | kosmetisch, kann entfernt werden |

| `fatal error U1077` beim Link | Linker-Ausgabe unterdrückt | `nmake` ohne `/NOLOGO` laufen lassen |



\---



\## 13. Was dieses Makefile nicht tut



\- \*\*Keine Header-Kopien.\*\* Header bleiben in `src\\prophysics\\header\\`.

\- \*\*Kein Build\_INFO.\*\* Das schreibt der Master-Build in `build\\main\\`.

\- \*\*Kein Signing.\*\* Das macht `build.ps1 -Sign`.

\- \*\*Keine Unit-Tests.\*\* Die laufen über `bin\\run\_alpha\_tests.ps1`.

\- \*\*Kein Debug-Build.\*\* Aktuell nur Release (`/O2`).

\- \*\*Kein inkrementeller Header-Check.\*\* Änderung an einem Header

&#x20; rebuildet alle 11 Module.



\---



\## 14. Siehe auch



\- `docs\\build\\BUILD\_SCRIPT.md` — Master-Build, Export, Signing

\- `docs\\build\\sdk\\Makefile.md` — SDK-Interface-Build

\- `docs\\build\\test\\Makefile.md` — Test-Build

\- `src\\prophysics\\header\\ProPhysics.h` — öffentliche API

\- `src\\prophysics\\header\\ProPhysics\_Internal.h` — interne Modul-Schnittstellen



\---



\*\*Ende Makefile-Dokumentation (ProPhysics Kernel).\*\*

