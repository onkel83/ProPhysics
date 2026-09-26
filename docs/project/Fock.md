# ProPhysics — Modul: Fock

**Datei:** `docs/project/Fock.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_Fock.c`
**Zweck:** Referenz für das Fock-Modul: 8-Moden-Fermion-Raum in Q31
mit Jordan-Wigner-Transformation.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Fock-Modul implementiert den **8-Moden-Fock-Raum** als 256-dim
komplexen Vektor in Q31. Es stellt die fermionischen Erzeuger,
Vernichter und Hopping-Operatoren mit korrekter Jordan-Wigner-Signatur
bereit.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Fock-Basis und Konventionen |
| §3 | Jordan-Wigner-Signatur |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Tensor.md` — Tensor-Fock-Adapter
- `docs/project/Density.md` — Fock-Dichte-Matrizen
- `docs/project/CONFIG.md` — Konstanten (`PRO_FOCK_MODES` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das Fock-Modul behandelt den **Fermion-Sektor** des Kernels. Es
arbeitet auf einem **eigenen Zustandsraum** (256-dim komplexer Vektor)
und **nicht** auf `amp_grid`.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Popcount** | `Fock_Popcount` (SWAR, 8 Bit) |
| **Lifecycle** | `Fock_Create`, `Fock_Destroy`, `Fock_Set_Basis`, `Fock_Count` |
| **Amplitude** | `Fock_Get_Amplitude`, `Fock_Set_Amplitude` |
| **Metriken** | `Fock_Norm`, `Fock_Particle_Number`, `Fock_Is_Zero` |
| **Hilfsoperationen** | `Fock_Clone`, `Fock_Compare`, `Fock_Scale` |
| **Erzeuger/Vernichter** | `Fock_Apply_Create`, `Fock_Apply_Annihilate`, `Fock_Apply_Create_Plus_Annihilate` |
| **Antikommutatoren** | `Fock_Apply_Anticomm_CD`, `Fock_Apply_Anticomm_CC`, `Fock_Apply_Anticomm_DD` |
| **Hopping** | `Fock_Apply_Hopping` |

**Was das Modul nicht tut:**

- Keine Zeitentwicklung (kein Tick, kein Unitär-Step).
- Keine Dichte-Matrizen (die macht `Density`).
- Keine Kopplung an `amp_grid` (die macht `Tensor` via Adapter).
- Kein Sampling.

**Aktivierung:** Keine. Das Modul wird **explizit** vom Nutzer
aufgerufen, nicht vom Kernel-Tick.

---

## §2 — Fock-Basis und Konventionen

### §2.1 — Basis

Die Basis ist **256-dimensional** (`PRO_FOCK_DIM = 256`). Jeder
Basiszustand entspricht einer **Besetzungsmaske** von 8 Moden:

```
Index 0..255 -> Bitmuster
Bit i = 1     -> Mode i besetzt
```

**Beispiele:**

| Index | Binär | Besetzte Moden |
|:-:|:-:|---|
| 0 | `00000000` | Vakuum |
| 1 | `00000001` | Mode 0 |
| 3 | `00000011` | Moden 0, 1 |
| 255 | `11111111` | Alle 8 Moden |

### §2.2 — Normierung

Die Summe `Σ |coeff[i]|²` wird in Q62 gehalten: `2^62` entspricht
physikalisch 1.0.

**Konsequenz:** `Fock_Norm` liefert einen Wert in `[0, 1]`.

### §2.3 — Speicherort

Fock-Zustände werden in `pu->fock_states` gehalten
(`ProFockState`-Array, Kapazität `PRO_FOCK_MAX_STATES`, default 64).

Jeder Slot hat ein `active`-Flag. Inaktive Slots sind frei für
Neuzuweisung.

### §2.4 — `n_particles`-Feld

`n_particles` wird **nicht** automatisch aktualisiert. `Set_Basis`
setzt es aus dem Popcount. Nach `Apply_Create`/`Apply_Annihilate`
ist `n_particles` **stale**.

**Empfehlung:** `Fock_Particle_Number` für den tatsächlichen
Erwartungswert nutzen, nicht `n_particles`.

---

## §3 — Jordan-Wigner-Signatur

### §3.1 — Formel

Fermionische Erzeuger/Vernichter benötigen einen **Vorzeichen-String**,
um die Antikommutations-Relationen zu erfüllen:

```
c+_i |n_0...n_7> = (-1)^{Σ_{j<i} n_j} (1-n_i) |...n_i=1...>
c_i  |n_0...n_7> = (-1)^{Σ_{j<i} n_j}    n_i  |...n_i=0...>
```

**Der Vorzeichen-String** ist `(-1)^{Σ_{j<i} n_j}` — abhängig von der
Besetzung aller Moden **strikt unter** i.

### §3.2 — Implementierung

Der Zähler wird über `ProPhysics_Fock_Popcount` berechnet:

```c
const uint8_t below_mask = (uint8_t)(mask - 1u);
const uint32_t below = ProPhysics_Fock_Popcount((uint8_t)(bits & below_mask));
```

Bei `below & 1 == 1` wird der Q31-Wert negiert.

### §3.3 — Warum diese Konvention?

Die Jordan-Wigner-Transformation ist die **Standard-Darstellung**
fermionischer Operatoren als Spin-Operatoren. Sie ist in der
Kern-Architektur als signed-permutation (Vorzeichen × Indextausch)
kodiert.

**Vorteil:** Keine Matrixmultiplikation. Nur Vorzeichen + Bitoperationen.

---

## §4 — Interne Helfer

### §4.1 — `pro_fock_apply_jw_sign`

```c
static inline ProAmpQ31 pro_fock_apply_jw_sign(
    ProAmpQ31 v, uint32_t below_count);
