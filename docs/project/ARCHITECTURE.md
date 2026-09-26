# ProPhysics — Architektur

**Datei:** `docs/project/ARCHITECTURE.md`
**Version:** 1.0
**Stand:** 2026-09-25 (Kernel-Version 3.0.0, Etappe 23)
**Zweck:** Beschreibt den Aufbau des Kernels, die Datenflüsse pro Tick
und die Design-Entscheidungen. Komplement zu `ProPhysics_API.md`
(was existiert) und `Project.md` (warum das Projekt existiert).

---

## §0 — Wozu dieses Dokument

Die API-Referenz sagt, **was** jede Funktion tut. Die Roadmap sagt,
**warum** das Projekt existiert. Dieses Dokument sagt, **wie** die Teile
zusammenhängen — wer verstehen will, wie ein Tick abläuft, wie die
Daten strukturiert sind, wo der Hotpath liegt und warum bestimmte
Entscheidungen getroffen wurden, liest dieses Dokument.

**Nicht in diesem Dokument:**

- Einzelne Funktionssignaturen → `ProPhysics_API.md`
- Testkriterien → `ProPhysics_Testkatalog.md`
- Etappen-Historie → `Project.md` §18

---

## §1 — Übersicht

### §1.1 — Der Kernel in einem Satz

> Ein einzelner `amp_grid` aus 8-dimensionalen komplexen Vektoren pro
> Gitterknoten, auf dem unitäre Dynamik, Eichfelder, Dirac-Spinoren,
> Verschränkung und Fermionen **parallel** existieren — aktiviert durch
> Flags, dispatcht in einem einzigen Tick, mit U5-Invariante als
> Erhaltungssatz.

### §1.2 — Das Schichten-Modell

Der Kernel ist **einschichtig** im Datenmodell (ein `amp_grid`), aber
**mehrschichtig** in der Dynamik. Fünf Schichten bauen aufeinander auf:

```
┌─────────────────────────────────────────────────────────────┐
│ Schicht 4 — Sampling                                         │
│   Metropolis, Lindblad, Dichte-Trilogie                      │
│   aktiviert durch: Test-Code (nicht im Tick)                 │
├─────────────────────────────────────────────────────────────┤
│ Schicht 3 — Beobachter und Verschränkung                     │
│   Shared (U4), Tensor-Paare, Fock-Raum                       │
│   aktiviert durch: pu->shared.active, pu->tensor_pairs       │
├─────────────────────────────────────────────────────────────┤
│ Schicht 2 — Struktur-Erweiterungen                           │
│   Spin-1/2, Dirac, SU(2)-Links                               │
│   aktiviert durch: reserved_gating, dirac_active, su2_*      │
├─────────────────────────────────────────────────────────────┤
│ Schicht 1 — Kern-Dynamik (U1-U5)                             │
│   Transport, Wave, Context, Guiding                          │
│   aktiv: immer                                                │
├─────────────────────────────────────────────────────────────┤
│ Schicht 0 — Fundament                                        │
│   ProUniverse, amp_grid, ProEdge, ProRegister                │
│   keine Dynamik — nur Daten                                   │
└─────────────────────────────────────────────────────────────┘
```

**Wichtig:** Alle Schichten teilen denselben `amp_grid`. Es gibt keine
zweite Zustandsrepräsentation. Was sich ändert, ist die **Dynamik**,
die auf `amp_grid` wirkt.

**Emergenz-Prinzip:** Schicht 2–4 sind **strukturelle Erweiterungen**.
Sie sind nicht aus U1–U5 ableitbar. Aber sie allein sind keine Physik
— erst ihre Kopplung an Schicht 1 erzeugt emergente Phänomene (Bloch,
Dirac, Tsirelson, Confinement).

### §1.3 — Modul-Landkarte

13 Kernel-Module, gruppiert nach Schicht:

