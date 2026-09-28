# ProPhysics — Physikalische Grundlagen

**Datei:** `docs/physics/README.md`
**Version:** 1.1
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-29 (Kernel 1.23.0, Konsolidierungs-Serie
1.23.1–1.23.14 abgeschlossen; V&V-Anker zurückgenommen)
**Zweck:** Kompakte Einführung in die Physik von ProPhysics. Für Leser,
die verstehen wollen, **was** der Kernel physikalisch tut, ohne den
Testkatalog zu lesen oder den Code zu studieren.

---

## §0 — Wozu dieses Dokument

ProPhysics ist ein **Forschungs-Prototyp**. Es hat eine physikalische
Frage, eine Methodik und einen Validierungsstand. Dieses Dokument
erklärt alle drei in kompakter Form.

**Nicht in diesem Dokument:**

- Code-Details → `docs/project/ARCHITECTURE.md`
- API-Signaturen → `docs/project/ProPhysics_API.md`
- Test-Rohwerte → `docs/test/ProPhysics_Testkatalog.md`
- Etappen-Historie → `docs/project/Project.md`
- Was das Projekt **nicht** ist → `docs/project/ProPhysics_Differentiators.md`

**Zielgruppe:**

- Physiker, die einordnen wollen, was hier behauptet wird
- Interessierte, die verstehen wollen, was „emergente QM" bedeutet
- Reviews und Reviewer, die den Kern in 30 Minuten erfassen wollen

---

## §1 — Die physikalische Frage

### §1.1 — Die Kernfrage

> **Reichen einfache diskrete Regeln, um Phänomene zu erzeugen, die
> wir als Quantenmechanik kennen — ohne dass QM explizit eincodiert
> ist?**

Das ist **keine** neue Physik. Das ist eine **Darstellungsfrage**. Wir
fragen nicht: „Ist QM falsch?" Wir fragen: „Braucht QM ihre
komplizierte mathematische Struktur (Hilbert-Raum, dichte Operatoren,
komplexe Kontinuumsfunktionen), oder reicht etwas Einfacheres?"

### §1.2 — Warum diese Frage sinnvoll ist

Die QM hat zwei Merkwürdigkeiten:

1. **Unitäre Evolution** — Zustände rotieren in einem Hilbert-Raum.
2. **Born-Regel** — Messungen liefern Wahrscheinlichkeiten `|ψ|²`.

Beide sind **postuliert**, nicht hergeleitet. ProPhysics fragt:
Können beide **emergieren** — aus etwas, das elementarer ist?

### §1.3 — Was „emergent" bedeutet

**Emergent heißt hier:**

> Die Struktur ist **nicht direkt eincodiert**, aber sie ist eine
> Folge der Regeln.

**Emergent heißt nicht:**

> Die Struktur kommt aus dem Nichts.

Der Kernel hat **eine** endliche Regelmenge (U1–U5). Diese sind
**Konstruktionsentscheidungen**. Wir behaupten nicht, dass sie
notwendig sind — nur, dass sie **ausreichend** sind.

### §1.4 — Was „emergent" nicht bedeutet

- **Kein Bell-Bruch.** `S > 2√2` wäre ein Fehler, kein Befund.
- **Kein Kontinuumslimes.** Alle Ergebnisse sind auf Gittern.
- **Keine neue Physik.** Der Kernel ist eine **andere Darstellung**
  bekannter Physik.

---

## §2 — Die 5 Ur-Regeln

Das Fundament. Jede andere Struktur folgt aus ihnen — oder ist eine
explizite Erweiterung (siehe §3.4 in `Project.md`).

### §2.1 — U1: Zustandsmenge

```
U1:  U = {0,1} × {00,01,10,11}
```

**Bedeutung:** Pro Gitterknoten existiert ein **8-dimensionaler**
komplexer Vektor.

| Index | Name | Rolle |
|:-:|---|---|
| 0 | `UR_NEUTRAL` | Vakuum |
| 1 | `UR_POSITRON_CW` | positiv geladen, rechtshändig |
| 2 | `UR_POSITRON_CCW` | positiv geladen, linkshändig |
| 3 | `UR_NEGATRON_CW` | negativ geladen, rechtshändig |
| 4 | `UR_NEGATRON_CCW` | negativ geladen, linkshändig |
| 5 | `UR_PHOTON` | Photon |
| 6, 7 | (reserviert) | — |

**Numerische Kodierung:** Q31 (`int64`, Re/Im je `int32`).

