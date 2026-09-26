# ProPhysics — Modul-Referenz: SU2_Dynamics

**Datei:** `docs/project/SU2_Dynamics.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_SU2_Dynamics.c`
**Zweck:** Referenz für die SU(2)-Link-Dynamik: klassisches
Yang-Mills mit Leapfrog-Integration, Plaquette-Action und
Link-Plaquette-Summe für Metropolis/HMC.

---

## §0 — Wie dieses Dokument zu lesen ist

Das SU(2)-Dynamics-Modul implementiert die **klassische
Yang-Mills-Zeitentwicklung** auf dem Gitter. Es arbeitet auf den
`ProEdge.su2_*`-Link-Feldern und den zugehörigen
`ProEdge.su2_E_*`-Feldvariablen. `amp_grid` bleibt unberührt.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Hamilton-Funktion und Konventionen |
| §3 | Leapfrog-Integration |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/SU2.md` — SU(2)-Links und Wilson-Loop
- `docs/project/CONFIG.md` — Konstanten (`PRO_SU2_SCALE` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das SU(2)-Dynamics-Modul behandelt die **klassische
Yang-Mills-Dynamik**. Es integriert die Bewegungsgleichungen
zwischen zwei Konfigurationen und stellt die Metropolis-relevanten
Energie-Funktionen bereit.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Aktivierung** | `Enable_SU2_Dynamics`, `Disable_SU2_Dynamics`, `Is_SU2_Dynamics_Active` |
| **Kopplung** | `Set_SU2_Yang_Mills` |
| **Leapfrog-Tick** | `Apply_SU2_Tick` |
| **Action** | `SU2_Plaquette_Action` |
| **Energie** | `SU2_Total_Energy` |
| **Link-lokale Action** | `SU2_Link_Plaquette_Sum` |

**Was das Modul nicht tut:**

- Keine Link-Initialisierung (macht `SU2.c` via `Set_Edge_SU2`).
- Keine Wilson-Loop-Berechnung (macht `SU2.c`).
- Kein Metropolis/HMC-Sampling (im Test-Harness).
- Keine Modifikation von `amp_grid`.

**Aktivierung:** `su2_dynamics_active == 0` deaktiviert den Tick.
`Apply_SU2_Tick` ist ein No-Op in diesem Zustand.

---

## §2 — Hamilton-Funktion und Konventionen

### §2.1 — Hamilton-Funktion

```
H = S_plaq + (1/2) · Σ_links |E|²
```

### §2.2 — Wilson-Plaquette-Action

```
S_plaq = Σ_plaq (1 − 0.5 · Re Tr(W_plaq))
W_plaq = U_μ(x) · U_ν(x+μ) · U_μ(x+ν)† · U_ν(x)†
```

Mit der Quaternion-Form `W = [[a, b], [−b*, a*]]` folgt
`Re Tr(W) = 2 · Re(a)`, also `S_plaq = Σ (1 − Re(a)/2^30)`.

### §2.3 — Feldstärke `E`

`E` wird in denselben `ProEdge`-Slots gehalten wie der Link, aber in
den Feldern `su2_E_a_re`, `su2_E_a_im`, `su2_E_b_re`, `su2_E_b_im`.
Skala: `PRO_SU2_SCALE = 2^30`.

### §2.4 — Kanal-Mapping

```
SU2_FWD_CH = { X_PLUS(0), Y_PLUS(2), Z_PLUS(4) }
SU2_REV_CH = { X_MINUS(1), Y_MINUS(3), Z_MINUS(6) }
```

`E` und Link-Felder werden nur auf **Vorwärts**-Kanälen geführt.
Rückwärts-Kanäle sind für Plaquette-Formeln nötig, werden aber
nicht direkt gespeichert.

### §2.5 — `g²` und `dt`

- `g²_q15`: Yang-Mills-Kopplung, in Q15 (`pu->su2_yang_mills_q15`).
- `dt_q15`: Zeitschritt, in Q15 (Argument von `Apply_SU2_Tick`).
- `half_dt_g2 = (dt_q15 · g²_q15) >> 16` — in Q15.

---

## §3 — Leapfrog-Integration

### §3.1 — Stoermer-Verlet

```
1. E ← E + (dt/2) · g² · F(U)
2. U ← exp(i · dt · E) · U
3. E ← E + (dt/2) · g² · F(U_neu)
```

`F(U)` ist die su(2)-Projektion der Staple-Summe um den Link.

### §3.2 — Warum Leapfrog?

- Symplektisch: erhält die Hamilton-Funktion über lange Zeiten
  (Energie-Drift nur O(dt²)).
- Zeitschritt-reversibel: `U(t) → U(−t)` ist exakt.
- Kein expliziter Runge-Kutta-Fehler-Aufbau.

### §3.3 — su(2)-Projektion

Für ein Quaternion `P = U · S†` ist die Projektion auf die
su(2)-Algebra:

```
F = (0, P_a_im, P_b_re, P_b_im)
```

Der Realteil `P_a_re` wird verworfen (er entspricht der
u(1)-Komponente).

### §3.4 — Staple-Summe

Pro Link werden `n` Staple-Beiträge aufsummiert (in 3D bis zu 4:
2 Richtungen × je Vorwärts/Rückwärts). Der zurückgegebene
Kraft-Wert ist `Σ_i P_i / n`.

**Bit-Identität:** Die Division ist eine Ganzzahl-Division
(abgerundet), Reihenfolge der Summierung unverändert.

---

## §4 — Interne Helfer

### §4.1 — `su2_link_exists`

```c
static inline bool su2_link_exists(
    const ProUniverse* pu, uint64_t x, uint8_t ch);