| Modul | Schicht | Aufgabe |
|---|:-:|---|
| `ProPhysics_Core.c` | 0+1 | Lifecycle, Topologie, Tick-Orchestrierung |
| `ProPhysics_Amp.c` | 1 | Unitäre Dynamik (Transport, Wave, Context) |
| `ProPhysics_Gauge.c` | 1+2 | U(1)-Eichstruktur, Born-Wahrscheinlichkeit |
| `ProPhysics_Observer.c` | 3 | Test-spezifische Diffusion |
| `ProPhysics_EPR.c` | 3 | Paarmessungen, Kollaps |
| `ProPhysics_Shared.c` | 3 | U4-Shared-Reference (Union-Find) |
| `ProPhysics_Dirac.c` | 2 | 4-Komponenten-Spinor |
| `ProPhysics_SU2.c` | 2 | SU(2)-Link-Kinematik |
| `ProPhysics_SU2_Dynamics.c` | 2 | Yang-Mills-Leapfrog |
| `ProPhysics_Tensor.c` | 3 | 2-Knoten-Verschränkung |
| `ProPhysics_Fock.c` | 3 | 8-Moden-Fock-Raum |
| `ProPhysics_Density.c` | 4 | Dichte-Matrizen + Lindblad |

### §1.4 — Header-Struktur

```
ProPhysics.h              (öffentliche API, sammelt alle Includes)
├── ProPhysics_Config.h    (Konstanten, self-contained)
├── ProPhysics_Types.h     (Structs, Enums, inline Helfer)
└── ProPhysics_Exports.h   (DLL-Macros)

ProPhysics_Internal.h     (interne Helfer, NICHT öffentlich)
```

`ProPhysics_Config.h` ist **self-contained** — sie inkludiert nichts
anderes aus dem Projekt. Damit ist die Konfiguration vor allen anderen
Headern verfügbar.

`ProPhysics_Internal.h` enthält `static inline`-Helfer (RNG, Sättigung,
Amplituden-Operationen, SU(2)-Arithmetik). Diese sind **modul-lokal**,
aber header-weit sichtbar.

---

## §2 — Datenmodell

### §2.1 — Die zentrale Datenstruktur: `ProUniverse`

Alle Zustände leben in **einem** Struct:

```c
typedef struct {
    uint64_t total_nodes;
    uint32_t grid_dim, grid_dim_shift, grid_dim_mask, grid_ndim;

    ProNode*       ur_grid;       /* Anzeige (U6) */
    ProRegister*   reg_source;    /* Topologie */
    ProRegister*   reg_target;    /* Topologie-Puffer */
    ProEdge*       edge_phases;   /* Kanten (U(1) + SU(2) + E) */
    ProAmpVector*  amp_grid;      /* FUNDAMENTALER ZUSTAND */
    ProAmpVector*  amp_scratch;   /* Ping-Pong-Puffer */

    /* ... weitere Felder ... */
} ProUniverse;
```

**Wichtige Eigenschaft:** `ProUniverse` enthält **Zeiger**, nicht die
Daten selbst. Die Allokation ist im `Initialize` gekapselt. Das erlaubt
dem Nutzer, `ProUniverse` auf dem Stack zu halten.

### §2.2 — Die sechs Kern-Arrays

| Array | Element-Typ | Größe | Rolle |
|---|---|---|---|
| `ur_grid` | `ProNode` | 8 B × N | **Anzeige** (U6-Zustand) |
| `reg_source` | `ProRegister` | 128 B × N | Topologie (aktiv) |
| `reg_target` | `ProRegister` | 128 B × N | Topologie (Puffer) |
| `edge_phases` | `ProEdge` | 40 B × N × 16 | Kanten-Metadaten |
| `amp_grid` | `ProAmpVector` | 64 B × N | **Fundamentaler Zustand** |
| `amp_scratch` | `ProAmpVector` | 64 B × N | Ping-Pong-Puffer |

**Gesamtgröße** für N Knoten: `(8 + 128 + 128 + 40·16 + 64 + 64) B × N`
= **904 B × N**. Bei `dim=64` (N=4096) sind das ~3,7 MB. Bei `dim=128²`
(N=16384) ~14,8 MB.

Bei `dim=64³` (N=262144) sind es ~237 MB — das ist der Speicherbedarf
des `Running-Coupling`-Tests.

### §2.3 — Warum sechs Arrays?

**Prinzip:** Ein Array pro Rolle, keine verschachtelten Structs.

