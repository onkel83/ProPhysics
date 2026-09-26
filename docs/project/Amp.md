# ProPhysics — Modul: Amp

**Datei:** `docs/project/Amp.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_Amp.c`
**Zweck:** Referenz für das Amp-Modul: unitäre Dynamik auf `amp_grid`.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Amp-Modul ist die **physikalische Kernschicht** des Kernels.
Es implementiert die U1–U5-Dynamik auf dem Amplitudengitter.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut (Rolle im Kernel) |
| §2 | Datenfluss und Abhängigkeiten |
| §3 | Interne Helfer (static, nicht Teil der API) |
| §4 | Öffentliche API (Funktionen und Semantik) |
| §5 | Verwendungsmuster |
| §6 | Fallstricke |
| §7 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/CONFIG.md` — Konstanten (`PRO_DEFAULT_TRANSPORT_THETA_Q15`, …)
- `CHANGELOG.md` — Änderungshistorie

**Konvention:** Funktionen mit Präfix `ProPhysics_` sind öffentlich.
Funktionen mit Präfix `pro_` sind intern (in `ProPhysics_Internal.h`
deklariert, in diesem Modul definiert).

---

## §1 — Was das Amp-Modul tut

Das Amp-Modul ist die **unitäre Dynamik** des Kernels. Es arbeitet
ausschließlich auf `pu->amp_grid` und `pu->amp_scratch`. Es modifiziert
**niemals** `ur_grid` (außer über die Guiding-Gleichung U6) und
**niemals** `edge_phases`.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Signed Permutations** | `Apply_Signed_Permutation`, `Compute_Context_Perm`, `Apply_Context_Tick` |
| **Transport** | `Apply_Edge_Transport`, `_Colored`, `_Colored_3D` |
| **Wave** | `Apply_Wave_Step` |
| **GP-Selbstkopplung** | `Apply_Nonlinear_Phase_Step`, `_Dilated`, `_Spin` |
| **Guiding (U6)** | `Apply_Guiding_Equation` |
| **Invariante (U5)** | `Measure_Amp_Invariant`, `Weighted_Norm` |
| **Dispatch** | `Apply_Amp_Step` |
| **Verifikation** | `Verify_Unitarity`, `Verify_U5_Commutator`, `Verify_Signed_Perm_*` |

**Was das Modul nicht tut:**

- Keine Topologie-Änderungen (`reg_source`, `reg_target`).
- Keine Kantenphasen (`edge_phases`).
- Keine SU(2)-Links (das macht `ProPhysics_SU2*.c`).
- Keine EPR-Propagation (das macht `ProPhysics_EPR.c`).
- Keine Dichte-Matrizen (das macht `ProPhysics_Density.c`).

---

## §2 — Datenfluss und Abhängigkeiten

### §2.1 — Ein-/Ausgänge pro Funktion

| Funktion | liest | schreibt |
|---|---|---|
| `Apply_Signed_Permutation` | `amp_grid` | `amp_grid` |
| `Compute_Context_Perm` | `reg_source`, `amp_grid` | `perm_out`, `sign_out` |
| `Apply_Context_Tick` | `reg_source`, `amp_grid` | `amp_grid` |
| `Apply_Edge_Transport` | `reg_source`, `amp_grid` | `amp_grid` (via scratch) |
| `Apply_Edge_Transport_Colored` | `reg_source`, `amp_grid` | `amp_grid` ↔ `amp_scratch` |
| `Apply_Wave_Step` | `reg_source`, `amp_grid` | `amp_grid` ↔ `amp_scratch` |
| `Apply_Guiding_Equation` | `amp_grid`, `u_field` | `ur_grid.type_state` |
| `Measure_Amp_Invariant` | `amp_grid`, `shared.parent` | — |
| `Apply_Amp_Step` | alles oben | alles oben |

### §2.2 — Abhängigkeiten zu anderen Modulen

| Wird aufgerufen von | Ruft auf |
|---|---|
| `ProPhysics_Tick` (Core) | `Apply_Amp_Step` |
| `Apply_Amp_Step` | `Apply_SU2_Tick` (SU2_Dynamics), `Shared_Tick_Reps` (Shared), `Apply_Dirac_Step` (Dirac), `Tensor_Sync_To_Amp` (Tensor), `TensorDensity_Sync_*` (Density), `Tensor_Apply_XX_Step` / `Tensor_Apply_Hopping` (Tensor) |
| `Apply_Wave_Step` (Amp) | `pro_wave_step_core` (Amp) |
| `Tensor_Apply_Hopping` (Tensor) | `pro_transport_coeffs` (Amp, in Internal.h) |

### §2.3 — Ping-Pong-Puffer

Transport und Wave-Step nutzen `pu->amp_scratch` als Puffer:
Lies aus `amp_grid`, schreibe nach `amp_scratch`, tausche am Ende.
Das verhindert Reihenfolge-Abhängigkeiten.

**Wichtig:** Nach jeder Ping-Pong-Operation zeigt `pu->amp_grid` auf
das andere Array. Wer die Adresse zwischen zwei Aufrufen speichert,
muss sie neu holen.

---

## §3 — Interne Helfer

Diese Funktionen sind `static` und nicht Teil der öffentlichen API.
Sie werden hier dokumentiert, damit die öffentlichen Wrapper lesbar
bleiben.

### §3.1 — `pro_amp_apply_matrix`

```c
static void pro_amp_apply_matrix(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE],
    const ProAmpVector* in,
    ProAmpVector* out);
