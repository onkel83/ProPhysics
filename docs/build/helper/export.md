# ProPhysics Export — Artefakt-Export mit ZIP-Archivierung

**Dateien:** `build\main\export.ps1` + `build\main\export.cmd`
**Version:** 1.0.0 (Build-System)
**Kernel:** 1.23.0 (Etappe 23)
**Zweck:** Baut (optional) via Master-Makefile, kopiert die Artefakte
plus passende Dokumentation in eine Zielstruktur und packt jede
Variante als ZIP-Archiv.

---

## 1. Was das Skript tut

Fuenf Aufgaben in einem Aufruf:

1. **Optional Build** — ruft `build\main\Makefile.nmake` mit dem
   passenden Target auf.
2. **Artefakte kopieren** — legt DLLs, LIBs, EXEs, Header und ggf.
   Beispiel-Quellen in eine Zielstruktur unter `out\`.
3. **Dokumentation kopieren** — kopiert die fuer den Export-Typ
   passenden MD-Dateien nach `<Paket>\docs\`.
4. **`README.md` + `BUILD_INFO.txt`** — erzeugt ein paket-spezifisches
   README und legt die Build-Metadaten bei.
5. **ZIP-Archivierung** — packt den Zielordner als
   `prophysics-<kind>-<version>.zip` neben den Ordner.

Der Export-Typ (`exe` / `sdk` / `kit` / `all`) bestimmt die
Zielstruktur. Der Scope (`kernel` / `prophysics` / `sdk` / `test` /
`all`) bestimmt, welche Komponenten einbezogen werden.

**Empfohlener Aufruf:** ueber `pro_run export …` in `tools\`. Siehe
`docs\build\pro_run.md`. Direkte Aufrufe ueber `export.cmd` bleiben
gueltig, wenn nur die Export-Komponente ohne Dispatch-Schicht
gebraucht wird.

---

## 2. Ablageort

```
<repo>\
├── build\
│   └── main\
│       ├── export.ps1       <- dieses Skript
│       ├── export.cmd       <- Wrapper fuer cmd.exe
│       └── Makefile.nmake   <- Master-Build
├── tools\
│   └── pro_run.ps1 / .cmd   <- zentraler Einstiegspunkt
├── bin\                     <- Quelle fuer exe/sdk/kit-Export
├── lib\                     <- Quelle fuer sdk/kit-Export
├── src\                     <- Quelle fuer kit-Export (Beispiele)
│   ├── prophysics\
│   ├── sdk\
│   └── test\
├── docs\                    <- Quelle fuer den Doku-Export
│   ├── build\
│   ├── project\
│   └── test\
├── BUILD_INFO.txt           <- Kopie landet im Paket
└── out\                     <- Ziel (Default)
    ├── exe\
    ├── sdk\
    ├── kit\
    ├── prophysics-exe-1.23.0.zip
    ├── prophysics-sdk-1.23.0.zip
    └── prophysics-kit-1.23.0.zip
```

Der Repo-Root wird ueber `$PSScriptRoot\..\..` ermittelt. Aufruf
funktioniert aus jedem CWD.

---

## 3. Aufruf

### 3.1 Ueber `pro_run` (empfohlen)

```cmd
:: SDK-Paket exportieren (Default-Export-Typ)
pro_run export

:: KIT-Paket nach D:\sdk-kit
pro_run export -Export kit -OutDir D:\sdk-kit

:: alle drei Pakete mit Rebuild
pro_run export -Export all -Rebuild -Clean

:: mit explizitem Version-Tag
pro_run export -Export sdk -Version 1.23.0
```

Details siehe `docs\build\pro_run.md`.

### 3.2 Ueber `export.cmd` (direkt)

```cmd
export.cmd exe
export.cmd sdk -Scope sdk
export.cmd kit -Version 1.23.0 -OutDir D:\dist
export.cmd all -Rebuild -Clean
```

Der `.cmd`-Wrapper setzt `chcp 65001` und reicht alle Argumente an
PowerShell weiter.

### 3.3 Direkt ueber PowerShell

```powershell
.\export.ps1 exe
.\export.ps1 sdk -Rebuild -Clean
.\export.ps1 kit -Scope all -Version 1.0.0
.\export.ps1 all -NoBuild -OutDir D:\release
```

### 3.4 Aus jedem CWD

```cmd
cd C:\Temp
H:\ProPhysics_SDK\ProPhysics\build\main\export.cmd sdk
```

---

## 4. Die Export-Typen

### 4.1 `exe` — Runtime-Paket

**Zweck:** Weitergabe an Anwender, die nur die Tests ausfuehren wollen.

**Ziel:** `out\exe\` (oder `-OutDir <pfad>\exe\`)

**Struktur:**

```
out\exe\
├── README.md                    ← generiert
├── BUILD_INFO.txt               ← Kopie aus Root
├── ProPhysics.dll
├── pro_sdk_interface.dll        (nur bei Scope ≥ sdk)
├── example_alpha_test.exe       (nur bei Scope ≥ test)
├── example_test_density.exe     (nur bei Scope ≥ test)
├── example_test_tensor.exe      (nur bei Scope ≥ test)
├── run_alpha_tests.cmd          (nur bei Scope ≥ test)
├── run_alpha_tests.ps1          (nur bei Scope ≥ test)
└── docs\
    ├── Project.md               ← docs\project\Project.md
    ├── ProPhysics_Testkatalog.md
    └── run_alpha_tests.md
