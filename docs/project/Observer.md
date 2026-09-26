# ProPhysics — Modul-Referenz: Observer

**Datei:** `docs/project/Observer.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_Observer.c`
**Zweck:** Referenz für das Observer-Modul: test-spezifische
Zusatzdynamik auf `amp_grid` (Diffusion, CHSH-Messung, chaotische
Quelle, Dephasing).

---

## §0 — Wie dieses Dokument zu lesen ist

Das Observer-Modul implementiert **test-spezifische** Zusatzdynamik,
die direkt auf `amp_grid` wirkt. Es ist **nicht** Teil der 5 Urregeln
und **nicht** Teil des SDK-Ticks. Es dient dazu, bestimmte Hypothesen
im Test-Harness zu prüfen (CHSH, Bell, Diffusion, Dephasing).

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Observer-Deskriptor und Konventionen |
| §3 | Physikalische Interpretation (bindend) |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Gauge.md` — `pro_amp_rotate_q16`, `pro_amp_to_lambda`
- `docs/project/Core.md` — `amp_grid` und `amp_scratch`
- `docs/project/CONFIG.md` — Konstanten (`PRO_DEPHASE_CHANNEL` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das Observer-Modul behandelt die **Test-Harness-Dynamik**: es
moduliert `amp_grid` lokal, um Hypothesen zu prüfen.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Deskriptor** | `Init_Observer` |
| **Lokales Lesen** | `Observer_Read_Local`, `Get_Environment_Trace` |
| **Lineare Diffusion** | `Apply_Local_Amplitude_Diffusion` |
| **Nichtlineare Diffusion** | `Apply_Nonlinear_Diffusion_Tick` |
| **CHSH-Messung** | `Observer_Measure_CHSH`, `Observer_Measure_CHSH_Projected` |
| **Chaotische Quelle** | `Init_Chaotic_Source` |
| **Dephasing** | `Apply_Local_Dephasing_Tick` |

**Was das Modul nicht tut:**

- Keine Zeitentwicklung im Sinne der Urregeln (kein Tick, kein
  Unitär-Step). Die Zeitentwicklung liegt in `Amp.c`.
- Kein RNG-gesteuerter Kollaps (das macht `EPR.c`).
- Kein SDK-Tick. Die Funktionen sind explizit aufzurufen.
- Keine Modifikation der Urregeln oder des U5-Pfads.

**Aktivierung:** Keine. Das Modul wird **explizit** vom Test-Harness
aufgerufen, nicht vom Kernel-Tick.

---

## §2 — Observer-Deskriptor und Konventionen

### §2.1 — Deskriptor

```c
typedef struct {
    uint64_t base_node;   /* Start-Knoten der Region */
    uint32_t size;        /* Anzahl Knoten, >= 1 */
    uint32_t _pad;        /* Padding auf 16 Bytes */
} ProObserver;
```

Ein Observer ist eine **zusammenhängende Region** `[base_node,
base_node + size)` in `amp_grid`. `_pad` dient der Cache-Alignment.

### §2.2 — Region und Lesen

`Observer_Read_Local` liefert den **Mittelwert** der Amplitudenvektoren
über die Region:

```
out_amp[b] = (1/size) * Σ_{k=base}^{base+size-1} amp_grid[k].coeff[b]
```

Rein lesend, Q31-Clamping nach der Summation.

### §2.3 — `amp_grid` / `amp_scratch` (Ping-Pong)

Diffusions-Operationen arbeiten mit zwei Puffern: `amp_grid` (Quelle)
und `amp_scratch` (Ziel). Nach der Berechnung tauscht ein Ping-Pong
die Rollen; `amp_scratch` wird zum neuen Arbeitspuffer für den
nächsten Pass.

**Konsequenz:** Ein Diffusions-Pass **ersetzt** `amp_grid`. Der alte
Zustand ist nach dem Ping-Pong in `amp_scratch` und wird beim
nächsten Pass überschrieben.

### §2.4 — Speicherort

Observer sind **Value-Objects** auf dem Stack des Aufrufers (kein
Slot-Allocator). `Init_Observer` weist sie rein zu, keine Allokation.

### §2.5 — Deterministische Seeds

Zwei Funktionen erzeugen deterministische „Zufalls"-Werte aus einem
Hash:

- `Init_Chaotic_Source`: `splitmix64(x*A + y*B + seed)`
- `Apply_Local_Dephasing_Tick`: `splitmix64(k*A + partner*B + ts*C)`

**Kein PRNG, keine externe Entropie.** Gleicher Seed ⇒ identisches
Ergebnis. Das ist eine bewusste Design-Entscheidung: Tests sind
reproduzierbar, keine `rand()`-Abhängigkeit.

Der CHSH-Projected-Pfad nutzt dagegen **xoshiro256\*\*** mit einem
explizit übergebenen `rng[4]` — das ist der einzige RNG-Pfad.

---

## §3 — Physikalische Interpretation (bindend)

**Alle Funktionen in diesem Modul sind test-spezifisch.** Sie sind:

- **NICHT** Teil von `pro_urregeln_apply` (in v3.0 entfernt),
- **NICHT** Teil des SDK-Ticks (`ProPhysics_SDK_Execute_Plastizitaet_Tick`),
- **NICHT** im U5-Invarianten-Pfad.

Sie modulieren `amp_grid` **direkt**, um Hypothesen zu prüfen.

**Konsequenz:** Ein CHSH-Wert `S > 2` in einem aufrufenden Test ist
**kein Bell-Bruch**, sondern entweder ein Superdeterminismus-Loop
oder ein Testdesign-Fehler. Die Diffusion/Dephasing-Operationen
sind ausdrücklich als Werkzeuge zur Prüfung dieser Hypothese
gedacht, nicht als Physik des Kernels.

---

## §4 — Interne Helfer

### §4.1 — `pro_amp_vector_abs2_sum`

```c
static inline ProU128 pro_amp_vector_abs2_sum(const ProAmpVector* v);
```

Summe `Σ_b |c_b|²` über alle `PRO_AMP_BASIS_SIZE` Koeffizienten eines
Vektors, als `ProU128` (Overflow-sicher).

**Ersetzt** 2× inline-Schleife in `Get_Environment_Trace` und in der
Sättigungs-Prüfung von `Apply_Nonlinear_Diffusion_Tick`.

### §4.2 — `pro_observer_diffusion_pass`

```c
static void pro_observer_diffusion_pass(
    ProUniverse* pu,
    int64_t alpha_q31,
    uint8_t n_nb,
    const uint8_t* ch_arr);