```

Wendet eine 8×8-Matrix `U` auf einen Amplitudenvektor an:
`out = U · in`. Komplexe Multiplikation mit Saturation.

**Verwendet von:** `ProPhysics_Apply_Unitary_Tick`.

### §3.2 — `pro_amp_rotate_by_phase`

```c
static inline ProAmpQ31 pro_amp_rotate_by_phase(
    ProAmpQ31 a, double phase_rad);
```

Rotiert einen Q31-Wert um `phase_rad` (Radian):
`a' = a · exp(i·phase_rad)` mit Saturation.

**Zentraler Baustein** aller Phase-Steps und des Wave-Steps.
Ersetzt 5 duplizierte Stellen (D1 im Refactoring 23).

**Verwendet von:** `Apply_Wave_Step`, `Apply_Nonlinear_Phase_Step`,
`_Dilated`, `_Spin`.

### §3.3 — `pro_amp_abs2_q62`

```c
static inline double pro_amp_abs2_q62(ProAmpQ31 a, double inv_q62);
```

Liefert `|a|² / 2⁶²` als physikalische Norm (Q62 → 1.0).
Overflow-sicher über `pro_amp_abs2` (uint64).

**Ersetzt** 3 duplizierte Stellen in den Phase-Steps.

### §3.4 — `pro_ensure_grid_shift`

```c
static void pro_ensure_grid_shift(ProUniverse* pu);
```

Stellt `pu->grid_dim_shift` und `pu->grid_dim_mask` aus `pu->grid_dim`
ein, falls noch nicht gesetzt. Idempotent.

**Voraussetzung:** `grid_dim` ist Zweierpotenz (sonst No-Op).

**Wichtig für R1:** Diese Funktion ist die **einzige** Stelle, an der
`log2(grid_dim)` berechnet wird. Alle anderen Hotpath-Funktionen nutzen
das vorberechnete `shift` und `mask`.

### §3.5 — `pro_amp_select_neighbor_channels`

```c
static uint8_t pro_amp_select_neighbor_channels(
    const ProUniverse* pu, uint8_t ch_arr[6]);
```

Liefert die Anzahl (4 oder 6) und füllt `ch_arr` mit den Kanälen:
- 2D: `{0, 1, 2, 3}` (4 Nachbarn)
- 3D: `PRO_NEIGHBOR_CHANNELS[0..5]` (6 Nachbarn)