```

**Enthaelt nicht:** Header, LIB-Dateien, Test-Quellcode, Python.

**ZIP:** `out\prophysics-exe-<version>.zip` — enthaelt den Inhalt des
Ordners flach (also `README.md`, DLLs, EXEs, `docs\` direkt).

### 4.2 `sdk` — SDK-Paket

**Zweck:** Weitergabe an externe Entwickler, die gegen den Kernel
programmieren wollen.

**Ziel:** `out\sdk\` (oder `-OutDir <pfad>\sdk\`)

**Struktur:**

```
out\sdk\
├── README.md                    ← generiert
├── BUILD_INFO.txt               ← Kopie aus Root
└── libs\
    ├── ProPhysics.dll
    ├── ProPhysics.lib
    ├── pro_sdk_interface.dll          (nur bei Scope ≥ sdk)
    ├── pro_sdk_interface.lib          (nur bei Scope ≥ sdk)
    ├── src\
    │   └── header\
    │       ├── prophysics\
    │       │   ├── ProPhysics.h
    │       │   ├── ProPhysics_Types.h
    │       │   ├── ProPhysics_Config.h
    │       │   ├── ProPhysics_Exports.h
    │       │   ├── ProPhysics_Version.h
    │       │   └── ProPhysics_Internal.h
    │       └── sdk\
    │           └── pro_sdk_interface.h
    └── docs\
        ├── Project.md
        ├── ProPhysics_Testkatalog.md
        ├── BUILD_SCRIPT.md
        ├── prophysics-Makefile.md
        └── sdk-Makefile.md
```

**Header-Layout:** Header werden in Unterordnern (`prophysics\`, `sdk\`)
gespiegelt, damit `#include`-Pfade konsistent bleiben:

```c
#include "prophysics/ProPhysics.h"
#include "sdk/pro_sdk_interface.h"
```

**Enthaelt nicht:** Test-Quellcode, EXE-Dateien, Python.

**ZIP:** `out\prophysics-sdk-<version>.zip`.

### 4.3 `kit` — Komplettes Entwicklungspaket (neu in v1.0.0)

**Zweck:** Weitergabe an Entwickler, die den Kernel in eigenen
Anwendungen nutzen und mit den Beispielprogrammen starten wollen.

**Ziel:** `out\kit\` (oder `-OutDir <pfad>\kit\`)

**Struktur:**

```
out\kit\
├── README.md                    ← generiert
├── BUILD_INFO.txt               ← Kopie aus Root
├── libs\                        ← identisch zu sdk\libs\
│   ├── ProPhysics.dll / .lib
│   ├── pro_sdk_interface.dll / .lib
│   ├── src\header\prophysics\*.h
│   └── src\header\sdk\*.h
├── examples\
│   ├── example_test_density.c   ← Dichte-API + Lindblad
│   └── example_test_tensor.c    ← Tensor-API + Fermionen + Fock
└── docs\                        ← erweiterter Doku-Auszug
    ├── Project.md
    ├── CONFIG.md
    ├── ProPhysics_Testkatalog.md
    ├── run_alpha_tests.md
    ├── BUILD_SCRIPT.md
    ├── prophysics-Makefile.md
    └── sdk-Makefile.md
