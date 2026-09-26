# ProPhysics — Modul: Density

**Datei:** `docs/project/Density.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_Density.c`
**Zweck:** Referenz für das Density-Modul: Dichte-Matrizen und
Lindblad-Operatoren auf drei Ebenen.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Density-Modul verwaltet **drei Dichte-Ebenen** unterschiedlicher
Dimension:

| Ebene | Struct | Dimension | Zweck |
|---|---|---|---|
| Knoten | `ProDensityMatrix` | 8×8 | Ein einzelner Gitterknoten |
| Tensor | `ProTensorDensity` | 64×64 | Zwei verschränkte Knoten |
| Fock | `ProFockDensity` | 256×256 | 8-Moden-Fermion-System |

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Die drei Dichte-Ebenen |
| §3 | Lindblad-Kanäle und ihre Semantik |
| §4 | Interne Helfer |
| §5 | Öffentliche API pro Ebene |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/CONFIG.md` — Konstanten (`PRO_DENSITY_DIM` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Density-Modul tut

Das Density-Modul implementiert **offene Quantensysteme** über
Dichte-Matrizen und Lindblad-Operatoren. Es arbeitet **nicht** auf
`amp_grid`, sondern auf separaten Dichte-Arrays.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **8×8-Knoten-Dichte** | `Density_Create` … `Density_Apply_Lindblad_Step` |
| **64×64-Tensor-Dichte** | `TensorDensity_Create` … `TensorDensity_Apply_Local_Lindblad` |
| **256×256-Fock-Dichte** | `FockDensity_Create` … `FockDensity_Partial_Trace_Modes` |
| **Auto-Sync** | `Density_Set_Auto_Sync`, `*_Sync_From_*`, `*_Sync_All_From_*` |

**Was das Modul nicht tut:**

- Keine unitäre Dynamik auf `amp_grid` (das macht `Amp`).
- Keine Fock-Raum-Erzeuger/Vernichter (das macht `Fock`).
- Keine Tensor-Paare (das macht `Tensor`).
- Kein Sampling (das macht der Test-Harness).

---

## §2 — Die drei Dichte-Ebenen

### §2.1 — Ebene 1: `ProDensityMatrix` (8×8)

**Dimension:** 8×8 (64 komplexe Werte = 512 B pro Dichte).

**Anwendung:** Ein einzelner Gitterknoten. Wird direkt mit einem
`ProAmpVector` initialisiert (`Density_From_Pure`) oder als Gemisch
(`Density_From_Mixture`).

**Speicherort:** `pu->density_matrices` (Array von `PRO_DENSITY_MAX`
Slots, default 64).

**Kein Binding** an einen Knoten — der Slot ist frei adressierbar.
Damit ist Ebene 1 **nicht** Teil von `auto_sync_density`.

### §2.2 — Ebene 2: `ProTensorDensity` (64×64)

**Dimension:** 64×64 (4096 komplexe Werte = 32 KB pro Dichte).

**Anwendung:** Ein verschränktes 2-Knoten-System. Wird aus einem
`ProAmpTensorPair` befüllt (`TensorDensity_From_Pair`).

**Speicherort:** `pu->tensor_densities` (Array von
`PRO_TENSOR_DENSITY_MAX` Slots, default 32).

**Binding:** Hat ein `pair_id`-Feld, das auf einen Slot in
`pu->tensor_pairs` zeigt. Wird von `auto_sync_density` genutzt.

### §2.3 — Ebene 3: `ProFockDensity` (256×256)

**Dimension:** 256×256 (65536 komplexe Werte = 512 KB pro Dichte).

**Anwendung:** Ein 8-Moden-Fermion-System. Wird aus einem `ProFockState`
befüllt (`FockDensity_From_Fock`).

**Speicherort:** `pu->fock_densities` (Array von
`PRO_FOCK_DENSITY_MAX` Slots, default 8).

**Binding:** Hat ein `fock_id`-Feld, das auf einen Slot in
`pu->fock_states` zeigt. Wird von `auto_sync_density` genutzt.

### §2.4 — Konventionen

**Alle Ebenen:**

- Q31-Repräsentation: `Tr(rho_stored) = 2^31` entspricht physikalisch 1.0.
- Diagonale startet bei `rho[0][0] = INT32_MAX` (reiner Grundzustand).

**Speicherlayout:**

- Zeilen-major: `rho[i * N + j]` für N×N-Matrix.
- `N` ist 8, 64 oder 256 je nach Ebene.

---

## §3 — Lindblad-Kanäle und ihre Semantik

Das Modul implementiert **drei Lindblad-Kanäle** aus
`ProLindbladKind`:

| Kind | Physik | Wirkt auf |
|---|---|---|
| `PRO_LINDBLAD_AMP_DAMP` | Spontane Emission | Population + Off-Diagonalen |
| `PRO_LINDBLAD_PHASE_DAMP` | Dephasierung | Nur Off-Diagonalen |
| `PRO_LINDBLAD_DEPOLARIZE` | Depolarisierung | Gesamte 2×2-Matrix |

### §3.1 — AMP_DAMP (Amplitude Damping)

**Physik:** Ein angeregter Zustand zerfällt in den Grundzustand.
Standardbeispiel: Atom im angeregten Zustand emittiert ein Photon.

**Formel (für 2-Level-System mit `decay_from → decay_to`):**

```
dρ[to][to]   = +γ · ρ[from][from]
dρ[from][from] = -γ · ρ[from][from]
dρ[0][1]     = -0.5·γ · ρ[0][1]
dρ[1][0]     = -0.5·γ · ρ[1][0]
```

**Konventionen der beiden Aufrufer:**

| Aufrufer | Index 0 | Index 1 | `decay_from` | `decay_to` |
|---|---|---|---|---|
| `TensorDensity` | `iu = UR_POSITRON_CW` (angeregt) | `id = UR_NEGATRON_CCW` (Grund) | 0 | 1 |
| `FockDensity` | Mode leer (Grund) | Mode besetzt (angeregt) | 1 | 0 |

**Wichtig:** Die physikalische Richtung ist in beiden Fällen
**„angeregt zerfällt zu Grund"**. Nur die Index-Konvention
unterscheidet sich.

### §3.2 — PHASE_DAMP (Dephasierung)

**Physik:** Die Off-Diagonalen zerfallen exponentiell. Populationen
bleiben unverändert. Standardbeispiel: elastische Stöße.

**Formel:**

```
dρ[0][1] = -γ · ρ[0][1]
dρ[1][0] = -γ · ρ[1][0]
```

**Symmetrisch** in beiden Niveaus. `decay_from` / `decay_to` werden
ignoriert.

### §3.3 — DEPOLARIZE (Depolarisierung)

**Physik:** Das System nähert sich dem maximal gemischten Zustand
`I/2` innerhalb des 2-Level-Blocks. Standardbeispiel: allgemeiner
Quantenkanal mit isotroper Rauschquelle.

**Formel:**

```
tr = ρ[0][0] + ρ[1][1]
dρ[0][0] = (2γ/3)·tr - (4γ/3)·ρ[0][0]
dρ[1][1] = (2γ/3)·tr - (4γ/3)·ρ[1][1]
dρ[0][1] = -(4γ/3)·ρ[0][1]
dρ[1][0] = -(4γ/3)·ρ[1][0]
```

**Symmetrisch** in beiden Niveaus. `decay_from` / `decay_to` werden
ignoriert.

### §3.4 — Wirkung der 8×8-Knoten-Dichte

Die 8×8-Knoten-Dichte hat eine **spezielle Konvention**: Der Kanal
wirkt nur auf den 2-dimensionalen Unterraum
`{UR_POSITRON_CW, UR_NEGATRON_CCW}` = Indizes `{1, 4}`.

**Nicht betroffen:** Alle anderen 6 Basis-Zustände.

**Beispiel AMP_DAMP auf Knoten:**

- Population von `UR_POSITRON_CW` (Index 1) zerfällt.
- Population von `UR_NEGATRON_CCW` (Index 4) wächst entsprechend.
- Alle Off-Diagonalen zwischen `{1, 4}` und anderen Zuständen werden gedämpft.

---

## §4 — Interne Helfer

### §4.1 — `pro_density_find_free_slot`

```c
static int32_t pro_density_find_free_slot(
    const void* arr, size_t stride_bytes, uint32_t cap);