**Verwendet von:** `Apply_Wave_Step`, `Apply_Amp_Step` (Wave-Schritt).

### §3.6 — `pro_amp_auto_sync`

```c
static void pro_amp_auto_sync(ProUniverse* pu);
```

Synchronisiert optional Tensor-Paare (`auto_sync_tensor`) und
Dichte-Trilogie (`auto_sync_density`) nach dem Tick.

**Ersetzt** zwei identische Blöcke in `Apply_Amp_Step` (Standard- und
Dirac-Pfad).

### §3.7 — `pro_transport_sequential_core`

```c
static void pro_transport_sequential_core(
    ProUniverse* pu,
    int64_t c_num, int64_t s_num, int64_t denom);
```

Sequenzieller Transport: für jede Kante `(k, nb)` mit `nb > k` wird
eine 2-Knoten-Rotation einmal angewendet. Kein Schachbrett-Muster.

**Nachteil:** Trotter-Fehler ~1,7e-4 pro Tick.
**Vorteil:** Einfacher, keine 2D-Koordinaten-Berechnung nötig.

**Verwendet von:** `Apply_Edge_Transport`, `Apply_Amp_Step` (Fallback
ohne `grid_dim`).

### §3.8 — `pro_transport_colored_core`

```c
static void pro_transport_colored_core(
    ProUniverse* pu,
    int64_t c_num, int64_t s_num, int64_t denom,
    uint32_t grid_dim, uint32_t dim_shift, uint32_t dim_mask,
    const uint8_t* sweep_channels, uint8_t n_sweeps,
    int use_3d_parity,
    const uint8_t* skip_marks);
```

Parametrisierter farbiger Transport (Schachbrett-Muster).

| Parameter | Bedeutung |
|---|---|
| `sweep_channels` | Kanal pro Sweep (Länge `n_sweeps`) |
| `n_sweeps` | 4 (2D) oder 6 (3D) |
| `use_3d_parity` | 1 = `(x+y+z) & 1`, 0 = `(x+y) & 1` |
| `skip_marks` | `NULL` oder `tensor_marks` (überspringt markierte Knoten) |

**Vorteil:** Exakt unitär (Trotter-Fehler entfällt).
**Verwendet von:** `Apply_Edge_Transport_Colored[_3D]`,
`Apply_Amp_Step` (Standard- und Marks-Pfad).

### §3.9 — `pro_wave_step_core`

```c
static void pro_wave_step_core(
    ProUniverse* pu,
    uint32_t phase_step_q15,
    uint8_t n_nb, const uint8_t* ch_arr,
    const uint8_t* skip_marks);
```

Unitärer Wave-Step. Rotiert jeden Basis-Koeffizienten um
`d_theta = s · local_energy / norm_sq`, wobei `local_energy` die
Summe der Nachbar-Amplituden einbezieht.

| Parameter | Bedeutung |
|---|---|
| `n_nb` | 4 (2D) oder 6 (3D) |
| `ch_arr` | Nachbar-Kanäle |
| `skip_marks` | `NULL` oder `tensor_marks` |

**Verwendet von:** `Apply_Wave_Step`, `Apply_Amp_Step` (Wave-Schritt).

---

## §4 — Öffentliche API

### §4.1 — Signed Permutations

#### `ProPhysics_Apply_Signed_Permutation`

```c
PROPHYSICS_API void ProPhysics_Apply_Signed_Permutation(
    ProUniverse* pu,
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE]);
```

Wendet eine signierte Permutation auf **alle** Knoten an:
`dst[i] = sign[i] · src[perm[i]]`.

**Mathematik:** Signed Permutations sind **exakt unitär** (keine
Q31-Rundung). Die U5-Invariante ist exakt erhalten (Drift = 0).

**Vorbedingung:** `perm` ist Bijektion, `sign ∈ {−1, +1}`.
Prüfung via `Verify_Signed_Perm_Unitarity`.

