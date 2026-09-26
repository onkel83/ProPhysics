# ProPhysics — Modul-Referenz: SU2

**Datei:** `docs/project/SU2.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_SU2.c`
**Zweck:** Referenz für das SU(2)-Eichfeld-Modul: Quaternion-Links,
Wilson-Loop, lokale Eichtransformation, Algebra-Verifikation.

---

## §0 — Wie dieses Dokument zu lesen ist

Das SU(2)-Modul implementiert die **nicht-abelsche Eichstruktur** des
Kernels als Quaternion-Links auf `ProEdge`. Es ist die
Verallgemeinerung des U(1)-Wilson-Loops (`Gauge.md`) auf SU(2).

**Aufbau dieses Dokuments:**

| Abschnitt | Inhalt |
|---|---|
| §1 | Was das Modul tut |
| §2 | Quaternion-Konvention und Skala 2^30 |
| §3 | Wilson-Loop und Eichinvarianz |
| §4 | Interne Helfer |
| §5 | Öffentliche API |
| §6 | Verwendungsmuster |
| §7 | Fallstricke |
| §8 | Siehe auch |

**Verwandte Dokumente:**

- `docs/project/ProPhysics_API.md` — vollständige Funktions-Referenz
- `docs/project/ARCHITECTURE.md` — Tick-Ablauf und Datenmodell
- `docs/project/Gauge.md` — U(1)-Eichstruktur (abelscher Fall)
- `docs/project/SU2_Dynamics.md` — SU(2)-Leapfrog-Dynamik
- `docs/project/CONFIG.md` — Konstanten (`PRO_SU2_SCALE` etc.)
- `CHANGELOG.md` — Änderungshistorie

---

## §1 — Was das Modul tut

Das SU(2)-Modul behandelt den **nicht-abelschen Eichsektor** des
Kernels. Es speichert SU(2)-Links als Quaternionen in `ProEdge.su2_*`
und stellt Wilson-Loops, lokale Eichtransformationen und eine
Algebra-Verifikation bereit.

**Verantwortungsbereiche:**

| Bereich | Funktionen |
|---|---|
| **Link-Lifecycle** | `Set_Edge_SU2`, `Set_Edge_SU2_AxisAngle`, `Get_Edge_SU2` |
| **Wilson-Loop** | `Wilson_Loop_SU2`, `Wilson_Loop_SU2_Trace` |
| **Lokale Eichung** | `Apply_Local_SU2_Gauge` |
| **Verifikation** | `Verify_SU2_Quaternion` |

**Was das Modul nicht tut:**

- Keine Leapfrog-Zeitentwicklung (die macht `SU2_Dynamics.c`).
- Keine Wilson-Action/Plaquette-Summe (in `SU2_Dynamics.c`).
- Keine Metropolis/HMC-Sampling (im Test-Harness).
- Keine Modifikation von `amp_grid`.

**Aktivierung:** Keine explizite Aktivierung nötig — der erste
`Set_Edge_SU2`-Aufruf setzt `pu->su2_active = 1`. Vorher ist der
SU(2)-Pfad inaktiv, und die `su2_*`-Felder sind 0.

---

## §2 — Quaternion-Konvention und Skala 2^30

### §2.1 — Repräsentation

Ein SU(2)-Link ist eine 2×2-unitäre Matrix der Form

```
U = [ a   b  ]       mit |a|² + |b|² = 1
    [-b*  a* ]
```

mit `a = a_re + i·a_im`, `b = b_re + i·b_im` in `int32_t` pro
Komponente.

### §2.2 — Skala 2^30 (nicht 2^31)

`PRO_SU2_SCALE = 2^30` (nicht 2^31).

**Grund:** Bei Skala 2^31 kann die Summe der vier Produktterme in
`a_new = a1·a2 - b1* · b2` bis `|2^62| + |2^62| = 2^63` gehen —
int64-Grenze. Bei 2^30 bleibt der Zwischenwert `≤ 4·2^60 = 2^62`
(sicher).

**Konsequenz:** Die Normierung ist `|a|² + |b|² = PRO_SU2_NORM = 2^60`.

### §2.3 — Speicherort

Links werden in `ProEdge.su2_a_re`, `su2_a_im`, `su2_b_re`, `su2_b_im`
gehalten (`int32_t` pro Komponente, 16 Bytes pro Link zusätzlich).
Siehe `ProPhysics_Types.h` für das Layout von `ProEdge`.

### §2.4 — `pro_su2_mul`, `pro_su2_conj`, `pro_su2_norm_sq`