- **`amp_grid` und `amp_scratch`** sind getrennt, weil Transport und
  Wave einen Ping-Pong-Puffer brauchen (kein in-place-Swap, weil sonst
  Doppelanwendungen passieren).
- **`reg_source` und `reg_target`** sind getrennt, weil der
  Plastizitäts-Callback `reg_target` mutiert und `reg_source` liest.
  Nach dem Callback werden sie vertauscht.
- **`ur_grid` und `edge_phases`** sind getrennt, weil sie
  unterschiedlich oft zugegriffen werden.

### §2.4 — Cache-Line-Alignment

Acht Hot-Arrays sind **Cache-Line-aligned** (`PRO_CACHE_LINE = 64`):

- `ur_grid`, `reg_source`, `reg_target`, `edge_phases`
- `amp_grid`, `amp_scratch`
- `u_field`, `shared.parent`

**Grund:** Bei einem sequenziellen Zugriff über die Knoten (wie im Tick)
sollen zwei aufeinanderfolgende Knoten nicht in derselben Cache-Line
liegen, wenn ihre Kanäle auf verschiedene Knoten zeigen. Und:
Alignment vermeidet Split-Loads.

**Allokation:** `pro_aligned_calloc` (definiert in
`ProPhysics_Internal.h`) verwendet `_aligned_malloc` (MSVC) bzw.
`aligned_alloc` (C11) bzw. `posix_memalign` (POSIX).

**Nicht-aligned:** `tensor_pairs`, `tensor_marks`, `fock_states`,
`density_matrices` — diese sind groß und selten zugegriffen.

### §2.5 — Was ist fundamental, was ist Anzeige?

| Zustand | Funda­mental? | Wer schreibt |
|---|:-:|---|
| `amp_grid` | **ja** | Dynamik (U1-U5) |
| `edge_phases.phase` | ja | `Apply_Local_Gauge` |
| `edge_phases.su2_*` | ja | SU(2)-API |
| `edge_phases.su2_E_*` | ja | Leapfrog |
| `shared.parent` | ja | `Entangle_Nodes` |
| `ur_grid.type_state` | **nein** | U6-Guiding |
| `ur_grid.field_helicity` | gemischt | Innere Uhr + EPR |
| `ur_grid.momentum_phase` | nein | `Inject_Momentum` |

**Konsequenz:** Wer `type_state` direkt setzt, umgeht die Physik. Der
nächste Tick überschreibt es via Guiding.

**Konsequenz für die API:** `ProPhysics_Set_Node_Amplitude` ist der
**physikalisch korrekte** Weg, Zustand zu setzen.
`ProPhysics_Spawn_Body` ist ein Test-Wrapper, der `type_state` und
`amp_grid` zusammen setzt.

---

## §3 — Der Tick — vollständiger Datenfluss

### §3.1 — Die Reihenfolge

`ProPhysics_Tick` führt acht Schritte in **fester** Reihenfolge aus:

```
ProPhysics_Tick(pu, callback)
    │
    ├─ (1) Lazy-Init reg_target, falls NULL
    │
    ├─ (2) current_cpu_tick++
    │
    ├─ (3) Advance_Internal_Clocks
    │       └─ Für jeden Knoten mit type_state != NEUTRAL, PHOTON:
    │           v = momentum_phase / 127
    │           γ⁻¹ = √(1 − v²)
    │           phase_accumulator += 256 · γ⁻¹
    │           bei Überlauf: field_helicity toggelt
    │
    ├─ (4) EPR-Propagation
    │       └─ Für jede EPR-Kante mit pending_ticks > 0:
    │           pending_ticks--
    │           bei 0: field_helicity[partner] = pending_helicity
    │
    ├─ (5) Apply_Amp_Step
    │       ├─ wenn su2_dynamics_active: Apply_SU2_Tick
    │       ├─ wenn shared.active:        Shared_Tick_Reps (return)
    │       ├─ wenn dirac_active:         Apply_Dirac_Step (return)
    │       └─ Standard:
    │           ├─ Context-Tick
    │           ├─ Edge-Transport (2D/3D, colored)
    │           ├─ Wave-Step
    │           ├─ Tensor-Paar-Kopplungen
    │           └─ Auto-Sync (Tensor → Amp, Density)
    │
    ├─ (6) Apply_Guiding_Equation
    │       └─ Für jeden Knoten:
    │           p_pos = |c_1|² + |c_2|²
    │           p_neg = |c_3|² + |c_4|²
    │           type_state = (u_field·(p_pos+p_neg) < 2¹⁶·p_pos)
    │                        ? UR_POSITRON_CW : UR_NEGATRON_CCW
    │
    ├─ (7) Graph-Plastizität (nur wenn callback != NULL)
    │       ├─ reg_target ← Kopie von reg_source
    │       ├─ Für jeden Knoten + Kanal:
    │       │   callback aufrufen
    │       │   mutierte Kanäle nach reg_target übernehmen
    │       └─ Swap reg_source ↔ reg_target
    │
    └─ (8) global_entropy_index = Anzahl aktiver Interaktionen
```