**Beispiel:**

```c
uint8_t perm[8] = {0, 2, 1, 4, 3, 5, 6, 7};   /* swap 1↔2, 3↔4 */
int8_t  sign[8] = {1, -1, 1, -1, 1, 1, 1, 1};  /* Vorzeichen auf 1, 3 */
ProPhysics_Apply_Signed_Permutation(&pu, perm, sign);
```

#### `ProPhysics_Verify_Signed_Perm_Unitarity`

```c
PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_Unitarity(
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE]);
```

Prüft: `perm` ist Bijektion, `sign ∈ {−1, +1}`.
Liefert `true` bei Erfolg.

#### `ProPhysics_Verify_Signed_Perm_U5`

```c
PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_U5(
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE]);
```

Prüft: U5-Kompatibilität — `PRO_U5_W[perm[i]] == PRO_U5_W[i]` für alle i.

#### `ProPhysics_Compute_Context_Perm`

```c
PROPHYSICS_API void ProPhysics_Compute_Context_Perm(
    const ProUniverse* pu,
    uint64_t node_idx,
    uint8_t  perm_out[PRO_AMP_BASIS_SIZE],
    int8_t   sign_out[PRO_AMP_BASIS_SIZE]);
```

Berechnet die **kontextabhängige** Permutation für einen Knoten.
Der Hash basiert auf den Amplituden der Nachbar-Knoten
(deterministisch, splitmix64).

**Wichtig:** Ergebnis hängt vom aktuellen `amp_grid`-Zustand ab.
Zwei Knoten mit demselben Nachbarkontext erhalten dieselbe Permutation.

#### `ProPhysics_Apply_Context_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_Context_Tick(ProUniverse* pu);
```

Wendet `Compute_Context_Perm` auf **alle** Knoten an. Äquivalent zu
einem Tick, in dem jeder Knoten eine andere signed permutation erfährt.

**Hotpath:** R1, R2 (kein div/mod, kein malloc).

### §4.2 — Transport

#### `ProPhysics_Apply_Edge_Transport`

```c
PROPHYSICS_API void ProPhysics_Apply_Edge_Transport(
    ProUniverse* pu, uint32_t theta_q15);
```

Sequenzieller Transport (kein Schachbrett). Nutzt Kanäle 0..3.
Nur sinnvoll bei nicht-2D-Gitter (`grid_dim == 0`).

**Trotter-Fehler:** ~1,7e-4 pro Tick. Für Unitariätstests reicht das
nicht (siehe `test_amp_invariance_bisect`).

#### `ProPhysics_Apply_Edge_Transport_Colored`

```c
PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim);
```

2D-Transport mit 4-Sweep-Schachbrett. **Exakt unitär**.
Nutzt die 4 Kanäle `{+y, +y, +x, +x}` mit alternierender Parität.

**Voraussetzung:** `grid_dim` Zweierpotenz.

#### `ProPhysics_Apply_Edge_Transport_Colored_3D`

```c
PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored_3D(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim);
```

3D-Transport mit 6-Sweep-Schachbrett. **Exakt unitär**.
Nutzt die 6 Kanäle `{+x, +x, +y, +y, +z, +z}`.

**Voraussetzung:** `grid_dim` Zweierpotenz.

### §4.3 — Wave-Step

#### `ProPhysics_Apply_Wave_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Wave_Step(
    ProUniverse* pu, uint32_t phase_step_q15);