Zentrale Arithmetik-Helfer sind in `ProPhysics_Internal.h` als
`static inline` deklariert (seit Etappe 22b), damit auch
`SU2_Dynamics.c` und `Amp.c` sie nutzen können.

---

## §3 — Wilson-Loop und Eichinvarianz

### §3.1 — Formel

Für einen geschlossenen Pfad `(x_0, c_0), …, (x_{n-1}, c_{n-1})`:

```
W(C) = U_0 · U_1 · … · U_{n-1}
```

mit `U_k = Link(path_nodes[k] → path_nodes[k+1 mod n])`.

**Vorwärts-Iteration.** Die `pro_su2_mul`-Akkumulation läuft
`k = 0, 1, …, n-1` mit `acc = acc · U_k`.

### §3.2 — Eichinvarianz

Unter `Apply_Local_SU2_Gauge(λ)` gilt:

```
U(x→y) ← g(x) · U(x→y) · g(y)†
```

Die inneren g-Faktoren auf einem geschlossenen Pfad heben sich auf:

```
W' = g(x_0) · W · g(x_0)†
```

**Konsequenz:** `Tr(W') = Tr(W)` (Spur-Invarianz). Das ist der
Test `test_su2_wilson_loop`.

**Wichtig:** Die Vorwärts-Reihenfolge ist **notwendig** für die
Eichinvarianz. Die frühere Rückwärts-Iteration (`U_{n-1}·…·U_0`)
war nicht eichinvariant, weil die inneren g-Faktoren dann nicht
paarweise invers sind.

### §3.3 — Trace

`Wilson_Loop_SU2_Trace` liefert `Tr(W) = 2·Re(a)`, normiert auf
`PRO_SU2_SCALE` (also `1.0 = Identität`).

Die Komponenten `a_im`, `b_re`, `b_im` werden verworfen.

### §3.4 — Pfad-Validierung

`pro_wilson_validate_path` (in `ProPhysics_Internal.h`, geteilt mit
`Gauge.c`) prüft:

- `pu`, `path_nodes`, `path_channels` nicht NULL,
- `path_len > 0`,
- Knoten-Indizes `< total_nodes`,
- Kanal-Indizes `< CHANNELS_MAX`.

Bei ungültigem Pfad liefert `Wilson_Loop_SU2` `false` und die
Ausgabeparameter bleiben unangetastet.

---

## §4 — Interne Helfer

### §4.1 — `pro_su2_normalize`

```c
static bool pro_su2_normalize(
    int32_t* a_re, int32_t* a_im, int32_t* b_re, int32_t* b_im);
```

Normiert ein Quaternion auf `|a|² + |b|² = PRO_SU2_NORM = 2^60`.

- Wenn Norm `0`: degeneriert → auf Identität zurücksetzen, `false`.
- Wenn Norm schon in `[2^60 - 4, 2^60 + 4]`: nichts tun, `true`.
- Sonst: Skalierung per `double` (nicht im Hotpath), `false`.

### §4.2 — `pro_su2_edge_mut` / `pro_su2_edge`

```c
static ProEdge*       pro_su2_edge_mut(ProUniverse* pu, uint64_t src, uint8_t ch);
static const ProEdge* pro_su2_edge(const ProUniverse* pu, uint64_t src, uint8_t ch);
```

Bounds-Checks und Rückgabe des Link-Slots
`edge_phases[src·CHANNELS_MAX + ch]`. `NULL` bei ungültigen Argumenten.

**Hinweis:** const/non-const-Paar; kann in C nicht sauber
zusammengeführt werden.

### §4.3 — `pro_su2_axis_angle_to_quat`

```c
static void pro_su2_axis_angle_to_quat(
    double nx, double ny, double nz, double alpha,
    int32_t* a_re, int32_t* a_im, int32_t* b_re, int32_t* b_im);
```

Konvertiert Achse `(nx, ny, nz)` und Winkel `alpha` (Radiant) in
ein Q30-Quaternion:

```
U = cos(alpha/2)·I − i·sin(alpha/2)·(n·σ)
a = cos(alpha/2) − i·sin(alpha/2)·nz
b = −sin(alpha/2)·(ny + i·nx)
```

Achse muss **normiert** sein — der Aufrufer ist verantwortlich.

**Ersetzt** 6× inline-Konvertierung in `pro_su2_verify_product` und
1× in `Set_Edge_SU2_AxisAngle`.

### §4.4 — `pro_su2_verify_product` / `pro_su2_verify_pauli`

Modul-private Verifikationshelfer. Siehe §5.4.

---

## §5 — Öffentliche API

### §5.1 — Link-Lifecycle