```

Wendet das Jordan-Wigner-Vorzeichen an:
`v' = (-1)^{below_count} · v`.

**Ersetzt** 3× wiederholten 4-Zeilen-Block in `Create`, `Annihilate`
und `Create_Plus_Annihilate`.

### §4.2 — `ProFockOpKind` (enum)

```c
typedef enum {
    PRO_FOCK_OP_CREATE     = 1,
    PRO_FOCK_OP_ANNIHILATE = 2
} ProFockOpKind;
```

Art einer fermionischen Operation.

### §4.3 — `pro_fock_apply_op`

```c
static void pro_fock_apply_op(ProUniverse* pu, uint64_t fock_id,
    ProFockOpKind kind, uint8_t mode);
```

Dünner Dispatcher für `Fock_Apply_Create` / `Fock_Apply_Annihilate`.

### §4.4 — `pro_fock_anticomm_impl`

```c
static bool pro_fock_anticomm_impl(
    ProUniverse* pu, uint64_t fock_id,
    uint8_t i, uint8_t j,
    ProFockOpKind a1, ProFockOpKind a2,
    ProFockOpKind b1, ProFockOpKind b2);
```

Generisches Antikommutator-Muster:

```
A = op(a2, i) · op(a1, j)
B = op(b2, j) · op(b1, i)
Ergebnis: A·psi + B·psi
```

**Ersetzt** 3× 30-Zeilen-Block in `CD`, `CC`, `DD`.

---

## §5 — Öffentliche API

### §5.1 — Popcount

```c
PROPHYSICS_API uint32_t ProPhysics_Fock_Popcount(uint8_t bits);
```

**SWAR-Popcount** auf 8 Bit. Liefert die Anzahl gesetzter Bits in
`[0, 8]`.

**Beispiel:**
```c
ProPhysics_Fock_Popcount(0x03) == 2;
ProPhysics_Fock_Popcount(0xFF) == 8;
```

### §5.2 — Lifecycle

| Funktion | Zweck |
|---|---|
| `Fock_Create(pu, &id)` | Reserviert Slot, initialisiert als Vakuum |
| `Fock_Destroy(pu, id)` | Gibt Slot frei |
| `Fock_Set_Basis(pu, id, bits)` | Setzt Zustand auf Basiszustand `bits` |
| `Fock_Count(pu)` | Anzahl aktiver Zustände |

### §5.3 — Amplitude

| Funktion | Zweck |
|---|---|
| `Fock_Get_Amplitude(pu, id, bits, &re, &im)` | Liest Koeffizient `bits` |
| `Fock_Set_Amplitude(pu, id, bits, re, im)` | Schreibt Koeffizient `bits` |

**Parameter `bits`:** 0..255 (Besetzungsmaske).

### §5.4 — Metriken

