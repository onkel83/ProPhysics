# ProPhysics — Modul-Referenz: Gauge

**Datei:** `docs/project/Gauge.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_Gauge.c`
**Zweck:** Referenz für das Gauge-Modul: U(1)-Eichstruktur,
Wilson-Loop, lokale Eichtransformation, Phase Plate und
Coulomb-Phase-Feld in Q16/Q30.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Gauge-Modul implementiert die **U(1)-Eichstruktur** des Kernels.
Es stellt Link-Variablen (Kantenphasen), Wilson-Loops, lokale
Eichtransformationen und ortsabhängige Phase-Felder bereit.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Phasen-Skalen und Konventionen |
| §3 | Wilson-Loop und Eichinvarianz |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/SU2.md` — SU(2)-Eichstruktur (nicht-abelsch)
- `docs/project/SU2_Dynamics.md` — SU(2)-Leapfrog-Dynamik
- `docs/project/CONFIG.md` — Konstanten (`CHANNELS_MAX` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das Gauge-Modul behandelt den **U(1)-Sektor** des Kernels. Es
arbeitet direkt auf `amp_grid` und den Kantenphasen `ProEdge.phase`.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Link-Variablen** | `Wilson_Loop` |
| **Globale Phase** | `Global_Phase` |
| **Born-Messung** | `Get_Born_Probability` |
| **λ-Feld** | `Make_Lambda_Field` |
| **Lokale Eichung** | `Apply_Local_Gauge` |
| **Phase Plate (2D)** | `Apply_Local_Phase_Plate` |
| **Coulomb-Feld (3D)** | `Apply_Coulomb_Phase_Field_3D` |
| **Test-Harness-Wrapper** | `Compute_Lambda`, `Sharp_Measure`, `Type_State_Measure` |

**Was das Modul nicht tut:**

- Keine SU(2)-Eichstruktur (die macht `SU2.c` / `SU2_Dynamics.c`).
- Keine Zeitentwicklung im engeren Sinn (kein Unitär-Step auf
  `amp_grid`; das macht `Amp.c`).
- Kein Sampling, kein RNG-getriebener Kollaps (die machen `EPR.c`
  und `Observer.c`).
- Keine Dichte-Matrizen (die macht `Density.c`).

**Aktivierung:** Keine. Das Modul wird **explizit** vom Nutzer
aufgerufen, nicht vom Kernel-Tick. (Ausnahme: Test-Harness, der
die Wrapper nutzt.)

---

## §2 — Phasen-Skalen und Konventionen

### §2.1 — Drei Skalen

| Skala | Wertebereich | Verwendung |
|---|---|---|
| **Q15** | `int32_t` (unbegrenzt) | Physikalische Kopplungsstärken (`strength_q15`, `softening_q8`) |
| **Q16** | `uint16_t`, 65536 = 2π | `ProEdge.phase`, `lambda_fx`, Rückgabe von `Wilson_Loop` |
| **Q30** | `int32_t`, 2^30 = 1 | Intern in `pro_amp_rotate_q16` (cos/sin-Tabelle) |

### §2.2 — Kantenphase (`ProEdge.phase`)

`ProEdge.phase` ist eine **Q16-Phase** (`uint16_t`). 65536 entspricht
einem vollen Umlauf (2π). Die Phase eines Links wird von
`Apply_Local_Gauge` exakt-ganzzahlig transformiert:

```
phi(x→y)  ←  phi(x→y) + λ(y) - λ(x)   mod 2^16
```

Keine Rundung, keine Festkomma-Fehler. Die Invarianz des
Wilson-Loops folgt aus dem paarweisen Wegheben der λ-Terme.

### §2.3 — Vorzeichen-Konvention im Wilson-Loop

Der U(1)-Loop iteriert **vorwärts** entlang des Pfads. Für die
abelsche Gruppe ist die Reihenfolge irrelevant; die Rückgabe ist die
Summe der Kantenphasen mod 2^16.

Der SU(2)-Loop iteriert **rückwärts** (Path-Ordered,
`W(C) = U_{n-1} · … · U_0`). Siehe `SU2.md`.

### §2.4 — Speicherort der Link-Variablen

Kantenphasen werden in `pu->edge_phases` gehalten
(`ProEdge`-Array, `total_nodes × CHANNELS_MAX`). Jeder Knoten hat
genau `CHANNELS_MAX` ausgehende Links. Das Layout ist **SoA-artig
flach**, kein Zeiger-Graph.

### §2.5 — `path_len == 1`-Sonderfall

Ein Self-Loop (`path_len == 1`) liefert die eine Phase direkt
zurück, ohne Modulo. Das ist der triviale Fall `W(1-Loop) = phi`.

---

## §3 — Wilson-Loop und Eichinvarianz

### §3.1 — Formel

Für einen geschlossenen Pfad `(x_0, c_0), …, (x_{n-1}, c_{n-1})`:

```
W(C) = Σ_k phi(x_k, c_k)   mod 2^16
```

Bei `path_len == 1`: `W = phi(x_0, c_0)`.

### §3.2 — Eichinvarianz

Unter `Apply_Local_Gauge(λ)` gilt:

```
phi(x→y) ← phi(x→y) + λ(y) - λ(x)
```

Auf einem geschlossenen Pfad heben sich die λ-Terme paarweise weg
(Teleskop-Summe):

```
Σ_k [λ(x_{k+1}) - λ(x_k)]  =  0
```

**Konsequenz:** `W(C)` ist invariant unter lokaler Eichtransformation.
Das ist der Test `test_wilson_loop_*`.

### §3.3 — Pfad-Validierung

`pro_wilson_validate_path` (in `ProPhysics_Internal.h`, geteilt mit
`SU2.c`) prüft:

- `pu`, `path_nodes`, `path_channels` nicht NULL,
- `path_len > 0`,
- Knoten-Indizes `< total_nodes`,
- Kanal-Indizes `< CHANNELS_MAX`.

Bei ungültigem Pfad liefert `ProPhysics_Wilson_Loop` den Wert `0`
in `*out_phase_fx` — **kein Fehlercode**, kein Rückgabewert.

---

## §4 — Interne Helfer

### §4.1 — `pro_amp_vector_rotate_q16`

```c
static void pro_amp_vector_rotate_q16(ProAmpVector* v, uint16_t phase_q16);
```

Rotiert alle `PRO_AMP_BASIS_SIZE` Koeffizienten eines Vektors um
dieselbe Q16-Phase.

**Ersetzt** 4× identische Schleife in `Global_Phase`,
`Apply_Local_Gauge` (Schritt 2), `Apply_Local_Phase_Plate`,
`Apply_Coulomb_Phase_Field_3D`.

**Reihenfolge bindend:** `b = 0..PRO_AMP_BASIS_SIZE-1`. Siehe §7.7.

### §4.2 — `pro_phase_q15_to_q16`

```c
static inline uint16_t pro_phase_q15_to_q16(int32_t phase_q15);
```

Normalisiert eine Q15-Phase (int32, evtl. negativ oder groß) auf
einen Q16-Winkel in `[0, 65536)`.

Faktor zwischen Eingang und Ausgang: **exakt 2**.

**Ersetzt** inline-Normalisierung in `Apply_Coulomb_Phase_Field_3D`.

### §4.3 — `pro_trig_init`

```c
static void pro_trig_init(void);
```

Füllt die modul-globalen Q30-Tabellen `PRO_COS_Q30[256]` und
`PRO_SIN_Q30[256]` lazy.

**Q30 statt Q15**, weil bei Q15 der Faktor `cos² + sin² ≈
(32767/32768)²` systematisch < 1 ist und über viele Rotationen zu
sichtbarem Normverlust akkumuliert (~6 % nach 1000 Ticks).

**Thread-Safety:** single-threaded (Konsistenz mit Kernel-Design).

### §4.4 — `pro_coulomb_cache_fill`

```c
static void pro_coulomb_cache_fill(int32_t strength_q15,
                                   uint32_t softening_q8);
