# ProPhysics — Konfiguration

**Datei:** `docs/project/CONFIG.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Zweck:** Referenz für alle Compile-Time-Konstanten in
`src/prophysics/header/ProPhysics_Config.h`.

---

## §0 — Wie dieses Dokument zu lesen ist

`ProPhysics_Config.h` ist **self-contained** und wird von jedem
ProPhysics-Header zuerst eingebunden. Alle Konstanten sind
**Compile-Time**. Nutzer können sie per `-D<NAME>=<WERT>` beim Build
überschreiben, sofern sie mit `#ifndef` geschützt sind.

**Dieses Dokument erklärt:**
- Was jede Konstante bedeutet
- Warum dieser Default gewählt ist
- Was passiert, wenn man sie ändert
- Welche Konstanten voneinander abhängen

**Überschreib-Regeln:**

| Zeichen | Bedeutung |
|---|---|
| ✅ | Mit `#ifndef` geschützt, per `-D` überschreibbar |
| ❌ | Abgeleitet, **nicht** überschreibbar (Compile-Fehler wenn versucht) |

**Sektionen entsprechen der Reihenfolge in `ProPhysics_Config.h`.**

---

## §1 — Topologie / Gittergröße

### §1.1 — `NODE_COUNT` ✅

| | |
|---|---|
| **Default** | `1048576ULL` (2²⁰) |
| **Bedeutung** | Standard-Gittergröße in Knoten |
| **Verwendung** | Wird von `ProPhysics_Initialize(&pu, NODE_COUNT)` als typischer Wert genutzt. Die Funktion akzeptiert aber jeden Wert > 0. |
| **Warum 2²⁰?** | Zweierpotenz. Passt zu Bit-Interleaved-Index-Berechnung (R1). 1 Mio Knoten ist ein guter Kompromiss für Tests: groß genug für aussagekräftige Simulationen, klein genug für 4–8 GB RAM. |
| **Änderung wirkt** | Nur auf Test-Harness und SDK-Runner. Der Kernel selbst verwendet immer `pu->total_nodes`. |

### §1.2 — `PRO_NODE_COUNT` ✅

| | |
|---|---|
| **Default** | `NODE_COUNT` |
| **Bedeutung** | Alias für `NODE_COUNT` |
| **Verwendung** | Legacy. Wird in neuem Code nicht mehr verwendet. |

### §1.3 — `MAX_NODES` ✅

| | |
|---|---|
| **Default** | `NODE_COUNT` |
| **Bedeutung** | Historische Obergrenze für statische Allokation |
| **Verwendung** | Legacy. Der Kernel allokiert dynamisch. |

### §1.4 — `MAX_SPARSE_TRACKING_NODES` ✅

| | |
|---|---|
| **Default** | `64000000ULL` |
| **Bedeutung** | Obergrenze für Sparse-Tracking-Strukturen |
| **Verwendung** | Aktuell ungenutzt. Für zukünftige Sparse-Erweiterungen. |

---

## §2 — Kanäle und Nachbarschaft

### §2.1 — `CHANNELS_MAX` ✅

| | |
|---|---|
| **Default** | `16` |
| **Bedeutung** | Kanäle pro Knoten |
| **Layout** | Siehe §2.3 |
| **Warum 16?** | Zweierpotenz, damit `PRO_CH_MASK` als Bit-Maske funktioniert. 6 Kanäle für 3D-Nachbarn (±x, ±y, ±z), 1 Kanal für Dephase, 1 für EPR, 8 Reserve. |
| **Änderung wirkt** | Auf `ProRegister`-Größe (128 B → `CHANNELS_MAX × 8`), `ProEdge`-Layout, Cache-Verhalten, und die Kanalschleifen in allen Hotpath-Funktionen. **Bricht Test-Regression** falls nicht alle Tests angepasst werden. |
| **Reduktions-Idee** | Etappe O1 (Cache-Optimierung) erwägt 16 → 8. Nicht implementiert. |

