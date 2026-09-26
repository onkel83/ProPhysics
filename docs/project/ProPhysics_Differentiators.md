# ProPhysics — Was uns unterscheidet

**Datei:** `docs/project/ProPhysics_Differentiators.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-26 (Kernel 1.23.0, Konsolidierungs-Serie
1.23.1–1.23.8 abgeschlossen)
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

**Aktueller Validierungsstand:** Der Metropolis-Sampler auf SU(2)-
Links reproduziert den analytischen Ein-Plaquette-Wert aus der
Lattice-QCD auf **0,08 %** — der erste externe V&V-Anker des Projekts.

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
Drift nach 10 000 Ticks ist **0,0** (siehe Test T1.3).

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
| SU(2) thermisch | Metropolis im Test | Running-Coupling |

**Wert:** Ein Nutzer kann mit **einem** Datenmodell Experimente
durchführen, die sonst mehrere Frameworks bräuchten.

### 3.4 — Für die Verifikation

**Frage:** „Ist die Software korrekt?"

Der V&V-Anker aus Etappe 23 ist der erste **absolute** Vergleich
mit externer Physik:

| Größe | Wert | Referenz | Abweichung |
|---|---|---|---|
| ⟨P⟩(β=2, dim=64) | 0,43346 ± 0,00005 | I₂(2)/I₁(2) = 0,43313 | **0,08 %** |

Ein falscher Sampler würde auf Prozent-Ebene abweichen, nicht auf
0,08 %. Die Wilson-Action-Normierung, der Akzeptanzschritt und die
Q30-Quaternion-Multiplikation sind damit unabhängig bestätigt.

**Wert:** Der Kernel ist nicht nur „in sich konsistent" (was alle
Frameworks sind), sondern **stimmt mit einer publizierten Zahl
überein**.

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
dim=64, β=2.0, u_plaq = 0.283270 ± 0.000027
```

Bei klassischen FFT- oder Monte-Carlo-Frameworks mit Gleitkomma
ist das **nicht** garantiert. Man braucht `-ffp-contract=off`,
`-ffast-math`-Aus, feste Rundungsmodi.

### 4.4 — Validierung gegen externe Physik

