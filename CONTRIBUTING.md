# ProPhysics — Beitragen

**Datei:** `CONTRIBUTING.md`
**Version:** 1.1
**Stand:** 2026-09-28 (Kernel-Version 1.23.0, Etappe 23, ProWB integriert)
**Repository:** https://github.com/onkel83/prophysics
**Zweck:** Verbindliche Anleitung für Beiträge zum Projekt. Wer zum
ersten Mal etwas beiträgt, liest dieses Dokument **vor** dem ersten
Commit.

---

## §0 — Bevor du anfängst

### §0.1 — Was dieses Projekt ist

ProPhysics ist ein **Forschungs-Prototyp**. Es ist kein Produkt, keine
Bibliothek, kein Framework. Es ist ein Experimentierfeld für die Frage,
ob Quantenmechanik aus einfachen diskreten Regeln emergieren kann.

**Konsequenz für Beiträge:** Wir nehmen keine Beiträge an, die:

- **das Projekt als Produkt positionieren** („macht es schneller",
  „fügt Feature X hinzu", ohne dass X zur Forschungsfrage passt)
- **neue Physik** behaupten, ohne sie zu testen
- **bestehende Tests brechen**, ohne dass der Bruch beabsichtigt und
  dokumentiert ist
- **den Code-Stil des Projekts** ändern
- **externe Abhängigkeiten** hinzufügen (siehe §5)

Wir nehmen Beiträge an, die:

- **einen neuen Test** hinzufügen, der eine neue emergente Struktur
  zeigt
- **einen Bug** beheben, mit reproduzierbarem Testfall
- **die Dokumentation** verbessern
- **die Build-Infrastruktur** robuster machen
- **einen neuen Kernel-Pfad** hinzufügen, der R1–R7 respektiert
- **die Web-Docs** verbessern (ProWB, CSS, Views)

### §0.2 — Was du **vor** dem ersten Commit lesen musst

| Dokument | Warum |
|---|---|
| `docs/project/Project.md` §2 | Projekt-Regeln (R1–R7) |
| `docs/project/Project.md` §3 | Die 5 Ur-Regeln (U1–U5) |
| `docs/project/ARCHITECTURE.md` | Wie der Kernel aufgebaut ist |
| `docs/build/pro_run.md` | Zentraler Einstiegspunkt für Build, Test, Export, Web |
| `docs/test/ProPhysics_Testkatalog.md` | Test-Philosophie |
| `docs/build/BUILD_SCRIPT.md` | Build-Konventionen |

Ohne diese sechs Dokumente fehlt dir das Verständnis, um sinnvolle
Beiträge zu machen.

**Optional, wenn du an den Web-Docs arbeitest:**

| Dokument | Warum |
|---|---|
| `src/prowb/README.md` | Builder-Referenz |
| `docs/web/README.md` | Manifest-Pflege |
| `docs/build/web-docs-ci.md` | CI-Workflow |

### §0.3 — Kontakt

| Zweck | Kanal |
|---|---|
| Bug-Reports | GitHub Issues: https://github.com/onkel83/prophysics/issues |
| Diskussion | GitHub Discussions: https://github.com/onkel83/prophysics/discussions |
| Sicherheitslücken | Privat per E-Mail (siehe `LICENSE.md`) |
| Kommerzielle Lizenzierung | siehe `COMMERCIAL.md` |
| Allgemeine Fragen | koehne83 at googlemail.com |

---

## §1 — Entwicklungsumgebung

### §1.1 — Was du brauchst

**Zwingend:**

| Tool | Version | Zweck |
|---|---|---|
| **Visual Studio** | 2019 oder 2022 | Compiler, Linker, nmake |
| **Windows SDK** | 10 oder 11 | Für `cl.exe` |
| **PowerShell** | 5.0+ | Build-Wrapper, `pro_run` |
| **cmd.exe** | Windows 10+ | Wrapper-Skripte |

**Optional:**

| Tool | Version | Zweck |
|---|---|---|
| **git** | 2.20+ | Versionskontrolle, `-GitStamp` |
| **Python** | 3.8+ | `analysis.py` (CSV-Auswertung) |
| **signtool** | (Windows SDK) | DLL-Signierung |

### §1.2 — Setup

**Schritt 1 — Repository klonen:**

```cmd
git clone https://github.com/onkel83/prophysics.git
cd prophysics
```

**Schritt 2 — Visual Studio Developer Prompt öffnen:**

```cmd
:: Start → "x64 Native Tools Command Prompt for VS 2022"
:: oder:
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

**Schritt 3 — Umgebung prüfen:**

```cmd
where nmake
where cl
where link
```

Alle drei müssen gefunden werden. Wenn nicht, ist der Developer-Prompt
nicht aktiv.

**Schritt 4 — Build testen:**

```cmd
cd tools
pro_run build
```

Ergebnis: `bin\` enthält 2 DLLs + 3 EXEs, `lib\` enthält 2 LIBs,
`bin\prowb\` enthält den ProWB-Builder, `BUILD_INFO.txt` im Root.

Alternativ direkt über den Wrapper (Legacy-Aufruf):

```cmd
cd build\main
build.cmd
```

**Schritt 5 — Tests testen:**

```cmd
cd tools
pro_run test -Prio 1
```

Ergebnis: 12/12 PASS in ~5 s.

**Schritt 6 (optional) — Web-Docs testen:**

```cmd
cd tools
pro_run web
```

Ergebnis: `out\web\index.html` wird erzeugt (~1,2 MB bei 44 Docs).

**Empfehlung:** Nutze `pro_run` als Einstiegspunkt. Die direkten
Wrapper (`build.cmd`, `export.cmd`, `run_alpha_tests.cmd`) bleiben
funktional, sind aber für den täglichen Gebrauch nicht mehr der
empfohlene Weg.

### §1.3 — Optionale Python-Umgebung

Für `python\analysis.py`:

```cmd
pip install pandas numpy matplotlib seaborn scipy scikit-learn
```

Nicht nötig, um Kernel-Code zu bearbeiten.

---

## §2 — Build

### §2.1 — Der empfohlene Weg: `pro_run`

`pro_run` ist der **zentrale Einstiegspunkt**. Er dispatcht auf die
Subskripte und nimmt konsistente Parameter entgegen.

```cmd
cd tools
pro_run build
```

| Aktion | Ruft auf |
|---|---|
| `pro_run build` | `build\main\build.ps1` |
| `pro_run export` | `build\main\export.ps1` |
| `pro_run test` | `tools\run_alpha_tests.ps1` |
| `pro_run web` | `build\prowb\prowb.exe` (via nmake) |
| `pro_run all` | build → test → export |
| `pro_run help` | Übersicht |

### §2.2 — Die wichtigsten Aufrufe

```cmd
cd tools

:: Alles bauen (Kernel + SDK + Tests + ProWB)
pro_run build

:: Nur Kernel
pro_run build -Mode kernel

:: Kompletter Rebuild
pro_run build -Mode all -Rebuild

:: Debug-Konfiguration
pro_run build -Config debug -Rebuild

:: SDK exportieren
pro_run export -Export sdk

:: Web-Docs bauen
pro_run web

:: Web-Docs komplett neu
pro_run web -Rebuild
```

**Legacy-Aufrufe** (bleiben funktional, aber nicht mehr empfohlen):

```cmd
cd build\main
build.cmd
build.cmd -Mode prophysics
build.cmd -Mode all -Rebuild
build.cmd -Mode all -Rebuild -GitStamp
build.cmd -Clean
```

### §2.3 — Sub-Makefiles (nur bei Debug)

Wenn du **direkt** an einem Sub-Makefile arbeitest:

```cmd
cd build\prophysics
nmake /NOLOGO /f Makefile.nmake

cd ..\sdk
nmake /NOLOGO /f Makefile.sdk.nmake

cd ..\test
nmake /NOLOGO /f Makefile.nmake

cd ..\prowb
nmake /NOLOGO /f Makefile.nmake
```

**Wichtig:** Sub-Makefiles haben `check_*`-Vorprüfungen. Wenn
`ProPhysics.lib` fehlt, bricht der SDK-Build mit klarer Meldung ab.

### §2.4 — Build-Zeiten

| Was | Zeit |
|---|---|
| Inkrementeller Build (nichts geändert) | < 1 s |
| Kernel allein | ~2 s |
| Kernel + SDK + Tests | ~5 s |
| ProWB allein | ~1 s |
| Kompletter Rebuild (inkl. ProWB) | ~6 s |

Der Build ist **schnell**. Wenn er länger dauert, stimmt etwas nicht.

---

## §3 — Tests

### §3.1 — Die Test-Suite

Alle Tests laufen über `pro_run test` (dispatcht auf
`tools\run_alpha_tests.ps1`):

```cmd
cd tools
pro_run test -Prio 1        :: 2D-Basis, ~5 s
pro_run test -Prio all      :: alles, ~74 min
pro_run test -Test Dirac    :: einzelner Test
```

**Prio-Übersicht:**

| Prio | Thema | Tests | Dauer |
|:-:|---|:-:|---:|
| 1 | 2D-Basis | 12 | ~5 s |
| 2 | Emergenz | 10 | ~6,5 min |
| 3 | Langlauf | 10 | ~1 min |
| 4 | 3D-Torus | 3 | ~1 min |
| 5 | Hydrogen + Shared | 4 | ~41 min |
| 6 | Spin-1/2 | 1 | < 1 s |
| 7 | Dirac | 1 | ~15 s |
| 8 | SU(2) + Running-Coupling | 2 | ~24 min |
| all | alle | 43 | ~74 min |

**Empfehlung für die Entwicklung:**

```cmd
:: Schneller Smoke-Test nach jeder Änderung
pro_run test -Prio 1

:: Vor dem Commit: alle schnellen Tests
pro_run test -Prio 1
pro_run test -Prio 6
pro_run test -Prio 7
```

**Legacy-Aufruf** (bleibt funktional):

```cmd
cd tools
run_alpha_tests.cmd -Prio 1
```

### §3.2 — Logs

Jeder Test schreibt ein Log nach `bin\logs\<timestamp>_<name>.log`.

**Log-Format:**

```
<vollständige stdout des Test-Prozesses>
--- STDERR ---    (nur wenn stderr nicht leer)
<stderr-Inhalt>
--- TIMEOUT nach Ns ---    (nur bei Timeout)
```

### §3.3 — Was ein Test prüfen muss

Ein neuer Test muss:

1. **Ein spezifisches Kriterium** haben (Zahl, Schwelle).
2. **Reproduzierbar** sein (deterministischer Seed).
3. **In `alpha_test_main.c` registriert** sein.
4. **In `tools/run_alpha_tests.ps1` eingetragen** sein.
5. **Im Testkatalog** dokumentiert sein.

**Nicht akzeptabel:**

- Tests, die immer PASS liefern (tautologisch)
- Tests, die von externen Ressourcen abhängen
- Tests ohne dokumentiertes Kriterium

### §3.4 — Einzelnen Test manuell ausführen

```cmd
cd bin
example_alpha_test.exe --test-dirac
example_alpha_test.exe --test-su2-wilson-loop
```

Die Ausgabe ist direkt sichtbar. Der Runner leitet stdout in eine Datei
um — dasselbe Ergebnis.

Alternativ über `pro_run`:

```cmd
cd tools
pro_run test -Test Dirac -LogDir C:\logs
```

---

## §4 — Einen neuen Test hinzufügen

Diese Anleitung zeigt, wie man einen Test nach dem **Etappe-Muster**
hinzufügt. Wir nehmen an, du willst einen Test
`test_my_new_physics` einführen.

### §4.1 — Übersicht

Ein neuer Test berührt **sechs** Stellen:

1. Neue Test-Datei in `src/test/`
2. Eintrag in `src/test/alpha_test_main.c`
3. Eintrag in `build/test/Makefile.nmake`
4. Eintrag in `tools/run_alpha_tests.ps1`
5. Doku in `docs/test/ProPhysics_Testkatalog.md`
6. Doku in `docs/test/run_alpha_tests.md`

Nach dem Eintrag in `tools/run_alpha_tests.ps1` ist der Test über
`pro_run test -Test <name>` erreichbar — **keine** zusätzliche
Änderung an `pro_run.ps1` nötig.

### §4.2 — Schritt 1: Neue Test-Datei

Erzeuge `src/test/alpha_test_my_new_physics.c`:

```c
/* ==========================================================================
 * ProPhysics - Alpha-Test: My New Physics (Etappe NN)
 * File: alpha_test_my_new_physics.c
 * ========================================================================== */

#include "alpha_test_common.h"

int alpha_test_my_new_physics_run(void)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* Setup */
    pu.grid_dim = 8;
    pu.grid_ndim = 2;
    /* ... Topologie aufbauen ... */

    /* Dynamik */
    for (int t = 0; t < 100; ++t) {
        ProPhysics_Tick(&pu, NULL);
    }

    /* Messung */
    double result = /* ... */;
    double expected = /* ... */;
    double rel_dev = fabs(result - expected) / fabs(expected);

    printf("[MNP] Ergebnis: %.6e\n", result);
    printf("[MNP] Erwartet: %.6e\n", expected);
    printf("[MNP] rel_dev = %.3e (Schwelle 1e-3)\n", rel_dev);

    ProPhysics_Free(&pu);

    if (rel_dev < 1e-3) {
        printf("[MNP] -> PASSED (My New Physics)\n");
        return 1;
    }
    printf("[MNP] -> FAILED\n");
    return 0;
}
```

**Konventionen:**

- Dateiname: `alpha_test_<thema>.c`
- Funktionsname: `alpha_test_<thema>_run(void)`
- Ausgabe-Prefix: `[XXX]` (kurz, groß)
- Exit-Status: `1` = PASS, `0` = FAIL
- Schlusszeile: `-> PASSED (...)` oder `-> FAILED`

### §4.3 — Schritt 2: In `alpha_test_main.c` registrieren

Öffne `src/test/alpha_test_main.c` und füge einen neuen CLI-Flag
hinzu. Das Muster:

```c
else if (strcmp(argv[1], "--test-my-new-physics") == 0) {
    return alpha_test_my_new_physics_run() ? 0 : 1;
}
```

Füge die Deklaration am Dateianfang hinzu:

```c
extern int alpha_test_my_new_physics_run(void);
```

### §4.4 — Schritt 3: In `build/test/Makefile.nmake`

Öffne `build/test/Makefile.nmake` und füge die neue Datei in die
`ALPHA_SOURCES`-Liste ein:

```
ALPHA_SOURCES = \
    $(SRC_DIR)\alpha_test_main.c \
    $(SRC_DIR)\alpha_test_common.c \
    $(SRC_DIR)\alpha_test_basic.c \
    ...
    $(SRC_DIR)\alpha_test_my_new_physics.c
