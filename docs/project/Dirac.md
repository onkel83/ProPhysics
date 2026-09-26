# ProPhysics — Modul: Dirac

**Datei:** `docs/project/Dirac.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_Dirac.c`
**Zweck:** Referenz für das Dirac-Modul: 4-Komponenten-Spinor und
Clifford-Algebra als signed permutations.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Dirac-Modul implementiert die **Dirac-Gleichung** auf dem
Amplitudengitter — mit **signed permutations** statt dichter
Matrixmultiplikation.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut (Rolle im Kernel) |
| §2 | Die 4-Komponenten-Darstellung |
| §3 | Gamma-Matrizen und ihre Repräsentation |
| §4 | α-Matrizen und 3D-Transport |
| §5 | Der Massenterm |
| §6 | Interne Helfer |
| §7 | Öffentliche API |
| §8 | Verwendungsmuster |
| §9 | Fallstricke |
| §10 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Amp.md` — Amp-Modul (Transport)
- `docs/project/CONFIG.md` — Konstanten (`PRO_DIRAC_DIM` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das Dirac-Modul erweitert die Amplituden-Basis des Kernels um eine
**4-Komponenten-Spinor-Struktur**. Die Komponenten liegen in den
Basis-Indizes **1..4**:

| Basis-Index | Komponente | Physik |
|:-:|---|---|
| 1 | `ψ_L↑` | linkshändig, Spin-up |
| 2 | `ψ_L↓` | linkshändig, Spin-down |
| 3 | `ψ_R↑` | rechtshändig, Spin-up |
| 4 | `ψ_R↓` | rechtshändig, Spin-down |

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Gamma-Tabellen** | Dirac- und Weyl-Darstellung (statisch) |
| **Alpha-Tabellen** | α^i = γ⁰γ^i für 3 Richtungen × 2 Darstellungen |
| **Algebra-Verifikation** | `Verify_Gamma_Algebra` |
| **Massenterm** | `Apply_Dirac_Mass_Term` |
| **Dirac-Tick** | `Apply_Dirac_Step` |

**Was das Modul nicht tut:**

- Keine Standard-Transport-Schritte (die macht `Amp`).
- Keine SU(2)-Links (`ProPhysics_SU2.c`).
- Kein Sampling.

**Aktivierung:** Das Modul wird aktiv, wenn `pu->dirac_active == 1`.
Dann dispatcht `Apply_Amp_Step` auf `Apply_Dirac_Step`.

---

## §2 — Die 4-Komponenten-Darstellung

### §2.1 — Basis-Layout

Von den 8 Basis-Zuständen pro Knoten sind **4 für Dirac reserviert**:

| Basis-Index | Konstante | Dirac-Komponente |
|:-:|---|---|
| 0 | `UR_NEUTRAL` | — |
| **1** | `UR_POSITRON_CW` | ψ_L↑ |
| **2** | `UR_POSITRON_CCW` | ψ_L↓ |
| **3** | `UR_NEGATRON_CW` | ψ_R↑ |
| **4** | `UR_NEGATRON_CCW` | ψ_R↓ |
| 5 | `UR_PHOTON` | — |
| 6, 7 | — | — |

**Mapping-Macros** (in `ProPhysics_Config.h`):

```c
#define PRO_DIRAC_TO_BASIS(ci)  ((uint8_t)((ci) + 1u))  /* 0..3 -> 1..4 */
#define PRO_DIRAC_FROM_BASIS(b) ((uint8_t)((b) - 1u))   /* 1..4 -> 0..3 */
```

### §2.2 — ProGammaOp-Struktur

Alle Gamma- und α-Matrizen werden als **signed permutations** dargestellt:

```c
typedef struct {
    uint8_t   perm[PRO_DIRAC_DIM];    /* Permutation 0..3 */
    ProAmpQ31 coeff[PRO_DIRAC_DIM];   /* Koeffizienten aus {+1,-1,+i,-i} */
} ProGammaOp;
```

**Wirkung auf einen Spinor:**

```
out[i] = coeff[i] · in[perm[i]]
```

**Vorteil:** Keine Matrixmultiplikation im Hotpath. 4 Indextäusche +
4 komplexe Multiplikationen mit Einheitskoeffizienten.

**Alle Koeffizienten** sind aus `{+1, -1, +i, -i}`. Multiplikation
mit diesen Werten ist eine Vorzeichen-/Re/Im-Operation.

---

## §3 — Gamma-Matrizen und ihre Repräsentation

### §3.1 — Dirac-Darstellung

| Index | Matrix | Form |
|:-:|---|---|
| 0 | γ⁰ | `diag(+1, +1, -1, -1)` |
| 1 | γ¹ | `[[0, σx], [-σx, 0]]` |
| 2 | γ² | `[[0, σy], [-σy, 0]]` |
| 3 | γ³ | `[[0, σz], [-σz, 0]]` |
| 4 | γ⁵ | `[[0, I], [I, 0]]` |

**Eigenschaften:**

- `{γ^μ, γ^ν} = 2·η^{μν}·I` mit `η = diag(+1, -1, -1, -1)`.
- `{γ⁵, γ^μ} = 0` für μ ∈ {0,1,2,3}.
- `(γ⁵)² = I`.
- `(γ^μ)² = ±I`.

### §3.2 — Weyl-Darstellung (chiral)

| Index | Matrix | Form |
|:-:|---|---|
| 0 | γ⁰ | `[[0, I], [I, 0]]` |
| 1 | γ¹ | identisch zu Dirac |
| 2 | γ² | identisch zu Dirac |
| 3 | γ³ | identisch zu Dirac |
| 4 | γ⁵ | `diag(-I, -I, +I, +I)` |

**Eigenschaften:** Dieselben wie Dirac-Darstellung. Die γ¹..γ³ sind
identisch, nur γ⁰ und γ⁵ unterscheiden sich.

### §3.3 — Auswahl der Darstellung

Der Nutzer setzt `pu->dirac_gamma_basis`:

```c
pu.dirac_gamma_basis = PRO_GAMMA_BASIS_DIRAC;   /* 0 */
pu.dirac_gamma_basis = PRO_GAMMA_BASIS_WEYL;    /* 1 */
```

Alle Operationen (Transport, Massenterm) verwenden dann die
entsprechende Tabelle.

### §3.4 — Verifikation

`ProPhysics_Verify_Gamma_Algebra` prüft:

- `{γ^μ, γ^ν} = 2·η^{μν}·I` für alle μ, ν ∈ {0..4} (mit η⁴⁴ = +1).
- Alle γ-Matrizen in **Dirac-Darstellung**.

**Rückgabe:** `max_err` (double). Idealwert: 0.

**Erwarteter Wert:** ~9,31e-10 (Q31-Rundung).

---

## §4 — α-Matrizen und 3D-Transport

### §4.1 — Definition

Die Dirac-Gleichung lautet in Hamilton-Form:

```
i·∂_t ψ = -i·α·∇ψ + m·β·ψ
```

wobei `α^i = γ⁰·γ^i`. Die α-Matrizen sind **hermitesch** und
**selbstinvers**: `(α^i)² = I`.

**Zahl der α-Matrizen:** 3 (für x, y, z).

### §4.2 — Dirac vs. Weyl

| Richtung | Dirac-Darstellung | Weyl-Darstellung |
|:-:|---|---|
| x | `[[0, σx], [σx, 0]]` | `diag(-σx, +σx)` |
| y | `[[0, σy], [σy, 0]]` | `diag(-σy, +σy)` |
| z | `[[0, σz], [σz, 0]]` | `diag(-σz, +σz)` |

### §4.3 — Transport-Muster

Der Dirac-Transport nutzt dieselbe **6-Sweep-Schachbrett-Struktur**
wie der Standard-3D-Transport, aber mit α-Kopplung:

```
Sweep 0, 1: x-Richtung (Kanal PRO_NEIGHBOR_X_PLUS)
Sweep 2, 3: y-Richtung (Kanal PRO_NEIGHBOR_Y_PLUS)
Sweep 4, 5: z-Richtung (Kanal PRO_NEIGHBOR_Z_PLUS)
```

**Paritätsmuster:** `(x + y + z) & 1`.

### §4.4 — 2-Knoten-Rotation mit α

Für jedes Knotenpaar `(a, b)` und jede Dirac-Komponente `i`:

```
a'[i] = c·a[i] + s·(α·b)[i]
b'[i] = c·b[i] - s·(α·a)[i]
```

wobei `(α·ψ)[i] = Σ_j α[i][j]·ψ[j]` eine **signed permutation**
ist.

**Parameter `c`, `s`:** Rationale Approximation aus
`pro_transport_coeffs` mit `theta_q15`.

---

## §5 — Der Massenterm

### §5.1 — Physik

Der Massenterm koppelt links- und rechtshändige Komponenten:

```
ψ_L' = c·ψ_L + s·ψ_R
ψ_R' = c·ψ_R - s·ψ_L
```

mit `c = cos(m·dt)`, `s = sin(m·dt)`.

**Kopplungs-Paare:** Pro Spin-Sektor:

| Spin | L-Index | R-Index |
|:-:|:-:|:-:|
| up | 1 (`UR_POSITRON_CW`) | 3 (`UR_NEGATRON_CW`) |
| down | 2 (`UR_POSITRON_CCW`) | 4 (`UR_NEGATRON_CCW`) |

### §5.2 — Koeffizienten-Berechnung

Die Koeffizienten werden über **rationale Approximation** von
`tan(θ/2)` bestimmt, wobei `θ = mass_q15 / 32768 · π`.

Die Funktion ruft **`pro_transport_coeffs`** auf (geteilt mit dem
Standard-Transport). Der Cache in `pu->last_*` wird mitgenutzt.

**Vorzeichen:** Bei negativem `mass_q15` wird `s_num` negiert.

### §5.3 — Knoten-Selektion

Nur Knoten mit `PRO_NODE_DIRAC_BIT` im `reserved_gating`-Byte werden
transformiert:

```c
if ((pu->ur_grid[k].reserved_gating & PRO_NODE_DIRAC_BIT) == 0u) {
    continue;
}
```

**Wenn `pu->ur_grid == NULL`:** Alle Knoten werden transformiert
(Fallback für Tests ohne `ur_grid`-Setup).

---

## §6 — Interne Helfer

### §6.1 — `pro_gamma_op_init`

```c
static void pro_gamma_op_init(ProGammaOp* op,
    uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3,
    ProAmpQ31 c0, ProAmpQ31 c1, ProAmpQ31 c2, ProAmpQ31 c3);
