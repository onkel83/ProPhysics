# ProPhysics — Modul: Core

**Datei:** `docs/project/Core.md`
**Version:** 1.23.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_Core.c`
**Zweck:** Referenz für das Core-Modul: Lifecycle, Topologie,
Tick-Orchestrierung.

---

## §0 — Wie dieses Dokument zu lesen ist

Das Core-Modul ist die **Basis-Schicht** des Kernels. Es verwaltet
den Lebenszyklus des `ProUniverse`, die Topologie und orchestriert
den Tick.

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut (Rolle im Kernel) |
| §2 | Datenfluss und Abhängigkeiten |
| §3 | Interne Helfer (static, nicht Teil der API) |
| §4 | Öffentliche API (Funktionen und Semantik) |
| §5 | Der Tick im Detail |
| §6 | Fallstricke |
| §7 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Amp.md` — Amp-Modul
- `docs/project/CONFIG.md` — Konstanten
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Core-Modul tut

Das Core-Modul ist die **unterste Schicht** des Kernels. Es ist für
drei Aufgaben zuständig:

| Aufgabe | Funktionen |
|---|---|
| **Lifecycle** | `Initialize`, `Free` |
| **Topologie** | `Link_Nodes`, `Unlink_Node`, `Set_Edge_Phase` |
| **Tick-Orchestrierung** | `ProPhysics_Tick`, `Advance_Internal_Clocks` |
| **Amp-Grid-Zugriff** | `Get_Amp_Grid`, `Set_Node_Amplitude`, `Get_Node_Amplitude`, `Sync_Amp_From_Type` |
| **Injektion** | `Inject_Momentum`, `Spawn_Body` |

**Was das Core-Modul nicht tut:**

- Keine unitäre Dynamik (`ProPhysics_Amp.c`).
- Keine EPR-Messungen (`ProPhysics_EPR.c`).
- Keine SU(2)-Links (`ProPhysics_SU2*.c`).
- Kein U6-Guiding (`ProPhysics_Amp.c`, `Apply_Guiding_Equation`).

**Rolle im Tick:** `ProPhysics_Tick` ist die **einzige öffentliche
Funktion**, die einen vollständigen Tick ausführt. Sie ruft alle
anderen Module in der richtigen Reihenfolge auf.

---

## §2 — Datenfluss und Abhängigkeiten

### §2.1 — Ein-/Ausgänge pro Funktion

| Funktion | liest | schreibt |
|---|---|---|
| `Initialize` | — | komplettes `ProUniverse` |
| `Free` | — | `memset(pu, 0)` |
| `Link_Nodes` | — | `reg_source` |
| `Unlink_Node` | — | `reg_source` |
| `Set_Edge_Phase` | — | `edge_phases.phase`, `.type` |
| `Inject_Momentum` | — | `ur_grid.momentum_phase` |
| `Spawn_Body` | — | `ur_grid`, `amp_grid` |
| `Set_Node_Amplitude` | — | `amp_grid.coeff[b]` |
| `Get_Node_Amplitude` | `amp_grid` | — |
| `Sync_Amp_From_Type` | `ur_grid` | `amp_grid` |
| `Advance_Internal_Clocks` | `ur_grid.momentum_phase` | `ur_grid.phase_accumulator`, `.field_helicity` |
| `Tick` | alle | alle |

### §2.2 — Was `Initialize` allokiert