```

Gemeinsamer Diffusions-Kern:

```
psi_k <- (1-alpha) * psi_k + (alpha/N) * Σ_{i<N} psi_{nb_i(k)}
```

Schreibt das Ergebnis nach `pu->amp_scratch`; der Aufrufer ruft
danach `pro_observer_ping_pong`.

**Ersetzt** 2× Diffusions-Schleife in `Apply_Local_Amplitude_Diffusion`
und `Apply_Nonlinear_Diffusion_Tick`.

### §4.3 — `pro_observer_ping_pong`

```c
static inline void pro_observer_ping_pong(ProUniverse* pu);
```

Tauscht `pu->amp_grid` und `pu->amp_scratch`. Nach einem
Diffusions-Pass liegt das Ergebnis in `amp_scratch`; dieser Tausch
macht es zum neuen `amp_grid`.

### §4.4 — `pro_observer_measure_axis`

```c
static inline int pro_observer_measure_axis(
    const ProAmpVector* v, double theta);
```

`sign(cos(theta - pro_amp_to_lambda(v)))`.

**Ersetzt** 4× inline-Aufruf in beiden CHSH-Varianten.

---

## §5 — Öffentliche API

### §5.1 — Deskriptor

```c
PROPHYSICS_API void ProPhysics_Init_Observer(
    ProUniverse* pu, ProObserver* obs,
    uint64_t base_node, uint32_t size);
```

Weist `obs` auf die Region `[base_node, base_node + size)` zu.
Setzt `obs` auf Null, wenn die Region ungültig ist (außerhalb
`total_nodes` oder `size == 0`).

**Kein Rückgabewert** — der Aufrufer prüft `obs->size > 0`.

### §5.2 — Lokales Lesen

```c
PROPHYSICS_API bool ProPhysics_Observer_Read_Local(
    const ProUniverse* pu, const ProObserver* obs,
    ProAmpVector* out_amp);
```

Mittelwert der `amp_grid`-Vektoren über die Region. Q31-Clamping
nach der Summation.

```c
PROPHYSICS_API ProU128 ProPhysics_Get_Environment_Trace(
    const ProUniverse* pu, const ProObserver* obs);
```

Summe `Σ_{k nicht in obs} Σ_b |c_b(k)|²`, als `ProU128`.

**Rein lesend.** Kein Ping-Pong.

### §5.3 — Diffusion

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Amplitude_Diffusion(
    ProUniverse* pu, uint32_t rate_percent);
```

Lineare Diffusion mit **4 Nachbarn** (2D) oder **6 Nachbarn** (3D,
`grid_ndim == 3`). `rate_percent` in `[0, 100]`; > 100 wird auf 100
geklemmt. **Ping-Pong** am Ende.

```c
PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Diffusion_Tick(
    ProUniverse* pu,
    uint32_t rate_percent,
    uint64_t saturation_threshold_q62);
```

