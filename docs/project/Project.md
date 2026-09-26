# ProPhysics — Roadmap / Projekt-Dokumentation

**Version:** 3.0
**Stand:** 2026-09-25 nach Etappe 23 (Running-Coupling mit V&V-Anker,
            Prio-All 43/43)
**Nächster Schritt:** Etappe 24 — Euklidisches Pfadintegral

---

## §0 — Zweck dieses Dokuments

1. **Projekt-Struktur** — Ordner, Dokumentation, Build-Skripte.
2. **Was wir haben** — abgeschlossene Etappen 1–23, mit quantitativen Belegen.
3. **Was wir glauben** — die Ontologie in 5 Ur-Regeln und 10 Thesen.
4. **Was fehlt** — offene Fähigkeitslücken bis „komplette QM" und bis Makrophysik.
5. **Unsere Regeln** — die Projekt-Invarianten, die keine Etappe brechen darf.
6. **Die Roadmap** — Etappen 24–27 und M1–M3.
7. **Was wir bewusst nicht behaupten** — die Grenzen des Anspruchs.

**Neu in v3.0:** Der Kernel ist gegen einen externen physikalischen
Referenzwert validiert (Etappe 23, V&V-Anker). Bis Etappe 22b waren
alle Tests **relativ** (Konsistenz, Selbsterhaltung). Etappe 23
liefert den ersten **absoluten** Vergleich mit publizierter
Lattice-QCD-Physik.

---

## §1 — Projekt-Struktur

### §1.1 — Ordner-Layout

```
ProPhysics\
├── BUILD_INFO.txt
├── bin\                        (DLLs, EXEs — flach, ohne Runner)
├── lib\                        (Import-Libs)
├── src\
│   ├── prophysics\             (13 Kernel-Module + 6 Header)
│   ├── sdk\                    (pro_sdk_interface.c + Header)
│   └── test\                   (alpha_test_*.c + 2 Example-Tests)
├── tools\                      (Test-Runner: .ps1 + .cmd)
├── build\                      (main / prophysics / sdk / test)
├── docs\                       (build / project / test)
├── python\
└── out\                        (Export-Ziel)
```

**Prinzipien:**

| Ordner | Regel |
|---|---|
| `bin\` | flach; nur DLLs + EXEs. Kein Code, kein Runner. |
| `lib\` | flach; nur Import-Libs. |
| `src\<modul>\` | `.c` direkt, `.h` in `header\`. |
| `tools\` | Skripte, die nicht zum Kernel-Build gehören (Test-Runner). |
| `build\<modul>\` | ein Makefile pro Modul, aus seinem Ordner aufgerufen. |
| `docs\<gebiet>\` | Markdown, thematisch getrennt. |
| `out\` | Laufzeit-Erzeugnis, nicht versioniert. |

### §1.2 — Dokument-Struktur

| Datei | Inhalt |
|---|---|
| `docs\project\Project.md` | dieses Dokument |
| `docs\test\ProPhysics_Testkatalog.md` | alle 43 Tests |
| `docs\test\run_alpha_tests.md` | Test-Runner |
| `docs\build\BUILD_SCRIPT.md` | Build-Übersicht |
| `docs\build\main\Makefile.md` | Master-Makefile |
| `docs\build\prophysics\Makefile.md` | Kernel-Build |
| `docs\build\sdk\Makefile.md` | SDK-Interface-Build |
| `docs\build\test\Makefile.md` | Test-Build |
| `docs\build\helper\build.md` | `build.ps1` / `build.cmd` |
| `docs\build\helper\export.md` | `export.ps1` / `export.cmd` |
| `docs\build\helper\write_build_info.md` | `write_build_info.ps1` |

### §1.3 — Build-Skripte

**Master (in `build\main\`):** `Makefile.nmake`, `build.ps1`/`.cmd`,
`export.ps1`/`.cmd`, `write_build_info.ps1`.

**Sub-Makefiles:** `build\prophysics\Makefile.nmake`,
`build\sdk\Makefile.sdk.nmake`, `build\test\Makefile.nmake`.

**Reihenfolge:** `prophysics` → `sdk` → `test`.

**Test-Runner (in `tools\`):** `run_alpha_tests.ps1` + `.cmd`.
`-ExeDir` Default = `<repo>\bin`, `-LogDir` Default = `<ExeDir>\logs`.

### §1.4 — Typischer Aufruf

```cmd
cd build\main
build.cmd                       :: alles bauen
build.cmd -Mode prophysics -Rebuild