```

Prüft, ob die Kante `(x, ch)` einen gültigen Nachbarn hat
(`nb < total_nodes && nb != x`). Kein Edge-Slot-Zugriff.

### §4.2 — `su2_read_link` / `su2_write_link`

```c
static inline void su2_read_link(const ProUniverse* pu, uint64_t x, uint8_t ch,
    int32_t* a_re, int32_t* a_im, int32_t* b_re, int32_t* b_im);
static inline void su2_write_link(ProUniverse* pu, uint64_t x, uint8_t ch,
    int32_t a_re, int32_t a_im, int32_t b_re, int32_t b_im);
```

Dünne Wrapper um `pro_su2_edge` / `pro_su2_edge_mut` (in
`ProPhysics_Internal.h`, seit Patch 1.23.7). Lesen/schreiben die
Link-Felder `su2_a_*`, `su2_b_*`.

### §4.3 — `su2_read_E` / `su2_write_E`

Wie §4.2, aber für die `su2_E_*`-Felder.

### §4.4 — `su2_accumulate_staple`

```c
static inline void su2_accumulate_staple(
    int32_t u_ar, int32_t u_ai, int32_t u_br, int32_t u_bi,
    int32_t s_ar, int32_t s_ai, int32_t s_br, int32_t s_bi,
    int64_t* acc_ai, int64_t* acc_br, int64_t* acc_bi);
```

Berechnet `P = U · S†`, verwirft den Realteil `P_a_re` und
addiert die drei su(2)-Komponenten in die Akkumulatoren.

### §4.5 — `su2_force_on_link`

```c
static void su2_force_on_link(const ProUniverse* pu, uint64_t x, uint8_t mu,
    int32_t* out_ai, int32_t* out_br, int32_t* out_bi);
```

Sammelt alle Staples um `(x, μ)` und liefert `Σ_i P_i / n` als
su(2)-Projektion. `n == 0` → alle Ausgaben 0.

### §4.6 — `su2_plaquette_action_at`

```c
static double su2_plaquette_action_at(
    const ProUniverse* pu, uint64_t y, uint8_t alpha, uint8_t beta);