```

**Enthaelt gegenueber `sdk` zusaetzlich:**
- Beispiel-Quellen unter `examples\`
- `docs\CONFIG.md` (alle Config-Konstanten)
- `docs\run_alpha_tests.md` (Test-Runner-Bedienung)

**Enthaelt nicht:** Test-Infrastruktur (`alpha_test_*.c`),
vollstaendige Doku-Mirror.

**ZIP:** `out\prophysics-kit-<version>.zip`.

### 4.4 `all` — alle drei Pakete (neu in v1.0.0)

**Zweck:** Ein Aufruf fuer alle Export-Varianten.

**Ziel:** erzeugt `out\exe\`, `out\sdk\`, `out\kit\` plus die drei
ZIP-Archive.

**Reihenfolge:** `exe` → `sdk` → `kit`. Fehlschlag eines Exports
bricht nicht ab — die anderen laufen weiter.

**Build:** einmal am Anfang (siehe §11).

**ZIP:** drei Archive in `out\`.

### 4.5 Was aus frueheren Versionen entfallen ist

| Alt | Neu | Grund |
|---|---|---|
| `-Mode src` | `-Export kit` | `kit` liefert dieselben Beispiele, ist aber als Entwickler-Paket klarer geschnitten |
| `-Mode <x>` | `-Export <x>` | Vokabular an `pro_run` angeglichen |
| `-OutRoot` | `-OutDir` (Alias bleibt) | Vokabular an `pro_run` angeglichen |
| kein ZIP | automatische ZIP-Erzeugung | siehe §5 |

Der Parameter `-Name` bleibt aus Kompatibilitaetsgruenden erhalten,
hat aber in der aktuellen Version keine Wirkung mehr — der `src`-Modus
ist aus der `ValidateSet` entfernt.

---

## 5. ZIP-Erzeugung (neu in v1.0.0)

Nach jedem Export wird der Zielordner zusaetzlich als ZIP archiviert.

### 5.1 Namensschema

```
prophysics-<kind>-<version>.zip
```

| Teil | Wert |
|---|---|
| `kind` | `exe`, `sdk`, `kit` |
| `version` | `MAJOR.MINOR.PATCH` aus `ProPhysics_Version.h`, **ohne `v`-Praefix** |

**Aktuell:** `prophysics-sdk-1.23.0.zip`.

### 5.2 Version-Override

Der ZIP-Name laesst sich mit `-Version` ueberschreiben:

```cmd
export.cmd sdk -Version 1.23.0
export.cmd kit -Version 1.23.0-rc1
```

Wenn `-Version` fehlt, wird die Version aus
`src\prophysics\header\ProPhysics_Version.h` gelesen.

### 5.3 ZIP-Inhalt

Das Archiv enthaelt **den Inhalt** des Zielordners, nicht den Ordner
selbst. Beispiel: `prophysics-sdk-1.23.0.zip` entpackt sich zu

```
libs\
README.md
BUILD_INFO.txt
```

und nicht zu `sdk\libs\...`. Das ist bewusst: das Archiv wird oft
direkt entpackt, der Empfaenger will den Inhalt, nicht einen Wrapper.

### 5.4 Was passiert bei fehlendem `OutDir`

`OutDir` wird angelegt, falls nicht vorhanden. Ein bereits
existierendes ZIP wird vor dem Schreiben geloescht
(`Compress-Archive -Force`).

### 5.5 Kompression

`-CompressionLevel Optimal`. Der Unterschied zu `Fastest` ist
typischerweise Faktor 2–3 bei SDK-Artefakten.

---

## 6. Dokumenten-Export — was wohin

### 6.1 Mapping pro Export-Typ

| Datei in `docs\` | `exe` | `sdk` | `kit` |
|---|:-:|:-:|:-:|
| `project\Project.md` | ✅ als `docs\Project.md` | ✅ als `docs\Project.md` | ✅ als `docs\Project.md` |
| `project\CONFIG.md` | – | – | ✅ |
| `test\ProPhysics_Testkatalog.md` | ✅ | ✅ | ✅ |
| `test\run_alpha_tests.md` | ✅ | – | ✅ |
| `build\BUILD_SCRIPT.md` | – | ✅ | ✅ |
| `build\prophysics\Makefile.md` | – | ✅ als `docs\prophysics-Makefile.md` | ✅ als `docs\prophysics-Makefile.md` |
| `build\sdk\Makefile.md` | – | ✅ als `docs\sdk-Makefile.md` | ✅ als `docs\sdk-Makefile.md` |
| `build\test\Makefile.md` | – | – | – |
| `build\main\Makefile.md` | – | – | – |
| `build\helper\*.md` | – | – | – |

**Rationale:**

- **`exe`:** Anwender braucht Test-Doku (welche Prios? wie aufrufen?)
  und Projekt-Uebersicht. Keine Build-Interna.
- **`sdk`:** Entwickler braucht Build-Doku (Makefile-Details fuer
  Kernel + SDK) plus Testkatalog (was der Kernel kann) plus
  Projekt-Uebersicht.
- **`kit`:** Entwickler plus Config-Referenz plus Runner-Doku. Wer
  die Beispiele nutzt, will auch wissen, wie er die Tests fahrt.

### 6.2 Was passiert, wenn eine Datei fehlt

Der `Copy-Docs`-Helper prueft jede Quelldatei. Fehlt eine, wird sie
mit `skip: <pfad>` gemeldet und die anderen werden trotzdem kopiert.
Der Export bricht **nicht** ab.

Beispiel:

```
[*] Export exe -> H:\...\out\exe
    OK  README.md
    OK  BUILD_INFO.txt
    --  skip: build\main\Makefile.md
    OK  Docs (2 Dateien)
