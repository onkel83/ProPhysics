# ProPhysics — Modul: EPR

**Datei:** `docs/project/EPR.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_EPR.c`
**Zweck:** Referenz für das EPR-Modul: Paarmessungen, Kollaps-Varianten
und Apparat-Subgraph.

---

## §0 — Wie dieses Dokument zu lesen ist

Das EPR-Modul implementiert **vier Varianten** von Paarmessungen und
den **Apparat-Subgraph** für den Superdeterminismus-Test.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Die vier Messvarianten im Vergleich |
| §3 | Kollaps-Physik und U4 |
| §4 | Apparat-Subgraph |
| §5 | Interne Helfer |
| §6 | Öffentliche API |
| §7 | Verwendungsmuster |
| §8 | Fallstricke |
| §9 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Shared.md` — U4-Shared-Reference
- `docs/project/CONFIG.md` — Konstanten (`PRO_EPR_CHANNEL` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das EPR-Modul behandelt **verschränkte Knotenpaare** und
**Paarmessungen**. Es ist für drei Aufgaben zuständig:

| Aufgabe | Funktionen |
|---|---|
| **Paarmessungen** | 4 Varianten (siehe §2) |
| **Runtime-Steuerung** | `Set_EPR_Delay`, `Get_EPR_Delay`, `Set_EPR_Debug` |
| **Apparat-Subgraph** | `Init_Apparatus`, `Set_Apparatus_Phase`, `Read_Apparatus_Theta` |

**Was das Modul nicht tut:**

- **Keine Propagation.** Die EPR-Signallaufzeit (`pending_ticks`) wird
  in `ProPhysics_Core.c` (Kernel-Tick) behandelt, nicht hier.
- **Keine Kopplung an SU(2).** Die Kantenphase ist U(1).
- **Kein Sampling-Design.** Der Nutzer liefert den RNG-State.

---

## §2 — Die vier Messvarianten im Vergleich

| Variante | Guard | Relation | RNG? | Typische Anwendung |
|---|---|---|---|---|
| `Measure_EPR_Pair` | `type_state` + `edge->type` | — (sharp) | nein | Regression, `--test-chsh` |
| `Measure_EPR_Pair_Amp` | `amp_grid` (nicht-trivial) | — (sharp) | nein | Option-B-Konformität |
| `Measure_EPR_Pair_Collapse` | `type_state` + `edge->type` | `edge->type` | **ja** | CHSH mit Tsirelson (2√2) |
| `Measure_EPR_Pair_Graph` | `type_state` | `edge->phase` | **ja** | CHSH via U(1)-Connection |

### §2.1 — Sharp-Messung (Varianten 1 und 2)

**Physik:** Deterministische Projektion auf `sign(cos(θ − λ))`.

**Kein RNG.** Beide sharp-Varianten sind **deterministisch**.

**`Measure_EPR_Pair`** prüft `type_state` und `edge->type`.
**`Measure_EPR_Pair_Amp`** prüft nur, dass `amp_grid` an beiden Knoten
nicht trivial ist. Damit ist es Option-B-konform.

### §2.2 — Born-Kollaps (Varianten 3 und 4)

**Physik:** Born-Regel + U4-Kollaps.

1. A wird bei `θ_a` via Born-Regel gemessen: `P(+1) = cos²((θ_a − λ_a)/2)`.
2. A kollabiert auf `λ_a_collapsed ∈ {θ_a, θ_a + π}`.
3. B kollabiert mit: `λ_b_collapsed = λ_a_collapsed + phase_offset`.
4. B wird bei `θ_b` via Born-Regel auf dem kollabierten Zustand gemessen.

**`phase_offset`-Berechnung:**

| Variante | Phase Offset |
|---|---|
| `_Collapse` | `(edge->type == SINGLET) ? π : 0` |
| `_Graph` | `2π · edge->phase / 65536` |

**Beide nutzen RNG** (`pro_uniform01`) — der Nutzer liefert den
xoshiro256**-State als `uint64_t rng[4]`.

### §2.3 — Warum zwei Kollaps-Varianten?

- **`_Collapse`** nutzt die **Typ-Beziehung** (`edge->type`): Singlet
  ist antikorreliert, Triplet korreliert.
- **`_Graph`** nutzt die **Kantenphase** (`edge->phase`): Die
  Korrelation kommt aus der U(1)-Connection.

Beide ergeben `S = 2√2` unter den richtigen Bedingungen — das ist der
Tsirelson-Wert. Der Unterschied ist, **woher die Korrelationsrelation
kommt**.

---

## §3 — Kollaps-Physik und U4

### §3.1 — Was U4 fordert

U4 sagt: `Ψ(x,y) ⟺ A(x) = A(y)`. Zwei Knoten sind verschränkt, wenn
sie dieselbe Amplitude teilen (Union-Find-Klasse).

**Konsequenz für Messungen:** Wenn A kollabiert, kollabiert B **mit**.
Es gibt keine Signallaufzeit zwischen A und B.

### §3.2 — Wie die Kollaps-Relation entsteht

Die Relation `λ_b = λ_a + phase_offset` ist die **Projektion der
gemeinsamen Amplitude** auf die Kollapsachse. Je nach Singlet/Triplet:

| Typ | Relation | Physik |
|---|---|---|
| Singlet | `λ_b = λ_a + π` | Antikorrelation |
| Triplet | `λ_b = λ_a` | Korrelation |

### §3.3 — Was der Kollaps **nicht** tut

- **Er modifiziert `amp_grid` nicht.** Alle vier Messvarianten sind
  rein lesend.
- **Er ändert keine Topologie.** `reg_source` und `edge_phases`
  bleiben unverändert.
- **Er ist pro Knoten lokal.** Kein globaler Zustandsübergang.

### §3.4 — Tsirelson-Limit

Mit den Kollaps-Relationen ergibt CHSH den Wert `S = 2√2 ≈ 2,8284` —
das **Tsirelson-Limit**. Werte darüber sind nicht-physikalisch und
würden einen Fehler signalisieren.

---

## §4 — Apparat-Subgraph

### §4.1 — Zweck

Der Apparat-Subgraph ist **keine Physik**. Er ist eine
**Test-Infrastruktur**, um die geteilte Vergangenheit zwischen
Apparat und Teilchen **topologisch** zu kodieren.

**Wichtige Design-Entscheidung:** Der Apparat "berechnet" `θ_a` nicht
emergent. Der Nutzer setzt die Phase direkt (`Set_Apparatus_Phase`)
und liest sie zurück (`Read_Apparatus_Theta`).

### §4.2 — Topologie

`Init_Apparatus(pu, base_node, dim)` verdrahtet einen **2D-Torus**
der Größe `dim × dim`:

| Kanal | Ziel |
|:-:|---|
| 0 | `+y` Nachbar |
| 1 | `−y` Nachbar |
| 2 | `+x` Nachbar |
| 3 | `−x` Nachbar |
| 4..15 | Self-Loop |

### §4.3 — Phase setzen und lesen

`Set_Apparatus_Phase` schreibt `phase_fx` (Q16) in
`ur_grid[k].phase_accumulator` aller Apparat-Knoten.

`Read_Apparatus_Theta` liest den **Mittelwert** und konvertiert Q16
nach Radiant:

```
theta = (mean_fx / 65536) · 2π
```

**Wichtig:** Der Mittelwert läuft über `uint64_t` (Overflow-sicher
für bis zu `dim² = 2³²` Summanden bei `dim = 65536`).

### §4.4 — Wo der Apparat verwendet wird

Im Test `test_chsh_superdet` (Superdeterminismus-Test). Der Apparat
kodiert die Wahl von `θ_a` und `θ_b`. Die eigentliche Messung nutzt
`Measure_EPR_Pair_Amp`.

---

## §5 — Interne Helfer

### §5.1 — `pro_epr_validate_pair`

```c
static uint64_t pro_epr_validate_pair(
    const ProUniverse* pu,
    uint64_t node_a,
    int require_type_state,
    int require_edge_type,
    int require_amp_nonzero);