### §2.2 — `SYMMETRY_CHANNELS` ✅

| | |
|---|---|
| **Default** | `CHANNELS_MAX` |
| **Bedeutung** | Anzahl symmetrischer Kanäle |
| **Verwendung** | Legacy-Erbe. In aktuellen Modulen nicht referenziert. |

### §2.3 — Kanal-Layout

| Index | Konstante | Bedeutung |
|:-:|---|---|
| 0 | `PRO_NEIGHBOR_X_PLUS` | +x-Richtung |
| 1 | `PRO_NEIGHBOR_X_MINUS` | −x-Richtung |
| 2 | `PRO_NEIGHBOR_Y_PLUS` | +y-Richtung |
| 3 | `PRO_NEIGHBOR_Y_MINUS` | −y-Richtung |
| 4 | `PRO_NEIGHBOR_Z_PLUS` | +z-Richtung |
| 5 | `PRO_DEPHASE_CHANNEL` | Dephase-Kanal |
| 6 | `PRO_NEIGHBOR_Z_MINUS` | −z-Richtung |
| 7..14 | (Reserve) | — |
| 15 | `PRO_EPR_CHANNEL` | EPR-Kopplung |

### §2.4 — `PRO_EPR_CHANNEL` ❌

| | |
|---|---|
| **Wert** | `CHANNELS_MAX - 1` = `15` |
| **Bedeutung** | EPR-Kanal |
| **Abgeleitet** | Folgt aus `CHANNELS_MAX` |

### §2.5 — `PRO_DEPHASE_CHANNEL` ❌

| | |
|---|---|
| **Wert** | `5` |
| **Bedeutung** | Dephase-Kanal |
| **Warum 5?** | Historisch gewachsen. Liegt zwischen den räumlichen Kanälen (0–4, 6) und dem EPR-Kanal (15). |

### §2.6 — `PRO_PHYSICAL_CHANNELS` ❌

| | |
|---|---|
| **Wert** | `PRO_EPR_CHANNEL` = `15` |
| **Bedeutung** | Anzahl physikalischer (nicht-EPR) Kanäle |

### §2.7 — `PRO_CH_MASK` ❌

| | |
|---|---|
| **Wert** | `CHANNELS_MAX - 1u` = `15u` |
| **Bedeutung** | Bit-Maske für Kanal-Index |
| **Verwendung** | Ringpuffer-Index-Berechnung (R1). Erfordert, dass `CHANNELS_MAX` eine Zweierpotenz ist. |

### §2.8 — Nachbar-Konstanten (0, 1, 2, 3, 4, 6) ❌

| Konstante | Wert | Bedeutung |
|---|:-:|---|
| `PRO_NEIGHBOR_X_PLUS` | 0 | +x-Richtung |
| `PRO_NEIGHBOR_X_MINUS` | 1 | −x-Richtung |
| `PRO_NEIGHBOR_Y_PLUS` | 2 | +y-Richtung |
| `PRO_NEIGHBOR_Y_MINUS` | 3 | −y-Richtung |
| `PRO_NEIGHBOR_Z_PLUS` | 4 | +z-Richtung |
| `PRO_NEIGHBOR_Z_MINUS` | 6 | −z-Richtung |

**Wichtig:** Die Kanäle sind **nicht** numerisch sortiert. `+z` (4)
und `−z` (6) umklammern den Dephase-Kanal (5). Das ist Absicht —
historisch gewachsen, aber stabil.

---

## §3 — Amplituden-Basis

### §3.1 — `PRO_AMP_BASIS_SIZE` ✅