```

Fehlt `docs\` komplett, kommt nur:

```
    --  docs\ nicht gefunden — kein Dokumenten-Export
```

---

## 7. README.md — paket-spezifischer Inhalt

Jedes Paket bekommt eine generierte `README.md` im Wurzelverzeichnis.
Der Inhalt haengt vom Export-Typ ab.

### 7.1 `exe`-README

Enthaelt:

- **Header** — Version, Zeitstempel, Host, Scope.
- **Paketinhalt** — Tabelle aller Dateien mit Rolle.
- **Schnellstart** — `run_alpha_tests.cmd -Prio all` und Einzeltest-Beispiel.
- **Prios-Tabelle** — alle 8 Prios mit Test-Anzahl und Dauer.
- **Doku-Verweise** — Liste der mitgelieferten MD-Dateien.

### 7.2 `sdk`-README

Enthaelt:

- **Header** — Version, Zeitstempel, Host, Scope.
- **Paketinhalt** — Ordnerstruktur mit Erklaerung.
- **Einbindung** — Include-Pfade, Library-Pfade, Link-Reihenfolge
  (`pro_sdk_interface.lib` **vor** `ProPhysics.lib`), MSVC-Beispiel.
- **DLL-Weitergabe** — welche DLLs neben der EXE liegen muessen.
- **Doku-Verweise**.

### 7.3 `kit`-README

Enthaelt:

- **Header** — Version, Zeitstempel, Host, Scope.
- **Paketinhalt** — Ordnerstruktur (libs + examples + docs).
- **Einbindung** — wie sdk, plus Beispiel-Compile-Zeile.
- **Doku-Verweise**.

### 7.4 Beispiel-Ausgabe (`exe`)

```markdown
# ProPhysics Package — EXE

**Version:** 1.23.0
**Erzeugt:** 2026-09-27 17:42:11 auf DEV-WORKSTATION
**Scope:** all

## Was ist in diesem Paket?

Ein lauffaehiges Runtime-Paket der ProPhysics-Alpha-Tests.
Alle DLLs und EXEs liegen **flach nebeneinander**, so
dass Windows die DLLs beim Start der EXEs automatisch findet.

| Datei | Rolle |
|---|---|
| `ProPhysics.dll` | Kernel-Bibliothek |
| `pro_sdk_interface.dll` | SDK-Interface |
| `example_alpha_test.exe` | Alpha-Test-Suite (43 Tests) |
| `example_test_density.exe` | Dichte-Regression (86 Checks) |
| `example_test_tensor.exe` | Tensor-/Fock-Regression (61 Checks) |
| `run_alpha_tests.cmd` / `.ps1` | Test-Runner |

## Schnellstart

```cmd
run_alpha_tests.cmd -Prio all
```

## Prioritaeten

| Prio | Thema | Tests | Typische Dauer |
|:-:|---|---:|---:|
| 1 | 2D-Basis | 12 | ~2 s |
| 2 | Emergenz | 10 | ~4,5 min |
| 3 | Langlauf | 10 | ~45 s |
| 4 | 3D-Torus | 3 | ~45 s |
| 5 | Hydrogen + Shared-Ref | 4 | ~36 min |
| 6 | Spin-1/2 | 1 | < 1 s |
| 7 | Dirac | 1 | ~15 s |
| 8 | SU(2) + Running-Coupling | 2 | ~30 min |
| `all` | alle | 43 | ~65 min |

## Dokumentation