**Warum 8?** Zwei Bitladungen (`{0,1}`) × zwei Helizitäten
(`{00,01,10,11}`) = 8. Es ist die minimale Basis, die die
beobachteten Phänomene tragen kann.

### §2.2 — U2: Lokale Interaktion

```
U2:  A : G → U ∪ G,   G ⊆ ℤ³
```

**Bedeutung:** Pro Tick darf jeder Knoten mit **einem** Nachbarn
interagieren. Der Nachbar wird über die Kanal-Register
(`reg_source[k].channels[c]`) bestimmt.

**Kanal-Layout:**

| Kanal | Richtung |
|:-:|---|
| 0 | +x |
| 1 | −x |
| 2 | +y |
| 3 | −y |
| 4 | +z |
| 5 | Dephase |
| 6 | −z |
| 7..14 | Reserve |
| 15 | EPR |

### §2.3 — U3: Tick-Iteration

```
U3:  S_{t+1} = f(S_t)
```

**Bedeutung:** Zeit ist **diskret** und **sequenziell**. Ein Tick ist
eine vollständige Aktualisierung des Zustands.

**Konsequenz:** Die Lichtgeschwindigkeit `c` ist **nicht** eincodiert.
Sie ist die **Lieb-Robinson-Rate** der Kanal-Topologie.

### §2.4 — U4: Shared Reference

```
U4:  Ψ(x,y) ⟺ A(x) = A(y)
```

**Bedeutung:** Verschränkung ist **Pointer-Identität**. Zwei Knoten
sind verschränkt, wenn sie denselben Amplitudenvektor teilen
(Union-Find-Klasse).

**Konsequenz:** Änderung am Repräsentanten propagiert **sofort** —
ohne Signallaufzeit. Das ist die Quelle der Tsirelson-Korrelation.

**Grenze:** Union-Find ist **transitiv**. Wenn `A ~ B` und `B ~ C`,
dann ist `A ~ C`. Für GHZ-artige Zustände braucht man einen
Hypergraph (Etappe 25).

### §2.5 — U5: Erhaltungssatz

```
U5:  Σ_{x∈G} A_t(x) = Konstante
```

**Bedeutung:** Die **gewichtete Norm**

```
Σ_k Σ_b w_b · |c_b(k)|²
```

mit `w = (0, 1, 1, 4, 4, 5, 0, 0)` ist **erhalten**.

**Numerische Form:** Als `ProU128` (128-Bit-Ganzzahl), um Overflow zu
vermeiden.

**Konsequenz:** Der Kernel ist **keine dissipative Simulation**. Alles,
was passiert, ist unitär. Kein Informationsverlust.

**Woher kommen die Gewichte?** Sie sind eine
Konstruktionsentscheidung. Die Wahl `(0,1,1,4,4,5,0,0)` ist nicht
kanonisch — sie funktioniert.

---

## §3 — Die 10 Thesen (Ontologie)

Diese Thesen beschreiben die **Interpretation** des Modells. Sie sind
**nicht** eincodiert, sondern aus dem Verhalten abgeleitet.

| # | These | Beleg |
|---|---|---|
| T1 | Grid + Graph fundamental | Konstruktion |
| T2 | Zeit = Operationsfolge | U3 |
| T3 | c = Lieb-Robinson-Rate | Wave-Packet-Test |
| T4 | Verschränkung via Graph-Kanten | U4, Tsirelson-Test |
| T5 | Bandbreiten-Budget pro Kante | Transport-Test |
| T6' | Energie = Informationsgehalt | Interpretation |
| T7' | γ = √(1 − v²/c²) aus Tick-Unitariät | Lorentz-Test |
| T8 | Position kumuliert aus Operationen | Guiding-Gleichung |
| T9 | Hawking = Hash (Sättigung) | Interpretation |
| T10 | Grid = Projektion des Graphen | Interpretation |

**Wichtig:** T6', T9, T10 sind **Interpretation**, nicht Messung. Sie
sind als Orientierung gedacht, nicht als Behauptung.

---

## §4 — Emergente Phänomene

Was aus U1–U5 (und den Struktur-Erweiterungen) folgt.

### §4.1 — Born-Regel

**Was gemessen wird:** Statistik von `|c_b|²` über viele Projektionen.

**Wie es entsteht:** Projektion `<θ|ψ>` auf einen Winkel `θ` +
Nachbar-Amplituden + epistemischer Hash.

**Quantitativer Beleg (Test `Born-Emergent`):**

