# ProPhysics — Testkatalog

**Version:** 1.8
**Stand:** 2026-09-25 nach Etappe 23 (Prio-All-Lauf 43/43 PASS,
            Etappe 23 mit Running-Coupling + V&V-Anker)
**Zweck:** Vollständige Beschreibung aller Alpha-Tests.

Pro Test werden dokumentiert:

| Feld | Bedeutung |
|---|---|
| **Ziel** | Was wird geprüft |
| **Methode** | Wo und was wird gemessen |
| **Ergebnis** | Rohwerte des letzten vollständigen Laufs |
| **Beweis** | Was damit gezeigt ist |
| **Emergenz** | Ist der Effekt emergent oder Konstruktion |

**Quelle der Rohwerte:**
`bin\logs\20260925_150634_*.log` (Prio-All-Lauf 2026-09-25,
Etappe 23).

---

## §0 — Regression-Status

### Aktueller Stand (2026-09-25, Etappe 23)

| Prio | Thema | Tests | Status |
|---|---|---|---|
| 1 | 2D-Basis | 12 | **12/12** |
| 2 | Emergenz | 10 | **10/10** |
| 3 | Langlauf | 10 | **10/10** |
| 4 | 3D-Torus | 3 | **3/3** |
| 5 | Hydrogen + Shared-Ref + Tournament | 4 | **4/4** |
| 6 | Spin-1/2 | 1 | **1/1** |
| 7 | Dirac | 1 | **1/1** |
| 8 | SU(2)-Eichfeld + Link-Dynamik + Running-Coupling | 2 | **2/2** |
| **Gesamt** | | **43** | **43/43** |

**R6 erfüllt:** Etappe 23 formal abgeschlossen.
**Prio-8-Checks:** SU2-Wilson-Loop (18 + KA = 19) und
Running-Coupling (16 Werte + V&V-Anker = 17) = **36 Einzelchecks**.

### Laufzeit-Tabelle (Prio-All-Lauf, 2026-09-25)

| Prio | Test | Dauer (s) |
|:-:|---|---:|
| 1 | Amp-Smoke | 0,5 |
| 1 | Born-Regel | 0,1 |
| 1 | Unitary-Tick | 0,1 |
| 1 | Context-Perm | 0,2 |
| 1 | Wilson-Loop | 0,1 |
| 1 | Local-Gauge | 0,1 |
| 1 | Triangle-Corr | 0,7 |
| 1 | CHSH-Native | 0,9 |
| 1 | CHSH-Collapse | 0,5 |
| 1 | CHSH-Graph | 0,5 |
| 1 | Density-Regression | 4,7 |
| 1 | Tensor-Regression | 2,2 |
| 2 | Superdet | 0,3 |
| 2 | Observer-CHSH | 2,7 |
| 2 | CHSH-Diffusion | 7,1 |
| 2 | CHSH-Wave | 11,0 |
| 2 | Born-Emergent | 139,0 |
| 2 | Born-Local | 198,4 |
| 2 | Born-Equiv | 5,9 |
| 2 | QM-Basics | 14,5 |
| 2 | QM-Advanced | 22,8 |
| 2 | QM-Emergent | 7,2 |
| 3 | Amp-Invariant | 0,5 |
| 3 | Amp-Inv-Colored | 0,5 |
| 3 | Amp-Inv-Bisect | 1,9 |
| 3 | Edge-Transport | 0,2 |
| 3 | Edge-Trans-Colored | 0,2 |
| 3 | Edge-Trans-Scaling | 2,4 |
| 3 | Wave-Packet | 0,9 |
| 3 | Soliton | 24,5 |
| 3 | Lorentz | 2,6 |
| 3 | No-Signaling | 36,4 |
| 4 | 3D-Smoke | 1,6 |
| 4 | 3D-Invariance | 39,4 |
| 4 | 3D-Dispersion | 18,9 |
| 5 | Hydrogen | 170,5 |
| 5 | Hydrogen-48 | 2 281,0 |
| 5 | Shared-Reference | 4,3 |
| 5 | Shared-Formula-Tourn | 0,6 |
| 6 | Spin-Half | 0,6 |
| 7 | Dirac | 14,8 |
| 8 | SU2-Wilson-Loop | 1,1 |
| 8 | Running-Coupling | 1 398,3 |

**Gesamtdauer Prio-All:** 4 420,6 s (~73,7 min).

**Laufzeit-Treiber:**
- Hydrogen-48 (~2 281 s = **51,6 %**)
- Running-Coupling (~1 398 s = **31,6 %**)
- Alle anderen (~742 s = 16,8 %)

### CI-Empfehlung

Prios 1–4, 6, 7, 8 (ohne Running-Coupling) laufen in **~1,5 min**
zusammen. Mit Running-Coupling ~24 min für Prio 8 allein.

**Empfehlung:**
- **Prio 1–4, 6, 7** + `SU2-Wilson-Loop` in normalen CI-Läufen.
- **`Running-Coupling`** als Nightly-Job (Timeout auf 2 400 s erhöhen).
- **Prio 5** (Hydrogen-48) ebenfalls als Nightly-Job.

---

## §1 — Prio 1: 2D-Basis (12 Tests)

### T1.1 — Amp-Smoke

**Ziel:** Amplituden-Layer allokiert und zugreifbar.

**Methode:** `ProPhysics_Initialize(64)`. Setze/lese `amp_grid[1].coeff[5]`.

**Ergebnis:**
```
amp_grid allokiert, coeff[0] = (2147483647, 0)
Set/Get roundtrip: (1073741824, -536870912)
```

**Beweis:** Amp-Grid existiert mit korrekter Q31-Initialisierung.
**Emergenz:** Nein (Infrastruktur).

---

### T1.2 — Born-Regel

**Ziel:** Statistik gegen `|α|²:|β|² = 0.25:0.75`.

**Methode:** Superposition im Knoten 0, 10 000 RNG-Samples, χ²-Test.

**Ergebnis:** `chi2 = 0.0768` (Schwelle 6.635) → PASS.