```

### §4.5 — Schritt 4: In `tools/run_alpha_tests.ps1`

Öffne `tools/run_alpha_tests.ps1` und füge den Test in den
`$TestCatalog`-Hashtable ein.

**Für einen neuen Prio-9-Test:**

```powershell
# In der Prio-9-Sektion:
'9' = @(
    @{ Name = 'My-New-Physics'
       Exe = 'example_alpha_test.exe'
       Args = '--test-my-new-physics'
       Timeout = 300 }
)
```

**Für einen bestehenden Prio:**

```powershell
# In der Prio-1-Sektion ergänzen:
@{ Name = 'My-New-Physics'
   Exe = 'example_alpha_test.exe'
   Args = '--test-my-new-physics'
   Timeout = 60 }
```

**Timeout wählen:**

| Test-Dauer | Timeout |
|---|---|
| < 1 s | 60 s |
| 1–10 s | 120 s |
| 10–60 s | 300 s |
| 1–10 min | 900 s |
| 10–60 min | 3600 s |

Der Timeout ist **großzügig** zu wählen (3–4× erwartete Dauer).

Nach dem Eintrag ist der Test über `pro_run test -Test My-New-Physics`
erreichbar.

### §4.6 — Schritt 5: In `ProPhysics_Testkatalog.md`

Füge einen neuen Abschnitt hinzu:

```markdown
### T9.1 — My New Physics

