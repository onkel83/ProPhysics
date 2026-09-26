# ProPhysics — API-Referenz

**Datei:** `docs/project/ProPhysics_API.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-26 (Kernel 1.23.0, Konsolidierungs-Serie
1.23.1–1.23.8 abgeschlossen)
**Zweck:** Vollständige Referenz aller öffentlichen Typen, Konstanten und
Funktionen. Dieses Dokument ist die verbindliche Schnittstellenbeschreibung
des Kernels.

---

## §0 — Wie man dieses Dokument liest

Die API-Referenz ist nach **Modulen** gegliedert. Jede Funktion hat:

| Feld | Bedeutung |
|---|---|
| **Signatur** | Der C-Prototyp, wie er in `ProPhysics.h` steht |
| **Zweck** | Was die Funktion tut |
| **Parameter** | Bedeutung jedes Arguments |
| **Rückgabe** | Bedeutung des Rückgabewerts |
| **Nebenwirkungen** | Was am `ProUniverse` verändert wird |
| **Thread-Safety** | ob und unter welchen Bedingungen parallelisierbar |
| **Hotpath** | ob die Funktion R1/R2 unterliegt |
| **Beispiel** | ein kurzes Code-Snippet |

**Konvention:** Alle Symbole mit Prefix `ProPhysics_` sind öffentlich. Alle
Symbole mit Prefix `pro_` sind **interne Helfer** (in `ProPhysics_Internal.h`)
und **nicht** Teil dieser API. Wer sie verwendet, benutzt undokumentiertes
Verhalten und muss mit Änderungen rechnen.

**Zitierweise:** `ProPhysics_Initialize` verweist auf die gleichnamige
Funktion in diesem Dokument.

**Versions-Kontext:** Diese Referenz beschreibt den Kernel in
**Phase 1, Etappe 23** (`Kernel: 1.23.0`). Die API-Stabilität ist durch
**R5** („keine stillen API-Brüche") garantiert — innerhalb einer Phase
(`1.x`) ist die API stabil; neue Funktionen kommen **additiv** hinzu.

---

## §1 — Übersicht und Aufbau

### §1.1 — Modul-Übersicht

Der Kernel besteht aus **12 `.c`-Modulen**. Die API ist entsprechend
gegliedert. Modul-Referenzen (Funktionsdetails, Konventionen,
Fallstricke) stehen in `docs/project/<Name>.md` — siehe §30.

| Modul | Datei | Aufgabe |
|---|---|---|
| Lifecycle | `ProPhysics_Core.c` | Initialisierung, Topologie, Tick-Orchestrierung |
| Amp | `ProPhysics_Amp.c` | Unitäre Dynamik auf `amp_grid` |
| Gauge | `ProPhysics_Gauge.c` | U(1)-Eichstruktur, Born-Wahrscheinlichkeit |
| EPR | `ProPhysics_EPR.c` | Paarmessungen, Kollaps, Apparat |
| Observer | `ProPhysics_Observer.c` | Test-spezifische Observer-Dynamik |
| Shared | `ProPhysics_Shared.c` | U4-Shared-Reference, Spin-Flip |
| Dirac | `ProPhysics_Dirac.c` | 4-Komponenten-Dirac-Spinor |
| SU2 | `ProPhysics_SU2.c` | SU(2)-Link-Kinematik |
| SU2_Dynamics | `ProPhysics_SU2_Dynamics.c` | Yang-Mills-Leapfrog-Dynamik |
| Tensor | `ProPhysics_Tensor.c` | 2-Knoten-Verschränkung |
| Fock | `ProPhysics_Fock.c` | 8-Moden-Fock-Raum |
| Density | `ProPhysics_Density.c` | Dichte-Matrizen + Lindblad |

### §1.2 — Namensschema

| Prefix | Bedeutung | Beispiel |
|---|---|---|
| `ProPhysics_<Modul>_<Verb>` | Öffentliche API | `ProPhysics_Tensor_Create_Pair` |
| `ProPhysics_<Verb>` | Modul-unabhängige Aktion | `ProPhysics_Initialize` |
| `PRO_<NAME>` | Konstante (Compile-Time) | `PRO_AMP_BASIS_SIZE` |
| `Pro<Name>` | Typ | `ProUniverse`, `ProEdge` |
| `pro_<name>` | interner Helfer | `pro_amp_abs2` |

Alle Symbole folgen einem **Modul-Prefix-Schema**. Die API-Stabilität
ist durch **R5** („keine stillen API-Brüche") garantiert: innerhalb
einer Phase (`1.x`) ist die API stabil; neue Funktionen kommen
additiv hinzu.

### §1.3 — Numerische Konventionen

| Typ | Skala | Anwendung | Wertebereich |
|---|---|---|---|
| **Q31** | `2³¹` | Amplituden (Re/Im je `int32`) | `[-2³¹, 2³¹)` |
| **Q30** | `2³⁰` | SU(2)-Links, elektrisches Feld | `[-2³⁰, 2³⁰)` |
| **Q15** | `2¹⁵` | Winkel, Kopplungen, Raten | `[-2¹⁵, 2¹⁵)` |
| **Q16** | `2¹⁶` | Phasen auf Kanten | `[0, 65535]` |
| **Q62** | `2⁶²` | Normierungs-Nenner (Fock, Dichte) | interne Konvention |

**Q31-Kodierung:** `ProAmpQ31` ist ein `int64`, das ein komplexes Paar
`(re, im)` in zwei `int32` packt:

```c
ProAmpQ31 a = pro_amp_pack(re, im);
int32_t re = pro_amp_real(a);
int32_t im = pro_amp_imag(a);
```

Für den Nutzer der API ist das **transparent**: Funktionen nehmen
`int32_t re_q31, int32_t im_q31` als getrennte Argumente und geben sie
getrennt zurück.

### §1.4 — Rückgabe-Konventionen

| Typ | Bedeutung |
|---|---|
| `bool` | `true` = Erfolg, `false` = Fehler |
| `void` | kein Rückgabewert; Fehler werden still ignoriert |
| `int` | `0` = Fehler, `1` = Erfolg (bei EPR-Messungen) |
| `double` | Größe; `1e300` = Fehler-Marker (bei Vergleichs-APIs) |
| `ProU128` | 128-Bit-Zähler; `PRO_U128_ZERO` = Fehler-Marker |

**Fehlerbehandlung:** Alle Funktionen prüfen ihre Argumente defensiv.
`NULL`-Zeiger, Out-of-Range-Indices und ungültige Zustände führen zu
einem **stillen** Rückgabewert (`false`, `0` oder `void`-Return), nicht zu
einem Crash. Das ist Absicht — der Kernel ist als embedded-fähig ausgelegt.

---

## §2 — Konstanten

Alle Konstanten sind in `ProPhysics_Config.h` definiert. Sie sind
**Compile-Time** und können per `-D<NAME>=<WERT>` beim Build überschrieben
werden (sofern mit `#ifndef` geschützt).

### §2.1 — Topologie

| Konstante | Default | Bedeutung |
|---|---|---|
| `NODE_COUNT` | `1048576ULL` | Standard-Gittergröße (2²⁰ Knoten) |
| `PRO_NODE_COUNT` | `NODE_COUNT` | Alias für `NODE_COUNT` |
| `MAX_NODES` | `NODE_COUNT` | Alias |
| `MAX_SPARSE_TRACKING_NODES` | `64000000ULL` | Obergrenze für Sparse-Tracking |

### §2.2 — Kanäle und Nachbarschaft

| Konstante | Wert | Bedeutung |
|---|---|---|
| `CHANNELS_MAX` | `16` | Kanäle pro Knoten |
| `PRO_EPR_CHANNEL` | `15` | EPR-Kanal |
| `PRO_DEPHASE_CHANNEL` | `5` | Dephase-Kanal |
| `PRO_PHYSICAL_CHANNELS` | `15` | Anzahl physikalischer Kanäle |
| `PRO_CH_MASK` | `15u` | Bit-Maske für Kanal-Index |
| `PRO_NEIGHBOR_X_PLUS` | `0u` | +x-Richtung |
| `PRO_NEIGHBOR_X_MINUS` | `1u` | −x-Richtung |
| `PRO_NEIGHBOR_Y_PLUS` | `2u` | +y-Richtung |
| `PRO_NEIGHBOR_Y_MINUS` | `3u` | −y-Richtung |
| `PRO_NEIGHBOR_Z_PLUS` | `4u` | +z-Richtung |
| `PRO_NEIGHBOR_Z_MINUS` | `6u` | −z-Richtung |

**Kanal-Layout:**

| Index | Verwendung |
|:-:|---|
| 0..4, 6 | Nachbarrichtungen (+x, −x, +y, −y, +z, −z) |
| 5 | Dephase |
| 7..14 | Reserve |
| 15 | EPR |

### §2.3 — Amplituden-Basis

| Konstante | Wert | Bedeutung |
|---|---|---|
| `PRO_AMP_BASIS_SIZE` | `8u` | Dimension der Basis pro Knoten |

**Basis-Indices** (definiert in `ProUrState`):

| Index | Name | Physikalische Rolle |
|:-:|---|---|
| 0 | `UR_NEUTRAL` | Leer / Vakuum |
| 1 | `UR_POSITRON_CW` | Spin-up / rechtshändig CW |
| 2 | `UR_POSITRON_CCW` | Spin-down / rechtshändig CCW |
| 3 | `UR_NEGATRON_CW` | linkshändig CW |
| 4 | `UR_NEGATRON_CCW` | linkshändig CCW |
| 5 | `UR_PHOTON` | Photon |
| 6, 7 | (reserviert) | — |

### §2.4 — U5-Gewichte

Definiert in `ProPhysics_Internal.h` als `PRO_U5_W[8]`:

```c
static const uint8_t PRO_U5_W[PRO_AMP_BASIS_SIZE] = {
    0u, 1u, 1u, 4u, 4u, 5u, 0u, 0u
};
```

Die U5-Invariante ist `Σ_k Σ_b PRO_U5_W[b] · |c_b(k)|²`.

### §2.5 — Tensor / Fock / Dichte

| Konstante | Wert | Bedeutung |
|---|---|---|
| `PRO_TENSOR_DIM` | `64u` | 8×8 Tensor-Zustand |
| `PRO_TENSOR_RHO_DIM` | `8u` | 8×8 Dichte einer Seite |
| `PRO_TENSOR_MAX_PAIRS` | `256u` | Max. Tensor-Paare |
| `PRO_FOCK_MODES` | `8u` | Anzahl Fermion-Moden |
| `PRO_FOCK_DIM` | `256u` | Fock-Basis (2⁸) |
| `PRO_FOCK_MAX_STATES` | `64u` | Max. Fock-Zustände |
| `PRO_DENSITY_DIM` | `8u` | 8×8 Knoten-Dichte |
| `PRO_DENSITY_MAX` | `64u` | Max. Knoten-Dichten |
| `PRO_TENSOR_DENSITY_DIM` | `64u` | 64×64 Tensor-Dichte |
| `PRO_TENSOR_DENSITY_MAX` | `32u` | Max. Tensor-Dichten |
| `PRO_FOCK_DENSITY_DIM` | `256u` | 256×256 Fock-Dichte |
| `PRO_FOCK_DENSITY_MAX` | `8u` | Max. Fock-Dichten |

### §2.6 — Dirac

| Konstante | Wert | Bedeutung |
|---|---|---|
| `PRO_DIRAC_DIM` | `4u` | 4 Spinor-Komponenten |
| `PRO_DIRAC_COMP_L_UP` | `0u` | ψ_L↑ |
| `PRO_DIRAC_COMP_L_DN` | `1u` | ψ_L↓ |
| `PRO_DIRAC_COMP_R_UP` | `2u` | ψ_R↑ |
| `PRO_DIRAC_COMP_R_DN` | `3u` | ψ_R↓ |
| `PRO_DIRAC_TO_BASIS(ci)` | `ci+1` | Dirac-Index → Basis-Index |
| `PRO_DIRAC_FROM_BASIS(b)` | `b-1` | Basis-Index → Dirac-Index |