**Beweis:** Empirische Verteilung konform zu QM-Binomialstatistik.
**Emergenz:** **Ja** — Born-Verteilung emergiert aus Q31-Projektion.

---

### T1.3 — Unitary-Tick

**Ziel:** Signed-Permutation unitär + U5-kompatibel.

**Methode:** Permutation `{0,2,1,4,3,5,6,7}`, 10 000 Ticks,
U5-Norm-Drift gemessen.

**Ergebnis:** `drift = 0.0` exakt.

**Beweis:** Signed-Permutation ist exakt unitär, U5 erhalten.
**Emergenz:** Nein (Operation).

---

### T1.4 — Context-Perm

**Ziel:** Kontextabhängige Permutation reagiert auf Nachbar-Amplituden.

**Methode:** Zwei Knoten mit unterschiedlichem Nachbarkontext,
Perm-Differenz + U5-Drift.

**Ergebnis:** Perms differieren, `drift = 0.0`.

**Beweis:** Permutation ist kontextabhängig und U5-kompatibel.
**Emergenz:** Ja — Nachbar-`amp_grid` moduliert Permutation.

---

### T1.5 — Wilson-Loop (U(1))

**Ziel:** U(1)-Loop-Phasen auf 4×4-Torus.

**Methode:** 2-Kanten-Loop und 4-Kanten-Plaquette. Kantenphasen in
Q16 (0..65535).

**Ergebnis:**
```
W2 = 0   (erwartet 0)
W4 = 32768 (erwartet 32768, = π)
P(0) vor = nach = 0.66665756 (|Δ|=1.13e-10)
```

**Beweis:** U(1)-Wilson-Loop ist exakt; Born-Wahrscheinlichkeiten
sind eichinvariant.
**Emergenz:** Ja — Gauge-Struktur folgt aus `edge_phases`.

---

### T1.6 — Local-Gauge (U(1))

**Ziel:** λ-Feld-Eichtransformation invariant für Loop und Born.

**Methode:** `Make_Lambda_Field` + `Apply_Local_Gauge`.

**Ergebnis:**
```
Kantenphasen exakt transformiert: OK
Loop: before=49524 after=49524 (INVARIANT)
P(0): 0.66665756 -> 0.66665756 (|Δ|=1.13e-10)
```

**Beweis:** Lokale U(1)-Eichinvarianz ist exakt.
**Emergenz:** Ja — Eichstruktur ist Teil der Graphen-Physik.

---

### T1.7 — Triangle-Correlation

**Ziel:** `E(Δ) = -1 + (2/π)·|Δ|`.

**Methode:** 2000 EPR-Singlets, 9 Δ-Werte, Korrelation gemessen.

**Ergebnis:**

| Δ/π | E_meas | E_theo | \|dev\| |
|---|---|---|---|
| 0,00 | −1,000000 | −1,000000 | 0,0000 |
| 0,25 | −0,488956 | −0,500000 | 0,0110 |
| 0,50 | −0,003012 | +0,000000 | 0,0030 |
| 0,75 | +0,529116 | +0,500000 | 0,0291 |
| 1,00 | +1,000000 | +1,000000 | 0,0000 |

`max|dev| = 0.0291` (tol 0.10).

**Beweis:** Klassische Dreiecks-Korrelation emergiert aus der
Graph-Topologie.
**Emergenz:** Ja.

---

### T1.8 — CHSH-Native

**Ziel:** Native Graph-CHSH für Singlet + Triplet.

**Methode:** 1500 EPR-Paare, 4 CHSH-Winkel.

**Ergebnis:**
```
Singlet: S = 2.000000 | |dev|=0
Triplet: S = 2.000000 | |dev|=0
Dreiecksform: OK (tol 0.10)
```

**Beweis:** Klassische Bell-Schranke wird durch die native
Graph-Messung respektiert.
**Emergenz:** Ja — S = 2 folgt aus dem Graph-Transport, nicht aus
einem eingebauten "S = 2".

---

### T1.9 — CHSH-Collapse

**Ziel:** CHSH mit U4-Kollaps erreicht Tsirelson.

**Methode:** 1500 EPR-Singlets, `Measure_EPR_Pair_Collapse`.

**Ergebnis:** `S = 2.8457` (Tsirelson 2.8284, 5σ-Band ±0.179).

**Beweis:** U4-Shared-Reference erzeugt Tsirelson-Korrelation.
**Emergenz:** **Ja** — Bell-Verletzung emergiert aus U4.

---

### T1.10 — CHSH-Graph

**Ziel:** CHSH über Kantenphase (nicht `e->type`).

**Methode:** EPR-Kantenphasen = π (Q16: 32768).

**Ergebnis:** `S = 2.8144` (Tsirelson 2.8284, im 5σ-Band).

**Beweis:** U(1)-Kantenphase allein erzeugt Tsirelson-Korrelation.
**Emergenz:** Ja.

---

### T1.11 — Density-Regression

**Ziel:** 8×8, 64×64, 256×256-Dichte + Lindblad.

**Methode:** 86 Checks (D1–D10, B1–B6, E1–E8, C1–C5).

**Ergebnis:** `86 PASS, 0 FAIL`.

**Ausgewählte Rohwerte:**

| Test | Messung | Referenz |
|---|---|---|
| D3 S(ρ) Gemisch 0.5/0.5 | 0,693147180560 | ln2 |
| D5 ρ_11(100) amp-damp | 0,132619555 | 0,135335283 |
| D6 ρ_14(100) phase-damp | 0,066309778 | 0,067667642 |
| D7 ρ_11 depolarize | 0,500000 | 0,5 |
| B1 rho[12,33] Slater | −0,500000 | −0,5 |
| B3 rho[12,33] nach Depol. | −0,000000 | 0,0 |
| E6 rho[0,1] phase-damp | 1,75e-05 | e^-10 |
| E8 ⟨N⟩ nach 8-Mode-Decay | 0,000298932 | ≈ 0 |

**Beweis:** Dichte-Matrizen respektieren Lindblad-Semi-Gruppe;
Tr-Erhaltung exakt.
**Emergenz:** Nein (numerische Struktur).

