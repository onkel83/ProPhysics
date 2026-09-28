# ProPhysics — Was uns unterscheidet

**Datei:** `docs/project/ProPhysics_Differentiators.md`
**Version:** 1.1
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-29 (Kernel 1.23.0, Konsolidierungs-Serie
1.23.1–1.23.14 abgeschlossen; Sampler-Korrektur Haar-Vorschlag;
V&V-Anker endgültig zurückgenommen)
**Zweck:** Erklärt sachlich, was ProPhysics anders macht als
etablierte Software, warum das wichtig ist, und welchen konkreten
Vorteil der aktuelle Stand bietet.

**Wichtig:** Dieses Dokument ist **kein Marketing**. Es beschreibt
die technische Architektur und den aktuellen Stand so, dass ein
Fachkollege die Unterschiede einordnen kann. Was ProPhysics **nicht**
ist, steht in §7 explizit drin.

---

## 1. Kurzfassung

ProPhysics ist ein **Simulations-Kernel für Quantenmechanik auf
diskreten Graphen**, der statt dichter unitärer Matrizen mit
**signed permutations** und **Q31-Integer-Arithmetik** arbeitet. Aus
fünf Urregeln entstehen emergente Phänomene wie Bloch-Dispersion,
Dirac-Struktur, Spin-1/2, fermionische Statistik und nicht-abelsche
SU(2)-Eichtheorie — ohne dass diese Strukturen explizit eincodiert
sind.

**Kernunterschied:** klassische Simulations-Frameworks (ITensor,
QuSpin, Qiskit) repräsentieren Zustände als dichte Vektoren und
Operatoren als dichte Matrizen. ProPhysics repräsentiert Operatoren
als **bijektive Indextausch-Operationen mit Vorzeichen** — ein
Modell, das speichereffizient, exakt (bis auf Q30-Rundung) und
cache-freundlich ist.

**Aktueller Validierungsstand:**

- **2D-SU(2)-Metropolis** reproduziert die **exakte Bessel-Ratio
  `I₂(β)/I₁(β)`** auf **< 1,2 %** bei vier β-Werten.
- **3D-SU(2)-String-Tension** zeigt streng monoton fallendes σ_a2 in β
  mit dim-Konvergenz < 0,5 % zwischen dim=16 und dim=32.
- **Kein externer V&V-Anker** für 3D — die publizierten Werte
  (Cahill & Prasad 1989) sind 4D, der Kernel ist 3D.