**Ziel:** Was wird geprüft.

**Methode:** Wo und was wird gemessen.

**Ergebnis:** Rohwerte des letzten vollständigen Laufs.

**Beweis:** Was damit gezeigt ist.

**Emergenz:** Ja / Nein.
```

Und in §0 (Regression-Status):

| Prio | Thema | Tests | Status |
|---|---|---|---|
| 9 | My New Physics | 1 | **1/1** |

### §4.7 — Schritt 6: In `run_alpha_tests.md`

Füge einen Eintrag in §5 (Prios im Überblick) und §6 (Test-Katalog)
hinzu. Aktualisiere die Gesamt-Test-Anzahl.

### §4.8 — Checkliste

- [ ] Neue `.c`-Datei in `src/test/` erstellt
- [ ] In `alpha_test_main.c` registriert (CLI-Flag + extern)
- [ ] In `build/test/Makefile.nmake` `ALPHA_SOURCES` eingetragen
- [ ] In `tools/run_alpha_tests.ps1` `$TestCatalog` eingetragen
- [ ] In `ProPhysics_Testkatalog.md` dokumentiert
- [ ] In `run_alpha_tests.md` dokumentiert
- [ ] Build läuft (`pro_run build`)
- [ ] Test läuft (`pro_run test -Prio N`)
- [ ] Alle bestehenden Tests laufen weiter

---

## §5 — Code-Stil

### §5.1 — Sprache

**C99.** Kein C11 (außer `aligned_alloc` — siehe unten), kein C++.
Keine Compiler-Erweiterungen außer `__forceinline` (MSVC) bzw.
`__attribute__((always_inline))` (GCC/Clang).

### §5.2 — Formatierung

| Element | Regel |
|---|---|
| Einrückung | 4 Leerzeichen (keine Tabs) |
| Zeilenlänge | max. 100 Zeichen |
| Klammern | Allman für Funktionen, K&R für Blöcke |
| Leerzeichen nach `if`, `for`, `while` | ja |
| `{` auf eigener Zeile | für Funktionen |

**Beispiel:**

```c
/* Funktion: Allman */
static void my_function(int x)
{
    /* Block: K&R */
    if (x > 0) {
        do_something();
    }
    else {
        do_something_else();
    }

    for (int i = 0; i < x; ++i) {
        /* ... */
    }
}
```

### §5.3 — Namenskonventionen

| Element | Konvention | Beispiel |
|---|---|---|
| Öffentliche Funktion | `ProPhysics_<Modul>_<Verb>` | `ProPhysics_Tensor_Create_Pair` |
| Öffentliche Konstante | `PRO_<NAME>` | `PRO_AMP_BASIS_SIZE` |
| Öffentlicher Typ | `Pro<Name>` | `ProUniverse` |
| Interne Funktion | `pro_<name>` | `pro_amp_abs2` |
| Interne Konstante | `PRO_<NAME>` | `PRO_U5_W` |
| Lokale Variable | `snake_case` | `node_count` |
| Statische Variable | `g_<name>` | `g_coulomb_phase_cache` |
| Macro | `PRO_<NAME>(args)` | `PRO_DIRAC_TO_BASIS` |

### §5.4 — Kommentare

**Datei-Header:** Jede `.c`-Datei beginnt mit einem Block-Kommentar:

```c
/* ==========================================================================
 * ProPhysics - <Modul>
 * File: <Dateiname>
 * Architecture: <kurze Beschreibung>
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * <Änderungen dieser Etappe>
 *
 * Verantwortlich fuer:
 *   - <Funktion 1>
 *   - <Funktion 2>
 * ========================================================================== */
```

**Funktions-Header:** Nicht jede Funktion braucht einen. Bei
komplexen Funktionen:

```c
/* ==========================================================================
 * <Funktionsname>
 *
 * <Zweck>
 *
 * <Physik, falls relevant>
 *
 * <Parameter>
 * ========================================================================== */
```

**Inline-Kommentare:** Nur wo nötig. Kein „erklärt das Offensichtliche".

### §5.5 — Was nicht erlaubt ist

- **`goto`** — nirgends.
- **`alloca`** — nirgends.
- **VLA** (Variable Length Arrays) — nirgends.
- **Compiler-spezifische Erweiterungen** außer den definierten Macros.
- **`printf` im Hotpath** — nur im Coldpath (Setup, Diagnose).
- **`malloc` im Hotpath** — siehe R2.
- **`div`/`mod` im Hotpath** — siehe R1.
- **Globale veränderliche Variablen** in `.c`-Dateien, außer sie sind
  bewusst als Cache/Singleton dokumentiert.

**Ausnahme für ProWB:** ProWB ist **nicht** im Kernel-Hotpath. Der
Builder darf `malloc` und `div`/`mod` verwenden. R1/R2 gelten für
den Kernel, nicht für Build-Tools.

### §5.6 — Header-Includes

Reihenfolge in einer `.c`-Datei:

```c
/* 1. Eigener Header (nur bei Modul-Modulen) */
#include "my_module.h"

/* 2. Projekt-Header */
#include "ProPhysics_Internal.h"

/* 3. Standard-Header */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
```

**`ProPhysics_Internal.h`** inkludiert bereits alle Standard-Header.
Für Kernel-Module reicht der eine Include.

---

## §6 — Was du **nicht** tun solltest

### §6.1 — Keine externen Abhängigkeiten

Der Kernel hat **keine** externen Abhängigkeiten außer der C-Standard-
Bibliothek (`msvcrt` auf Windows). Das ist Absicht.

**Verboten:**

- `pthreads`, `OpenMP`, `MPI`
- `BLAS`, `LAPACK`, `FFTW`
- `zlib`, `libpng`, `libjpeg`
- `GLib`, `Boost`, `Eigen`
- Jede andere Nicht-Standard-Bibliothek

**Ausnahme:** `aligned_alloc` (C11) — wird über `pro_aligned_calloc`
gekapselt und auf MSVC durch `_aligned_malloc` ersetzt.

**Ausnahme ProWB:** Der Web-Docs-Builder nutzt ausschließlich die
C-Standard-Bibliothek. Keine externen Parser-Bibliotheken (kein
`cmark`), keine Template-Engines.

### §6.2 — Keine neuen Floating-Point-Operationen im Hotpath

Der Hotpath verwendet **Integer-Arithmetik** (Q31). Neue Funktionen
sollen dem folgen.

**Ausnahmen:**

- `Wave_Step` verwendet `cos`/`sin` — bewusst, da die Phase
  kontinuierlich ist.
- SU(2)-`exp_apply` verwendet `cos`/`sin` — bewusst, da die
  Exponential-Map nicht exakt in Integer darstellbar ist.

Neue Funktionen sollten **Integer-Arithmetik** bevorzugen.

### §6.3 — Keine API-Änderungen ohne Test-Anpassung

Wenn du eine Signatur änderst:

1. **Alle Aufrufer** anpassen.
2. **Alle Tests**, die die Funktion verwenden, anpassen.
3. **Doku** aktualisieren.
4. **Im Commit-Message** erwähnen.

R5 verbietet stille API-Brüche. Wenn eine API sich ändert, muss der
Test mit.

### §6.4 — Keine neuen Flags ohne Doku

Jedes neue Flag in `ProUniverse` muss:

1. In `ProPhysics_Config.h` (falls Konstante) definiert sein.
2. In `ProPhysics_Types.h` als Feld vorhanden sein.
3. In `ProPhysics_Initialize` initialisiert werden.
4. In `ARCHITECTURE.md` §4 dokumentiert sein.
5. In der API-Referenz beschrieben sein.

### §6.5 — Keine neuen Tests ohne Doku

Siehe §4.8 — jeder neue Test braucht Einträge in sechs Dateien.

### §6.6 — Keine neuen Build-Targets ohne `pro_run`-Integration

Wenn du ein neues Build-Target hinzufügst (wie ProWB in `1.23.11`):

1. **Sub-Makefile** anlegen (`build\<target>\Makefile.nmake`).
2. **Master-Makefile** um ein Target erweitern.
3. **`pro_run`** um eine Aktion erweitern (Dispatch).
4. **Doku** in `docs\build\<target>\Makefile.md`.
5. **`docs\build\pro_run.md`** aktualisieren.
6. **CI-Workflow** (falls relevant).

Der `pro_run`-Dispatch ist Pflicht, damit Nutzer nur **einen**
Einstiegspunkt lernen müssen.

---

## §7 — Git-Workflow

### §7.1 — Branches

| Branch | Zweck |
|---|---|
| `main` | Stabiler Stand, jeder Commit baut + testet |
| `etappe-NN` | Entwicklung einer neuen Etappe |
| `fix-<thema>` | Bug-Fix |
| `docs-<thema>` | Nur Dokumentation |

**Für Fork-Workflow:** Forke das Repo, arbeite auf einem Feature-Branch,
stelle einen Pull Request.

### §7.2 — Commit-Messages

**Format:**

```
<Etappe>: <kurze Beschreibung>

<optional: längere Erklärung>

<optional: Tests>
```

**Beispiele:**

```
Etappe 24: Euklidisches Pfadintegral

Fügt test_path_integral_equivalence hinzu. Prio-9-Test, ~30 s.
Vergleicht Operator-Formalismus mit Pfadintegral bei dim=16.

Tests: Prio 1 (12/12), Prio 9 (1/1)
```

```
Fix: Backward-Staple in su2_force_on_link

Die fehlende †-Dagger auf U_μ(x-ν) führte zu T16 = 8.06e-03.
Nach dem Fix: T16 = 2.44e-03.

Tests: Prio 8 (2/2)
```

```
Docs: API-Referenz und Architektur-Dokument

- docs/project/ProPhysics_API.md (neu)
- docs/project/SDK_API.md (neu)
- docs/project/ARCHITECTURE.md (neu)
```

```
1.23.11: ProWB / Web-Docs Integration

- src/prowb/ (Builder)
- build/prowb/ (Makefile)
- docs/web/ (Manifest + Templates)
- .github/workflows/web-docs.yml (CI)
- pro_run web (neue Aktion)

Tests: 43/43 PASS (unverändert)
```

**Konventionen:**

- Erste Zeile: max. 72 Zeichen.
- Erste Zeile: Imperativ oder Nominal (nicht „Ich habe...").
- Leerzeile nach der ersten Zeile.
- Tests am Ende erwähnen.

### §7.3 — Was **nicht** in einen Commit gehört

- Build-Artefakte (`bin\`, `lib\`, `_obj\`)
- `BUILD_INFO.txt` (wird generiert)
- `out\` (Export-Ziel)
- `bin\prowb\` (Build-Tool-Artefakt)
- IDE-Dateien (`.vs\`, `.vscode\`, `.idea\`)
- Logs (`bin\logs\`)
- Persönliche Notizen

Die `.gitignore` deckt das ab. Wenn du etwas findest, das fehlt,
ergänze die `.gitignore`.

### §7.4 — Vor jedem Commit

Führe die **schnelle Regression** aus:

```cmd
cd tools

:: 1. Rebuild (Kernel + SDK + Tests + ProWB)
pro_run build -Mode all -Rebuild

:: 2. Schnelle Prios
pro_run test -Prio 1
pro_run test -Prio 6
pro_run test -Prio 7
```

Alle drei Prios müssen PASS liefern. Wenn nicht: **nicht committen**.

**Für eine neue Etappe:** Auch `pro_run test -Prio 8 -Test SU2-Wilson-Loop`
laufen lassen (~2 s statt 24 min).

**Für eine neue Doku-Etappe (wie `1.23.11`):** Zusätzlich

```cmd
pro_run web
```

und im Browser prüfen, dass `out\web\index.html` korrekt aussieht.

**Vor einem Release / Push auf `main`:**

```cmd
pro_run test -Prio all
```

Der komplette Lauf (~74 min) ist Pflicht. R6 verlangt, dass jede
Etappe mit einem grünen Regressionstest endet. **Kein Push ohne
43/43 PASS.**

### §7.5 — Pull Request

Ein PR ist **klein** zu halten. Ein PR = eine Änderung. Wenn du zehn
Sachen ändern willst, mache zehn PRs.

**PR-Beschreibung:**

```markdown
## Was

<Kurze Beschreibung der Änderung>

## Warum

<Motivation, ggf. Link auf Issue>

## Wie getestet

- [ ] `pro_run build -Mode all -Rebuild` läuft fehlerfrei
- [ ] Prio 1: 12/12 PASS
- [ ] Prio 6: 1/1 PASS
- [ ] Prio 7: 1/1 PASS
- [ ] (falls zutreffend) Neuer Test: <Name>
- [ ] (falls zutreffend) `pro_run web` läuft fehlerfrei

## Was sich ändert

- Datei A: <Änderung>
- Datei B: <Änderung>

## R1–R7

- R1: <erfüllt / nicht betroffen>
- R2: <erfüllt / nicht betroffen>
- ...
```

### §7.6 — Review-Prozess

Alle PRs werden **einzeln** reviewed. Erwartung:

- **R1–R7** werden geprüft.
- **Tests** müssen laufen.
- **Doku** muss mit-aktualisiert sein.
- **Stil** muss passen.
- **`pro_run`-Integration** (falls relevant) muss passen.

**Ablehnungsgründe:**

- „Das Projekt ist anders" ist keine Begründung. Konkrete Kriterien.
- „Ich habe keine Zeit für Tests" → dann ist der PR nicht fertig.
- „Andere Frameworks machen das auch" → wir sind nicht andere Frameworks.

---

## §8 — Spezielle Beiträge

### §8.1 — Einen Bug melden

**Guter Bug-Report:**

```markdown
## Was passiert

`pro_run test -Prio 8` liefert `SU2-Wilson-Loop FAIL`.

## Was erwartet wird

`SU2-Wilson-Loop PASS` mit 18/18.

## Reproduktion

1. `git clone https://github.com/onkel83/prophysics.git`
2. `cd prophysics\build\main`
3. `build.cmd -Mode all -Rebuild`
4. `cd ..\..\tools`
5. `pro_run test -Prio 8`

## Umgebung

- Windows 11 23H2
- VS 2022 Community
- Kernel-Version 1.23.0
- Commit: <SHA>

## Log

Siehe `bin\logs\<timestamp>_SU2-Wilson-Loop.log`.
```

**Schlechter Bug-Report:**

> „Bei mir geht's nicht."

### §8.2 — Ein Feature vorschlagen

**Guter Feature-Vorschlag:**

```markdown
## Was

Neue Funktion `ProPhysics_Wilson_Loop_Average(pu, m, n)`.

## Warum

Creutz-Ratio braucht Wilson-Loop-Mittelwerte. Ohne diese Funktion
muss der Test die Loop-Summen selbst berechnen.

## Wie

Implementation analog zu `ProPhysics_SU2_Link_Plaquette_Sum` —
read-only, O(Plaquettes).

## R-Konformität

- R1: Bit-Shift, kein div/mod
- R2: kein malloc
- R7: additive Erweiterung, kein Pfadwechsel

## `pro_run`-Auswirkung

Keine. Neuer Test kann in bestehende Prio 8 aufgenommen werden.
```

**Schlechter Feature-Vorschlag:**

> „Macht den Kernel schneller."

### §8.3 — Dokumentation beitragen

Doku-Beiträge sind **genauso wichtig** wie Code-Beiträge. Wir
akzeptieren:

- Tippfehler-Fixes
- Klarstellungen
- Neue Beispiele
- Übersetzungen (Englisch)
- API-Dokumentation für undokumentierte Funktionen
- **Web-Docs-Verbesserungen** (CSS, Views, Manifest-Pflege)

**Doku-Beiträge** durchlaufen ein **leichteres** Review — kein
Test-Lauf nötig, wenn nur Markdown geändert wird. Bei
Web-Docs-Änderungen (CSS, JS, HTML) sollte `pro_run web` laufen
und `out\web\index.html` im Browser geprüft werden.

### §8.4 — Physik beitragen

**Das ist der schwierigste Beitragstyp.** Wir akzeptieren:

- **Neue emergente Strukturen**, die aus U1–U5 folgen (mit Test).
- **V&V-Anker** gegen publizierte Physik.
- **Analytische Beweise** für Kernel-Eigenschaften.

Wir akzeptieren **nicht:**

- Neue Physik, die nicht aus U1–U5 folgt.
- Behauptungen ohne Test.
- „Quantengravitation" oder ähnliche große Worte ohne Substanz.

**Für einen Physik-Beitrag:** Öffne **zuerst** eine Discussion, um
die Idee zu diskutieren. Wir vermeiden, dass jemand Wochen arbeitet
und dann den Beitrag ablehnen müssen.

---

## §9 — Häufige Fragen

**F: Ich will einen Algorithmus verbessern. Wo fange ich an?**
A: Prüfe zuerst, ob der Algorithmus im Hotpath oder Coldpath liegt.
Hotpath-Änderungen brauchen R1/R2-Konformität und einen Test, der
beweist, dass die Ergebnisse gleich bleiben (oder besser werden).
Coldpath-Änderungen sind einfacher.

**F: Ich will einen neuen Basis-Zustand hinzufügen. Geht das?**
A: Nein, ohne Bruch. `PRO_AMP_BASIS_SIZE = 8` ist fix. Eine Änderung
würde alle bestehenden Tests brechen. Wenn du einen 9. Zustand
brauchst, diskutiere das in einer Discussion — es gibt Wege
(Sub-Basis, Erweiterung des Enums, ...), aber sie sind nicht trivial.

**F: Ich will den Kernel multithreaded machen.**
A: Das ist ein **großer** Eingriff. Der Kernel ist bewusst
single-threaded. Diskutiere das in einer Discussion, bevor du Code
schreibst.

**F: Ich finde einen Bug in einer Test-Ausgabe. Was tun?**
A: Öffne ein Issue mit dem vollständigen Log. Kleine Bugs in
Diagnose-Texten werden meist in einer Woche gefixt.

**F: Ich will den Code-Stil ändern.**
A: Nein. Der Stil ist bewusst so, wie er ist. Konsistenz ist wichtiger
als persönliche Vorlieben.

**F: Ich will die Tests von Prio 5 und 8 schneller machen.**
A: Willkommen. Aber die Tests messen **echte Physik**. Wenn du sie
schneller machst, muss die Physik erhalten bleiben (gleiche Ergebnisse
mit engeren Toleranzen). Reduziere nicht die Gittergröße, nur um
schneller zu sein.

**F: Ich brauche eine Funktion, die es nicht gibt.**
A: Öffne ein Issue mit dem Use-Case. Wir fügen Funktionen **additiv**
hinzu (R5). Wenn die Funktion zur Forschungsfrage passt, bauen wir sie.

**F: Ich will mit Python analysis.py verbessern.**
A: Willkommen. `python/analysis.py` ist die einzige Python-Datei.
Sie ist nicht Teil des Kernels, unterliegt nicht R1/R2.

**F: Ich will die Web-Docs verbessern.**
A: Willkommen. Lies zuerst `src/prowb/README.md` (Builder),
`docs/web/README.md` (Manifest-Pflege) und `docs/build/web-docs-ci.md`
(CI). Neue Doku-Einträge brauchen nur eine Zeile in
`docs\web\manifest.txt` — kein C-Code. Für CSS/JS/View-Änderungen:
`pro_run web` laufen lassen und `out\web\index.html` im Browser
prüfen.

**F: Ich will eine neue Sektion in den Web-Docs.**
A: Das erfordert **C-Änderung** in `prowb.c` (Sektionen sind hart
kodiert). Lies `docs/web/README.md` §5, bevor du anfängst. Öffne
zuerst eine Discussion.

**F: Ich will einen neuen Build-Schritt hinzufügen.**
A: Lies §6.6 — neue Build-Targets brauchen `pro_run`-Integration.
Ein neues Target, das nicht über `pro_run` erreichbar ist, wird
abgelehnt.

**F: Mein PR wurde abgelehnt. Was jetzt?**
A: Der Reviewer hat Gründe genannt. Lies sie. Wenn du denkst, dass
die Ablehnung falsch ist, antworte **sachlich** im PR — nicht per
E-Mail, nicht in einem neuen PR. Wir klären das öffentlich.

---

## §10 — Anerkennung

Beiträge werden in `CONTRIBUTORS.md` gelistet.

**Was wir anerkennen:**

- Code-Beiträge (Tests, Bug-Fixes, Features)
- Doku-Beiträge
- **Web-Docs-Beiträge** (Manifest, CSS, Views)
- Bug-Reports mit Reproduktion
- Physik-Beiträge mit Test
- Reviews von anderen PRs

**Was wir nicht anerkennen:**

- „Ich habe eine Frage gestellt."
- „Ich habe einen Tippfehler gemeldet, ohne ihn zu fixen."
- Beiträge, die den Kern des Projekts ändern wollen.

---

## §11 — Lizenz

Alle Beiträge stehen unter der Projekt-Lizenz (siehe `LICENSE.md`).
Wer einen Pull Request stellt, stimmt der Lizenz zu.

**Für kommerzielle Nutzer:** Der Beitrag ist Teil des kostenlosen
Projekts. Wer kommerziell nutzen will, braucht eine separate Lizenz
(siehe `COMMERCIAL.md`).

**Für Beitragende:** Du behältst das Copyright an deinem Beitrag.
Du lizenzierst ihn unter derselben Lizenz wie das Projekt.

---

## §12 — Verhaltenskodex

**Kurzfassung:** Sei ein anständiger Mensch.

- **Keine Belästigung** jeder Art.
- **Keine Diskriminierung** aufgrund von Geschlecht, Herkunft,
  Religion, sexueller Orientierung, Behinderung.
- **Keine persönlichen Angriffe.** Kritik am Code ist willkommen,
  Kritik an Personen nicht.
- **Keine Verschwörungserzählungen.** Das Projekt ist Wissenschaft,
  nicht Ideologie.

Verstöße werden **einmal** verwarnt, dann **dauerhaft** gesperrt.

---

## §13 — Siehe auch

| Thema | Datei |
|---|---|
| Projekt-Regeln | `docs/project/Project.md` §2 |
| Architektur | `docs/project/ARCHITECTURE.md` |
| Kernel-API | `docs/project/ProPhysics_API.md` |
| SDK-API | `docs/project/SDK_API.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Runner | `docs/test/run_alpha_tests.md` |
| **Zentraler Einstiegspunkt** | **`docs/build/pro_run.md`** |
| Build-System | `docs/build/BUILD_SCRIPT.md` |
| **ProWB-Makefile** | **`docs/build/prowb/Makefile.md`** |
| **Web-Docs-CI** | **`docs/build/web-docs-ci.md`** |
| **ProWB-Builder** | **`src/prowb/README.md`** |
| **Web-Docs-Manifest** | **`docs/web/README.md`** |
| Lizenz | `LICENSE.md` |
| Kommerzielle Lizenz | `COMMERCIAL.md` |

---

**Ende CONTRIBUTING v1.1.**