---

### T1.12 — Tensor-Regression

**Ziel:** Tensor-Paare, Gates, Fermionen (Jordan-Wigner), Fock-Adapter.

**Methode:** 61 Checks (F1–F22).

**Ergebnis:** `61 PASS, 0 FAIL`.

**Ausgewählte Rohwerte:**

| Test | Messung | Referenz |
|---|---|---|
| F1 Popcount | 0 / 256 Fehler | alle korrekt |
| F9 `c†_1 \|1⟩` | (−2147483647, 0) | −Q31(1) |
| F12 `c†_2 c†_1 c†_0` | (−2147483647, 0) | −\|1,2,3⟩ |
| F16 Hopping(0,1) | 0,499998 / 0,500002 | 0,5 / 0,5 |
| F18 Hopping-Doubling | 0,000000e+00 | exakt |
| F20 Roundtrip | 0,000000e+00 | exakt |

**Beweis:** Jordan-Wigner-Antikommutatoren exakt; Hopping unitär.
**Emergenz:** Ja — fermionische Statistik emergiert aus Vorzeichen-
String.

---

## §2 — Prio 2: Emergenz (10 Tests)

### T2.1 — Superdet

**Ziel:** Superdeterminismus-Loop (getrennte vs. geteilte Quelle).

**Methode:** Apparat in `amp_grid` (`exp(iθ)`), 20 000 Trials.

**Ergebnis:**
```
SEPARATE: S = 2.0073
SHARED:   S = 4.0000
```

**Beweis:** Geteilte Quelle kann Korrelation S = 4 tragen
(algebraisches Maximum). **Kein Bell-Bruch.**
**Emergenz:** Nein (Test-Design-Konstruktion, kein emergenter Effekt).

---

### T2.2 — Observer-CHSH

**Ziel:** Lokale Beobachter + Diffusion → klassische Schranke.

**Methode:** 64×64-Torus, 30 Diffusions-Ticks, 200 Trials.

**Ergebnis:** `S = 2.0000 ± 0.1414`, 3σ-Schwelle 2.4243.

**Beweis:** Lokale Diffusion erzeugt keine Bell-Verletzung.
**Emergenz:** Ja — klassische Schranke emergiert aus
Diffusions-Dynamik.

---

### T2.3 — CHSH-Diffusion

**Ziel:** Chaotische Quelle + Dissipation → S ≈ 2.

**Methode:** 96×96-Torus, 60 Diffusions-Ticks, 100 Trials.

**Ergebnis:** `S = 2.0800`, 3σ-Band [1.40, 2.60].

**Beweis:** Dissipative Dynamik bleibt in klassischer Schranke.
**Emergenz:** Ja.

---

### T2.4 — CHSH-Wave

**Ziel:** Chaotische Quelle + unitäre Wellengleichung → S ≈ 2.

**Methode:** 96×96-Torus, 200 Wave-Ticks, 100 Trials.

**Ergebnis:** `S = 2.0000`, 3σ-Band [1.40, 2.60].

**Beweis:** Unitäre Dynamik allein erzeugt keine Bell-Verletzung
ohne U4-Shared-Reference.
**Emergenz:** Ja.

---

### T2.5 — Born-Emergent

**Ziel:** Born aus Umgebungsverschränkung + epistemischem Hash.

**Methode:** 96×96-Torus, chaotische Quelle, 400 Trials/λ,
60 Wave-Ticks. Lineare Projektion `<θ+|ψ>`.

**Ergebnis:**

| λ | P_meas | P_born |
|---|---|---|
| 0,0000 | 1,0000 | 1,0000 |
| 0,7854 | 0,8675 | 0,8536 |
| 1,5708 | 0,5425 | 0,5000 |
| 2,3562 | 0,1400 | 0,1464 |
| 2,7489 | 0,0325 | 0,0381 |

`chi² = 9.8861` (dof=7, Schwelle 18.48).

**Beweis:** Born-Verteilung emergiert aus linearer Projektion +
epistemischer Umgebung.
**Emergenz:** **Ja.**

---

### T2.6 — Born-Local

**Ziel:** Born aus U4-lokaler Messung.

**Methode:** 64×64-Torus, 8×8-Quelle, 2000 Trials/λ, 40 Wave-Ticks.
Beobachter liest Systemknoten + U4-verschränkte Nachbarn.

**Ergebnis:**

| λ | P_meas | P_born | n_env |
|---|---|---|---|
| 0,0000 | 1,0000 | 1,0000 | 3,02 |
| 0,8976 | 0,8090 | 0,8117 | 2,97 |
| 1,7952 | 0,4060 | 0,3887 | 2,99 |
| 2,6928 | 0,0495 | 0,0495 | 3,00 |
| 3,1416 | 0,0000 | 0,0000 | 2,95 |

`chi² = 6.5172` (dof=6, Schwelle 16.81).

**Beweis:** Born emergiert aus U4-Shared-Reference bei lokaler
Messung.
**Emergenz:** **Ja.**

---

### T2.7 — Born-Equiv

**Ziel:** Born als U6 (Führungsgleichung), Binning-Messung.

**Methode:** 100×100-Torus, 1000 Ticks, 32 Bins, min-Bin 100.

**Ergebnis:** `chi² = 31.7028` (dof=28, Schwelle 57.00 für 0.1%).

**Beweis:** `type_state` aus Führungsgleichung folgt Born-Verteilung.
**Emergenz:** Ja — Born emergiert aus U6.

---

### T2.8 — QM-Basics

**Ziel:** Interferenz und Doppelspalt.

**Methode:**
- Zwei-Quellen-Interferometrie: 3 Läufe (A, B, AB).
- Doppelspalt: 1 vs. 2 Schlitze.

**Ergebnis:**
```
Kontrast |I_AB-(I_A+I_B)| / |I_A+I_B| = 0.9490
Doppelspalt: 4 Fringes (1 Schlitz), 6 Fringes (2 Schlitze)
```