| | |
|---|---|
| **Default** | `8u` |
| **Bedeutung** | Dimension der Amplituden-Basis pro Gitterknoten |
| **Basis-Indizes** | 0 = NEUTRAL, 1..4 = Dirac/Spin-Komponenten, 5 = PHOTON, 6..7 = Reserve |
| **Warum 8?** | 2 Bitladungen × 2 Helizitäten × 2 (Vakuum, Photon) = 8. **Minimale Basis**, die alle beobachteten Phänomene trägt. |
| **Änderung wirkt** | Auf `ProAmpVector`-Größe (64 B → `PRO_AMP_BASIS_SIZE × 8`), alle Schleifen über Basis-Indizes, U5-Gewichtsvektor. **Bricht Test-Regression** massiv. |
| **Nicht empfohlen** | Diese Konstante ist fundamental. Änderung nur im Rahmen einer Major-Änderung mit vollständigem Test-Rewrite. |

---

## §4 — Tensor / Fock / Dichte

### §4.1 — `PRO_TENSOR_DIM` ✅

| | |
|---|---|
| **Default** | `64u` |
| **Bedeutung** | Dimension des Tensor-Zustands (8×8 Kronecker-Produkt zweier Knoten) |
| **Abhängigkeit** | Muss `8² = 64` sein (folgt aus 8-Basis) |
| **Änderung wirkt** | Auf `ProAmpTensorPair.coeff[]`. Muss synchron mit `PRO_AMP_BASIS_SIZE` geändert werden. |

### §4.2 — `PRO_TENSOR_RHO_DIM` ✅

| | |
|---|---|
| **Default** | `8u` |
| **Bedeutung** | Dimension der reduzierten Dichtematrix eines Tensor-Paars |
| **Abhängigkeit** | Muss `= PRO_AMP_BASIS_SIZE` sein |

### §4.3 — `PRO_TENSOR_MAX_PAIRS` ✅

| | |
|---|---|
| **Default** | `256u` |
| **Bedeutung** | Maximale Anzahl gleichzeitig aktiver Tensor-Paare |
| **Warum 256?** | Reicht für alle aktuellen Tests. Begrenzt den Speicher auf `256 × sizeof(ProAmpTensorPair)` ≈ 130 KB. |
| **Änderung wirkt** | Auf `ProUniverse.tensor_pair_capacity` und die Slot-Suche in `Tensor_Create_Pair` (O(N)). |

### §4.4 — `PRO_FOCK_MODES` ✅

| | |
|---|---|
| **Default** | `8u` |
| **Bedeutung** | Anzahl Fermion-Moden im Fock-Raum |
| **Warum 8?** | Passt exakt in ein `uint8_t` (Bitmaske = Besetzung). |
| **Änderung wirkt** | Auf `PRO_FOCK_DIM` (2^N), Jordan-Wigner-String-Länge, Popcount. |

### §4.5 — `PRO_FOCK_DIM` ✅

| | |
|---|---|
| **Default** | `256u` |
| **Bedeutung** | Fock-Basis-Dimension = 2^`PRO_FOCK_MODES` |
| **Abhängigkeit** | Muss `2^PRO_FOCK_MODES` sein |
| **Änderung wirkt** | Auf `ProFockState.coeff[]` (2 KB pro Zustand). |

### §4.6 — `PRO_FOCK_MAX_STATES` ✅

| | |
|---|---|
| **Default** | `64u` |
| **Bedeutung** | Maximale Anzahl gleichzeitig aktiver Fock-Zustände |
| **Änderung wirkt** | Auf `ProUniverse.fock_state_capacity`. |

### §4.7 — `PRO_DENSITY_DIM` ✅

| | |
|---|---|
| **Default** | `8u` |
| **Bedeutung** | Dimension der Knoten-Dichtematrix (8×8) |
| **Abhängigkeit** | Muss `= PRO_AMP_BASIS_SIZE` sein |

### §4.8 — `PRO_DENSITY_MAX` ✅

| | |
|---|---|
| **Default** | `64u` |
| **Bedeutung** | Maximale Anzahl gleichzeitig aktiver Knoten-Dichten |
| **Speicher** | `64 × 4 KB = 256 KB` |

### §4.9 — `PRO_TENSOR_DENSITY_DIM` ✅