Schritt 1: lineare Diffusion mit **4 Nachbarn** (fest, auch in 3D).
Schritt 2: Groß-Pitaevskii-artige Sättigung — falls
`Σ_b |c_b|² > saturation_threshold_q62`, skaliere alle Koeffizienten
mit `threshold/dichte`. **Ping-Pong** am Ende.

`saturation_threshold_q62 == 0` deaktiviert die Sättigung.

### §5.4 — CHSH-Messung

```c
PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH(
    const ProUniverse* pu,
    const ProObserver* obs_A, const ProObserver* obs_B,
    double theta_a, double theta_b,
    int* out_a, int* out_b);
```

Mittelwert-basierte Messung: liest `Read_Local` pro Observer,
berechnet `lambda` und wertet `sign(cos(theta - lambda))` aus.

```c
PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH_Projected(
    const ProUniverse* pu,
    const ProObserver* obs_A, const ProObserver* obs_B,
    double theta_a, double theta_b,
    uint64_t rng[4],
    int* out_a, int* out_b,
    uint64_t* out_node_a, uint64_t* out_node_b);
```

Projizierte Messung: zieht **einen** Knoten pro Observer via
xoshiro256\*\* (`pro_xoshiro_next`). `out_node_a`/`out_node_b` sind
optional.

**Rein lesend**, kein Ping-Pong, keine Modifikation von `amp_grid`.

### §5.5 — Chaotische Quelle

```c
PROPHYSICS_API void ProPhysics_Init_Chaotic_Source(
    ProUniverse* pu, uint64_t base_node, uint32_t dim, uint64_t seed);
```

Initialisiert eine `dim × dim`-Region ab `base_node` mit
deterministischen Amplituden:

```
lambda(x,y) = 2π · fract( splitmix64(x·A + y·B + seed) / 2^64 )
psi(x,y)    = cos(lambda/2) |up> + sin(lambda/2) |down>
```

**Überschreibt** die Region vollständig (setzt alle anderen
Basiskomponenten auf 0).

### §5.6 — Dephasing

```c
PROPHYSICS_API void ProPhysics_Apply_Local_Dephasing_Tick(
    ProUniverse* pu, uint32_t strength_percent);
```

Rotiert `coeff[UR_NEGATRON_CCW]` um einen deterministischen Winkel:

```
h   = splitmix64(k·A + channels[5][k]·B + type_state·C)
phi = U(0,1)(h) · 2π · (strength_percent/100)
c4  = c4 · exp(i·phi)
```

**Unär** — `|c4|²` bleibt erhalten. `strength_percent == 0` ist No-Op,
`100` ergibt volle 2π-Rotation.

---

## §6 — Verwendungsmuster

### §6.1 — Observer anlegen und lokal lesen

```c
ProObserver obs_A;
ProPhysics_Init_Observer(&pu, &obs_A, /*base*/ 0u, /*size*/ 64u);

ProAmpVector amp_A;
ProPhysics_Observer_Read_Local(&pu, &obs_A, &amp_A);
/* amp_A ist der Mittelwert der ersten 64 Knoten */
```

### §6.2 — Environment-Trace

```c
ProObserver obs;
ProPhysics_Init_Observer(&pu, &obs, 0u, 64u);

const ProU128 env = ProPhysics_Get_Environment_Trace(&pu, &obs);
const double env_d = pro_u128_to_double(env);
/* env_d = Σ_{k ≥ 64} Σ_b |c_b(k)|² */
```

### §6.3 — Lineare Diffusion

```c
/* 10 % Diffusion, 2D (4 Nachbarn) */
ProPhysics_Apply_Local_Amplitude_Diffusion(&pu, 10u);

/* danach: amp_grid wurde durch den Ping-Pong ersetzt */
```

### §6.4 — Nichtlineare Diffusion mit Sättigung

```c
/* Sättigung bei Q62 = 2^60, d.h. 1/4 der Einheitsnorm */
const uint64_t threshold = 1ULL << 60;
ProPhysics_Apply_Nonlinear_Diffusion_Tick(&pu, 20u, threshold);
```

### §6.5 — CHSH-Mittelwert

```c
ProObserver obs_A, obs_B;
ProPhysics_Init_Observer(&pu, &obs_A,   0u, 64u);
ProPhysics_Init_Observer(&pu, &obs_B, 512u, 64u);

int a = 0, b = 0;
ProPhysics_Observer_Measure_CHSH(&pu, &obs_A, &obs_B,
    0.0,             /* theta_a */
    PRO_PI * 0.25,   /* theta_b */
    &a, &b);
```

### §6.6 — CHSH projiziert (mit Diagnose)