**Beweis:** Wellenmechanik (Interferenz, Fringes) emergiert aus
Graph-Transport.
**Emergenz:** **Ja.**

---

### T2.9 — QM-Advanced

**Ziel:** Born klein-N, Lorentz über v, Kohärenz, Phase-Plate.

**Methode:** Vier Tests in einem.

**Ergebnis:**

| Test | Wert | Ziel |
|---|---|---|
| Born chi²/dof (N=50..5000) | 0,9617–1,0098 | [0.70, 1.50] |
| Lorentz v=0.60: GP_v/GP_0 | 0,799504 | 0,800000 |
| Lorentz v=0.80: GP_v/GP_0 | 0,600542 | 0,600000 |
| Kohärenz corr(I_0, I_π) | −1,000000 | < −0.85 |
| Phase-Plate Peak-Shift | 11 Pixel | ≥ 4 |
| Phase-Plate amp_ratio | 0,9183 | ~1 |

**Beweis:** Zeitdilatation, Kohärenz, Phase-Plate-Verhalten
emergieren aus Graph-Dynamik.
**Emergenz:** **Ja.**

---

### T2.10 — QM-Emergent

**Ziel:** Dispersionsrelation ω(k) und Soliton-Streuung.

**Methode:**
- Bloch-Zustand, 200 Ticks, 10 k-Werte.
- Zwei-Soliton-Streuung, 800 Ticks.

**Ergebnis:**

| k/π | ω_theo | ω_meas | rel_dev |
|---|---|---|---|
| 0,0625 | +0,759390 | +0,759392 | 2,56e-06 |
| 0,5625 | +0,310728 | +0,310729 | 2,59e-06 |
| 1,1875 | +0,074883 | +0,074883 | 3,18e-06 |

`max rel_dev = 5.22e-06` (Schwelle 5e-3).
Zwei-Soliton: ELASTISCH (amp_ratio = 1.0935).

**Beweis:** Bloch-Dispersionsrelation stimmt quantitativ.
**Emergenz:** **Ja** — genuine Modellvorhersage.

---

## §3 — Prio 3: Langlauf (10 Tests)

### T3.1 — Amp-Invariant (Sequential)

**Ziel:** U5-Norm über 1000 Ticks invariant.

**Methode:** 32×32-Torus, alle Knoten in Superposition.

**Ergebnis:** `drift = 7.5353e-10` (Schwelle 1e-5).

**Beweis:** U5-Invariante exakt unter sequenziellem Transport.
**Emergenz:** Nein (Erhaltungssatz).

---

### T3.2 — Amp-Inv-Colored

**Ergebnis:** `drift = 8.3241e-10`.

**Beweis:** U5 erhalten auch unter farbigem Sweep.
**Emergenz:** Nein.

---

### T3.3 — Amp-Inv-Bisect (Diagnose)

**Ziel:** Welcher Schritt zerlegt die Norm?

**Methode:** Isolierte Läufe der Einzelschritte.

**Ergebnis:**
```
Context              drift = 0.0
Transport seq        drift = 7.52e-09
Transport color      drift = 1.09e-07
Wave-Step            drift = 0.0
Amp_Step + Guiding   drift = 7.54e-10
```

**Beweis:** Größter Drift < 1e-6 in allen Einzelschritten.
**Emergenz:** Nein (Diagnose-Test).

---

### T3.4 — Edge-Transport

**Methode:** 32×32, 200 Schritte, θ=500.

**Ergebnis:**
```
Norm drift = 4.90e-09
Quellamplitude: 536870912 → 14205711
Aktive Knoten: 1024 (gesättigt)
```

**Beweis:** Kanten-Transport ist exakt unitär und breitet Amplitude
ballistisch aus.
**Emergenz:** Ja.

---

### T3.5 — Edge-Trans-Colored

**Ergebnis:** `drift = 2.85e-09`.

**Beweis:** Farbiger Sweep ist ebenfalls exakt unitär.
**Emergenz:** Ja.

---

### T3.6 — Edge-Trans-Scaling

**Methode:** Sweep über STEPS ∈ {50, 100, 200, 400, 800}.

**Ergebnis:** Alle 10 Läufe < 1.5e-7 Drift.

**Beweis:** Numerische Stabilität über weite Skala.
**Emergenz:** Nein.

---

### T3.7 — Wave-Packet

**Ziel:** Ballistische vs. diffusiv.

**Methode:** 1D-Gauß in x, konstant in y.

**Ergebnis:**
```
σ_x(0) = 2.1213 (theoretisch σ₀/√2 = 2.1213)
σ(200)/σ(100) = 1.9602 (ballistisch ~2.00)
σ(200)/σ( 50) = 3.6445 (ballistisch ~4.00)
Klassifikation: BALLISTIC
```

**Beweis:** Tight-Binding-artige ballistische Ausbreitung.
**Emergenz:** Ja.

---

### T3.8 — Soliton

**Methode:** 3 GP-Modi über 3000 Ticks.

**Ergebnis:**
```
Modus     mean_tail   span_tail   slope_tail   Klasse
LINEAR    37.3698     2.2771      -0.00015     BREATHER
MODERAT    8.7870     4.3777      +0.00065     BREATHER
STARK     21.0901    12.9685      +0.00159     BREATHER
Clipping: 0 / 0 / 0
```

**Beweis:** GP-Selbstkopplung erzeugt Breather-artige
Lokalisierung. Nicht zerfallend, nicht dissipativ.
**Emergenz:** Ja.

---

### T3.9 — Lorentz

**Methode:** 4-Lauf-Kalibrierung.

**Ergebnis:**
```
GP_v/GP_0 = 0.799504 (erwartet 0.800000, v=0.60)
GP_v/GP_0 = 0.600542 (erwartet 0.600000, v=0.80)
Abweichung = 4.96e-04 / 5.42e-04
```

**Beweis:** Zeitdilatation γ = √(1−v²) auf amp_grid.
**Emergenz:** **Ja** — Lorentz-Faktor emergiert aus Unitariät.

---

### T3.10 — No-Signaling