| | |
|---|---|
| **Default** | `64u` |
| **Bedeutung** | Dimension der Tensor-Dichtematrix (64×64) |
| **Abhängigkeit** | Muss `= PRO_TENSOR_DIM` sein |
| **Speicher** | `64² × 8 B = 32 KB` pro Dichte |

### §4.10 — `PRO_TENSOR_DENSITY_MAX` ✅

| | |
|---|---|
| **Default** | `32u` |
| **Bedeutung** | Maximale Anzahl gleichzeitig aktiver Tensor-Dichten |
| **Speicher** | `32 × 32 KB = 1 MB` |

### §4.11 — `PRO_FOCK_DENSITY_DIM` ✅

| | |
|---|---|
| **Default** | `256u` |
| **Bedeutung** | Dimension der Fock-Dichtematrix (256×256) |
| **Abhängigkeit** | Muss `= PRO_FOCK_DIM` sein |
| **Speicher** | `256² × 8 B = 512 KB` pro Dichte |

### §4.12 — `PRO_FOCK_DENSITY_MAX` ✅

| | |
|---|---|
| **Default** | `8u` |
| **Bedeutung** | Maximale Anzahl gleichzeitig aktiver Fock-Dichten |
| **Speicher** | `8 × 512 KB = 4 MB` |

---

## §5 — Dirac-Struktur

### §5.1 — `PRO_DIRAC_DIM` ✅

| | |
|---|---|
| **Default** | `4u` |
| **Bedeutung** | Anzahl der Dirac-Spinor-Komponenten |
| **Warum 4?** | Standard: {L↑, L↓, R↑, R↓}. Nicht änderbar ohne komplette Dirac-Implementierung zu ersetzen. |

### §5.2 — Dirac-Komponenten-Indizes ❌

| Konstante | Wert | Bedeutung |
|---|:-:|---|
| `PRO_DIRAC_COMP_L_UP` | 0 | ψ_L↑ (linkshändig, Spin-up) |
| `PRO_DIRAC_COMP_L_DN` | 1 | ψ_L↓ |
| `PRO_DIRAC_COMP_R_UP` | 2 | ψ_R↑ (rechtshändig, Spin-up) |
| `PRO_DIRAC_COMP_R_DN` | 3 | ψ_R↓ |

### §5.3 — Mapping-Macros ❌

| Macro | Zweck |
|---|---|
| `PRO_DIRAC_TO_BASIS(ci)` | Komponenten-Index (0..3) → Basis-Index (1..4) |
| `PRO_DIRAC_FROM_BASIS(b)` | Basis-Index (1..4) → Komponenten-Index (0..3) |

**Warum der Offset 1?** Basis-Index 0 ist `UR_NEUTRAL`. Die Dirac-Komponenten belegen Basis 1..4.

---

## §6 — SU(2)-Eichfeld: Kinematik

### §6.1 — `PRO_SU2_SCALE` ✅

| | |
|---|---|
| **Default** | `1073741824` (2³⁰) |
| **Bedeutung** | Skala für Quaternion-Komponenten |
| **Warum 2³⁰ und nicht 2³¹?** | Bei Skala 2³¹ kann die Summe der vier Produktterme in `pro_su2_mul` bis `4 × 2⁶² = 2⁶⁴` gehen — **int64-Overflow**. Bei 2³⁰ bleibt der Zwischenwert ≤ 2⁶², sicher. |
| **Änderung wirkt** | Auf alle SU(2)-Arithmetik-Operationen und die Wilson-Loop-Normierung. |

### §6.2 — `PRO_SU2_NORM` ✅

| | |
|---|---|
| **Default** | `1152921504606846976LL` (2⁶⁰) |
| **Bedeutung** | Ziel-Norm eines SU(2)-Links: `|a|² + |b|² = 2⁶⁰` |
| **Abhängigkeit** | Muss `(PRO_SU2_SCALE)²` sein |

### §6.3 — `PRO_SU2_IDENT_RE` ✅