| Datei | Inhalt |
|---|---|
| `docs\run_alpha_tests.md` | Test-Runner-Bedienung, Prios, Logs |
| `docs\ProPhysics_Testkatalog.md` | alle 43 Tests mit Kriterien |
| `docs\Project.md` | Ontologie und Roadmap |
| `BUILD_INFO.txt` | Version, Artefakt-Liste, Zeitstempel |
```

---

## 8. Parameter

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Export` | `exe` \| `sdk` \| `kit` \| `all` | *(mandatory)* | Export-Typ |
| `-Scope` | `kernel` \| `prophysics` \| `sdk` \| `test` \| `all` | `all` | Was einbezogen wird |
| `-OutDir` | Pfad | `<repo>\out` | Wurzelverzeichnis. Alias: `-OutRoot` |
| `-Version` | String | Version aus `ProPhysics_Version.h` | Tag fuer ZIP-Namen (ohne `v`) |
| `-Name` | String | (leer) | Legacy, inaktiv |
| `-NoBuild` | Switch | aus | nmake ueberspringen |
| `-Rebuild` | Switch | aus | nmake clean + Build vor Export |
| `-Clean` | Switch | aus | Zielordner vorher rekursiv loeschen |
| `-DryRun` | Switch | aus | nur auflisten, nichts kopieren/packen |

**Hinweise:**

- `-Export` ist **Pflicht**. Es gibt keinen Default — der Aufrufer
  muss sich entscheiden, welches Paket er will.
- `-OutRoot` bleibt als Alias fuer `-OutDir` erhalten. Neue Aufrufer
  sollten `-OutDir` verwenden.
- `-Version` wird in den ZIP-Namen eingebaut. Die Version im README
  und in BUILD_INFO kommt aus `ProPhysics_Version.h` — die beiden
  koennen auseinanderlaufen, wenn `-Version` gesetzt ist und vom
  Header abweicht. Das ist bewusst: der ZIP-Name ist ein Release-Tag,
  der Header-Stand ist ein Build-Datum.
- `-Name` ist ein Relikt aus dem `src`-Modus. Er wird aktuell
  gespeichert, aber nicht verwendet.
- `-NoBuild` funktioniert in `exe`, `sdk`, `kit` und `all`.

---

## 9. Scope × Export — Auswirkung

| Scope | `exe` | `sdk` | `kit` |
|---|---|---|---|
| `kernel` / `prophysics` | nur `ProPhysics.dll` | Kernel (DLL + LIB + Header) | Kernel + Beispiele |
| `sdk` | + `pro_sdk_interface.dll` | + SDK (DLL + LIB + Header) | + SDK + Beispiele |
| `test` | + 3 EXEs + Runner | + SDK-Header | + SDK + Beispiele |
| `all` | identisch mit `test` | identisch mit `test` | alle Komponenten |

**Kumulativ:** `-Scope sdk` schliesst `prophysics` ein. `-Scope test`
schliesst `sdk` und `prophysics` ein. Der Scope nach oben ist immer
vollstaendig.

**Der Scope beeinflusst die Docs nicht.** Die Dokumenten-Auswahl haengt
nur vom Export-Typ ab.

---

## 10. Beispiel-Aufrufe

### 10.1 Komplettes Runtime-Paket

```cmd
export.cmd exe
```