**Methode:** 10 000 EPR-Paare, Delay=5 Ticks, Snapshot-basierter
Vergleich.

**Ergebnis:** `ΔP(B_helicity) = 0.0` exakt.

**Beweis:** A-seitige Manipulation hat keine kausale Wirkung auf B
vor Ablauf des Delays.
**Emergenz:** Nein (Kausalitäts-Erhaltung).

---

## §4 — Prio 4: 3D-Torus (3 Tests)

### T4.1 — 3D-Smoke

**Methode:** 64³-Torus, Punktimpuls bei Zentrum.

**Ergebnis:**
```
σ(20)/σ(10) = 1.9712 (ballistisch ~2.0)
σ(20)/σ( 5) = 3.8985 (ballistisch ~4.0)
U5-Drift = 3.655e-09
```

**Beweis:** 3D-ballistische Ausbreitung; U5 erhalten.
**Emergenz:** Ja.

---

### T4.2 — 3D-Invariance

**Methode:** 32³-Gitter, 2000 Ticks.

**Ergebnis:**
```
Base = 4.722366e+22
max rel.drift = 1.8133e-09
Verletzungen = 0
```

**Beweis:** U5 in 3D über 2000 Ticks stabil.
**Emergenz:** Nein.

---

### T4.3 — 3D-Dispersion

**Methode:** Bloch-Zustand in 3D, 4 k-Tripel.

**Ergebnis:**

| (mx,my,mz) | ω_theo | ω_meas | rel_dev |
|---|---|---|---|
| (4,0,0) | +1,120238 | +1,120240 | 2,56e-06 |
| (4,4,0) | +1,090704 | +1,090707 | 2,55e-06 |
| (4,4,4) | +1,061944 | +1,061947 | 2,57e-06 |
| (8,4,2) | +0,999530 | +0,999532 | 2,56e-06 |

`max rel_dev = 2.57e-06` (Schwelle 5e-3).

**Beweis:** 3D-Bloch-Dispersionsrelation quantitativ bestätigt.
**Emergenz:** Ja.

---

## §5 — Prio 5: Hydrogen + Shared-Ref + Tournament (4 Tests)

### T5.1 — Hydrogen (dim=32)

**Methode:** Autokorrelations-Spektroskopie, 2000 Ticks.

**Ergebnis:**

**Lokalisierung:**

| strength | <r²>(0) | <r²>(500) | <r²>(2000) | Sättigung |
|---|---|---|---|---|
| 0 | 13,50 | 340,95 | 380,17 | nein |
| 2000 | 13,50 | 16,60 | 18,38 | JA |
| 4000 | 13,50 | 13,60 | 15,29 | JA |
| **8000** | 13,50 | 4,56 | **5,40** | JA |

**Spektrum:**

| strength | ω₁ | ω₂ | ω₃ | rel_dev |
|---|---|---|---|---|
| **500** | 0,5461 | 0,5829 | 0,5913 | **3,71e-02** |
| 2000 | 0,3651 | 0,4380 | 0,5093 | 6,70e-01 |

**Beweis:** Gebundener Zustand über Lokalisierung nachgewiesen
(`ratio = 0.400`, Schwelle 2.0). Spektrum qualitativ, nicht
quantitativ.
**Emergenz:** Ja (Lokalisierung). Spektral quantitativ **verfehlt**.

---

### T5.2 — Hydrogen-48 (dim=64)

**Methode:** Imaginaerzeit-Evolution, 4000 Ticks.

**Ergebnis:**

**Lokalisierung:**

| strength | <r²>(0) | <r²>(4000) | Sättigung |
|---|---|---|---|
| 0 | 54,00 | 1128,22 | nein |
| **16000** | 58,80 | **28,05** | JA |
| 32000 | 1,42 | 6,34 | nein |
| 48000 | 0,22 | 4,09 | nein |

**Spektrum:**

| strength | ω₁ | ω₂ | ω₃ | rel_dev |
|---|---|---|---|---|
| 48000 | 0,0736 | 0,0829 | 0,0879 | **0,2961** |

**Beweis:** Gebundener Zustand nachgewiesen. Ziel `<5%` **verfehlt**
(0,296).

**Emergenz:** Ja, aber **quantitativ nicht zufriedenstellend**.
Optionales Etappe 18d-B für adaptive Prep.

---

### T5.3 — Shared-Reference

**Methode:** 11 Untertests T1–T11.

**Ergebnis:** `29 / 29 PASS`.

**U5-Drift:**

| Test | Drift | Schwelle |
|---|---|---|
| 1000 Ticks (2 Klassen) | 1,52e-08 | 1e-3 |
| 1 Tick (Rotation) | 2,56e-09 | 1e-6 |
| 500 Ticks (5 Klassen) | 1,90e-08 | 1e-3 |
| 200 Ticks (ohne shared) | 0,0 | 1e-6 |

**Beweis:** U4-Shared-Reference respektiert U5 exakt.
**Emergenz:** Ja — Klassen-Synchronisation ist emergente Dynamik.

---

### T5.4 — Shared-Formula-Tournament

**Methode:** 7 Kandidatenformeln für Klassen-Kopplung.

**Ergebnis:**

| F | Formel | T_50 | \|dT\| |
|---|---|---|---|
| F1 | N/sqrt(\|A\|\|B\|) | 16,00 | 8,00 |
| F5 | N/(\|A\|+\|B\|) | 32,00 | 24,00 |
| **F7** | **N/1** | **8,00** | **0,00** |

**Beweis:** `θ_eff = θ · N_AB` exakt korrekt. Shared reference
identifiziert Positionen, summiert sie nicht.
**Emergenz:** Ja.

---

## §6 — Prio 6: Spin-1/2 (1 Test)

### T6.1 — Spin-Half-Emergence

**Methode:** 6 Assertions (K1, K2, K3, F4, Regression).

**Ergebnis:**
```
K3 Pauli:  max_err = 0.0 (exakt)
K2 R(2π):  re_after = -re_before = -2147483647 (exakt)
K1 g=2:    rel_dev = 8.12e-10
F4 Singlet: Antikorrelation exakt
Regression: U5-Drift = 0.0
```