```

Initialisiert eine `ProGammaOp` in einer Zeile.

**Ersetzt** 16 wiederholte Init-Blöcke.

### §6.2 — `pro_dirac_init_once`

```c
static void pro_dirac_init_once(void);
```

Lazy-Init aller Tabellen (Dirac-Gamma, Weyl-Gamma, Dirac-Alpha,
Weyl-Alpha). Idempotent über `static bool s_ready`.

**Thread-Safety:** Nicht thread-safe. Im Kernel single-threaded.

### §6.3 — `pro_dirac_table`, `pro_dirac_alpha`

```c
static const ProGammaOp* pro_dirac_table(uint8_t basis, int index);
static const ProGammaOp* pro_dirac_alpha(uint8_t basis, int direction);
```

Tabellen-Auswahl basierend auf `pu->dirac_gamma_basis`.

### §6.4 — `pro_dirac_apply_op`

```c
static void pro_dirac_apply_op(const ProGammaOp* op,
    const ProAmpVector* in,
    ProAmpQ31 out[PRO_DIRAC_DIM]);
```

Wendet eine `ProGammaOp` auf die 4 Dirac-Komponenten an.

### §6.5 — `pro_dirac_transport_2node_with_alpha`

```c
static void pro_dirac_transport_2node_with_alpha(
    ProAmpVector* va, ProAmpVector* vb,
    const ProGammaOp* alpha,
    int64_t c_num, int64_t s_num, int64_t denom);