```c
PROPHYSICS_API bool ProPhysics_Set_Edge_SU2(
    ProUniverse* pu, uint64_t src, uint8_t ch,
    int32_t a_re_q30, int32_t a_im_q30,
    int32_t b_re_q30, int32_t b_im_q30);
```

Setzt einen Link aus vier Q30-Komponenten. Normalisiert automatisch
auf `PRO_SU2_NORM`. Setzt `pu->su2_active = 1`.

```c
PROPHYSICS_API bool ProPhysics_Set_Edge_SU2_AxisAngle(
    ProUniverse* pu, uint64_t src, uint8_t ch,
    double nx, double ny, double nz, double alpha);
```

Convenience-Variante: Achse + Winkel. Normalisiert die Achse
zuerst; bei `|(nx,ny,nz)| < 1e-12` Rückgabe `false`.

```c
PROPHYSICS_API bool ProPhysics_Get_Edge_SU2(
    const ProUniverse* pu, uint64_t src, uint8_t ch,
    int32_t* out_a_re_q30, int32_t* out_a_im_q30,
    int32_t* out_b_re_q30, int32_t* out_b_im_q30);
```

Alle Ausgabeparameter sind optional (NULL erlaubt).

### §5.2 — Wilson-Loop

```c
PROPHYSICS_API bool ProPhysics_Wilson_Loop_SU2(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t*  path_channels,
    uint32_t        path_len,
    int32_t* out_a_re_q30, int32_t* out_a_im_q30,
    int32_t* out_b_re_q30, int32_t* out_b_im_q30);
```

Berechnet `W(C) = U_0·…·U_{n-1}`. Vorwärts-Iteration.

```c
PROPHYSICS_API double ProPhysics_Wilson_Loop_SU2_Trace(
    const ProUniverse* pu,
    const uint64_t* path_nodes,
    const uint8_t*  path_channels,
    uint32_t        path_len);
```

`Tr(W)/PRO_SU2_SCALE = 2·Re(a)/2^30`. `1.0 = Identität`.

### §5.3 — Lokale Eichung

```c
PROPHYSICS_API void ProPhysics_Apply_Local_SU2_Gauge(
    ProUniverse* pu, const int32_t* lambda_q30);
```

Führt `U(x→y) ← g(x)·U(x→y)·g(y)†` aus.

`lambda_q30` ist ein Array der Länge `total_nodes · 4`, mit der
Layout-Konvention `[a_re, a_im, b_re, b_im]` pro Knoten.

**Der Wilson-Loop ist invariant** (siehe §3.2).

### §5.4 — Verifikation

```c
PROPHYSICS_API double ProPhysics_Verify_SU2_Quaternion(void);
```

Prüft zwei Aussagen:

1. **Quaternion-Produkt = 2×2-Matrix-Multiplikation** in double.
2. **Pauli-Kommutator** `[i·σx, i·σy] = −2·i·σz`.

**Rückgabe:** Maximaler Fehler über beide Prüfungen. „Bestanden" bei
`< 1e-6` (im Test).

---

## §6 — Verwendungsmuster

### §6.1 — Link aus Achse + Winkel setzen

```c
/* Rotation um z-Achse um π/2 */
ProPhysics_Set_Edge_SU2_AxisAngle(&pu,
    /*src*/ 0u, /*ch*/ 0u,
    /*nx*/ 0.0, /*ny*/ 0.0, /*nz*/ 1.0,
    /*alpha*/ PRO_PI * 0.5);
```

### §6.2 — Link aus Q30-Komponenten setzen

```c
/* Identitaet */
ProPhysics_Set_Edge_SU2(&pu, 0u, 0u,
    PRO_SU2_IDENT_RE, PRO_SU2_IDENT_IM,
    0, 0);
```

### §6.3 — Link lesen

```c
int32_t a_re, a_im, b_re, b_im;
ProPhysics_Get_Edge_SU2(&pu, 0u, 0u, &a_re, &a_im, &b_re, &b_im);
```

### §6.4 — Wilson-Loop auf einem Quadrat

```c
const uint64_t nodes[4]    = { 0u, 1u, 2u, 3u };
const uint8_t  channels[4] = { 0u, 0u, 0u, 0u };

const double tr = ProPhysics_Wilson_Loop_SU2_Trace(
    &pu, nodes, channels, 4u);
/* tr = 2*Re(a)/2^30, 1.0 wenn Loop = Identitaet */
```

### §6.5 — Lokale Eichung

```c
/* g(x) als Q30-Array der Laenge total_nodes*4 vorbereiten */
int32_t* lambda = (int32_t*)calloc(pu.total_nodes * 4u, sizeof(int32_t));
/* ... lambda fuellen ... */

ProPhysics_Apply_Local_SU2_Gauge(&pu, lambda);

/* Spur ist invariant: test_su2_wilson_loop prueft das. */
```