| | |
|---|---|
| **Default** | `1073741824` (2³⁰) |
| **Bedeutung** | Realteil der Identität: 1.0 in Skala 2³⁰ |
| **Verwendung** | Initialwert für alle Links in `ProPhysics_Initialize` |

### §6.4 — `PRO_SU2_IDENT_IM` ✅

| | |
|---|---|
| **Default** | `0` |
| **Bedeutung** | Imaginärteil der Identität |

---

## §6b — SU(2)-Dynamik

### §6b.1 — `PRO_SU2_YM_DEFAULT_Q15` ✅

| | |
|---|---|
| **Default** | `1000` |
| **Bedeutung** | Default-Kopplungskonstante g² in Q15 |
| **Physik** | 1000 ≈ 0.03 im dimensionslosen Gittermaß |
| **Nur wirksam bei** | `pu->su2_dynamics_active == 1` |
| **Änderung wirkt** | Auf Kraftstärke im Leapfrog. Größere Werte → stärkere Link-Rotation pro Tick. |
| **Stabilitätsgrenze** | Leapfrog bleibt stabil, solange `dt × g² × N ≪ 1`. Bei g² = 1000, dt = 1000 (Q15) ist der Test noch stabil (T16). |

### §6b.2 — `PRO_SU2_LEAPFROG_DT_Q15` ✅

| | |
|---|---|
| **Default** | `1000u` |
| **Bedeutung** | Zeitschritt dt pro Leapfrog-Aufruf, in Q15 |
| **Physik** | 1000 ≈ 0.03 |
| **Änderung wirkt** | Auf alle drei Schritte des Leapfrog (E-Update, Link-Update, E-Update). |
| **Stabilitätsgrenze** | dt × g² < ~0.5. Bei dt = 1000 (Q15) und g² = 1000 ist das Verhältnis ~0.0009 — deutlich stabil. Größere dt möglich, aber dann ist die Energie-Drift T16 empfindlicher. |

---

## §7 — Node-Bits

### §7.1 — `PRO_NODE_SPIN_FLIP_BIT` ❌

| | |
|---|---|
| **Wert** | `0x01u` (Bit 0) |
| **Bedeutung** | Markiert einen Knoten als spin-flipped |
| **Verwendung** | In `ProNode.reserved_gating`. Wird von `ProPhysics_Entangle_Nodes_Singlet` gesetzt. |

### §7.2 — `PRO_NODE_DIRAC_BIT` ❌

| | |
|---|---|
| **Wert** | `0x02u` (Bit 1) |
| **Bedeutung** | Markiert einen Knoten als Dirac-aktiv |
| **Verwendung** | In `ProNode.reserved_gating`. Wird vom Test-Setup gesetzt; `Apply_Dirac_Mass_Term` prüft dieses Bit. |

**Reserve:** Bits 2..7 sind frei für zukünftige Erweiterungen.

---

## §8 — Physik-Konstanten

### §8.1 — `PRO_2PI` ✅

| | |
|---|---|
| **Default** | `6.283185307179586476925286766559` |
| **Bedeutung** | 2π in voller double-Präzision |
| **Verwendung** | Winkel-Berechnungen in Transport, Gauge, Phase |

### §8.2 — `PRO_INV_2PI` ✅

| | |
|---|---|
| **Default** | `0.15915494309189533576888376337251` |
| **Bedeutung** | 1/(2π) |
| **Verwendung** | Phasen-Umrechnung Radian → Q16, `ProPhysics_Set_Edge_Phase` |

### §8.3 — `PRO_8_OVER_2PI` ✅

| | |
|---|---|
| **Default** | `1.2732395447351626861510701069801` |
| **Bedeutung** | 8/(2π) |
| **Verwendung** | Historisch. In aktuellem Code nicht mehr referenziert. |

### §8.4 — `PRO_2PI_INV_Q16` ✅

| | |
|---|---|
| **Default** | `(65536.0 / PRO_2PI)` |
| **Bedeutung** | Umrechnungsfaktor für Q16-Phasen |
| **Verwendung** | Debug, Diagnose |