### §2.7 — SU(2)

| Konstante | Wert | Bedeutung |
|---|---|---|
| `PRO_SU2_SCALE` | `1073741824` | 2³⁰ |
| `PRO_SU2_NORM` | `1152921504606846976LL` | 2⁶⁰ |
| `PRO_SU2_IDENT_RE` | `1073741824` | 1.0 in Q30 |
| `PRO_SU2_IDENT_IM` | `0` | 0.0 |
| `PRO_SU2_YM_DEFAULT_Q15` | `1000` | g² in Q15 |
| `PRO_SU2_LEAPFROG_DT_Q15` | `1000u` | dt in Q15 |

### §2.8 — Node-Bits

| Konstante | Wert | Bedeutung |
|---|---|---|
| `PRO_NODE_SPIN_FLIP_BIT` | `0x01u` | Bit 0: Spin-Flip (Etappe 19) |
| `PRO_NODE_DIRAC_BIT` | `0x02u` | Bit 1: Dirac-Knoten (Etappe 21) |

### §2.9 — Physik-Konstanten

| Konstante | Wert |
|---|---|
| `PRO_2PI` | `6.283185307179586` |
| `PRO_INV_2PI` | `0.159154943091895` |
| `PRO_8_OVER_2PI` | `1.273239544735163` |
| `PRO_INV_127` | `0.007874015748031` |
| `PRO_Q31_HALF_SQRT2` | `1518500249` |
| `PRO_DEFAULT_PHASE_STEP_Q15` | `1000u` |
| `PRO_DEFAULT_TRANSPORT_THETA_Q15` | `500u` |

### §2.10 — Cache und Debug

| Konstante | Default | Bedeutung |
|---|---|---|
| `PRO_CACHE_LINE` | `64u` | Cache-Line-Größe (Bytes) |
| `PRO_DEBUG_RING_SIZE` | `256u` | Ringpuffergröße für EPR-Debug |

Beide **müssen Zweierpotenzen** sein — `ProPhysics_Config.h` erzwingt das
mit `#error`.

---

## §3 — Typen

### §3.1 — `ProU128`

```c
typedef struct { uint64_t lo; uint64_t hi; } ProU128;
#define PRO_U128_ZERO ((ProU128){ 0u, 0u })
```

128-Bit-Ganzzahl (unsigned) in zwei 64-Bit-Hälften. Wird für
**overflow-sichere** Berechnungen verwendet (U5-Invariante, Fock-Norm,
Environment-Trace).

**Helfer (static inline in `ProPhysics_Types.h`):**

| Funktion | Wirkung |
|---|---|
| `pro_u128_from_u64(x)` | `uint64` → `ProU128` |
| `pro_u128_add(a, b)` | Addition mit Carry |
| `pro_u128_cmp(a, b)` | Vergleich: −1, 0, +1 |
| `pro_u128_is_zero(a)` | Prüft auf 0 |
| `pro_u128_shl(a, n)` | Links-Shift um `n` Bits |
| `pro_u128_mul_u64(a, b)` | Multiplikation zweier `uint64` |
| `pro_u128_mul_small(a, b)` | `ProU128` × `uint64` |
| `pro_u128_to_double(a)` | Konvertierung nach `double` |

### §3.2 — `ProAmpQ31` und `ProAmpVector`

```c
typedef int64_t ProAmpQ31;
typedef struct { ProAmpQ31 coeff[PRO_AMP_BASIS_SIZE]; } ProAmpVector;
```

`ProAmpQ31` ist ein gepacktes komplexes Paar in Q31. Öffentliche API
verwendet es **nur** als Element von `ProAmpVector` und in
`ProPhysics_Set_Node_Amplitude` / `ProPhysics_Get_Node_Amplitude`,
wo Real- und Imaginärteil getrennt übergeben werden.

**Helfer (static inline in `ProPhysics_Types.h`):**

| Funktion | Wirkung |
|---|---|
| `pro_amp_pack(re, im)` | Packt zwei `int32` zu `ProAmpQ31` |
| `pro_amp_real(a)` | Liest Realteil |
| `pro_amp_imag(a)` | Liest Imaginärteil |
| `pro_amp_mul_q31(a, b)` | Komplexe Multiplikation in Q31 |
| `pro_amp_add_q31(a, b)` | Komplexe Addition, mit Saturation |
| `pro_amp_conj_q31(a)` | Komplexe Konjugation |
| `pro_amp_scale_q31(a, s)` | Skalierung um Q31-Faktor |

`ProAmpVector` ist der **fundamentale Zustandsvektor** pro Gitterknoten.
`amp_grid[k].coeff[b]` ist die Amplitude von Knoten `k` in Basis `b`.

### §3.3 — `ProUrState`

```c
typedef enum {
    UR_NEUTRAL       = 0x00,
    UR_POSITRON_CW   = 0x01,
    UR_POSITRON_CCW  = 0x02,
    UR_NEGATRON_CW   = 0x03,
    UR_NEGATRON_CCW  = 0x04,
    UR_PHOTON        = 0x05
} ProUrState;
```

Der **type_state** eines Knotens. Wird in `ProNode.type_state`
gespeichert und vom U6-Guiding aus `amp_grid` rekonstruiert.

**Wichtig:** `type_state` ist eine **Anzeige**, nicht das Fundament.
Der fundamentale Zustand liegt in `amp_grid`. Wer `type_state` direkt
setzt, umgeht das Guiding — das ist nur für Tests gedacht.

### §3.4 — `ProNode`

```c
typedef struct {
    uint8_t  type_state;
    uint8_t  field_helicity;
    uint8_t  momentum_phase;
    uint8_t  reserved_gating;
    uint16_t phase_accumulator;
    uint16_t _reserved_pad;
} ProNode;
```

| Feld | Bereich | Bedeutung |
|---|---|---|
| `type_state` | 0..5 | U6-Zustand (Anzeige) |
| `field_helicity` | 0..2 | Helizität (0 = neutral, 1/2 = ±) |
| `momentum_phase` | 0..255 | Impuls in Q7 (0..127 ≈ v/c) |
| `reserved_gating` | Bitfeld | Bit 0: Spin-Flip, Bit 1: Dirac |
| `phase_accumulator` | 0..65535 | Innere Uhr, Q16-Phase |

### §3.5 — `ProRegister`

```c
typedef struct { uint64_t channels[CHANNELS_MAX]; } ProRegister;
```

Kanal-Register eines Knotens. `channels[c]` ist der Zielknoten-Index für
Kanal `c`. Ist `channels[c] == k`, ist der Kanal eine Self-Loop.

### §3.6 — `ProEdge`

```c
typedef struct {
    /* Offset 0..7: U(1)/EPR */
    uint16_t phase;
    uint8_t  type;
    uint8_t  _reserved;
    uint8_t  pending_helicity;
    uint8_t  last_sent_helicity;
    uint16_t pending_ticks;
    /* Offset 8..23: SU(2)-Link (Q30) */
    int32_t  su2_a_re;
    int32_t  su2_a_im;
    int32_t  su2_b_re;
    int32_t  su2_b_im;
    /* Offset 24..39: E-Feld (Q30) */
    int32_t  su2_E_a_re;
    int32_t  su2_E_a_im;
    int32_t  su2_E_b_re;
    int32_t  su2_E_b_im;
} ProEdge;
```

`sizeof(ProEdge) == 40` Bytes. Layout:

| Offset | Feld | Zweck |
|---:|---|---|
| 0 | `phase` | U(1)-Kantenphase (Q16) |
| 2 | `type` | `ProEdgeType` |
| 4 | `pending_helicity` | EPR: Signal unterwegs |
| 5 | `last_sent_helicity` | EPR: zuletzt gesendet |
| 6 | `pending_ticks` | EPR: Delay-Counter |
| 8..23 | `su2_a_*, su2_b_*` | SU(2)-Link (Q30) |
| 24..39 | `su2_E_*` | Chromoelektrisches Feld (Q30) |

**Bei `su2_active == 0`** werden die SU(2)-Felder nicht gelesen (R7).
**Bei `su2_dynamics_active == 0`** werden die E-Felder nicht verändert,
aber gespeichert.

### §3.7 — `ProEdgeType`

```c
typedef enum {
    PRO_EDGE_NONE    = 0,
    PRO_EDGE_SINGLET = 1,
    PRO_EDGE_TRIPLET = 2,
    PRO_EDGE_PRODUCT = 3
} ProEdgeType;
```

Korrelationstyp einer EPR-Kante. Bestimmt, welche Relation beim Kollaps
angewendet wird.

### §3.8 — `ProObserver`

```c
typedef struct {
    uint64_t base_node;
    uint32_t size;
    uint32_t _pad;
} ProObserver;
```

Ein Beobachter ist ein **zusammenhängender Knotenbereich**
`[base_node, base_node + size)`.

### §3.9 — `ProSharedInfo`

```c
typedef struct {
    uint64_t* parent;
    uint8_t   active;
    uint8_t   _pad[7];
} ProSharedInfo;
```

Union-Find-Struktur für U4-Shared-Reference. `parent[k]` ist der
Repräsentant der Klasse von Knoten `k`. Bei `active == 0` ist jeder
Knoten seine eigene Klasse.

### §3.10 — `ProAmpTensorPair`

```c
typedef struct {
    uint64_t    node_a;
    uint64_t    node_b;
    uint32_t    pair_id;
    uint32_t    active;
    int32_t     coupling_q15;
    uint32_t    _coupling_pad;
    int32_t     hopping_theta_q15;
    uint8_t     hopping_i;
    uint8_t     hopping_j;
    uint16_t    _hopping_pad;
    ProAmpQ31   coeff[PRO_TENSOR_DIM];
} ProAmpTensorPair;
```

Ein Tensor-Paar ist ein **isolierter 2-Knoten-Zustand** in `coeff[64]`
(8×8 Kronecker-Produkt). `coupling_q15` steuert die XX-Kopplung,
`hopping_*` das fermionische Hopping.

### §3.11 — `ProFockState`

```c
typedef struct {
    uint64_t    fock_id;
    uint32_t    active;
    uint32_t    n_particles;
    ProAmpQ31   coeff[PRO_FOCK_DIM];
} ProFockState;
```

`coeff[i]` ist die Amplitude des Besetzungszustands `i` (8-Bit-Muster).
Normierung: `Σ|c|² = 2⁶²` entspricht physikalisch 1.

### §3.12 — `ProDensityMatrix`

```c
typedef struct {
    uint32_t    active;
    uint32_t    dim;
    uint64_t    _pad;
    ProAmpQ31   rho[PRO_DENSITY_DIM * PRO_DENSITY_DIM];
} ProDensityMatrix;
```

8×8-Dichtematrix eines Knotens in Q31. Spur: `Tr(ρ) = 1.0` entspricht
Summe `2³¹`.

### §3.13 — `ProTensorDensity` / `ProFockDensity`

```c
typedef struct {
    uint32_t   active;
    uint32_t   pair_id;
    uint64_t   _pad;
    ProAmpQ31  rho[PRO_TENSOR_DENSITY_DIM * PRO_TENSOR_DENSITY_DIM];
} ProTensorDensity;

typedef struct {
    uint32_t   active;
    uint32_t   _pad;
    uint64_t   fock_id;
    ProAmpQ31  rho[PRO_FOCK_DENSITY_DIM * PRO_FOCK_DENSITY_DIM];
} ProFockDensity;
```

64×64- bzw. 256×256-Dichtematrizen für Tensor-Paare und Fock-Zustände.

### §3.14 — `ProLindbladKind`

```c
typedef enum {
    PRO_LINDBLAD_NONE       = 0,
    PRO_LINDBLAD_AMP_DAMP   = 1,
    PRO_LINDBLAD_PHASE_DAMP = 2,
    PRO_LINDBLAD_DEPOLARIZE = 3
} ProLindbladKind;
```