```

Füllt `g_coulomb_phase_cache[256]` mit
`phase_q15 = strength_q15 · 256 / (r_q8 + softening_q8)`.

Cache wird nur neu gefüllt, wenn sich `(strength_q15,
softening_q8)` ändert. Siehe §7.4.

---

## §5 — Öffentliche API

### §5.1 — Wilson-Loop

```c
PROPHYSICS_API void ProPhysics_Wilson_Loop(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t*  path_channels,
    uint32_t        path_len,
    uint16_t*       out_phase_fx);
```

**Physik:** Summe der Kantenphasen mod 2^16 über einen geschlossenen
Pfad. Eichinvariant (siehe §3.2).

**Rückgabe:** `*out_phase_fx` = Q16-Phase. Bei ungültigem Pfad `0`.

### §5.2 — Globale Phase

```c
PROPHYSICS_API void ProPhysics_Global_Phase(
    ProUniverse* pu, uint16_t phase_fx);
```

Multipliziert **jeden** Amplitudenvektor mit `exp(i·phase_fx)`.

**No-Op**, wenn `phase_fx == 0`.

### §5.3 — Born-Wahrscheinlichkeit

```c
PROPHYSICS_API double ProPhysics_Get_Born_Probability(
    const ProUniverse* pu, uint64_t node_idx, uint8_t basis_idx);