| Funktion | Zweck |
|---|---|
| `Fock_Norm(pu, id)` | `Σ \|c_i\|² / 2^62` |
| `Fock_Particle_Number(pu, id)` | `Σ \|c_i\|² · popcount(i) / 2^62` |
| `Fock_Is_Zero(pu, id)` | Alle Koeffizienten 0? |

### §5.5 — Hilfsoperationen

| Funktion | Zweck |
|---|---|
| `Fock_Clone(pu, src, &dst)` | Kopie eines Zustands |
| `Fock_Compare(pu, a, b)` | Maximaler Koeffizienten-Differenzbetrag |
| `Fock_Scale(pu, id, scale_q31)` | Skaliert alle Koeffizienten |

### §5.6 — Erzeuger / Vernichter

| Funktion | Physik |
|---|---|
| `Fock_Apply_Create(pu, id, mode)` | `c†_mode` |
| `Fock_Apply_Annihilate(pu, id, mode)` | `c_mode` |
| `Fock_Apply_Create_Plus_Annihilate(pu, id, mode)` | `(c†_mode + c_mode)` |

**Rückgabe:** `false`, wenn das Ergebnis exakt Null ist (Pauli-Blockade).

**`mode`:** 0..7 (Moden-Index).

**Jordan-Wigner-Signatur wird automatisch angewendet.**

### §5.7 — Antikommutatoren

| Funktion | Antikommutator |
|---|---|
| `Fock_Apply_Anticomm_CD(pu, id, i, j)` | `{c_i, c†_j}` |
| `Fock_Apply_Anticomm_CC(pu, id, i, j)` | `{c_i, c_j}` |
| `Fock_Apply_Anticomm_DD(pu, id, i, j)` | `{c†_i, c†_j}` |

**Semantik:** Wendet `AB + BA` auf den Zustand an.

**Rein diagnostisch.** Für die kanonischen Werte:
- `{c_i, c†_j} = δ_ij` → Ergebnis ist δ_ij · ψ.
- `{c_i, c_j} = 0` → Ergebnis ist 0.
- `{c†_i, c†_j} = 0` → Ergebnis ist 0.

### §5.8 — Hopping

```c
PROPHYSICS_API bool ProPhysics_Fock_Apply_Hopping(
    ProUniverse* pu, uint64_t fock_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
```

**Physik:** `U(θ) = exp(-i·θ·H)` mit `H = c†_i·c_j + c†_j·c_i`.

**Wirkung:** 2×2-Rotation in jedem Unterraum `{|S>, |S'>}`, wobei
`|S'>` = `|S>` mit i↔j getauscht.

**Jordan-Wigner-Zwischenstring** liefert das Vorzeichen.

**Erhält Norm und Teilchenzahl exakt.**

---

## §6 — Verwendungsmuster

### §6.1 — Vakuum anlegen und Besetzung setzen

```c
uint64_t fid = 0;
ProPhysics_Fock_Create(&pu, &fid);

ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* |1> */

const double n = ProPhysics_Fock_Particle_Number(&pu, fid);
/* n = 1.0 */
```

### §6.2 — Erzeuger und Vernichter

```c
/* c†_1 |vac> = -|1,2> (JW-Vorzeichen) */
ProPhysics_Fock_Set_Basis(&pu, fid, 0x00u);
ProPhysics_Fock_Apply_Create(&pu, fid, 0u);   /* c†_0 */
ProPhysics_Fock_Apply_Create(&pu, fid, 1u);   /* c†_1 */

int32_t re = 0, im = 0;
ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x03u, &re, &im);
/* re = -INT32_MAX, im = 0 */
```

### §6.3 — Jordan-Wigner-Signatur prüfen

```c
ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* |1> */
ProPhysics_Fock_Apply_Create(&pu, fid, 1u);   /* c†_1 |1> = -|1,2> */

int32_t re = 0, im = 0;
ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x03u, &re, &im);
/* re = -INT32_MAX */
```

### §6.4 — Pauli-Blockade

```c
ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* Mode 0 besetzt */
const bool ok = ProPhysics_Fock_Apply_Create(&pu, fid, 0u);
/* ok = false, c†_0 |1> ist nicht erlaubt */

const bool is_zero = ProPhysics_Fock_Is_Zero(&pu, fid);
/* is_zero = true */
```

### §6.5 — Hopping