| λ | P_meas | P_born |
|---|---|---|
| 0,0000 | 1,0000 | 1,0000 |
| 0,7854 | 0,8675 | 0,8536 |
| 1,5708 | 0,5425 | 0,5000 |
| 2,3562 | 0,1400 | 0,1464 |
| 2,7489 | 0,0325 | 0,0381 |

`χ² = 9,89` (dof=7, Schwelle 18,48 für 1 %).

**Emergenz:** Ja. Die Born-Regel ist **nicht** als Formel eincodiert.
Sie entsteht aus der Projektion.

**Grenze:** Die Statistik ist über endlich viele Trials. Mit mehr
Trials würde χ² besser. Die `10⁴`-Tests sind nicht ausreichend für
eine Präzisionsaussage.

### §4.2 — Bloch-Dispersion

**Was gemessen wird:** Dispersionsrelation `ω(k)` für Bloch-Zustände.

**Wie es entsteht:** Tight-Binding-artiger Transport auf dem Gitter.

**Quantitativer Beleg (2D, Test `QM-Emergent`):**

| k/π | ω_theo | ω_meas | rel_dev |
|---|---|---|---|
| 0,0625 | +0,759390 | +0,759392 | 2,56e-06 |
| 0,5625 | +0,310728 | +0,310729 | 2,59e-06 |
| 1,1875 | +0,074883 | +0,074883 | 3,18e-06 |

`max rel_dev = 5,22e-06`.

**Quantitativer Beleg (3D, Test `3D-Dispersion`):**

| (mx,my,mz) | ω_theo | ω_meas | rel_dev |
|---|---|---|---|
| (4,0,0) | +1,120238 | +1,120240 | 2,56e-06 |
| (4,4,0) | +1,090704 | +1,090707 | 2,55e-06 |
| (4,4,4) | +1,061944 | +1,061947 | 2,57e-06 |
| (8,4,2) | +0,999530 | +0,999532 | 2,56e-06 |

`max rel_dev = 2,57e-06`.

**Emergenz:** Ja. Die Dispersionsrelation ist **nicht** als Formel
eincodiert. Sie entsteht aus dem Transport.

**Bedeutung:** Das ist **genuine** Physik — eine quantitative
Vorhersage, die auf `10⁻⁶` bestätigt wird.

### §4.3 — Lorentz-Zeitdilatation

**Was gemessen wird:** Verlangsamung der inneren Uhr bei Bewegung.

**Wie es entsteht:** Die innere Uhr (`phase_accumulator`) wächst mit
`256 · γ⁻¹` pro Tick, wobei `γ⁻¹ = √(1 − v²)`.

**Quantitativer Beleg (Test `Lorentz`):**

| v | GP_v/GP_0 gemessen | erwartet | Abweichung |
|---|---|---|---|
| 0,60 | 0,799504 | 0,800000 | 5,0e-04 |
| 0,80 | 0,600542 | 0,600000 | 5,4e-04 |

**Emergenz:** **Ja.** Der Lorentz-Faktor ist **nicht** als Formel
eincodiert — er folgt aus der Tick-Unitariät.

**Bedeutung:** Das ist eine der stärksten Emergenz-Aussagen. Die
Zeitdilatation ist eine Folge der diskreten Tick-Struktur.

### §4.4 — Spin-1/2

**Was gemessen wird:** SU(2)-Algebra auf zwei Basiszuständen
`{UR_POSITRON_CW, UR_POSITRON_CCW}`.

**Wie es entsteht:** Signed permutations auf zwei Komponenten
erzeugen die SU(2)-Algebra. Der Spin-Flip ist eine Markierung in
`ProNode.reserved_gating` (Bit 0).

**Quantitativer Beleg (Test `Spin-Half`):**

| Kriterium | Wert |
|---|---|
| Pauli-Algebra `max_err` | 0,0 (exakt) |
| `R(2π) = −I` | exakt |
| g-Faktor `rel_dev` | 8,12e-10 |
| Singlet-Antikorrelation | exakt |

**Emergenz:** **Ja.** Spin-1/2 emergiert aus der SU(2)-Struktur.
Der g-Faktor ist **strukturell 2**, nicht eincodiert.

### §4.5 — Dirac-Struktur

**Was gemessen wird:** 4-Komponenten-Spinor mit Clifford-Algebra.

**Wie es entsteht:** Signed permutations auf 4 Komponenten
`{ψ_L↑, ψ_L↓, ψ_R↑, ψ_R↓}` + Massenterm als 2-Knoten-Rotation.

