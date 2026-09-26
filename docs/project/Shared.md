# ProPhysics — Modul-Referenz: Shared

**Datei:** `docs/project/Shared.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_Shared.c`
**Zweck:** Referenz für das Shared-Reference-Modul: Union-Find-basierte
Klassen auf `amp_grid`, Singlet-Korrelation, klassenweiser Tick.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Shared-Modul implementiert **Klassen** von Knoten, die denselben
Amplitudenzustand teilen („shared reference"). Es ist die Grundlage
für EPR-artige Korrelationen, Singlet-Zustände und klassenweise
Zeitentwicklung.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Klassen und Union-Find |
| §3 | Spin-Flip und Singlet |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/EPR.md` — EPR-Messvarianten (nutzt Shared)
- `docs/project/Core.md` — `amp_grid`, `reg_source`, `ur_grid`
- `docs/project/CONFIG.md` — Konstanten (`PRO_EPR_CHANNEL`, `PRO_DEPHASE_CHANNEL`)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das Shared-Modul behandelt den **Korrelations-Sektor** des Kernels.
Es gruppiert Knoten in **Klassen**, in denen alle Mitglieder denselben
Amplitudenzustand haben. Klassen sind nicht hierarchisch — sie werden
über ein Union-Find gehalten.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Sync** | `Shared_Sync` |
| **Klassenbildung** | `Entangle_Nodes`, `Entangle_Nodes_Singlet` |
| **Klassenauflösung** | `Dissociate_Node` |
| **Query** | `Is_Entangled`, `Get_Representative`, `Shared_Class_Count` |
| **Spin-Flip** | `Is_Spin_Flipped`, `Get_Node_Spin_View` |
| **Klassen-Tick** | `Shared_Tick_Reps` |

**Was das Modul nicht tut:**

- Keine Zeitentwicklung im engeren Sinn (Unitär-Steps auf `amp_grid`
  liegen in `Amp.c`).
- Keine EPR-Messung (die liegt in `EPR.c`).
- Keine Korrelations-Messung über mehrere Klassen (die liegt im
  Test-Harness).

**Aktivierung:** Keine. Das Modul wird **explizit** vom Nutzer
aufgerufen, nicht vom Kernel-Tick. `Shared_Tick_Reps` ist ein
klassenweiser Tick, nicht Teil des globalen `ProPhysics_Tick`.

---

## §2 — Klassen und Union-Find

### §2.1 — Klassen als Äquivalenz-Relation

Zwei Knoten `a` und `b` sind **in derselben Klasse**, wenn
`pro_shared_find_const(pu, a) == pro_shared_find_const(pu, b)`.

Der Repräsentant einer Klasse ist der Wurzelknoten des Union-Find.
Er ist **nicht** stabil über Aufrufe: `Dissociate_Node` kann die
Wurzel verschieben.

### §2.2 — `pu->shared.parent`

Das Union-Find wird in `pu->shared.parent` gehalten
(`uint64_t`-Array, `total_nodes` Einträge):

- `parent[k] == k` → `k` ist Wurzel.
- `parent[k] != k` → `k` zeigt auf einen anderen Knoten derselben
  Klasse.

`pu->shared.active` ist ein Flag: `0` heißt, kein Knoten ist
entangled (`parent[k] == k` für alle `k`).

### §2.3 — Pfadkompression

`pro_shared_find` (mutierend) komprimiert den Pfad — nach dem Aufruf
zeigt jeder durchlaufene Knoten direkt auf die Wurzel.

`pro_shared_find_const` (lesend) komprimiert **nicht** — die
read-only Query-Funktionen (`Is_Entangled`, `Get_Representative`,
`Shared_Class_Count`) dürfen `pu->shared.parent` nicht ändern.

**Konsequenz:** Die Query-Funktionen sind `O(Tiefe)` statt
`O(α(N))`, aber sie sind rein lesend.

### §2.4 — Copy-on-Sync

`Shared_Sync` kopiert für jeden Non-Root `k` die Amplitude vom
Repräsentanten:

```
amp_grid[k] = amp_grid[find(k)]
```

**Richtung:** Root → Non-Root. Der Root ist die „Quelle der Wahrheit"
einer Klasse. Wer den Root mutiert, mutiert beim nächsten Sync die
ganze Klasse.

### §2.5 — Klassen-Tick

`Shared_Tick_Reps` läuft in drei Phasen:

1. **Sync** — alle Mitglieder auf Root-Zustand bringen.
2. **Paar-Sammlung** — Kanten zwischen verschiedenen Klassen zählen,
   dedupliziert nach `(rep_a, rep_b)` mit `rep_a < rep_b`.
3. **Paar-Rotation** — jeder Paar-Eintrag rotiert die beiden
   Repräsentanten in einem 2×2-Block um `theta_eff`, dann **Sync**.

`theta_eff = theta_rad · count`, geklemmt auf `π/2`. `count` ist die
Anzahl der Kanten zwischen den beiden Klassen.

---

## §3 — Spin-Flip und Singlet

### §3.1 — Kodierung

Spin-Flip wird in `ProNode.reserved_gating` Bit 0 kodiert
(`PRO_NODE_SPIN_FLIP_BIT`). Kein Struct-Change — Bit 0 war zuvor
ungenutzt.

### §3.2 — Lesen (`Get_Node_Spin_View`)

```c
if (spin_flipped) {
    out_re_up   =  c_down;
    out_re_down = -c_up;
} else {
    out_re_up   =  c_up;
    out_re_down =  c_down;
}
```

`c_up = coeff[UR_POSITRON_CW]`, `c_down = coeff[UR_POSITRON_CCW]`.
Nur die Realteile werden exportiert.

### §3.3 — Singlet-Korrelation

`Entangle_Nodes_Singlet(a, b)` ruft `Entangle_Nodes(a, b)` auf und
markiert **`b`** als spin-flipped. Der Wurzelknoten `a` bleibt normal.

**Konsequenz:** Ein Singlet-Paar `(a, b)` hat in der `b`-Klasse einen
umgekehrten Spin-View. Das ist die Grundlage für die
Singlet-Korrelation in `EPR.c`.

---

## §4 — Interne Helfer

### §4.1 — `pro_node_spin_flipped` / `pro_node_set_spin_flip`

```c
static inline bool pro_node_spin_flipped(const ProUniverse* pu, uint64_t k);
static inline void pro_node_set_spin_flip(ProUniverse* pu, uint64_t k, int on);
```

Bit-0-Zugriff auf `ProNode.reserved_gating`. Bounds- und
Null-Checks eingebaut.

### §4.2 — `pro_shared_find` / `pro_shared_find_const`

```c
static uint64_t pro_shared_find(ProUniverse* pu, uint64_t k);
static uint64_t pro_shared_find_const(const ProUniverse* pu, uint64_t k);
```

Union-Find. `_find` (mutierend) komprimiert Pfade, `_find_const`
(lesend) nicht.

### §4.3 — `pro_shared_pair_rotate_q31`

```c
static inline void pro_shared_pair_rotate_q31(
    ProAmpQ31* a, ProAmpQ31* b, double c, double s);
```

2×2-Komplex-Rotation:

```
a' = c*a + i*s*b
b' = c*b + i*s*a
```

**Ersetzt** die inline-Form in `Shared_Tick_Reps`.

**Anmerkung:** Dieselbe Matrix-Form existiert in `ProPhysics_Fock.c`
(`Apply_Hopping`) mit `s_signed = ±s`. Eine spätere Etappe kann den
Helfer nach `ProPhysics_Internal.h` verschieben, sodass `Fock.c` und
`SU2.c` ihn teilen.

---

## §5 — Öffentliche API

### §5.1 — Sync

```c
PROPHYSICS_API void ProPhysics_Shared_Sync(ProUniverse* pu);
```

Synchronisiert alle Non-Roots auf ihre Wurzel. No-Op, wenn
`pu->shared.active == 0`.

### §5.2 — Entangle / Dissociate

```c
PROPHYSICS_API bool ProPhysics_Entangle_Nodes(
    ProUniverse* pu, uint64_t a, uint64_t b);
```

Vereinigt die Klassen von `a` und `b`. `a` wird zur Wurzel der neuen
Klasse. Rückgabe `false` bei ungültigen Argumenten.

```c
PROPHYSICS_API bool ProPhysics_Entangle_Nodes_Singlet(
    ProUniverse* pu, uint64_t a, uint64_t b);
```

Wie `Entangle_Nodes`, markiert danach `b` als spin-flipped.

```c
PROPHYSICS_API bool ProPhysics_Dissociate_Node(ProUniverse* pu, uint64_t a);
```

Löst `a` aus seiner Klasse. Drei Fälle:

1. `a` ist nicht Wurzel → `a` wird eigene Klasse.
2. `a` ist Wurzel einer Einzelklasse → nur Spin-Flip-Reset.
3. `a` ist Wurzel einer Mehrklasse → erster direkter Nachfolger
   wird neue Wurzel, Spin-Flip von `a` zurückgesetzt.

### §5.3 — Query

```c
PROPHYSICS_API bool ProPhysics_Is_Entangled(
    const ProUniverse* pu, uint64_t a, uint64_t b);

PROPHYSICS_API uint64_t ProPhysics_Get_Representative(
    const ProUniverse* pu, uint64_t a);

PROPHYSICS_API uint64_t ProPhysics_Shared_Class_Count(
    const ProUniverse* pu);
```

`Is_Entangled` = „gleiche Klasse". `Get_Representative` = Wurzel.
`Class_Count` = Anzahl der Wurzeln (= Klassen).

### §5.4 — Spin-Flip-Query

```c
PROPHYSICS_API bool ProPhysics_Is_Spin_Flipped(
    const ProUniverse* pu, uint64_t k);

PROPHYSICS_API bool ProPhysics_Get_Node_Spin_View(
    const ProUniverse* pu, uint64_t k,
    int32_t* out_re_up, int32_t* out_re_down);
```

### §5.5 — Klassen-Tick

```c
PROPHYSICS_API void ProPhysics_Shared_Tick_Reps(
    ProUniverse* pu, uint32_t phase_step_q15);
```

Klassenweiser Tick. `phase_step_q15` ist **ungenutzt** (Reserve für
eine spätere Etappe); der aktuelle Tick nutzt einen festen
`PRO_DEFAULT_TRANSPORT_THETA_Q15`.

**Konsequenz:** Bei sehr vielen Klassen (viele Knoten, wenige
Kanten) kann die Paar-Sammlung `O(total_nodes · CHANNELS_MAX)`
dominieren; der Tick ist **nicht** für den Hotpath gedacht.

---

## §6 — Verwendungsmuster

### §6.1 — Zwei Knoten verheiraten

```c
ProPhysics_Entangle_Nodes(&pu, /*a*/ 0u, /*b*/ 512u);

const uint64_t rep = ProPhysics_Get_Representative(&pu, 512u);
/* rep = 0 (a ist Wurzel) */

const bool e = ProPhysics_Is_Entangled(&pu, 0u, 512u);
/* e = true */
```

### §6.2 — Singlet-Paar

```c
ProPhysics_Entangle_Nodes_Singlet(&pu, 0u, 512u);

const bool flipped = ProPhysics_Is_Spin_Flipped(&pu, 512u);
/* flipped = true */

int32_t up = 0, dn = 0;
ProPhysics_Get_Node_Spin_View(&pu, 512u, &up, &dn);
/* up, dn sind die umgekehrten Komponenten von Knoten 0 */
```

### §6.3 — Sync nach manueller Root-Mutation

```c
/* Root 0 mutieren (z.B. via ProPhysics_Apply_Global_Phase) */
ProPhysics_Apply_Global_Phase(&pu, /*phase*/ 32768u);

/* Alle Mitglieder der Klasse 0 nachziehen */
ProPhysics_Shared_Sync(&pu);
```

### §6.4 — Klasse auflösen

```c
/* Knoten 512 aus seiner Klasse lösen */
ProPhysics_Dissociate_Node(&pu, 512u);

const bool e = ProPhysics_Is_Entangled(&pu, 0u, 512u);
/* e = false */
```

### §6.5 — Klassen-Tick

```c
/* Ein Tick ueber alle Klassen-Paare */
ProPhysics_Shared_Tick_Reps(&pu, /*phase_step_q15*/ 0u);

/* Paare wurden rotiert, alle Mitglieder synchronisiert */
```

### §6.6 — Klassen zählen

```c
const uint64_t n_classes = ProPhysics_Shared_Class_Count(&pu);
/* n_classes = total_nodes, wenn nichts entangled ist */
```

---

## §7 — Fallstricke

### §7.1 — Root-Mutation + Sync

Wer einen Knoten mutiert, der **nicht** Root ist, verliert die
Mutation beim nächsten `Shared_Sync` (der Knoten wird auf den
Root-Zustand zurückgesetzt).

**Empfehlung:** Root mutieren, dann Sync.

### §7.2 — Repräsentant ist nicht stabil

`Get_Representative` liefert die aktuelle Wurzel. Nach
`Entangle_Nodes` oder `Dissociate_Node` kann das eine andere sein.
Wer den Repräsentanten cacht, muss ihn ggf. neu holen.

### §7.3 — `_find` mutiert, `_find_const` nicht

`pro_shared_find` komprimiert Pfade → ändert `pu->shared.parent`.
Deshalb ist die Query-API const: sie nutzt `pro_shared_find_const`
und komprimiert nicht.

**Konsequenz:** Wiederholte Query-Aufrufe können `O(Tiefe)`
kosten. Bei sehr tiefen Bäumen empfiehlt sich ein einmaliger
`Shared_Sync`-Aufruf (der intern `_find` nutzt und komprimiert).

### §7.4 — `Shared_Tick_Reps` ist `O(total_nodes · CHANNELS_MAX)`

Die Paar-Sammlung läuft über alle Knoten und alle Kanäle. Bei
`dim = 64` (262k Knoten) ist das spürbar. Der Tick ist **nicht**
für den Hotpath gedacht.

### §7.5 — `SHARED_MAX_PAIRS` ist eine harte Grenze

Die Paar-Sammlung fasst maximal `1024` verschiedene Paare. Bei mehr
Klassen-Paaren werden weitere Kanten **stillschweigend ignoriert**.

**Konsequenz:** Bei großen Gittern und vielen Entanglements kann
der Tick unvollständig wirken. `SHARED_MAX_PAIRS` in
`ProPhysics_Config.h` verschieben (Folge-Etappe).

### §7.6 — `phase_step_q15` ist ungenutzt

`ProPhysics_Shared_Tick_Reps` nimmt `phase_step_q15`, ignoriert
ihn aber. Die aktuelle Implementierung nutzt einen festen
`PRO_DEFAULT_TRANSPORT_THETA_Q15`.

**Empfehlung:** Bei nächster Gelegenheit entweder Parameter
nutzen oder aus der Signatur entfernen.

### §7.7 — `Entangle_Nodes` mit `a == b`

Liefert `true` ohne Wirkung. Kein Fehler, kein Zustandswechsel.

### §7.8 — Spin-Flip ist nur auf Non-Roots sinnvoll

`Entangle_Nodes_Singlet` markiert `b` — aber **nicht** die ganze
b-Klasse. Nach einem `Shared_Sync` werden nur die direkten
Mitglieder von `b` auf `b`'s Zustand gezogen; andere Mitglieder
der Klasse (über `b`'s Wurzel) verlieren das Flag.

**Konsequenz:** Singlet-Korrelation funktioniert nur bei
**paarweisen** Klassen (`a`-Wurzel, `b`-Blatt). Bei größeren
Klassen ist das Verhalten undefiniert.

### §7.9 — Dephasing-Kanal ausgenommen

`Shared_Tick_Reps` ignoriert `PRO_EPR_CHANNEL` und
`PRO_DEPHASE_CHANNEL`. Das verhindert, dass diese Kanäle
versehentlich Klassen-Kopplung erzeugen.

### §7.10 — `Dissociate_Node` verschiebt die Wurzel

Bei einer Mehrklasse wird der **erste gefundene direkte Nachfolger**
zur neuen Wurzel. Das ist deterministisch, aber nicht stabil über
Dissociate-/Entangle-Zyklen.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| EPR-Modul | `docs/project/EPR.md` |
| Gauge-Modul | `docs/project/Gauge.md` |
| Observer-Modul | `docs/project/Observer.md` |
| Core-Modul | `docs/project/Core.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/kernel/ProPhysics_Shared.c` |

---

**Ende Shared.md v1.0.**