```

Pre-Flight-Check für eine EPR-Paarmessung. Liefert den `partner`-Index
bei Erfolg oder `UINT64_MAX` bei Fehler.

**Drei konfigurierbare Anforderungen:**

| Parameter | Prüft |
|---|---|
| `require_type_state` | `type_state != UR_NEUTRAL` an beiden Knoten |
| `require_edge_type` | `edge->type ∈ {SINGLET, TRIPLET}` |
| `require_amp_nonzero` | Mindestens ein `coeff[b] != 0` an beiden Knoten |

**Ersetzt** 3× Guard-Blöcke.

### §5.2 — `pro_epr_sharp_measure_pair`

```c
static void pro_epr_sharp_measure_pair(
    const ProAmpVector* va,
    const ProAmpVector* vb,
    double theta_a, double theta_b,
    int* out_a, int* out_b);
```

Wendet `pro_measure_sharp` auf beide Seiten an.

**Ersetzt** 2× identische Sequenz.

### §5.3 — `pro_epr_measure_born_collapse`

```c
static void pro_epr_measure_born_collapse(
    const ProAmpVector* va,
    double theta_a, double theta_b,
    double phase_offset_rad,
    uint64_t rng[4],
    int* out_a, int* out_b);
```

Born-Collapse-Sequenz für ein EPR-Paar. Der Parameter
`phase_offset_rad` kodiert die B-Kollaps-Relation.

**Ersetzt** 2× Born-Collapse-Sequenz.

---

## §6 — Öffentliche API

### §6.1 — Messfunktionen

| Funktion | Guard | RNG | Relation |
|---|---|:-:|---|
| `Measure_EPR_Pair` | type_state + edge_type | nein | sharp |
| `Measure_EPR_Pair_Amp` | amp_grid non-zero | nein | sharp |
| `Measure_EPR_Pair_Collapse` | type_state + edge_type | **ja** | edge->type |
| `Measure_EPR_Pair_Graph` | type_state | **ja** | edge->phase |

**Gemeinsame Signatur-Form:**

```c
PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_XXX(
    ProUniverse* pu,
    uint64_t node_a,
    double   theta_a,
    double   theta_b,
    [uint64_t rng[4],]      /* nur bei Kollaps-Varianten */
    int* out_a, int* out_b);