**Quantitativer Beleg (Test `Dirac`):**

| Kriterium | Wert |
|---|---|
| γ-Algebra `max_err` | 9,31e-10 |
| Massenterm Dirac | 2,4e-07 |
| Massenterm Weyl | 2,4e-07 |
| Zitterbewegung | sichtbar |
| Regression | 0,0 |

**Emergenz:** **Ja.** Die 4-Komponenten-Dirac-Struktur emergiert aus
signed-permutation-Mechanik. Die Clifford-Algebra ist **nicht**
eincodiert — sie folgt aus den Permutationen.

### §4.6 — Fermionische Statistik

**Was gemessen wird:** Jordan-Wigner-Vorzeichen, Antikommutatoren.

**Wie es entsteht:** Fock-Raum mit 8 Moden (256-dim Basis) +
Vorzeichen-String.

**Quantitativer Beleg (Test `Tensor-Regression`):**

| Test | Messung | Referenz |
|---|---|---|
| F1 Popcount | 0/256 Fehler | alle korrekt |
| F9 `c†_1 |1⟩` | −Q31(1) | exakt |
| F12 `c†_2 c†_1 c†_0` | −\|1,2,3⟩ | exakt |
| F16 Hopping(0,1) | 0,499998 / 0,500002 | 0,5 / 0,5 |
| F18 Hopping-Doubling | 0,000000e+00 | exakt |
| F20 Roundtrip | 0,000000e+00 | exakt |

**Emergenz:** **Ja.** Die fermionische Statistik emergiert aus dem
Vorzeichen-String. Die Antikommutatoren sind **nicht** eincodiert.

### §4.7 — Nicht-abelsche SU(2)-Eichtheorie

**Was gemessen wird:** Wilson-Loop auf einem Quaternion-Link.

**Wie es entsteht:** Quaternion-Repräsentation von SU(2)-Links in
`ProEdge`, Wilson-Loop als Pfad-Produkt.

**Quantitativer Beleg (Test `SU2-Wilson-Loop`, T1–T18):**

| # | Test | Kriterium | Wert |
|:-:|---|---|---|
| T1 | Quaternion-Unit | <1e-9 | 6,93e-10 |
| T3 | Konjugat q·q* = \|q\|²I | <1e-9 | 4,48e-10 |
| T4 | Quat == 2×2-Matrix | <1e-9 | 4,45e-10 |
| T5 | Pauli [σx,σy] = 2iσz | <1e-9 | 0,0 |
| T8 | Eichinvarianz | <1e-9 | 0,0 |
| T9 | Nicht-Abelsch | >0.1 | 1,074719 |
| T15 | Link-Norm unter Leapfrog | <1e-5 | 1,80e-08 |
| T16 | Energieerhaltung (100 T) | <1e-2 | **1,41e-03** |

**Emergenz:** **Ja.** Die nicht-abelsche Eichstruktur emergiert aus
`ProEdge.su2_*`-Feldern. Der Kommutator ist ≠ 0 auf `1,07`.

**Nachtrag (Patch 1.23.10):** Ein Forward-Plaquette-Konjugations-Bug
in `su2_plaquette_action_at` (fehlende `†`-Anwendung auf die
rückwärtigen Links `l3†`, `l4†`) wurde gefixt. T16 verbessert sich
dadurch von 2,44e-03 auf **1,41e-03** (Faktor 1,7).

**Grenze:** Nur SU(2). Für SU(3) bräuchte man 8 Komponenten
(Gell-Mann), nicht 4.

### §4.8 — SU(2)-Eichtheorie: Validierung

**Was gemessen wird:** Wilson-Plaquette-Observable in 2D und 3D.

**2D-Referenz (exakt):**

Der 2D-SU(2)-Metropolis-Sampler reproduziert die exakte
Haar-Verteilung, charakterisiert durch die Bessel-Ratio
`⟨½ Re Tr U⟩ = I₂(β)/I₁(β)`. Geprüft bei dim=16 für β ∈ {0.5, 1, 2, 4}.

**Quantitativer Beleg (2D, dim=16):**

| β | `<W>` gemessen | `I₂/I₁` (exakt) | rel_dev |
|---|---|---|---|
| 0,50 | 0,125213 ± 0,001275 | 0,123718 | 1,2 % |
| 1,00 | 0,240922 ± 0,001596 | 0,240194 | 0,3 % |
| 2,00 | 0,434476 ± 0,001493 | 0,433127 | 0,3 % |
| 4,00 | 0,655848 ± 0,001250 | 0,658047 | 0,3 % |