```c
uint64_t rng[4] = { 0xDEADBEEFull, 0xCAFEBABEull, 0x12345678ull, 0xABCDEF01ull };
int a = 0, b = 0;
uint64_t node_a = 0, node_b = 0;

ProPhysics_Observer_Measure_CHSH_Projected(&pu,
    &obs_A, &obs_B,
    /* theta_a */ 0.0,
    /* theta_b */ PRO_PI * 0.25,
    rng,
    &a, &b,
    &node_a, &node_b);
/* node_a und node_b enthalten die tatsächlich gezogenen Knoten */
```

### §6.7 — Chaotische Quelle

```c
/* 32×32-Region ab Knoten 1024, Seed 42 */
ProPhysics_Init_Chaotic_Source(&pu, 1024u, 32u, 42u);
```

### §6.8 — Dephasing

```c
/* 50 % Dephasing-Stärke auf coeff[NEGATRON_CCW] */
ProPhysics_Apply_Local_Dephasing_Tick(&pu, 50u);
```

---

## §7 — Fallstricke

### §7.1 — Test-spezifisch, kein Kernel-Verhalten

Funktionen dieses Moduls sind **ausdrücklich nicht** Teil der
Physik des Kernels. Sie modulieren `amp_grid` direkt und
überschreiben den Zustand. Nach einem Aufruf kann `amp_grid`
nicht mehr als „physikalisch entwickelter" Zustand interpretiert
werden, ohne die Test-Semantik zu kennen.

### §7.2 — `S > 2` ist kein Bell-Bruch

Der CHSH-Wert aus einem Test mit diesem Modul kann `> 2` sein.
Das ist **kein** Verstoß gegen die Bell-Ungleichung, sondern ein
Superdeterminismus-Loop oder Testdesign-Fehler. Die Diffusion/
Dephasing-Operationen sind Werkzeuge, um genau diese Hypothese
zu prüfen.

### §7.3 — Ping-Pong tauscht `amp_grid` / `amp_scratch`

Nach `Apply_Local_Amplitude_Diffusion` und
`Apply_Nonlinear_Diffusion_Tick` zeigt `amp_grid` auf den
**ehemaligen Scratch-Puffer**. Wer einen Zeiger auf den alten
`amp_grid` gespeichert hat, muss ihn neu holen.

### §7.4 — `rate_percent > 100` wird geklemmt

Beide Diffusions-Funktionen klemmen `rate_percent` auf `100`
(statt Fehler zu melden). `rate_percent == 0` ist No-Op.

### §7.5 — `Nonlinear` nutzt **4** Nachbarn, auch in 3D

`Apply_Nonlinear_Diffusion_Tick` verwendet **immer** die 2D-Reihenfolge
`{0, 1, 2, 3}`, unabhängig von `grid_ndim`. Das ist bewusst
bit-identisch zum Vorzustand (vor Etappe 17). Wer 3D-Diffusion mit
Sättigung will, muss das in einer neuen Etappe ändern.

`Apply_Local_Amplitude_Diffusion` nutzt dagegen **4 oder 6** Nachbarn
je nach `grid_ndim`.

### §7.6 — Sättigung liest aus `amp_scratch`

Die Sättigung in `Nonlinear` operiert **vor** dem Ping-Pong auf
`pu->amp_scratch`. Nach dem Ping-Pong ist der Scratch-Puffer der
alte `amp_grid`. Wer zwischen Schritt 1 und Ping-Pong eingreifen
will, muss das innerhalb dieses Moduls tun.

### §7.7 — `rng` muss 4 `uint64_t` sein

`Observer_Measure_CHSH_Projected` erwartet `uint64_t rng[4]` für
xoshiro256\*\*. Der Aufrufer ist für die Initialisierung
verantwortlich (vier Nicht-Null-Werte).

### §7.8 — Dephasing rotiert nur `NEGATRON_CCW`

`Apply_Local_Dephasing_Tick` rotiert **nur** `coeff[UR_NEGATRON_CCW]`
(Basisindex 4). Die übrigen Basiszustände bleiben unverändert. Das
erhält `|c4|²`, ist aber keine allgemeine Dephasing-Operation.

### §7.9 — Deterministische Hashes, kein PRNG

`Init_Chaotic_Source` und `Apply_Local_Dephasing_Tick` nutzen
splitmix64-basierte Hashes. Gleicher Aufruf ⇒ gleiches Ergebnis.
Nur der CHSH-Projected-Pfad nutzt RNG (xoshiro256\*\*).

### §7.10 — `Init_Chaotic_Source` überschreibt die Region

Die Funktion setzt **alle** `PRO_AMP_BASIS_SIZE` Koeffizienten
der Region auf 0, dann `POSITRON_CW` und `NEGATRON_CCW` auf die
chaotischen Werte. Andere Basiszustände sind danach 0.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Gauge-Modul | `docs/project/Gauge.md` |
| Core-Modul | `docs/project/Core.md` |
| EPR-Modul | `docs/project/EPR.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/kernel/ProPhysics_Observer.c` |

---

**Ende Observer.md v1.0.**