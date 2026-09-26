# ProPhysics — Modul-Referenz: Tensor

**Datei:** `docs/project/Tensor.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_Tensor.c`
**Zweck:** Referenz für das Tensor-Modul: 2-Knoten-Verschränkung in
Q31, Partial Traces, Sync zu/aus `amp_grid`, Gatter, Fermionen-Adapter.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Tensor-Modul behandelt **2-Knoten-Verschränkung**. Tensor-Paare
sind isolierte Objekte (kein globaler `amp_grid`-Zustand), die auf
Wunsch mit `amp_grid` synchronisiert werden.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Basis-Layout und Konventionen |
| §3 | Physikalische Semantik (Partial Traces, Sync) |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Density.md` — Dichte-Trilogie (nutzt `pro_jacobi_sym_eigen`)
- `docs/project/Fock.md` — Fermionen-Raum (Tensor-Adapter)
- `docs/project/SU2.md` — SU(2)-Links (Spin-Rotation)
- `docs/project/CONFIG.md` — Konstanten (`PRO_TENSOR_DIM` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das Tensor-Modul behandelt **2-Knoten-Systeme**. Es speichert den
Zustand als 64-dimensionalen komplexen Vektor in Q31 (2^31−1 = 1.0),
mit Basis-Layout `coeff[a·8 + b]` — `a` = Knoten A, `b` = Knoten B.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Jacobi-Eigenzerlegung** | `pro_jacobi_sym_eigen` (shared mit `Density.c`) |
| **Lifecycle** | `Tensor_Create_Pair`, `Destroy_Pair`, `Set_State`, `Get_State` |
| **Lokale Operatoren** | `Tensor_Apply_Local_Op` (8×8) |
| **Partial Traces** | `Tensor_Partial_Trace_A`, `_B` |
| **Entropie** | `Von_Neumann_Entropy`, `Tensor_Entanglement_Entropy` |
| **Projektive Messung** | `Tensor_Measure_Projective` |
| **Sync** | `Tensor_Sync_To_Amp`, `Tensor_Sync_From_Amp` |
| **1-Qubit-Gatter** | Hadamard, X, Y, Z (je als Wrapper um `Apply_Single_Qubit_Gate`) |
| **2-Qubit-Gatter** | CNOT, CZ, SWAP, √SWAP (je als Wrapper um `Apply_Two_Qubit_Gate`) |
| **Verschränkungsmaß** | `Tensor_Concurrence` |
| **Mark/Sync** | `Tensor_Mark_Nodes`, `Unmark_Nodes`, `Is_Node_Marked`, `Set_Auto_Sync`, `Sync_Marked_To_Amp` |
| **XX-Kopplung** | `Tensor_Set_Coupling`, `Get_Coupling`, `Apply_XX_Step` |
| **SU(2)-Spin** | `Apply_SU2_Rotation`, `Measure_Spin`, `Verify_SU2_Algebra` |
| **Fermionen** | `Is_Antisymmetric`, `Pauli_Violation`, `Antisymmetrize`, `Fermionize`, `Set_Slater`, `Apply_Hopping`, `Set_Hopping`, `Get_Hopping` |
| **Fock-Adapter** | `Tensor_To_Fock`, `Fock_To_Tensor` |

**Was das Modul nicht tut:**

- Kein Sampling außerhalb von `Tensor_Measure_Projective`.
- Kein Tick — Tensor-Operationen sind explizit aufzurufen.
- Keine globalen Amplituden — nur 2-Knoten-Blöcke.

**Aktivierung:** Keine. Alle Operationen sind explizit.

---

## §2 — Basis-Layout und Konventionen

### §2.1 — 2-Qubit-Basis

Der Tensor-Zustand ist ein komplexer 64-Vektor:
`coeff[a·8 + b]` mit `a, b ∈ [0, 8)`.

`a` ist der Basiszustand von Knoten A, `b` von Knoten B.

### §2.2 — Qubit-Unterbasis

Die **Qubit-Basis** ist `PRO_QUBIT_BASIS[2] = {1, 4}` — d.h.
`UR_POSITRON_CW` und `UR_NEGATRON_CCW`.

Die Gates Hadamard, X, Y, Z, CNOT, CZ, SWAP, √SWAP wirken **nur** auf
dieser 2-dimensionalen Unterebene. Die übrigen 6 Basiszustände sind
Zuschauer.

### §2.3 — Skala

Q31: `INT32_MAX = 2^31−1 = 1.0`.

Q62: `2^62 = 1.0` (nur in `Tensor_Antisymmetrize`-Rückgabe).

### §2.4 — Antisymmetrie-Konvention

Für Fermionen-Zustände gilt `psi[a, b] = −psi[b, a]`. Die Diagonale
ist 0 (`psi[a, a] = 0`, Pauli-Prinzip).

### §2.5 — Speicherort

Tensor-Paare werden in `pu->tensor_pairs` gehalten (Kapazität
`tensor_pair_capacity`). Jeder Slot hat ein `active`-Flag.

---

## §3 — Physikalische Semantik

### §3.1 — Partial Trace

Für einen 2-Qubit-Zustand `psi[a, b]`:

```
ρ_A[a][a'] = Σ_b psi[a, b] · conj(psi[a', b])
ρ_B[b][b'] = Σ_a psi[a, b] · conj(psi[a, b'])
```

Beide Operationen sind **rein lesend** und liefern eine 8×8-Hermitesche
Matrix in Q31.

### §3.2 — Von-Neumann-Entropie

```
S(ρ) = −Σ_k μ_k · log(μ_k)
```

mit den Eigenwerten `μ_k` der **reellen 16×16-Einbettung** von `ρ`.

**Faktor 0.5** in `S -= 0.5 * mu * log(mu)`: die 16×16-Realisierung
dupliziert jeden Eigenwert, daher die Halbierung.

### §3.3 — Sync-Richtungen

| Richtung | Was passiert | Wofür |
|---|---|---|
| `Sync_To_Amp` | Tensor → amp_grid (dominanter Eigenvektor) | Marginale eines verschränkten Paares nach `amp_grid` |
| `Sync_From_Amp` | amp_grid → Tensor (äußeres Produkt) | Aus zwei unabhängigen Knoten einen Produktzustand bauen |

### §3.4 — Messung

`Tensor_Measure_Projective` ist die **echte Born-Regel** im
Projektor-Unterraum `{|1,1>, |1,4>, |4,1>, |4,4>}`:

```
p(σ_a, σ_b | θ_a, θ_b) = |⟨θ_a, σ_a; θ_b, σ_b | psi⟩|²
```

Sampling via xoshiro256\*\*.

**Rein lesend.** `p_total < 0.5` ⇒ `false` (Zustand im
Projektor-Unterraum ausgelöscht).

### §3.5 — Fermionen

`Set_Slater(i, j)` erzeugt:

```
psi[i, j] = +1/√2
psi[j, i] = −1/√2
```

`Apply_Hopping(i, j, θ)` wirkt als `U(θ) = exp(−iθH)` mit
`H = c†_i c_j + c†_j c_i` im antisymmetrischen Sektor.

`Fermionize` = Antisymmetrisieren + Normieren.

---

## §4 — Interne Helfer

### §4.1 — `pro_tensor_cmul_acc`

```c
static inline void pro_tensor_cmul_acc(
    int64_t ar, int64_t ai, int64_t br, int64_t bi,
    int64_t* acc_re, int64_t* acc_im);