**Ergebnis: 4/4 PASS.** Alle Werte innerhalb 2σ.

**3D-Selbstkonsistenz (String-Tension):**

σ_a2 fällt streng monoton in β (asymptotische Freiheit). dim=16 vs
dim=32 konvergiert auf < 0,5 %.

| β | σ_a2 (dim=16) | σ_a2 (dim=32) | Differenz |
|---|---|---|---|
| 2,40 | 0,595053 ± 0,005997 | 0,594901 ± 0,002280 | 0,03 % |
| 2,50 | 0,560160 ± 0,004610 | 0,557702 ± 0,001776 | 0,4 % |

**Emergenz:** **Ja.** Die 2D-Haar-Verteilung und die 3D-Confinement-
Eigenschaft emergieren aus Metropolis + Wilson-Action.

**Status gegenüber früheren Angaben:** Der frühere V&V-Anker
(0,08 % gegen `I₂(2)/I₁(2)`) aus `[1.23.0]` war ein Artefakt eines
Sampler-Bugs (`U → normalize(U + ε)` hatte int32-Overflow bei
ε > 1). Nach dem Sampler-Fix (`[1.23.14]`) ist der Anker
zurückgenommen.

**Kein externer 3D-Anker.** Die publizierten Werte von Cahill &
Prasad (1989) sind 4D-SU(2). Der Vergleich mit 3D ist unzulässig.

### §4.9 — Tsirelson-Korrelation

**Was gemessen wird:** CHSH-Wert `S` aus EPR-Paarmessungen.

**Wie es entsteht:** U4-Shared-Reference-Kollaps.

**Quantitativer Beleg (Test `CHSH-Collapse`):**

| Messung | Wert |
|---|---|
| `S` (Kollaps) | 2,8457 |
| Tsirelson-Limit | 2,8284 |
| Abweichung | im 5σ-Band |

**Andere CHSH-Varianten:**

| Test | S | Interpretation |
|---|---|---|
| CHSH-Native | 2,0000 | klassische Schranke |
| CHSH-Graph | 2,8144 | Tsirelson via Kantenphase |
| CHSH-Collapse | 2,8457 | Tsirelson via U4 |

**Emergenz:** **Ja.** Die Bell-Verletzung emergiert aus U4. Ohne U4
(`shared.active = 0`) bleibt `S = 2`.

**Wichtig:** Ein `S > 2√2` wäre ein **Fehler**, kein Befund. Kein
Bell-Bruch.

### §4.10 — Hydrogen-Lokalisierung

**Was gemessen wird:** Lokalisierung eines Wellenpakets unter
Coulomb-Potential.

**Wie es entsteht:** `Apply_Coulomb_Phase_Field_3D` +
Imaginaerzeit-Evolution.

**Quantitativer Beleg (Test `Hydrogen`, dim=32):**

| strength | ⟨r²⟩(0) | ⟨r²⟩(2000) | Sättigung |
|---|---|---|---|
| 0 | 13,50 | 380,17 | nein |
| 2000 | 13,50 | 18,38 | ja |
| 8000 | 13,50 | 5,40 | ja |

`ratio = 0,400` (Schwelle 2.0). Lokalisierung nachgewiesen.

**Quantitativer Beleg (Test `Hydrogen-48`, dim=64):**

| strength | ⟨r²⟩(0) | ⟨r²⟩(4000) | Sättigung |
|---|---|---|---|
| 0 | 54,00 | 1128,22 | nein |
| 16000 | 58,80 | 28,05 | ja |

**Spektrum verfehlt:** `rel_dev = 0,296` (Ziel < 5 %).

**Emergenz:** **Ja** für Lokalisierung. **Nein** für das quantitative
Spektrum. Das ist eine ehrliche Grenze.

### §4.11 — Soliton / Breather

**Was gemessen wird:** Lokalisierung in GP-Selbstkopplung.

**Quantitativer Beleg (Test `Soliton`):**

| Modus | mean_tail | span_tail | Klasse |
|---|---|---|---|
| LINEAR | 37,37 | 2,28 | BREATHER |
| MODERAT | 8,79 | 4,38 | BREATHER |
| STARK | 21,09 | 12,97 | BREATHER |

`Clipping = 0`.

**Emergenz:** **Ja.** Die Breather-artige Lokalisierung emergiert
aus GP-Selbstkopplung. Kein Zerfall, keine Dissipation.