| Array | Typ | Aligned? | Größe pro Knoten |
|---|---|:-:|---:|
| `ur_grid` | `ProNode` | ✅ | 8 B |
| `reg_source` | `ProRegister` | ✅ | 128 B |
| `reg_target` | `ProRegister` | ✅ | 128 B |
| `edge_phases` | `ProEdge` | ✅ | 640 B (`40 B × 16`) |
| `amp_grid` | `ProAmpVector` | ✅ | 64 B |
| `amp_scratch` | `ProAmpVector` | ✅ | 64 B |
| `u_field` | `uint16_t` | ✅ | 2 B |
| `shared.parent` | `uint64_t` | ✅ | 8 B |
| `tensor_pairs` | `ProAmpTensorPair` | ❌ | — (256 Slots) |
| `tensor_marks` | `uint8_t` | ❌ | 1 B |
| `fock_states` | `ProFockState` | ❌ | — (64 Slots) |
| `density_matrices` | `ProDensityMatrix` | ❌ | — (64 Slots) |
| `tensor_densities` | `ProTensorDensity` | ❌ | — (32 Slots) |
| `fock_densities` | `ProFockDensity` | ❌ | — (8 Slots) |

**Gesamt** für N Knoten: **~1042 B × N** + feste Slots.

### §2.3 — Abhängigkeiten zu anderen Modulen

| Ruft auf | aus Modul |
|---|---|
| `Apply_Amp_Step` | Amp |
| `Apply_Guiding_Equation` | Amp |
| `pro_amp_pack`, `pro_amp_real`, `pro_amp_imag` | Types |
| `pro_aligned_calloc`, `pro_aligned_free`, `pro_splitmix64` | Internal |
| `is_valid_state` | Internal |

---

## §3 — Interne Helfer

### §3.1 — Debug-Ringbuffer

Der Kernel schreibt EPR-Propagations-Events in einen **lock-freien
Single-Thread-Ringpuffer**. Kein `printf` im Hotpath.

```c
typedef struct {
    ProDebugEvent events[PRO_DEBUG_RING_SIZE];
    uint32_t      head;
    uint32_t      tail;
    uint64_t      dropped;
} ProDebugRing;
```

| Feld | Bedeutung |
|---|---|
| `events[]` | Ringpuffer (256 Slots default) |
| `head` | Nächster Schreibslot |
| `tail` | Nächster Leseslot |
| `dropped` | Anzahl verworfener Events (bei vollem Ring) |

**Funktionen:**

- `pro_debug_push(tick, src, dst, helicity, kind)` — schreibt ein Event
- `pro_debug_reset()` — leert den Ring

**Wichtig:** Der Ring ist global (`static ProDebugRing g_debug_ring`).
Bei parallelen `ProUniverse`-Instanzen werden Events gemeinsam
gespeichert. Das ist eine **Test-Infrastruktur**, kein API-Feature.

### §3.2 — `pro_registers_self_init`

```c
static void pro_registers_self_init(ProRegister* regs, uint64_t n);
```

Setzt alle Kanäle auf Self-Loop:
```c
regs[i].channels[c] = i;   /* fuer alle i, c */
```

**Verwendung:**

- `Initialize`: für `reg_source` und `reg_target`.
- `Tick`: Lazy-Init von `reg_target`, falls NULL.

**Ersetzt** zwei duplizierte Schleifen (D1 im Refactoring).

---

## §4 — Öffentliche API

### §4.1 — Lifecycle

#### `ProPhysics_Initialize`

```c
PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count);
```

**Zweck:** Allokiert und initialisiert ein Universum.

**Parameter:**

- `pu` — Zeiger auf ein vom Nutzer allokiertes `ProUniverse`
- `node_count` — Anzahl Knoten (> 0)

**Nebenwirkungen:**

- Alle Arrays werden allokiert.
- Register-Default: Self-Loop in allen Kanälen.
- `edge_phases[*].su2_*` = Identität, `su2_E_*` = 0.
- `shared.parent[k] = k`.
- `u_field[k]` = deterministischer splitmix64-Wert.
- Alle Aktivierungs-Flags = 0.

**Fehlerbehandlung:** Bei OOM oder Overflow wird `pu` auf 0 gesetzt
und die Funktion kehrt zurück. Nachfolgende API-Aufrufe liefern
`false`.

**Beispiel:**

```c
ProUniverse pu;
ProPhysics_Initialize(&pu, 64 * 64);
pu.grid_dim = 64;
pu.grid_ndim = 2;
```