### §8.5 — `PRO_INV_127` ✅

| | |
|---|---|
| **Default** | `0.00787401574803149606299212598425` |
| **Bedeutung** | 1/127 |
| **Verwendung** | Umrechnung `momentum_phase` (0..127) → `v/c` (0..1) in `Advance_Internal_Clocks` |

### §8.6 — `PRO_Q31_HALF_SQRT2` ✅

| | |
|---|---|
| **Default** | `1518500249` |
| **Bedeutung** | 1/√2 in Q31 (Re-Wert von `(1/√2, 0)`) |
| **Verwendung** | Hadamard-Gate, Superpositions-Aufbau |

---

## §9 — Physik-Defaults

### §9.1 — `PRO_DEFAULT_PHASE_STEP_Q15` ✅

| | |
|---|---|
| **Default** | `1000u` |
| **Bedeutung** | Default-Wert für `phase_step_q15` in `Apply_Amp_Step` |
| **Physik** | Der Wave-Step-Rotationswinkel pro Tick |
| **Änderung wirkt** | Auf globale Dynamik-Geschwindigkeit. Größer → schnellere Oszillation. |

### §9.2 — `PRO_DEFAULT_TRANSPORT_THETA_Q15` ✅

| | |
|---|---|
| **Default** | `500u` |
| **Bedeutung** | Default-Wert für `theta_q15` im Edge-Transport |
| **Physik** | Der Mischwinkel pro Transport-Schritt |
| **Änderung wirkt** | Auf Ausbreitungsgeschwindigkeit. Größer → schnellere ballistische Ausbreitung. |
| **Wichtig für Tests** | Tests mit fester Bloch-Vorhersage (Dispersion) hängen von diesem Wert ab. |

---

## §10 — Compiler-Attribute

### §10.1 — `PRO_INLINE` ✅

| | |
|---|---|
| **Default** | Compiler-spezifisch: MSVC `__forceinline`, GCC/Clang `inline __attribute__((always_inline))`, sonst `inline` |
| **Bedeutung** | Erzwingt Inlining für Hotpath-Helfer |
| **Änderung wirkt** | Auf Compiler-Optimierung. Überschreiben selten sinnvoll. |

---

## §11 — Legacy-Typedefs

### §11.1 — `Real` ❌

| | |
|---|---|
| **Typ** | `double` |
| **Bedeutung** | Historischer Alias |
| **Verwendung** | Legacy. In neuem Code nicht mehr verwendet. |

### §11.2 — `Index` ❌

| | |
|---|---|
| **Typ** | `uint64_t` |
| **Bedeutung** | Historischer Alias |
| **Verwendung** | Legacy. In neuem Code nicht mehr verwendet. |

---

## §12 — Cache- und Debug-Konfiguration

### §12.1 — `PRO_CACHE_LINE` ✅

| | |
|---|---|
| **Default** | `64u` |
| **Bedeutung** | Cache-Line-Größe in Bytes |
| **Verwendung** | Alignment für Hot-Arrays in `pro_aligned_calloc` |
| **Bedingung** | **Muss Zweierpotenz sein** (Compile-Fehler sonst) |
| **Warum 64?** | Standard-Cache-Line auf x86-64 und ARM64. |
| **Änderung wirkt** | Auf Speicherverbrauch (`+63 B pro Allokation`) und Cache-Verhalten. |

### §12.2 — `PRO_DEBUG_RING_SIZE` ✅

| | |
|---|---|
| **Default** | `256u` |
| **Bedeutung** | Ringpuffergröße für EPR-Debug-Events |
| **Verwendung** | `ProPhysics_Core.c` (Debug-Ring) |
| **Bedingung** | **Muss Zweierpotenz sein** (Compile-Fehler sonst) |
| **Warum 256?** | Reicht für Debug-Ausgaben der ersten Ticks. Größer → mehr Speicher, kaum Nutzen. |
| **Änderung wirkt** | Auf Debug-Ring-Kapazität. Ring läuft im Hotpath, aber nur bei `epr_debug >= 1`. |