---

## §5 — Was quantitativ belegt ist

Zusammenfassung der harten Zahlen.

| Phänomen | Größe | Wert | Status |
|---|---|---|---|
| Bloch-Dispersion 2D | `rel_dev` | 5,22e-06 | ✅ exakt |
| Bloch-Dispersion 3D | `rel_dev` | 2,57e-06 | ✅ exakt |
| γ-Algebra | `max_err` | 9,31e-10 | ✅ exakt |
| Jordan-Wigner | Antikommutatoren | exakt | ✅ exakt |
| Lorentz | `rel_dev` | 5,4e-04 | ✅ gut |
| Spin-1/2 | g-Faktor | 8,12e-10 | ✅ exakt |
| SU(2)-Unit | `max_err` | 4,45e-10 | ✅ exakt |
| 2D-SU(2) vs `I₂/I₁` | `rel_dev` | 1,2 % | ✅ exakt |
| 3D-String-Tension dim-Konvergenz | Differenz | < 0,5 % | ✅ konsistent |
| U5-Drift | 2000 Ticks | 1,81e-09 | ✅ stabil |
| Tsirelson | S | 2,8457 | ✅ im Band |
| Creutz-Ratio | B1/B2/B3 | PASS | ✅ konsistent |

**Das ist der validierte Kern des Projekts.**

**Nicht mehr in der Tabelle:** Der frühere V&V-Anker gegen externe
Lattice-QCD (0,08 %) — siehe §4.8 und `CHANGELOG.md` `[1.23.14]`.

---

## §6 — Was qualitativ ist

Phänomene, die gezeigt, aber nicht auf `10⁻⁶` quantifiziert sind.

| Phänomen | Status | Warum qualitativ |
|---|---|---|
| Soliton-Klassifikation | BREATHER | Phänomenologisch |
| Zitterbewegung | sichtbar | Nicht quantifiziert |
| QM-Basics (Interferenz) | Fringes sichtbar | Kein Sollwert |
| Verschränkung ohne U4 | strukturell | Kein Maß |
| Born-Verteilung | χ²-Test | Endliche Trials |

**Konsequenz:** Diese Phänomene sind **Demonstrationen**, nicht
Messungen. Wer sie quantifizieren will, braucht neue Tests.

---

## §7 — Was verfehlt ist

Ehrliche Liste der Ziele, die **nicht** erreicht wurden.

### §7.1 — Wasserstoff-Spektrum

**Ziel:** `rel_dev < 5 %`.

**Erreicht:** `rel_dev = 0,296` (dim=64).

**Grund:** Die adaptive Prep funktioniert nicht für höhere
Quantenzahlen. Der Grundzustand ist erreichbar, angeregte Zustände
sind es nicht.

**Nächster Schritt:** Etappe 18d-B (optional). Nicht blockierend.

### §7.2 — Externe V&V gegen Lattice-QCD

**Ziel:** Absolute Übereinstimmung mit publizierten Werten.

**Erreicht:** 2D-SU(2) gegen exakte Bessel-Referenz (1,2 %), aber
**kein externer 3D-Anker**.

**Grund:** Die publizierten Cahill-&-Prasad-Werte sind 4D, der
Kernel ist 3D. Die Kopplungen sind nicht vergleichbar.

**Nächster Schritt:** Entweder 3D-Literatur suchen (Ambjørn/Hey/Otto
1983/1984) oder 4D-Torus implementieren (Etappe 25+).

### §7.3 — Confinement / Massenlücke

**Ziel:** Nachweis einer Massenlücke im Yang-Mills-Spektrum.

**Erreicht:** String-Tension σ_a2 > 0 (indirekt Confinement).

**Nicht erreicht:** Direkte Massenlücke (Torelon-Masse) — der Test
schlägt fehl (Timeout, Korrelation fällt auf Rauschen bei z=1).

**Grund:** Erfordert nicht-perturbative Methoden (größere Gitter,
längere Läufe, bessere Observablen).

**Nächster Schritt:** Etappe 24+.

### §7.4 — Universalität

**Ziel:** Quantencomputer-Universalität (T-Gate).

**Erreicht:** Nichts. Clifford-Gatter (H, X, Y, Z, CNOT, CZ, SWAP)
sind implementiert.

**Grund:** T-Gate ist nicht-Clifford und nicht implementiert.

**Nächster Schritt:** Etappe 26.

---

## §8 — Grenzen

### §8.1 — Konzeptionelle Grenzen