cd ..\..\tools
run_alpha_tests.cmd -Prio all
```

---

## §2 — Projekt-Regeln (unverhandelbar)

### R1 — Kein `div` / `mod` im Hotpath

Erlaubt: Bit-Shift, Bit-Mask bei Zweierpotenzen, `pro_ensure_grid_shift`.
Verboten: `k % dim`, `k / dim` in Apply-/Tick-Funktionen.
`dim` muss Zweierpotenz sein.

### R2 — Kein `malloc` / `calloc` / `free` im Hotpath

Erlaubt: `ProPhysics_Initialize`, `ProPhysics_Free`, `*_Create`,
Test-Setup, dokumentierte Diagnose-Ausnahmen.
Verboten: `Apply_*`, `*_Tick`, pro Knoten.

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

### R4 — Unitäre Dynamik

Jeder Transport-, Wave-, Context-, Gauge-Schritt unitär bis auf
Q31-Rundung.

### R5 — Keine stillen API-Brüche

Bei Änderung einer API-Signatur wird der betroffene Test mit angepasst.
Für Refactoring 22: alle alten SDK-Symbole bleiben funktional
(`ProPhysics_SDK_Execute_Plastizitaet_Tick` delegiert jetzt an
`ProPhysics_Tick`; `ProPhysics_ScientificRuleCallback` ist Alias
für `ProPhysics_RuleCallback`).

### R6 — Jede Etappe endet mit einem Test

`test_<etappe>_<aspekt>`, registriert in `alpha_test_main.c` und
`tools\run_alpha_tests.ps1`. Etappen-Test grün = alle bisherigen
Tests weiter grün.

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
| **23** | **nur lesende API-Erweiterung** | **bit-identisch** |

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
| **SU(2)-Metropolis (Thermostat)** | **23** | **read-only** | **kein Flag** | **`SU2_Link_Plaquette_Sum` (Vorbereitung für Metropolis im Test)** |

Alle fünf sind Struktur-Erweiterungen, keine Reduktion auf die reine
Ur-Grammatik.

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
- **23 — Links thermalisieren; kanonische Verteilung als Zustand.**

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

---

## §6 — Was wir haben (Etappen 1–23)

### §6.1 — Kernel-Umfang

| Kategorie | Umfang |
|---|---|
| Kernel-Module | 13 `.c` |
| Kernel-Header | 6 `.h` |
| Test-Module | 18 `.c` (+ 1 Header) |
| SDK | 1 `.c` + 1 `.h` |
| Codezeilen | ~36 000 LOC gesamt |
| Datenstrukturen | `ProUniverse`, `ProNode`, `ProRegister`, `ProAmpVector`, `ProEdge`, `ProSharedInfo`, Tensor/Fock/Density-Trilogie |
| Dimensionen | 2D (dim ∈ {16,32,64,128}), 3D (dim ∈ {16,32,64}) |
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
| **Gauge SU(2), thermalisiert (Metropolis)** | ✅ **neu in 23** |
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
| **Wilson-Action-Plaquette-Validierung** | ✅ **neu in 23** |

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
| **13** | **SU(2)-Metropolis / Wilson-Action** | **V&V-Anker 0,08 % (siehe unten)** |

**Beleg 13 — SU(2)-Metropolis-Validierung (Etappe 23, NEU):**

Erste **absolute** Validierung des Kernels gegen externe
Lattice-QCD-Physik.

| Größe | Wert |
|---|---|
| Observable | ⟨P⟩ = ⟨Re Tr W⟩ / 2 = 1 − 2·u_plaq |
| β=2.0, dim=64 | 0.43346 ± 0.00005 |
| Referenz I₂(2)/I₁(2) | 0.43313 |
| Abweichung | **0,08 %** |

**Skalen-Unabhängigkeit u_plaq(β, dim):**

| β | dim=16 | dim=32 | dim=64 |
|---|---|---|---|
| 0.50 | 0.438085 | 0.438256 | 0.438134 |
| 1.00 | 0.379640 | 0.379966 | 0.379875 |
| 2.00 | 0.283311 | 0.283425 | 0.283270 |
| 4.00 | 0.170166 | 0.170304 | 0.170344 |

Streuung zwischen den drei dim-Werten < 0,1 %. Fehlerbalken skaliert
wie 1/√N (Verhältnis 16→32 = 0,30, 32→64 = 0,35, erwartet 0,354).

**Bedeutung:** Die Wilson-Action-Normierung, der Metropolis-
Akzeptanzschritt und die Q30-Quaternion-Multiplikation sind
unabhängig validiert. Ein falscher Sampler würde auf Prozent-Ebene
abweichen, nicht auf 0,08 %.

**Grenze:** Der V&V-Anker ist eine Ein-Plaquette-Approximation
(starke-Kopplungs-Limes). Bei β=2 sind Multi-Loop-Korrekturen klein
aber nicht null. Für den Test reicht das, weil der Sampler-Akzeptanz-
Fehler bei falscher Verteilung **prozentual** wäre, nicht 0,08 %.

### §6.4 — Was plausibel, aber nicht quantitativ belegt ist

- Born-Emergenz (Stichproben klein).
- Soliton-Klassifikation (phänomenologisch).
- Verschränkung in `amp_grid` ohne U4 (strukturell, nicht quantitativ).
- Dirac-Kontinuums-Dispersion ω² = k² + m² (braucht Renormierung, Etappe 24+).
- **β-Funktion** — `u_plaq` allein ist skalenunempfindlich. Eine echte
  β-Funktions-Messung braucht eine zweite Observable (Creutz-Ratio),
  Etappe 23b optional.

### §6.5 — Prio-all-Regression (2026-09-25 nach Etappe 23)

| Prio | Thema | Tests | Status |
|---|---|---|---|
| 1 | 2D-Basis | 12 | 12/12 |
| 2 | Emergenz | 10 | 10/10 |
| 3 | Langlauf | 10 | 10/10 |
| 4 | 3D-Torus | 3 | 3/3 |
| 5 | Hydrogen + Shared-Ref + Tournament | 4 | 4/4 |
| 6 | Spin-1/2 | 1 | 1/1 |
| 7 | Dirac | 1 | 1/1 |
| 8 | SU(2)-Eichfeld + Link-Dynamik + Running-Coupling | 2 | 2/2 |
| **Gesamt** | | **43** | **43/43** |

**Prio-All-Laufzeit:** 4 420,6 s (~73,7 min).

**Laufzeit-Treiber:**
- Hydrogen-48 (~2 281 s = 51,6 %)
- Running-Coupling (~1 398 s = 31,6 %)
- Alle anderen (~742 s = 16,8 %)

**CI-Empfehlung:**
- **Prio 1–4, 6, 7** + `SU2-Wilson-Loop` (~1,5 min) in normalen CI-Läufen.
- **`Running-Coupling`** als Nightly-Job (Timeout 2 400 s).
- **Prio 5** (Hydrogen-48) ebenfalls als Nightly-Job.

---

## §7 — Etappen 18–23: Details

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

**Backward-Staple-Fix (in Etappe 23 verifiziert):**
Der Backward-Staple in `su2_force_on_link` hatte fehlende
`†`-Dagger auf `U_μ(x-ν)`. Der Fix reduzierte:
- T15: 2,02e-08 → **1,80e-08**
- T16: 8,06e-03 → **2,44e-03** (Faktor 3,3)

Die Energiedrift ist jetzt auf dem Niveau, das symplektische
O(Δ²)-Oszillation für `dt=500, g²=500` hergibt.

**Test:** 18/18 + Kernel-Algebra = 19/19 PASS.

### §7.6 — Etappe 23: SU(2)-Metropolis / Wilson-Action-Validierung (NEU)

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

**R7-Konformität:** Die neue Funktion ist read-only. Sie kann keinen
bestehenden Pfad ändern. Alle Tests vor Etappe 23 bleiben bit-identisch.

**Testergebnis: 43/43 PASS.**

**Ergebnisse (dim=64):**

| β | u_plaq | u_err | ⟨P⟩ = 1 − 2·u_plaq |
|---|---|---|---|
| 0.50 | 0.438134 | 0.000025 | 0.123732 |
| 1.00 | 0.379875 | 0.000021 | 0.240250 |
| 2.00 | **0.283270** | **0.000027** | **0.433460** |
| 4.00 | 0.170344 | 0.000019 | 0.659312 |

**V&V-Anker:**

| Größe | Wert | Abweichung |
|---|---|---|
| ⟨P⟩(β=2.0, dim=64) | 0.433460 ± 0.000054 | |
| Referenz I₂(2)/I₁(2) | 0.43313 | |
| **Abweichung** | | **0,08 %** |

**Was das Projekt ab jetzt ist:**
Der Kernel ist **validiert** gegen einen externen physikalischen
Referenzwert. Bis Etappe 22b waren alle Tests relativ
(„Größe A = Größe B", „Drift < X"). Etappe 23 liefert den ersten
**absoluten** Vergleich mit publizierter Lattice-QCD-Physik.

**Was noch fehlt:**
- **β-Funktions-Messung.** `u_plaq` allein ist skalen-unempfindlich.
  Eine echte β-Funktion braucht zwei Observablen (z.B. Creutz-Ratio).
  Etappe 23b optional.

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

Stand nach Etappe 23: U1–U3 vollständig, U4 vollständig,
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
| `test_su2_wilson_loop` T15–T18 | ✅ 4/4 |

### §12b.2 — Physikalische Bedeutung

| Kriterium | Inhalt | Ergebnis |
|---|---|---|
| Link-Norm unter Leapfrog | unitär bis auf Q30-Rundung | **1,80e-08** |
| Energieerhaltung | symplektisch (O(dt²)) | **2,44e-03** |
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

**Backward-Staple-Fix:** In Etappe 23 bestätigt. T16 von 8,06e-03
auf 2,44e-03 (Faktor 3,3), T15 von 2,02e-08 auf 1,80e-08.

**Kein Link-Update basierend auf `amp_grid`.** Die Links und die
Amplituden sind bisher unabhängig.

---

## §12c — Etappe 23 abgeschlossen (Metropolis / Wilson-Action) — NEU

### §12c.1 — Was implementiert ist

| Komponente | Status |
|---|---|
| `alpha_test_running_coupling.c` (neuer Test) | ✅ |
| Metropolis-Sampler auf SU(2)-Links | ✅ |
| `ProPhysics_SU2_Link_Plaquette_Sum` (Kernel-Erweiterung, read-only) | ✅ |
| Sweep dim ∈ {16, 32, 64} × β ∈ {0.5, 1, 2, 4} | ✅ |
| V&V-Anker gegen I₂(2)/I₁(2) | ✅ 0,08 % |

### §12c.2 — Physikalische Bedeutung

**Der Kernel ist zum ersten Mal validiert.** Ein Sampler, der
falsch implementiert wäre, würde auf Prozent-Ebene abweichen, nicht
auf 0,08 %.

| Größe | Wert |
|---|---|
| Wilson-Action-Normierung | ✅ β = 4/g²-Äquivalent |
| Metropolis-Akzeptanz | ✅ Boltzmann-konform |
| Quaternion-Multiplikation | ✅ Q30-Rundung akkumuliert nicht |
| Observable UV-Konvergenz | ✅ Δ < 0,1 % für dim 16/32/64 |

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

**Keine β-Funktions-Messung.** `u_plaq` ist skalen-unempfindlich.
Für eine echte β-Funktion braucht es eine zweite Observable
(z.B. Creutz-Ratio) bei größeren Gittern — Etappe 23b optional.

**V&V-Anker basiert auf Ein-Plaquette-Approximation.** Bei β=2
gibt es Multi-Loop-Korrekturen. Für die Sampler-Validierung reicht
das, weil ein falscher Sampler prozentual abweichen würde.

**Nightly-Job-Test.** Running-Coupling läuft ~1 400 s (dim=64
dominiert). Nicht in normalen CI-Läufen.

---

## §13 — Roadmap Etappen 24–27 + M1–M3 + O1

```
22 ✅ → 22b ✅ → 23 ✅ → [23b optional] → 24 → 25 → 26 → 27 → M1 → M2 → M3 → (O1)
```

### Etappe 23b — Creutz-Ratio (optional)

- Neue Kernel-Funktion `Wilson_Loop_Average(m, n)` (read-only).
- Creutz-Ratio `χ = -ln(W(1,1)·W(2,2) / (W(1,2)·W(2,1)))`.
- Erste genuine β-Funktions-Messung im Kernel.
- Nur bei Bedarf; 23 kann auch ohne 23b als abgeschlossen gelten.

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
12. Keine β-Funktion ohne zweite Observable (u_plaq allein ist
    skalenunempfindlich).
13. **V&V-Anker (0,08 %) ist Ein-Plaquette-Näherung**, nicht exakte
    Gitter-QCD. Für die Sampler-Validierung reicht es.

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
| **SU(2)-Metropolis / V&V-Anker** | ✅ **0,08 %** | **23** |
| Wasserstoff quantitativ | ⚠ 0,296 | 18d-B |
| Creutz-Ratio / β-Funktion | offen | 23b |
| Euklidisches Pfadintegral | offen | 24 |
| Hypergraph für GHZ | offen | 25 |
| Universalität | offen | 26 |
| Q61-Migration | offen | 27 |
| U4' Bad | offen | M1 |
| U5' Plastizität | offen | M2 |
| Makrophysik-Konsistenz | offen | M3 |
| Cache-Optimierung | aufgeschoben | O1 |

---

## §16 — Was bis zur vollständigen QM fehlt

Geordnet nach **Kategorie**. Jede Zeile = eine geschlossene Lücke.
Kategorien sind unabhängig; Etappen können mehrere betreffen.

### §16.1 — Struktur-Lücken

| # | Lücke | Was fehlt | Etappe |
|---|---|---|---|
| S2 | Renormierung | Skalenabhängigkeit von Kopplungen | 23b/24 |
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
| E2 | Kontinuums-Dispersion | `ω² = k² + m²` quantitativ | 23b/24 |
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
| N2 | Monte-Carlo-Sampling | ✅ Metropolis für SU(2)-Links | 23 |
| N3 | Memory-Bandwidth-Bottleneck | `CHANNELS_MAX`-Reduktion, SoA | O1 |

### §16.4 — Makrophysik-Lücken

| # | Lücke | Was fehlt | Etappe |
|---|---|---|---|
| M1' | Kopplung an Bad | U4'-Regel | M1 |
| M2' | Gravitation | U5'-Regel (dynamische Topologie) | M2 |
| M3' | Kosmologie | Expansion, Horizont | — |
| M4' | Kontinuumslimes von Raumzeit | Projektion Grid ↔ Graph | M3 |

### §16.5 — Was nicht mehr fehlt (Stand nach 23)

- ✅ Fundament U1–U5.
- ✅ Unitäre QM-Dynamik.
- ✅ 2-Teilchen-Verschränkung.
- ✅ Spin-1/2 aus SU(2).
- ✅ Dirac-Struktur (4-Komponenten).
- ✅ U(1)-Eichtheorie (abelsch).
- ✅ SU(2)-Eichtheorie (nicht-abelsch, kinematisch).
- ✅ SU(2)-Link-Dynamik (Leapfrog, klassisch).
- ✅ **Metropolis-Sampling auf SU(2)-Links (kanonische Verteilung).**
- ✅ **Wilson-Action-V&V-Anker gegen externe Referenz.**
- ✅ Jordan-Wigner / Fermionen.
- ✅ Lindblad / Offene Systeme.
- ✅ Soliton / Breather.

### §16.6 — Kurzfassung

**Bis „vollständige QM" fehlen 4 Kern-Etappen** (24, 25, 26, 27)
plus die SU(3)-Erweiterung und der Higgs-Mechanismus als
nicht-terminierte Strukturen. Optional Etappe 23b für β-Funktion.

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
| **Backward-Staple-Fix (`su2_force_on_link`)** | **23** | **Bug-Fix bestätigt** |
| **`alpha_test_running_coupling.c`** | **23** | **neuer Test** |

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
| **3.0** | **2026-09-25** | **Etappe 23: SU(2)-Metropolis / Wilson-Action-Validierung. Prio-All 43/43. Kernel auf 13 Module unverändert, 1 neue read-only Funktion `SU2_Link_Plaquette_Sum`. Neuer Test `alpha_test_running_coupling.c`. **Erste absolute Validierung gegen externe Lattice-QCD-Physik** (V&V-Anker: ⟨P⟩(β=2) = 0,43346 vs. Referenz 0,43313, Abweichung 0,08 %). Backward-Staple-Fix in 22b bestätigt (T16 2,44e-03, T15 1,80e-08). §2 R7 um Etappe 23 (read-only). §3.4 fünfte Schicht-2-Erweiterung (Metropolis). §6.1 Kernel-Umfang auf 36 kLOC. §6.3 Beleg 13 (V&V-Anker). §6.5 43/43. Neuer §7.6 Etappe-23-Details. Neuer §12c Etappe-23-Zusammenfassung. §13 Roadmap 23 ✅, 23b optional. §14 Punkt 13 (V&V-Grenze). §15 Offene Punkte: Etappe 23 ✅, Creutz-Ratio offen. §16 N2 erledigt. §17 Chronik: Backward-Staple-Fix + neue Testdatei. §18 Historie. **Major-Bump wegen Statuswechsel: Kernel ist validiert.** |

---

**Ende Roadmap v3.0.**