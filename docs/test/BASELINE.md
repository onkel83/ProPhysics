# ProPhysics — Test-Baseline

**Datei:** `docs/test/BASELINE.md`
**Version:** 1.3
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-29 (nach Etappe 23c Rev.2, Patch 1.23.14 —
Sampler-Korrektur und V&V-Anker-Rücknahme)
**Zweck:** Referenz-Lauf der Tests mit Rohwerten. Dient als
Vergleichsanker für Regressionstests.

---

## §0 — Was dieses Dokument ist

Ein **Snapshot** des aktuellen Test-Suite-Ergebnisses (Kernel 1.23.0,
Etappe 23, Patch-Stand `1.23.14`). Wer eine Änderung am Kernel
vornimmt, kann seine Ergebnisse gegen diese Werte vergleichen.

**Nicht** in diesem Dokument:

- Test-Methodik → `docs/test/ProPhysics_Testkatalog.md`
- Test-Runner-Bedienung → `docs/test/run_alpha_tests.md`
- Changelog → `CHANGELOG.md`

**Quelle:** `bin\logs\<timestamp>_*.log` des letzten Prio-All-Laufes
plus der 23b/23c/23e-Läufe.

**Wichtige Änderung gegenüber v1.2:** Der frühere V&V-Anker
(„0,08 % gegen `I₂(2)/I₁(2)`") ist mit Patch `1.23.13` zurückgenommen
und mit `1.23.14` endgültig durch eine **korrekte 2D-Referenz**
(`I₂/I₁` statt `I₁/I₀`) und einen **Selbstkonsistenz-Test** für die
3D-String-Tension ersetzt. Siehe §2.7.

---

## §1 — Prio-Übersicht

| Prio | Thema | Tests | PASS |
|---|:-:|:-:|:-:|
| 1 | 2D-Basis | 12 | 12/12 |
| 2 | Emergenz | 10 | 10/10 |
| 3 | Langlauf | 10 | 10/10 |
| 4 | 3D-Torus | 3 | 3/3 |
| 5 | Hydrogen + Shared-Ref + Tournament | 4 | 4/4 |
| 6 | Spin-1/2 | 1 | 1/1 |
| 7 | Dirac | 1 | 1/1 |
| 8 | SU(2) + Running-Coupling + Creutz-Ratio + Metropolis-2D | 5 | 5/5 |
| **Gesamt** | | **46** | **46/46** |

**Prio-All-Laufzeit (ohne `Creutz-Ratio-Full` und `String-Tension-Full`):**
~4 530 s (~75,5 min).

**Laufzeit-Treiber:**

- Hydrogen-48 (~2 281 s = **50,4 %**)
- Running-Coupling (~1 398 s = **30,9 %**)
- Creutz-Ratio (Fast, ~110 s)
- Creutz-Ratio-Full (~2 400 s, **nur Nightly**)
- String-Tension (Fast, ~15 min)
- Metropolis-2D (Fast, ~3 min)
- Alle anderen (~850 s = 18,7 %)

---

## §2 — Numerische Anker (Rohwerte)

Auszug der kritischen Messwerte aus dem Baseline-Lauf. Vollständige
Rohwerte pro Test: siehe `docs/test/ProPhysics_Testkatalog.md`.

### §2.1 — Bloch-Dispersion

| Test | Wert |
|---|---|
| Bloch 2D `max rel_dev` | **5,22e-06** |
| Bloch 3D `max rel_dev` | **2,57e-06** |

### §2.2 — U5-Invariante

| Test | Kontext | Drift |
|---|---|---|
| 2D-Basis | 1000 Ticks | 7,54e-10 |
| 3D-Invariance | 2000 Ticks | 1,81e-09 |
| SU(2) | 500 Ticks, aktive Links | 1,57e-09 |
| Shared Reference | 1000 Ticks, 2 Klassen | 1,52e-08 |

### §2.3 — CHSH

| Variante | S |
|---|---|
| CHSH-Native (Singlet) | 2,0000 |
| CHSH-Collapse (U4) | 2,8457 |
| CHSH-Graph (Kantenphase) | 2,8144 |
| Tsirelson-Limit | 2,8284 |

### §2.4 — Algebra

| Test | Wert |
|---|---|
| γ-Algebra `max_err` | 9,31e-10 |
| Pauli `max_err` | 0,0 (exakt) |
| SU(2)-Quaternion `max_err` | 4,45e-10 |

### §2.5 — Lorentz

| v/c | GP_v/GP_0 gemessen | erwartet | Abweichung |
|---|---|---|---|
| 0,60 | 0,799504 | 0,800000 | 4,96e-04 |
| 0,80 | 0,600542 | 0,600000 | 5,42e-04 |

### §2.6 — SU(2)-Leapfrog

| Test | Wert |
|---|---|
| Link-Norm `max_dev` | 1,80e-08 |
| Energie-Drift (100 Ticks) | **1,41e-03** |

**Hinweis (1.23.10):** Der Wert für T16 (Energie-Drift) hat sich von
2,44e-03 auf **1,41e-03** verbessert (Faktor 1,7). Ursache: ein
Konjugations-Bug in `su2_plaquette_action_at` wurde gefixt.

### §2.7 — SU(2)-Metropolis: 2D-Referenz und 3D-Selbstkonsistenz

**WICHTIG — V&V-Anker-Rücknahme:**

Der ursprüngliche V&V-Anker aus `[1.23.0]` (0,08 % gegen
`I₂(2)/I₁(2)`) ist mit `[1.23.13]` und `[1.23.14]` **endgültig
zurückgenommen**. Ursache: Der Sampler `U → normalize(U + ε)` hatte
einen int32-Overflow bei ε > 1 und produzierte eine systematisch
verschobene Verteilung. Nach dem Wechsel auf den Haar-Vorschlag
`U → R·U` ist der tatsächliche Wert `⟨P⟩(β=2, dim=64) = 0,4549`
— ca. 5 % Abweichung zur SPA-Referenz, im Rahmen der erwarteten
Multi-Loop-Korrektur.

**2D-SU(2)-Referenz (exakt, dim=16, FAST):**

Der Metropolis-Sampler reproduziert die exakte Bessel-Ratio
`⟨½ Re Tr U⟩ = I₂(β)/I₁(β)`. Die alte Referenz `I₁(β)/I₀(β)` ist
die U(1)-Formel (ohne Haar-Maß `sin²(α)`) und war falsch.

| β | `<W>` gemessen | `I₂/I₁` (exakt) | rel_dev |
|---|---|---|---|
| 0.50 | 0,125213 ± 0,001275 | 0,123718 | 1,2 % |
| 1.00 | 0,240922 ± 0,001596 | 0,240194 | 0,3 % |
| 2.00 | 0,434476 ± 0,001493 | 0,433127 | 0,3 % |
| 4.00 | 0,655848 ± 0,001250 | 0,658047 | 0,3 % |

**Ergebnis: 4/4 PASS.** Alle Werte innerhalb 2σ.

**3D-SU(2)-String-Tension (Selbstkonsistenz, FAST):**

Fit: `ln W(m,n) = -σ_a2·(m·n) - μ_a·(m+n) + c`.
Fit-Filter: `W > 1e-3`, `rel.err < 0.5`.

| β | σ_a2 (dim=16) | σ_a2 (dim=32) | chi2/dof (dim=16) |
|---|---|---|---|
| 2.40 | 0,595053 ± 0,005997 | 0,594901 ± 0,002280 | 0,49 |
| 2.50 | 0,560160 ± 0,004610 | 0,557702 ± 0,001776 | 1,07 |
| 2.70 | 0,492428 ± 0,003875 | (nicht gemessen) | 0,85 |
| 3.00 | 0,401811 ± 0,002644 | (nicht gemessen) | 3,11 |

**Ergebnis:**

- σ_a2 fällt **streng monoton** in β (asymptotische Freiheit).
- dim=16 vs dim=32 stimmt bei β=2,40 auf **0,03 %**, bei β=2,50 auf
  **0,4 %** überein. Echte UV-Konvergenz.

**Kein externer V&V-Anker.** Die Referenzwerte von Cahill & Prasad
(1989) sind **4D-SU(2)**, der Kernel ist **3D**. Die Kopplungen sind
nicht vergleichbar. Die Abweichung um Faktor ~3 ist die 3D/4D-Differenz,
kein Kernel-Bug.

**Timeout-Hinweis:** Bei dim=32, 100 Bins, 4 β ist die Laufzeit
> 900 s. Für die Prio-All-Regression auf 30 Bins reduzieren oder
Timeout auf 3600 s erhöhen.

### §2.8 — Code-Qualität

| Prüfung | Ergebnis |
|---|---|
| Compiler-Warnungen (`/W4` Kernel, `/W3` SDK, `/W3` Test) | **0** |
| Linker-Fehler | 0 |
| Build-Zeit kompletter Rebuild | ~5 s |

---

## §3 — Was gegen diese Baseline geprüft wird

Bei jeder Änderung am Kernel:

1. **Test-Suite läuft 46/46 PASS** (keine Regression).
2. **Numerische Anker bleiben innerhalb ihrer Toleranzen** (siehe
   Testkatalog).
3. **2D-Metropolis** bleibt innerhalb 2σ zur exakten Bessel-Referenz.
4. **3D-String-Tension** bleibt konsistent über dim:
   dim-Konvergenz < 1 %.
5. **Creutz-Ratio-Konsistenz** bleibt erhalten:
   - χ > 0 für alle (dim, β),
   - χ monoton fallend in β,
   - χ dim-konsistent innerhalb 3σ.
6. **Compiler-Warnungen bleiben bei 0** bei `/W4` (Kernel) bzw.
   `/W3` (SDK/Test).

**Bit-Identität** ist nur bei deterministischen Tests garantiert
(Wave-Step und SU(2)-exp nutzen `cos`/`sin` — IEEE-754-konform auf
x86-64, aber nicht plattformübergreifend bit-identisch). Der
Creutz-Ratio-Test ist **deterministisch** (fester Seed pro
`(dim, β)`), sollte also bei gleichem Compiler/Build reproduzierbar
sein.

---

## §4 — Pflege

Diese Datei wird **bei jedem Etappen-Abschluss** aktualisiert. Die
Rohwerte kommen aus dem aktuellen Prio-All-Lauf.

**Bei Inkonsistenz:** Die Rohwerte in `ProPhysics_Testkatalog.md`
sind die ausführlichere Referenz. `BASELINE.md` ist eine Kurzfassung.

---

## §5 — Siehe auch

| Thema | Datei |
|---|---|
| Testkatalog (vollständig) | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Runner | `docs/test/run_alpha_tests.md` |
| Changelog | `CHANGELOG.md` |
| Physik-Übersicht | `docs/physics/README.md` |
| SU2-Modul | `docs/project/SU2.md` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende BASELINE v1.3.**