```

Unitärer Wave-Step. Für jeden Koeffizienten `c_b(k)`:

```
L        = −n_nb · c_b(k) + Σ_nachbarn c_b(nb)
d_theta  = s · (Re(c)·Re(L) + Im(c)·Im(L)) / |c|²
c_b'(k)  = c_b(k) · exp(i · d_theta)
```

`s = phase_step_q15 / 32768`.

**Wichtig:** Verwendet `cos`/`sin` (Floating-Point). Damit ist die
Funktion **nicht bit-deterministisch** über Plattformen hinweg, aber
IEEE-754-stabil auf x86-64.

**Hotpath:** R2 (kein malloc), aber `cos`/`sin` pro Koeffizient.

### §4.4 — Guiding (U6)

#### `ProPhysics_Apply_Guiding_Equation`

```c
PROPHYSICS_API void ProPhysics_Apply_Guiding_Equation(ProUniverse* pu);
```

Rekonstruiert `type_state` aus den Amplituden:

```
p_pos = |c_1|² + |c_2|²
p_neg = |c_3|² + |c_4|²
type_state = (u_field[k] · (p_pos + p_neg) < 2¹⁶ · p_pos)
    ? UR_POSITRON_CW : UR_NEGATRON_CCW
```

**Kein RNG.** `u_field` ist deterministisch aus `Initialize`.

**Wird intern von** `ProPhysics_Tick` **aufgerufen.**

### §4.5 — Invariante (U5)

#### `ProPhysics_Measure_Amp_Invariant`

```c
PROPHYSICS_API ProU128 ProPhysics_Measure_Amp_Invariant(
    const ProUniverse* pu);
```

Berechnet die U5-Invariante:

```
Σ_k Σ_b PRO_U5_W[b] · |c_b(k)|²
```

**Klassen-Regel:** Bei aktivem `shared.active` wird nur der
Repräsentant jeder Klasse gezählt.

**Overflow-sicher:** `ProU128`.

#### `ProPhysics_Weighted_Norm`

```c
PROPHYSICS_API ProU128 ProPhysics_Weighted_Norm(
    const ProUniverse* pu, uint64_t node_idx);
```

U5-Norm eines einzelnen Knotens.

#### `ProPhysics_Get_U_Field`

```c
PROPHYSICS_API const uint16_t* ProPhysics_Get_U_Field(
    const ProUniverse* pu);
```

Direkter Lesezugriff auf `u_field` (Q16-Werte pro Knoten).

### §4.6 — Verifikation

#### `ProPhysics_Verify_Unitarity`

```c
PROPHYSICS_API double ProPhysics_Verify_Unitarity(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);
```

Prüft `U · U† = I`. Rückgabe: `max_err` (double). Idealwert: 0.

#### `ProPhysics_Verify_U5_Commutator`

```c
PROPHYSICS_API double ProPhysics_Verify_U5_Commutator(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);
```

Prüft, ob `U` die U5-Gewichte erhält: Für alle `i, j` mit
`PRO_U5_W[i] != PRO_U5_W[j]` muss `U[i][j] == 0` sein.

**Rückgabe:** `max_err` (double). Idealwert: 0.

### §4.7 — Unitary Tick (Test-Harness)

#### `ProPhysics_Apply_Unitary_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_Unitary_Tick(
    ProUniverse* pu,
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);
```

Wendet eine **statische** 8×8-Matrix auf alle Knoten an:
`amp_grid[k] = U · amp_grid[k]`.

**Verwendet nur in Tests.** Der Standard-Tick nutzt keine statische
Matrix, sondern kontextabhängige signed permutations.

### §4.8 — Dispatch

#### `ProPhysics_Apply_Amp_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Amp_Step(
    ProUniverse* pu, uint32_t phase_step_q15);
```

**Hauptfunktion des Ticks.** Wird von `ProPhysics_Tick` aufgerufen.

**Dispatch-Reihenfolge:**

| Priorität | Bedingung | Wirkung |
|:-:|---|---|
| 1 | `su2_dynamics_active` | `Apply_SU2_Tick` (parallel) |
| 2 | `shared.active` | `Shared_Tick_Reps` + **return** |
| 3 | `dirac_active` | `Apply_Dirac_Step` + Auto-Sync + **return** |
| 4 | Standard | Context → Transport → Wave → Tensor → Auto-Sync |

**Priorität 1 läuft parallel**, weil SU(2)-Dynamik nur `ProEdge`
modifiziert, nicht `amp_grid`.

**Prioritäten 2 und 3 sind Rückkehr-Pfade** (vollständige Alternativen
zum Standard-Pfad).

**R7-Konformität:** Bei allen Flags == 0 ist der Standard-Pfad
bit-identisch zum Zustand vor den Flag-Erweiterungen.

### §4.9 — GP-Selbstkopplung

#### `ProPhysics_Apply_Nonlinear_Phase_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step(
    ProUniverse* pu,
    int32_t  g_q15_signed,
    uint32_t phase_step_q15);