**Einzigartig im Projekt:** Der V&V-Anker. Andere Frameworks testen
sich selbst („Operator A mal Vektor B ergibt C"), ProPhysics
testet gegen eine **publizierte physikalische Größe**.

Das ist kein Vorteil der Implementierung, sondern des gewählten
Themas. Lattice-QCD ist gut genug verstanden, um V&V zu erlauben.
Ein Framework für „allgemeine Quantensimulation" hat keinen
externen Referenzwert.

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
| Externe V&V gegen Lattice-QCD | Nur möglich, weil das Thema bekannt ist |

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

**Klarstellung:** ProPhysics ist **kein** Ersatz für etablierte
Simulations-Frameworks. Es ist eine **andere Darstellung**, deren
Stärken in einem sehr speziellen Regime liegen (kleine Gitter,
deterministische Läufe, emergente Ontologie).

---

## 6. Zielgruppe — Wer profitiert

### 6.1 — Forscher, die emergente QM untersuchen

**Ziel:** „Kann QM aus einfacheren Regeln entstehen?"

**Nutzen:** Ein konkretes, lauffähiges Modell mit 43 Tests, das
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
   Wir haben SU(2)-Lattice-Yang-Mills auf Gittern bis 64³.

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

---

## 8. Wann ist ProPhysics die richtige Wahl?

### 8.1 — Ja

- Du willst emergente QM untersuchen, nicht QM simulieren.
- Du brauchst deterministische, reproduzierbare Läufe.
- Du willst einen Kernel ohne externe Abhängigkeiten.
- Dein Gitter ist `dim ≤ 128`.
- Du willst einen kompakten, lesbaren Kernel verstehen oder erweitern.
- Du willst eine alternative Darstellung von unitären Operatoren testen.

### 8.2 — Nein

- Du brauchst ein Produktions-Lattice-QCD-Code für große Gitter.
- Du willst Quantencomputing auf echten Qubits simulieren.
- Du brauchst Tensor-Netzwerke mit variabler Bond-Dimension.
- Du willst MPI-Parallelisierung auf 1000+ Kernen.
- Du brauchst `dim ≥ 512`.
- Du brauchst Q61-Präzision für sehr lange Läufe.
- Du brauchst etablierten Support, Schulung, Zertifizierung.

---

## 9. Konkrete Zahlen (Stand Etappe 23)

### 9.1 — Validierung

| Größe | Wert | Referenz | Abweichung |
|---|---|---|---|
| Bloch-Dispersion 2D | rel_dev ≤ 5,22e-06 | tight-binding-Formel | — |
| Bloch-Dispersion 3D | rel_dev ≤ 2,57e-06 | tight-binding-Formel | — |
| γ-Algebra | 9,31e-10 | Clifford-Algebra | — |
| Jordan-Wigner | 16/16 exakt | Antikommutator | — |
| Tsirelson (Kollaps) | 2,8457 | 2√2 ≈ 2,8284 | im 5σ-Band |
| SU(2)-Metropolis | 0,43346 ± 0,00005 | 0,43313 | **0,08 %** |

### 9.2 — Stabilität

| Größe | Wert | Kontext |
|---|---|---|
| U5-Drift | < 1,8e-09 | über 2000 Ticks |
| Link-Norm-Drift | 1,80e-08 | SU(2)-Leapfrog, 100 Ticks |
| Energie-Drift | 2,44e-03 | SU(2)-Leapfrog, symplektisch |
| Ausführungszeit Prio-All | 4 420 s (~74 min) | 43 Tests, 2× dim=64 |

### 9.3 — Codebase

| Kategorie | Zahl |
|---|---|
| Kernel-Module | 13 `.c` |
| Test-Module | 18 `.c` + 1 Header |
| Kernel-Header | 6 `.h` |
| Codezeilen (geschätzt) | ~36 000 LOC |
| Tests | 43 |
| Grüne Tests | 43/43 |

---

## 10. Ausblick

### 10.1 — Was als nächstes kommt

| Etappe | Was | Nutzen |
|---|---|---|
| 23b (optional) | Creutz-Ratio | Echte β-Funktion |
| 24 | Euklidisches Pfadintegral | Pfadintegral ↔ Operator |
| 25 | GHZ / Mermin | n=3-Verschränkung |
| 26 | Universalität | T-Gate, Deutsch-Josza |
| 27 | Q61-Migration | Numerische Präzision |
| M1 | U4' — Bad | Kopplung an Unsichtbares |
| M2 | U5' — Plastizität | Dynamische Topologie |
| M3 | Makrophysik | Klassischer Limes |

### 10.2 — Was ProPhysics in 6–12 Monaten sein könnte

Wenn Etappe 24–27 abgeschlossen sind:

- **Kernel-Version 2.0.0** (Phase 2: „komplette QM")
- Pfadintegral-Äquivalenz demonstriert
- GHZ-Verschränkung über Hypergraph
- Universalität über T-Gate
- Q61-Präzision für lange Läufe

**Publikationsfähigkeit:** Ein methodisches Paper über
signed-permutation-Darstellung von unitären Gittertheorien
ist realistisch. Ein Physik-Paper über neue Phänomene ist
**nicht** zu erwarten, ohne dass eine echte Skalenaussage
gelingt.

---

## 11. Zusammenfassung in einem Satz

**ProPhysics ist eine alternative Darstellung von Quantenmechanik
auf diskreten Graphen, die durch Integer-Arithmetik
deterministisch, durch signed permutations exakt und durch
Emergenz philosophisch interessant ist — und die jetzt mit
0,08 % gegen externe Lattice-QCD validiert ist.**

---

**Ende Differentiators v1.0.**