```

Sucht den ersten Slot mit `active == 0` in einem Array, dessen erstes
Member ein `uint32_t active`-Flag ist.

**Parameter:**

| Parameter | Bedeutung |
|---|---|
| `arr` | Zeiger auf das Array (`pu->density_matrices` etc.) |
| `stride_bytes` | `sizeof(struct)` des Array-Elements |
| `cap` | Anzahl Slots im Array |

**Rückgabe:** Slot-Index in `[0, cap)` oder `-1` wenn alle belegt.

**Ersetzt** die lineare Suche in allen drei `*_Create`-Funktionen.

### §4.2 — `pro_lindblad_2x2`

```c
static void pro_lindblad_2x2(
    const double M_re[2][2], const double M_im[2][2],
    double dM_re[2][2], double dM_im[2][2],
    uint32_t kind, double gamma,
    int decay_from, int decay_to);
```

2×2-Lindblad-Kern auf einem 2-Level-Block.

**Parameter:**

| Parameter | Bedeutung |
|---|---|
| `M_re`, `M_im` | Input 2×2-Matrix (Real- und Imaginärteil) |
| `dM_re`, `dM_im` | Output Delta (muss vom Aufrufer genullt werden) |
| `kind` | `PRO_LINDBLAD_*` |
| `gamma` | Kanalstärke |
| `decay_from` | Index (0 oder 1) des zerfallenden Zustands (nur AMP_DAMP) |
| `decay_to` | Index (0 oder 1) des aufnehmenden Zustands (nur AMP_DAMP) |

**Ersetzt** zwei duplizierte Funktionen (`pro_td_apply_2x2_lindblad`,
`pro_fock_2x2_kern`).

### §4.3 — `pro_td_re` / `pro_td_im` / `pro_td_set`

```c
static inline double pro_td_re(const ProTensorDensity* td, uint32_t i, uint32_t j);
static inline double pro_td_im(const ProTensorDensity* td, uint32_t i, uint32_t j);
static inline void pro_td_set(ProTensorDensity* td, uint32_t i, uint32_t j,
    double re, double im);