```

GP-Selbstkopplung: `phase_b = g · |c_b|² · dt` für alle Basis-Komponenten.

**Verwendung:** Soliton-Tests, Breather-Experimente.

#### `ProPhysics_Apply_Nonlinear_Phase_Step_Dilated`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(
    ProUniverse* pu,
    int32_t  g_q15_signed,
    uint32_t phase_step_q15);
```

GP-Selbstkopplung mit Lorentz-Dilatation. Der Faktor `γ⁻¹` wird aus
dem lokalen Phasengradienten berechnet:

```
γ⁻¹ = √(1 − β²),  β² = sin²(kx/2) + sin²(ky/2)
```

wobei `kx, ky` die Phasendifferenzen zu `+x`- und `+y`-Nachbarn sind.

**Nur 2D.**

**Verwendung:** Lorentz-Zeitdilatations-Test.

#### `ProPhysics_Apply_Nonlinear_Phase_Step_Spin`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Spin(
    ProUniverse* pu,
    int32_t  g_spin_q15_signed,
    uint32_t phase_step_q15);
```

Spin-abhängige Phase auf `{UR_POSITRON_CW (up), UR_POSITRON_CCW (down)}`:

```
phase_up   = +g_spin · |c_up|² · dt
phase_down = −g_spin · |c_down|² · dt
```

**Verwendung:** Spin-1/2-Emergenz-Test (g-Faktor = 2).

---

## §5 — Verwendungsmuster

### §5.1 — Voller Tick (Standard)

```c
ProUniverse pu;
ProPhysics_Initialize(&pu, 64 * 64);
pu.grid_dim = 64;
pu.grid_ndim = 2;

/* Topologie aufbauen ... */

for (int t = 0; t < 100; ++t) {
    ProPhysics_Tick(&pu, NULL);   /* ruft Apply_Amp_Step intern */
}
```

### §5.2 — Manueller Einzelschritt (Test/Debug)

```c
/* Nur den Wave-Step, ohne Transport */
ProPhysics_Apply_Wave_Step(&pu, 1000);

/* Nur Transport */
ProPhysics_Apply_Edge_Transport_Colored(&pu, 500, 64);

/* Nur Context */
ProPhysics_Apply_Context_Tick(&pu);
```

**Achtung:** Diese Aufrufe umgehen den Dispatch in `Apply_Amp_Step`.
Der nächste `ProPhysics_Tick` läuft normal weiter.

### §5.3 — U5-Prüfung nach jedem Tick

```c
const ProU128 before = ProPhysics_Measure_Amp_Invariant(&pu);
ProPhysics_Tick(&pu, NULL);
const ProU128 after = ProPhysics_Measure_Amp_Invariant(&pu);

const double drift = fabs(pro_u128_to_double(after) -
                          pro_u128_to_double(before))
                     / pro_u128_to_double(before);
printf("U5-Drift: %.4e\n", drift);
```

### §5.4 — Signed-Permutation prüfen

```c
uint8_t perm[8] = {0, 2, 1, 4, 3, 5, 6, 7};
int8_t  sign[8] = {1, -1, 1, -1, 1, 1, 1, 1};