**Wichtig:** `grid_dim` und `grid_ndim` werden **nicht** von
`Initialize` gesetzt. Der Nutzer muss sie explizit konfigurieren.

#### `ProPhysics_Free`

```c
PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu);
```

**Zweck:** Gibt alle Ressourcen frei. `memset(pu, 0)`.

**Idempotent:** Doppelter Aufruf ist safe.

### §4.2 — Topologie

#### `ProPhysics_Link_Nodes`

```c
PROPHYSICS_API void ProPhysics_Link_Nodes(ProUniverse* pu,
    uint64_t src, uint64_t target, uint8_t channel_idx);
```

**Zweck:** Setzt `reg_source[src].channels[channel_idx] = target`.

**Beispiel:** 2D-Torus verdrahten:

```c
for (uint32_t y = 0; y < DIM; ++y) {
    for (uint32_t x = 0; x < DIM; ++x) {
        const uint64_t k = y * DIM + x;
        ProPhysics_Link_Nodes(&pu, k, y * DIM + ((x + 1) % DIM),
            PRO_NEIGHBOR_X_PLUS);
        ProPhysics_Link_Nodes(&pu, k, y * DIM + ((x + DIM - 1) % DIM),
            PRO_NEIGHBOR_X_MINUS);
        ProPhysics_Link_Nodes(&pu, k, ((y + 1) % DIM) * DIM + x,
            PRO_NEIGHBOR_Y_PLUS);
        ProPhysics_Link_Nodes(&pu, k, ((y + DIM - 1) % DIM) * DIM + x,
            PRO_NEIGHBOR_Y_MINUS);
    }
}
```

#### `ProPhysics_Unlink_Node`

```c
PROPHYSICS_API void ProPhysics_Unlink_Node(ProUniverse* pu,
    uint64_t src, uint8_t channel_idx);
```

**Zweck:** Setzt Kanal auf Self-Loop (`src` → `src`).

#### `ProPhysics_Set_Edge_Phase`

```c
PROPHYSICS_API void ProPhysics_Set_Edge_Phase(ProUniverse* pu,
    uint64_t src, uint8_t ch, double phase_radians, uint8_t edge_type);
```

**Zweck:** Setzt die U(1)-Kantenphase und den Korrelationstyp.

**Parameter:**

| Parameter | Bedeutung |
|---|---|
| `phase_radians` | Phase in Radiant (wird auf `[0, 2π)` normalisiert) |
| `edge_type` | `PRO_EDGE_NONE`, `_SINGLET`, `_TRIPLET`, `_PRODUCT` |

**Wichtig:** Modifiziert **nur** `phase` und `type`. SU(2)-Felder
bleiben unangetastet.

### §4.3 — Amp-Grid-Zugriff

#### `ProPhysics_Get_Amp_Grid`

```c
PROPHYSICS_API ProAmpVector* ProPhysics_Get_Amp_Grid(ProUniverse* pu);
```

**Zweck:** Direkter Zeiger auf `amp_grid`. Nur lesend verwenden,
wenn der Kernel nicht umgangen werden soll.

#### `ProPhysics_Set_Node_Amplitude`

```c
PROPHYSICS_API bool ProPhysics_Set_Node_Amplitude(ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx,
    int32_t re_q31, int32_t im_q31);
```

**Zweck:** Setzt eine einzelne Basis-Amplitude.

**Q31-Konvention:** `INT32_MAX` entspricht physikalisch 1.0.

**Beispiel:**

```c
/* Knoten 0, Basis 1 (UR_POSITRON_CW), Wert (1, 0) */
ProPhysics_Set_Node_Amplitude(&pu, 0, UR_POSITRON_CW, INT32_MAX, 0);
```

#### `ProPhysics_Get_Node_Amplitude`

```c
PROPHYSICS_API bool ProPhysics_Get_Node_Amplitude(const ProUniverse* pu,
    uint64_t node_idx, uint8_t basis_idx,
    int32_t* out_re_q31, int32_t* out_im_q31);
```