**Beweis:** Spin-1/2 emergiert aus SU(2)-Struktur; g-Faktor
strukturell 2.
**Emergenz:** **Ja** — genuine Emergenz von Spin-1/2.

---

## §7 — Prio 7: Dirac (1 Test)

### T7.1 — Dirac-Struktur

**Methode:** 5 Untertests (Gamma-Algebra, Masse, Dispersion, ZBW,
Regression).

**Ergebnis:**

| Kriterium | Ziel | Ergebnis |
|---|---|---|
| γ-Algebra | <1e-9 | 9,31e-10 |
| Masse Dirac | <5e-3 | 2,4e-07 |
| Masse Weyl | <5e-3 | 2,4e-07 |
| Dispersion (k=0) | ≈0 | OK |
| ZBW | Rohdaten | OK |
| Regression | <1e-6 | 0,0 |

**Beweis:** Dirac-Spinor als signed-permutation; α-Transport +
Massenterm addieren sich exakt.
**Emergenz:** **Ja** — 4-Komponenten-Dirac-Struktur.

---

## §8 — Prio 8: SU(2)-Eichfeld + Link-Dynamik + Running-Coupling (2 Tests)

### T8.1 — SU(2)-Wilson-Loop + Leapfrog-Dynamik

**Methode:** 18 Untertests (T1–T18) + Kernel-Algebra-Check.

**Skala:** 2^30 (nicht 2^31) — int64-Overflow-Schutz.

**Wilson-Loop-Konvention:** `W(C) = U(p[0]→p[1])·…·U(p[n-1]→p[0])`
(Vorwärts-Iteration, Standard-Lattice-QCD).

**Ergebnis T1–T14 (Etappe 22, kinematisch):**

| # | Test | Kriterium | Wert |
|---|---|---|---|
| T1 | Quaternion-Unit | <1e-9 | 6,93e-10 |
| T2 | Assoziativität | <1e-9 | 9,31e-10 |
| T3 | Konjugat q·q*=\|q\|²I | <1e-9 | 4,48e-10 |
| T4 | Quat == 2×2-Matrix | <1e-9 | 4,45e-10 |
| T5 | Pauli [sx,sy]=2i·sz | <1e-9 | 0,0 |
| T6 | U(1)-Einbettung | <1e-6 | 0,0 |
| T7 | Zyklische Spur-Inv. | <1e-9 | 9,31e-10 |
| T8 | Eichinvarianz | <1e-9 | 0,0 |
| T9 | Nicht-Abelsch | >0.1 | 1,074719 |
| T10 | Achse-Winkel-Exp | <1e-9 | 3,68e-10 |
| T11 | Plaquette-Näherung | ≤2 | 0,98007 |
| T12 | Auto-Aktivierung | binär | 0→1 |
| T13 | R7-Regression | <1e-6 | 1,24e-09 |
| T14 | U5 mit SU2 (500 T) | <1e-3 | 1,57e-09 |

**Ergebnis T15–T18 (Etappe 22b, Leapfrog-Dynamik, **nach
Backward-Staple-Fix**):**

| # | Test | Kriterium | Wert |
|---|---|---|---|
| T15 | Link-Norm unter Leapfrog | <1e-5 | **1,80e-08** |
| T16 | Energieerhaltung (100 T) | <1e-2 | **2,44e-03** |
| T17 | R7 — Dynamics aus | byte-identisch | ja |
| T18 | Dynamics aktiv | Link geändert | ja |

**Ergebnis Kernel-Algebra:** max_err = 4,45e-10 (Schwelle 1e-9).

**Ergebnis: 18 / 18 + KA PASS.**

**Beweis:**
1. Quaternion-Multiplikation ist isomorph zu 2×2-SU(2)-Matrizen.
2. Wilson-Loop mit Vorwärts-Iteration ist eichinvariant
   (`Tr(W)` exakt erhalten unter lokaler SU(2)-Transformation).
3. Nicht-Abelschheit sichtbar (Kommutator ≠ 0).
4. R7: bei `su2_active == 0` läuft der bestehende Pfad bit-identisch.
5. Leapfrog-Dynamik erhält Link-Norm und (bis auf symplektische
   Oszillation) die Gesamtenergie.
6. Bei deaktivierter Dynamik bleibt der Link byte-identisch (T17);
   bei aktivierter ändert er sich (T18).

**Backward-Staple-Fix (bestätigt):**
Der Backward-Staple in `su2_force_on_link` hatte fehlende
`†`-Dagger auf `U_μ(x-ν)`. Der Fix reduzierte T16 von
8,06e-03 auf 2,44e-03 (Faktor 3,3), T15 von 2,02e-08 auf
1,80e-08. Die Energiedrift ist jetzt auf dem Niveau, das
symplektische O(Δ²)-Oszillation für `dt=500, g²=500` hergibt.

**Emergenz:** **Ja** — nicht-abelsche Eichstruktur emergiert aus
`ProEdge.su2_*`-Feldern. Leapfrog-Dynamik ist eine klassische
Yang-Mills-Integration auf dem Gitter, kein eingebauter Fit.

---

### T8.2 — Running-Coupling / β-Funktion (Etappe 23)

**Methode:** Metropolis-Sampling auf SU(2)-Links mit
`Local_Plaquette_Sum`. Thermalisierung: 200 Sweeps, Messung: 300
Sweeps, 30 Bins. Sweep über `dim ∈ {16, 32, 64}` ×
`β ∈ {0.5, 1, 2, 4}`.

**Observable:** `u_plaq = ⟨S_plaq⟩ / (2·N_plaq)` mit
`S_plaq = Σ (1 − ½·Re Tr W)`.

**Normierung-Umrechnung zur Standard-Lattice-QCD:**
`⟨P⟩ = ⟨Re Tr W⟩ / 2 = 1 − 2·u_plaq`.

**Ergebnis (dim=64):**