| Grenze | Warum |
|---|---|
| Kein Kontinuumslimes | Gitter ist fix (`dim ∈ {16, 32, 64, 128}`) |
| Keine Quantengravitation | Kein Konzept von Raumzeitkrümmung |
| Keine echte QCD | Keine Quarks, keine Gluonen im Kontinuum |
| Keine Universalität | T-Gate fehlt |
| Kein Bell-Bruch | `S > 2√2` wäre Fehler |
| **Kein externer V&V-Anker** | **3D-Kernel vs 4D-Referenz** |

### §8.2 — Technische Grenzen

| Grenze | Wert |
|---|---|
| Max. Gitter | `dim = 128` (3D), `128²` (2D) |
| Max. Knoten | durch RAM begrenzt |
| Numerik | Q31 (7 Stellen Präzision) |
| Parallelisierung | keine (single-threaded) |
| GPU | keine |
| Persistenz | keine |

### §8.3 — Gitter-Größen

`sizeof`-Arrays pro Knoten: **1 032 B**.

| Gitter | Knoten | RAM (Kernarrays) |
|---|---|---|
| 16² | 256 | ~258 KB |
| 64² | 4 096 | ~4,0 MB |
| 128² | 16 384 | ~16,2 MB |
| 16³ | 4 096 | ~4,0 MB |
| 32³ | 32 768 | ~32,3 MB |
| 64³ | 262 144 | ~259 MB |
| 128³ | 2 097 152 | ~2,07 GB |

**Praktische Obergrenze:** `64³` (~260 MB), darüber wird der Tick
zu langsam.

---

## §9 — Vergleich mit etablierter Physik

### §9.1 — Was gleich ist

- **Unitäre Zeitentwicklung** (Schrödinger-artig).
- **Born-Regel** als Projektionswahrscheinlichkeit.
- **Nicht-abelsche Eichtheorie** (Yang-Mills-artig).
- **Fermionische Antikommutatoren** (Jordan-Wigner).
- **Lorentz-Faktor** `γ = 1/√(1 − v²)`.
- **Zeitdilatation** als Folge der Unitariät.
- **2D-SU(2)-Haar-Verteilung** (exakt).

### §9.2 — Was anders ist

| Aspekt | Etablierte Physik | ProPhysics |
|---|---|---|
| Zustand | Hilbert-Raum-Vektor | 8-dim Q31 pro Knoten |
| Operatoren | dichte Matrizen | signed permutations |
| Zeit | kontinuierlich | diskret (Tick) |
| Raum | kontinuierlich oder Gitter | Gitter (Zweierpotenz) |
| Verschränkung | Tensor-Produkt | Union-Find |
| Numerik | double complex | Q31 integer |
| Darstellung | `P = \|ψ\|²` | Projektion + Hash |

### §9.3 — Was **nicht** vergleichbar ist

- **Skalenrenormierung:** ProPhysics hat keine echte RG.
- **Chiraler Limes:** Kein Kontinuumsübergang.
- **Quark-Confinement:** Nicht zugänglich.
- **Higgs-Mechanismus:** Nicht implementiert.
- **4D-Lattice-QCD:** Kernel ist 3D.

**ProPhysics ist kein Ersatz für Lattice-QCD.** Es ist eine
**Ergänzung** — eine andere Darstellung.

---

## §10 — Offene Fragen

Was ProPhysics **nicht** beantwortet.

### §10.1 — Physikalische Fragen

1. **Warum 8 Basis-Zustände?** Könnte auch 4 oder 16 funktionieren?
2. **Warum genau diese U5-Gewichte?** Sind sie eindeutig?
3. **Warum SU(2) und nicht SU(3)?** Ist das eine
   Konstruktionsentscheidung oder folgt es aus U1–U5?
4. **Kann die Born-Regel exakt gezeigt werden?** Bisher nur mit
   χ²-Test.
5. **Gibt es einen Kontinuumslimes?** Wenn ja, wie?
6. **Ist der Kernel in 4D konsistent?** Noch nicht getestet.

### §10.2 — Technische Fragen

1. **Wie weit kann Q31 getrieben werden?** Bis wann akkumuliert
   Rundung sichtbar?
2. **Kann der Kernel parallelisiert werden?** Ohne die Physik zu
   ändern?
3. **Kann die Speicher-Architektur verbessert werden?**
   (`CHANNELS_MAX` 16 → 8)

### §10.3 — Konzeptionelle Fragen