Lindblad-Kanaltyp. `NONE` = No-Op, die anderen drei implementieren
Standard-Kanäle.

### §3.15 — `ProGammaBasis`

```c
typedef enum {
    PRO_GAMMA_BASIS_DIRAC = 0,
    PRO_GAMMA_BASIS_WEYL = 1
} ProGammaBasis;
```

Dirac-Darstellung der γ-Matrizen.

### §3.16 — `ProSU2Basis`

```c
typedef enum {
    PRO_SU2_BASIS_PAULI    = 0,
    PRO_SU2_BASIS_GELLMANN = 1    /* reserviert */
} ProSU2Basis;
```

SU(2)-Generator-Basis. Aktuell nur `PAULI` implementiert.

### §3.17 — `ProUniverse`

```c
typedef struct {
    uint64_t total_nodes;
    uint32_t grid_dim;
    uint32_t grid_dim_shift;
    uint32_t grid_dim_mask;
    uint32_t grid_ndim;

    ProNode* ur_grid;
    ProRegister* reg_source;
    ProRegister* reg_target;
    uint64_t     current_cpu_tick;
    uint32_t     global_entropy_index;
    uint32_t     _tick_pad;

    ProEdge* edge_phases;
    ProAmpVector* amp_grid;
    ProAmpVector* amp_scratch;

    ProAmpTensorPair* tensor_pairs;
    uint64_t          tensor_pair_count;
    uint64_t          tensor_pair_capacity;

    uint16_t* u_field;
    uint32_t epr_delay_ticks;
    uint32_t epr_debug;
    uint64_t epr_signal_count;

    uint8_t* tensor_marks;
    uint8_t  auto_sync_tensor;
    uint8_t  auto_sync_density;
    uint8_t  _tensor_flags_pad[6];

    int64_t  last_theta_q15;
    int64_t  last_c_num;
    int64_t  last_s_num;
    int64_t  last_denom;
    uint8_t  transport_cache_valid;
    uint8_t  _transport_pad[7];

    ProFockState* fock_states;
    uint64_t      fock_state_count;
    uint64_t      fock_state_capacity;

    ProDensityMatrix* density_matrices;
    uint64_t          density_count;
    uint64_t          density_capacity;

    ProTensorDensity* tensor_densities;
    uint64_t          tensor_density_count;
    uint64_t          tensor_density_capacity;

    ProFockDensity* fock_densities;
    uint64_t        fock_density_count;
    uint64_t        fock_density_capacity;

    ProSharedInfo shared;
    uint64_t _shared_tick_count;

    uint8_t       dirac_active;
    uint8_t       dirac_gamma_basis;
    uint8_t       _dirac_pad[2];
    int32_t       dirac_mass_q15;

    uint8_t       su2_active;
    uint8_t       su2_gauge_basis;
    uint8_t       _su2_pad[2];
    int32_t       su2_coupling_q15;

    uint8_t       su2_dynamics_active;
    uint8_t       _su2b_pad[3];
    int32_t       su2_yang_mills_q15;

    uint32_t _core_magic;
    uint32_t _core_pad;
} ProUniverse;
```

`ProUniverse` ist das **zentrale Datenobjekt**. Es wird vom Nutzer
allokiert (meist auf dem Stack oder in einem Wrapper-Struct) und über
`ProPhysics_Initialize` / `ProPhysics_Free` verwaltet.

**Wichtige Felder für den Nutzer:**

| Feld | Typ | Zweck |
|---|---|---|
| `total_nodes` | `uint64_t` | Anzahl Knoten (read-only nach Init) |
| `grid_dim` | `uint32_t` | 2D/3D-Gitterkante (Zweierpotenz) |
| `grid_ndim` | `uint32_t` | 2 oder 3 |
| `current_cpu_tick` | `uint64_t` | Tick-Zähler |
| `dirac_active` | `uint8_t` | 0/1: Dirac-Pfad |
| `su2_active` | `uint8_t` | 0/1: SU(2)-Links aktiv |
| `su2_dynamics_active` | `uint8_t` | 0/1: Leapfrog-Dynamik aktiv |

**Interne Felder** (`_pad`, `_core_magic`, Cache-Felder) dürfen **nicht**
direkt manipuliert werden.

---

## §4 — Lifecycle

### §4.1 — `ProPhysics_Initialize`

```c
PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count);
```

**Zweck:** Allokiert und initialisiert ein Universum.

**Parameter:**
- `pu` — Zeiger auf ein vom Nutzer allokiertes `ProUniverse`. Wird
  `memset(0)` gesetzt und neu befüllt.
- `node_count` — Anzahl Knoten. Muss > 0 sein.

**Rückgabe:** keine.

**Nebenwirkungen:**
- Alle Hot-Arrays werden per `pro_aligned_calloc` (Cache-Line-aligned)
  allokiert.
- `reg_source`, `reg_target`: alle Kanäle zeigen auf den Knoten selbst.
- `edge_phases`: alle `phase = 0`, `type = PRO_EDGE_NONE`,
  `su2_*` = Identität, `su2_E_*` = 0.
- `shared.parent[k] = k`.
- `u_field[k]` = deterministischer splitmix64-Wert.
- `_core_magic` = `PRO_CORE_MAGIC`.

**Fehlerbehandlung:** Bei OOM oder Overflow wird `pu` auf 0 gesetzt und
die Funktion kehrt zurück. Ein nachfolgender API-Aufruf liefert `false`.

**Beispiel:**

```c
ProUniverse pu;
ProPhysics_Initialize(&pu, 64 * 64);
pu.grid_dim = 64;
pu.grid_ndim = 2;
```

**Wichtig:** `grid_dim`, `grid_ndim` werden **nicht** von `Initialize`
gesetzt. Der Nutzer muss sie explizit konfigurieren.

### §4.2 — `ProPhysics_Free`

```c
PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu);
```

**Zweck:** Gibt alle von `Initialize` allokierten Ressourcen frei.

**Parameter:** `pu` — das zu befreiende Universum.

**Nebenwirkungen:** `memset(pu, 0, sizeof(*pu))`. Nach dem Aufruf ist
`pu` in einem definierten Nullzustand. Ein erneuter `Initialize`-Aufruf
ist erlaubt.

**Beispiel:**

```c
ProPhysics_Free(&pu);
```

**Wichtig:** Doppeltes `Free` ist safe (durch `memset`).

### §4.3 — `ProPhysics_Init` (Macro)

```c
#define ProPhysics_Init(pu, count) ProPhysics_Initialize((pu), (count))
```

Alias für `ProPhysics_Initialize`. Für Rückwärtskompatibilität.

### §4.4 — `ProPhysics_Tick`

```c
PROPHYSICS_API void ProPhysics_Tick(ProUniverse* pu,
    ProPhysics_RuleCallback callback);
```

**Zweck:** Führt **einen vollständigen Tick** des Universums aus.

**Parameter:**
- `pu` — das Universum.
- `callback` — optionaler Plastizitäts-Callback (darf `NULL` sein).

**Ablauf (in dieser Reihenfolge):**

1. Lazy-Init von `reg_target`, falls `NULL`.
2. `current_cpu_tick++`.
3. `Advance_Internal_Clocks` — innere Uhr pro Knoten.
4. **EPR-Propagation** — verarbeitet `pending_ticks` auf EPR-Kanten.
5. `Apply_Amp_Step` — U1-U5-Dynamik (Context + Transport + Wave).
6. `Apply_Guiding_Equation` — U6-Anzeige (`type_state` aus `amp_grid`).
7. **Graph-Plastizität** — `callback` + Swap `reg_source ↔ reg_target`.
8. `global_entropy_index` = Anzahl aktiver Interaktionen.

**Callback-Signatur:**

```c
typedef void (*ProPhysics_RuleCallback)(
    uint8_t current_state,
    uint8_t target_state,
    uint64_t* current_channels,
    uint64_t* target_channels,
    uint8_t* out_next_state,
    uint8_t* out_next_target_state,
    uint64_t current_idx,
    uint64_t total_nodes);
```

Der Callback darf:
- `current_channels` und `target_channels` **in-place permutieren**.
- `out_next_state` und `out_next_target_state` setzen (aktuell ignoriert
  — die Zustände werden im nächsten Tick via Guiding rekonstruiert).

**`callback == NULL`:** Die Topologie bleibt unverändert. Der Rest läuft
**bit-identisch** zu einem No-Op-Callback (R7).

**Beispiel:**

```c
/* Ein Tick ohne Plastizität */
ProPhysics_Tick(&pu, NULL);

/* Ein Tick mit Plastizität */
ProPhysics_Tick(&pu, my_rule_callback);
```

**Wichtig:** `ProPhysics_Tick` ist die **einzige** Funktion, die einen
kompletten Tick ausführt. Einzelschritte (`Apply_Amp_Step`, etc.) sind
zwar öffentlich, sollten aber nur für Tests verwendet werden.

### §4.5 — `ProPhysics_Advance_Internal_Clocks`

```c
PROPHYSICS_API void ProPhysics_Advance_Internal_Clocks(ProUniverse* pu);
```

**Zweck:** Aktualisiert die innere Uhr pro Knoten. Wird **intern** von
`ProPhysics_Tick` aufgerufen.

**Physik:** Für jeden Knoten mit `type_state != UR_NEUTRAL, UR_PHOTON`:
- `v = momentum_phase / 127`
- `γ⁻¹ = √(1 − v²)`
- `delta = 256 · γ⁻¹` (Q-skalierter Phasenzuwachs)
- `phase_accumulator += delta`; bei Überlauf wird `field_helicity`
  getoggelt.

**Beispiel:** Wird normalerweise nicht direkt aufgerufen.

---

## §5 — Topologie

### §5.1 — `ProPhysics_Link_Nodes`

```c
PROPHYSICS_API void ProPhysics_Link_Nodes(ProUniverse* pu,
    uint64_t src, uint64_t target, uint8_t channel_idx);
```

**Zweck:** Setzt `reg_source[src].channels[channel_idx] = target`.

**Parameter:**
- `src`, `target` — Knoten-Indizes (`< total_nodes`).
- `channel_idx` — Kanal (`< CHANNELS_MAX`).

**Fehlerbehandlung:** Silent return bei Out-of-Range oder `NULL`.

**Beispiel:**

```c
/* Verbinde Knoten 5 mit Knoten 6 über +x-Kanal */
ProPhysics_Link_Nodes(&pu, 5, 6, PRO_NEIGHBOR_X_PLUS);
```

### §5.2 — `ProPhysics_Unlink_Node`

```c
PROPHYSICS_API void ProPhysics_Unlink_Node(ProUniverse* pu,
    uint64_t src, uint8_t channel_idx);
```

**Zweck:** Setzt den Kanal auf Self-Loop (`src` → `src`).

**Beispiel:**

```c
ProPhysics_Unlink_Node(&pu, 5, PRO_NEIGHBOR_X_PLUS);
```

---

## §6 — Injektion

### §6.1 — `ProPhysics_Inject_Momentum`

```c
PROPHYSICS_API void ProPhysics_Inject_Momentum(ProUniverse* pu,
    uint64_t node_idx, double velocity_ratio);
```

**Zweck:** Setzt `momentum_phase = round(velocity_ratio × 127)`.

**Parameter:**
- `velocity_ratio` — `v/c`, geclamped auf `[0, 1]`.

**Beispiel:**

```c
ProPhysics_Inject_Momentum(&pu, 42, 0.6);  /* v = 0.6c */
```

### §6.2 — `ProPhysics_Spawn_Body`

```c
PROPHYSICS_API void ProPhysics_Spawn_Body(ProUniverse* pu,
    uint64_t node_idx, uint8_t state, uint8_t helicity);
```

**Zweck:** Setzt `type_state` und `field_helicity`, nullt den
Amp-Vektor und setzt `coeff[state] = 1.0`.

**Parameter:**
- `state` — `ProUrState`-Wert (0..5).
- `helicity` — 0, 1 oder 2.