```

`acc += a · b` in Q31 (Rundung pro Term vor Akkumulation).

**Ersetzt** 8× inline-c-Mul in `Apply_Local_Op`, `Apply_Single_Qubit_Gate`,
`Apply_Two_Qubit_Gate`, `Sync_From_Amp`, `Apply_SU2_Rotation`.

### §4.2 — `pro_tensor_cmul_conj_acc`

```c
static inline void pro_tensor_cmul_conj_acc(
    int64_t ar, int64_t ai, int64_t br, int64_t bi,
    int64_t* acc_re, int64_t* acc_im);
```

`acc += a · conj(b)` in Q31.

**Ersetzt** 2× inline-c-Mul in `Partial_Trace_A` und `_B`.

### §4.3 — `pro_rho_to_real16`

```c
static void pro_rho_to_real16(
    const ProAmpQ31 rho[8][8], double M[16][16]);
```

Bettet eine 8×8 komplexe Q31-Dichtematrix in eine reelle
16×16-Matrix ein. Für Jacobi-Eigenzerlegung.

**Verwendung:** `Von_Neumann_Entropy`, `Sync_To_Amp`.

### §4.4 — `pro_write_dominant_eigvec`

```c
static void pro_write_dominant_eigvec(
    ProUniverse* pu, uint64_t node, double M[16][16]);