```

2-Knoten-Rotation mit α-Kopplung.

### §6.6 — `pro_dirac_transport_3d`

```c
static void pro_dirac_transport_3d(
    ProUniverse* pu, uint32_t theta_q15, uint32_t grid_dim);
```

3D-Transport mit α-Kopplung (6 Sweeps).

---

## §7 — Öffentliche API

### §7.1 — `ProPhysics_Verify_Gamma_Algebra`

```c
PROPHYSICS_API double ProPhysics_Verify_Gamma_Algebra(void);
```

**Zweck:** Prüft die Clifford-Algebra `{γ^μ, γ^ν} = 2·η^{μν}·I`
für Dirac-Darstellung.

**Rückgabe:** `max_err` (double). Idealwert: 0.

**Test-Ergebnis:** 9,31e-10.

### §7.2 — `ProPhysics_Apply_Dirac_Mass_Term`

```c
PROPHYSICS_API void ProPhysics_Apply_Dirac_Mass_Term(
    ProUniverse* pu,
    int32_t mass_q15);
```

**Zweck:** Wendet einen Massenterm-Schritt auf alle Dirac-Knoten an.

**Parameter:**

| Parameter | Bedeutung |
|---|---|
| `mass_q15` | Masse in Q15 (0 = No-Op) |
| Vorzeichen | Wird auf `s_num` angewendet |

**Nur aktive Knoten:** `reserved_gating & PRO_NODE_DIRAC_BIT`.

### §7.3 — `ProPhysics_Apply_Dirac_Step`

```c
PROPHYSICS_API void ProPhysics_Apply_Dirac_Step(
    ProUniverse* pu,
    int32_t  mass_q15,
    uint32_t theta_q15);
