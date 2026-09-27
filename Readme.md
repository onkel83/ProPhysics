# ProPhysics

Ein topologischer Graph-Kernel in C99, der Quantenmechanik
**emergieren** lässt — aus signed permutations, 8-dimensionaler
Amplituden-Basis und einer einzigen shared-reference-Regel.

**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-28 (Konsolidierungs-Serie 1.23.1–1.23.11 abgeschlossen,
ProWB integriert)
**Lizenz:** siehe `LICENSE.md`
**Status:** validierter Forschungs-Prototyp

---

## 1. Was ist ProPhysics?

ProPhysics ist ein **Simulations-Kernel** für Quantenmechanik auf einem
diskreten Graphen. Anders als klassische Lattice-QFT-Simulatoren
verzichtet ProPhysics auf dichte unitäre Matrizen und komplexe
Gleitkomma-Arithmetik. Stattdessen arbeitet der Kernel mit:

- **signed permutations** als fundamentale unitäre Operationen
- **8-dimensionaler Amplituden-Basis** pro Gitterknoten
- **Q31-Integer-Arithmetik** (int64, Re/Im je int32)
- **shared reference** (Union-Find) für Verschränkung
- **Quaternion-Links** für nicht-abelsche Eichtheorie (Skala 2³⁰)

Der Kernel ist **kein Kontinuumslimes**, **keine Quantengravitation**
und **keine Alternative zu etablierten Lattice-QCD-Codes**. Er ist ein
**Experimentierfeld** für die Frage: Reichen einfache diskrete
Strukturen, um Phänomene zu erzeugen, die wir als „Quantenmechanik"
kennen?

**Was er kann:**

- Unitäre Dynamik auf 2D/3D-Torus (bis 128³)
- Bloch-Dispersion (tight-binding, 2D/3D) auf `rel_dev < 5e-6`
- Dirac-Spinor (4-Komponenten), Clifford-Algebra auf `9e-10`
- Jordan-Wigner-Fermionen, 8-Moden-Fock-Raum
- SU(2)-Eichtheorie (kinematisch + Leapfrog-Dynamik)
- Metropolis-Sampling auf SU(2)-Links
- Lindblad / offene Systeme
- Lorentz-Zeitdilatation aus Tick-Unitariät
- Born-Verteilung aus Projektion
- Tsirelson-Korrelation aus U4
- **Web-Docs** (ProWB-Builder, publizierbar)

**Was er bewusst nicht ist:**

- Keine Simulation echter QCD (zu klein, zu langsam)
- Keine Quantengravitation
- Keine Aussage über den Kontinuumslimes
- Keine Behauptung, dass die Urregeln „die richtigen" sind

---

## 2. Ziel des Projekts

**Kernfrage:** Können die 5 Urregeln (U1–U5) Quantenmechanik tragen,
ohne dass QM explizit eincodiert wird?

```
U1:  U = {0,1} × {00,01,10,11}          (8 Basis-Zustände)
U2:  A : G → U ∪ G, G ⊆ ℤ³              (Interaktion pro Tick)
U3:  S_{t+1} = f(S_t)                   (Tick-Iteration)
U4:  Ψ(x,y) ⟺ A(x) = A(y)               (shared reference)
U5:  Σ_{x∈G} A_t(x) = Konstante         (Bit-Erhaltung)
```

**Was das bedeutet:**

- **U1** legt fest, was existiert (8 Basis-Zustände pro Knoten).
- **U2** legt fest, was passiert (lokal, pro Tick).
- **U3** legt die Reihenfolge fest.
- **U4** erzeugt Verschränkung über Pointer-Identität.
- **U5** ist der Erhaltungssatz.

Der Kernel soll zeigen, dass aus diesen fünf Regeln
**emergent** entstehen: Born-Regel, Bloch-Dispersion,
Lorentz-Invarianz, Dirac-Struktur, Spin-1/2, nicht-abelsche
Eichtheorie, fermionische Statistik.

**Was „emergent" hier heißt:** Der Code enthält **keine** Formel
wie `P = |ψ|²` oder `ω² = k² + m²`. Diese Strukturen entstehen aus
der Wechselwirkung der 5 Urregeln.

**Was „emergent" hier nicht heißt:** Der Kernel ist keine neue Physik.
Er ist eine **andere Darstellung** bekannter Physik. Die Frage ist,
ob diese Darstellung **sparsamer** oder **tiefer** ist.