Ergebnis: `nmake all` laeuft, dann alle DLLs und EXEs nach `out\exe\`,
plus `README.md`, `BUILD_INFO.txt`, `docs\`, plus
`out\prophysics-exe-1.23.0.zip`.

### 10.2 SDK-Paket aus bereits gebauten Artefakten

```cmd
export.cmd sdk -NoBuild
```

Ergebnis: kein nmake-Aufruf, nur Kopieren. Voraussetzung: `bin\` und
`lib\` enthalten die aktuellen DLLs und Libs.

### 10.3 KIT-Paket mit Rebuild

```cmd
export.cmd kit -Scope all -Rebuild -Clean -OutDir D:\dist
```

Ergebnis: kompletter Neu-Build, Zielordner `D:\dist\kit\` geleert,
KIT-Layout erstellt. ZIP: `D:\dist\prophysics-kit-1.23.0.zip`.

### 10.4 Alle drei Pakete auf einmal

```cmd
export.cmd all -Rebuild -Clean
```

Ergebnis: ein Build, dann `out\exe\`, `out\sdk\`, `out\kit\` plus drei
ZIPs.

### 10.5 Mit explizitem Version-Tag

```cmd
export.cmd sdk -Version 1.23.0-rc1
```

Ergebnis: `out\prophysics-sdk-1.23.0-rc1.zip`.

### 10.6 In ein anderes Zielverzeichnis

```cmd
export.cmd exe -OutDir D:\dist
```

Ergebnis: `D:\dist\exe\`, `D:\dist\prophysics-exe-1.23.0.zip`.

### 10.7 Dry-Run

```cmd
export.cmd sdk -Rebuild -DryRun
```

Ergebnis: zeigt, welches nmake-Target aufgerufen wuerde und welche
Dateien kopiert wuerden — ohne Schreibzugriff.

---

## 11. Zielverzeichnis- und ZIP-Bestimmung

| `-Export` | `-OutDir` (Default `<repo>\out`) | Ordner | ZIP |
|---|---|---|---|
| `exe` | `<repo>\out` | `<repo>\out\exe\` | `<repo>\out\prophysics-exe-<version>.zip` |
| `sdk` | `<repo>\out` | `<repo>\out\sdk\` | `<repo>\out\prophysics-sdk-<version>.zip` |
| `kit` | `<repo>\out` | `<repo>\out\kit\` | `<repo>\out\prophysics-kit-<version>.zip` |
| `all` | `<repo>\out` | alle drei | alle drei ZIPs |
| `exe` | `D:\dist` | `D:\dist\exe\` | `D:\dist\prophysics-exe-<version>.zip` |
| `sdk` | `D:\dist` | `D:\dist\sdk\` | `D:\dist\prophysics-sdk-<version>.zip` |
| `kit` | `D:\dist` | `D:\dist\kit\` | `D:\dist\prophysics-kit-<version>.zip` |

Der Zielordner wird **angelegt**, falls nicht vorhanden. Mit `-Clean`
wird er vorher rekursiv geleert. Ein bereits existierendes ZIP wird
vor dem Schreiben geloescht.

---

## 12. Interner Ablauf

```
export.ps1
    │
    ├─ UTF-8 einstellen
    │
    ├─ Version bestimmen (-Version oder aus Header)
    │
    ├─ Build ausfuehren (ausser -NoBuild)
    │   ├─ nmake-Target aus Scope + -Rebuild ableiten
    │   ├─ -Rebuild gesetzt → rebuild_<scope>
    │   └─ sonst → <scope>
    │
    ├─ Zielverzeichnis anlegen
    │
    ├─ Fuer jeden Export-Typ in ($Export == 'all' ? {exe, sdk, kit} : {$Export}):
    │   │
    │   ├─ Clean (falls -Clean)
    │   ├─ Ensure-Dir Zielordner
    │   ├─ Artefakte kopieren (DLLs/LIBs/EXEs/Header/Beispiele je nach Typ)
    │   ├─ Copy-BuildInfo → Zielordner
    │   ├─ Copy-Docs (typ-abhaengige Auswahl) → <Ziel>\docs\
    │   ├─ New-PackageReadme (typ-abhaengig) → <Ziel>\README.md
    │   └─ New-PackageZip → <OutDir>\prophysics-<kind>-<version>.zip
    │
    └─ Zusammenfassung (Datei-Anzahl + ZIP-Groesse je Typ)
```

---

## 13. Was exportiert wird — im Detail

### 13.1 `exe`-Typ

| Quelle | Ziel | Bedingung |
|---|---|---|
| `bin\ProPhysics.dll` | `out\exe\` | immer |
| `bin\pro_sdk_interface.dll` | `out\exe\` | Scope ≥ sdk |
| `bin\example_*.exe` | `out\exe\` | Scope ≥ test |
| `tools\run_alpha_tests.cmd` | `out\exe\` | Scope ≥ test |
| `tools\run_alpha_tests.ps1` | `out\exe\` | Scope ≥ test |
| `BUILD_INFO.txt` | `out\exe\` | immer (falls vorhanden) |
| `docs\project\Project.md` | `out\exe\docs\` | immer |
| `docs\test\ProPhysics_Testkatalog.md` | `out\exe\docs\` | immer |
| `docs\test\run_alpha_tests.md` | `out\exe\docs\` | immer |
| generiert | `out\exe\README.md` | immer |

### 13.2 `sdk`-Typ

| Quelle | Ziel | Bedingung |
|---|---|---|
| `bin\ProPhysics.dll` | `out\sdk\libs\` | immer |
| `lib\ProPhysics.lib` | `out\sdk\libs\` | immer |
| `bin\pro_sdk_interface.dll` | `out\sdk\libs\` | Scope ≥ sdk |
| `lib\pro_sdk_interface.lib` | `out\sdk\libs\` | Scope ≥ sdk |
| `src\prophysics\header\*.h` | `out\sdk\libs\src\header\prophysics\` | immer |
| `src\sdk\header\*.h` | `out\sdk\libs\src\header\sdk\` | Scope ≥ sdk |
| `BUILD_INFO.txt` | `out\sdk\` | immer (falls vorhanden) |
| `docs\project\Project.md` | `out\sdk\docs\` | immer |
| `docs\test\ProPhysics_Testkatalog.md` | `out\sdk\docs\` | immer |
| `docs\build\BUILD_SCRIPT.md` | `out\sdk\docs\` | immer |
| `docs\build\prophysics\Makefile.md` | `out\sdk\docs\prophysics-Makefile.md` | immer |
| `docs\build\sdk\Makefile.md` | `out\sdk\docs\sdk-Makefile.md` | immer |
| generiert | `out\sdk\README.md` | immer |

### 13.3 `kit`-Typ

| Quelle | Ziel | Bedingung |
|---|---|---|
| (wie `sdk`-Typ, unter `libs\`) | `out\kit\libs\` | immer |
| `src\test\example_test_density.c` | `out\kit\examples\` | immer |
| `src\test\example_test_tensor.c` | `out\kit\examples\` | immer |
| `BUILD_INFO.txt` | `out\kit\` | immer (falls vorhanden) |
| `docs\project\Project.md` | `out\kit\docs\` | immer |
| `docs\project\CONFIG.md` | `out\kit\docs\` | immer |
| `docs\test\ProPhysics_Testkatalog.md` | `out\kit\docs\` | immer |
| `docs\test\run_alpha_tests.md` | `out\kit\docs\` | immer |
| `docs\build\BUILD_SCRIPT.md` | `out\kit\docs\` | immer |
| `docs\build\prophysics\Makefile.md` | `out\kit\docs\prophysics-Makefile.md` | immer |
| `docs\build\sdk\Makefile.md` | `out\kit\docs\sdk-Makefile.md` | immer |
| generiert | `out\kit\README.md` | immer |

---

## 14. Konsolenausgabe

### 14.1 Start

```
============================================================
  ProPhysics Export  |  Export: sdk  Scope: all