### §6.6 — Algebra-Verifikation

```c
const double err = ProPhysics_Verify_SU2_Quaternion();
/* err < 1e-6  ->  Produkt + Pauli OK */
```

---

## §7 — Fallstricke

### §7.1 — Skala 2^30, nicht 2^31

Die Komponenten sind in `[-2^30, 2^30]`. Wer mit externen
Werten arbeitet, muss sie entsprechend skalieren (Skala 2^31
würde bei der Quaternion-Multiplikation überlaufen).

### §7.2 — Normalisierung ist Pflicht

`Set_Edge_SU2` normalisiert **automatisch** auf `PRO_SU2_NORM`.
Wer die rohen Komponenten aus `Get_Edge_SU2` liest, bekommt die
normierten Werte — nicht die Eingabewerte.

**Konsequenz:** `Set_Edge_SU2(…, 2^30, 0, 0, 0)` speichert
möglicherweise leicht andere Werte, wenn die Norm nicht exakt
`2^60` war.

### §7.3 — Auto-Aktivierung

Der erste `Set_Edge_SU2`-Aufruf setzt `pu->su2_active = 1`.
Vorher sind alle `su2_*`-Felder 0 (Identität entspricht nicht der
Null-Belegung!).

**Konsequenz:** `Wilson_Loop_SU2` vor der ersten Setzung liest
Nullen — das ergibt keinen sinnvollen Loop. Immer erst Setzen,
dann Lesen.

### §7.4 — Wilson-Loop ist vorwärts, nicht rückwärts

Die Iteration ist `W(C) = U_0·U_1·…·U_{n-1}` mit **`U_k` rechts**
an den Akkumulator. Wer die Reihenfolge umdreht, bricht die
Eichinvarianz (siehe §3.2).

### §7.5 — `Tr(U) = 2·Re(a)` — Vorzeichen

Die Normierung auf `PRO_SU2_SCALE` gibt `1.0` für die Identität
(`a = 2^30 + 0i`). Andere Phasen (z. B. `a = −2^30`) ergeben `−1.0`.

**Konsequenz:** `|Tr(U)|` kann als „Nähe zur Identität" interpretiert
werden, aber das Vorzeichen unterscheidet `U` von `−U`.

### §7.6 — `Apply_Local_SU2_Gauge` überschreibt `edge_phases`

Die Funktion überschreibt die `su2_*`-Felder der Kanten. Die
U(1)-Phase (`ProEdge.phase`) bleibt unberührt. Die beiden
Eichstrukturen sind orthogonal.

### §7.7 — `Apply_Local_SU2_Gauge` iteriert alle Knoten × alle Kanäle

Bei `dim = 64` (262k Knoten) × 16 Kanäle = 4M Iterationen mit je
zwei `pro_su2_mul`. Das ist **nicht** für den Hotpath gedacht.

### §7.8 — `pro_su2_axis_angle_to_quat` erwartet normierte Achse

Wer die Funktion direkt aufruft (statt über
`Set_Edge_SU2_AxisAngle`), muss die Achse normieren. Eine nicht
normierte Achse erzeugt ein nicht-unitäres Quaternion, das die
Normalisierung in `Set_Edge_SU2` nachträglich korrigiert —
aber die Physik ist dann eine andere als beabsichtigt.

### §7.9 — `lambda_q30`-Layout

`Apply_Local_SU2_Gauge` erwartet das Layout
`lambda_q30[x*4 + {0,1,2,3}] = {a_re, a_im, b_re, b_im}`.
Kein Bounds-Check auf `lambda_q30` — der Aufrufer muss
`total_nodes * 4` `int32_t` bereitstellen.

### §7.10 — Verifikation: Toleranz

`Verify_SU2_Quaternion` prüft **nicht** gegen `1e-6`, sondern
liefert nur den Fehler. Der Test-Harness wendet die Toleranz an.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Vollständige API-Referenz | `docs/project/ProPhysics_API.md` |
| Architektur und Tick-Ablauf | `docs/project/ARCHITECTURE.md` |
| U(1)-Gauge | `docs/project/Gauge.md` |
| SU(2)-Dynamik | `docs/project/SU2_Dynamics.md` |
| Shared-Modul | `docs/project/Shared.md` |
| Konstanten | `docs/project/CONFIG.md` |
| Änderungshistorie | `CHANGELOG.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Quelldatei | `src/prophysics/kernel/ProPhysics_SU2.c` |
| Interne API | `ProPhysics_Internal.h` |

---

**Ende SU2.md v1.0.**