---

## 3. Wo wir stehen

### 3.1 — Status

| Kategorie | Status |
|---|---|
| Fundament (U1–U5) | ✅ abgeschlossen |
| Unitäre Dynamik 2D/3D | ✅ exakt |
| Bloch-Dispersion | ✅ quantitativ bestätigt (`rel_dev < 5e-6`) |
| Dirac-Struktur | ✅ abgeschlossen |
| Spin-1/2 aus SU(2) | ✅ abgeschlossen |
| SU(2)-Eichtheorie (kinematisch) | ✅ abgeschlossen |
| SU(2)-Link-Dynamik (Leapfrog) | ✅ abgeschlossen |
| SU(2)-Metropolis | ✅ abgeschlossen |
| **V&V-Anker** | ✅ **0,08 % gegen externe Lattice-QCD** |
| Fermionen (Jordan-Wigner) | ✅ abgeschlossen |
| Lindblad / Dichte-Matrizen | ✅ abgeschlossen |
| **Tests** | **43/43 PASS** |
| **Web-Docs (ProWB)** | ✅ abgeschlossen |

### 3.2 — Was Etappe 23 erreicht hat

Die erste **absolute** Validierung des Kernels:

> Der Metropolis-Sampler auf SU(2)-Links reproduziert den analytischen
> Ein-Plaquette-Wert `⟨P⟩ = I₂(β)/I₁(β) = 0.43313` bei β=2 auf
> `0.43346 ± 0.00005` — Abweichung **0,08 %**.

Das bedeutet: Wilson-Action-Normierung, Metropolis-Akzeptanzschritt
und Q30-Quaternion-Multiplikation sind unabhängig validiert. Ein
falscher Sampler würde auf Prozent-Ebene abweichen, nicht auf 0,08 %.

### 3.3 — Was als nächstes kommt

| Etappe | Thema | Ziel |
|---|---|---|
| 24 | Euklidisches Pfadintegral | Pfadintegral ↔ Operator-Formalismus |
| 25 | GHZ / Mermin | n=3-Verschränkung, Hypergraph |
| 26 | Universalität | T-Gate, Deutsch-Josza |
| 27 | Q61-Migration | Numerische Präzision (int128) |
| M1 | U4' — Bad | Kopplung an unsichtbare Freiheitsgrade |
| M2 | U5' — Plastizität | Dynamische Topologie |
| M3 | Makrophysik | Klassischer Limes |