```

`P(b | k) = |c_b(k)|² / Σ_j |c_j(k)|²`. Rein lesend.

**Rückgabe:** Wert in `[0, 1]`. Bei `Σ_j |c_j|² = 0` → `0.0`.

### §5.4 — λ-Feld

```c
PROPHYSICS_API void ProPhysics_Make_Lambda_Field(
    uint64_t seed, uint16_t* lambda_fx, uint64_t total_nodes);
```

**splitmix64-Stream** → deterministische Q16-Phasen pro Knoten.
Gleicher Seed ⇒ gleiches Feld.

### §5.5 — Lokale Eichtransformation

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Gauge(
    ProUniverse* pu, const uint16_t* lambda_fx);
```

1. Kantenphase: `phi(x→y) += λ(y) - λ(x)` mod 2^16 (exakt).
2. Amplitude: `psi(x) ← e^{iλ(x)} psi(x)` (Q30-Rotation).

Der Wilson-Loop ist invariant (siehe §3.2).

### §5.6 — Phase Plate (2D)

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Phase_Plate(
    ProUniverse* pu,
    uint32_t x0, uint32_t y0,
    uint32_t w,  uint32_t h,
    uint16_t phase_q15);
```

Rotiert alle Knoten im 2D-Rechteck `[x0, x0+w) × [y0, y0+h)` um
denselben Winkel. **Nur ein Tick**, nicht persistent — der Aufrufer
ruft pro Tick auf, wenn eine dauerhafte Phase Plate gewünscht ist.

**Nur 2D.** Rückkehr ohne Wirkung bei `grid_ndim == 3`.

### §5.7 — Coulomb-Phase-Feld (3D)

```c
PROPHYSICS_API void ProPhysics_Apply_Coulomb_Phase_Field_3D(
    ProUniverse* pu,
    uint32_t cx, uint32_t cy, uint32_t cz,
    int32_t  strength_q15,
    uint32_t softening_q8);
```

Radialsymmetrisches `1/r`-Phase-Feld um `(cx, cy, cz)` im 3D-Torus.
Pro Tick wird jeder Knoten um

```
phase_q15(r) = strength_q15 / (r + softening_q8/256)
```

rotiert, mit Torus-Abstand `dx = min(|x-cx|, dim-|x-cx|)`.

Entspricht Schrödinger-Zeitentwicklung mit `V(r) = -K/r`.

**Nur 3D** und **nur Zweierpotenz-`dim`** (siehe §7.5).

### §5.8 — Test-Harness-Wrapper

| Funktion | Zweck |
|---|---|
| `ProPhysics_Compute_Lambda` | Wrapper um `pro_amp_to_lambda` |
| `ProPhysics_Sharp_Measure` | Wrapper um `pro_measure_sharp` |
| `ProPhysics_Type_State_Measure` | Deterministische Messung (U6c) |

**`Type_State_Measure`:** `sign(cos(θ - λ_state))`, mit
`λ_state = 0` für `UR_POSITRON_CW` und `π` für `UR_NEGATRON_CCW`.
Kein RNG.

---

## §6 — Verwendungsmuster

### §6.1 — Wilson-Loop auf einem 4-Knoten-Quadrat

```c
ProUniverse pu;
/* ... Setup, Kantenphasen gesetzt ... */