```

**Zweck:** Ein vollständiger Dirac-Tick.

**Ablauf:**

1. **Räumlicher Transport** mit α-Kopplung:
   - 3D (`grid_ndim == 3`): `pro_dirac_transport_3d`.
   - 2D (`grid_dim > 0`): `Apply_Edge_Transport_Colored` (Fallback).
   - Flach (`grid_dim == 0`): `Apply_Edge_Transport` (Fallback).
2. **Massenterm:** `Apply_Dirac_Mass_Term` (nur bei `mass_q15 != 0`).

**Dispatch:** Wird intern von `Apply_Amp_Step` aufgerufen, wenn
`pu->dirac_active == 1`.

---

## §8 — Verwendungsmuster

### §8.1 — Dirac-Tick mit Masse

```c
ProUniverse pu;
ProPhysics_Initialize(&pu, 32 * 32 * 32);
pu.grid_dim = 32;
pu.grid_ndim = 3;

/* Dirac aktivieren */
pu.dirac_active = 1;
pu.dirac_gamma_basis = PRO_GAMMA_BASIS_DIRAC;
pu.dirac_mass_q15 = 1000;   /* m ≈ 0.03 */

/* Knoten als Dirac markieren */
for (uint64_t k = 0; k < pu.total_nodes; ++k) {
    pu.ur_grid[k].reserved_gating |= PRO_NODE_DIRAC_BIT;
}

/* Topologie aufbauen ... */

/* Dirac-Initialzustand setzen (z.B. Bloch-Zustand) */
/* ... */