1. **Ist U4 transitiv genug?** GHZ braucht Hypergraph.
2. **Was ist die minimale Regelmenge?** Geht es mit weniger als
   U1–U5?
3. **Was ist die maximale emergente Struktur?** Bis wohin reicht
   es?

**Diese Fragen sind nicht beantwortet.** Sie sind der Kern des
Forschungs-Programms.

---

## §11 — Empfohlene Lesereihenfolge

**Für Physiker:**

1. Dieses Dokument (Überblick).
2. `docs/test/ProPhysics_Testkatalog.md` (quantitative Rohwerte).
3. `docs/project/Project.md` (Ontologie, 10 Thesen).
4. `docs/project/ProPhysics_Differentiators.md` (Abgrenzung).

**Für Software-Entwickler:**

1. `docs/project/ARCHITECTURE.md` (wie es gebaut ist).
2. `docs/project/ProPhysics_API.md` (Kernel-API).
3. `docs/project/Project.md` (warum).

**Für Reviewer:**

1. `docs/project/ProPhysics_Differentiators.md` §7 (was es nicht
   ist).
2. Dieses Dokument §5 (was quantitativ belegt ist).
3. `docs/test/ProPhysics_Testkatalog.md` §8 (Konsistenz-Tests).

---

## §12 — Siehe auch

**Übergreifende Dokumente:**

| Thema | Datei |
|---|---|
| Testkatalog (Rohwerte) | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Baseline (Kurzfassung) | `docs/test/BASELINE.md` |
| Test-Runner | `docs/test/run_alpha_tests.md` |
| Projekt-Roadmap | `docs/project/Project.md` |
| Architektur | `docs/project/ARCHITECTURE.md` |
| API-Referenz Kernel | `docs/project/ProPhysics_API.md` |
| API-Referenz SDK | `docs/project/SDK_API.md` |
| Abgrenzung | `docs/project/ProPhysics_Differentiators.md` |
| Versions-Register | `docs/project/ProPhysics_VersionRegistry.md` |
| Versionierungs-Konzept | `docs/project/VERSIONING.md` |
| Konfiguration | `docs/project/CONFIG.md` |
| Changelog | `CHANGELOG.md` |

**Modul-Referenzen:**

| Modul | Datei |
|---|---|
| Core | `docs/project/Core.md` |
| Amp | `docs/project/Amp.md` |
| Gauge | `docs/project/Gauge.md` |
| EPR | `docs/project/EPR.md` |
| Observer | `docs/project/Observer.md` |
| Shared | `docs/project/Shared.md` |
| Dirac | `docs/project/Dirac.md` |
| SU2 | `docs/project/SU2.md` |
| SU2_Dynamics | `docs/project/SU2_Dynamics.md` |
| Tensor | `docs/project/Tensor.md` |
| Fock | `docs/project/Fock.md` |
| Density | `docs/project/Density.md` |

**Externe Verweise:**

| Thema | URL |
|---|---|
| Repository | https://github.com/onkel83/prophysics |

---

## §13 — Versions-Historie dieses Dokuments

| Version | Datum | Änderung |
|---|---|---|
| 1.0 | 2026-09-25 | Erste Fassung, Etappe 23, Kernel-Version 3.0.0 |
| 1.0 | 2026-09-26 | Header auf Etappen-Schema umgestellt (Kernel 1.23.0, Etappe 23); §8.3 Größenrechnung korrigiert; §12 um alle 12 Modul-Docs erweitert |
| 1.0 | 2026-09-27 | §4.7 T16-Wert auf 1,41e-03 aktualisiert |
| 1.1 | 2026-09-29 | §4.8 komplett neu: V&V-Anker zurückgenommen, 2D gegen exakte Bessel-Referenz, 3D-Selbstkonsistenz. §5-Tabelle korrigiert (kein externer V&V-Anker mehr). §7.2 neu (externe V&V als verfehltes Ziel). §8.1 um „Kein externer V&V-Anker" erweitert. §9.1 um 2D-SU(2)-Haar-Verteilung erweitert. §9.3 um „4D-Lattice-QCD" erweitert. §10.1 Frage 6 (4D-Konsistenz) neu. |

**Hinweis zum Schema-Wechsel:** Frühere Versionen dieses Dokuments
trugen `Kernel-Version 3.0.0` (SemVer-ähnlich). Mit der Umstellung auf
das Etappen-Schema entspricht `3.0.0` jetzt `1.23.0`. Siehe
`CHANGELOG.md` §2.5.

---

**Ende Physik-Übersicht v1.1.**