const uint64_t nodes[4]    = { 0u, 1u, 2u, 3u };
const uint8_t  channels[4] = { 0u, 0u, 0u, 0u };

uint16_t w = 0u;
ProPhysics_Wilson_Loop(&pu, nodes, channels, 4u, &w);
/* w = Summe der vier Phasen mod 2^16 */
```

### §6.2 — Eichinvarianz prüfen

```c
uint16_t w_before = 0u;
ProPhysics_Wilson_Loop(&pu, nodes, channels, 4u, &w_before);

uint16_t lambda[pu.total_nodes];
ProPhysics_Make_Lambda_Field(0xDEADBEEFull, lambda, pu.total_nodes);
ProPhysics_Apply_Local_Gauge(&pu, lambda);

uint16_t w_after = 0u;
ProPhysics_Wilson_Loop(&pu, nodes, channels, 4u, &w_after);

/* w_before == w_after  (Eichinvarianz) */
```

### §6.3 — λ-Feld deterministisch reproduzieren

```c
uint16_t lam_a[1024], lam_b[1024];
ProPhysics_Make_Lambda_Field(42u, lam_a, 1024u);
ProPhysics_Make_Lambda_Field(42u, lam_b, 1024u);
/* memcmp(lam_a, lam_b, sizeof(lam_a)) == 0 */
```

### §6.4 — Born-Wahrscheinlichkeit eines Basiszustands

```c
ProPhysics_Set_Basis(&pu, /* ... */);

const double p0 = ProPhysics_Get_Born_Probability(&pu, 0u, 0u);
const double p1 = ProPhysics_Get_Born_Probability(&pu, 0u, 1u);
/* p0 + p1 + ... + p_{N-1} = 1.0 (bis auf Q31-Rundung) */
```

### §6.5 — Doppelspalt mit Phase Plate (2D)

```c
/* dim = 128, Doppelspalt bei y = 64, Phase Plate über einen Arm */
ProPhysics_Apply_Local_Phase_Plate(&pu,
    64u, 64u, 32u, 1u,       /* x0, y0, w, h */
    (uint16_t)32768u);       /* halbe Phase = π */
```

**Pro Tick aufrufen**, wenn die Phase kumulativ wirken soll.

### §6.6 — Coulomb-Feld im 3D-Torus

```c
/* dim = 32, Zentrum (16, 16, 16), softening = 1.0 in Q8 */
ProPhysics_Apply_Coulomb_Phase_Field_3D(&pu,
    16u, 16u, 16u,
    /* strength_q15 */ 16384,   /* ~π */
    /* softening_q8 */ 256u);    /* 1.0 */
```

Der Cache wird beim ersten Aufruf gefüllt; weitere Aufrufe mit
gleichem `(strength_q15, softening_q8)` sind Div-frei.

### §6.7 — Type-State-Messung

```c
const int s = ProPhysics_Type_State_Measure(/* θ */ 0.0,
                                             UR_POSITRON_CW);
/* s = +1 (cos(0) > 0) */

const int s2 = ProPhysics_Type_State_Measure(/* θ */ 0.0,
                                              UR_NEGATRON_CCW);