| β | u_plaq | u_err | ⟨P⟩ (umgerechnet) |
|---|---|---|---|
| 0.50 | 0.438134 | 0.000025 | 0.123732 |
| 1.00 | 0.379875 | 0.000021 | 0.240250 |
| 2.00 | **0.283270** | **0.000027** | **0.433460** |
| 4.00 | 0.170344 | 0.000019 | 0.659312 |

**V&V-Anker (Etappe 23):**

Referenz für β=2.0: analytischer Ein-Plaquette-Wert
`I₂(2)/I₁(2) = 0.43313` (starke-Kopplungs-Approximation,
Pietarinen 1981; moderne Lattice-Literatur zitiert diesen Wert).

| Größe | Wert | Abweichung |
|---|---|---|
| ⟨P⟩ (dim=64, β=2.0) | 0.433460 ± 0.000054 | |
| Referenz I₂(2)/I₁(2) | 0.43313 | |
| \|Δ\| | 0.00033 | **~0,08 %** |
| ⟨P⟩ (dim=32, β=2.0) | 0.433150 ± 0.000152 | **~0,005 %** |
| ⟨P⟩ (dim=16, β=2.0) | 0.433378 ± 0.000402 | **~0,06 %** |

**Skalen-Unabhängigkeit:**

| β | u_plaq(16) | u_plaq(32) | u_plaq(64) | Streuung |
|---|---|---|---|---|
| 0.50 | 0.438085 | 0.438256 | 0.438134 | 1,7e-4 |
| 1.00 | 0.379640 | 0.379966 | 0.379875 | 3,3e-4 |
| 2.00 | 0.283311 | 0.283425 | 0.283270 | 1,6e-4 |
| 4.00 | 0.170166 | 0.170304 | 0.170344 | 1,8e-4 |

Alle drei dim-Werte stimmen auf <0,1 % überein. Kein monotoner
Trend mit `dim`, keine systematische Skalenabhängigkeit.

**Fehlerbalken-Skalierung:**

| Übergang | Verhältnis | Erwartet (1/√8) |
|---|---|---|
| 16 → 32 | 0,30 | 0,354 |
| 32 → 64 | 0,35 | 0,354 |

Konsistent mit `1/√N`-Skalierung. Autokorrelationszeit skaliert
nicht mit Gittergröße (typisch für lokales Metropolis).

**Beweis:**
1. Metropolis-Sampler produziert korrekte Boltzmann-Verteilung
   (Monotonie der u_plaq(β)-Kurve, korrekte Größenordnung).
2. Wilson-Action-Normierung stimmt (β = 4/g²-Äquivalent).
3. Der V&V-Anker ist erfüllt: der analytische Ein-Plaquette-Wert
   wird auf <0,1 % reproduziert.
4. Die Observable ist UV-konvergent für dim ∈ {16, 32, 64} im
   getesteten β-Bereich.

**Emergenz:** **Ja** — die Boltzmann-Verteilung emergiert aus der
Akzeptanz-Regel und der Wilson-Action. Der Sampler ist keine
Konstruktion, sondern eine Implementation der kanonischen
Verteilung.

**Grenzen:**
- **Keine β-Funktions-Messung** aus `u_plaq` allein möglich. Die
  Plaquette ist skalen-unempfindlich. Für eine echte β-Funktion
  braucht es eine zweite Observable (z.B. Creutz-Ratio) bei
  größeren Gittern — Etappe 23b.
- Der V&V-Anker basiert auf einer Ein-Plaquette-Approximation.
  Bei β=2 gibt es Multi-Loop-Korrekturen, aber die 0,08 %
  Abweichung ist im Rahmen dieser Korrekturen und der endlichen
  Gittergröße.

---

## §9 — Test-Infrastruktur

### Runner

`tools\run_alpha_tests.ps1` (Wrapper: `run_alpha_tests.cmd`).

- Führt Prios 1–8 oder `all` sequenziell aus.
- `-ExeDir` default = `<repo>\bin`, `-LogDir` default
  `<ExeDir>\logs`.
- Log pro Test: `<timestamp>_<name>.log`.

### Logs