```

Berechnet `S_plaq(W_{α,β}(y)) = 1 − Re(a_W)/2^30`.
Rückgabe `0.0`, wenn einer der vier Links nicht existiert.

**Verwendung:** Sowohl im Hauptloop von
`ProPhysics_SU2_Plaquette_Action` (mit `α = μ`, `β = ν` als
sortiertes FWD-Paar) als auch in `SU2_Link_Plaquette_Sum`.

### §4.7 — `su2_leapfrog_kick_E`

```c
static void su2_leapfrog_kick_E(ProUniverse* pu, int64_t half_dt_g2);
```

Führt Schritt 1 oder Schritt 3 der Leapfrog-Integration aus:
`E ← E + half_dt_g2 · F(U)`. Wird zweimal pro Tick aufgerufen.

---

## §5 — Öffentliche API

### §5.1 — Aktivierung

```c
PROPHYSICS_API void ProPhysics_Enable_SU2_Dynamics(ProUniverse* pu);
PROPHYSICS_API void ProPhysics_Disable_SU2_Dynamics(ProUniverse* pu);
PROPHYSICS_API int  ProPhysics_Is_SU2_Dynamics_Active(const ProUniverse* pu);
```

Setzt bzw. liest `pu->su2_dynamics_active`.

### §5.2 — Kopplung

```c
PROPHYSICS_API void ProPhysics_Set_SU2_Yang_Mills(ProUniverse* pu, int32_t g2_q15);
```

Setzt `pu->su2_yang_mills_q15`. `g² == 0` deaktiviert den Tick
implizit (siehe §7.3).

### §5.3 — Leapfrog-Tick

```c
PROPHYSICS_API void ProPhysics_Apply_SU2_Tick(ProUniverse* pu, uint32_t dt_q15);
```

Führt **einen vollständigen Leapfrog-Schritt** aus (E-Kick,
Link-Update, E-Kick). No-Op, wenn
`su2_dynamics_active == 0`, `dt_q15 == 0` oder
`su2_yang_mills_q15 == 0`.

### §5.4 — Action

```c
PROPHYSICS_API double ProPhysics_SU2_Plaquette_Action(const ProUniverse* pu);
```

Summe der Wilson-Plaquette-Action über alle Plaquettes:
`Σ (1 − Re(a)/2^30)`. Rein lesend.

### §5.5 — Energie

```c
PROPHYSICS_API double ProPhysics_SU2_Total_Energy(const ProUniverse* pu);
```

`H = S_plaq + (1/2) Σ |E|²`. Rein lesend.

### §5.6 — Link-lokale Action

```c
PROPHYSICS_API double ProPhysics_SU2_Link_Plaquette_Sum(
    const ProUniverse* pu, uint64_t x, uint8_t mu);
```

Summe der Wilson-Action aller Plaquettes, die `(x, μ)` enthalten.
In 3D bis zu vier Beiträge:

- `W_{α,β}(x)` für `{α,β} = sort({μ, ν})`
- `W_{α,β}(x − e_ν)`

**Metropolis-Nutzung:** `dS = Link_Plaquette_Sum(neu) −
Link_Plaquette_Sum(alt)` in einem Aufruf pro Update.

Rein lesend.

---

## §6 — Verwendungsmuster

### §6.1 — Dynamik aktivieren

```c
ProPhysics_Set_SU2_Yang_Mills(&pu, /*g²_q15*/ 16384);  /* ≈ 0.5 */
ProPhysics_Enable_SU2_Dynamics(&pu);

ProPhysics_Apply_SU2_Tick(&pu, /*dt_q15*/ 327);  /* ≈ 0.01 */
/* E und U wurden um einen Leapfrog-Schritt integriert */
```

### §6.2 — Plaquette-Action eines Zustands

```c
const double s_plaq = ProPhysics_SU2_Plaquette_Action(&pu);
/* Summe über alle Plaquettes; 0.0 für Identitäts-Konfiguration */
```

### §6.3 — Gesamt-Energie

```c
const double H = ProPhysics_SU2_Total_Energy(&pu);
/* H = S_plaq + 0.5 * Σ |E|² */
```

### §6.4 — Link-lokale Action (Metropolis)

```c
/* Vor dem Link-Update */
const double s_old = ProPhysics_SU2_Link_Plaquette_Sum(&pu, x, mu);

/* Testkonfiguration setzen (Prophysics_Set_Edge_SU2) */

/* Nach dem Link-Update */
const double s_new = ProPhysics_SU2_Link_Plaquette_Sum(&pu, x, mu);