```

Q31-Zugriff auf die 64×64-Tensor-Dichte mit Stride 64.

### §4.4 — `pro_fd_re` / `pro_fd_im` / `pro_fd_set`

```c
static inline double pro_fd_re(const ProFockDensity* fd, uint32_t i, uint32_t j);
static inline double pro_fd_im(const ProFockDensity* fd, uint32_t i, uint32_t j);
static inline void pro_fd_set(ProFockDensity* fd, uint32_t i, uint32_t j,
    double re, double im);
```

Q31-Zugriff auf die 256×256-Fock-Dichte mit Stride 256.

### §4.5 — Datei-lokale Scratch-Puffer

```c
static double g_td_r_re[64][64], g_td_r_im[64][64];
static double g_td_d_re[64][64], g_td_d_im[64][64];
```

Vier 64×64-`double`-Puffer (32 KB je Puffer, 128 KB total) für
`TensorDensity_Apply_Local_Lindblad`. Single-threaded.

**Zweck:** Vermeidung von `malloc` im 64×64-Lindblad-Pfad.
Aliasing strukturell ausgeschlossen (separate r- und d-Puffer).

---

## §5 — Öffentliche API

### §5.1 — 8×8-Knoten-Dichte

| Funktion | Zweck |
|---|---|
| `Density_Create` | Reserviert Slot, initialisiert ρ = \|0⟩⟨0\| |
| `Density_Destroy` | Gibt Slot frei |
| `Density_Count` | Anzahl aktiver Dichten |
| `Density_From_Pure` | Setzt ρ = \|ψ⟩⟨ψ\| aus `ProAmpVector` |
| `Density_From_Mixture` | Setzt ρ = Σ w_i·\|ψ_i⟩⟨ψ_i\| |
| `Density_Get_Element` / `Set_Element` | Einzelner Eintrag |
| `Density_Trace` | Tr(ρ) |
| `Density_Purity` | Tr(ρ²) |
| `Density_Von_Neumann_Entropy` | −Tr(ρ ln ρ) |
| `Density_Offdiag_Magnitude` | Σ_{i≠j} \|ρ_ij\|² |
| `Density_Apply_Unitary` | ρ ← U·ρ·U† |
| `Density_Apply_Lindblad_Step` | Ein Lindblad-Schritt |

### §5.2 — 64×64-Tensor-Dichte

| Funktion | Zweck |
|---|---|
| `TensorDensity_Create` | Reserviert Slot mit `pair_id`-Binding |
| `TensorDensity_Destroy` | Gibt Slot frei |
| `TensorDensity_Count` | Anzahl aktiver Dichten |
| `TensorDensity_From_Pair` | ρ = \|ψ⟩⟨ψ\| aus `ProAmpTensorPair` |
| `TensorDensity_Get_Element` | Einzelner Eintrag |
| `TensorDensity_Trace` | Tr(ρ) |
| `TensorDensity_Purity` | Tr(ρ²) |
| `TensorDensity_Partial_Trace_A` | ρ_A = Tr_B(ρ) in 8×8-Knoten-Dichte |
| `TensorDensity_Partial_Trace_B` | ρ_B = Tr_A(ρ) in 8×8-Knoten-Dichte |
| `TensorDensity_Apply_Local_Lindblad` | Lindblad auf einer Seite (A oder B) |

### §5.3 — 256×256-Fock-Dichte

| Funktion | Zweck |
|---|---|
| `FockDensity_Create` | Reserviert Slot mit `fock_id`-Binding |
| `FockDensity_Destroy` | Gibt Slot frei |
| `FockDensity_Count` | Anzahl aktiver Dichten |
| `FockDensity_From_Fock` | ρ = \|ψ⟩⟨ψ\| aus `ProFockState` |
| `FockDensity_Get_Element` / `Set_Element` | Einzelner Eintrag |
| `FockDensity_Trace` | Tr(ρ) |
| `FockDensity_Purity` | Tr(ρ²) |
| `FockDensity_Mode_Occupation` | ⟨n_m⟩ für einen Mode |
| `FockDensity_Apply_Mode_Lindblad` | Lindblad auf einen Mode |
| `FockDensity_Partial_Trace_Modes` | ρ_A = Tr_B(ρ) über n_keep Moden |

### §5.4 — Auto-Sync

```c
PROPHYSICS_API void ProPhysics_Density_Set_Auto_Sync(ProUniverse* pu, int enable);
```

Aktiviert/deaktiviert Auto-Sync für **beide** Bindings (Tensor + Fock).

Wenn aktiv (`pu->auto_sync_density == 1`), ruft `Apply_Amp_Step` nach
jedem Tick:

- `ProPhysics_TensorDensity_Sync_All_From_Pairs(pu)`
- `ProPhysics_FockDensity_Sync_All_From_Fock(pu)`

**Wichtig:** `ProDensityMatrix` (8×8) ist **nicht** Teil von Auto-Sync.
Diese Ebene hat kein Binding an einen Knoten.

---

## §6 — Verwendungsmuster

### §6.1 — Reine Knoten-Dichte

```c
uint32_t rho_id;
ProPhysics_Density_Create(&pu, &rho_id);