```c
ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* |1> */
ProPhysics_Fock_Apply_Hopping(&pu, fid, 0u, 1u,
    (int32_t)lround((M_PI / 4.0) * 32768.0));

const double norm = ProPhysics_Fock_Norm(&pu, fid);
const double n = ProPhysics_Fock_Particle_Number(&pu, fid);
/* norm = 1.0, n = 1.0 */
```

### §6.6 — Antikommutator prüfen

```c
/* {c_0, c†_1} |1,2> = 0 */
ProPhysics_Fock_Set_Basis(&pu, fid, 0x03u);   /* |1,2> */
ProPhysics_Fock_Apply_Anticomm_CD(&pu, fid, 0u, 1u);

const bool is_zero = ProPhysics_Fock_Is_Zero(&pu, fid);
/* is_zero = true */
```

### §6.7 — Fermionisches Hopping-Doubling

```c
/* Zwei π/4-Hoppings = ein π/2-Hopping */
ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);

const int32_t theta_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
ProPhysics_Fock_Apply_Hopping(&pu, fid, 0u, 1u, theta_pi4);
ProPhysics_Fock_Apply_Hopping(&pu, fid, 0u, 1u, theta_pi4);

/* Zustand sollte jetzt -i·|2> sein (nur Amplitude auf Mode 1) */
```

---

## §7 — Fallstricke

### §7.1 — `n_particles` ist stale

`ProFockState.n_particles` wird nur von `Set_Basis` gesetzt. Nach
Erzeuger/Vernichter-Operationen ist es **veraltet**.

**Empfehlung:** `ProPhysics_Fock_Particle_Number` nutzen.

### §7.2 — Jordan-Wigner-Vorzeichen

Das Vorzeichen **muss** angewendet werden, sonst schlagen die
Antikommutatoren fehl. Die öffentlichen Funktionen tun das
automatisch.

### §7.3 — Null-Ergebnis bei Pauli-Blockade

`Apply_Create` auf besetzter Mode oder `Apply_Annihilate` auf leerer
Mode liefert **exakt Null**. Die Rückgabe ist `false`, und der
Zustand ist danach leer.

**Konsequenz:** Nach einem `false` ist der Zustand verloren. Nutzen
Sie `Fock_Clone` vorher, wenn Sie das vermeiden wollen.

### §7.4 — `mode >= PRO_FOCK_MODES` wird abgewiesen

`mode` muss in `[0, 7]` sein. Sonst Rückgabe `false`.

### §7.5 — `Fock_Apply_Hopping` mit `i == j` wird abgewiesen

`orbital_i == orbital_j` ist keine gültige Hopping-Konfiguration.

### §7.6 — `theta_q15 == 0` ist No-Op

`Fock_Apply_Hopping` mit `theta_q15 == 0` gibt `true` zurück, ohne
den Zustand zu ändern.

### §7.7 — Q62-Normierung

Die Norm ist in Q62 (`2^62` = physikalisch 1.0). Bei Summen über 256
Koeffizienten muss `ProU128` verwendet werden, um Overflow zu
vermeiden.

### §7.8 — `Compare`-Rückgabe

`Fock_Compare` liefert den **maximalen** Koeffizienten-Differenzbetrag
(über alle 256 Koeffizienten). Bei einem Fehler (out-of-range, inaktiv)
liefert es `1e300`.

### §7.9 — Antikommutatoren modifizieren den Zustand

`Fock_Apply_Anticomm_*` schreiben das Ergebnis der Operation
**zurück** in den Slot. Der ursprüngliche Zustand ist danach
überschrieben.

**Für diagnostische Tests:** `Fock_Clone` vorher, oder direkt auf
frisch erzeugten Basiszuständen testen.

### §7.10 — Clone-Kapazität

`Fock_Apply_Anticomm_*` benötigt **2 freie Slots** (für die
Zwischenklone). Bei `PRO_FOCK_MAX_STATES = 64` und vielen aktiven
Zuständen kann das fehlschlagen.

**Empfehlung:** Antikommutator-Tests auf kleinem `Fock_Count`
durchführen.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Tensor-Fock-Adapter | `docs/project/Tensor.md` |
| Fock-Dichte | `docs/project/Density.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/ProPhysics_Fock.c` |

---

**Ende Fock.md v1.23.0.**