/* s2 = -1 (cos(-π) < 0) */
```

---

## §7 — Fallstricke

### §7.1 — `phase_q15` in `Apply_Local_Phase_Plate` ist Q16

Der Parameter heißt `phase_q15`, ist aber **Q16** (65536 = 2π). Er
wird direkt an `pro_amp_rotate_q16` durchgereicht. Der Name ist
historisch; die Konvention steht im Funktionskommentar.

**Empfehlung:** Beim nächsten API-Bruch umbenennen in `phase_q16`.

### §7.2 — `path_len == 0`

Ein leerer Pfad wird von `pro_wilson_validate_path` abgewiesen;
`*out_phase_fx` bleibt `0`.

**Kein Fehlercode** — der Aufrufer muss den Rückgabewert nicht
prüfen, aber `0` ist ein gültiger Wilson-Loop-Wert. Bei
diagnostischen Tests ggf. vorher `path_len > 0` prüfen.

### §7.3 — `pro_trig_init` ist lazy, nicht thread-safe

Die Q30-Tabellen werden beim ersten `pro_amp_rotate_q16`-Aufruf
gefüllt. Zwei Threads würden parallel dieselben Werte schreiben
(race, aber idempotent). **Konsistenz mit single-threaded
Kernel-Design.**

### §7.4 — Coulomb-Cache ist modul-global

`ProPhysics_Apply_Coulomb_Phase_Field_3D` hält einen
**modul-globalen** Cache, geschlüsselt auf `(strength_q15,
softening_q8)`. Der Cache wird neu gefüllt, wenn sich einer der
Werte ändert.

**Konsequenzen:**

- **Nicht thread-safe.**
- Bei Parameter-Sweeps wird der Cache ggf. pro Aufruf neu gefüllt.
- Der Cache überlebt `ProUniverse`-Lifecycle.

### §7.5 — `Apply_Coulomb_Phase_Field_3D` nur 3D + Zweierpotenz

Rückkehr ohne Wirkung bei:

- `grid_ndim != 3`,
- `dim & (dim-1) != 0` (keine Zweierpotenz),
- `strength_q15 == 0`,
- `cx`, `cy` oder `cz` außerhalb `[0, dim)`.

Alle Fälle sind stille No-Ops.

### §7.6 — `Apply_Local_Phase_Plate` nur 2D

Rückkehr ohne Wirkung bei `grid_ndim == 3`. Der Doppelspalt-Test
arbeitet in 2D. Für 3D müsste die Schleife auf einen Quader
verallgemeinert werden.

### §7.7 — Rotationsreihenfolge ist bindend

`pro_amp_vector_rotate_q16` iteriert `b = 0..PRO_AMP_BASIS_SIZE-1`.
Diese Reihenfolge ist deterministisch; sie bestimmt (falls
`pro_amp_rotate_q16` RNG nutzen würde) die Reproduzierbarkeit.

**Änderungen an der Reihenfolge erfordern eine neue Etappe.**

### §7.8 — `Make_Lambda_Field` mit `seed = 0`

`seed = 0` ist ein gültiger Seed (splitmix64 mit `h = 0`). Kein
Sonderfall, keine No-Op.

### §7.9 — `Global_Phase` läuft über `total_nodes`

Auch über Knoten, deren Amplitude Null ist. Wer Performance
optimieren will, muss vorher prüfen, ob der Vektor nicht schon
`phase_fx == 0` hat (interner Skip).

### §7.10 — `Apply_Local_Gauge` ändert Kantenphasen **und** Amplituden

Die Funktion hat zwei Wirkungen:

1. Kantenphase `phi(x→y) += λ(y) - λ(x)`.
2. Amplitude `psi(x) ← e^{iλ(x)} psi(x)`.

Beide Wirkungen sind **notwendig** für die Eichinvarianz des
Wilson-Loops. Wer nur eine anwendet, bricht die Invarianz.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| SU(2)-Modul | `docs/project/SU2.md` |
| SU(2)-Dynamik | `docs/project/SU2_Dynamics.md` |
| Fock-Modul | `docs/project/Fock.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/kernel/ProPhysics_Gauge.c` |
| Interne API | `ProPhysics_Internal.h` |
| Kernel-API | `ProPhysics.h` |

---

**Ende Gauge.md v1.0.**