ProAmpVector psi = {0};
psi.coeff[UR_POSITRON_CW]  = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);
psi.coeff[UR_NEGATRON_CCW] = pro_amp_pack(PRO_Q31_HALF_SQRT2, 0);

ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

const double tr = ProPhysics_Density_Trace(&pu, rho_id);
const double purity = ProPhysics_Density_Purity(&pu, rho_id);
/* tr ≈ 1.0, purity ≈ 1.0 */
```

### §6.2 — Gemisch initialisieren

```c
ProAmpVector states[2] = {
    pure_basis(UR_POSITRON_CW),
    pure_basis(UR_NEGATRON_CCW),
};
const double weights[2] = { 0.5, 0.5 };

ProPhysics_Density_From_Mixture(&pu, rho_id, states, weights, 2);
/* Tr(ρ) = 1, Purity = 0.5, S = ln 2 */
```

### §6.3 — Lindblad-Dephasierung

```c
for (int t = 0; t < 100; ++t) {
    ProPhysics_Density_Apply_Lindblad_Step(
        &pu, rho_id, PRO_LINDBLAD_PHASE_DAMP, 0.02);
}
/* Off-Diagonalen ≈ 0, Diagonale erhalten */
```

### §6.4 — Tensor-Dichte + Partial Trace

```c
uint32_t pid = 0;
ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

uint32_t rho_t_id = 0;
ProPhysics_TensorDensity_Create(&pu, pid, &rho_t_id);
ProPhysics_TensorDensity_From_Pair(&pu, rho_t_id, pid);

uint32_t rho_a_id = 0;
ProPhysics_Density_Create(&pu, &rho_a_id);
ProPhysics_TensorDensity_Partial_Trace_A(&pu, rho_t_id, rho_a_id);

const double S_A = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_a_id);
/* S_A = ln 2 (Bell-Zustand) */
```

### §6.5 — Lindblad auf Tensor-Paar

```c
/* Depolarisierung der A-Seite über 100 Schritte */
for (int t = 0; t < 100; ++t) {
    ProPhysics_TensorDensity_Apply_Local_Lindblad(
        &pu, rho_t_id, 0 /* side A */,
        PRO_LINDBLAD_DEPOLARIZE, 0.1);
}
/* Kohärenz von ρ_full verschwindet, aber ρ_A bleibt diag(0.5,0.5) */
```

### §6.6 — Fock-Dichte mit Mode-Lindblad

```c
uint64_t fid;
ProPhysics_Fock_Create(&pu, &fid);
ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);  /* |1⟩ */