for (int t = 0; t < 100; ++t) {
    ProPhysics_Tick(&pu, NULL);   /* dispatcht auf Dirac-Step */
}
```

### §8.2 — Gamma-Algebra prüfen

```c
const double err = ProPhysics_Verify_Gamma_Algebra();
if (err > 1e-9) {
    fprintf(stderr, "Gamma-Algebra verletzt: %.4e\n", err);
}
```

### §8.3 — Weyl-Darstellung

```c
pu.dirac_active = 1;
pu.dirac_gamma_basis = PRO_GAMMA_BASIS_WEYL;
pu.dirac_mass_q15 = 500;

/* Verhalten identisch zu Dirac, nur andere Basis */
for (int t = 0; t < 50; ++t) {
    ProPhysics_Tick(&pu, NULL);
}
```

### §8.4 — Manueller Massenterm

```c
/* Ohne vollen Dirac-Step */
ProPhysics_Apply_Dirac_Mass_Term(&pu, 1000);

/* Nur der Massenterm wird angewendet, kein Transport */
```

### §8.5 — Masseloser Dirac

```c
pu.dirac_active = 1;
pu.dirac_mass_q15 = 0;   /* Massenterm ist No-Op */

/* Nur der räumliche Transport läuft */
```

---

## §9 — Fallstricke

### §9.1 — Dirac-Bit muss gesetzt sein

`Apply_Dirac_Mass_Term` transformiert **nur** Knoten mit
`reserved_gating & PRO_NODE_DIRAC_BIT`. Ohne dieses Bit ist der
Massenterm ein No-Op.

**Fallback:** Wenn `pu->ur_grid == NULL`, werden alle Knoten
transformiert.

### §9.2 — Reihenfolge Transport → Massenterm

`Apply_Dirac_Step` führt erst den Transport aus, dann den Massenterm.
Diese Reihenfolge ist physikalisch sinnvoll (Räumliche Evolution vor
lokaler Rotation).

### §9.3 — 2D-Fallback

Bei `grid_dim > 0` und `grid_ndim == 2` wird der **Standard-Transport**
(`Apply_Edge_Transport_Colored`) verwendet, **ohne α-Kopplung**.

**Konsequenz:** In 2D ist der Dirac-Transport unvollständig. Die
volle α-Kopplung gibt es nur in 3D.

### §9.4 — `mass_q15 == 0` ist No-Op

Der Massenterm wird komplett übersprungen. Nur Transport läuft.

### §9.5 — `theta_q15 == 0` ist No-Op

Der Transport wird komplett übersprungen. Nur Massenterm läuft.

### §9.6 — Vorzeichen-Konvention

Bei `mass_q15 < 0` wird nur `s_num` negiert. `c_num` und `denom`
bleiben positiv. Das ist äquivalent zu einer Rotation in
umgekehrter Richtung.

### §9.7 — Lazy-Init ist nicht thread-safe

`pro_dirac_init_once` schützt nicht gegen Race Conditions. Bei
Multi-Threaded-Nutzung müsste der Init explizit vor dem ersten Tick
erfolgen (z.B. durch Aufruf von `ProPhysics_Verify_Gamma_Algebra`).

### §9.8 — Diract-Transport in 3D mit `dim_mask`

`pro_dirac_transport_3d` prüft `(grid_dim & (grid_dim - 1u)) == 0`.
Ohne Zweierpotenz erfolgt **stiller Return**.

### §9.9 — Cache-Sharing mit Transport

Der Massenterm nutzt den **`pro_transport_coeffs`-Cache** in
`pu->last_*`. Bei Aufruf mit einem anderen `theta_q15` als der letzte
Transport überschreibt der Massenterm den Cache.

**Konsequenz:** Kein Fehler, aber ggf. Cache-Thrashing bei
abwechselnden Aufrufen mit verschiedenen Winkeln.

---

## §10 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Amp-Modul | `docs/project/Amp.md` |
| Core-Modul | `docs/project/Core.md` |
| Density-Modul | `docs/project/Density.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/ProPhysics_Dirac.c` |

---

**Ende Dirac.md v1.23.0.**