### §3.2 — Warum diese Reihenfolge?

**Uhr vor Dynamik:** Die innere Uhr modifiziert `field_helicity`, was
in der EPR-Propagation gelesen wird. Also muss sie zuerst laufen.

**EPR vor Amp-Step:** Die EPR-Propagation liefert Helizitäts-Änderungen
an Partner-Knoten. Diese beeinflussen die Dynamik erst im **nächsten**
Tick — die Kausalität bleibt erhalten. Deshalb ist die Reihenfolge so,
dass EPR-Änderungen in `amp_grid` noch nicht sichtbar sind.

**Amp-Step vor Guiding:** Guiding liest `amp_grid` und schreibt
`type_state`. Es muss also **nach** der Dynamik laufen.

**Plastizität zuletzt:** Die Topologie-Änderung beeinflusst erst den
nächsten Tick. Sie muss nach der Dynamik laufen, damit die aktuelle
Dynamik auf der alten Topologie arbeitet.

### §3.3 — Dispatch in `Apply_Amp_Step`

Die Dispatch-Reihenfolge hat Priorität:

```
1. su2_dynamics_active  → Leapfrog (kein return!)
2. shared.active        → Shared_Tick_Reps (return)
3. dirac_active         → Apply_Dirac_Step (return)
4. Standard             → Context + Transport + Wave + Tensor
```

**Warum `su2_dynamics_active` kein `return` macht:** Die SU(2)-Dynamik
modifiziert nur `ProEdge`, nicht `amp_grid`. Sie kann parallel zum
Standard-Pfad laufen.

**Warum `shared.active` ein `return` macht:** Shared-Tick-Reps ist
eine **komplette Alternative** zum Standard-Pfad. Beide gleichzeitig
würden doppelt zählen.

**Warum `dirac_active` ein `return` macht:** Dirac-Step ist ein
**vollständiger** Tick (Transport mit α-Kopplung + Massenterm).

### §3.4 — R7 — Warum die Reihenfolge nie geändert wird

R7 verlangt: **Keine Etappe ändert bestehende Pfade.** Konkret:
`su2_active == 0`, `dirac_active == 0`, `shared.active == 0` muss
**bit-identisch** zum Zustand vor Etappe 22 sein.

Erreicht durch:

- **Flags als Guards:** Jeder neue Pfad ist durch ein Flag geschützt.
- **Additive Erweiterungen:** Neue Funktionen laufen **nach** den alten,
  nicht stattdessen.
- **Keine Struct-Änderungen an Bestandsfeldern:** `ProEdge` wurde
  erweitert, aber die ersten 8 Bytes sind unverändert.

**Test:** Die Prio-1-Tests sind seit Etappe 9 unverändert. Sie sind
der Regressions-Anker.

---

## §4 — Aktivierungs-Flags und Dispatch

### §4.1 — Flag-Übersicht