**Nach Etappe 27:** Kernel-Version **2.0.0** (Phase 2, „komplette QM").
**Nach M1–M3:** Kernel-Version **3.0.0** (Phase 3, Makrophysik).

Siehe `docs\project\VERSIONING.md` für das vollständige Schema.

---

## 4. Need-Liste — Was zum Bauen und Ausführen benötigt wird

### 4.1 — Hardware

| Komponente | Minimum | Empfohlen |
|---|---|---|
| CPU | x86-64, 2 Kerne | 4+ Kerne (nmake parallelisiert) |
| RAM | 512 MB | 4 GB (für dim=64 Metropolis + parallele Läufe) |
| Speicherplatz | 200 MB | 500 MB (Build-Artefakte + Logs + Exporte) |
| GPU | nicht benötigt | nicht benötigt |

**RAM-Verbrauch pro Lauf:**

| Test | Knoten | RAM |
|---|---|---|
| Prio 1–4 (2D/3D) | bis 128² | < 20 MB |
| Prio 5 (Hydrogen-48) | 64³ = 262 144 | ~270 MB |
| Prio 8 (Running-Coupling) | 64³ = 262 144 | ~262 MB |

**Speicherplatz für Artefakte:**

- Build (`bin\`, `lib\`, `bin\prowb\`): ~6 MB
- Logs (nach Prio-All): ~10 MB
- Export (`out\`, ohne `out\web\`): ~30–50 MB je nach Modus
- Web-Docs (`out\web\`): ~1,5 MB

### 4.2 — Software (Build)

**Zwingend erforderlich:**

| Tool | Version | Zweck |
|---|---|---|
| **Visual Studio** | 2019 oder 2022 | Compiler `cl.exe`, Linker `link.exe`, `nmake.exe` |
| **Windows SDK** | 10 oder 11 | Für `cl.exe` Abhängigkeiten |
| **PowerShell** | 5.0+ | `pro_run`, Build-Wrapper, Export-Wrapper |
| **cmd.exe** | Windows 10+ | Wrapper für `.cmd`-Skripte |

**Optional:**

| Tool | Zweck |
|---|---|
| **git** | Für `-GitStamp` (Build-Metadaten) |
| **signtool.exe** | Für `build.cmd -Sign` (DLL-Signierung, aus Windows SDK) |

**Build-Reihenfolge:** `prophysics` → `sdk` → `test`.
ProWB ist eigenständig und läuft im Master-`all` **nach** `test`.
Wird automatisch durch das Master-Makefile erzwungen.

**Empfohlener Einstiegspunkt:** `pro_run` (in `tools\`). Dispatcht
auf `build.ps1`, `export.ps1`, `run_alpha_tests.ps1` und den
ProWB-Builder. Details siehe `docs\build\pro_run.md`.

### 4.3 — Software (Ausführung)

**Zum Ausführen der Test-Suite:**

- Nur Windows (EXEs sind x86-64-Binaries)
- Keine zusätzlichen Laufzeit-Abhängigkeiten
- Alle DLLs liegen in `bin\` neben den EXEs

**Zum Ansehen der Web-Docs:**

- Jeder moderne Browser (Chrome, Firefox, Edge, Safari)
- Kein Server nötig — `out\web\index.html` ist eine statische Datei

**Zum Ausführen von `analysis.py` (CSV-Auswertung, optional):**

| Tool | Version |
|---|---|
| Python | 3.8+ |
| pandas | 1.0+ |
| numpy | 1.18+ |
| matplotlib | 3.2+ |
| seaborn | 0.11+ |
| scipy | 1.5+ |
| scikit-learn | 0.24+ |

Installation:
```cmd
pip install pandas numpy matplotlib seaborn scipy scikit-learn
```

### 4.4 — Umgebungs-Setup

**Entwicklungsumgebung (einmalig):**

1. Visual Studio 2019/2022 installieren mit den Workloads:
   - „Desktop development with C++"
   - „Windows 10 SDK" oder „Windows 11 SDK"

2. Optional: Git installieren
   - https://git-scm.com/download/win

3. Optional: Python 3 installieren
   - https://www.python.org/downloads/

**Bei jedem Build:**

Visual Studio Developer Command Prompt öffnen:
```cmd
:: Start → „x64 Native Tools Command Prompt for VS 2022"
:: Oder manuell:
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

**Prüfen, dass die Umgebung stimmt:**

```cmd
where nmake
where cl
where link
```

Alle drei müssen gefunden werden. Wenn nicht, ist der Developer
Prompt nicht aktiv.

**Empfohlener Workflow:**

```cmd
cd tools
pro_run build            :: Kernel + SDK + Tests + ProWB
pro_run test -Prio 1     :: schnelle Regression
```

---

## 5. Build & Run — Schnellstart

### 5.1 — Build (5 Sekunden bis 2 Minuten)

```cmd
cd tools
pro_run build
```

Baut Kernel + SDK + Tests + ProWB. Ergebnis in `bin\`, `lib\`,
`bin\prowb\`.

**Weitere Aufrufe:**

```cmd
pro_run build -Mode kernel        :: nur Kernel
pro_run build -Mode all -Rebuild  :: clean + alles neu
pro_run build -Config debug       :: Debug-Konfiguration
pro_run build -DryRun             :: nur auflisten
```

**Legacy-Aufrufe** (bleiben funktional, sind aber nicht mehr der
empfohlene Weg):

```cmd
cd build\main
build.cmd
build.cmd -Mode prophysics
build.cmd -Mode all -Rebuild
build.cmd -Clean
build.cmd -Rebuild -GitStamp
```

### 5.2 — Test (2 Sekunden bis 74 Minuten)

```cmd
cd tools
pro_run test -Prio all    :: alle 43 Tests, ~74 min
```

**Für schnelle Regressionen:**

```cmd
pro_run test -Prio 1      :: 2D-Basis, ~5 s
pro_run test -Prio 7      :: Dirac, ~15 s
```

**Für die volle Physik-Prüfung:**

```cmd
pro_run test -Prio 8      :: SU(2) + Running-Coupling, ~24 min
```

**Achtung:** Prio 8 enthält zwei Tests. Der zweite
(`Running-Coupling`) läuft ~23 min. Für CI-Zwecke:

```cmd
pro_run test -Test SU2-Wilson-Loop    :: nur der kurze Test, ~2 s
```

Siehe `docs\test\run_alpha_tests.md` für Details.

### 5.3 — Einzelner Test (manuell)

```cmd
cd tools
pro_run test -Test Running-Coupling -LogDir C:\logs
```

**Alternativ direkt:**

```cmd
cd bin
example_alpha_test.exe --test-su2-wilson-loop
example_alpha_test.exe --test-running-coupling   :: ~23 min
```

### 5.4 — Export (optional)

```cmd
cd tools
pro_run export -Export sdk -Clean      :: SDK-Paket nach out\sdk\
pro_run export -Export exe             :: Runtime-Paket nach out\exe\
pro_run export -Export kit             :: SDK + Beispiele + Docs
pro_run export -Export all             :: alle drei
```

**Legacy-Aufruf:**

```cmd
cd build\main
export.cmd sdk -Scope all -Clean
export.cmd kit -Version 1.23.0
```

### 5.5 — Web-Docs (optional)

```cmd
cd tools
pro_run web                :: Web-Docs nach out\web\
pro_run web -Rebuild       :: clean + ProWB-Build + Web-Docs
pro_run web -OutDir D:\portal
```

Ergebnis: `out\web\index.html` (~1,2 MB bei 44 Docs).
Öffne die Datei im Browser (Doppelklick) — keine Server nötig.

Details siehe `docs\build\prowb\Makefile.md`, `src\prowb\README.md`,
`docs\web\README.md`.

---

## 6. Dokumentation

### Kern-Dokumente

| Dokument | Inhalt |
|---|---|
| `README.md` (Root) | **diese Datei** |
| `CHANGELOG.md` | Änderungsprotokoll |
| `TODO.md` | Aufgaben-Register |
| `CONTRIBUTING.md` | Beitragen |
| `CONTRIBUTORS.md` | Beitragende |
| `LICENSE.md` | Lizenz |
| `COMMERCIAL.md` | Kommerzielle Lizenzierung |

### Projekt-Dokumente

| Dokument | Inhalt |
|---|---|
| `docs\project\Project.md` | Ontologie, Roadmap, Regeln, Etappen-Historie |
| `docs\project\ARCHITECTURE.md` | Kernel-Aufbau, Tick-Ablauf |
| `docs\project\ProPhysics_API.md` | Vollständige Funktions-Referenz |
| `docs\project\ProPhysics_Differentiators.md` | Abgrenzung zu anderen Frameworks |
| `docs\project\ProPhysics_VersionRegistry.md` | Versionen aller Dateien |
| `docs\project\CONFIG.md` | Compile-Time-Konstanten |
| `docs\project\VERSIONING.md` | Versionsschema |
| `docs\project\SDK_API.md` | SDK-Interface-Referenz |
| `docs\project\<Modul>.md` | 12 Modul-Referenzen |

### Test-Dokumente

| Dokument | Inhalt |
|---|---|
| `docs\test\ProPhysics_Testkatalog.md` | Alle 43 Tests mit Kriterien |
| `docs\test\run_alpha_tests.md` | Test-Runner-Bedienung |
| `docs\test\BASELINE.md` | Test-Baseline (Kurzfassung) |
| `docs\test\WRITING_TESTS.md` | Anleitung zum Test-Schreiben |

### Build-Dokumente

| Dokument | Inhalt |
|---|---|
| `docs\build\BUILD_SCRIPT.md` | Build-System-Übersicht |
| `docs\build\pro_run.md` | Zentraler Einstiegspunkt |
| `docs\build\web-docs-ci.md` | Web-Docs CI-Workflow |
| `docs\build\main\Makefile.md` | Master-Makefile |
| `docs\build\prophysics\Makefile.md` | Kernel-Build |
| `docs\build\sdk\Makefile.md` | SDK-Interface-Build |
| `docs\build\test\Makefile.md` | Test-Build |
| `docs\build\prowb\Makefile.md` | ProWB-Build |
| `docs\build\helper\*.md` | PowerShell-Wrapper |

### Web-Docs / ProWB

| Dokument | Inhalt |
|---|---|
| `src\prowb\README.md` | Builder-Referenz |
| `docs\web\README.md` | Manifest-Pflege |

### Physik

| Dokument | Inhalt |
|---|---|
| `docs\physics\README.md` | Physik-Übersicht |

**Empfohlene Lesereihenfolge für neue Mitwirkende:**

1. **diese Datei** (Überblick)
2. `docs\project\Project.md` (was das Projekt ist)
3. `docs\build\pro_run.md` (wie man baut, testet, exportiert)
4. `docs\test\ProPhysics_Testkatalog.md` (was getestet wird)
5. `docs\test\run_alpha_tests.md` (wie man Tests fährt)
6. `src\prowb\README.md` (wie die Web-Docs entstehen)

---

## 7. Projekt-Struktur

```
ProPhysics\
├── README.md                    # diese Datei (Kopie in docs\project\)
├── BUILD_INFO.txt               # Build-Metadaten (auto-generiert)
│
├── bin\                         # DLLs + EXEs (flach)
│   └── prowb\                   # ProWB-Builder (separat)
│       └── prowb.exe
├── lib\                         # Import-Libs (flach)
│
├── src\
│   ├── prophysics\              # Kernel (12 Module + 6 Header)
│   ├── sdk\                     # SDK-Interface
│   ├── test\                    # Test-Module (18 + 2 Example)
│   └── prowb\                   # Web-Docs-Builder (2 Module + 2 Header)
│
├── tools\                       # pro_run + Test-Runner (PS1 + CMD)
├── build\                       # Build-Infrastruktur
│   ├── main\                    # Master-Orchestrierung
│   ├── prophysics\              # Kernel-Makefile
│   ├── sdk\                     # SDK-Makefile
│   ├── test\                    # Test-Makefile
│   └── prowb\                   # ProWB-Makefile
├── docs\                        # Dokumentation
│   ├── project\                 # Roadmap, Module, API
│   ├── test\                    # Testkatalog + Runner
│   ├── build\                   # Build-Doku
│   ├── physics\                 # Physik-Übersicht
│   └── web\                     # Web-Docs-Quelle (Manifest + Assets)
├── python\                      # analysis.py (CSV-Auswertung)
└── out\                         # Export-Ziel
    ├── exe\
    ├── sdk\
    ├── kit\
    └── web\                     # Web-Docs (ProWB-Output)
```

**Prinzipien:**

- `bin\` und `lib\` flach (Windows-Loader)
- `bin\prowb\` separat (ProWB ist ein Build-Tool, kein Runtime-Artefakt)
- `.c` direkt, `.h` in `header\`
- Keine OBJ-Reste (cleanup nach Link)
- Test-Runner und `pro_run` in `tools\`, nicht in `bin\`
- `docs\web\` ist Konfiguration + Layout, keine MD-Kopien
- `out\web\` wird vom CI deployt (GitHub Pages)

---

## 8. Physikalische Grundlagen (Kurzfassung)

### 8.1 — Amplituden-Basis

Jeder Gitterknoten trägt einen 8-dimensionalen komplexen Vektor in Q31:

| Index | Zustand |
|---|---|
| 0 | UR_NEUTRAL |
| 1 | UR_POSITRON_CW |
| 2 | UR_POSITRON_CCW |
| 3 | UR_NEGATRON_CW |
| 4 | UR_NEGATRON_CCW |
| 5 | UR_PHOTON |
| 6 | (reserviert) |
| 7 | (reserviert) |

### 8.2 — U5-Gewichte

```
w = (0, 1, 1, 4, 4, 5, 0, 0)
```

Die U5-Invariante ist `Σ_k Σ_b w_b · |c_b(k)|²`.

### 8.3 — Erweiterungen (Schicht 2)

| Erweiterung | Basis | Aktivierung |
|---|---|---|
| Spin-1/2 | {CW, CCW} | `reserved_gating` Bit 0 |
| Dirac | Basis 1–4 | `reserved_gating` Bit 1 |
| SU(2) kinematisch | `ProEdge.su2_*` | `su2_active` |
| SU(2) Dynamik | `ProEdge.su2_E_*` | `su2_dynamics_active` |

### 8.4 — Numerische Präzision

| Typ | Skala | Anwendung |
|---|---|---|
| Q31 | 2³¹ (int32) | Amplituden |
| Q30 | 2³⁰ (int32) | SU(2)-Links (int64-Overflow-Schutz) |

---

## 9. Projekt-Regeln (R1–R7)

Unverhandelbare Invarianten. Keine Etappe darf sie brechen.

| Regel | Bedeutung |
|---|---|
| **R1** | Kein `div`/`mod` im Hotpath |
| **R2** | Kein `malloc`/`calloc` im Hotpath |
| **R3** | U5-Invariante bleibt erhalten |
| **R4** | Unitäre Dynamik |
| **R5** | Keine stillen API-Brüche |
| **R6** | Jede Etappe endet mit einem Test |
| **R7** | Keine Etappe ändert bestehende Pfade |

Details in `docs\project\Project.md` §2.

---

## 10. Was ProPhysics nicht ist

- **Keine Quantengravitation**
- **Keine Simulation echter QCD**
- **Kein Kontinuumslimes**
- **Kein Bell-Bruch** (S > 2 wäre Superdeterminismus-Loop)
- **Keine Behauptung, dass die Urregeln „wahr" sind**
- **Keine Alternative zu etablierten Lattice-QFT-Codes** (ITensor, QDP++, MILC)
- **Keine publizierte Physik** (Stand 2026-09-28; Preprint in Vorbereitung)

---

## 11. Publikations-Status

**Aktuell:** Kein Paper eingereicht.

**Kandidaten für Preprint:**

1. **Methodik:** „Quaternion-valued edges on a signed-permutation
   lattice: an exactly gauge-invariant SU(2) toy model"
   — Ziel: arXiv:hep-lat

2. **Überblick:** „A computational exploration of quantum structures
   from discrete signed permutations" — Ziel: arXiv:quant-ph

3. **Software:** ProPhysics als C99-Kernel mit deterministischer
   Q31-Arithmetik und externem V&V-Anker
   — Ziel: CPC oder JOSS (in Erwägung)

**Was für ein Paper noch fehlt:**

- Zwei Observablen (Creutz-Ratio) für echte β-Funktion
- Größere Gitter (dim ≥ 128)
- Vergleich mit etablierten Lattice-QCD-Werten
- Benchmark gegen ITensor / QuSpin / Qiskit

---

## 12. Kontakt und Mitwirkung

**Repository:** https://github.com/onkel83/prophysics
**Issue-Tracker:** https://github.com/onkel83/prophysics/issues
**Lizenz:** siehe `LICENSE.md`

**Beiträge willkommen.** Vor dem ersten Commit bitte lesen:

- `CONTRIBUTING.md` — Beitragenden-Anleitung
- `docs\project\Project.md` §2 (Projekt-Regeln)
- `docs\test\ProPhysics_Testkatalog.md` (Test-Philosophie)
- `docs\build\pro_run.md` (Build-Workflow)

**Schnelleinstieg für Mitwirkende:**

```cmd
git clone https://github.com/onkel83/prophysics.git
cd prophysics

:: Visual Studio Developer Prompt öffnen
cd tools
pro_run build            :: alles bauen
pro_run test -Prio 1     :: 12/12 PASS in ~5 s
```

---

## 13. Versionierung

**Kernel-Version** (`src\prophysics\header\ProPhysics_Version.h`):

| Version | Bedeutung |
|---|---|
| `1.23.0` | **jetzt** — Phase 1, Etappe 23, validierter Kernel |
| `2.0.0` | Phase 2, „komplette QM" (nach Etappe 24–27) |
| `3.0.0` | Phase 3, Makrophysik (nach M1–M3) |

**Changelog-Version:** `1.23.11` (Konsolidierungs-Serie abgeschlossen).

**Doc-Versionen** sind unabhängig und niedriger nummeriert
(z. B. `Project.md` v1.x).

**Regel:** Kernel-Version springt **nur** bei physikalischem
Paradigmenwechsel (Phase). PATCH-Sprünge innerhalb einer Etappe
ändern die Kernel-Version **nicht**.

Details siehe `docs\project\VERSIONING.md`.

---

![CI](https://github.com/onkel83/prophysics/actions/workflows/ci.yml/badge.svg)
![Web-Docs](https://github.com/onkel83/prophysics/actions/workflows/web-docs.yml/badge.svg)

---

**Ende README v1.1.**