**Zweck:** Liest eine einzelne Basis-Amplitude.

#### `ProPhysics_Sync_Amp_From_Type`

```c
PROPHYSICS_API void ProPhysics_Sync_Amp_From_Type(ProUniverse* pu);
```

**Zweck:** Setzt `amp_grid[k].coeff[type_state[k]] = 1.0`, nullt die
anderen Komponenten. Wird von `Initialize` aufgerufen.

**Verwendung:** Nach direktem Setzen von `type_state` (Test-Setup),
um `amp_grid` anzu-passen.

### §4.4 — Injektion

#### `ProPhysics_Inject_Momentum`

```c
PROPHYSICS_API void ProPhysics_Inject_Momentum(ProUniverse* pu,
    uint64_t node_idx, double velocity_ratio);
```

**Zweck:** Setzt `momentum_phase = round(velocity_ratio × 127)`.

**Bereich:** `velocity_ratio ∈ [0, 1]` wird geclamped.

**Wirkung:** Die innere Uhr läuft ab dem nächsten Tick langsamer
(Lorentz-Faktor).

#### `ProPhysics_Spawn_Body`

```c
PROPHYSICS_API void ProPhysics_Spawn_Body(ProUniverse* pu,
    uint64_t node_idx, uint8_t state, uint8_t helicity);
```

**Zweck:** Setzt `type_state`, `field_helicity`, nullt `amp_grid`
und setzt `coeff[state] = 1.0`.

### §4.5 — Uhr

#### `ProPhysics_Advance_Internal_Clocks`

```c
PROPHYSICS_API void ProPhysics_Advance_Internal_Clocks(ProUniverse* pu);
```

**Zweck:** Aktualisiert die innere Uhr pro Knoten.

**Physik:**

```
v = momentum_phase / 127
γ⁻¹ = √(1 − v²)
delta = round(256 · γ⁻¹)
phase_accumulator += delta
bei Überlauf: field_helicity toggelt
```

**Wird intern von** `ProPhysics_Tick` **aufgerufen.**

### §4.6 — Tick

#### `ProPhysics_Tick`

```c
PROPHYSICS_API void ProPhysics_Tick(ProUniverse* pu,
    ProPhysics_RuleCallback callback);
```

**Zweck:** Führt **einen vollständigen Tick** des Universums aus.

**Parameter:**

- `callback` — optionaler Plastizitäts-Callback (`NULL` erlaubt).

**Wichtig:** Dies ist die **einzige** Funktion, die einen kompletten
Tick ausführt. Details siehe §5.

---

## §5 — Der Tick im Detail

### §5.1 — Reihenfolge (8 Schritte)

```
ProPhysics_Tick(pu, callback)
    │
    ├─ (1) Lazy-Init reg_target
    │
    ├─ (2) current_cpu_tick++
    │
    ├─ (3) Advance_Internal_Clocks
    │
    ├─ (4) EPR-Propagation
    │
    ├─ (5) Apply_Amp_Step
    │
    ├─ (6) Apply_Guiding_Equation
    │
    ├─ (7) Graph-Plastizität (nur bei callback != NULL)
    │
    └─ (8) global_entropy_index
```

### §5.2 — Warum diese Reihenfolge?

**Uhr vor Dynamik:** Die innere Uhr modifiziert `field_helicity`,
was in der EPR-Propagation gelesen wird.

**EPR vor Amp-Step:** Die EPR-Propagation liefert Helizitäts-Änderungen
an Partner-Knoten. Diese beeinflussen die Dynamik erst im **nächsten**
Tick. Die Kausalität bleibt erhalten.

**Amp-Step vor Guiding:** Guiding liest `amp_grid` und schreibt
`type_state`. Es muss also **nach** der Dynamik laufen.

**Plastizität zuletzt:** Die Topologie-Änderung beeinflusst erst den
nächsten Tick. Sie muss nach der Dynamik laufen.