| Flag | Feld | Wer setzt | Wirkung |
|---|---|---|---|
| Spin-1/2 | `reserved_gating` Bit 0 | `Entangle_Nodes_Singlet` | Spin-Flip-Markierung |
| Dirac | `reserved_gating` Bit 1 + `dirac_active` | `ProPhysics_Apply_Dirac_Step` | Dirac-Pfad |
| Shared | `shared.active` | `Entangle_Nodes` | Klassen-Tick |
| SU(2) aktiv | `su2_active` | `Set_Edge_SU2` | Links lesbar |
| SU(2) Dyn | `su2_dynamics_active` | `Enable_SU2_Dynamics` | Leapfrog-Schritt |
| Auto-Sync Tensor | `auto_sync_tensor` | `Tensor_Set_Auto_Sync` | Sync nach Tick |
| Auto-Sync Density | `auto_sync_density` | `Density_Set_Auto_Sync` | Sync nach Tick |

### §4.2 — Warum so viele Flags?

**Prinzip:** Jede Schicht-2/3/4-Erweiterung ist durch **ein Flag**
aktivierbar und **standardmäßig aus**.

**Warum:** R7. Ohne Flags müsste jede Erweiterung den Standard-Pfad
ändern, und die Prio-1-Tests würden brechen.

**Nebeneffekt:** Ein Nutzer, der nur Kernel-Physik will, ignoriert
alle Flags. Die Flags sind nicht störend.

### §4.3 — Flag-Interaktionen

| Kombination | Verhalten |
|---|---|
| `su2_active + su2_dynamics_active` | Links + E-Felder werden aktualisiert |
| `shared.active + dirac_active` | Shared hat Vorrang, Dirac wird ignoriert |
| `dirac_active + su2_dynamics_active` | Beide laufen (SU2-Tick zuerst) |
| Alle aus | Standard-Pfad (bit-identisch zu Etappe 9) |

**Shared hat Vorrang vor Dirac** — weil Shared-Tick-Reps eine komplette
Alternative ist, während Dirac-Step einen einzelnen Tick ausführt.

---

## §5 — Hotpath vs. Coldpath

### §5.1 — Definition

| Pfad | Ausführung | Regeln |
|---|---|---|
| **Hotpath** | pro Tick, pro Knoten | R1 (kein div/mod), R2 (kein malloc) |
| **Coldpath** | Setup, Allocation, Diagnose | Keine Regel-Einschränkung |

### §5.2 — Hotpath-Funktionen

Diese Funktionen laufen pro Tick über alle Knoten und unterliegen R1/R2:

- `ProPhysics_Advance_Internal_Clocks`
- `ProPhysics_Apply_Amp_Step` (und alle seine Kinder)
- `ProPhysics_Apply_Guiding_Equation`
- `ProPhysics_Tick` (EPR-Propagation)
- `ProPhysics_Apply_SU2_Tick`

### §5.3 — Coldpath-Funktionen

Diese Funktionen werden selten aufgerufen und dürfen div/mod/malloc:

- `ProPhysics_Initialize` (einmalig)
- `ProPhysics_Free` (einmalig)
- `ProPhysics_Make_Lambda_Field` (Setup)
- `ProPhysics_Apply_Local_Gauge` (Setup/Demo)
- `ProPhysics_Apply_Coulomb_Phase_Field_3D` (Setup)
- `ProPhysics_FockDensity_Apply_Mode_Lindblad` (malloc 2 MB Scratch)

### §5.4 — Wie der Hotpath R1/R2 erfüllt

**R1 — Kein div/mod:**

- Index-Berechnung über `grid_dim_shift` und `grid_dim_mask`.
  `k & mask` statt `k % dim`, `k >> shift` statt `k / dim`.
- Kanäle sind Compile-Time-Konstanten (`0..15`).
- `pro_div_round(x, d)` mit `d` fest — wird vom Compiler zu Shifts
  optimiert.

**R2 — Kein malloc:**

- Alle Puffer sind in `Initialize` allokiert.
- `amp_scratch` ist der Ping-Pong-Puffer.
- `reg_target` ist der Plastizitäts-Puffer.
- Statische Puffer (in `Density.c`) für 64×64-Lindblad.

**Ausnahme:** `FockDensity_Apply_Mode_Lindblad` allokiert 2 MB Scratch.
Das ist **dokumentiert** und passiert **nicht im Tick**, sondern im
Test-Harness.

### §5.5 — Messung

Der Tick ist **O(N · C)** in der Knotenzahl:

- Context-Tick: N × 8 Basis-Indizes
- Transport: N × 4 Kanäle
- Wave-Step: N × 8 × n_nb
- Guiding: N × 8