**Beispiel:**

```c
ProPhysics_Spawn_Body(&pu, 42, UR_POSITRON_CW, 1);
```

---

## §7 — Amp-Grid-Zugriff

### §7.1 — `ProPhysics_Get_Amp_Grid`

```c
PROPHYSICS_API ProAmpVector* ProPhysics_Get_Amp_Grid(ProUniverse* pu);
```

**Zweck:** Direkter Zeiger auf das `amp_grid`-Array. **Nur lesend**
verwenden, wenn man den Kernel nicht umgehen will.

**Rückgabe:** `pu->amp_grid` oder `NULL`.

### §7.2 — `ProPhysics_Set_Node_Amplitude`

```c
PROPHYSICS_API bool ProPhysics_Set_Node_Amplitude(ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx,
    int32_t re_q31, int32_t im_q31);
```

**Zweck:** Setzt `amp_grid[node_idx].coeff[basis_idx]` auf `(re, im)`.

**Rückgabe:** `false` bei Out-of-Range.

**Beispiel:**

```c
/* Setze Knoten 5, Basis 1 auf (1, 0) */
ProPhysics_Set_Node_Amplitude(&pu, 5, UR_POSITRON_CW, INT32_MAX, 0);
```

### §7.3 — `ProPhysics_Get_Node_Amplitude`

```c
PROPHYSICS_API bool ProPhysics_Get_Node_Amplitude(const ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx,
    int32_t* out_re_q31, int32_t* out_im_q31);
```

**Zweck:** Liest `amp_grid[node_idx].coeff[basis_idx]`.

**Rückgabe:** `false` bei Out-of-Range oder `NULL`.

### §7.4 — `ProPhysics_Sync_Amp_From_Type`

```c
PROPHYSICS_API void ProPhysics_Sync_Amp_From_Type(ProUniverse* pu);
```

**Zweck:** Setzt `amp_grid[k].coeff[type_state[k]] = 1.0` für alle Knoten,
nullt die anderen Komponenten. Wird von `Initialize` aufgerufen.

---

## §8 — Signed Permutations

### §8.1 — `ProPhysics_Apply_Signed_Permutation`