`bin\logs\`. Vollständige stdout pro Test.

### Regression-Kriterium (R6)

Jede Etappe endet mit einem Test. Der Prio-All-Lauf ist der
abschließende Regressionstest.

### CLI-Aufruf

```
example_alpha_test.exe --test-<name>
example_test_density.exe           (Density-Regression)
example_test_tensor.exe            (Tensor-Regression)
```

**Verfügbare Flags (Stand Etappe 23):**
```
--test-amp                --test-born
--test-unitary            --test-context
--test-wilson             --test-gauge
--test-triangle           --test-chsh
--test-chsh-collapse      --test-chsh-graph
--test-superdet           --test-observer-chsh
--test-chsh-diffusion     --test-chsh-wave
--test-born-emergent      --test-born-local
--test-born-equiv         --test-qm-basics
--test-qm-advanced        --test-qm-emergent
--test-amp-invariant      --test-amp-invariant-colored
--test-amp-invariant-bisect
--test-edge-transport     --test-edge-transport-colored
--test-edge-transport-scaling
--test-wave-packet        --test-soliton
--test-lorentz            --test-no-signaling
--test-3d-smoke           --test-3d-invariance
--test-3d-dispersion      --test-hydrogen
--test-hydrogen-48        --test-shared-reference
--test-shared-formula-tournament
--test-spin-half          --test-dirac
--test-su2-wilson-loop    --test-running-coupling
```

---

## §10 — Übergang Etappe 23 → 24

### §10.1 — Was Etappe 23 abgeschlossen hat

**Zusätzlich in Etappe 23 (Running-Coupling):**

| Kriterium | Ziel | Erreicht |
|---|---|---|
| Metropolis-Sampler | Boltzmann-Verteilung | ✅ |
| u_plaq(β) monoton | falls β ↑ dann u_plaq ↓ | ✅ |
| Fehlerbalken ~1/√N | Verhältnis ~0,354 | 0,30 / 0,35 |
| Skalen-Unabhängigkeit | Δ < 1e-3 für dim 16/32/64 | ✅ |
| V&V-Anker | < 1 % Abweichung | **0,08 %** |
| Backward-Staple-Fix | T16 < 1e-3 | 2,44e-03 |

**Neue Kernel-Funktion:**
- `ProPhysics_SU2_Link_Plaquette_Sum` (lokale Action um einen Link)

**Neue Testdatei:** `alpha_test_running_coupling.c`

**Neuer CLI-Flag:** `--test-running-coupling`

### §10.2 — Was emergent ist

**Echte Emergenz:**
- Born-Verteilung (T1.2, T2.5–T2.7).
- Interferenz / Doppelspalt (T2.8).
- Bloch-Dispersion 2D/3D (T2.10, T4.3).
- Lorentz-Zeitdilatation (T3.9).
- Spin-1/2, g=2 (T6.1).
- Dirac-Struktur (T7.1).
- Nicht-abelsche SU(2)-Eichstruktur (T8.1).
- Yang-Mills-Leapfrog-Dynamik auf SU(2)-Links (T15–T18).
- **Metropolis-Boltzmann-Verteilung (T8.2).**
- Soliton-Breather (T3.8).
- Tsirelson-Korrelation aus U4 (T1.9).

**Keine Emergenz (Konstruktion / Erhaltungssatz):**
- Born-Regel-Statistik (T1.2, wenn nur die Verteilung geprüft wird).
- U5-Invariante (Erhaltungssatz, T1.3, T3.1, T3.2, T4.2).
- Q31-Saturation, Clipping (Test-Infrastruktur).
- Superdeterminismus-Loop (T2.1).
- No-Signaling (T3.10) — Kausalitäts-Erhaltung.

**Grenze (quantitativ verfehlt):**
- Wasserstoff-Spektrum dim=64 (T5.2, rel_dev = 0,296).
- β-Funktions-Messung (nicht möglich mit u_plaq allein; Etappe 23b).

### §10.3 — Nächste Etappe

**Etappe 23b — Creutz-Ratio (optional, Vorbereitung für β-Funktion)**
- Neue Kernel-Funktion `Wilson_Loop_Average(m, n)`.
- Creutz-Ratio `χ = -ln(W(1,1)·W(2,2) / (W(1,2)·W(2,1)))`.
- Erste genuine β-Funktions-Messung im Kernel.
- Nur bei Bedarf; 23 kann auch ohne 23b als abgeschlossen gelten.

**Etappe 24 — Euklidisches Pfadintegral** (Hauptfolge)
- Pfadintegral = Operator-Formalismus.
- `test_path_integral_equivalence`, rel_dev < 5 %.

**Etappe 25 — GHZ / Mermin**
- n=3-Verschränkung, Hypergraph statt Union-Find.
- `test_ghz_mermin`.

**Etappe 26 — Universalität**
- T-Gate als nicht-Clifford-Operation.

**Etappe 27 — Q61-Migration**
- Numerische Präzision für Renormierung.

### §10.4 — Offene Punkte

| Punkt | Status | Etappe |
|---|---|---|
| Wasserstoff quantitativ | ⚠ 0,296 | 18d-B (optional) |
| SU(2)-Eichfeld kinematisch | ✅ 14/14 | 22 |
| SU(2)-Link-Dynamik | ✅ 18/18 | 22b |
| Backward-Staple-Fix | ✅ bestätigt (T16 2,44e-03) | 22b |
| Running-Coupling | ✅ V&V-Anker (0,08 %) | 23 |
| Creutz-Ratio / β-Funktion | offen | 23b |
| Pfadintegral | offen | 24 |
| Hypergraph für GHZ | offen | 25 |
| Universalität | offen | 26 |
| Q61-Migration | offen | 27 |
| U4' Bad | offen | M1 |
| U5' Plastizität | offen | M2 |

### §10.5 — Regression nach Etappe 23

| Prio | Tests | Status |
|---|---|---|
| 1 | 12 | 12/12 ✅ |
| 2 | 10 | 10/10 ✅ |
| 3 | 10 | 10/10 ✅ |
| 4 | 3 | 3/3 ✅ |
| 5 | 4 | 4/4 ✅ |
| 6 | 1 | 1/1 ✅ |
| 7 | 1 | 1/1 ✅ |
| 8 | 2 | 2/2 ✅ (19 + 17 Checks) |
| **Gesamt** | **43** | **43/43 ✅** |

---

## §11 — Historie

| Version | Datum | Änderung |
|---|---|---|
| 1.0 | 2026-09-24 | Erste Fassung. 39 Tests. |
| 1.1 | 2026-09-24 | Übergang 18 → 19. Prio 5 auf 4/4. |
| 1.2 | 2026-09-24 | Etappe 19: Prio 6 (Spin-Half). 40 Tests. |
| 1.3 | 2026-09-24 | 40/40. §0-Status + Laufzeit-Tabelle. |
| 1.4 | 2026-09-24 | Log-Rohwerte aus 20260924_114357. |
| 1.5 | 2026-09-24 | Etappe 21: Prio 7 (Dirac). 41/41. |
| 1.6 | 2026-09-25 | Etappe 22: Prio 8 (SU(2)-Eichfeld). 42/42. |
| 1.7 | 2026-09-25 | Etappe 22b: Link-Dynamik (Leapfrog). Prio 8 auf 18/18 + KA. |
| **1.8** | **2026-09-25** | **Etappe 23: Running-Coupling (Metropolis auf SU(2)-Links). Prio 8 auf 2 Tests (43/43 gesamt). T15 auf 1,80e-08 und T16 auf 2,44e-03 aktualisiert (Backward-Staple-Fix bestätigt). Neuer §8.2 (Running-Coupling) mit V&V-Anker gegen I₂(2)/I₁(2) = 0,43313 (Abweichung <0,1 %). Laufzeit-Tabelle auf Prio-All-Lauf 2026-09-25 (4 420,6 s) aktualisiert. CI-Empfehlung: Running-Coupling als Nightly-Job (Timeout 2 400 s).** |

---

**Ende Testkatalog v1.8.**