============================================================
  Repo:    H:\ProPhysics_SDK\ProPhysics
  Ausgabe: H:\ProPhysics_SDK\ProPhysics\out
  Version: 1.23.0
```

### 14.2 Waehrend des Exports

```
[*] Baue (all)...
...
    OK  Build abgeschlossen.
[*] Export sdk -> H:\...\out\sdk
    OK  BUILD_INFO.txt
    OK  Docs (5 Dateien)
    OK  README.md
[*] ZIP: prophysics-sdk-1.23.0.zip
    OK  ZIP erzeugt (128.4 KB)
    OK  Fertig: H:\...\out\sdk
```

### 14.3 Zusammenfassung

```
------------------------------------------------------------
  Zusammenfassung
------------------------------------------------------------
  exe   17 Datei(en)  ->  prophysics-exe-1.23.0.zip (241.7 KB)
  sdk   14 Datei(en)  ->  prophysics-sdk-1.23.0.zip (128.4 KB)
  kit   21 Datei(en)  ->  prophysics-kit-1.23.0.zip (196.3 KB)
```

Farbcodierung wie bei `build.ps1`:

| Farbe | Bedeutung |
|---|---|
| Cyan | Schritt-Markierung |
| Gruen | Erfolg |
| Gelb | Warnung (nicht kritisch) |
| Rot | Fehler |
| Grau | Dry-Run-Detail |

---

## 15. Exit-Codes

| Code | Bedeutung |
|---|---|
| `0` | Erfolg |
| `2` | `nmake` nicht im PATH (nur bei Build) |
| `≠0` | `nmake`-Exit-Code |

Fehlende Quelldateien (Docs, Header) brechen nicht ab — sie werden
mit `skip:` geloggt, und der Export laeuft weiter.

---

## 16. Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `nmake nicht im PATH` | VS-Developer-Prompt fehlt | `vcvars64.bat` ausfuehren |
| `Quelle fehlt: ...\bin` | Kernel nicht gebaut | ohne `-NoBuild` neu laufen |
| `Quelle fehlt: ...\lib` | Kernel + SDK nicht gebaut | `pro_run build -Mode sdk` |
| `example_*.exe` fehlen im Ziel | Scope zu niedrig | `-Scope test` oder `-Scope all` |
| `pro_sdk_interface.dll` fehlt | Scope zu niedrig | `-Scope sdk` oder hoeher |
| `docs\ nicht gefunden` | Doku nie angelegt | `docs\project\`, `docs\test\` pruefen |
| `skip: build\main\Makefile.md` | Doc-Datei fehlt | Doku nachliefern |
| `BUILD_INFO.txt fehlt` | Build nie gelaufen | `pro_run build` laufen lassen |
| Umlaute in Ausgabe kaputt | PowerShell umging `chcp` | `export.cmd` statt `export.ps1` |
| `Compress-Archive fehlgeschlagen` | Datei gesperrt oder Ziel voll | Prozess pruefen, Ziel leeren |
| ZIP-Name zeigt unerwartete Version | Header-Version geaendert | `-Version` explizit setzen |
| `-Name` wirkungslos | `src`-Modus entfernt | `-Export kit` nutzen |

---

## 17. Was dieses Skript nicht tut

- **Kein Signing.** DLLs bleiben unsigniert — fuer Signatur
  `build.ps1 -Sign` verwenden, dann exportieren.
- **Kein Git-Push.** Kein Upload irgendwohin.
- **Kein Test-Lauf.** Der Test-Runner bleibt separat.
- **Kein Diff.** Es wird blind kopiert, kein Vergleich mit vorherigem
  Export.
- **Kein Docs-Rendering.** Markdown bleibt Markdown, keine PDF/HTML-
  Konvertierung.
- **Kein Lizenz-File.** Falls eine `LICENSE` im Root liegt, wird sie
  **nicht** automatisch mitkopiert.
- **Kein SHA-Hash.** Die ZIPs werden nicht mit einer Pruefsumme
  versehen.

---

## 18. Parameter-Referenz (kompakt)

```
export.cmd <exe|sdk|kit|all>
           [-Scope <kernel|prophysics|sdk|test|all>]
           [-OutDir <pfad>]
           [-Version <string>]
           [-NoBuild] [-Rebuild] [-Clean] [-DryRun]