```c
PROPHYSICS_API void ProPhysics_Apply_Signed_Permutation(ProUniverse* pu,
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Wendet eine signierte Permutation auf `amp_grid` an.

**Mathematik:** `dst[i] = sign[i] · src[perm[i]]` für alle `i`.

**Beispiel:**

```c
uint8_t perm[8] = {0, 2, 1, 4, 3, 5, 6, 7};
int8_t  sign[8] = {1, 1, -1, 1, 1, 1, 1, 1};
ProPhysics_Apply_Signed_Permutation(&pu, perm, sign);
```

### §8.2 — `ProPhysics_Verify_Signed_Perm_Unitarity`

```c
PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_Unitarity(
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Prüft, ob `perm` eine Bijektion ist und `sign ∈ {−1, +1}`.

### §8.3 — `ProPhysics_Verify_Signed_Perm_U5`

```c
PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_U5(
    const uint8_t perm[PRO_AMP_BASIS_SIZE],
    const int8_t  sign[PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Prüft, ob die Permutation die U5-Gewichte erhält:
`PRO_U5_W[perm[i]] == PRO_U5_W[i]`.

### §8.4 — `ProPhysics_Compute_Context_Perm`

```c
PROPHYSICS_API void ProPhysics_Compute_Context_Perm(
    const ProUniverse* pu,
    uint64_t node_idx,
    uint8_t  perm_out[PRO_AMP_BASIS_SIZE],
    int8_t   sign_out[PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Berechnet die kontextabhängige Permutation für einen Knoten.

**Physik:** Hasht die Nachbar-Amplituden (splitmix64) und leitet daraus
deterministisch die Permutation und Vorzeichen ab. Kein PRNG.

### §8.5 — `ProPhysics_Apply_Context_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_Context_Tick(ProUniverse* pu);
```

**Zweck:** Wendet `Compute_Context_Perm` auf alle Knoten an.

**Hotpath:** R1, R2 (kein div/mod, kein malloc).

---

## §9 — Transport und Wave

### §9.1 — `ProPhysics_Apply_Edge_Transport`

```c
PROPHYSICS_API void ProPhysics_Apply_Edge_Transport(ProUniverse* pu,
    uint32_t theta_q15);
```

**Zweck:** 1D-Transport (oder 2D-Fallback ohne Gitterkoordinaten) über
alle Kanten.

**Physik:** Für jedes Knotenpaar `(k, nb)` mit `nb > k` wird eine
2-Knoten-Rotation mit Winkel `θ` angewendet.

**Beispiel:**

```c
ProPhysics_Apply_Edge_Transport(&pu, PRO_DEFAULT_TRANSPORT_THETA_Q15);
```

### §9.2 — `ProPhysics_Apply_Edge_Transport_Colored`

```c
PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim);
```

**Zweck:** 2D-Transport mit **Schachbrett-Muster** (4 Sweeps).

**Physik:** Verhindert Doppelanwendung durch Aufteilen der Kanten in
2 Paritäten pro Richtung. Rotiert nur Kanten mit `(x+y) & 1 == parity`.

**Voraussetzung:** `grid_dim` Zweierpotenz.

### §9.3 — `ProPhysics_Apply_Edge_Transport_Colored_3D`

```c
PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored_3D(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim);
```

**Zweck:** 3D-Transport mit 6 Sweeps (x, y, z je 2 Paritäten).

**Voraussetzung:** `grid_dim` Zweierpotenz.

### §9.4 — `ProPhysics_Apply_Wave_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Wave_Step(ProUniverse* pu,
    uint32_t phase_step_q15);
```

**Zweck:** Unitärer Wave-Step mit **lokaler Energie** als Phase.

**Physik:**

```
d_theta = s · (Re(c)·Re(L) + Im(c)·Im(L)) / (|c|²)
```

wobei `L = −n_nb·c + Σ_nachbarn`. `s` ist `phase_step_q15 / 32768`.

**Hotpath:** Verwendet `cos`/`sin` pro Basis-Koeffizient. Kein div/mod
im Index-Pfad.

### §9.5 — `ProPhysics_Apply_Amp_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Amp_Step(ProUniverse* pu,
    uint32_t phase_step_q15);
```

**Zweck:** Hauptfunktion des SDK-Ticks. Führt die komplette U1-U5-Dynamik
aus.

**Dispatch-Reihenfolge:**

1. `su2_dynamics_active` → `Apply_SU2_Tick`.
2. `shared.active` → `Shared_Tick_Reps` (return).
3. `dirac_active` → `Apply_Dirac_Step` (return).
4. Standard-Pfad: Context → Transport → Wave → Tensor → Sync.

**Wichtig:** Wird **intern** von `ProPhysics_Tick` aufgerufen. Direkter
Aufruf nur für Tests.

### §9.6 — `ProPhysics_Apply_Guiding_Equation`

```c
PROPHYSICS_API void ProPhysics_Apply_Guiding_Equation(ProUniverse* pu);
```

**Zweck:** U6-Führungsgleichung — rekonstruiert `type_state` aus den
Amplituden.

**Physik:**

```
p_pos = |c_1|² + |c_2|²
p_neg = |c_3|² + |c_4|²
type_state = (u_field[k] · (p_pos + p_neg) < 2¹⁶ · p_pos)
    ? UR_POSITRON_CW : UR_NEGATRON_CCW
```

Wird **intern** von `ProPhysics_Tick` aufgerufen.

---

## §10 — U(1)-Eichstruktur

### §10.1 — `ProPhysics_Wilson_Loop`

```c
PROPHYSICS_API void ProPhysics_Wilson_Loop(const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t* path_channels,
    uint32_t        path_len,
    uint16_t* out_phase_fx);
```

**Zweck:** Summe der Kantenphasen eines geschlossenen Pfads, modulo 2¹⁶.

**Parameter:**
- `path_nodes` — Array von Knoten-Indizes.
- `path_channels` — Array von Kanal-Indizes (gleiche Länge).
- `path_len` — Anzahl Kanten im Pfad.
- `out_phase_fx` — Ausgabe-Phase (Q16).

**Reihenfolge:** Vorwärts (abelsch, irrelevant).

**Beispiel:**

```c
uint64_t nodes[4] = {0, 1, 2, 3};
uint8_t  chans[4] = {0, 2, 1, 3};
uint16_t phase;
ProPhysics_Wilson_Loop(&pu, nodes, chans, 4, &phase);
```

### §10.2 — `ProPhysics_Global_Phase`

```c
PROPHYSICS_API void ProPhysics_Global_Phase(ProUniverse* pu,
    uint16_t phase_fx);
```

**Zweck:** Multipliziert jeden Amplitudenvektor mit `exp(i·phase)`.

### §10.3 — `ProPhysics_Make_Lambda_Field`

```c
PROPHYSICS_API void ProPhysics_Make_Lambda_Field(uint64_t seed,
    uint16_t* lambda_fx, uint64_t total_nodes);
```

**Zweck:** Erzeugt deterministisch ein λ-Feld (Q16-Phasen) aus `seed`.

**Beispiel:**

```c
uint16_t* lambda = malloc(pu.total_nodes * sizeof(uint16_t));
ProPhysics_Make_Lambda_Field(0xDEADBEEF, lambda, pu.total_nodes);
```

### §10.4 — `ProPhysics_Apply_Local_Gauge`

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Gauge(ProUniverse* pu,
    const uint16_t* lambda_fx);
```

**Zweck:** Lokale U(1)-Eichtransformation.

**Physik:**
- Kantenphase: `φ(x→y) ← φ(x→y) + λ(y) − λ(x)` (exakt).
- Amplitude: `ψ(x) ← exp(i·λ(x)) · ψ(x)` (Q30-Rotation).

**Wilson-Loop ist invariant.**

### §10.5 — `ProPhysics_Apply_Local_Phase_Plate`

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Phase_Plate(
    ProUniverse* pu,
    uint32_t x0, uint32_t y0,
    uint32_t w, uint32_t h,
    uint16_t phase_q15);
```

**Zweck:** Ortsabhängige Phase auf Rechteck `[x0, x0+w) × [y0, y0+h)`.

**Nur 2D.** Bei `grid_ndim == 3` return.

### §10.6 — `ProPhysics_Apply_Coulomb_Phase_Field_3D`

```c
PROPHYSICS_API void ProPhysics_Apply_Coulomb_Phase_Field_3D(
    ProUniverse* pu,
    uint32_t cx, uint32_t cy, uint32_t cz,
    int32_t  strength_q15,
    uint32_t softening_q8);
```

**Zweck:** Radialsymmetrisches 1/r-Phase-Feld (Coulomb-Potential) auf
3D-Torus.

**Physik:** `phase(r) = strength / (r + softening)`. Vorab-Cache für
`r < 1` (Q8-Tabelle, 256 Einträge).

**Nur 3D.**

---

## §11 — Born-Wahrscheinlichkeit

### §11.1 — `ProPhysics_Get_Born_Probability`

```c
PROPHYSICS_API double ProPhysics_Get_Born_Probability(const ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx);
```

**Zweck:** `P(b | k) = |c_b|² / Σ_j |c_j|²`.

**Rückgabe:** Wahrscheinlichkeit in `[0, 1]`, oder `0.0` bei Out-of-Range.

**Beispiel:**

```c
double p = ProPhysics_Get_Born_Probability(&pu, 42, UR_POSITRON_CW);
```

---

## §12 — Messungen und λ

### §12.1 — `ProPhysics_Compute_Lambda`

```c
PROPHYSICS_API double ProPhysics_Compute_Lambda(const ProUniverse* pu,
    uint64_t node_idx);
```

**Zweck:** Projektionsachse λ eines Knotens.

**Physik:** `λ = 2·atan2(c_4, c_1)` (Basis {1, 4}).

### §12.2 — `ProPhysics_Sharp_Measure`

```c
PROPHYSICS_API int ProPhysics_Sharp_Measure(double theta, double lambda);
```

**Zweck:** `sign(cos(θ − λ))`. Deterministisch.

**Rückgabe:** `+1` oder `−1`.

### §12.3 — `ProPhysics_Type_State_Measure`

```c
PROPHYSICS_API int ProPhysics_Type_State_Measure(double theta, uint8_t type_state);
```

**Zweck:** Deterministische Messung basierend auf `type_state`.

**Physik:** `λ_state = 0` für `UR_POSITRON_CW`, `π` für `UR_NEGATRON_CCW`.

---

## §13 — EPR

### §13.1 — `ProPhysics_Measure_EPR_Pair`

```c
PROPHYSICS_API int ProPhysics_Measure_EPR_Pair(ProUniverse* pu,
    uint64_t node_a, double theta_a, double theta_b,
    uint64_t rng[4], int* out_a, int* out_b);
```

**Zweck:** Deterministische Paarmessung mit `type_state`-Guard.

**Rückgabe:** `1` bei Erfolg, `0` bei Fehler.

**Wichtig:** `rng` wird **nicht** verwendet (deterministisch). Bleibt
aus Kompatibilität in der Signatur.

### §13.2 — `ProPhysics_Measure_EPR_Pair_Amp`

```c
PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Amp(
    ProUniverse* pu,
    uint64_t node_a,
    double   theta_a,
    double   theta_b,
    int* out_a, int* out_b);
```

**Zweck:** Wie `Measure_EPR_Pair`, aber `amp_grid`-Guard statt
`type_state`.

### §13.3 — `ProPhysics_Measure_EPR_Pair_Collapse`

```c
PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Collapse(
    ProUniverse* pu,
    uint64_t node_a, double theta_a, double theta_b,
    uint64_t rng[4], int* out_a, int* out_b);
```

**Zweck:** Kollaps-Messung via U4-Shared-Reference.

**Physik:**
1. A wird bei θ_a via Born-Regel gemessen.
2. A kollabiert auf `λ_a ∈ {θ_a, θ_a + π}`.
3. B kollabiert mit: `λ_b = λ_a + π` (Singlet) oder `λ_b = λ_a` (Triplet).
4. B wird bei θ_b via Born-Regel gemessen.

**Ergebnis:** `S = 2√2` (Tsirelson).

### §13.4 — `ProPhysics_Measure_EPR_Pair_Graph`

```c
PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Graph(
    ProUniverse* pu,
    uint64_t node_a, double theta_a, double theta_b,
    uint64_t rng[4], int* out_a, int* out_b);
```

**Zweck:** Wie `_Collapse`, aber `λ_b`-Relation kommt aus
`edge->phase` (U(1)-Connection), nicht aus `edge->type`.

**Physik:** `λ_b = λ_a + 2π · phase / 65536`.

### §13.5 — `ProPhysics_Set_EPR_Delay`

```c
PROPHYSICS_API void ProPhysics_Set_EPR_Delay(ProUniverse* pu,
    uint32_t delay_ticks);
```

**Zweck:** Setzt die Signal-Verzögerung in Ticks.

### §13.6 — `ProPhysics_Get_EPR_Delay`

```c
PROPHYSICS_API uint32_t ProPhysics_Get_EPR_Delay(const ProUniverse* pu);
```

**Zweck:** Liest die aktuelle Verzögerung.

### §13.7 — `ProPhysics_Set_EPR_Debug`

```c
PROPHYSICS_API void ProPhysics_Set_EPR_Debug(ProUniverse* pu, uint32_t level);
```

**Zweck:** Aktiviert Debug-Events (Level 0 = aus, ≥ 1 = an). Events
landen im internen Ringpuffer, nicht auf stdout.

---

## §14 — Apparat (Superdeterminismus-Test)

### §14.1 — `ProPhysics_Init_Apparatus`

```c
PROPHYSICS_API void ProPhysics_Init_Apparatus(ProUniverse* pu,
    uint64_t base_node, uint32_t dim);
```

**Zweck:** Initialisiert einen 2D-Apparat-Torus (`dim × dim`) ab
`base_node`.

### §14.2 — `ProPhysics_Set_Apparatus_Phase`

```c
PROPHYSICS_API void ProPhysics_Set_Apparatus_Phase(ProUniverse* pu,
    uint64_t base_node, uint32_t dim, uint16_t phase_fx);
```

**Zweck:** Setzt die Phase aller Apparat-Knoten auf `phase_fx`.

### §14.3 — `ProPhysics_Read_Apparatus_Theta`

```c
PROPHYSICS_API double ProPhysics_Read_Apparatus_Theta(const ProUniverse* pu,
    uint64_t base_node, uint32_t dim);
```

**Zweck:** Liest die mittlere Phase des Apparats als Radiant.

---

## §15 — Observer

### §15.1 — `ProPhysics_Init_Observer`

```c
PROPHYSICS_API void ProPhysics_Init_Observer(ProUniverse* pu,
    ProObserver* obs, uint64_t base_node, uint32_t size);
```

**Zweck:** Initialisiert einen Observer-Deskriptor. Keine Allokation.

### §15.2 — `ProPhysics_Observer_Read_Local`

```c
PROPHYSICS_API bool ProPhysics_Observer_Read_Local(const ProUniverse* pu,
    const ProObserver* obs, ProAmpVector* out_amp);
```

**Zweck:** Mittelwert der `amp_grid`-Vektoren über die Observer-Region.

### §15.3 — `ProPhysics_Get_Environment_Trace`

```c
PROPHYSICS_API ProU128 ProPhysics_Get_Environment_Trace(
    const ProUniverse* pu, const ProObserver* obs);
```

**Zweck:** Summe `Σ_{x ∉ obs} Σ_b |c_b(x)|²` als `ProU128`.

### §15.4 — `ProPhysics_Apply_Local_Amplitude_Diffusion`

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Amplitude_Diffusion(
    ProUniverse* pu, uint32_t rate_percent);
```

**Zweck:** Lineare Diffusion:
`ψ_k ← (1−α)·ψ_k + (α/n_nb)·Σ ψ_nb`.

**`rate_percent`** in `[0, 100]` → `α = rate_percent / 100`.

### §15.5 — `ProPhysics_Observer_Measure_CHSH`

```c
PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH(
    const ProUniverse* pu,
    const ProObserver* obs_A, const ProObserver* obs_B,
    double theta_a, double theta_b,
    int* out_a, int* out_b);
```

**Zweck:** CHSH-Messung basierend auf Observer-Mittelwerten.

### §15.6 — `ProPhysics_Init_Chaotic_Source`

```c
PROPHYSICS_API void ProPhysics_Init_Chaotic_Source(ProUniverse* pu,
    uint64_t base_node, uint32_t dim, uint64_t seed);
```

**Zweck:** Initialisiert eine `dim × dim`-Quelle mit chaotischen
λ-Werten (splitmix64).

**Physik:** `ψ = cos(λ/2)|up> + sin(λ/2)|down>`.

### §15.7 — `ProPhysics_Apply_Nonlinear_Diffusion_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Diffusion_Tick(
    ProUniverse* pu, uint32_t rate_percent,
    uint64_t saturation_threshold_q62);
```

**Zweck:** Diffusion + GP-artige Sättigung.

**Sättigung:** `threshold == 0` deaktiviert.

### §15.8 — `ProPhysics_Observer_Measure_CHSH_Projected`

```c
PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH_Projected(
    const ProUniverse* pu,
    const ProObserver* obs_A, const ProObserver* obs_B,
    double theta_a, double theta_b,
    uint64_t rng[4],
    int* out_a, int* out_b,
    uint64_t* out_node_a, uint64_t* out_node_b);
```

**Zweck:** CHSH-Messung mit zufälligem Einzelknoten pro Seite.

### §15.9 — `ProPhysics_Apply_Local_Dephasing_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Dephasing_Tick(
    ProUniverse* pu, uint32_t strength_percent);
```

**Zweck:** Dephasing von `coeff[UR_NEGATRON_CCW]` um einen
deterministischen Winkel.

---

## §16 — Shared Reference (U4)

### §16.1 — `ProPhysics_Entangle_Nodes`

```c
PROPHYSICS_API bool ProPhysics_Entangle_Nodes(ProUniverse* pu,
    uint64_t a, uint64_t b);
```

**Zweck:** Vereinigt die Klassen von `a` und `b` (Union-Find). Nach dem
Aufruf ist `amp_grid[a] == amp_grid[b]` (shared pointer).

### §16.2 — `ProPhysics_Dissociate_Node`

```c
PROPHYSICS_API bool ProPhysics_Dissociate_Node(ProUniverse* pu, uint64_t a);
```

**Zweck:** Trennt Knoten `a` von seiner Klasse.

### §16.3 — `ProPhysics_Is_Entangled`

```c
PROPHYSICS_API bool ProPhysics_Is_Entangled(
    const ProUniverse* pu, uint64_t a, uint64_t b);
```

**Zweck:** Prüft, ob `a` und `b` in derselben Klasse sind.

### §16.4 — `ProPhysics_Get_Representative`

```c
PROPHYSICS_API uint64_t ProPhysics_Get_Representative(
    const ProUniverse* pu, uint64_t a);
```

**Zweck:** Repräsentant der Klasse von `a`.

### §16.5 — `ProPhysics_Shared_Sync`

```c
PROPHYSICS_API void ProPhysics_Shared_Sync(ProUniverse* pu);
```

**Zweck:** Kopiert den Repräsentanten-Amp-Vektor auf alle Mitglieder.

### §16.6 — `ProPhysics_Shared_Class_Count`

```c
PROPHYSICS_API uint64_t ProPhysics_Shared_Class_Count(const ProUniverse* pu);
```

**Zweck:** Anzahl der Klassen.

### §16.7 — `ProPhysics_Shared_Tick_Reps`

```c
PROPHYSICS_API void ProPhysics_Shared_Tick_Reps(
    ProUniverse* pu, uint32_t phase_step_q15);
```

**Zweck:** Klassen-Tick. Rotiert Repräsentanten paarweise. Wird
**intern** von `Apply_Amp_Step` aufgerufen, wenn `shared.active == 1`.

---

## §17 — Spin-1/2 (Etappe 19)

### §17.1 — `ProPhysics_Entangle_Nodes_Singlet`

```c
PROPHYSICS_API bool ProPhysics_Entangle_Nodes_Singlet(
    ProUniverse* pu, uint64_t a, uint64_t b);
```

**Zweck:** Wie `Entangle_Nodes`, markiert `b` zusätzlich als spin-flipped.

### §17.2 — `ProPhysics_Is_Spin_Flipped`

```c
PROPHYSICS_API bool ProPhysics_Is_Spin_Flipped(
    const ProUniverse* pu, uint64_t k);
```

**Zweck:** Liest das Spin-Flip-Flag (`reserved_gating` Bit 0).

### §17.3 — `ProPhysics_Get_Node_Spin_View`

```c
PROPHYSICS_API bool ProPhysics_Get_Node_Spin_View(
    const ProUniverse* pu, uint64_t k,
    int32_t* out_re_up, int32_t* out_re_down);
```

**Zweck:** Liest `(c_up, c_down)` mit angewendetem Flip.

**Physik:** Bei Flip: `(c_up, c_down) → (c_down, −c_up)`.

### §17.4 — `ProPhysics_Apply_Nonlinear_Phase_Step_Spin`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Spin(
    ProUniverse* pu,
    int32_t  g_spin_q15_signed,
    uint32_t phase_step_q15);
```

**Zweck:** Spin-abhängige Phase auf {up, down}.

**Physik:** `phase = ±g · |c_b|² · dt`.

---

## §18 — Dirac (Etappe 21)

### §18.1 — `ProPhysics_Verify_Gamma_Algebra`

```c
PROPHYSICS_API double ProPhysics_Verify_Gamma_Algebra(void);
```

**Zweck:** Prüft `{γ^μ, γ^ν} = 2η^{μν}·I` und `{γ⁵, γ^μ} = 0`.

**Rückgabe:** `max_err` über alle Kommutatoren.

### §18.2 — `ProPhysics_Apply_Dirac_Mass_Term`

```c
PROPHYSICS_API void ProPhysics_Apply_Dirac_Mass_Term(
    ProUniverse* pu, int32_t mass_q15);
```

**Zweck:** Lokale L↔R-Rotation pro Spin-Kanal.

### §18.3 — `ProPhysics_Apply_Dirac_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Dirac_Step(
    ProUniverse* pu, int32_t mass_q15, uint32_t theta_q15);