const double dS = s_new - s_old;
/* Metropolis-Akzeptanz: exp(-g² * dS) */
```

### §6.5 — Energie-Erhaltung prüfen

```c
const double h0 = ProPhysics_SU2_Total_Energy(&pu);
for (int i = 0; i < 1000; ++i) {
    ProPhysics_Apply_SU2_Tick(&pu, 327);
}
const double h1 = ProPhysics_SU2_Total_Energy(&pu);
/* |h1 - h0| sollte klein sein (O(dt²) pro Schritt, kein Drift) */
```

---

## §7 — Fallstricke

### §7.1 — Nur Vorwärts-Kanäle

Link-Felder und E-Felder werden **nur** auf den Vorwärts-Kanälen
`{X_PLUS, Y_PLUS, Z_PLUS}` geführt. Rückwärts-Kanten sind
dieselben physischen Links wie die Vorwärts-Kanten am
Nachbarknoten (bis auf das Vorzeichen).

**Konsequenz:** `Get_Edge_SU2(pu, x, X_MINUS)` liefert **nicht**
den Link `(x → x−e_x)`, sondern einen unabhängigen Slot.
Die Physik nutzt nur Vorwärts-Kanten.

### §7.2 — `su2_dynamics_active` und `su2_active` sind verschieden

- `su2_active` (in `SU2.c` gesetzt): Link-Felder sind belegt.
- `su2_dynamics_active`: Leapfrog-Tick ist aktiv.

Ein Zustand kann `su2_active == 1, su2_dynamics_active == 0` haben
(Links gesetzt, aber keine Zeitentwicklung).

### §7.3 — `g² == 0` deaktiviert den Tick

`Apply_SU2_Tick` kehrt früh zurück, wenn `su2_yang_mills_q15 == 0`.
Das ist die semantisch korrekte Grenze (keine Kraft ⇒ keine
Bewegung), aber überraschend, wenn man `Enable` gesetzt hat und
dann nichts passiert.

### §7.4 — `dt_q15 == 0` ist No-Op

Kein Fehler, einfach keine Integration.

### §7.5 — Leapfrog-Reihenfolge ist bindend

Die Reihenfolge E-Kick, Link-Update, E-Kick ist die
Stoermer-Verlet-Form. Wer sie umdreht, bricht die Symplektizität
(Energie-Drift statt O(dt²)).

### §7.6 — `su2_plaquette_action_at` liest nur Vorwärts-Links

Alle vier Links werden als Vorwärts-Kanten gelesen. Die
Backward-Staple in `su2_force_on_link` nutzt die `nu_rev`-Kanäle
nur zum **Adressieren** der Vorwärts-Kanten an Nachbarknoten.

### §7.7 — `Link_Plaquette_Sum` ist keine reine Funktion der Kante

Die zurückgegebene Action hängt von **allen vier** Links jeder
Plaquette ab, nicht nur von `(x, μ)`. Ein Link-Update ändert nur
`(x, μ)`, aber die Summe berücksichtigt die Nachbarn.

**Konsequenz:** In Metropolis korrekt — die Nachbarn sind während
des Updates fix.

### §7.8 — Kein Cache

`SU2_Plaquette_Action` iteriert jedes Mal über alle Knoten und
alle Plaquette-Richtungen. Kein Caching. Bei `dim = 64` und
262k Knoten ist das spürbar.

### §7.9 — Division in `su2_force_on_link`

`acc_ai / n` ist eine **Ganzzahl-Division** (abgerundet). Bei
negativen Akkumulatoren rundet C gegen Null.

**Konsequenz:** Die Kraft ist nicht exakt symmetrisch. Für
Leapfrog ist das akzeptabel (symplektische Integratoren tolerieren
Rundung), aber die Energie-Erhaltung ist nicht mehr exakt.

### §7.10 — `su2_E_*` werden nicht automatisch initialisiert

`Set_Edge_SU2` setzt **nur** die Link-Felder, nicht die E-Felder.
Die E-Felder sind zu Beginn 0 (durch `ProUniverse`-Init) und
werden vom ersten E-Kick gesetzt.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| SU(2)-Modul | `docs/project/SU2.md` |
| U(1)-Gauge | `docs/project/Gauge.md` |
| Shared-Modul | `docs/project/Shared.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/kernel/ProPhysics_SU2_Dynamics.c` |
| Interne API | `ProPhysics_Internal.h` |

---

**Ende SU2_Dynamics.md v1.0.**