```

Diagonalisiert `M` (in-place), findet den dominanten Eigenwert und
schreibt den zugehörigen Eigenvektor in `amp_grid[node]`.

### §4.5 — `pro_tensor_get` / `pro_tensor_set`

Zugriff auf `coeff[pro_tensor_idx(a, b)]` als `double`-Paar
(Real- und Imaginärteil).

### §4.6 — `pro_pair_to_bits`

```c
static inline uint8_t pro_pair_to_bits(int i, int j);
```

`(1 << i) | (1 << j)` — Umrechnung Orbital-Paar → Fock-Besetzungsmaske.

---

## §5 — Öffentliche API

### §5.1 — Jacobi-Eigenzerlegung

```c
void pro_jacobi_sym_eigen(double A[16][16], double V[16][16], int n);
```

Reelle symmetrische n×n-Matrix (n ≤ 16), in-place. Bei `V != NULL`
werden die Eigenvektoren akkumuliert.

**Shared mit `Density.c`** — kein `PROPHYSICS_API`-Makro, aber
externes Linkage.

### §5.2 — Lifecycle

```c
PROPHYSICS_API bool ProPhysics_Tensor_Create_Pair(
    ProUniverse* pu, uint64_t node_a, uint64_t node_b, uint32_t* out_pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Destroy_Pair(
    ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Set_State(
    ProUniverse* pu, uint32_t pair_id, const ProAmpQ31 coeff[PRO_TENSOR_DIM]);
PROPHYSICS_API bool ProPhysics_Tensor_Get_State(
    const ProUniverse* pu, uint32_t pair_id, ProAmpQ31 coeff_out[PRO_TENSOR_DIM]);
```

Initialer Zustand nach `Create_Pair`: `|0⟩⊗|0⟩` (`coeff[0] = INT32_MAX`).

### §5.3 — Lokale Operatoren

```c
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Local_Op(
    ProUniverse* pu, uint32_t pair_id, int side,
    const ProAmpQ31 U[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
```

`side == 0` ⇒ `U ⊗ I`, `side == 1` ⇒ `I ⊗ U`.

### §5.4 — Partial Traces und Entropie

```c
PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_A(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_B(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
PROPHYSICS_API double ProPhysics_Von_Neumann_Entropy(
    const ProAmpQ31 rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
PROPHYSICS_API double ProPhysics_Tensor_Entanglement_Entropy(
    const ProUniverse* pu, uint32_t pair_id);
```

### §5.5 — Messung

```c
PROPHYSICS_API bool ProPhysics_Tensor_Measure_Projective(
    const ProUniverse* pu, uint32_t pair_id,
    double theta_a, double theta_b,
    uint64_t rng[4], int* out_a, int* out_b);
```

**Echte Born-Regel** im Projektor-Unterraum. `rng` muss 4
`uint64_t` enthalten.

### §5.6 — Sync

```c
PROPHYSICS_API bool ProPhysics_Tensor_Sync_To_Amp(ProUniverse* pu);
PROPHYSICS_API bool ProPhysics_Tensor_Sync_From_Amp(
    ProUniverse* pu, uint32_t pair_id);
```

`Sync_To_Amp` iteriert über alle aktiven Paare und schreibt je die
dominanten Eigenvektoren von `ρ_A` nach `node_a` und von `ρ_B` nach
`node_b`.

### §5.7 — Gatter

```c
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Single_Qubit_Gate(
    ProUniverse* pu, uint32_t pair_id, int side,
    const ProAmpQ31 U[2][2]);
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Two_Qubit_Gate(
    ProUniverse* pu, uint32_t pair_id, const ProAmpQ31 G[4][4]);
```

**Convenience-Wrapper:** `Hadamard`, `X`, `Y`, `Z`, `CNOT`, `CZ`,
`SWAP`, `SqrtSwap`. Alle wirken nur auf `PRO_QUBIT_BASIS = {1, 4}`.

### §5.8 — Concurrence

```c
PROPHYSICS_API double ProPhysics_Tensor_Concurrence(
    const ProUniverse* pu, uint32_t pair_id);
```

`C = 2·|c_00·c_11 − c_01·c_10| / Σ|c|²` in der Qubit-Unterebene.

### §5.9 — Mark/Sync

```c
PROPHYSICS_API bool ProPhysics_Tensor_Mark_Nodes(ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Unmark_Nodes(ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Is_Node_Marked(const ProUniverse* pu, uint64_t node);
PROPHYSICS_API void ProPhysics_Tensor_Set_Auto_Sync(ProUniverse* pu, int enable);
PROPHYSICS_API bool ProPhysics_Tensor_Sync_Marked_To_Amp(ProUniverse* pu);
```

### §5.10 — XX-Kopplung

```c
PROPHYSICS_API bool ProPhysics_Tensor_Set_Coupling(
    ProUniverse* pu, uint32_t pair_id, int32_t theta_q15);
PROPHYSICS_API int32_t ProPhysics_Tensor_Get_Coupling(
    const ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Apply_XX_Step(
    ProUniverse* pu, uint32_t pair_id);
```

`U(θ) = cos(θ)·I − i·sin(θ)·σx⊗σx`.

### §5.11 — SU(2)-Spin

```c
PROPHYSICS_API bool ProPhysics_Apply_SU2_Rotation(
    ProUniverse* pu, uint64_t node, int which_spinor,
    double nx, double ny, double nz, double alpha);
PROPHYSICS_API bool ProPhysics_Measure_Spin(
    const ProUniverse* pu, uint64_t node, int which_spinor,
    double* out_sx, double* out_sy, double* out_sz);
PROPHYSICS_API double ProPhysics_Verify_SU2_Algebra(void);
```

`Apply_SU2_Rotation` wirkt auf `{PRO_SPINOR_UP[which_spinor],
PRO_SPINOR_DN[which_spinor]}` in `amp_grid[node]`.

### §5.12 — Fermionen

```c
PROPHYSICS_API bool ProPhysics_Tensor_Is_Antisymmetric(
    const ProUniverse* pu, uint32_t pair_id, int32_t tol_q31);
PROPHYSICS_API double ProPhysics_Tensor_Pauli_Violation(
    const ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API double ProPhysics_Tensor_Antisymmetrize(
    ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Fermionize(
    ProUniverse* pu, uint32_t pair_id);
PROPHYSICS_API bool ProPhysics_Tensor_Set_Slater(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_a, uint8_t orbital_b);
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Hopping(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
PROPHYSICS_API bool ProPhysics_Tensor_Set_Hopping(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
PROPHYSICS_API bool ProPhysics_Tensor_Get_Hopping(
    const ProUniverse* pu, uint32_t pair_id,
    uint8_t* out_i, uint8_t* out_j, int32_t* out_theta_q15);
```

`Antisymmetrize` liefert die **neue** Norm² in Q62 zurück.
`Fermionize` = `Antisymmetrize` + Normieren.

### §5.13 — Fock-Adapter

```c
PROPHYSICS_API bool ProPhysics_Tensor_To_Fock(
    ProUniverse* pu, uint32_t pair_id, uint64_t* out_fock_id);
PROPHYSICS_API bool ProPhysics_Fock_To_Tensor(
    const ProUniverse* pu, uint64_t fock_id, uint32_t pair_id);
```

**Randbedingungen:**

- `Tensor_To_Fock`: Tensor muss antisymmetrisch sein (Toleranz 1000).
- `Fock_To_Tensor`: Fock-State darf nur Popcounts = 2 haben.

---

## §6 — Verwendungsmuster

### §6.1 — Bell-Zustand präparieren

```c
uint32_t pid = 0;
ProPhysics_Tensor_Create_Pair(&pu, /*a*/ 0u, /*b*/ 64u, &pid);

ProPhysics_Tensor_Apply_Hadamard(&pu, pid, /*side*/ 0);
ProPhysics_Tensor_Apply_CNOT(&pu, pid, /*control*/ 0);

const double c = ProPhysics_Tensor_Concurrence(&pu, pid);
/* c = 1.0 (maximal verschränkt) */
```

### §6.2 — Entropie messen

```c
const double S = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
/* S = log(2) ≈ 0.693 für Bell-Zustand */
```

### §6.3 — Tensor in amp_grid schreiben

```c
ProPhysics_Tensor_Mark_Nodes(&pu, pid);
ProPhysics_Tensor_Sync_To_Amp(&pu);
/* amp_grid[0] und amp_grid[64] enthalten jetzt die Marginalen */
```

### §6.4 — Aus amp_grid einen Produktzustand bauen

```c
ProPhysics_Tensor_Sync_From_Amp(&pu, pid);
/* coeff[a*8+b] = amp_grid[node_a].coeff[a] * amp_grid[node_b].coeff[b] */
```

### §6.5 — Slater-Determinante

```c
ProPhysics_Tensor_Set_Slater(&pu, pid, /*i*/ 0u, /*j*/ 1u);
/* psi[0,1] = +1/√2, psi[1,0] = -1/√2 */
```

### §6.6 — Fermionisches Hopping

```c
const int32_t theta_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
ProPhysics_Tensor_Apply_Hopping(&pu, pid, /*i*/ 0u, /*j*/ 1u, theta_pi4);
```

### §6.7 — Projektive Messung

```c
uint64_t rng[4] = { 0xDEADBEEFull, 0xCAFEBABEull, 0x12345678ull, 0xABCDEF01ull };
int a = 0, b = 0;
ProPhysics_Tensor_Measure_Projective(&pu, pid,
    /*theta_a*/ 0.0, /*theta_b*/ PRO_PI * 0.25,
    rng, &a, &b);
/* a, b ∈ {+1, −1} */
```

### §6.8 — Tensor ↔ Fock

```c
/* Tensor → Fock */
uint64_t fid = 0;
ProPhysics_Tensor_To_Fock(&pu, pid, &fid);

/* Fock → Tensor */
ProPhysics_Fock_To_Tensor(&pu, fid, pid);
```

---

## §7 — Fallstricke

### §7.1 — Qubit-Basis ist nur {1, 4}

Alle Gates wirken ausschließlich auf `PRO_QUBIT_BASIS[0]=1` und
`PRO_QUBIT_BASIS[1]=4`. Die übrigen 6 Basiszustände bleiben
unverändert. Wer andere Basen manipulieren will, muss
`Apply_Local_Op` mit einer 8×8-Matrix verwenden.

### §7.2 — `pro_jacobi_sym_eigen` modifiziert `A` in-place

`A` enthält nach dem Aufruf die Eigenwerte auf der Diagonale. Wer
`A` danach noch braucht, muss vorher kopieren.

### §7.3 — `V` ist optional, aber spaltenweise

Wenn `V != NULL`, enthält Spalte `k` den Eigenvektor zu `A[k][k]`.
**Nicht zeilenweise** — häufiger Fehler.

### §7.4 — `Sync_To_Amp` iteriert **alle** Paare

Es gibt keine Selektion nach Mark. `Sync_Marked_To_Amp` ist
derzeit ein Synonym für `Sync_To_Amp` (Marken werden nicht
ausgewertet).

### §7.5 — `Sync_From_Amp` erzeugt Produktzustand

Der resultierende Tensor ist **immer** separierbar
(`concurrence == 0`), da `coeff[a·8+b] = amp_a · amp_b`.

### §7.6 — `Tensor_Measure_Projective` prüft `p_total < 0.5`

Bei einem Zustand, dessen Überlappung mit dem Projektor-Unterraum
`< 0.5` ist, liefert die Funktion `false` und **keine** Ausgabe.

### §7.7 — `Apply_Hopping` erhält die Antisymmetrie nur bei antisymmetrischem Input

Der Hopping-Operator wirkt auf beide Paare `(i,b)` und `(b,i)` und
schreibt das Vorzeichen mit. Bei nicht-antismmetrischem Input
ist das Ergebnis undefiniert.

### §7.8 — `Set_Slater` mit `orbital_a == orbital_b`

Wird abgewiesen (`false`). Pauli-Prinzip: Diagonale bleibt 0.

### §7.9 — `Tensor_To_Fock` prüft Antisymmetrie mit Toleranz 1000

Diese Toleranz ist **absolut** in Q31-Skala. Bei Zuständen mit
sehr kleiner Norm (< 1) greift der Sonderfall (`norm_sq < 1.0
⇒ true`).

### §7.10 — `Fock_To_Tensor` prüft Popcounts = 2

Ein Fock-State mit einem Zustand `|i,j,k>` (Popcount 3) oder
`|i>` (Popcount 1) wird abgewiesen. Nur der 2-Teilchen-Sektor
ist mit Tensor kompatibel.

### §7.11 — SU(2)-Rotation nutzt 2×2-Format, nicht Quaternion

`Apply_SU2_Rotation` baut die 2×2-Matrix inline; nicht zu
verwechseln mit der Quaternion-Repräsentation in `SU2.c`. Die
beiden sind äquivalent, aber die Formate sind unterschiedlich.

### §7.12 — `pro_jacobi_sym_eigen` konvergiert bis `off < 1e-24`

Das kann bei sehr kleinen Matrizen (< 4×4) zu viele Sweeps
benötigen. `MAX_SWEEPS = 100` ist die harte Grenze.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Dichte-Modul | `docs/project/Density.md` |
| Fock-Modul | `docs/project/Fock.md` |
| SU(2)-Modul | `docs/project/SU2.md` |
| Gauge-Modul | `docs/project/Gauge.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/kernel/ProPhysics_Tensor.c` |
| Interne API | `ProPhysics_Internal.h` |

---

**Ende Tensor.md v1.0.**