---

## §13 — Überschreib-Tabelle (Quick Reference)

| Konstante | Überschreibbar | Abhängigkeiten | Empfehlung |
|---|:-:|---|---|
| `NODE_COUNT` | ✅ | keine | 2²⁰ bis 2²⁴ |
| `CHANNELS_MAX` | ✅ | Zweierpotenz; `PRO_CH_MASK` | 16 (nicht ändern) |
| `PRO_AMP_BASIS_SIZE` | ✅ | fundamental | 8 (nicht ändern) |
| `PRO_TENSOR_DIM` | ✅ | `= 8²` | 64 |
| `PRO_TENSOR_RHO_DIM` | ✅ | `= PRO_AMP_BASIS_SIZE` | 8 |
| `PRO_FOCK_MODES` | ✅ | ≤ 8 (uint8_t-Maske) | 8 |
| `PRO_FOCK_DIM` | ✅ | `= 2^PRO_FOCK_MODES` | 256 |
| `PRO_DIRAC_DIM` | ✅ | fundamental | 4 |
| `PRO_SU2_SCALE` | ✅ | int64-Overflow-Grenze | 2³⁰ |
| `PRO_SU2_NORM` | ✅ | `= PRO_SU2_SCALE²` | 2⁶⁰ |
| `PRO_SU2_YM_DEFAULT_Q15` | ✅ | Stabilitätsgrenze | 1000 |
| `PRO_SU2_LEAPFROG_DT_Q15` | ✅ | Stabilitätsgrenze | 1000 |
| `PRO_NODE_SPIN_FLIP_BIT` | ❌ | Bitfeld | 0x01 |
| `PRO_NODE_DIRAC_BIT` | ❌ | Bitfeld | 0x02 |
| `PRO_2PI`, `PRO_INV_2PI` | ✅ | mathematisch fix | nicht ändern |
| `PRO_Q31_HALF_SQRT2` | ✅ | mathematisch fix | 1518500249 |
| `PRO_DEFAULT_PHASE_STEP_Q15` | ✅ | Test-Regression | 1000 |
| `PRO_DEFAULT_TRANSPORT_THETA_Q15` | ✅ | Test-Regression (Dispersion) | 500 |
| `PRO_CACHE_LINE` | ✅ | Zweierpotenz | 64 |
| `PRO_DEBUG_RING_SIZE` | ✅ | Zweierpotenz | 256 |

---

## §14 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `error: PRO_CACHE_LINE muss Zweierpotenz sein` | `-DPRO_CACHE_LINE=48` o.ä. | Zweierpotenz wählen (32, 64, 128) |
| `error: PRO_DEBUG_RING_SIZE muss Zweierpotenz sein` | dito | dito |
| `PRO_AMP_BASIS_SIZE` geändert → Linker-Fehler | Strukturgrößen passen nicht mehr | Alle abhängigen Konstanten mit-ändern |
| Transport zu langsam / keine Ausbreitung | `PRO_DEFAULT_TRANSPORT_THETA_Q15` zu klein | Erhöhen (max. 32767) |
| Wave-Step oszilliert zu schnell | `PRO_DEFAULT_PHASE_STEP_Q15` zu groß | Verkleinern |
| SU(2)-Link-Drift zu groß | `PRO_SU2_YM_DEFAULT_Q15` zu groß | Verkleinern (z.B. 500) |
| `-DPRO_SU2_SCALE=2147483648` → Overflow in T15 | 2³¹ ist zu groß | Bei 2³⁰ bleiben |

---

## §15 — Siehe auch

| Thema | Datei |
|---|---|
| API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur | `docs/project/ARCHITECTURE.md` |
| Projekt-Regeln (R1–R7) | `docs/project/Project.md` §2 |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Changelog | `CHANGELOG.md` |

---

**Ende CONFIG.md v1.23.0.**