Bei `dim=64` (N=4096), 2D: **~10⁵ Operationen pro Tick**. Bei 1 GHz
sind das ~0,1 ms pro Tick.

Bei `dim=64³` (N=262144): **~10⁷ Operationen pro Tick**. ~10 ms pro Tick.

Der `Running-Coupling`-Test braucht ~1 400 s für 500 Sweeps × 262144
Knoten. Das sind **~5,3 ms pro Knoten-Sweep**, was konsistent ist.

---

## §6 — Der Ping-Pong-Mechanismus

### §6.1 — Warum Ping-Pong?

Transport und Wave-Step sind **nicht-lokal**: Um `amp_grid[k]` neu zu
berechnen, braucht man `amp_grid[nb]` für alle Nachbarn. Wenn man
in-place schreibt, liest man teilweise alte und teilweise neue Werte
— das Ergebnis hängt von der Iterationsreihenfolge ab.

**Lösung:** Lies aus `src`, schreibe nach `dst`, tausche am Ende.

```c
ProAmpVector* src = pu->amp_grid;
ProAmpVector* dst = pu->amp_scratch;

for (k = 0; k < N; ++k) {
    /* lies src[k], src[nb] */
    /* schreibe dst[k] */
}

/* Swap */
pu->amp_grid = dst;
pu->amp_scratch = src;
```

### §6.2 — Wo Ping-Pong verwendet wird

- `ProPhysics_Apply_Edge_Transport_Colored`
- `ProPhysics_Apply_Edge_Transport_Colored_3D`
- `ProPhysics_Apply_Wave_Step`
- `ProPhysics_Apply_Local_Amplitude_Diffusion`
- `ProPhysics_Apply_Nonlinear_Diffusion_Tick`

### §6.3 — Wo Ping-Pong nicht nötig ist

- `ProPhysics_Apply_Context_Tick`: pro Knoten isoliert (Permutation),
  in-place erlaubt.
- `ProPhysics_Apply_Signed_Permutation`: dito.
- `ProPhysics_Apply_Guiding_Equation`: schreibt `type_state`, liest
  `amp_grid` — kein Konflikt.
- SU(2)-Leapfrog: pro Link isoliert.

---

## §7 — R1–R7 als Architekturprinzipien

R1–R7 sind nicht nachträglich aufgezwungene Regeln, sondern
**Architektur-Entscheidungen**. Jede Regel hat einen konkreten
Grund im Design.

### §7.1 — R1: Kein div/mod im Hotpath

**Grund:** Division ist auf x86-64 20–40 Takte, Shift ist 1 Takt. Bei
10⁷ Operationen pro Tick macht das den Unterschied zwischen 10 ms und
200 ms.

**Umsetzung:** `grid_dim` muss Zweierpotenz sein. Der Nutzer muss das
garantieren (die API prüft es still).

### §7.2 — R2: Kein malloc im Hotpath

**Grund:** `malloc` ist nicht-deterministisch. Bei Echtzeit-Simulation
oder bei deterministischen Tests ist das inakzeptabel.

**Umsetzung:** Alle Puffer in `Initialize`. `amp_scratch` und
`reg_target` sind die einzigen während des Ticks verwendeten Puffer.

### §7.3 — R3: U5-Invariante bleibt erhalten

**Grund:** U5 ist der **einzige** Erhaltungssatz des Systems. Ohne ihn
ist der Kernel keine Physik-Simulation.

**Umsetzung:** Alle Operationen, die `amp_grid` modifizieren, sind
entweder U5-erhaltend (Transport, Wave, Context) oder U5-neutral
(Dirac-Massenterm ist 2-Knoten-Rotation, auch U5-erhaltend). Die
Messung in `Measure_Amp_Invariant` prüft das.

### §7.4 — R4: Unitäre Dynamik

**Grund:** Die QM-Äquivalenz hängt davon ab, dass die Zeitentwicklung
unitär ist. Nicht-unitäre Schritte würden die Norm nicht erhalten.

**Umsetzung:** Signed Permutations sind per Konstruktion unitär.
2-Knoten-Rotationen sind unitär (cos/sin). Q31-Approximation ist
unitär bis auf Rundung.