if (!ProPhysics_Verify_Signed_Perm_Unitarity(perm, sign)) {
    /* perm ist keine Bijektion oder sign außerhalb {−1, +1} */
}
if (!ProPhysics_Verify_Signed_Perm_U5(perm, sign)) {
    /* Permutation verletzt U5-Gewichte */
}
ProPhysics_Apply_Signed_Permutation(&pu, perm, sign);
```

### §5.5 — Kontext-Permutation inspizieren

```c
uint8_t perm[8];
int8_t  sign[8];
ProPhysics_Compute_Context_Perm(&pu, 42, perm, sign);

printf("Knoten 42:\n");
for (int i = 0; i < 8; ++i) {
    printf("  coeff[%d] <- %+d * coeff[%d]\n",
           i, sign[i], perm[i]);
}
```

### §5.6 — Auto-Sync aktivieren

```c
ProPhysics_Tensor_Set_Auto_Sync(&pu, 1);
ProPhysics_Density_Set_Auto_Sync(&pu, 1);

/* Jeder Tick synchronisiert jetzt Tensor und Dichte automatisch. */
ProPhysics_Tick(&pu, NULL);
```

---

## §6 — Fallstricke

### §6.1 — Ping-Pong-Puffer

Nach jeder Transport-/Wave-Operation ist `pu->amp_grid` **nicht
mehr** das ursprüngliche Array. Wer einen Zeiger gespeichert hat,
muss ihn neu holen:

```c
/* FALSCH: */
ProAmpVector* p = pu.amp_grid;
ProPhysics_Apply_Edge_Transport_Colored(&pu, 500, 64);
/* p zeigt jetzt auf amp_scratch */

/* RICHTIG: */
ProPhysics_Apply_Edge_Transport_Colored(&pu, 500, 64);
ProAmpVector* p = pu.amp_grid;   /* jetzt aktuell */
```

### §6.2 — `grid_dim` muss Zweierpotenz sein

`Apply_Edge_Transport_Colored[_3D]` erfordern Zweierpotenz. Ohne sie
ist das Verhalten undefiniert (silent return bei 3D, falscher Index
bei 2D).

**Empfehlung:** Immer `grid_dim ∈ {16, 32, 64, 128}` verwenden.

### §6.3 — `phase_step_q15 == 0`

Alle Phase-Steps und der Wave-Step sind No-Op bei `phase_step_q15 == 0`.
Das ist **korrekt**, kann aber bei Tests zu Verwirrung führen
(Erwartung: "Tick läuft", Realität: "nur Transport").

### §6.4 — `g_q15_signed == 0`

Gleiches für die GP-Steps: `g == 0` ist No-Op.

### §6.5 — `norm_sq < 1.0` im Wave-Step

Der Wave-Step prüft `|c|² < 1` und überspringt solche Koeffizienten
(Division-By-Zero-Schutz). Bei sehr kleinen Amplituden ist der
Wave-Step dort ein No-Op.

### §6.6 — `tensor_marks` und Wave-Step

Wenn `pu->tensor_marks != NULL`:
- Markierte Knoten werden nicht rotiert.
- Nachbarn zählen nicht zur lokalen Energie.
- `Apply_Amp_Step` nutzt den Marks-Pfad **automatisch**.

### §6.7 — Reihenfolge in `Apply_Amp_Step`

Die Reihenfolge **Context → Transport → Wave → Tensor → Sync** ist
nicht beliebig. Sie wurde empirisch als stabilste Reihenfolge
bestimmt:

- Context zuerst: verändert die Basiszuordnung, bevor Energie berechnet wird.
- Transport vor Wave: räumliche Ausbreitung vor lokaler Rotation.
- Wave vor Tensor: lokale Energie nutzt aktuelle Amplituden.
- Sync zuletzt: alle Änderungen sichtbar.

**Nicht ändern ohne Test-Regression.**

---

## §7 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Physik-Übersicht | `docs/physics/README.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/ProPhysics_Amp.c` |
| Interne Deklarationen | `src/prophysics/header/ProPhysics_Internal.h` |

---

**Ende Amp.md v1.23.0.**