Der frühere V&V-Anker aus Etappe 23 („0,08 %") ist zurückgenommen:
Er war ein Artefakt eines **int32-Overflow-Bugs** im Metropolis-
Vorschlag `U → normalize(U + ε)` bei ε > 1.

---

## 2. Was ProPhysics anders macht

### 2.1 — Signed Permutations statt dichter Matrizen

**Klassisch:** Ein lokaler Operator ist eine 8×8-komplexe Matrix.
Multiplikation ist `O(N³)`, Speicher `O(N²)`.

**ProPhysics:** Ein lokaler Operator ist eine **Permutation** `perm[8]`
plus **Vorzeichen** `sign[8]`. Multiplikation ist `O(N)`, Speicher
`O(N)` (2 Bytes pro Basisindex).

**Konsequenz:** Der Context-Tick ist eine reine Indextausch-Operation.
Kein Multiplikations-Overflow, keine Rundung, exakt unitär. Der
Drift nach 10 000 Ticks ist **0,0**.

**Grenze:** Signed permutations sind eine Teilmenge der unitären
Gruppe. Sie können **nicht** jede unitäre Transformation darstellen.
Für die Transport-, Wave- und Context-Schritte reicht es aus (weil
diese Schritte aus signierten Permutationen zusammengesetzt sind),
für beliebige Unitarien nicht.

### 2.2 — Q31-Integer statt Floating Point im Hotpath

**Klassisch:** `std::complex<double>` oder `complex<float>`, jede
Operation ist eine IEEE-754-Gleitkomma-Operation.

**ProPhysics:** Komplexe Zahlen sind `int64`, Re/Im je `int32`
(Q31-Skala `INT32_MAX`). Addition ist Integer-Addition. Multiplikation
ist `int64`-Multiplikation mit Shift-um-31.

**Konsequenz:**
- **Determinismus:** Gleiche Eingabe ergibt gleiche Ausgabe, auf
  jeder x86-64-CPU. Keine `-ffast-math`-Abhängigkeiten.
- **Reproduzierbarkeit:** Test-Logs sind bit-identisch zwischen Läufen.
- **Portabilität:** Kein Unterschied zwischen SSE, AVX, ARM-NEON
  (nur Geschwindigkeit).
- **Kein `NaN`-Risiko:** Kein Unterlauf, kein Overflow in
  unendlich, alle Werte sind geschlossene Ganzzahlen.

**Grenze:** Dynamikbereich ist `[-2³¹, 2³¹)`. Bei sehr vielen
Rotationen akkumuliert Q30-Rundung (gemessen: `1,8e-08` Drift
über 100 Ticks für SU(2)-Link-Normen). Für die aktuelle
Testsuite unkritisch; für sehr lange Läufe (Millionen von Ticks)
müsste auf Q61 migriert werden (Etappe 27).

### 2.3 — Emergenz statt Konstruktion

**Klassisch:** Ein Framework für Quantenmechanik hat `expectation_value`,
`apply_operator`, `born_rule` als API. Der Nutzer ruft diese auf.

**ProPhysics:** Die API kennt **keine Born-Regel**. Sie kennt nur
`ProPhysics_Get_Born_Probability`, das eine **Projektion** berechnet.
Die Born-Verteilung **entsteht** aus dem Zusammenspiel von
Projektion, Nachbar-Amplituden und Tick-Iteration. Der Test
`test_born_emergent` misst χ²/dof = **9,89 / 7 = 1,41** — die
Born-Verteilung ist da, aber niemand hat sie programmiert.

**Konsequenz:** Der Kernel ist als **Ontologie** formulierbar, nicht
als Werkzeugkasten. Die Frage „Welche Regeln braucht man
mindestens?" wird empirisch beantwortbar.

**Grenze:** „Emergent" heißt hier: **nicht direkt eincodiert**.
Es heißt **nicht**: aus dem Nichts. Die 5 Urregeln sind eine
Konstruktionsentscheidung. Wir behaupten nicht, dass sie
notwendig sind, nur dass sie **ausreichend** sind.

### 2.4 — Shared Reference statt Amplitude kopieren

**Klassisch:** Verschränkung ist ein **Zustand** (Bell-Paar,
GHZ-Zustand, Tensor-Produkt). Sie ist in den Amplituden gespeichert.

**ProPhysics:** Verschränkung ist eine **Klassen-Zugehörigkeit**
(Union-Find). Zwei Knoten sind verschränkt, wenn sie denselben
Repräsentanten haben. Die Amplituden werden **physikalisch geteilt**
(shared pointer), nicht kopiert.

**Konsequenz:**
- **Tsirelson-Korrelation** (`S = 2,8457`, Tsirelson-Limit `2,8284`)
  entsteht aus dem Kollaps-Mechanismus, nicht aus einem Bell-Zustand.
- **Speicher:** Eine Klasse von N Knoten braucht 1 Amplitude statt N.
- **Synchronisation:** Änderung am Repräsentanten propagiert sofort.

**Grenze:** Union-Find ist **transitiv**. Wenn A mit B verschränkt
ist und B mit C, dann ist A automatisch mit C verschränkt. Für
GHZ-artige Zustände braucht man einen Hypergraph — Etappe 25.

### 2.5 — Quaternion-Links statt komplexer SU(2)-Matrizen

**Klassisch:** Ein SU(2)-Link ist eine 2×2-komplexe Matrix, also
8 Gleitkommazahlen. Multiplikation ist 8 komplexe Multiplikationen
plus 4 Additionen.

**ProPhysics:** Ein SU(2)-Link ist ein **Quaternion-Paar** `(a, b)`
mit `|a|² + |b|² = 2⁶⁰`, gespeichert als 4 × `int32`. Multiplikation
ist eine geschlossene Formel mit 16 `int64`-Multiplikationen
(optimiert auf 8 durch Ausklammern).

**Konsequenz:**
- **Speicher:** 16 Bytes pro Link statt 64 (8 × double).
- **Skala 2³⁰** statt 2³¹, um int64-Overflow zu vermeiden (max.
  Zwischenwert `4·2⁶⁰ = 2⁶²`).
- **Exakt** bis auf Q30-Rundung. Wilson-Loop ist eichinvariant auf
  `9,31e-10`, Kommutator ≠ 0 auf `1,07`.

**Grenze:** Nur SU(2). Für SU(3) (Etappe 26+) bräuchte man
Gell-Mann-Matrizen, die sich nicht als 4-Tupel darstellen lassen.
Würde auf 8 oder 9 Komponenten erweitern.

### 2.6 — Cache-Aligned + Zero-Alloc Hotpath

**Klassisch:** Häufig `std::vector`, `Eigen::Matrix`, mit
Heap-Allokationen während der Dynamik.

**ProPhysics:**
- **Cache-Line-Aligned Alloc** (`pro_aligned_calloc`) für 8
  Hot-Arrays (`ur_grid`, `reg_source`, `amp_grid`, `edge_phases`, …).
- **Kein `malloc` / `calloc` / `free`** in irgendeiner
  `Apply_*`-Funktion.
- **Kein `div` / `mod`** in irgendeiner `Apply_*`-Funktion.
- **Test-Gitter-Größen** sind Zweierpotenzen, damit Shift/Mask
  statt Division.

**Konsequenz:** Deterministische Laufzeit ohne GC-Pausen,
vorhersehbares Cache-Verhalten. Der Test `Running-Coupling` läuft
262 144 Knoten × 500 Sweeps in ~1 400 s = ~5,3 ms pro Knoten-Sweep.

**Grenze:** Der Kernel ist **single-threaded**. Keine
Parallelisierung im Kernel selbst. Die Tests nutzen mehrere Kerne
über `nmake /MP`, aber der Tick läuft sequenziell.

### 2.7 — Haar-Vorschlag statt additiver Störung

**Klassisch (im Test-Code):** Ein Metropolis-Vorschlag, der eine
Gauss-Störung zu den Quaternion-Komponenten addiert und anschließend
renormiert. Dieser Vorschlag ist **approximativ symmetrisch** und
kann bei großen ε-Werten **overflowen** (int32-Wraparound).

**ProPhysics (im Test-Code seit `1.23.14`):** Ein **Haar-Vorschlag**
`U → R·U` mit `R = exp(-i·(α/2)·n·σ)`, `α ~ N(0, ε²)`, `n` uniform
auf S². Dieser Vorschlag ist:

- **Exakt symmetrisch** (R† hat dieselbe Verteilung wie R).
- **Overflow-frei** (int64-Zwischenergebnis, Saturation nach int32).
- **Korrekt** bzgl. der SU(2)-Haar-Verteilung.

**Konsequenz:** Die 2D-SU(2)-Metropolis-Validierung ist auf `< 1,2 %`
gegen die exakte Bessel-Ratio `I₂(β)/I₁(β)` gelungen. Mit dem
alten additiven Vorschlag war die Verteilung systematisch um bis zu
51 % verschoben (bei β=0.5).

**Grenze:** Der Haar-Vorschlag ist aufwendiger pro Schritt
(cos/sin für den halben Winkel). Bei kleinen Gittern irrelevant, bei
`dim ≥ 64` spürbar. Kandidat für Optimierung in Etappe O1.

---

## 3. Warum das wichtig ist

### 3.1 — Für die Wissenschaft

**Frage:** „Warum diese Urregeln und nicht andere?"

ProPhysics ist ein **Testfeld** für diese Frage. Es erlaubt,
empirisch zu prüfen, welche Phänomene aus welchen Regeln folgen.
Beispiel: Wenn U4 (shared reference) entfernt wird, verschwindet
Tsirelson-Korrelation (Test T2.4: `S = 2.0000` statt `2,846`).
Wenn U5 verletzt wird, zerfällt die Bloch-Dispersion.

**Wert:** Konkrete, falsifizierbare Aussagen über eine
Regel-Ontologie. Kein Paper, aber ein Werkzeug, um die Frage
zu stellen.

### 3.2 — Für die numerische Simulation

**Frage:** „Geht es auch deterministisch und exakt?"

Die signed-permutation-Darstellung ist **keine** Approximation. Sie
ist eine andere Parametrisierung der unitären Gruppe. Für Operatoren,
die als signierte Permutationen darstellbar sind (Transport, Context,
Wave-Steps), liefert sie **exakte** Resultate ohne Gleitkomma-Fehler.

**Wert:** Für sehr lange Läufe (Millionen von Ticks) akkumuliert
Gleitkomma-Arithmetik systematische Fehler. Integer-Arithmetik
akkumuliert Rundung, aber **keinen** Bias.

### 3.3 — Für die Cross-Domain-Simulation

**Frage:** „Kann ein Kernel mehrere Physik-Bereiche abdecken?"

ProPhysics enthält **einen** Amp-Grid, auf dem parallel folgende
Strukturen existieren:

| Struktur | Aktivierung | Tests |
|---|---|---|
| Unitäre QM | immer | T1.1–T1.4 |
| U(1)-Eichfeld | `edge_phases` | T1.5, T1.6 |
| EPR/CHSH | `PRO_EPR_CHANNEL` | T1.7–T1.10 |
| Lindblad | Dichte-Trilogie | Density-Regression |
| Fermionen | Jordan-Wigner | Tensor-Regression |
| Spin-1/2 | `reserved_gating` Bit 0 | Spin-Half |
| Dirac | `reserved_gating` Bit 1 | Dirac |
| SU(2) kinematisch | `su2_active` | SU2-Wilson-Loop |
| SU(2) dynamisch | `su2_dynamics_active` | SU2-Wilson-Loop T15–T18 |
| SU(2) thermisch | Metropolis-Haar im Test | Running-Coupling + Creutz-Ratio + String-Tension |
| SU(2) 2D-Referenz | Metropolis-Haar im Test | Metropolis-2D |

**Wert:** Ein Nutzer kann mit **einem** Datenmodell Experimente
durchführen, die sonst mehrere Frameworks bräuchten.

### 3.4 — Für die Verifikation

**Frage:** „Ist die Software korrekt?"

Der Kernel hat eine **zweistufige** Validierung:

**Stufe 1 — 2D gegen exakte Referenz:**

Der 2D-SU(2)-Metropolis-Sampler reproduziert die exakte Bessel-
Referenz `I₂(β)/I₁(β)` mit rel_dev < 1,2 % bei vier β-Werten.

| β | `<W>` gemessen | `I₂/I₁` (exakt) | rel_dev |
|---|---|---|---|
| 0,50 | 0,125213 ± 0,001275 | 0,123718 | 1,2 % |
| 1,00 | 0,240922 ± 0,001596 | 0,240194 | 0,3 % |
| 2,00 | 0,434476 ± 0,001493 | 0,433127 | 0,3 % |
| 4,00 | 0,655848 ± 0,001250 | 0,658047 | 0,3 % |

Alle vier Werte innerhalb 2σ. Das ist die **stärkste
Validierungsaussage** des Projekts: der Kernel reproduziert eine
analytisch bekannte Verteilung.

**Stufe 2 — 3D Selbstkonsistenz:**

σ_a2 fällt streng monoton in β (asymptotische Freiheit). Die
Dimension-Konvergenz dim=16 → dim=32 liegt bei < 0,5 %:

| β | σ_a2 (dim=16) | σ_a2 (dim=32) | Differenz |
|---|---|---|---|
| 2,40 | 0,595053 ± 0,005997 | 0,594901 ± 0,002280 | 0,03 % |
| 2,50 | 0,560160 ± 0,004610 | 0,557702 ± 0,001776 | 0,4 % |

**Ehrliche Grenze:** Es gibt **keinen externen 3D-V&V-Anker**. Die
publizierten Werte (Cahill & Prasad 1989) sind 4D, der Kernel ist
3D. Die Kopplungen sind nicht vergleichbar; die Abweichung um
Faktor ~3 ist die 3D/4D-Differenz.

**Der frühere 0,08-%-Anker ist endgültig zurückgenommen.** Er war
ein Artefakt eines **int32-Overflow-Bugs** im Metropolis-Vorschlag
`U → normalize(U + ε)` bei ε > 1.

---

## 4. Konkrete Vorteile (Stand jetzt)

### 4.1 — Speicherverbrauch

| Szenario | Klassisch (double complex) | ProPhysics |
|---|---|---|
| 1 Million Knoten × 8 Basis | 128 MB (Amplituden) | 64 MB (Q31-Paare) |
| 1 Million Kanten SU(2)-Links | 64 MB (2×2 komplex, 8 doubles) | 40 MB (4 × int32 + Metadaten) |
| Union-Find-Parent | (nicht üblich) | 8 MB (uint64) |

**Kein spektakulärer Gewinn** — Faktor ~2. Der eigentliche Gewinn
liegt darin, dass Hot-Arrays **cache-aligned** sind und **keine
Zeiger-Indirektion** haben.

### 4.2 — Rechenzeit

**Beispiel: Context-Tick pro Knoten**

| Operation | Klassisch | ProPhysics |
|---|---|---|
| Operator anwenden | 64 komplexe Multiplikationen + 56 Additionen | 8 Indextäusche + Vorzeichen-Negation |
| Byte-Code | ~500 Instruktionen | ~20 Instruktionen |
| Cache-Zugriffe | 3 Cache-Lines (Matrix + Ein-/Ausgabe) | 1–2 Cache-Lines |

**Faktor ~25 auf der Operationsebene.** In der Praxis weniger,
weil der Transport (der dichte Multiplikation braucht) dominiert.

### 4.3 — Deterministische Reproduzierbarkeit

**Beispiel:** Der Test `Running-Coupling` mit festem Seed liefert
**bit-identische** Log-Ausgaben zwischen Läufen.

```
dim=64, β=2.0, u_plaq = 0.272552 ± 0.000027
```

Bei klassischen FFT- oder Monte-Carlo-Frameworks mit Gleitkomma
ist das **nicht** garantiert. Man braucht `-ffp-contract=off`,
`-ffast-math`-Aus, feste Rundungsmodi.

**Achtung:** Der Wave-Step und die SU(2)-exp-Map nutzen `cos`/`sin`.
Diese sind IEEE-754-konform auf x86-64, aber nicht plattformübergreifend
bit-identisch. Auf **einer** Plattform ist Reproduzierbarkeit garantiert.

### 4.4 — Validierung gegen externe Physik

**Teilweise erreicht.** Andere Frameworks testen sich selbst
(„Operator A mal Vektor B ergibt C"). ProPhysics testet gegen
**analytisch bekannte** Referenz:

- **2D-SU(2)-Haar-Verteilung** gegen exakte Bessel-Ratio `I₂/I₁`:
  < 1,2 % Abweichung. **Validiert.**

Für 3D-SU(2) fehlt der externe Anker (siehe §3.4). Das ist kein
Vorteil der Implementierung, sondern des gewählten Themas.
Lattice-QCD ist gut genug verstanden, um V&V zu erlauben — aber die
publizierten 3D-SU(2)-Werte stammen aus den 1980er Jahren und sind
schwer zugänglich.

### 4.5 — Single-Binary, keine externen Abhängigkeiten

**Beispiel:** `ProPhysics.dll` (Kernel) + `pro_sdk_interface.dll`
(SDK) sind die kompletten Laufzeit-Abhängigkeiten der Test-EXEs.
Keine `std::`-Laufzeit (nur `msvcrt`), keine `libm`, keine
`libstdc++`.

**Nutzen:**
- Einfaches Deployment
- Keine DLL-Hölle
- Kein Python, kein Java, kein Node.js

**Vergleich:** Andere wissenschaftliche Simulations-Frameworks
benötigen typisch 10–100 MB Abhängigkeiten (NumPy, SciPy, MPI,
Boost, BLAS, LAPACK).

---

## 5. Ehrliche Gegenüberstellung

### 5.1 — Was ProPhysics **kann**, was andere nicht können

| Fähigkeit | Warum besonders |
|---|---|
| Signed-permutation-Darstellung unitärer Operatoren | In dieser Form unüblich; üblich sind dichte Matrizen |
| Integer-Arithmetik ohne Gleitkomma im Hotpath | Deterministisch, portabel |
| Union-Find als Verschränkungs-Mechanismus | Physikalisch ungewöhnlich, praktisch effizient |
| Emergenz-Tests (Born, Bloch, Lorentz) mit einem Kernel | Zeigt Regel-Ontologie empirisch |
| **2D-SU(2)-Validierung gegen exakte Bessel-Ratio** | **Analytisch exakte Referenz** |
| **Haar-Vorschlag für Metropolis** | **Exakt symmetrisch, overflow-frei** |

### 5.2 — Was andere **können**, was ProPhysics nicht kann

| Fähigkeit | Wer es hat |
|---|---|
| Tensor-Netzwerke mit variabler Bond-Dimension | ITensor, TeNPy |
| Beliebige SU(N), SO(N), Sp(N) | MILC (QCD), QuSpin |
| MPI-Parallelisierung auf Cluster | QDP++, MILC, Chroma |
| Beliebige Kontinuums-Gitter (Voronoi, FEM) | FEniCS, Deal.II |
| Quantencomputing (Gate-Modell, Transpiler) | Qiskit, Cirq |
| Dichtematrix-Renormierung (DMRG, MPS) | ITensor, TeNPy |
| Exakte Diagonalisierung großer Hamiltonians | QuSpin, ALPS |
| Adiabatische Zeitentwicklung | QuTiP, QuSpin |
| **4D-Lattice-QCD** | **MILC, Chroma, QDP++** |

**Klarstellung:** ProPhysics ist **kein** Ersatz für etablierte
Simulations-Frameworks. Es ist eine **andere Darstellung**, deren
Stärken in einem sehr speziellen Regime liegen (kleine Gitter,
deterministische Läufe, emergente Ontologie).

---

## 6. Zielgruppe — Wer profitiert

### 6.1 — Forscher, die emergente QM untersuchen

**Ziel:** „Kann QM aus einfacheren Regeln entstehen?"

**Nutzen:** Ein konkretes, lauffähiges Modell mit 46 Tests, das
als Referenzpunkt für eigene Experimente dient. Alle Regeln sind
explizit in `Project.md` §3 dokumentiert, alle Tests in
`ProPhysics_Testkatalog.md`.

### 6.2 — Lehrende, die QM-Phänomene veranschaulichen wollen

**Ziel:** Studenten verstehen lassen, wie Bloch-Dispersion, Born-
Regel, Dirac-Struktur funktionieren.

**Nutzen:** Ein Kernel, den man **lesen** kann. 13 Module, ~36 kLOC,
alle Algorithmen inline dokumentiert. Die Tests sind
**Demonstrationen** (z.B. `test_born_rule` zeigt die Born-Statistik
in 10 000 Messungen).

### 6.3 — Ingenieure, die deterministische Simulation brauchen

**Ziel:** Reproduzierbare Ergebnisse, keine Gleitkomma-Überraschungen.

**Nutzen:** Der Kernel läuft bit-identisch auf jeder x86-64-CPU.
Kein `NaN`, kein Overflow, kein Unterlauf. Für sehr lange
Simulationen ist das ein Vorteil.

### 6.4 — Kommerzielle Anwender (Lizenz erforderlich)

**Ziel:** Integration in ein Produkt oder eine Dienstleistung.

**Nutzen:** Ein kompakter Kernel ohne externe Abhängigkeiten, der
in ein C/C++-Projekt eingebettet werden kann. Lizenz siehe
`COMMERCIAL.md`.

---

## 7. Was ProPhysics **nicht** ist — und das ehrlich

1. **Keine Quantengravitation.** Wir haben keine Schleifen, keine
   Planck-Skala, keine gekrümmte Raumzeit.

2. **Keine Simulation echter QCD.** Wir haben keine Quarks, keine
   Gluonen im Kontinuum, keine Renormierung im strengen Sinne.
   Wir haben SU(2)-Lattice-Yang-Mills auf **3D**-Gittern bis 64³.

3. **Kein Kontinuumslimes.** Alle Ergebnisse sind auf Gittern mit
   `dim ∈ {16, 32, 64, 128}` gemessen. Es gibt keine Extrapolation
   auf `dim → ∞`.

4. **Kein Bell-Bruch.** Der Test `test_chsh_edge_type` zeigt
   `S = 2,000000` (native Graph-Messung) und `S = 2,8457`
   (Kollaps-Messung, im 5σ-Band um Tsirelson). Ein `S > 2√2`
   wäre ein Superdeterminismus-Loop oder ein Test-Design-Fehler,
   **kein** physikalischer Befund.

5. **Keine Behauptung, dass die Urregeln „wahr" sind.** Wir zeigen,
   dass sie **ausreichend** sind für die beobachteten Phänomene.
   Andere Regel-Sätze könnten dasselbe leisten.

6. **Keine publizierte Physik.** Der Kernel ist ein
   Forschungs-Prototyp. Ein Preprint ist in Vorbereitung, aber
   nichts ist eingereicht.

7. **Keine Konkurrenz zu etablierten Lattice-QCD-Codes.** MILC,
   QDP++, Chroma sind auf Cluster skaliert, können
   `dim ≥ 256`, nutzen MPI, GPU, und sind über 20 Jahre optimiert.
   ProPhysics ist eine **Ergänzung**, kein Ersatz.

8. **Keine Universalität im strengen Sinne.** Der T-Gate (nicht-
   Clifford) ist nicht implementiert. Ohne ihn ist der Kernel
   kein universeller Quantencomputer-Simulator — Etappe 26.

9. **Keine Q61-Präzision.** Q31 reicht für die aktuelle Testsuite.
   Bei Renormierung mit zwei Observablen wird Q30-Rundung
   sichtbar — Etappe 27.

10. **Keine Parallelisierung.** Der Kernel ist single-threaded.
    Für größere Gitter bräuchte man OpenMP oder MPI-Erweiterungen.

11. **Kein externer V&V-Anker für 3D.** Die 2D-SU(2)-Validierung
    gegen exakte Bessel-Physik ist vorhanden (Etappe 23e Rev.2).
    Für 3D ist nur die Selbstkonsistenz (σ_a2 monoton fallend,
    dim-Konvergenz) verfügbar. Die publizierten Werte (Cahill &
    Prasad 1989) sind 4D und daher nicht vergleichbar.

12. **Kein 4D-Kernel.** Der Kernel unterstützt 2D und 3D-Tori.
    Ein 4D-Torus (`wire_torus_4d`) ist nicht implementiert.
    Kandidat für Etappe 25+.

---

## 8. Wann ist ProPhysics die richtige Wahl?

### 8.1 — Ja

- Du willst emergente QM untersuchen, nicht QM simulieren.
- Du brauchst deterministische, reproduzierbare Läufe.
- Du willst einen Kernel ohne externe Abhängigkeiten.
- Dein Gitter ist `dim ≤ 128`.
- Du willst einen kompakten, lesbaren Kernel verstehen oder erweitern.
- Du willst eine alternative Darstellung von unitären Operatoren testen.
- **Du willst 2D-SU(2) gegen exakte Bessel-Physik validieren.**

### 8.2 — Nein

- Du brauchst ein Produktions-Lattice-QCD-Code für große Gitter.
- Du willst Quantencomputing auf echten Qubits simulieren.
- Du brauchst Tensor-Netzwerke mit variabler Bond-Dimension.
- Du willst MPI-Parallelisierung auf 1000+ Kernen.
- Du brauchst `dim ≥ 512`.
- Du brauchst Q61-Präzision für sehr lange Läufe.
- Du brauchst etablierten Support, Schulung, Zertifizierung.
- **Du brauchst einen externen V&V-Anker gegen publizierte 3D-Werte.**

---

## 9. Konkrete Zahlen (Stand Etappe 23c Rev.2 + 23e Rev.2)

### 9.1 — Validierung

| Größe | Wert | Referenz | Status |
|---|---|---|---|
| Bloch-Dispersion 2D | rel_dev ≤ 5,22e-06 | tight-binding-Formel | ✅ exakt |
| Bloch-Dispersion 3D | rel_dev ≤ 2,57e-06 | tight-binding-Formel | ✅ exakt |
| γ-Algebra | 9,31e-10 | Clifford-Algebra | ✅ exakt |
| Jordan-Wigner | 16/16 exakt | Antikommutator | ✅ exakt |
| Tsirelson (Kollaps) | 2,8457 | 2√2 ≈ 2,8284 | ✅ im Band |
| **2D-SU(2) vs `I₂/I₁`** | **rel_dev ≤ 1,2 %** | **exakte Bessel** | **✅ validiert** |
| **3D-σ_a2 dim-Konvergenz** | **< 0,5 %** | **kein externer Anker** | **⚠ selbstkonsistent** |
| V&V-Anker (β=2, dim=64) | ~5 % | I₂(2)/I₁(2) = 0,43313 | ⚠ zurückgenommen |
| Creutz-Ratio | B1/B2/B3 PASS | — | ✅ konsistent |

### 9.2 — Stabilität

| Größe | Wert | Kontext |
|---|---|---|
| U5-Drift | < 1,8e-09 | über 2000 Ticks |
| Link-Norm-Drift | 1,80e-08 | SU(2)-Leapfrog, 100 Ticks |
| Energie-Drift | 1,41e-03 | SU(2)-Leapfrog, symplektisch |
| Ausführungszeit Prio-All | ~4 530 s (~75,5 min) | 46 Tests, ohne Full-Modi |

### 9.3 — Codebase

| Kategorie | Zahl |
|---|---|
| Kernel-Module | 13 `.c` |
| Test-Module | 19 `.c` + 1 Header |
| Kernel-Header | 6 `.h` |
| Codezeilen (geschätzt) | ~36 000 LOC |
| Tests | 46 |
| Grüne Tests | 46/46 |

---

## 10. Ausblick

### 10.1 — Was als nächstes kommt

| Etappe | Was | Nutzen |
|---|---|---|
| 24 | Euklidisches Pfadintegral | Pfadintegral ↔ Operator |
| 25 | GHZ / Mermin | n=3-Verschränkung |
| 26 | Universalität | T-Gate, Deutsch-Josza |
| 27 | Q61-Migration | Numerische Präzision |
| M1 | U4' — Bad | Kopplung an Unsichtbares |
| M2 | U5' — Plastizität | Dynamische Topologie |
| M3 | Makrophysik | Klassischer Limes |
| 23c-B (optional) | 3D-SU(2)-Literatur | Echter externer V&V-Anker |
| 25+ (optional) | 4D-Torus | Cahill & Prasad direkt vergleichbar |

### 10.2 — Was ProPhysics in 6–12 Monaten sein könnte

Wenn Etappe 24–27 abgeschlossen sind:

- **Kernel-Version 2.0.0** (Phase 2: „komplette QM")
- Pfadintegral-Äquivalenz demonstriert
- GHZ-Verschränkung über Hypergraph
- Universalität über T-Gate
- Q61-Präzision für lange Läufe
- Möglicherweise 4D-Torus und externer 3D-Anker

**Publikationsfähigkeit:** Ein methodisches Paper über
signed-permutation-Darstellung von unitären Gittertheorien
ist realistisch. Ein Physik-Paper über neue Phänomene ist
**nicht** zu erwarten, ohne dass eine echte Skalenaussage
gelingt oder ein externer 3D-Anker gefunden wird.

---

## 11. Zusammenfassung in einem Satz

**ProPhysics ist eine alternative Darstellung von Quantenmechanik
auf diskreten Graphen, die durch Integer-Arithmetik
deterministisch, durch signed permutations exakt und durch
Emergenz philosophisch interessant ist — in 2D validiert gegen
exakte Bessel-Physik, in 3D selbstkonsistent, aber ohne externen
V&V-Anker.**

---

## 12 — Siehe auch

| Thema | Datei |
|---|---|
| Physik-Übersicht | `docs/physics/README.md` |
| Projekt-Roadmap | `docs/project/Project.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Baseline | `docs/test/BASELINE.md` |
| Changelog | `CHANGELOG.md` |
| SU2-Modul | `docs/project/SU2.md` |
| SU2-Dynamik | `docs/project/SU2_Dynamics.md` |
| Konfiguration | `docs/project/CONFIG.md` |
| Versions-Register | `docs/project/ProPhysics_VersionRegistry.md` |
| Lizenz | `LICENSE.md` |
| Kommerzielle Lizenz | `COMMERCIAL.md` |
| Repository | https://github.com/onkel83/prophysics |

---

## 13 — Änderungshistorie dieses Dokuments

| Datum | Version | Änderung |
|---|:-:|---|
| 2026-09-26 | 1.0 | Erste Fassung. |
| 2026-09-29 | 1.1 | **V&V-Anker-Rücknahme.** §1 Kurzfassung: Validierungsstand auf 2D-Bessel und 3D-Selbstkonsistenz umgestellt. §2.7 neu: Haar-Vorschlag. §3.4 komplett neu: zweistufige Validierung (2D exakt, 3D selbstkonsistent), V&V-Anker als zurückgenommen markiert. §4.4 umgeschrieben. §5.1 um 2D-Bessel und Haar-Vorschlag erweitert. §5.2 um 4D-Lattice-QCD. §7 Punkt 11 und 12 neu (kein 3D-Anker, kein 4D-Kernel). §8.1 und §8.2 um V&V-Status erweitert. §9.1 Validierungstabelle um 2D/3D/Creutz-Zeilen. §9.3 Tests 43 → 46. §10 um 4D-Torus und 3D-Literatur. §11 Zusammenfassung umgeschrieben. §12 Siehe-auch. §13 neu. |

---

**Ende Differentiators v1.1.**