### §7.5 — R5: Keine stillen API-Brüche

**Grund:** Die Tests sind der Regression-Anker. Wenn eine
API-Signatur sich ändert, muss der Test mit angepasst werden — sonst
bricht die Regression unsichtbar.

**Umsetzung:** Neue Funktionen kommen **additiv** hinzu. Alte
Signaturen bleiben. Wenn eine Signatur sich ändern **muss**, wird der
betroffene Test gleichzeitig angepasst.

### §7.6 — R6: Jede Etappe endet mit einem Test

**Grund:** Ohne Test ist eine Änderung nicht überprüfbar.

**Umsetzung:** Jede Etappe registriert einen neuen Test in
`alpha_test_main.c` und in `tools/run_alpha_tests.ps1`. Der Prio-All-Lauf
ist der abschließende Regressionstest.

### §7.7 — R7: Keine Etappe ändert bestehende Pfade

**Grund:** Die Prio-1-Tests sind seit Etappe 9 unverändert. Sie testen
das Fundament. Wenn eine neue Etappe den Standard-Pfad ändert, brechen
sie.

**Umsetzung:** Neue Funktionalität läuft in **parallelen** Funktionen
oder wird durch **Flags** aktiviert. Standard-Verhalten bei
`flag == 0` ist bit-identisch.

**Konsequenz:** Der Kernel hat **mehrere Pfade**, die sich gegenseitig
ausschließen. Der Dispatch in `Apply_Amp_Step` ist die Konsequenz.

---

## §8 — Determinismus und Reproduzierbarkeit

### §8.1 — Woher kommt Determinismus?

Drei Quellen:

1. **Integer-Arithmetik im Hotpath.** Kein Floating-Point, außer in
   `Wave_Step` (dortige `cos`/`sin`-Aufrufe sind plattformabhängig,
   aber IEEE-754-standardisiert).
2. **Kein `malloc` im Hotpath.** Keine Allokations-Reihenfolge-Effekte.
3. **Deterministische RNG.** `xoshiro256**` mit explizitem State.

### §8.2 — Wo Determinismus nicht garantiert ist

- **`Wave_Step`** verwendet `cos`/`sin` in `double`. Die libm-
  Implementierung ist auf x86-64 konsistent, aber andere Plattformen
  können leicht abweichen.
- **`Apply_Local_Dephasing_Tick`** verwendet `pro_amp_rotate_q16`, das
  auf einer Q30-Tabelle basiert, die in `pro_trig_init` mit `cos`/`sin`
  gefüllt wird.

### §8.3 — Konsequenz für Tests

Test-Logs sind auf derselben Maschine bit-identisch zwischen Läufen.
Auf verschiedenen Maschinen können sie in den letzten Stellen abweichen.

**Für die Regression:** Die Tests prüfen Größen mit Toleranzen
(`< 5 %`, `< 1e-6`, etc.), nicht bit-identische Werte.

---

## §9 — Erweiterungspunkte

### §9.1 — Wo man neue Physik einbaut

| Erweiterungs-Typ | Wo | Wie |
|---|---|---|
| Neuer Basis-Zustand | `ProUrState` + `PRO_AMP_BASIS_SIZE` | Struct-Änderung + neue Dynamik |
| Neue Kante-Metadaten | `ProEdge` | Struct-Änderung + neue Lese-/Schreib-Funktionen |
| Neue Knoten-Metadaten | `ProNode` (nur `reserved_gating`) | Bit setzen |
| Neue Tick-Dynamik | `ProPhysics_Amp.c` | Neue `Apply_*`-Funktion + Dispatch in `Apply_Amp_Step` |
| Neue Topologie-Änderung | Callback | Neuer `ProPhysics_RuleCallback` |
| Neue Messung | Neues Modul | Neue `ProPhysics_*`-Funktionen |

### §9.2 — Was man **nicht** ändern sollte

- **`amp_grid`-Layout:** 8 Basis-Zustände sind fix. Eine Änderung
  würde alle bestehenden Tests brechen.
- **`reg_source`-Layout:** 16 Kanäle sind fix.
- **Tick-Reihenfolge:** Neue Schritte werden **eingefügt**, nicht
  umsortiert.