```

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `-Export` | Choice | *(mandatory)* | `exe` \| `sdk` \| `kit` \| `all` |
| `-Scope` | Choice | `all` | Was einbezogen wird |
| `-OutDir` | Pfad | `<repo>\out` | Wurzelverzeichnis |
| `-Version` | String | Header-Version | Tag fuer ZIP-Namen |
| `-NoBuild` | Switch | aus | nmake ueberspringen |
| `-Rebuild` | Switch | aus | clean + Build |
| `-Clean` | Switch | aus | Zielordner leeren |
| `-DryRun` | Switch | aus | nur auflisten |

---

## 19. Beispiele fuer typische Szenarien

### 19.1 Release-Paket fuer Anwender

```cmd
export.cmd exe -Scope all -Rebuild -Clean
```

Liefert `out\exe\` und `out\prophysics-exe-1.23.0.zip` mit:

- Allem, was zum Ausfuehren der Tests noetig ist.
- `README.md` mit Schnellstart und Prios-Tabelle.
- `docs\` mit Testkatalog und Runner-Doku.
- `BUILD_INFO.txt` als Versionsnachweis.

Anwender entpackt das ZIP irgendwohin, fuehrt `run_alpha_tests.cmd`
aus. Fertig.

### 19.2 SDK an externen Entwickler

```cmd
export.cmd sdk -Scope sdk -Rebuild -Clean -OutDir D:\dist
```

Liefert `D:\dist\sdk\` und `D:\dist\prophysics-sdk-1.23.0.zip` mit:

- `libs\` mit Header, LIBs und DLLs.
- `libs\docs\` mit Build-Doku.
- `README.md` mit Einbindungs-Anleitung.

Der Empfaenger kann gegen `pro_sdk_interface.lib` linken und die
Header unter `libs\src\header\prophysics\` finden.

### 19.3 Komplettes Entwicklungspaket

```cmd
export.cmd kit -Scope all -OutDir D:\sdk-kit
```

Liefert `D:\sdk-kit\kit\` und `D:\sdk-kit\prophysics-kit-1.23.0.zip`
mit SDK, Beispielen und erweitertem Doku-Auszug.

### 19.4 Alles in einem Aufruf

```cmd
export.cmd all -Rebuild -Clean -OutDir D:\release
```

Erzeugt `D:\release\{exe,sdk,kit}\` und die drei ZIPs.

### 19.5 CI-Export

```cmd
export.cmd all -Scope all -NoBuild -Clean -OutDir %CI_ARTIFACT_DIR%
```

Im CI ist der Build schon gelaufen. Der Export kopiert nur die
Artefakte plus Docs und packt die ZIPs ins Artefakt-Verzeichnis.

---

## 20. Siehe auch

- `docs\build\pro_run.md` — zentraler Einstiegspunkt
- `docs\build\BUILD_SCRIPT.md` — Uebersicht des Build-Systems
- `docs\build\helper\build.md` — Build-Wrapper
- `docs\build\helper\write_build_info.md` — BUILD_INFO.txt
- `docs\build\main\Makefile.md` — Master-Makefile
- `docs\build\prophysics\Makefile.md` — Kernel-Build
- `docs\build\sdk\Makefile.md` — SDK-Build
- `docs\build\test\Makefile.md` — Test-Build
- `docs\test\run_alpha_tests.md` — Test-Runner
- `docs\test\ProPhysics_Testkatalog.md` — Test-Uebersicht

---

**Ende Export-Wrapper-Dokumentation (v1.0.0).**