# ProPhysics — Roadmap / Projekt-Dokumentation

**Datei:** `docs/project/Project.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-27 nach Etappe 23b (Creutz-Ratio-Konsistenz-Test,
            V&V-Anker-Rücknahme) und Konsolidierungs-Serie
            1.23.1–1.23.13. Prio-All 45/45, Compiler-Warnungen 0,
            Web-Docs publizierbar.
**Nächster Schritt:** Etappe 24 — Euklidisches Pfadintegral

---

## §0 — Zweck dieses Dokuments

1. **Projekt-Struktur** — Ordner, Dokumentation, Build-Skripte.
2. **Was wir haben** — abgeschlossene Etappen 1–23b, mit quantitativen Belegen.
3. **Was wir glauben** — die Ontologie in 5 Ur-Regeln und 10 Thesen.
4. **Was fehlt** — offene Fähigkeitslücken bis „komplette QM" und bis Makrophysik.
5. **Unsere Regeln** — die Projekt-Invarianten, die keine Etappe brechen darf.
6. **Die Roadmap** — Etappen 24–27 und M1–M3.
7. **Was wir bewusst nicht behaupten** — die Grenzen des Anspruchs.

**Neu in dieser Fassung (Etappe 23b):** Der V&V-Anker aus Etappe 23
(„0,08 % gegen `I₂(2)/I₁(2)`") wurde **zurückgenommen**. Ursache war
ein latenter Plaquette-Konjugations-Bug, der in Patch `1.23.10`
korrigiert wurde und `u_plaq(β=2, dim=64)` von `0,283270` auf
`0,272552` verschob (`⟨P⟩ = 0,4549`, ca. 5 % Abweichung zur
Ein-Plaquette-Approximation — erwartete Multi-Loop-Korrektur). An
die Stelle des Einzelobservablen-Ankers tritt der **Creutz-Ratio-
Konsistenz-Test**, der drei Loop-Größen (`W(1,1)`, `W(2,1)`,
`W(2,2)`) gegeneinander prüft und damit den Metropolis-Sampler
auf physikalische Konsistenz statt auf einen einzelnen Skalar
verifiziert. Bis Etappe 22b waren alle Tests **relativ**
(Konsistenz, Selbsterhaltung); Etappe 23 lieferte den ersten
**absoluten** Vergleich mit publizierter Lattice-QCD-Physik;
Etappe 23b zeigt die Grenze dieser Validierung und ersetzt sie
durch ein robusteres Konsistenzkriterium. Zusätzlich wurden in der
Konsolidierungs-Serie `1.23.1`–`1.23.13` alle Kernel-Module auf ein
einheitliches Schema gebracht, der Patch 1.23.7 vollständig
nachgeholt (Backlog B7), zwei SU(2)-Konjugations-Bugs gefixt
(Backward-Staple in `1.23.7`, Forward-Plaquette in `1.23.10`), die
Compiler-Warnungen auf 0 reduziert, der ProWB-Web-Docs-Builder
integriert (`1.23.11`) und die CI-Dokumentation konsolidiert
(`1.23.12`). Siehe `CHANGELOG.md` §2.5 für die Abbildung alter auf
neue Versionen (`3.0.0` → `1.23.0`).

---

## §1 — Projekt-Struktur

### §1.1 — Ordner-Layout

```
ProPhysics\
├── BUILD_INFO.txt
├── bin\                        (DLLs, EXEs — flach, ohne Runner)
│   └── prowb\                  (ProWB-Builder-EXE, separat)
├── lib\                        (Import-Libs)
├── src\
│   ├── prophysics\             (13 Kernel-Module + 6 Header)
│   ├── sdk\                    (pro_sdk_interface.c + Header)
│   ├── test\                   (alpha_test_*.c + 2 Example-Tests)
│   └── prowb\                  (Web-Docs-Builder, 2 Module + 2 Header)
├── tools\                      (Test-Runner: .ps1 + .cmd)
├── build\                      (main / prophysics / sdk / test / prowb)
├── docs\
│   ├── build\                  (Build-Doku)
│   ├── project\                (Roadmap, Module, API, Config)
│   ├── physics\                (Physik-Übersicht)
│   ├── test\                   (Testkatalog, Runner, Baseline)
│   └── web\                    (Web-Docs-Quelle: Manifest + Assets)
├── python\
└── out\                        (Export-Ziel + out\web)
```

**Prinzipien:**

| Ordner | Regel |
|---|---|
| `bin\` | flach; nur DLLs + EXEs. Kein Code, kein Runner. |
| `bin\prowb\` | separat; ProWB ist ein Build-Tool, kein Runtime-Artefakt. |
| `lib\` | flach; nur Import-Libs. |
| `src\<modul>\` | `.c` direkt, `.h` in `header\`. |
| `tools\` | Skripte, die nicht zum Kernel-Build gehören (Test-Runner). |
| `build\<modul>\` | ein Makefile pro Modul, aus seinem Ordner aufgerufen. |
| `docs\<gebiet>\` | Markdown, thematisch getrennt. |
| `docs\web\` | **Konfiguration + Layout**, keine MD-Kopien. Manifest verweist auf kanonische Pfade. |
| `out\` | Laufzeit-Erzeugnis, nicht versioniert. |
| `out\web\` | ProWB-Output; wird vom CI deployt. |

### §1.2 — Dokument-Struktur

**Projekt-Dokumente (`docs/project/`):**

| Datei | Inhalt |
|---|---|
| `Project.md` | dieses Dokument (Roadmap, Ontologie, Historie) |
| `ProPhysics_API.md` | vollständige Funktions-Referenz |
| `ProPhysics_Differentiators.md` | Abgrenzung zu anderen Frameworks |
| `ProPhysics_VersionRegistry.md` | Versionen aller Dateien |
| `CONFIG.md` | Compile-Time-Konstanten |
| `VERSIONING.md` | Etappen-Versionierungs-Konzept |

**Modul-Referenzen (`docs/project/`):**

| Datei | Modul |
|---|---|
| `Amp.md` | `ProPhysics_Amp.c` |
| `Core.md` | `ProPhysics_Core.c` |
| `Density.md` | `ProPhysics_Density.c` |
| `Dirac.md` | `ProPhysics_Dirac.c` |
| `EPR.md` | `ProPhysics_EPR.c` |
| `Fock.md` | `ProPhysics_Fock.c` |
| `Gauge.md` | `ProPhysics_Gauge.c` |
| `Observer.md` | `ProPhysics_Observer.c` |
| `Shared.md` | `ProPhysics_Shared.c` |
| `SU2.md` | `ProPhysics_SU2.c` (Version 1.1 seit 23b) |
| `SU2_Dynamics.md` | `ProPhysics_SU2_Dynamics.c` |
| `Tensor.md` | `ProPhysics_Tensor.c` |

**Test-Dokumente (`docs/test/`):**

| Datei | Inhalt |
|---|---|
| `ProPhysics_Testkatalog.md` | alle 45 Tests |
| `run_alpha_tests.md` | Test-Runner |
| `BASELINE.md` | Test-Baseline (Kurzfassung) |
| `WRITING_TESTS.md` | Anleitung zum Test-Schreiben |

**Build-Dokumente (`docs/build/`):**

| Datei | Inhalt |
|---|---|
| `BUILD_SCRIPT.md` | Build-Übersicht |
| `pro_run.md` | zentraler Einstiegspunkt |
| `web-docs-ci.md` | Web-Docs CI-Workflow (GitHub Actions) |
| `ci.md` | Standard-CI-Workflow |
| `alpha-nightly.md` | Alpha-Nightly-Workflow |
| `main/Makefile.md` | Master-Makefile |
| `prophysics/Makefile.md` | Kernel-Build |
| `sdk/Makefile.md` | SDK-Interface-Build |
| `test/Makefile.md` | Test-Build |
| `prowb/Makefile.md` | ProWB-Makefile |
| `helper/build.md` | `build.ps1` / `build.cmd` |
| `helper/export.md` | `export.ps1` / `export.cmd` |
| `helper/write_build_info.md` | `write_build_info.ps1` |
| `examples/BUILD_INFO.txt` | Beispiel-Datei |

**Web-Docs-Quelle (`docs/web/`):**

| Datei | Inhalt |
|---|---|
| `manifest.txt` | 46 Einträge, 6 Sektionen |
| `README.md` | Manifest-Pflege, Sektionen, Templates, Themes |
| `src/parts/*.html` | Header, Nav, Footer |
| `src/css/*.css` | 8 CSS-Dateien (Reset, Vars, Layout, Themes) |
| `src/js/*.js` | Router, Theme-Switch, Daten-Bridge |
| `src/views/*.html` | 10 Views (Home, 6 Sektionen, Utility) |

**ProWB-Dokumentation (`src/prowb/`):**

| Datei | Inhalt |
|---|---|
| `README.md` | Builder-Referenz, Manifest-Format, CLI, Fallstricke |

### §1.3 — Build-Skripte

**Master (in `build\main\`):** `Makefile.nmake`, `build.ps1`/`.cmd`,
`export.ps1`/`.cmd`, `write_build_info.ps1`.

**Sub-Makefiles:** `build\prophysics\Makefile.nmake`,
`build\sdk\Makefile.sdk.nmake`, `build\test\Makefile.nmake`,
`build\prowb\Makefile.nmake`.

**Reihenfolge:** `prophysics` → `sdk` → `test` (Kernel-Kette);
`prowb` läuft im Master-`all` **nach** `test`, ist aber eigenständig
baubar.

**Test-Runner (in `tools\`):** `run_alpha_tests.ps1` + `.cmd`
(Version 1.0.2 seit 23b), `pro_run.ps1` + `.cmd`.
`-ExeDir` Default = `<repo>\bin`, `-LogDir` Default = `<ExeDir>\logs`.

**Web-Docs-Quelle (in `docs\web\`):** `manifest.txt`,
`src\parts\*.html`, `src\css\*.css`, `src\js\*.js`,
`src\views\*.html`.

**Web-Docs-CI:** `.github\workflows\web-docs.yml` — baut ProWB +
Web-Docs, deployt auf GitHub Pages.

**Standard-CI:** `.github\workflows\ci.yml` — Prio 1, 6, 7 +
`SU2-Wilson-Loop` auf jedem Push/PR.

**Alpha-Nightly:** `.github\workflows\alpha-nightly.yml` — manueller
Trigger, Scope-Auswahl.

### §1.4 — Typischer Aufruf

```cmd
cd build\main
build.cmd                       :: alles bauen (kernel + sdk + test + prowb)
build.cmd -Mode prophysics -Rebuild

cd ..\..\tools
run_alpha_tests.cmd -Prio all
```

**Mit Web-Docs:**

```cmd
cd tools
pro_run all                     :: build → test → export
pro_run web                     :: Web-Docs bauen
```

---

## §2 — Projekt-Regeln (unverhandelbar)

### R1 — Kein `div` / `mod` im Hotpath

Erlaubt: Bit-Shift, Bit-Mask bei Zweierpotenzen, `pro_ensure_grid_shift`.
Verboten: `k % dim`, `k / dim` in Apply-/Tick-Funktionen.
`dim` muss Zweierpotenz sein.

**Neu in 23b:** `ProPhysics_Wilson_Loop_Average` nutzt ausschließlich
`grid_dim_mask` und `grid_dim_shift` für die Koordinaten-Arithmetik.

### R2 — Kein `malloc` / `calloc` / `free` im Hotpath

Erlaubt: `ProPhysics_Initialize`, `ProPhysics_Free`, `*_Create`,
Test-Setup, dokumentierte Diagnose-Ausnahmen.
Verboten: `Apply_*`, `*_Tick`, pro Knoten.

**Ausnahme für ProWB:** ProWB ist **nicht** im Kernel-Hotpath. Der
Builder nutzt `malloc` für den MD-Parser und Template-Expansion. Das
ist R2-konform, weil ProWB außerhalb des Kernels lebt.

**Neu in 23b:** `ProPhysics_Wilson_Loop_Average` ist Stack-only.

### R3 — U5-Invariante bleibt erhalten

Σ_k Σ_b w_b · |c_b(k)|² mit w = (0,1,1,4,4,5,0,0).
Drift < 5 % über 2000 Ticks; in der Praxis < 10⁻⁴.

**Nachweise:**

| Etappe | Kontext | Drift |
|---|---|---|
| 17 | 32³, 2000 Ticks | 1,81e-09 |
| 18e | 1000 Ticks, shared | 3,59e-08 |
| 19 | 200 Ticks, ohne Spin | 0,0 |
| 21 | 100 Ticks, ohne Dirac | 0,0 |
| 21 | 80 Ticks, Dirac-Pfad | ≤ 8e-09 |
| 22 | 100 Ticks, `su2_active == 0` | 1,24e-09 |
| 22 | 500 Ticks, aktive SU(2)-Links | 1,57e-09 |
| 22b | 100 Ticks, `su2_dynamics_active == 0` | 0,0 |
| 22b | 100 Ticks, aktive Leapfrog-Dynamik | 0,0 |
| **23b** | **Creutz-Ratio-Lauf, read-only** | **0,0 (keine amp_grid-Mutation)** |

### R4 — Unitäre Dynamik

Jeder Transport-, Wave-, Context-, Gauge-Schritt unitär bis auf
Q31-Rundung.

### R5 — Keine stillen API-Brüche

Bei Änderung einer API-Signatur wird der betroffene Test mit angepasst.
Für **Refactoring 22**: alle alten SDK-Symbole bleiben funktional
(`ProPhysics_SDK_Execute_Plastizitaet_Tick` delegiert jetzt an
`ProPhysics_Tick`; `ProPhysics_ScientificRuleCallback` ist Alias
für `ProPhysics_RuleCallback`). Für die **Konsolidierungs-Serie
`1.23.1`–`1.23.13`**: alle Änderungen sind rein intern (Auslagerung
in `static`-Helfer, Header-Konsolidierung, Modul-Docs, Warnungs-Fix,
zwei Konjugations-Fixes), keine Signaturänderung.
**Für `1.23.11` (ProWB):** `prowb_build()` (Legacy) bleibt funktional;
`prowb_build_from_manifest()` ist **additiv**.
**Für `1.23.13` (Creutz-Ratio):**
`ProPhysics_Wilson_Loop_Average` ist **additiv**; keine bestehende
Signatur geändert. Alle Kernel-APIs unverändert.

### R6 — Jede Etappe endet mit einem Test

`test_<etappe>_<aspekt>`, registriert in `alpha_test_main.c` und
`tools\run_alpha_tests.ps1`. Etappen-Test grün = alle bisherigen
Tests weiter grün.

**23b:** `test_creutz_ratio(bool full_dims)`. Zwei Katalog-Einträge
(`Creutz-Ratio` Fast-Modus, `Creutz-Ratio-Full` Nightly-Modus).

**Ausnahme `1.23.11` (ProWB):** ProWB ist ein Build-Tool, kein
Kernel-Modul. Es hat keine Tests im Kernel-Sinne. Verifikation läuft
über `pro_run web` + Browser-Sichtprüfung + CI-Workflow.

### R7 — Keine Etappe ändert bestehende Pfade

Neue Funktionalität in parallelen Funktionen oder als Dispatch über
`grid_ndim` / `*_active`.

**Konformitäten:**

| Etappe | Flag | Pfad bei 0 |
|---|---|---|
| 18e | `shared.active` | bit-identisch |
| 19 | `reserved_gating & 0x01` | bit-identisch |
| 21 | `dirac_active` | bit-identisch |
| 22 | `su2_active` | bit-identisch |
| 22b | `su2_dynamics_active` | bit-identisch |
| 23 | nur lesende API-Erweiterung | bit-identisch |
| 1.23.1–1.23.10 | reine Konsolidierung | bit-identisch (T16 verbessert) |
| 1.23.11 | neue Tool-Komponente | Kernel bit-identisch |
| 1.23.12 | CI-Doku | Kernel bit-identisch |
| **1.23.13 (23b)** | **additive read-only Funktion** | **bit-identisch** (`Wilson_Loop_Average` greift nur auf bestehende `ProEdge`-Felder lesend zu) |

---

## §3 — Die Ur-Regeln

```
U1:  U = {0,1} × {00,01,10,11}          (8 Basis-Zustände)
U2:  A : G → U ∪ G, G ⊆ ℤ³              (Interaktion/Beobachtung pro Tick)
U3:  S_{t+1} = f(S_t)                   (Tick-Iteration)
U4:  Ψ(x,y) ⟺ A(x) = A(y)               (shared reference)
U5:  Σ_{x∈G} A_t(x) = Konstante         (Bit-Erhaltung)
```

### §3.1 — Lesart

- **U1** = Zustandsmenge. `PRO_AMP_BASIS_SIZE = 8`.
- **U2** = prozeduraler Operator pro Tick.
- **U3** = Reihenfolge der Interaktionen.
- **U4** = Pointer-Identität (shared reference).
- **U5** = Bit-Erhaltung.

### §3.2 — Aspekte

| Regel | Aspekt |
|---|---|
| U1 | Was existiert |
| U2 | Was passiert |
| U3 | In welcher Ordnung |
| U4 | Referenz-Struktur |
| U5 | Invariante |

### §3.3 — Fundament-Abschluss (Etappe 18e)

| Ur-Regel | Kernfunktion | Test | Status |
|---|---|---|---|
| U1 | `PRO_AMP_BASIS_SIZE = 8` | `test_born_rule` | ✅ |
| U2 | `Apply_Edge_Transport`, `Apply_Context_Tick`, `Apply_Wave_Step` | `test_3d_invariance` | ✅ |
| U3 | `ProPhysics_Tick` (seit Refactoring 22) | `test_3d_invariance` | ✅ |
| U4 | `Entangle_Nodes`, `Shared_Tick_Reps` | `test_shared_reference` | ✅ |
| U5 | `Measure_Amp_Invariant` | `test_amp_invariant` | ✅ |

### §3.4 — Schicht-2-Erweiterungen

| Erweiterung | Etappe | Basiszustände | Flag | Kernfunktion |
|---|---|---|---|---|
| Spin-1/2 | 19 | {CW, CCW} | `reserved_gating` Bit 0 | `Apply_Nonlinear_Phase_Step_Spin` |
| Dirac | 21 | Basis 1–4 | `reserved_gating` Bit 1 | `Apply_Dirac_Step`, `Apply_Dirac_Mass_Term` |
| SU(2)-Eichfeld (kinematisch) | 22 | Basis 1–4 (Links in `ProEdge`) | `su2_active` | `Set_Edge_SU2`, `Wilson_Loop_SU2`, `Apply_Local_SU2_Gauge` |
| SU(2)-Link-Dynamik (Leapfrog) | 22b | `ProEdge.su2_E_*` | `su2_dynamics_active` | `Apply_SU2_Tick`, `SU2_Plaquette_Action`, `SU2_Total_Energy` |
| SU(2)-Metropolis (Thermostat) | 23 | read-only | kein Flag | `SU2_Link_Plaquette_Sum` (Vorbereitung für Metropolis im Test) |
| **SU(2)-Loop-Mittelung** | **23b** | **read-only** | **`su2_active`** | **`Wilson_Loop_Average`** |

Alle sechs sind Struktur-Erweiterungen, keine Reduktion auf die reine
Ur-Grammatik.

### §3.5 — Werkzeug-Erweiterungen (nicht Kernel)

| Erweiterung | Etappe | Zweck |
|---|---|---|
| ProWB-Builder | 1.23.11 | Web-Docs aus Markdown generieren |
| Web-Docs | 1.23.11 | Statisches Portal (`out\web\index.html`) |
| Web-Docs-CI | 1.23.11 | Auto-Deploy auf GitHub Pages |
| Standard-CI | 1.23.9 | Prio 1/6/7 + SU2-Wilson-Loop auf Push |
| Alpha-Nightly | 1.23.12 | Manueller Langlauf (Prio 5 + RC) |

Diese Erweiterungen sind **nicht Teil der Physik**. Sie ändern keinen
Kernel-Pfad und sind R7-konform.

---

## §4 — Die Ontologie (10 Thesen)

| # | These |
|---|---|
| T1 | Grid + Graph fundamental |
| T2 | Zeit = Operationsfolge |
| T3 | c = Lieb-Robinson-Rate |
| T4 | Verschränkung via Graph-Kanten (ER=EPR) |
| T5 | Bandbreiten-Budget pro Kante |
| T6' | Energie = Informationsgehalt |
| T7' | γ = √(1 − v²/c²) aus Tick-Unitariät |
| T8 | Position kumuliert aus Operationen |
| T9 | Hawking = Hash (Sättigung) |
| T10 | Grid = Projektion des Graphen |

**Beiträge pro Etappe zu T4:**

- 18c/18e — U4 als Union-Find-Klasse + Klassen-Tick.
- 19 — Singlet-Antikorrelation via Spin-Flip.
- 21 — Dirac-Spinor als 4-Komponenten-Erweiterung.
- 22 — SU(2)-Link-Struktur auf `ProEdge`; nicht-abelsche Korrelation.
- 22b — Links werden dynamisch; Yang-Mills-Feld als aktive Struktur.
- 23 — Links thermalisieren; kanonische Verteilung als Zustand.
- 23b — **Loop-Konsistenz über drei Wilson-Schleifen**; die
  Konfigurations-Verteilung ist mit der Theorie konsistent.

---

## §5 — Beobachter und Subgraph

**Kernaussage:** Beobachter = endlicher Subgraph.

**Regeln:**

1. Beobachter sind endliche Subgraphen.
2. Operationen sind Kanten über die Grenze.
3. Verschränkung ist Struktur, kein Ereignis.
4. Zeit ist subgraph-relativ.

**Beiträge:**

| Etappe | Subgraph-Erweiterung |
|---|---|
| 18e | Union-Find-Klasse |
| 19 | Singlet-Klasse mit Spin-Aspekt |
| 21 | Dirac-Klasse mit Chiralitäts-Achse |
| 22 | Klassen tragen SU(2)-Link-Struktur |
| 22b | Klassen haben dynamische Link-Felder |
| **23b** | **Klassen tragen messbare Loop-Statistik** |

---

## §6 — Was wir haben (Etappen 1–23b)

### §6.1 — Kernel-Umfang

| Kategorie | Umfang |
|---|---|
| Kernel-Module | 12 `.c` (+ `SU2_Dynamics.c` = 13) |
| Kernel-Header | 6 `.h` |
| Test-Module | 19 `.c` (+ 1 Header, seit 23b) |
| SDK | 1 `.c` + 1 `.h` |
| ProWB-Module | 2 `.c` + 2 `.h` (Build-Tool, kein Kernel) |
| Codezeilen | ~36 000 LOC gesamt (~800 LOC ProWB zusätzlich) |
| Datenstrukturen | `ProUniverse`, `ProNode`, `ProRegister`, `ProAmpVector`, `ProEdge`, `ProSharedInfo`, Tensor/Fock/Density-Trilogie |
| Dimensionen | 2D (dim ∈ {16,32,64,128}), 3D (dim ∈ {16,32,64,128}) |
| Amplituden-Basis | 8-dim |
| Dirac-Komponenten | 4 |
| SU(2)-Link | Quaternion (a, b), Skala 2³⁰ |
| SU(2)-Feld E | 4 × int32 pro Kante (Skala 2³⁰) |
| Numerische Basis | Q31 |

### §6.2 — Funktionsumfang

| Bereich | Status |
|---|---|
| U1–U6-Invarianten | ✅ |
| Transport (2D/3D) | ✅ exakt unitär |
| Gauge U(1) | ✅ |
| Gauge SU(2), kinematisch | ✅ seit 22 |
| Gauge SU(2), dynamisch (Leapfrog) | ✅ seit 22b |
| Gauge SU(2), thermalisiert (Metropolis) | ✅ seit 23 |
| Gauge SU(2), Loop-Konsistenz (Creutz-Ratio) | ✅ seit 23b |
| Observer | ✅ |
| Verschränkung (Tensor, CHSH) | ✅ |
| U4 Shared Reference | ✅ seit 18e |
| Spin-1/2 | ✅ seit 19 |
| Dirac + α^i | ✅ seit 21 |
| Fermionen (Jordan-Wigner) | ✅ |
| Fock 8-Moden | ✅ |
| Dichte-Trilogie + Lindblad | ✅ |
| Soliton / Lorentz | ✅ |
| 3D-Torus + Bloch | ✅ |
| Phase Plate | ✅ |
| Coulomb-Field 3D | ✅ |
| Imaginaerzeit-Prep | ✅ |
| Wilson-Action-Plaquette-Validierung | ✅ seit 23 (SPA-Referenz ~5 %, siehe §6.3 Beleg 13) |
| **Wilson-Loop-Average (Kernel-Funktion)** | **✅ seit 23b** |
| **Creutz-Ratio-Konsistenz-Test** | **✅ seit 23b** |
| Compiler-Warnungen auf `/W4` (Kernel) | ✅ 0 (seit 1.23.10) |
| Web-Docs-Builder (ProWB) | ✅ seit 1.23.11 |
| Web-Docs-CI | ✅ seit 1.23.11 |
| Standard-CI | ✅ seit 1.23.9 |
| Alpha-Nightly | ✅ seit 1.23.12 |

### §6.3 — Numerisch hart belegte Resultate

| # | Resultat | Wert |
|---|---|---|
| 1 | Bloch-Dispersion 2D | rel_dev ≤ 5,22e-06 |
| 2 | Bloch-Dispersion 3D | rel_dev ≤ 2,57e-06 |
| 3 | Unitärer Transport 2D/3D | Drift ≈ 1e-09 |
| 4 | Jordan-Wigner | 16/16 exakt |
| 5 | Bell-CHSH (Kollaps/Native/Graph) | 2,846 / 2,000 / 2,814 |
| 6 | Lindblad | Abweichung < 3e-03 |
| 7 | Coulomb/Hydrogen | ratio = 0,400 (dim 32); rel_dev = 0,296 (dim 64) |
| 8 | U4 Shared Reference | 29/29 |
| 9 | Spin-1/2 | K1 8,12e-10; K2 exakt; K3 = 0 |
| 10 | Dirac | γ-Algebra 9,31e-10; Masse 2,4e-07 |
| 11 | SU(2)-Eichfeld (kinematisch) | 14/14 + Kernel-Algebra 4,45e-10 |
| 12 | SU(2)-Link-Dynamik (Leapfrog) | 18/18 + KA = 19/19 |
| 13 | SU(2)-Metropolis / SPA-Referenz | **ca. 5 % Abweichung (Rücknahme des 0,08-%-Ankers, siehe unten)** |
| **14** | **Creutz-Ratio-Konsistenz (23b)** | **B1/B2/B3 PASS, `W(1,1)(β=2) ≈ 0,454`** |

**Beleg 13 — SU(2)-Metropolis / SPA-Referenz (Etappe 23, revidiert 23b):**

Der ursprüngliche V&V-Anker (0,08 % Abweichung zur
Ein-Plaquette-Approximation `I₂(2)/I₁(2) = 0,43313`) beruhte auf
`u_plaq(β=2, dim=64) = 0,283270`. Nach dem Plaquette-Konjugations-
Fix in `1.23.10` ist der tatsächliche Wert:

| Größe | Wert |
|---|---|
| `u_plaq(β=2, dim=64)` | **0,272552** |
| `⟨P⟩ = 1 − 2·u_plaq` | **0,454896** |
| Referenz `I₂(2)/I₁(2)` | 0,43313 |
| Abweichung | **ca. 5 %** |

Die ~5 % sind im Rahmen der erwarteten Multi-Loop-Korrekturen zur
Ein-Plaquette-Approximation. **Dies ist kein V&V-Anker mehr.**
Die Validierung der **Sampler-Korrektheit** läuft ab `1.23.13`
über die Creutz-Ratio-Konsistenz (siehe Beleg 14).

**Beleg 14 — Creutz-Ratio-Konsistenz (Etappe 23b):**

Der Metropolis-Sampler wird über drei Loop-Größen geprüft:

```
χ(2,2) = −ln( W(2,2)·W(1,1) / W(2,1)² )
```

mit `W(m,n) = ⟨Re Tr(W_C)/2⟩` gemittelt über alle `m × n`-Loops der
drei Ebenen xy, xz, yz eines 3D-Torus.

| β | dim=16 `χ(2,2)` | dim=32 `χ(2,2)` |
|---|---|---|
| 1,00 | 1,04844 ± 0,19338 | 1,51631 ± 0,08838 |
| 2,00 | 0,78591 ± 0,01758 | 0,77838 ± 0,00518 |
| 4,00 | 0,24010 ± 0,00281 | 0,23930 ± 0,00078 |

**Prüf-Ergebnisse:**

| Kriterium | Ergebnis |
|---|---|
| B1 (χ > 0 für alle dim,β) | **PASS** |
| B2 (χ monoton fallend in β pro dim) | **PASS** |
| B3 (χ konsistent über dim, 3σ) | **PASS** |

**Kreuzvalidierung gegen Running-Coupling:**

`W(1,1)(β=2, dim=32) = 0,454` vs.
`RC-⟨P⟩(β=2, dim=32) = 1 − 2·u_plaq ≈ 0,433`. Übereinstimmung auf
**~4,8 %** — im Rahmen der erwarteten Multi-Loop-Korrektur.
Zwei unabhängige Observablen messen dieselbe physikalische Größe.

**Bedeutung:** Die Wilson-Action-Normierung, der Metropolis-
Akzeptanzschritt, die Q30-Quaternion-Multiplikation und die
Vorwärts/Rückwärts-Adjungierung des Loops sind konsistent. Ein
falscher Sampler würde die drei Loop-Größen nicht in der erwarteten
Relation zueinander liefern.

**Grenze:** Die Creutz-Ratio ist ein **Konsistenz-Test**, kein
absoluter V&V-Anker. Die absolute Übereinstimmung mit der
SPA-Referenz bleibt bei ~5 % (Multi-Loop-Korrektur). Der FAST-Modus
(`dim ∈ {16, 32}`) läuft in der Prio-All-Regression; der FULL-Modus
(`dim ∈ {16, 32, 64, 128}`) ist ein Nightly-Kandidat.

### §6.4 — Was plausibel, aber nicht quantitativ belegt ist

- Born-Emergenz (Stichproben klein).
- Soliton-Klassifikation (phänomenologisch).
- Verschränkung in `amp_grid` ohne U4 (strukturell, nicht quantitativ).
- Dirac-Kontinuums-Dispersion ω² = k² + m² (braucht Renormierung, Etappe 24+).
- **Absolute SPA-Übereinstimmung** — der korrigierte Wert liegt bei
  ~5 %; die Abweichung ist im Rahmen der erwarteten Multi-Loop-
  Korrekturen, aber nicht quantitativ aufgelöst. Ein vollständiger
  RC-Neulauf für β≠2, dim=64 steht aus.

### §6.5 — Prio-all-Regression (2026-09-27 nach Etappe 23b)

| Prio | Thema | Tests | Status |
|---|---|---|---|
| 1 | 2D-Basis | 12 | 12/12 |
| 2 | Emergenz | 10 | 10/10 |
| 3 | Langlauf | 10 | 10/10 |
| 4 | 3D-Torus | 3 | 3/3 |
| 5 | Hydrogen + Shared-Ref + Tournament | 4 | 4/4 |
| 6 | Spin-1/2 | 1 | 1/1 |
| 7 | Dirac | 1 | 1/1 |
| 8 | SU(2)-Eichfeld + Link-Dynamik + Running-Coupling + Creutz-Ratio | 4 | 4/4 |
| **Gesamt** | | **45** | **45/45** |

**Prio-All-Laufzeit:** ~4 530 s (~75,5 min) mit `Creutz-Ratio`
(Fast). Der FAST-Modus fügt ~110 s hinzu.

**Laufzeit-Treiber:**
- Hydrogen-48 (~2 281 s = 50,4 %)
- Running-Coupling (~1 398 s = 30,9 %)
- Creutz-Ratio-Full (~2 400 s, **nur Nightly**)
- Alle anderen (~850 s = 18,7 %)

**CI-Empfehlung:**
- **Prio 1–4, 6, 7** + `SU2-Wilson-Loop` + optional `Creutz-Ratio`
  (~1,5–3 min) in normalen CI-Läufen.
- **`Running-Coupling`** und **`Creutz-Ratio-Full`** als Nightly-Jobs.
- **Prio 5** (Hydrogen-48) ebenfalls als Nightly-Job.
- **Web-Docs** in eigenem Workflow (`.github/workflows/web-docs.yml`).

**Konsolidierungs-Serie:** Die Patches `1.23.1`–`1.23.13` ändern
außer den zwei Konjugations-Fixes und der neuen Loop-Funktion keine
Physik. Alle numerischen Anker bleiben innerhalb ihrer Toleranzen.
T16 (Energie-Drift) verbessert sich durch die Fixes von `8,06e-03`
(`1.23.0`) über `2,44e-03` (`1.23.7`) auf `1,41e-03` (`1.23.10`).
Der RC-Rohwert `u_plaq(β=2, dim=64)` ändert sich von `0,283270`
auf `0,272552` (`1.23.10`).

**ProWB-Patch `1.23.11`:** Ändert **keine** Kernel-Funktion. Alle
Tests bleiben bit-identisch. Zusätzliche Verifikation: `pro_run web`
läuft fehlerfrei, `out\web\index.html` ~1,2 MB, alle 6 Sektionen
erreichbar, Themes funktionieren.

**CI-Doku `1.23.12`:** Rein dokumentarisch.

**Creutz-Ratio `1.23.13`:** Additive Kernel-Funktion
(`Wilson_Loop_Average`) und neuer Test (`alpha_test_creutz_ratio.c`).
Alle bestehenden Tests bleiben bit-identisch.

---

## §7 — Etappen 18–23b: Details

### §7.1 — Etappe 18: Coulomb/Hydrogen

- 18a: `Apply_Coulomb_Phase_Field_3D` (dim=32).
- 18b: Imaginaerzeit-Prep, `rel_dev = 0,139`.
- 18c: U4 Shared Reference (Union-Find), 23/23.
- 18d: dim=64, bestes Spektrum `rel_dev = 0,296`.
- 18e: Klassen-Tick, `θ_eff = θ · N_AB`, 29/29.

### §7.2 — Etappe 19: Spin-1/2

Neue API: `Entangle_Nodes_Singlet`, `Is_Spin_Flipped`,
`Get_Node_Spin_View`, `Apply_Nonlinear_Phase_Step_Spin`.
Flag Bit 0. 6/6.

### §7.3 — Etappe 21 + 21b: Dirac

Neue API: `Verify_Gamma_Algebra`, `Apply_Dirac_Mass_Term`,
`Apply_Dirac_Step`. Flag Bit 1, `dirac_active`.
6/6.

### §7.4 — Etappe 22: SU(2)-Eichfeld, kinematisch

**Design-Entscheidungen:**

| Frage | Antwort |
|---|---|
| Skala | 2³⁰ (int64-Overflow-Schutz) |
| `ProEdge`-Layout | 4 × int32 (`su2_a_re/ai`, `su2_b_re/bi`); 12 → 24 B |
| Wilson-Loop-Konvention | Vorwärts, `W(C) = U(p[0]→p[1])·…·U(p[n-1]→p[0])` |
| Aktivierung | implizit via `Set_Edge_SU2` |
| Gauge-Basis | Pauli (`PRO_SU2_BASIS_PAULI`) |

**Neue API (7 Funktionen):**

| Funktion | Wirkung |
|---|---|
| `ProPhysics_Set_Edge_SU2` | Link setzen (auto-aktiviert) |
| `ProPhysics_Set_Edge_SU2_AxisAngle` | Link via Achse-Winkel |
| `ProPhysics_Get_Edge_SU2` | Link lesen |
| `ProPhysics_Wilson_Loop_SU2` | Loop-Matrix |
| `ProPhysics_Wilson_Loop_SU2_Trace` | `Tr(W) = 2·Re(a)` |
| `ProPhysics_Apply_Local_SU2_Gauge` | `U → g(x)·U·g(y)†` |
| `ProPhysics_Verify_SU2_Quaternion` | Algebra-Check |

**Neues Modul:** `ProPhysics_SU2.c` (12. Kernel-Modul).

**Refactoring 22 (parallel):**

- Kernel-Tick (`ProPhysics_Tick`) jetzt in `ProPhysics_Core.c`.
- Cache-Aligned-Alloc (`pro_aligned_calloc`) für 8 Hot-Arrays.
- Zentrale Helfer in `Internal.h`.
- Konstanten zentral in `ProPhysics_Config.h`.
- Test-Runner von `bin\` nach `tools\`.
- EPR-Debug-Ring statt printf im Kernel-Tick.

**Test:** 14/14 + Kernel-Algebra PASS.

### §7.5 — Etappe 22b: SU(2)-Link-Dynamik (Leapfrog)

**Design-Entscheidungen:**

| Frage | Antwort |
|---|---|
| Algorithmus | Leapfrog (Stoermer-Verlet), klassisches Yang-Mills |
| Hamilton-Funktion | `H = S_plaq + ½ Σ_links |E|²` |
| Link-Update | `U ← exp(i · dt · E) · U` (exp-Map über Quaternion) |
| Kraft | su(2)-Projektion der Staple-Summe |
| Skala | 2³⁰, dt in Q15 |
| Aktivierung | explizit via `ProPhysics_Enable_SU2_Dynamics` |

**Neue API (8 Funktionen):**

| Funktion | Wirkung |
|---|---|
| `ProPhysics_Enable_SU2_Dynamics` | Dynamik anschalten |
| `ProPhysics_Disable_SU2_Dynamics` | Dynamik abschalten (R7) |
| `ProPhysics_Is_SU2_Dynamics_Active` | Status abfragen |
| `ProPhysics_Set_SU2_Yang_Mills` | Kopplung g² in Q15 |
| `ProPhysics_Apply_SU2_Tick` | Ein Leapfrog-Schritt |
| `ProPhysics_SU2_Plaquette_Action` | Globale Plaquette-Action |
| `ProPhysics_SU2_Total_Energy` | H = S_plaq + ½ Σ |E|² |
| `ProPhysics_SU2_Link_Plaquette_Sum` | Lokale Plaquette-Summe um Link (Metropolis) |

**`ProEdge`-Erweiterung:** 24 B → 40 B.

**Neues Modul:** `ProPhysics_SU2_Dynamics.c` (13. Kernel-Modul).

**Backward-Staple-Fix (Patch 1.23.7, in Etappe 23 verifiziert):**
Der Backward-Staple in `su2_force_on_link` hatte fehlende
`†`-Dagger auf `U_μ(x-ν)`. Der Fix reduzierte:
- T15: 2,02e-08 → **1,80e-08**
- T16: 8,06e-03 → **2,44e-03** (Faktor 3,3)

**Forward-Plaquette-Konjugations-Fix (Patch 1.23.10):**
In `su2_plaquette_action_at` wurden `l3br_n`/`l4br_n` berechnet,
aber `l3br`/`l4br` verwendet. Der Fix reduzierte T16 weiter von
2,44e-03 auf **1,41e-03** (Faktor 1,7). T11 (rein-imaginäres `b`)
bleibt unverändert, weil dort `l3br = l4br = 0`.

Die Energiedrift ist jetzt auf dem Niveau, das symplektische
O(Δ²)-Oszillation für `dt=500, g²=500` hergibt.

**Test:** 18/18 + Kernel-Algebra = 19/19 PASS.

### §7.6 — Etappe 23: SU(2)-Metropolis / Wilson-Action-Validierung

**Design-Entscheidungen:**

| Frage | Antwort |
|---|---|
| Algorithmus | Metropolis (kanonische Verteilung `exp(-β S_plaq)`) |
| Vorschlag | Gauss-Störung der Quaternion-Komponenten, Kernel renormiert via `Set_Edge_SU2` |
| ΔS-Berechnung | `Link_Plaquette_Sum` (Kernel, read-only) |
| Akzeptanz | `min(1, exp(-β ΔS))` |
| Observable | `u_plaq = ⟨S_plaq⟩ / (2·N_plaq)` |
| Sweep | Thermalisierung 200 Sweeps, Messung 300 Sweeps, 30 Bins |
| Test-Matrix | dim ∈ {16, 32, 64} × β ∈ {0.5, 1, 2, 4} |
| Speicherort | Test-Code (`alpha_test_running_coupling.c`), nicht Kernel |

**Kernel-Beitrag (1 neue Funktion):**

| Funktion | Wirkung |
|---|---|
| `ProPhysics_SU2_Link_Plaquette_Sum` | Summe der Wilson-Aktionen aller Plaquettes, die Link (x,μ) enthalten. Read-only, `O(1)` pro Link. |

**Design-Prinzip:** Der Kernel bekommt nur die **atomare Primitive**
(Wilson-Action einer Plaquette). Der **Algorithmus** (Metropolis-Sweep,
RNG, Akzeptanz, Binning) lebt im Test. Damit bleibt der Kernel frei
von Thermostat-Logik, und die Trennung „Physik-Engine vs. Sampling-
Layer" ist sauber.

**R7-Konformität:** Die neue Funktion ist read-only.

**Testergebnis: 43/43 PASS** (Stand 1.23.0, vor Bugfixes).

**Ergebnisse (dim=64, Stand 1.23.0 — durch Bugfix 1.23.10 überholt):**

| β | u_plaq (alt) | u_err | ⟨P⟩ (alt) |
|---|---|---|---|
| 0.50 | 0.438134 | 0.000025 | 0.123732 |
| 1.00 | 0.379875 | 0.000021 | 0.240250 |
| 2.00 | 0.283270 | 0.000027 | 0.433460 |
| 4.00 | 0.170344 | 0.000019 | 0.659312 |

**V&V-Anker (historisch, mit 23b zurückgenommen):**

| Größe | Wert | Abweichung |
|---|---|---|
| ⟨P⟩(β=2.0, dim=64) | 0.433460 ± 0.000054 | |
| Referenz I₂(2)/I₁(2) | 0.43313 | |
| **Abweichung** | | **0,08 %** |

**Nach Bugfix 1.23.10:** `u_plaq(β=2, dim=64) = 0,272552`, also
`⟨P⟩ = 0,454896`. Abweichung zur SPA-Referenz: **~5 %** (erwartete
Multi-Loop-Korrektur). Der 0,08-%-Anker wurde mit `1.23.13`
zurückgenommen (siehe §6.3).

**Was das Projekt ab jetzt ist:**
Der Kernel ist nicht mehr über einen Einzelobservablen-Anker
validiert, sondern über einen **Konsistenz-Test** der drei
Loop-Größen (Creutz-Ratio, §7.7). Die absolute Abweichung zur
SPA-Referenz bleibt bei ~5 % (Multi-Loop-Korrektur).

### §7.7 — Etappe 23b: Creutz-Ratio-Konsistenz-Test

**Design-Entscheidungen:**

| Frage | Antwort |
|---|---|
| Observable | `χ(2,2) = −ln( W(2,2)·W(1,1) / W(2,1)² )` |
| W(m,n) | `⟨Re Tr(W_C)/2⟩` gemittelt über alle m×n-Loops der 3 Ebenen (xy, xz, yz) |
| Kernel-Funktion | `ProPhysics_Wilson_Loop_Average(pu, m, n)` (read-only) |
| Kernel-Slot-Konvention | Nur Vorwärts-Links gepflegt; Rückwärts via Adjungierte an vorheriger Position |
| Sweep | FAST: dim ∈ {16, 32}; FULL: dim ∈ {16, 32, 64, 128} |
| β-Werte | {1,0; 2,0; 4,0} |
| Sweep-Parameter | Thermalisierung 200, Messung 300, 30 Bins à 10 Sweeps |
| Loop-Messung | Am Ende jedes Bins (Kosten-Optimierung für dim=128) |
| Determinismus | Seed `0xC0DE0000 + (dim<<16) + round(β·1000)` |
| PASS-Kriterien | B1 (χ>0), B2 (χ monoton in β), B3 (χ dim-konsistent, 3σ) |
| Speicherort | Kernel: `ProPhysics_SU2.c` (Loop-Funktion); Test: `alpha_test_creutz_ratio.c` |

**Kernel-Beitrag (1 neue Funktion + 2 interne Helfer):**

| Funktion | Wirkung |
|---|---|
| `ProPhysics_Wilson_Loop_Average` | Mittelwert `Re Tr(W_C)/2` über alle m×n-Loops der drei Ebenen. Read-only. |
| `pro_su2_loop_step` (static) | Vorwärts-Kanten-Multiplikation. |
| `pro_su2_loop_step_backward` (static) | Rückwärts-Kanten-Multiplikation via Adjungierte. |

**FAST-Ergebnisse (dim ∈ {16, 32}):**

**dim=16:**

| β | W(1,1) | W(2,1) | W(2,2) | χ(2,2) |
|---|---|---|---|---|
| 1,00 | 0,24120 ± 0,00066 | 0,05850 ± 0,00093 | 0,00300 ± 0,00081 | 1,04844 ± 0,19338 |
| 2,00 | 0,45589 ± 0,00073 | 0,20912 ± 0,00082 | 0,04394 ± 0,00085 | 0,78591 ± 0,01758 |
| 4,00 | 0,72683 ± 0,00042 | 0,54315 ± 0,00069 | 0,31931 ± 0,00121 | 0,24010 ± 0,00281 |

**dim=32:**

| β | W(1,1) | W(2,1) | W(2,2) | χ(2,2) |
|---|---|---|---|---|
| 1,00 | 0,24124 ± 0,00020 | 0,05826 ± 0,00023 | 0,00343 ± 0,00028 | 1,51631 ± 0,08838 |
| 2,00 | 0,45436 ± 0,00027 | 0,20742 ± 0,00032 | 0,04350 ± 0,00029 | 0,77838 ± 0,00518 |
| 4,00 | 0,72734 ± 0,00018 | 0,54383 ± 0,00034 | 0,32009 ± 0,00046 | 0,23930 ± 0,00078 |

**Konsistenz-Prüfung:**

| Kriterium | Ergebnis |
|---|---|
| B1 (χ > 0 für alle dim,β) | **PASS** |
| B2 (χ monoton fallend in β pro dim) | **PASS** |
| B3 (χ konsistent über dim, 3σ) | **PASS** |

**Bugfix in Etappe 23b (intern):** Die erste Version von
`Wilson_Loop_Average` verwendete die Rückwärts-Slots direkt, die
der Kernel aber nie beschreibt. Der Fix liest jetzt auf
Rückwärts-Segmenten immer den Vorwärts-Link an der vorherigen
Position und adjungiert. Vor dem Fix lieferte der Test Noise-Werte
(W ~ 1e-4); nach dem Fix physikalisch sinnvolle Werte
(W(1,1) ~ 0,45 bei β=2).

**Physikalische Bedeutung:** Die drei Loop-Größen stehen in der
erwarteten Relation zueinander (`W(2,2) < W(2,1) < W(1,1)`); die
Creutz-Ratio fällt monoton in β (Confinement-Signal); sie ist
dim-konsistent innerhalb 3σ (UV-Konsistenz für dim ∈ {16, 32}).
Die Kreuzvalidierung `W(1,1) ↔ RC-⟨P⟩` stimmt auf ~4,8 % überein.

**Testergebnis: 45/45 PASS.**

**Grenzen:** Der FAST-Modus läuft in der Prio-All-Regression;
der FULL-Modus (`dim ∈ {16, 32, 64, 128}`) ist ein Nightly-Kandidat.
β=1 hat große Fehlerbalken (schwache Kopplung, große Fluktuationen);
für engere Fehler wäre `CR_N_MEASURE` (300 → 1000) nötig.

### §7.8 — ProWB / Web-Docs (Patch 1.23.11)

**Design-Entscheidungen:**

| Frage | Antwort |
|---|---|
| Sprache | C99 (konsistent mit Kernel) |
| Abhängigkeiten | keine (außer msvcrt) |
| MD-Quellen | **nicht kopiert**; Manifest verweist auf kanonische Pfade |
| Sektionen | 6 (Overview, Physics, Modules, API, Tests, Build) + Settings-Utility |
| Theme-Default | `proedc` (dark industrial, petroleum) |
| Nav | flach, aus Manifest generiert |
| CI | GitHub Actions → GitHub Pages |
| Legacy-API | `prowb_build()` bleibt; `prowb_build_from_manifest()` neu |

**Neue Komponenten:**

| Pfad | Zweck |
|---|---|
| `src\prowb\prowb.c` | Builder-Kern (Manifest-Parser, Template-Expansion) |
| `src\prowb\md_parser.c` | Eigenständiger GFM-Parser |
| `build\prowb\Makefile.nmake` | Baut `bin\prowb\prowb.exe` |
| `docs\web\manifest.txt` | 46 Einträge, 6 Sektionen |
| `docs\web\src\*` | Templates, CSS, JS, Views |
| `.github\workflows\web-docs.yml` | CI-Workflow |

**Kernel-Beitrag:** **keiner.** ProWB ändert keinen Kernel-Code,
keine ABI, keine Tests.

**Verifikation:**

- `pro_run web` läuft fehlerfrei.
- `out\web\index.html` ~1,2 MB.
- Alle 6 Sektionen erreichbar.
- Themes funktionieren (`default` / `light` / `matrix`).
- CI deployt auf GitHub Pages.

**R7-Konformität:** Additive Erweiterung. Der Kernel-Pfad ist
bit-identisch.

### §7.9 — CI-Dokumentation (Patch 1.23.12)

Drei GitHub-Actions-Workflows, alle dokumentiert:

| Workflow | Trigger | Laufzeit | Doku |
|---|---|---|---|
| `ci.yml` | Push + PR auf `main` | ~1,5 min | `docs\build\ci.md` |
| `web-docs.yml` | Push auf `main` (Pfad-Filter) | ~1 min | `docs\build\web-docs-ci.md` |
| `alpha-nightly.yml` | **manuell** | ~23–64 min | `docs\build\alpha-nightly.md` |

**Kernel-Beitrag:** keiner. Additive Doku.

---

## §8 — Was für Makrophysik fehlt

### §8.1 — U4' — Bad

Kopplung an unsichtbare Freiheitsgrade (anderer Subgraph).

### §8.2 — U5' — Plastizität

Dynamische Topologie (Kantenstärken aus Amplitude).

### §8.3 — Neue Fünf-Regel-Struktur

1. U1 — Topologie.
2. U2 — Unitäre Dynamik.
3. U3 — Projektion.
4. U4' — Bad.
5. U5' — Plastizität.

Stand nach Etappe 23b: U1–U3 vollständig, U4 vollständig,
U4' teilweise, U5' fehlt.

---

## §9 — Etappe 18 abgeschlossen

Vier Bausteine: Coulomb-Field, Imaginaerzeit-Prep,
U4 Shared Reference, U4 Klassen-Tick. 29/29.

---

## §10 — Etappe 19 abgeschlossen

| Kriterium | Inhalt | Ergebnis |
|---|---|---|
| K3 | SU(2)-Algebra | `max_err = 0` |
| K2 | `R(2π) = −I` | exakt |
| K1 | g-Faktor | 8e-10 |

Spin-1/2 emergent aus SU(2)-Struktur.

---

## §11 — Etappe 21 abgeschlossen

| Kriterium | Ergebnis |
|---|---|
| γ-Algebra | 9,3e-10 |
| Massenterm Dirac | 2,4e-07 |
| Massenterm Weyl | 2,4e-07 |
| Dispersions-Charakter | OK |
| Zitterbewegung | sichtbar |
| Regression | 0,0 |

Dirac-Struktur emergent aus signed-permutation-Mechanik.

---

## §12 — Etappe 22 abgeschlossen (kinematisch)

### §12.1 — Was implementiert ist

| Komponente | Status |
|---|---|
| `ProPhysics_SU2.c` (neues Modul) | ✅ |
| Quaternion-Arithmetik in 2³⁰ | ✅ |
| `Set_Edge_SU2` + Auto-Aktivierung | ✅ |
| `Set_Edge_SU2_AxisAngle` | ✅ |
| `Get_Edge_SU2` | ✅ |
| `Wilson_Loop_SU2` (vorwärts, eichinvariant) | ✅ |
| `Wilson_Loop_SU2_Trace` | ✅ |
| `Apply_Local_SU2_Gauge` | ✅ |
| `Verify_SU2_Quaternion` | ✅ |
| `ProEdge`-Erweiterung (24 B) | ✅ |
| Cache-Aligned-Alloc | ✅ |
| Kernel-Tick in `ProPhysics_Core.c` | ✅ |
| Test-Runner in `tools\` | ✅ |
| `test_su2_wilson_loop` (14 + KA) | ✅ 15/15 |

### §12.2 — Physikalische Bedeutung

| Kriterium | Inhalt | Ergebnis |
|---|---|---|
| Quaternion-Algebra | Isomorphie zu 2×2-SU(2) | 4,45e-10 |
| Wilson-Loop-Eichinvarianz | `Tr(W)` unter `g(x)·U·g(y)†` | 0,0 |
| Nicht-Abelschheit | `[U_0,U_1] ≠ 0` | 1,0747 |
| U5 unter aktiven SU(2)-Links | 500 Ticks | 1,57e-09 |

**Nicht-abelsche Eichstruktur emergent. Erster Baustein einer
Yang-Mills-artigen Theorie im Kernel.**

### §12.3 — R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ Bit-Mask statt Modulo |
| R2 | ✅ Alloc nur in Initialize |
| R3 | ✅ U5 erhalten mit und ohne su2_active |
| R4 | ✅ Quaternion-Produkt exakt unitär |
| R5 | ✅ SDK-Wrapper unverändert aufrufbar |
| R6 | ✅ Prio-8-Test registriert |
| R7 | ✅ `su2_active == 0` bit-identisch |

### §12.4 — Grenze

**Keine Link-Dynamik im Tick.** Die Links sind statische Metadaten.
Ein Link-Update-Tick ist Etappe 22b oder 23+.

**Nachtrag:** Etappe 22b schließt diese Lücke.

---

## §12b — Etappe 22b abgeschlossen (Link-Dynamik)

### §12b.1 — Was implementiert ist

| Komponente | Status |
|---|---|
| `ProPhysics_SU2_Dynamics.c` (neues Modul) | ✅ |
| Leapfrog-Integration (`Apply_SU2_Tick`) | ✅ |
| Staple-Force mit su(2)-Projektion | ✅ |
| `SU2_Plaquette_Action` (globale Action) | ✅ |
| `SU2_Total_Energy` (H = S + ½Σ|E|²) | ✅ |
| `SU2_Link_Plaquette_Sum` (lokale Action) | ✅ |
| `ProEdge.su2_E_*` (4 × int32, 16 B) | ✅ |
| `su2_dynamics_active` (R7-Flag) | ✅ |
| `su2_yang_mills_q15` (Kopplung) | ✅ |
| `pro_su2_mul/conj/norm_sq` in `Internal.h` | ✅ |
| `pro_su2_exp_apply` in `Internal.h` | ✅ |
| `pro_su2_edge*` in `Internal.h` (Patch 1.23.10) | ✅ |
| `test_su2_wilson_loop` T15–T18 | ✅ 4/4 |

### §12b.2 — Physikalische Bedeutung

| Kriterium | Inhalt | Ergebnis |
|---|---|---|
| Link-Norm unter Leapfrog | unitär bis auf Q30-Rundung | **1,80e-08** |
| Energieerhaltung | symplektisch (O(dt²)) | **1,41e-03** |
| R7-Konformität | `su2_dynamics_active == 0` | byte-identisch |
| Aktive Dynamik | Link ändert sich | ja |

**Erste dynamische Yang-Mills-Integration im Kernel.**

### §12b.3 — R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ Bit-Shift, keine div/mod im Hotpath |
| R2 | ✅ Kein malloc/calloc im Hotpath |
| R3 | ✅ U5 bleibt erhalten |
| R4 | ✅ Link-Update unitär |
| R5 | ✅ Alle alten SU(2)-Funktionen unverändert |
| R6 | ✅ Prio-8-Test um T15–T18 erweitert |
| R7 | ✅ `su2_dynamics_active == 0` bit-identisch |

### §12b.4 — Grenzen und offene Punkte

**Kein Thermostat.** Leapfrog läuft auf einer Energie-Hyperfläche,
nicht auf einer kanonischen Verteilung. Etappe 23 schließt diese
Lücke durch Metropolis im Test.

**Backward-Staple-Fix:** In Etappe 23 bestätigt (Patch 1.23.7).
T16 von 8,06e-03 auf 2,44e-03 (Faktor 3,3), T15 von 2,02e-08 auf
1,80e-08.

**Forward-Plaquette-Konjugations-Fix:** In Patch 1.23.10 behoben.
T16 weiter von 2,44e-03 auf 1,41e-03 (Faktor 1,7). T11
(rein-imaginäres `b`) unverändert, weil dort `l3br = l4br = 0`.

**Kein Link-Update basierend auf `amp_grid`.** Die Links und die
Amplituden sind bisher unabhängig.

---

## §12c — Etappe 23 abgeschlossen (Metropolis / SPA-Referenz)

### §12c.1 — Was implementiert ist

| Komponente | Status |
|---|---|
| `alpha_test_running_coupling.c` (neuer Test) | ✅ |
| Metropolis-Sampler auf SU(2)-Links | ✅ |
| `ProPhysics_SU2_Link_Plaquette_Sum` (Kernel-Erweiterung, read-only) | ✅ |
| Sweep dim ∈ {16, 32, 64} × β ∈ {0.5, 1, 2, 4} | ✅ |

### §12c.2 — SPA-Referenz und ihre Rücknahme (23b)

**Der ursprüngliche V&V-Anker (0,08 %) ist zurückgenommen.**

Der Anker beruhte auf `u_plaq(β=2, dim=64) = 0,283270` (Stand
`1.23.0`). Nach dem Plaquette-Konjugations-Fix in `1.23.10`:

| Größe | Wert |
|---|---|
| `u_plaq(β=2, dim=64)` | **0,272552** |
| `⟨P⟩ = 1 − 2·u_plaq` | **0,454896** |
| Referenz `I₂(2)/I₁(2)` | 0,43313 |
| Abweichung | **ca. 5 %** |

Die ~5 % sind im Rahmen der erwarteten Multi-Loop-Korrekturen zur
Ein-Plaquette-Approximation (SPA). **Kein 0,08 %-Anker mehr.**

**Die Validierung der Sampler-Korrektheit** läuft ab `1.23.13`
über die **Creutz-Ratio** (§12d).

### §12c.3 — R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ Bit-Shift, keine div/mod |
| R2 | ✅ Kein malloc/calloc im Kernel |
| R3 | ✅ U5 bleibt erhalten |
| R4 | ✅ Wilson-Action exakt |
| R5 | ✅ Alle alten Funktionen unverändert |
| R6 | ✅ Prio-8 um `Running-Coupling` erweitert |
| R7 | ✅ Read-only Kernel-Erweiterung; keine Pfadänderung |

### §12c.4 — Grenzen

**Kein V&V-Anker mehr.** Der Wert liegt bei ~5 % zur SPA-Referenz
(Multi-Loop-Korrektur).

**RC-Rohwerte für β≠2 sind vorläufig.** Ein vollständiger
RC-Neulauf nach `1.23.10` steht aus. Die alten Werte sind um
ca. −3 bis −4 % zu korrigieren (Größenordnung aus der
β=2-Korrektur).

**Nightly-Job-Test.** Running-Coupling läuft ~1 400 s (dim=64
dominiert). Nicht in normalen CI-Läufen.

---

## §12d — Etappe 23b abgeschlossen (Creutz-Ratio)

### §12d.1 — Was implementiert ist

| Komponente | Status |
|---|---|
| `alpha_test_creutz_ratio.c` (neuer Test) | ✅ |
| `ProPhysics_Wilson_Loop_Average` (Kernel, read-only) | ✅ |
| `pro_su2_loop_step` + `_backward` (interne Helfer) | ✅ |
| CLI-Flag `--test-creutz-ratio` | ✅ |
| CLI-Flag `--creutz-full` | ✅ |
| FAST-Modus (dim ∈ {16, 32}) | ✅ |
| FULL-Modus (dim ∈ {16, 32, 64, 128}) | ✅ |
| Test-Runner-Erweiterung (`Creutz-Ratio`, `Creutz-Ratio-Full`) | ✅ |
| Test-Header (`alpha_test_common.h` v3.2) | ✅ |
| Test-Harness (`alpha_test_main.c`) | ✅ |
| Build-Integration (`build/test/Makefile.nmake` v3.3) | ✅ |
| Konsistenz-Kriterien B1/B2/B3 | ✅ 3/3 |

### §12d.2 — Physikalische Bedeutung

| Kriterium | Inhalt | Ergebnis |
|---|---|---|
| B1 | χ > 0 für alle (dim, β) | **PASS** |
| B2 | χ monoton fallend in β | **PASS** |
| B3 | χ dim-konsistent (3σ) | **PASS** |
| Kreuzvalidierung | `W(1,1)(β=2)` vs. `RC-⟨P⟩(β=2)` | ~4,8 % |
| Loop-Relation | `W(2,2) < W(2,1) < W(1,1)` | ✅ |

**Der Metropolis-Sampler ist konsistent.** Die drei Loop-Größen
stehen in der erwarteten Relation; die Creutz-Ratio fällt monoton
in β (Confinement); sie ist dim-unabhängig innerhalb 3σ für
`dim ∈ {16, 32}`.

### §12d.3 — R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ Bit-Mask für Koordinaten, kein div/mod |
| R2 | ✅ Stack-only in `Wilson_Loop_Average` |
| R3 | ✅ read-only; keine amp_grid-Mutation |
| R4 | ✅ Quaternion-Produkt unverändert |
| R5 | ✅ Additiv; keine bestehende Signatur geändert |
| R6 | ✅ Prio-8 um zwei Katalogeinträge erweitert |
| R7 | ✅ Bei `su2_active == 0` gibt `Wilson_Loop_Average` 0.0 zurück; kein bestehender Pfad geändert |

### §12d.4 — Grenzen

**Konsistenz-Test, kein V&V-Anker.** Die Creutz-Ratio prüft die
innere Konsistenz des Samplers, nicht die absolute Übereinstimmung
mit einer externen Referenz.

**FULL-Modus nur Nightly.** dim=128 dominiert die Laufzeit (~40 min).
Der FAST-Modus (dim ∈ {16, 32}) läuft in der Prio-All-Regression.

**β=1-Statistik verrauscht.** Fehlerbalken ±0,19 bei dim=16
(schwache Kopplung, große Fluktuationen). Für engere Fehler wäre
`CR_N_MEASURE` (300 → 1000) nötig.

**Kein Link-Update basierend auf `amp_grid`.** Die Links und die
Amplituden sind bisher unabhängig.

---

## §12e — ProWB / Web-Docs abgeschlossen (Patch 1.23.11)

### §12e.1 — Was implementiert ist

| Komponente | Status |
|---|---|
| `src/prowb/prowb.c` (Builder-Kern) | ✅ |
| `src/prowb/md_parser.c` (GFM-Parser) | ✅ |
| `src/prowb/header/prowb.h` | ✅ |
| `src/prowb/header/md_parser.h` | ✅ |
| `src/prowb/README.md` | ✅ |
| `build/prowb/Makefile.nmake` | ✅ |
| `docs/web/manifest.txt` (46 Einträge) | ✅ |
| `docs/web/README.md` | ✅ |
| `docs/web/src/parts/*.html` | ✅ |
| `docs/web/src/css/*.css` (8 Dateien) | ✅ |
| `docs/web/src/js/*.js` (2 Dateien) | ✅ |
| `docs/web/src/views/*.html` (10 Dateien) | ✅ |
| `.github/workflows/web-docs.yml` | ✅ |
| Master-`prowb`-Target | ✅ |
| `pro_run web`-Aktion | ✅ |

### §12e.2 — Bedeutung

**Die Doku ist publizierbar.** Bisher war die Markdown-Doku nur
direkt im Repo lesbar. ProWB rendert sie als statisches Web-Portal
mit Navigation, Themes und Sektionen — deploybar über GitHub Pages.

**Keine MD-Kopien.** Die MD-Dateien bleiben an ihren kanonischen
Pfaden (`docs/project/`, `docs/test/`, `docs/build/`, `docs/physics/`).
Das Manifest verweist auf sie. Änderungen wirken beim nächsten Build
automatisch.

### §12e.3 — R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ ProWB nutzt `div`/`mod` außerhalb des Kernels — irrelevant |
| R2 | ✅ ProWB nutzt `malloc` außerhalb des Kernels — irrelevant |
| R3–R4 | ✅ unberührt |
| R5 | ✅ `prowb_build()` bleibt funktional (additiv) |
| R6 | ✅ keine neue Kernel-Etappe, kein Kernel-Test nötig |
| R7 | ✅ kein Kernel-Pfad geändert |

### §12e.4 — Grenzen

**Kein dynamisches Sektions-System.** Die 6 Sektionen sind in
`prowb.c` hart kodiert. Eine 7. Sektion erfordert C-Änderung (siehe
`src/prowb/README.md` §9.4).

**Keine Cross-Refs in MD.** Markdown-Links wie `[link](other.md)`
bleiben tot. Phase-2-Thema (siehe `TODO.md` §5.9).

**Keine Volltextsuche.** Kein Index. Phase-2-Thema.

**Kein PDF-Export.** Nur HTML. Phase-2-Thema.

---

## §13 — Roadmap Etappen 24–27 + M1–M3 + O1

```
22 ✅ → 22b ✅ → 23 ✅ → 23b ✅ → 24 → 25 → 26 → 27 → M1 → M2 → M3 → (O1)
       ▲
       └── 1.23.1–1.23.13 (Konsolidierung + ProWB + CI + Creutz-Ratio, kein Phasenwechsel)
```

### Etappe 24 — Euklidisches Pfadintegral ⏭️

- Pfadintegral = Operator-Formalismus.
- `test_path_integral_equivalence`, rel_dev < 5 %.

### Etappe 25 — GHZ / Mermin

- n=3-Verschränkung.
- Hypergraph statt Union-Find.
- `test_ghz_mermin`, S > 4 klassisch vs. 4√2 QM.

### Etappe 26 — Universalität / Algorithmen

- T-Gate, Deutsch-Josza, kleiner Grover.
- `test_universality`.

### Etappe 27 — Q61-Migration

- `ProAmpQ31` (int64) → `ProAmpQ61` (128-bit struct).
- `ProAmpVector` 64 B → 128 B.
- `test_q61_drift`, Drift < 1e-12 über 2000 Ticks.

### Etappe 18d-B (optional) — Wasserstoff-Revision

- Adaptive Prep für höhere strength.
- Nicht blockierend.

### Etappe M1 — U4' — Bad

- Kopplung an unsichtbare Freiheitsgrade.
- `test_thermalization` — Boltzmann-Verteilung aus Mikrodynamik.

### Etappe M2 — U5' — Plastizität

- Dynamische Topologie.
- `test_gravitational_attraction`.

### Etappe M3 — Makrophysik-Konsistenz

- Klassischer Limes, Kontinuumslimes.
- `test_classical_limit`.

### Etappe O1 (aufgeschoben) — Cache-Optimierung

- `CHANNELS_MAX` 16 → 8, SoA-Layout, `ProRegister`-Alignment.
- Nach Etappe 25 und M1/M2.

### Was nicht mehr in der Roadmap steht

- **23b (Creutz-Ratio)** — erledigt (`1.23.13`).
- **CI-Doku** — erledigt (`1.23.12`).
- **ProWB / Web-Docs** — erledigt (`1.23.11`).

---

## §14 — Was wir bewusst nicht behaupten

1. Keine Quantengravitation.
2. Nicht „warum QM?" — nur emergente Struktur.
3. Kein Kontinuumslimes.
4. Kein Bell-Bruch (S > 2 = Superdeterminismus-Loop).
5. Kein perfektes Wasserstoff-Spektrum bei dim=64 (0,296).
6. Keine Thermalisierung ohne U4'.
7. U4-Shared-Reference ist transitiv; GHZ braucht Hypergraph.
8. `dim` muss Zweierpotenz sein (48 verboten).
9. Spin-1/2, Dirac, SU(2)-Feld sind Struktur-Erweiterungen.
10. Dirac-Dispersion im Kontinuumslimes braucht Renormierung.
11. SU(2)-Link-Dynamik ist **klassisch** (Leapfrog), keine
    Quantenfeldtheorie.
12. **Kein V&V-Anker.** Der 0,08-%-Anker aus `1.23.0` wurde mit
    `1.23.13` zurückgenommen; der korrigierte Wert liegt bei ~5 %
    zur SPA-Referenz (Multi-Loop-Korrektur).
13. **Creutz-Ratio ist ein Konsistenz-Test, kein absoluter Anker.**
    Sie prüft die Relation dreier Loop-Größen, nicht die
    Übereinstimmung mit einer externen Referenz.
14. **ProWB ist ein Werkzeug, keine Physik.** Es ändert keinen
    Kernel-Pfad.

---

## §15 — Offene Punkte

| Punkt | Status | Etappe |
|---|---|---|
| Fundament U1–U5 | ✅ | 18e |
| Spin-1/2 | ✅ | 19 |
| Dirac + α^i | ✅ | 21/21b |
| SU(2)-Eichfeld (kinematisch) | ✅ 14/14 | 22 |
| Refactoring 22 | ✅ | 22 |
| SU(2)-Link-Dynamik | ✅ 18/18 | 22b |
| Backward-Staple-Fix | ✅ bestätigt (T16 2,44e-03) | 22b |
| Forward-Plaquette-Konjugations-Fix | ✅ bestätigt (T16 1,41e-03) | 1.23.10 |
| SU(2)-Metropolis / SPA-Referenz | ⚠ V&V-Anker zurückgenommen; RC-Neulauf ausstehend | 23 + 23b |
| Konsolidierungs-Serie `1.23.1`–`1.23.13` | ✅ | 23 + 23b |
| ProWB / Web-Docs | ✅ (Patch 1.23.11) | 23 |
| Standard-CI | ✅ (Patch 1.23.9) | 23 |
| Alpha-Nightly | ✅ (Patch 1.23.12) | 23 |
| **Creutz-Ratio-Konsistenz** | ✅ FAST (dim ∈ {16, 32}) | 23b |
| **Creutz-Ratio-Full** | ⏳ Nightly-Job (dim=128 dominiert) | 23b |
| **RC-Neulauf (β ≠ 2, dim=64)** | offen (nicht blockierend) | — |
| Wasserstoff quantitativ | ⚠ 0,296 | 18d-B |
| Euklidisches Pfadintegral | offen | 24 |
| Hypergraph für GHZ | offen | 25 |
| Universalität | offen | 26 |
| Q61-Migration | offen | 27 |
| U4' Bad | offen | M1 |
| U5' Plastizität | offen | M2 |
| Makrophysik-Konsistenz | offen | M3 |
| Cache-Optimierung | aufgeschoben | O1 |
| Web-Docs Phase 2 (Cross-Refs, Suche, i18n) | offen | — |

---

## §16 — Was bis zur vollständigen QM fehlt

Geordnet nach **Kategorie**. Jede Zeile = eine geschlossene Lücke.
Kategorien sind unabhängig; Etappen können mehrere betreffen.

### §16.1 — Struktur-Lücken

| # | Lücke | Was fehlt | Etappe |
|---|---|---|---|
| S2 | Renormierung | Skalenabhängigkeit von Kopplungen | 24 |
| S3 | Pfadintegral-Äquivalenz | Euklidischer Formalismus ↔ Operator | 24 |
| S4 | n-Teilchen-Verschränkung | Hypergraph statt Union-Find | 25 |
| S5 | Universalität | T-Gate als nicht-Clifford-Operation | 26 |
| S6 | Höhere Präzision | Q61-Migration (int128) | 27 |
| S7 | SU(3) / nicht-abelsche Erweiterung höherer Ordnung | 3-dim Farbraum, Gell-Mann | — |
| S8 | Elektroschwache Struktur | Higgs-Mechanismus, SU(2)×U(1) | — |
| S9 | Generationen-Struktur | 3 Fermion-Familien | — |

### §16.2 — Emergenz-Lücken

| # | Lücke | Was fehlt | Etappe |
|---|---|---|---|
| E2 | Kontinuums-Dispersion | `ω² = k² + m²` quantitativ | 24 |
| E3 | Confinement / Massenlücke | nicht-perturbative Yang-Mills-Physik | 24+ |
| E4 | Instantonen | topologische Link-Konfigurationen | 24+ |
| E5 | Wasserstoff quantitativ < 5 % | adaptive Prep | 18d-B |
| E6 | Thermalisierung | Boltzmann-Verteilung aus Mikrodynamik | M1 |
| E7 | Dekohärenz | Kopplung an unsichtbaren Subgraph | M1 |
| E8 | Klassischer Limes | `ħ → 0`-Limit sichtbar | M3 |

### §16.3 — Numerik-Lücken

| # | Lücke | Was fehlt | Etappe |
|---|---|---|---|
| N1 | Q31-Rundungsgrenze | sichtbar in Renormierung und langen Läufen | 27 |
| N2 | Monte-Carlo-Sampling | ✅ Metropolis + Creutz-Ratio | 23 + 23b |
| N3 | Memory-Bandwidth-Bottleneck | `CHANNELS_MAX`-Reduktion, SoA | O1 |

### §16.4 — Makrophysik-Lücken

| # | Lücke | Was fehlt | Etappe |
|---|---|---|---|
| M1' | Kopplung an Bad | U4'-Regel | M1 |
| M2' | Gravitation | U5'-Regel (dynamische Topologie) | M2 |
| M3' | Kosmologie | Expansion, Horizont | — |
| M4' | Kontinuumslimes von Raumzeit | Projektion Grid ↔ Graph | M3 |

### §16.5 — Was nicht mehr fehlt (Stand nach 23b + 1.23.13)

- ✅ Fundament U1–U5.
- ✅ Unitäre QM-Dynamik.
- ✅ 2-Teilchen-Verschränkung.
- ✅ Spin-1/2 aus SU(2).
- ✅ Dirac-Struktur (4-Komponenten).
- ✅ U(1)-Eichtheorie (abelsch).
- ✅ SU(2)-Eichtheorie (nicht-abelsch, kinematisch).
- ✅ SU(2)-Link-Dynamik (Leapfrog, klassisch).
- ✅ Metropolis-Sampling auf SU(2)-Links (kanonische Verteilung).
- ✅ Wilson-Action-Validierung (SPA-Referenz ~5 %, ehrliche Schranke).
- ✅ **Creutz-Ratio-Konsistenz-Test (drei Loop-Größen).**
- ✅ Jordan-Wigner / Fermionen.
- ✅ Lindblad / Offene Systeme.
- ✅ Soliton / Breather.
- ✅ **Publizierbare Web-Docs** (ProWB, Patch 1.23.11).
- ✅ **CI-Infrastruktur** (Standard-CI + Alpha-Nightly + Web-Docs-CI).

### §16.6 — Kurzfassung

**Bis „vollständige QM" fehlen 4 Kern-Etappen** (24, 25, 26, 27)
plus die SU(3)-Erweiterung und der Higgs-Mechanismus als
nicht-terminierte Strukturen. Optional Etappe 18d-B für
Wasserstoff-Revision.

**Bis Makrophysik fehlen 3 Etappen** (M1, M2, M3).

---

## §17 — Chronik der Kernel-Änderungen

| Funktion / Feld | Etappe | Status |
|---|---|---|
| `Apply_Edge_Transport` | 9 | stabil |
| `Apply_Edge_Transport_Colored[_3D]` | 9 / 17 | stabil |
| `Apply_Wave_Step` | 6i / 17 | n_nb ∈ {4,6} |
| `Apply_Local_Phase_Plate` | 17b | stabil |
| `Apply_Coulomb_Phase_Field_3D` | 18 | stabil |
| `Apply_Nonlinear_Phase_Step_Dilated` | 12 | stabil |
| `Apply_Amp_Step` | 16e … 22b | Dispatch shared + Dirac + SU(2)-Dynamics |
| `ProSharedInfo` + 6 API-Funktionen | 18c | Union-Find |
| `Shared_Tick_Reps` | 18e | stabil |
| `Measure_Amp_Invariant` | 8 / 18c | Klassen-basiert |
| `Entangle_Nodes_Singlet` | 19 | stabil |
| `Is_Spin_Flipped` / `Get_Node_Spin_View` | 19 | stabil |
| `Apply_Nonlinear_Phase_Step_Spin` | 19 | stabil |
| `Verify_Gamma_Algebra` | 21 | realisiert |
| `Apply_Dirac_Mass_Term` / `Apply_Dirac_Step` | 21 / 21b | realisiert |
| `ProGammaBasis` | 21 | stabil |
| `dirac_active`, `dirac_gamma_basis`, `dirac_mass_q15` | 21 | stabil |
| `PRO_NODE_DIRAC_BIT` | 21 | stabil |
| `ProPhysics_Tick` (Kernel) | 22 | refactoring |
| `ProPhysics_RuleCallback` | 22 | refactoring |
| `ProPhysics_Set_Edge_SU2` | 22 | realisiert |
| `ProPhysics_Set_Edge_SU2_AxisAngle` | 22 | realisiert |
| `ProPhysics_Get_Edge_SU2` | 22 | realisiert |
| `ProPhysics_Wilson_Loop_SU2` | 22 | realisiert |
| `ProPhysics_Wilson_Loop_SU2_Trace` | 22 | realisiert |
| `ProPhysics_Apply_Local_SU2_Gauge` | 22 | realisiert |
| `ProPhysics_Verify_SU2_Quaternion` | 22 | realisiert |
| `ProSU2Basis` | 22 | stabil |
| `su2_active`, `su2_gauge_basis`, `su2_coupling_q15` | 22 | stabil |
| `ProEdge.su2_a_re/ai`, `su2_b_re/bi` | 22 | stabil |
| `pro_aligned_calloc` / `pro_aligned_free` | 22 | refactoring |
| `pro_sat_i32` | 22 | refactoring |
| `pro_amp_abs2` | 22 | refactoring (Bug-Fix) |
| `pro_apply_signed_perm_vec` | 22 | refactoring |
| `pro_amp_vec_norm_sq_range` | 22 | refactoring |
| `pro_pauli_get` | 22 | refactoring |
| `pro_wilson_validate_path` | 22 | refactoring |
| `ProPhysics_Enable_SU2_Dynamics` | 22b | realisiert |
| `ProPhysics_Disable_SU2_Dynamics` | 22b | realisiert |
| `ProPhysics_Is_SU2_Dynamics_Active` | 22b | realisiert |
| `ProPhysics_Set_SU2_Yang_Mills` | 22b | realisiert |
| `ProPhysics_Apply_SU2_Tick` | 22b | realisiert |
| `ProPhysics_SU2_Plaquette_Action` | 22b | realisiert |
| `ProPhysics_SU2_Total_Energy` | 22b | realisiert |
| `ProPhysics_SU2_Link_Plaquette_Sum` | 22b | realisiert (read-only) |
| `su2_dynamics_active`, `su2_yang_mills_q15` | 22b | stabil |
| `ProEdge.su2_E_a_re/ai`, `su2_E_b_re/bi` | 22b | stabil |
| `pro_su2_mul` / `pro_su2_conj` / `pro_su2_norm_sq` (Internal.h) | 22b | refactoring |
| `pro_su2_exp_apply` (Internal.h) | 22b | realisiert |
| `ProPhysics_SU2_Dynamics.c` | 22b | neues Modul |
| **Backward-Staple-Fix (`su2_force_on_link`)** | **1.23.7** | **Bug-Fix** |
| **`alpha_test_running_coupling.c`** | **23** | **neuer Test** |
| **Konsolidierung aller 12 Kernel-Module** | **1.23.1–1.23.10** | **rein intern** |
| **12 neue Modul-Docs (`docs/project/*.md`)** | **1.23.1–1.23.10** | **vollständig** |
| **`pro_su2_edge*` nach `Internal.h` (B7-Auflösung)** | **1.23.7 + 1.23.10** | **refactoring** |
| **Forward-Plaquette-Konjugations-Fix (`su2_plaquette_action_at`)** | **1.23.10** | **Bug-Fix** |
| **Compiler-Warnungen auf 0 reduziert** | **1.23.10** | **cleanup** |
| **ProWB-Builder (`src/prowb/`)** | **1.23.11** | **neues Tool** |
| **ProWB-Makefile (`build/prowb/`)** | **1.23.11** | **neues Build-Target** |
| **Web-Docs-Quelle (`docs/web/`)** | **1.23.11** | **neue Komponente** |
| **Web-Docs-CI (`.github/workflows/web-docs.yml`)** | **1.23.11** | **neue CI-Pipeline** |
| **`pro_run web`** | **1.23.11** | **neue Aktion** |
| **CI-Dokumentation (`docs/build/ci.md`, `alpha-nightly.md`)** | **1.23.12** | **neue Doku** |
| **Alpha-Nightly-Workflow (`.github/workflows/alpha-nightly.yml`)** | **1.23.12** | **neue CI-Pipeline** |
| **`docs/project/README.md` (Website)** | **1.23.12** | **neue Doku** |
| **`ProPhysics_Wilson_Loop_Average`** | **23b / 1.23.13** | **realisiert** |
| **`pro_su2_loop_step` / `_backward` (statisch)** | **23b / 1.23.13** | **refactoring** |
| **`alpha_test_creutz_ratio.c`** | **23b / 1.23.13** | **neuer Test** |
| **CLI-Flags `--test-creutz-ratio`, `--creutz-full`** | **23b / 1.23.13** | **neue Flags** |
| **Test-Runner-Einträge `Creutz-Ratio`, `Creutz-Ratio-Full`** | **23b / 1.23.13** | **neue Katalogzeilen** |
| **`SU2.md` v1.1** | **23b / 1.23.13** | **erweitert** |

---

## §18 — Historie

| Version | Datum | Änderung |
|---|---|---|
| 1.0 | 16e'' | Erste Fassung |
| 1.1 | 2026-09-21 | Etappe 17 |
| 1.2 | 2026-09-21 | Etappe 17b: Phase Plate |
| 1.3 | 2026-09-21 | Etappe 17b: Kalibrierung |
| 1.4 | 2026-09-21 | Prio-all 35/35 |
| 1.5 | 2026-09-23 | Etappe 18: Coulomb/Hydrogen, 36/36 |
| 2.0 | 2026-09-23 | Ontologie-Update, 10 Thesen, U4'/U5' |
| 2.1 | 2026-09-23 | Etappe 18b: Imaginaerzeit, 37/37 |
| 2.2 | 2026-09-23 | Etappe 18c: U4 Shared Reference, 38/38 |
| 2.3 | 2026-09-23 | Etappe 18d: dim=64 |
| 2.4 | 2026-09-24 | Etappen 18d + 18e, 39/39 |
| 2.5 | 2026-09-24 | Konsolidierung nach 18e |
| 2.6 | 2026-09-24 | Etappe 19: Spin-1/2, 40/40 |
| 2.7 | 2026-09-24 | Etappe 21 + 21b: Dirac, 41/41 |
| 2.8 | 2026-09-25 | Etappe 22 + Refactoring: SU(2)-Eichfeld (14/14 + KA). Prio-All 42/42. |
| 2.9 | 2026-09-25 | Etappe 22b: SU(2)-Link-Dynamik (Leapfrog). Prio-8 auf 18/18 + KA. |
| 3.0 | 2026-09-25 | Etappe 23: SU(2)-Metropolis / Wilson-Action-Validierung. Prio-All 43/43. 1 neue read-only Funktion `SU2_Link_Plaquette_Sum`. Neuer Test `alpha_test_running_coupling.c`. Erste absolute Validierung gegen externe Lattice-QCD-Physik (V&V-Anker: ⟨P⟩(β=2) = 0,43346 vs. Referenz 0,43313, Abweichung 0,08 %). |
| 1.0 | 2026-09-26 | Schema-Wechsel auf Etappen-basierte Versionierung (Kernel 1.23.0, Etappe 23). Doc-Version von `3.0` auf `1.0`. Konsolidierungs-Serie `1.23.1`–`1.23.8`: alle 12 Kernel-Module auf einheitliches Schema, 12 neue Modul-Docs in `docs/project/`, Header konsolidiert, Backlog B7 (SU(2)-Edge-Zugriff) teilweise gelöst. Alle Änderungen bit-identisch, 43/43 PASS. |
| 1.0 | 2026-09-27 | Nachtrag Konsolidierungs-Serie `1.23.9` und `1.23.10`. `1.23.9`: Release-Vorbereitung (SDK-Versionierung, Build-Skripte, Sub-Makefiles, Build-Docs auf Etappe 23; Lizenz-URL, VERSIONING.md, Repo-Hygiene, BASELINE.md, Beispiel-BUILD_INFO). `1.23.10`: Reparatur des unvollständigen Patch 1.23.7 (`pro_su2_edge*` nach `Internal.h`, Bugfix `LNK2001`); Forward-Plaquette-Konjugations-Fix in `su2_plaquette_action_at` (T16 verbessert von 2,44e-03 auf 1,41e-03, Faktor 1,7); ungenutzte Variablen entfernt; `uint64_t`→`uint32_t`-Casts; `PRO_NODE_*_MASK` eingeführt. Compiler-Warnungen auf 0. Regression 14/14 PASS. |
| 1.0 | 2026-09-28 | Nachtrag Patch `1.23.11` (ProWB / Web-Docs Integration). §1.1 Ordner-Layout um `src/prowb/`, `build/prowb/`, `bin/prowb/`, `docs/web/`, `out/web/` erweitert. §1.2 Dokument-Struktur um ProWB-Docs, Web-Docs-Quelle und CI-Doku erweitert. §1.3 Build-Skripte um ProWB-Makefile, Web-Docs-CI. §3.5 neue Kategorie „Werkzeug-Erweiterungen". §6.1 Kernel-Umfang um ProWB-Module. §6.2 Funktionsumfang um Web-Docs-Builder und CI. §7.7 neue ProWB-Sektion. §12d neue ProWB-Abgeschlossen-Sektion. §13 Roadmap um 1.23.11-Hinweis. §14 Punkt 14 (ProWB ist Werkzeug). §15 offene Punkte um ProWB + Web-Docs-Phase-2. §16.5 Web-Docs als „nicht mehr fehlend". §17 Chronik um 5 ProWB-Einträge erweitert. §18 Historie um diesen Eintrag. |
| 1.0 | 2026-09-28 | Nachtrag Patch `1.23.12` (CI-Dokumentation). §1.2 Dokument-Struktur um CI-Docs (`ci.md`, `alpha-nightly.md`). §1.3 Build-Skripte um drei CI-Workflows. §3.5 Werkzeug-Erweiterungen um Standard-CI und Alpha-Nightly. §6.2 Funktionsumfang um CI-Zeilen. §6.5 CI-Empfehlung um Nightly-Scopes. §7.9 neue CI-Doku-Sektion. §12d.4 Grenzen. §15 offene Punkte um Alpha-Nightly. §16.5 CI-Infrastruktur als „nicht mehr fehlend". §17 Chronik um 3 CI-Einträge. §18 Historie um diesen Eintrag. |
| **1.0** | **2026-09-27** | **Nachtrag Etappe 23b / Patch `1.23.13` (Creutz-Ratio + V&V-Anker-Rücknahme). §0 Einleitung um V&V-Anker-Rücknahme und Creutz-Ratio. §2 R3-Tabelle um 23b-Nachweis. §2 R6 um `test_creutz_ratio` ergänzt. §2 R7 um 1.23.13-Zeile. §3.4 neue Zeile „SU(2)-Loop-Mittelung". §4 Beitrag zu T4 um 23b. §5 Beitrag zu Subgraph. §6.1 Kernel-Umfang (19 Test-Module). §6.2 Funktionsumfang (Loop-Average + Creutz-Ratio). §6.3 Beleg 13 umgeschrieben (V&V-Anker-Rücknahme, korrigierter Wert 0,272552, ~5 %); Beleg 14 neu (Creutz-Ratio-Konsistenz mit FAST-Tabelle). §6.4 β-Funktions-Punkt entfernt. §6.5 Prio-All 43 → 45, Laufzeit, CI-Empfehlung. §7.6 V&V-Anker-Sektion umgeschrieben. §7.7 neue Creutz-Ratio-Sektion. §7.8 ProWB-Sektion umnummeriert. §7.9 CI-Doku-Sektion neu. §12c.2 SPA-Referenz + Rücknahme. §12d neue Creutz-Ratio-Abgeschlossen-Sektion. §12e ProWB-Sektion umnummeriert. §13 Roadmap: 23b ✅. §14 Punkt 12–13 (kein V&V-Anker; Creutz-Ratio = Konsistenz). §15 offene Punkte aktualisiert. §16.3 N2 ✅. §16.5 Creutz-Ratio als „nicht mehr fehlend". §17 Chronik um 6 23b-Einträge. §18 dieser Eintrag.** |

---

**Ende Roadmap v1.0.**