### §5.3 — Der Callback

**Wann wird er aufgerufen?**

Pro Knoten mit `type_state != UR_NEUTRAL` und pro Kanal, in dem der
Nachbar nicht NEUTRAL ist.

**Was darf er?**

- Die Kanäle in-place permutieren.
- `out_next_state` und `out_next_target_state` setzen (aktuell ignoriert).

**Was passiert mit seinen Änderungen?**

Nach dem Callback werden die mutierten Kanäle nach `reg_target`
geschrieben. Am Ende wird `reg_source ↔ reg_target` geswappt.

**Bei `callback == NULL`:**

- Kein Callback-Aufruf.
- Kein Swap.
- `global_entropy_index = 0`.
- Der Rest läuft **bit-identisch**.

### §5.4 — Beispiel: Null-Callback vs. No-Op-Callback

```c
/* Variante A: keine Topologie-Änderung */
ProPhysics_Tick(&pu, NULL);

/* Variante B: Callback, der nichts tut */
void no_op(uint8_t cs, uint8_t ts, uint64_t* cc, uint64_t* tc,
           uint8_t* ns, uint8_t* nts, uint64_t idx, uint64_t n) {
    (void)cs; (void)ts; (void)cc; (void)tc;
    (void)ns; (void)nts; (void)idx; (void)n;
}
ProPhysics_Tick(&pu, no_op);
```

**Beide Varianten liefern den gleichen `amp_grid`-Zustand.**

Der Unterschied: Variante B führt den Swap durch (Topologie wird
identisch kopiert), Variante A nicht. Das ist R7-konform.

---

## §6 — Fallstricke

### §6.1 — `grid_dim` muss **nach** `Initialize` gesetzt werden

```c
ProPhysics_Initialize(&pu, 64 * 64);   /* total_nodes = 4096 */
pu.grid_dim = 64;                       /* explizit setzen */
pu.grid_ndim = 2;
```

Ohne `grid_dim` verwendet `Apply_Amp_Step` den sequenziellen
Transport (Trotter-Fehler ~1,7e-4/Tick).

### §6.2 — `Free` ist idempotent, `Initialize` nicht

```c
ProPhysics_Free(&pu);       /* safe */
ProPhysics_Free(&pu);       /* safe (no-op) */

ProPhysics_Initialize(&pu, 1000);
ProPhysics_Initialize(&pu, 2000);   /* FREE + Neu-Init */
```

`Initialize` erkennt, ob `pu->_core_magic == PRO_CORE_MAGIC` und ruft
`Free` vor dem Neu-Init auf.

### §6.3 — Register-Zeiger nach `Tick` neu holen

Der Tick swappt `reg_source ↔ reg_target`. Wer einen Zeiger auf
`reg_source` gespeichert hat, muss ihn neu holen.

### §6.4 — `type_state` ist Anzeige

`type_state` ist eine **Anzeige** (U6), keine fundamentale Größe.
`Apply_Guiding_Equation` überschreibt es jeden Tick aus `amp_grid`.
Wer `type_state` direkt setzt und im nächsten Tick lesen will, muss
**sofort** lesen (vor dem nächsten Tick).

### §6.5 — EPR-Propagation-Delay

`Set_EPR_Delay(pu, 0)` bedeutet: Signal kommt im nächsten Tick an.
`Set_EPR_Delay(pu, 5)` bedeutet: 6 Ticks Verzögerung (Delay + 1).

### §6.6 — Debug-Ring ist global

Der Debug-Ring `g_debug_ring` ist `static` und **global**. Bei
mehreren `ProUniverse`-Instanzen werden alle Events gemeinsam
gespeichert. Das ist eine Test-Infrastruktur, kein API-Feature.

---

## §7 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| Amp-Modul | `docs/project/Amp.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/ProPhysics_Core.c` |

---

**Ende Core.md v1.23.0.**