uint32_t rho_f_id;
ProPhysics_FockDensity_Create(&pu, fid, &rho_f_id);
ProPhysics_FockDensity_From_Fock(&pu, rho_f_id, fid);

/* Mode 0 auf 100 Ticks mit Amp-Damp */
for (int t = 0; t < 100; ++t) {
    ProPhysics_FockDensity_Apply_Mode_Lindblad(
        &pu, rho_f_id, 0,
        PRO_LINDBLAD_AMP_DAMP, 0.02);
}

const double n0 = ProPhysics_FockDensity_Mode_Occupation(
    &pu, rho_f_id, 0);
/* n0 ≈ exp(-2.0) ≈ 0.135 */
```

### §6.7 — Auto-Sync aktivieren

```c
ProPhysics_Density_Set_Auto_Sync(&pu, 1);

/* Jeder Tick synchronisiert Tensor- und Fock-Dichten aus ihren
 * Grundzuständen. */
for (int t = 0; t < 10; ++t) {
    ProPhysics_Tick(&pu, NULL);
}
```

---

## §7 — Fallstricke

### §7.1 — `From_Pure` mit Null-Vektor

Wenn `Σ_b |c_b|² == 0`, gibt `From_Pure` **`false`** zurück. Die
Dichte bleibt unverändert.

### §7.2 — `From_Mixture` mit Null-Gewichtssumme

Wenn `Σ w_i <= 0`, gibt `From_Mixture` **`false`** zurück.

### §7.3 — Purity-Normierung

`Purity = Σ|ρ_ij|² / 2^62`. Bei einem reinen Zustand (`ρ = |ψ⟩⟨ψ|`)
ist `Purity = 1.0`, unabhängig von der Dimension.

### §7.4 — `Apply_Local_Lindblad` mit `side`

- `side = 0` → wirkt auf `A`-Freiheitsgrad (erste 8 Basis-Zustände)
- `side = 1` → wirkt auf `B`-Freiheitsgrad (zweite 8 Basis-Zustände)

**Falscher Wert:** Rückgabe `false`.

### §7.5 — `Apply_Mode_Lindblad` mit `mode >= 8`

Rückgabe `false`. Der Fock-Raum hat nur 8 Moden.

### §7.6 — Speicher-Allokation in `FockDensity_Apply_Mode_Lindblad`

Diese Funktion ruft **`malloc`** (2 MB Scratch). Das ist eine
**dokumentierte Ausnahme** von R2 („kein malloc im Hotpath"):

- Sie läuft **nicht im Tick**, sondern im Test-Harness.
- Das 64×64-Pendant nutzt file-lokale Puffer statt malloc.
- Das 256×256-Pendant würde 2 MB Stack sprengen.

### §7.7 — Scratch-Puffer sind global

Die vier `g_td_*`-Puffer (128 KB) sind **file-lokale Globals**.
Bei parallelen Aufrufen auf mehreren `ProUniverse`-Instanzen kommt
es zu Race Conditions.

**Konsequenz:** Single-threaded pro Prozess. Für Multi-Threading
müsste der Scratch in `pu` wandern.

### §7.8 — `Sync_All_From_*` mit ungültigem Binding

Wenn ein Slot aktiv ist, aber `pair_id` / `fock_id` **out-of-range**
oder **UINT64_MAX**, wird der Slot **übersprungen** (Rückgabe
`true` am Ende).

### §7.9 — Reihenfolge `TensorDensity_Apply_Local_Lindblad`

Der Aufruf mit `side = 0` und `side = 1` hintereinander ergibt
**nicht** dasselbe wie ein einzelner Aufruf mit beiden Seiten
zusammen. Die Kanäle werden sequenziell angewendet:

```c
/* Sequenziell korrekt: */
TensorDensity_Apply_Local_Lindblad(&pu, rho_id, 0, DEPOLARIZE, 0.1);
TensorDensity_Apply_Local_Lindblad(&pu, rho_id, 1, DEPOLARIZE, 0.1);
```

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Amp-Modul | `docs/project/Amp.md` |
| Core-Modul | `docs/project/Core.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/ProPhysics_Density.c` |

---

**Ende Density.md v1.23.0.**