```

**Rückgabe:** `1` bei Erfolg, `0` bei Fehler.

**`out_a`, `out_b`:** Werte `+1` oder `−1`.

### §6.2 — Runtime-API

| Funktion | Zweck |
|---|---|
| `Set_EPR_Delay(pu, delay)` | Verzögerung in Ticks |
| `Get_EPR_Delay(pu)` | Aktuelle Verzögerung lesen |
| `Set_EPR_Debug(pu, level)` | Debug-Level (0 = aus) |

**`delay == 0`:** Signal kommt im **nächsten** Tick an (nicht sofort).
**`delay == N`:** Signal kommt N+1 Ticks später an.

### §6.3 — Apparat-Funktionen

| Funktion | Zweck |
|---|---|
| `Init_Apparatus(pu, base, dim)` | 2D-Torus verdrahten |
| `Set_Apparatus_Phase(pu, base, dim, phase_fx)` | Phase auf alle Apparat-Knoten schreiben |
| `Read_Apparatus_Theta(pu, base, dim)` | Mittelwert als Radiant lesen |

---

## §7 — Verwendungsmuster

### §7.1 — Sharp-Messung (deterministisch)

```c
int a = 0, b = 0;
if (ProPhysics_Measure_EPR_Pair(&pu, node_a,
    0.0, M_PI / 4.0, NULL, &a, &b)) {
    /* a, b ∈ {+1, -1} */
}
```

### §7.2 — Kollaps-Messung mit RNG

```c
uint64_t rng[4] = { 1, 2, 3, 4 };
int a = 0, b = 0;
if (ProPhysics_Measure_EPR_Pair_Collapse(&pu, node_a,
    0.0, M_PI / 4.0, rng, &a, &b)) {
    /* a, b sind Born-sampled */
}
```

### §7.3 — CHSH-Sweep

```c
const double thA[2] = { 0.0, M_PI / 2.0 };
const double thB[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };

double E[2][2];
for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
        double sum = 0.0;
        uint32_t count = 0;
        for (uint32_t t = 0; t < 1500; ++t) {
            int a = 0, b = 0;
            if (ProPhysics_Measure_EPR_Pair_Collapse(&pu, node_a,
                thA[i], thB[j], rng, &a, &b)) {
                sum += (double)(a * b);
                count++;
            }
        }
        E[i][j] = count ? sum / count : 0.0;
    }
}

const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
/* S ≈ 2 * sqrt(2) */
```

### §7.4 — Apparat für Superdeterminismus-Test

```c
ProPhysics_Init_Apparatus(&pu, 0, 16);
ProPhysics_Set_Apparatus_Phase(&pu, 0, 16, 32768);   /* π/2 in Q16 */

const double theta = ProPhysics_Read_Apparatus_Theta(&pu, 0, 16);
/* theta ≈ π/2 */
```

### §7.5 — Kollaps mit Kantenphase

```c
/* Setze die Kantenphase zwischen zwei EPR-Partnern. */
ProPhysics_Set_Edge_Phase(&pu, node_a, PRO_EPR_CHANNEL,
    M_PI, PRO_EDGE_NONE);

/* Messung nutzt jetzt die Phase, nicht den Typ. */
int a = 0, b = 0;
ProPhysics_Measure_EPR_Pair_Graph(&pu, node_a,
    0.0, M_PI / 4.0, rng, &a, &b);
```

---

## §8 — Fallstricke

### §8.1 — Sharp-Messungen ignorieren RNG

`Measure_EPR_Pair` und `Measure_EPR_Pair_Amp` sind **deterministisch**.
Der `rng`-Parameter von `_Pair` wird **nicht verwendet** (bleibt für
API-Kompatibilität).

### §8.2 — `_Graph` liest `e->type` nicht

`Measure_EPR_Pair_Graph` prüft **nicht**, ob `e->type` gesetzt ist.
Nur die Phase zählt. Wenn `e->phase == 0`, ist das eine **Triplet-artige**
Relation.

### §8.3 — Kollaps modifiziert nichts

Alle vier Messvarianten sind **rein lesend**. Sie ändern weder
`amp_grid` noch Topologie.

**Wenn Kollaps persistieren soll:** Der Nutzer muss die Amplituden
selbst setzen (`Set_Node_Amplitude`).

### §8.4 — RNG-State wird inkrementell verändert

`pro_uniform01(rng)` **modifiziert** den `rng`-State. Nach einem
Kollaps ist der State nicht mehr derselbe wie vorher.

**Konsequenz:** Für reproduzierbare Trials muss der RNG-State
**explizit** vor jedem Trial gesetzt werden.

### §8.5 — Delay = 0 ≠ sofort

Ein Signal mit `delay = 0` wird im **nächsten** Tick ausgeliefert, nicht
im gleichen. Grund: Der Tick verarbeitet `pending_ticks > 0` erst nach
dem Enqueue.

### §8.6 — Apparat `dim` muss Zweierpotenz sein?

**Nein.** Der Apparat nutzt einen **modulo-basierten** Torus
(`(y == 0 ? dim - 1 : y - 1)`), nicht die Shift/Mask-Konvention.
Jede `dim ≥ 2` funktioniert.

### §8.7 — Apparat-Phase ist Q16

`phase_fx` ist **Q16**, nicht Q15. Der Wertebereich ist
`[0, 65535]`, was `[0, 2π)` entspricht.

**Verwechslungsgefahr:** `phase_q15` in Gauge-Modul ist Q15, nicht Q16.

### §8.8 — Edge-Type-Guard in `_Collapse`

`_Collapse` prüft `edge->type ∈ {SINGLET, TRIPLET}` **vor** der Messung.
Wenn `edge->type == PRO_EDGE_NONE`, wird die Messung abgelehnt.

---

## §9 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Shared-Reference (U4) | `docs/project/Shared.md` |
| Gauge (U(1)-Connection) | `docs/project/Gauge.md` |
| Core-Modul | `docs/project/Core.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/ProPhysics_EPR.c` |

---

**Ende EPR.md v1.23.0.**