```

**Zweck:** Ein Dirac-Tick: räumlicher Transport mit α-Kopplung +
Massenterm.

**Dispatch:** Wird intern von `Apply_Amp_Step` aufgerufen, wenn
`dirac_active == 1`.

---

## §19 — SU(2)-Eichfeld (Etappe 22)

### §19.1 — `ProPhysics_Set_Edge_SU2`

```c
PROPHYSICS_API bool ProPhysics_Set_Edge_SU2(
    ProUniverse* pu, uint64_t src, uint8_t ch,
    int32_t a_re_q30, int32_t a_im_q30,
    int32_t b_re_q30, int32_t b_im_q30);
```

**Zweck:** Setzt einen SU(2)-Link in Quaternion-Form `(a, b)`.

**Skala:** 2³⁰. Die Funktion **normalisiert** automatisch.

**Auto-Aktivierung:** Setzt `su2_active = 1`.

### §19.2 — `ProPhysics_Set_Edge_SU2_AxisAngle`

```c
PROPHYSICS_API bool ProPhysics_Set_Edge_SU2_AxisAngle(
    ProUniverse* pu, uint64_t src, uint8_t ch,
    double nx, double ny, double nz, double alpha);
```

**Zweck:** Setzt Link via Achse-Winkel:
`U = cos(α/2)·I − i·sin(α/2)·(n·σ)`.

### §19.3 — `ProPhysics_Get_Edge_SU2`

```c
PROPHYSICS_API bool ProPhysics_Get_Edge_SU2(
    const ProUniverse* pu, uint64_t src, uint8_t ch,
    int32_t* out_a_re_q30, int32_t* out_a_im_q30,
    int32_t* out_b_re_q30, int32_t* out_b_im_q30);
```

**Zweck:** Liest einen SU(2)-Link.

### §19.4 — `ProPhysics_Wilson_Loop_SU2`

```c
PROPHYSICS_API bool ProPhysics_Wilson_Loop_SU2(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t* path_channels,
    uint32_t        path_len,
    int32_t* out_a_re_q30, int32_t* out_a_im_q30,
    int32_t* out_b_re_q30, int32_t* out_b_im_q30);
```

**Zweck:** SU(2)-Wilson-Loop entlang eines geschlossenen Pfads.

**Konvention:** `W(C) = U(p[0]→p[1]) · U(p[1]→p[2]) · ... · U(p[n-1]→p[0])`
(Vorwärts, Standard-Lattice-QCD).

### §19.5 — `ProPhysics_Wilson_Loop_SU2_Trace`

```c
PROPHYSICS_API double ProPhysics_Wilson_Loop_SU2_Trace(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t* path_channels,
    uint32_t        path_len);
```

**Zweck:** `Tr(W) = 2·Re(a)`, normiert auf 1.0 = Identität.

### §19.6 — `ProPhysics_Apply_Local_SU2_Gauge`

```c
PROPHYSICS_API void ProPhysics_Apply_Local_SU2_Gauge(
    ProUniverse* pu, const int32_t* lambda_q30);
```

**Zweck:** Lokale SU(2)-Eichtransformation:
`U(x→y) ← g(x)·U(x→y)·g(y)†`.

**Parameter:** `lambda_q30` ist ein Array der Länge `total_nodes · 4`,
das pro Knoten `(a_re, a_im, b_re, b_im)` in Q30 enthält.

### §19.7 — `ProPhysics_Verify_SU2_Quaternion`

```c
PROPHYSICS_API double ProPhysics_Verify_SU2_Quaternion(void);
```

**Zweck:** Prüft Quaternion-Algebra gegen 2×2-Matrix-Multiplikation
und Pauli-Kommutator.

**Rückgabe:** `max_err`.

---

## §20 — SU(2)-Dynamik (Etappe 22b)

### §20.1 — `ProPhysics_Enable_SU2_Dynamics`

```c
PROPHYSICS_API void ProPhysics_Enable_SU2_Dynamics(ProUniverse* pu);
```

**Zweck:** Aktiviert Leapfrog-Dynamik für SU(2)-Links.

### §20.2 — `ProPhysics_Disable_SU2_Dynamics`

```c
PROPHYSICS_API void ProPhysics_Disable_SU2_Dynamics(ProUniverse* pu);
```

**Zweck:** Deaktiviert Dynamik. R7-konform: keine Pfadänderung.

### §20.3 — `ProPhysics_Is_SU2_Dynamics_Active`

```c
PROPHYSICS_API int ProPhysics_Is_SU2_Dynamics_Active(const ProUniverse* pu);
```

**Zweck:** `0`/`1`.

### §20.4 — `ProPhysics_Set_SU2_Yang_Mills`

```c
PROPHYSICS_API void ProPhysics_Set_SU2_Yang_Mills(
    ProUniverse* pu, int32_t g2_q15);
```

**Zweck:** Setzt die Yang-Mills-Kopplung `g²` in Q15.

### §20.5 — `ProPhysics_Apply_SU2_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_SU2_Tick(
    ProUniverse* pu, uint32_t dt_q15);
```

**Zweck:** Ein Leapfrog-Schritt (Stoermer-Verlet).

**Ablauf:**
1. `E ← E + (dt/2)·g²·F(U)`
2. `U ← exp(i·dt·E)·U`
3. `E ← E + (dt/2)·g²·F(U_neu)`

**Dispatch:** Wird intern von `Apply_Amp_Step` aufgerufen, wenn
`su2_dynamics_active == 1`.

### §20.6 — `ProPhysics_SU2_Plaquette_Action`

```c
PROPHYSICS_API double ProPhysics_SU2_Plaquette_Action(const ProUniverse* pu);
```

**Zweck:** Globale Wilson-Plaquette-Action
`S_plaq = Σ (1 − 0.5·Re Tr(W))`.

### §20.7 — `ProPhysics_SU2_Total_Energy`

```c
PROPHYSICS_API double ProPhysics_SU2_Total_Energy(const ProUniverse* pu);
```

**Zweck:** `H = S_plaq + ½ Σ |E|²`.

### §20.8 — `ProPhysics_SU2_Link_Plaquette_Sum`

```c
PROPHYSICS_API double ProPhysics_SU2_Link_Plaquette_Sum(
    const ProUniverse* pu, uint64_t x, uint8_t mu);
```

**Zweck:** Summe der Wilson-Aktionen aller Plaquettes, die Link `(x, mu)`
enthalten. In 3D: 4 Plaquettes.

**Anwendung:** Metropolis / HMC — `dS = Link_Plaquette_Sum(neu) −
Link_Plaquette_Sum(alt)`.

**Read-only.**

---

## §21 — Tensor

### §21.1 — Lifecycle

```c
PROPHYSICS_API bool ProPhysics_Tensor_Create_Pair(
    ProUniverse* pu, uint64_t node_a, uint64_t node_b,
    uint32_t* out_pair_id);

PROPHYSICS_API bool ProPhysics_Tensor_Destroy_Pair(
    ProUniverse* pu, uint32_t pair_id);
```

**Zweck:** Erzeugt/zerstört ein Tensor-Paar zwischen zwei Knoten.

**Fehler:** `Create` liefert `false`, wenn `node_a == node_b` oder
Kapazität erschöpft.

### §21.2 — Zustands-Zugriff

```c
PROPHYSICS_API bool ProPhysics_Tensor_Set_State(
    ProUniverse* pu, uint32_t pair_id,
    const ProAmpQ31 coeff[PRO_TENSOR_DIM]);

PROPHYSICS_API bool ProPhysics_Tensor_Get_State(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 coeff_out[PRO_TENSOR_DIM]);
```

### §21.3 — Gatter

**1-Qubit-Gatter:**

```c
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Single_Qubit_Gate(
    ProUniverse* pu, uint32_t pair_id, int side,
    const ProAmpQ31 U[2][2]);
```

`side = 0` → `U ⊗ I`, `side = 1` → `I ⊗ U`.

**Convenience-Wrapper:**

| Funktion | Gatter |
|---|---|
| `Tensor_Apply_Hadamard(pu, pair_id, side)` | H |
| `Tensor_Apply_X(pu, pair_id, side)` | X |
| `Tensor_Apply_Y(pu, pair_id, side)` | Y |
| `Tensor_Apply_Z(pu, pair_id, side)` | Z |

**2-Qubit-Gatter:**

```c
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Two_Qubit_Gate(
    ProUniverse* pu, uint32_t pair_id,
    const ProAmpQ31 G[4][4]);
```

**Convenience-Wrapper:**

| Funktion | Gatter |
|---|---|
| `Tensor_Apply_CNOT(pu, pair_id, side_control)` | CNOT |
| `Tensor_Apply_CZ(pu, pair_id)` | CZ |
| `Tensor_Apply_SWAP(pu, pair_id)` | SWAP |
| `Tensor_Apply_SqrtSwap(pu, pair_id)` | √SWAP |

### §21.4 — Partial Traces

```c
PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_A(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);

PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_B(
    const ProUniverse* pu, uint32_t pair_id,
    ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
```

**Rückgabe:** `ρ_A` bzw. `ρ_B` als 8×8-Matrix in Q31.

### §21.5 — Entropie

```c
PROPHYSICS_API double ProPhysics_Von_Neumann_Entropy(
    const ProAmpQ31 rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);

PROPHYSICS_API double ProPhysics_Tensor_Entanglement_Entropy(
    const ProUniverse* pu, uint32_t pair_id);
```

### §21.6 — Messung

```c
PROPHYSICS_API bool ProPhysics_Tensor_Measure_Projective(
    const ProUniverse* pu, uint32_t pair_id,
    double theta_a, double theta_b,
    uint64_t rng[4], int* out_a, int* out_b);
```

**Zweck:** Echte Born-Regel-Messung auf Tensor-Paar.

### §21.7 — Sync

```c
PROPHYSICS_API bool ProPhysics_Tensor_Sync_To_Amp(ProUniverse* pu);

PROPHYSICS_API bool ProPhysics_Tensor_Sync_From_Amp(
    ProUniverse* pu, uint32_t pair_id);
```

**`Sync_To_Amp`:** Schreibt dominante Eigenvektoren nach `amp_grid`.
**`Sync_From_Amp`:** Äußeres Produkt der Knoten-Amplituden.

### §21.8 — Fermionen

```c
PROPHYSICS_API bool ProPhysics_Tensor_Fermionize(
    ProUniverse* pu, uint32_t pair_id);

PROPHYSICS_API bool ProPhysics_Tensor_Is_Antisymmetric(
    const ProUniverse* pu, uint32_t pair_id, int32_t tol_q31);

PROPHYSICS_API double ProPhysics_Tensor_Pauli_Violation(
    const ProUniverse* pu, uint32_t pair_id);

PROPHYSICS_API double ProPhysics_Tensor_Antisymmetrize(
    ProUniverse* pu, uint32_t pair_id);

PROPHYSICS_API bool ProPhysics_Tensor_Set_Slater(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_a, uint8_t orbital_b);
```

### §21.9 — Hopping und XX-Kopplung

```c
PROPHYSICS_API bool ProPhysics_Tensor_Apply_Hopping(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);

PROPHYSICS_API bool ProPhysics_Tensor_Set_Hopping(
    ProUniverse* pu, uint32_t pair_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);

PROPHYSICS_API bool ProPhysics_Tensor_Set_Coupling(
    ProUniverse* pu, uint32_t pair_id, int32_t theta_q15);
```

### §21.10 — Mark / Auto-Sync

```c
PROPHYSICS_API bool ProPhysics_Tensor_Mark_Nodes(
    ProUniverse* pu, uint32_t pair_id);

PROPHYSICS_API bool ProPhysics_Tensor_Unmark_Nodes(
    ProUniverse* pu, uint32_t pair_id);

PROPHYSICS_API void ProPhysics_Tensor_Set_Auto_Sync(
    ProUniverse* pu, int enable);
```

### §21.11 — SU(2)-Rotation auf Spinor-Paaren

```c
PROPHYSICS_API bool ProPhysics_Apply_SU2_Rotation(
    ProUniverse* pu, uint64_t node, int which_spinor,
    double nx, double ny, double nz, double alpha);

PROPHYSICS_API bool ProPhysics_Measure_Spin(
    const ProUniverse* pu, uint64_t node, int which_spinor,
    double* out_sx, double* out_sy, double* out_sz);

PROPHYSICS_API double ProPhysics_Verify_SU2_Algebra(void);
```

### §21.12 — Tensor ↔ Fock Adapter

```c
PROPHYSICS_API bool ProPhysics_Tensor_To_Fock(
    ProUniverse* pu, uint32_t pair_id, uint64_t* out_fock_id);

PROPHYSICS_API bool ProPhysics_Fock_To_Tensor(
    const ProUniverse* pu, uint64_t fock_id, uint32_t pair_id);
```

---

## §22 — Fock

### §22.1 — Popcount

```c
PROPHYSICS_API uint32_t ProPhysics_Fock_Popcount(uint8_t bits);
```

**Zweck:** SWAR-Popcount auf 8 Bit.

### §22.2 — Lifecycle

```c
PROPHYSICS_API bool ProPhysics_Fock_Create(
    ProUniverse* pu, uint64_t* out_fock_id);

PROPHYSICS_API bool ProPhysics_Fock_Destroy(
    ProUniverse* pu, uint64_t fock_id);

PROPHYSICS_API uint64_t ProPhysics_Fock_Count(const ProUniverse* pu);
```

### §22.3 — Basiszustand

```c
PROPHYSICS_API bool ProPhysics_Fock_Set_Basis(
    ProUniverse* pu, uint64_t fock_id, uint8_t bits);
```

**Zweck:** Setzt Fock-Zustand auf Basiszustand `bits` (Bitmaske).

### §22.4 — Amplitude

```c
PROPHYSICS_API bool ProPhysics_Fock_Get_Amplitude(
    const ProUniverse* pu, uint64_t fock_id, uint8_t bits,
    int32_t* out_re_q31, int32_t* out_im_q31);

PROPHYSICS_API bool ProPhysics_Fock_Set_Amplitude(
    ProUniverse* pu, uint64_t fock_id, uint8_t bits,
    int32_t re_q31, int32_t im_q31);
```

### §22.5 — Erzeuger / Vernichter

```c
PROPHYSICS_API bool ProPhysics_Fock_Apply_Create(
    ProUniverse* pu, uint64_t fock_id, uint8_t mode);

PROPHYSICS_API bool ProPhysics_Fock_Apply_Annihilate(
    ProUniverse* pu, uint64_t fock_id, uint8_t mode);

PROPHYSICS_API bool ProPhysics_Fock_Apply_Create_Plus_Annihilate(
    ProUniverse* pu, uint64_t fock_id, uint8_t mode);
```

**Rückgabe:** `false`, wenn Ergebnis exakt Null ist (Pauli-Blockade).

### §22.6 — Antikommutatoren

```c
PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_CD(
    ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j);

PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_CC(
    ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j);

PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_DD(
    ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j);
```

### §22.7 — Hopping

```c
PROPHYSICS_API bool ProPhysics_Fock_Apply_Hopping(
    ProUniverse* pu, uint64_t fock_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
```

### §22.8 — Norm, Teilchenzahl, Is_Zero

```c
PROPHYSICS_API double ProPhysics_Fock_Norm(
    const ProUniverse* pu, uint64_t fock_id);

PROPHYSICS_API double ProPhysics_Fock_Particle_Number(
    const ProUniverse* pu, uint64_t fock_id);

PROPHYSICS_API bool ProPhysics_Fock_Is_Zero(
    const ProUniverse* pu, uint64_t fock_id);
```

### §22.9 — Clone, Compare, Scale

```c
PROPHYSICS_API bool ProPhysics_Fock_Clone(
    ProUniverse* pu, uint64_t src_fock_id, uint64_t* out_dst_fock_id);

PROPHYSICS_API double ProPhysics_Fock_Compare(
    const ProUniverse* pu, uint64_t fid_a, uint64_t fid_b);

PROPHYSICS_API bool ProPhysics_Fock_Scale(
    ProUniverse* pu, uint64_t fock_id, int32_t scale_q31);
```

---

## §23 — Density

### §23.1 — 8×8-Knoten-Dichte

```c
PROPHYSICS_API bool ProPhysics_Density_Create(
    ProUniverse* pu, uint32_t* out_rho_id);

PROPHYSICS_API bool ProPhysics_Density_Destroy(
    ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API uint64_t ProPhysics_Density_Count(const ProUniverse* pu);

PROPHYSICS_API bool ProPhysics_Density_From_Pure(
    ProUniverse* pu, uint32_t rho_id, const ProAmpVector* psi);

PROPHYSICS_API bool ProPhysics_Density_From_Mixture(
    ProUniverse* pu, uint32_t rho_id,
    const ProAmpVector* states, const double* weights, uint32_t n);

PROPHYSICS_API bool ProPhysics_Density_Get_Element(
    const ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t* out_re, int32_t* out_im);

PROPHYSICS_API bool ProPhysics_Density_Set_Element(
    ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t re, int32_t im);

PROPHYSICS_API double ProPhysics_Density_Trace(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API double ProPhysics_Density_Purity(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API double ProPhysics_Density_Von_Neumann_Entropy(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API double ProPhysics_Density_Offdiag_Magnitude(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API bool ProPhysics_Density_Apply_Unitary(
    ProUniverse* pu, uint32_t rho_id,
    const ProAmpQ31 U[PRO_DENSITY_DIM][PRO_DENSITY_DIM]);

PROPHYSICS_API bool ProPhysics_Density_Apply_Lindblad_Step(
    ProUniverse* pu, uint32_t rho_id,
    uint32_t lindblad_kind, double strength);
```

### §23.2 — 64×64-Tensor-Dichte

```c
PROPHYSICS_API bool ProPhysics_TensorDensity_Create(
    ProUniverse* pu, uint32_t pair_id, uint32_t* out_rho_id);

PROPHYSICS_API bool ProPhysics_TensorDensity_Destroy(
    ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API uint64_t ProPhysics_TensorDensity_Count(
    const ProUniverse* pu);

PROPHYSICS_API bool ProPhysics_TensorDensity_From_Pair(
    ProUniverse* pu, uint32_t rho_id, uint32_t pair_id);

PROPHYSICS_API bool ProPhysics_TensorDensity_Get_Element(
    const ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t* out_re, int32_t* out_im);

PROPHYSICS_API double ProPhysics_TensorDensity_Trace(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API double ProPhysics_TensorDensity_Purity(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API bool ProPhysics_TensorDensity_Partial_Trace_A(
    const ProUniverse* pu, uint32_t rho_id, uint32_t out_rho_id);

PROPHYSICS_API bool ProPhysics_TensorDensity_Partial_Trace_B(
    const ProUniverse* pu, uint32_t rho_id, uint32_t out_rho_id);

PROPHYSICS_API bool ProPhysics_TensorDensity_Apply_Local_Lindblad(
    ProUniverse* pu, uint32_t rho_id, int side,
    uint32_t lindblad_kind, double strength);
```

### §23.3 — 256×256-Fock-Dichte

```c
PROPHYSICS_API bool ProPhysics_FockDensity_Create(
    ProUniverse* pu, uint64_t fock_id, uint32_t* out_rho_id);

PROPHYSICS_API bool ProPhysics_FockDensity_Destroy(
    ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API uint64_t ProPhysics_FockDensity_Count(
    const ProUniverse* pu);

PROPHYSICS_API bool ProPhysics_FockDensity_From_Fock(
    ProUniverse* pu, uint32_t rho_id, uint64_t fock_id);

PROPHYSICS_API bool ProPhysics_FockDensity_Get_Element(
    const ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t* out_re, int32_t* out_im);

PROPHYSICS_API bool ProPhysics_FockDensity_Set_Element(
    ProUniverse* pu, uint32_t rho_id, uint32_t i, uint32_t j,
    int32_t re, int32_t im);

PROPHYSICS_API double ProPhysics_FockDensity_Trace(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API double ProPhysics_FockDensity_Purity(
    const ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API double ProPhysics_FockDensity_Mode_Occupation(
    const ProUniverse* pu, uint32_t rho_id, uint32_t mode);

PROPHYSICS_API bool ProPhysics_FockDensity_Apply_Mode_Lindblad(
    ProUniverse* pu, uint32_t rho_id, uint32_t mode,
    uint32_t lindblad_kind, double strength);

PROPHYSICS_API bool ProPhysics_FockDensity_Partial_Trace_Modes(
    const ProUniverse* pu, uint32_t rho_id,
    uint32_t n_keep, ProAmpQ31* out_rho, uint32_t* out_dim);
```

### §23.4 — Auto-Sync

```c
PROPHYSICS_API void ProPhysics_Density_Set_Auto_Sync(
    ProUniverse* pu, int enable);

PROPHYSICS_API bool ProPhysics_TensorDensity_Sync_From_Pair(
    ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API bool ProPhysics_FockDensity_Sync_From_Fock(
    ProUniverse* pu, uint32_t rho_id);

PROPHYSICS_API bool ProPhysics_TensorDensity_Sync_All_From_Pairs(
    ProUniverse* pu);

PROPHYSICS_API bool ProPhysics_FockDensity_Sync_All_From_Fock(
    ProUniverse* pu);
```

---

## §24 — U5-Invariante

### §24.1 — `ProPhysics_Measure_Amp_Invariant`

```c
PROPHYSICS_API ProU128 ProPhysics_Measure_Amp_Invariant(
    const ProUniverse* pu);
```

**Zweck:** U5-Invariante `Σ_k Σ_b PRO_U5_W[b] · |c_b(k)|²`.

**Wichtig:** Bei aktivem `shared.active` zählt nur der Repräsentant jeder
Klasse.

### §24.2 — `ProPhysics_Weighted_Norm`

```c
PROPHYSICS_API ProU128 ProPhysics_Weighted_Norm(
    const ProUniverse* pu, uint64_t node_idx);
```

**Zweck:** U5-Norm eines einzelnen Knotens.

### §24.3 — `ProPhysics_Get_U_Field`

```c
PROPHYSICS_API const uint16_t* ProPhysics_Get_U_Field(const ProUniverse* pu);
```

**Zweck:** Liest das `u_field`-Array (Q16-Werte pro Knoten).

### §24.4 — `ProPhysics_Verify_Unitarity`

```c
PROPHYSICS_API double ProPhysics_Verify_Unitarity(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Prüft `U · U† = I`.

**Rückgabe:** `max_err`.

### §24.5 — `ProPhysics_Verify_U5_Commutator`

```c
PROPHYSICS_API double ProPhysics_Verify_U5_Commutator(
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Prüft, ob `U` die U5-Gewichte erhält.

### §24.6 — `ProPhysics_Apply_Unitary_Tick`

```c
PROPHYSICS_API void ProPhysics_Apply_Unitary_Tick(ProUniverse* pu,
    const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);
```

**Zweck:** Wendet eine 8×8-Matrix `U` auf alle Knoten an.

---

## §25 — Nichtlineare Phase-Steps

### §25.1 — `ProPhysics_Apply_Nonlinear_Phase_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step(
    ProUniverse* pu,
    int32_t  g_q15_signed,
    uint32_t phase_step_q15);
```

**Zweck:** GP-Selbstkopplung: `phase = g · |c_b|² · dt`.

### §25.2 — `ProPhysics_Apply_Nonlinear_Phase_Step_Dilated`

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(
    ProUniverse* pu,
    int32_t  g_q15_signed,
    uint32_t phase_step_q15);
```

**Zweck:** GP-Selbstkopplung mit **Lorentz-Dilatation** basierend auf
Nachbar-Phasengradienten.

**Nur 2D.**

---

## §26 — Vollständige Anwendungsbeispiele

### §26.1 — Minimalbeispiel: 2D-Gitter initialisieren und ticken

```c
#include "ProPhysics.h"

int main(void) {
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64 * 64);
    pu.grid_dim = 64;
    pu.grid_ndim = 2;

    /* Nachbarschaft aufbauen */
    for (uint32_t y = 0; y < 64; ++y) {
        for (uint32_t x = 0; x < 64; ++x) {
            const uint64_t k = y * 64 + x;
            const uint64_t xp = y * 64 + ((x + 1) % 64);
            const uint64_t xm = y * 64 + ((x + 63) % 64);
            const uint64_t yp = ((y + 1) % 64) * 64 + x;
            const uint64_t ym = ((y + 63) % 64) * 64 + x;
            ProPhysics_Link_Nodes(&pu, k, xp, PRO_NEIGHBOR_X_PLUS);
            ProPhysics_Link_Nodes(&pu, k, xm, PRO_NEIGHBOR_X_MINUS);
            ProPhysics_Link_Nodes(&pu, k, yp, PRO_NEIGHBOR_Y_PLUS);
            ProPhysics_Link_Nodes(&pu, k, ym, PRO_NEIGHBOR_Y_MINUS);
        }
    }

    /* 100 Ticks */
    for (int t = 0; t < 100; ++t) {
        ProPhysics_Tick(&pu, NULL);
    }

    /* U5 prüfen */
    ProU128 norm = ProPhysics_Measure_Amp_Invariant(&pu);

    ProPhysics_Free(&pu);
    return 0;
}
```

### §26.2 — Born-Statistik messen

```c
ProPhysics_Initialize(&pu, 64);
ProPhysics_Set_Node_Amplitude(&pu, 0, UR_POSITRON_CW, INT32_MAX / 2, 0);
ProPhysics_Set_Node_Amplitude(&pu, 0, UR_NEGATRON_CCW, INT32_MAX * 3 / 4, 0);

double p_up = ProPhysics_Get_Born_Probability(&pu, 0, UR_POSITRON_CW);
/* p_up ≈ 0.25 */
```

### §26.3 — SU(2)-Wilson-Loop messen

```c
ProUniverse pu;
ProPhysics_Initialize(&pu, 16 * 16 * 16);
pu.grid_dim = 16;
pu.grid_ndim = 3;
/* ... Nachbarschaft aufbauen ... */

/* Setze einen Link */
ProPhysics_Set_Edge_SU2_AxisAngle(&pu, 0, PRO_NEIGHBOR_X_PLUS,
    0.0, 0.0, 1.0, 0.5);

/* 2x2-Plaquette in der xy-Ebene */
uint64_t nodes[4] = { 0, 1, 16 * 1 + 1, 16 * 1 };
uint8_t  chans[4] = {
    PRO_NEIGHBOR_X_PLUS,
    PRO_NEIGHBOR_Y_PLUS,
    PRO_NEIGHBOR_X_MINUS,
    PRO_NEIGHBOR_Y_MINUS
};
double tr = ProPhysics_Wilson_Loop_SU2_Trace(&pu, nodes, chans, 4);
```

### §26.4 — SU(2)-Metropolis-Sampling

```c
ProPhysics_Enable_SU2_Dynamics(&pu);
ProPhysics_Set_SU2_Yang_Mills(&pu, 1000);

uint64_t rng[4] = { 1, 2, 3, 4 };

/* Thermalisierung */
for (int i = 0; i < 200; ++i) {
    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        for (uint8_t mu = 0; mu < 6; mu += 2) {
            /* Vorschlag: Gauss-Störung */
            double eps = 0.3;
            double da = eps * (pro_uniform01(rng) - 0.5);
            /* ... */
            double S_old = ProPhysics_SU2_Link_Plaquette_Sum(&pu, k, mu);
            /* Link ändern ... */
            double S_new = ProPhysics_SU2_Link_Plaquette_Sum(&pu, k, mu);
            double dS = S_new - S_old;
            double beta = 2.0;
            if (dS < 0.0 || pro_uniform01(rng) < exp(-beta * dS)) {
                /* akzeptieren */
            } else {
                /* Link zurücksetzen */
            }
        }
    }
}
```

### §26.5 — Dirac-Tick

```c
pu.dirac_active = 1;
pu.dirac_gamma_basis = PRO_GAMMA_BASIS_DIRAC;
pu.dirac_mass_q15 = 100;

for (int t = 0; t < 100; ++t) {
    ProPhysics_Tick(&pu, NULL);
}
```

### §26.6 — Tensor-Paar mit Bell-Zustand

```c
uint32_t pair_id;
ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pair_id);

/* Bell-Zustand |Φ+> = (|00> + |11>) / √2 */
ProAmpQ31 coeff[64] = {0};
coeff[0] = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);
coeff[1 * 8 + 1] = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);
ProPhysics_Tensor_Set_State(&pu, pair_id, coeff);

double C = ProPhysics_Tensor_Concurrence(&pu, pair_id);
/* C ≈ 1.0 */
```

### §26.7 — Fock-Antikommutator prüfen

```c
uint64_t fid;
ProPhysics_Fock_Create(&pu, &fid);

/* {c_0, c+_1} |0> = 0 */
ProPhysics_Fock_Apply_Anticomm_CD(&pu, fid, 0, 1);

double norm = ProPhysics_Fock_Norm(&pu, fid);
/* norm ≈ 0 */
```

### §26.8 — Lindblad-Dephasing

```c
uint32_t rho_id;
ProPhysics_Density_Create(&pu, &rho_id);

ProAmpVector psi = {0};
psi.coeff[UR_POSITRON_CW] = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);
psi.coeff[UR_NEGATRON_CCW] = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);

ProPhysics_Density_From_Pure(&pu, rho_id, &psi);
ProPhysics_Density_Apply_Lindblad_Step(
    &pu, rho_id, PRO_LINDBLAD_PHASE_DAMP, 0.1);

double purity = ProPhysics_Density_Purity(&pu, rho_id);
/* purity < 1.0 nach Dephasing */
```

---

## §27 — Fehlerbehandlung und Grenzen

### §27.1 — Was passiert bei Fehlern?

Alle öffentlichen Funktionen prüfen Argumente defensiv:

| Fehlerfall | Reaktion |
|---|---|
| `NULL`-Zeiger | Stiller Return (`false`, `0`, `void`) |
| Out-of-Range-Index | Stiller Return |
| Ungültiger Zustand | Stiller Return |
| Nicht-Zweierpotenz `grid_dim` | Silent Return |
| Speicher-Erschöpfung bei `Initialize` | `pu` wird genullt |

**Es gibt keine Exception, keine Fehlermeldung.** Der Kernel ist
embedded-fähig und verwendet keine `errno`-Konventionen.

### §27.2 — Was der Nutzer selbst prüfen muss

| Prüfung | Wo |
|---|---|
| `grid_dim` Zweierpotenz | vor `Apply_Edge_Transport_Colored[_3D]` |
| `grid_ndim` korrekt (2 oder 3) | vor 3D-Funktionen |
| Alle Knoten erreichbar | vor `ProPhysics_Tick` |
| `total_nodes` passt zu `amp_grid` | nach `Initialize` |

### §27.3 — Grenzen der API

| Limit | Wert |
|---|---|
| Max. Knoten | `SIZE_MAX / sizeof(ProAmpVector)` |
| Max. Kanäle | 16 (`CHANNELS_MAX`) |
| Max. Tensor-Paare | 256 |
| Max. Fock-Zustände | 64 |
| Max. Dichte-Matrizen | 64 |
| Max. Q31-Wert | `INT32_MAX` = 2147483647 |
| Max. Q30-Wert | `2³⁰` = 1073741824 |
| Kernel-Threading | Single-threaded |

### §27.4 — R-Konformität der API

| Regel | Bedeutung | Auswirkung auf API |
|---|---|---|
| **R1** | Kein `div`/`mod` im Hotpath | Alle Hotpath-Funktionen nutzen Bit-Shift |
| **R2** | Kein `malloc`/`calloc` im Hotpath | Ausnahme: `FockDensity_Apply_Mode_Lindblad` (2 MB Scratch, dokumentiert) |
| **R3** | U5-Invariante bleibt erhalten | Transport, Wave, Context sind U5-erhaltend |
| **R4** | Unitäre Dynamik | Alle Transport-/Wave-/Dirac-Steps sind unitär |
| **R5** | Keine stillen API-Brüche | Neue Funktionen kommen **additiv** hinzu |
| **R6** | Jede Etappe endet mit einem Test | Prio-All-Lauf prüft alle |
| **R7** | Keine Etappe ändert bestehende Pfade | Aktivierung via Flags, Default = alt |

---

## §28 — Was diese API **nicht** ist

- **Kein Objekt-orientiertes Interface.** Alle Funktionen arbeiten auf
  `ProUniverse*` — kein Opaque-Pointer.
- **Kein Thread-sicheres Interface.** Parallele Aufrufe auf demselben
  `ProUniverse` sind verboten.
- **Keine dynamische Typisierung.** Alle Typen sind compile-time.
- **Kein Allocator-Interface.** Der Nutzer kontrolliert nicht, wo der
  Kernel allokiert.
- **Kein Persistenz-Interface.** Es gibt kein `Save`/`Load`.
- **Keine Netzwerk- oder Datei-IO.** Alles läuft im RAM.
- **Keine Versionskompatibilität.** Bei strukturellen Änderungen
  ändern sich Struct-Größen — der Nutzer muss neu kompilieren.

---

## §29 — Versions-Historie dieses Dokuments

| Version | Datum | Änderung |
|---|---|---|
| 1.0 | 2026-09-25 | Erste Fassung, Etappe 23 |
| 1.0 | 2026-09-26 | Header auf Etappen-Schema umgestellt (Kernel 1.23.0, Etappe 23); §1.1 Modul-Zählung auf 12 korrigiert; §30 (Siehe auch) ergänzt; Verweise auf Modul-Docs aktualisiert |

**Hinweis zum Schema-Wechsel:** Frühere Versionen dieses Dokuments
trugen `Kernel-Version 3.0.0` (SemVer-ähnlich). Mit der Umstellung auf
das Etappen-Schema entspricht `3.0.0` jetzt `1.23.0`. Siehe
`CHANGELOG.md` §2.5.

---

## §30 — Siehe auch

| Thema | Datei |
|---|---|
| Changelog | `CHANGELOG.md` |
| Konfiguration | `docs/project/CONFIG.md` |
| Versions-Register | `docs/project/ProPhysics_VersionRegistry.md` |
| Amp-Modul | `docs/project/Amp.md` |
| Core-Modul | `docs/project/Core.md` |
| Density-Modul | `docs/project/Density.md` |
| Dirac-Modul | `docs/project/Dirac.md` |
| EPR-Modul | `docs/project/EPR.md` |
| Fock-Modul | `docs/project/Fock.md` |
| Gauge-Modul | `docs/project/Gauge.md` |
| Observer-Modul | `docs/project/Observer.md` |
| Shared-Modul | `docs/project/Shared.md` |
| SU2-Modul | `docs/project/SU2.md` |
| SU2-Dynamik | `docs/project/SU2_Dynamics.md` |
| Tensor-Modul | `docs/project/Tensor.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Build-System | `docs/build/BUILD_SCRIPT.md` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende API-Referenz v1.0.**