- **Q31-Skala:** Alle Amplituden-Operationen sind Q31. Eine Änderung
  würde alle Konstanten brechen.

### §9.3 — Der empfohlene Weg für eine neue Erweiterung

1. **Neuen `ProEdge`-Feld-Block** hinzufügen (analog zu SU(2)).
2. **Neues Modul** für die Erweiterung anlegen.
3. **Dispatch-Flag** in `ProUniverse` hinzufügen.
4. **Dispatch-Check** in `Apply_Amp_Step` einfügen.
5. **Test** in `alpha_test_main.c` registrieren.
6. **Runner**-Eintrag in `tools/run_alpha_tests.ps1`.
7. **Doku** in `Testkatalog.md` und `Project.md`.

**Beispiel SU(2):** Genau so wurde es gemacht. `ProEdge` +4 Felder,
neues Modul `ProPhysics_SU2.c`, `su2_active`-Flag, Dispatch in
`Apply_Amp_Step` (nur bei Dynamik), Test `alpha_test_su2.c`.

---

## §10 — Grenzen der Architektur

### §10.1 — Was die Architektur **nicht** kann

- **Mehr als 16 Kanäle pro Knoten.** `CHANNELS_MAX` ist fix.
- **Mehr als 8 Basis-Zustände.** `PRO_AMP_BASIS_SIZE` ist fix.
- **Gleichzeitige Aktivierung aller Erweiterungen.** Shared-Tick-Reps
  und Dirac-Pfad schließen sich aus.
- **Mehrere gleichzeitige Ticks.** Single-threaded.
- **Atomare Topologie-Änderungen.** Der Callback mutiert lokal, dann
  wird geswappt.

### §10.2 — Was die Architektur **könnte**, aber nicht tut

- **Parallele Ticks.** Kein OpenMP im Kernel.
- **Verteilte Ticks.** Kein MPI.
- **GPU-Beschleunigung.** Keine CUDA/OpenCL.
- **Adaptive Auflösung.** `dim` ist statisch.
- **Persistenz.** Kein Save/Load.

Diese sind bewusst **nicht** implementiert. Der Kernel ist ein
**Forschungs-Prototyp**, kein Produktions-Code.

---

## §11 — Vergleich mit anderen Frameworks

### §11.1 — Was anders ist

| Framework | Datenmodell | Hotpath |
|---|---|---|
| ProPhysics | 8-dim Vektoren, signed perms | Integer Q31 |
| ITensor | MPS/MPO Tensoren | double complex |
| QuSpin | dichte/sparse Matrizen | double complex |
| Qiskit | Gate-Listen | Statevector double |
| MILC | Gitter-Felder (QCD) | double, MPI |
| QDP++ | Gitter-Felder | double, MPI |

**ProPhysics-Unterschied:**

- Integer-Arithmetik statt Floating-Point
- Signed Permutations statt dichte Matrizen
- Union-Find statt Amplituden-Kopie
- Single-threaded
- 36 kLOC, ein Autor

### §11.2 — Was gleich ist

- Gitter-basiert
- Lokale Updates
- Unitäre Dynamik
- Test-getriebene Entwicklung

---

## §12 — Was die Architektur ehrlich nicht ist

- **Keine Quantengravitation.** Kein Konzept von Raumzeitkrümmung.
- **Kein Kontinuumslimes.** Gitter ist fix.
- **Keine echte QCD.** Kein Confinement-Beweis, keine Skalenrenormierung.
- **Keine Universalität im strengen Sinne.** T-Gate fehlt.
- **Keine Produktions-Performance.** Single-threaded, keine GPU.

---

## §13 — Siehe auch

| Thema | Datei |
|---|---|
| API-Referenz | `docs/project/ProPhysics_API.md` |
| SDK-API | `docs/project/SDK_API.md` |
| Projekt-Roadmap | `docs/project/Project.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Runner | `docs/test/run_alpha_tests.md` |
| Build-System | `docs/build/BUILD_SCRIPT.md` |
| Master-Makefile | `docs/build/main/Makefile.md` |

---

**Ende Architektur-Dokument v1.0.**
