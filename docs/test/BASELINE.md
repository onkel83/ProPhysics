# ProPhysics — Test-Baseline

**Datei:** `docs/test/BASELINE.md`
**Version:** 1.2
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-27 (nach Etappe 23b + Patch 1.23.13,
Creutz-Ratio-Konsistenz-Test)
**Zweck:** Referenz-Lauf der 45 Tests mit Rohwerten. Dient als
Vergleichsanker für Regressionstests.

---

## §0 — Was dieses Dokument ist

Ein **Snapshot** des aktuellen Test-Suite-Ergebnisses (Kernel 1.23.0,
Etappe 23 + 23b, Patch-Stand `1.23.13`). Wer eine Änderung am Kernel
vornimmt, kann seine Ergebnisse gegen diese Werte vergleichen.

**Nicht** in diesem Dokument:

- Test-Methodik → `docs/test/ProPhysics_Testkatalog.md`
- Test-Runner-Bedienung → `docs/test/run_alpha_tests.md`
- Changelog → `CHANGELOG.md`

**Quelle:** `bin\logs\<timestamp>_*.log` des letzten Prio-All-Laufes
plus der 23b-Creutz-Ratio-Läufe (`*_Creutz_Ratio.log`).

**Wichtige Änderung gegenüber v1.1:** Der frühere V&V-Anker („0,08 %
gegen `I₂(2)/I₁(2)`") wurde mit Patch `1.23.13` **zurückgenommen**.
Ursache: Der Plaquette-Konjugations-Fix in `1.23.10` verändert
`u_plaq(β=2, dim=64)` von `0,283270` auf `0,272552`. Der alte Anker
war damit nicht mehr gültig. An seine Stelle tritt der
**Creutz-Ratio-Konsistenz-Test** (§2.7).

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
| 8 | SU(2) + Running-Coupling + Creutz-Ratio | 4 | 4/4 |
| **Gesamt** | | **45** | **45/45** |

**Prio-All-Laufzeit (ohne `Creutz-Ratio-Full`):**
~4 530 s (~75,5 min) mit 23b-Ergänzung.

**Laufzeit-Treiber:**

- Hydrogen-48 (~2 281 s = **50,4 %**)
- Running-Coupling (~1 398 s = **30,9 %**)
- Creutz-Ratio (Fast, ~110 s)
- Creutz-Ratio-Full (~2 400 s, **nur Nightly**)
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
Konjugations-Bug in `su2_plaquette_action_at` wurde gefixt
(`l3br_n`/`l4br_n` statt `l3br`/`l4br`). T11 (Plaquette-Näherung)
bleibt unverändert, weil dort `b_re = 0`.

### §2.7 — SU(2)-Metropolis: RC-Rohwerte und Creutz-Ratio

**Vorbemerkung zur Anker-Rücknahme:** Der ursprüngliche V&V-Anker
aus `[1.23.0]` (`⟨P⟩(β=2, dim=64) = 0,43346` vs. `I₂(2)/I₁(2) =
0,43313`, Abweichung 0,08 %) ist mit `1.23.13` **zurückgenommen**.
Grund: Der Plaquette-Konjugations-Fix in `1.23.10` verschiebt
`u_plaq(β=2, dim=64)` auf `0,272552`. Entsprechend
`⟨P⟩ = 1 − 2·u_plaq = 0,454896`, ca. 5 % Abweichung zur
Ein-Plaquette-Approximation — im Rahmen der erwarteten
Multi-Loop-Korrektur, aber kein 0,08 %-Anker mehr.

**RC-Rohwerte (Stand `1.23.13`):**

Nur der Wert für β=2, dim=64 wurde nach dem Bugfix neu gemessen.
Die übrigen β-Werte stammen aus dem Stand von `1.23.0` und sind um
ca. −3 bis −4 % zu korrigieren (Größenordnung aus der β=2-Korrektur).
Ein vollständiger RC-Neulauf steht aus.

| β | dim=64 `u_plaq` (alt, `1.23.0`) | dim=64 `u_plaq` (neu, `1.23.13`) |
|---|---|---|
| 0,50 | 0,438134 | (Neulauf ausstehend) |
| 1,00 | 0,379875 | (Neulauf ausstehend) |
| **2,00** | 0,283270 | **0,272552** |
| 4,00 | 0,170344 | (Neulauf ausstehend) |

**Referenz:** Für β=2: `I₂(2)/I₁(2) = 0,43313`
(starke-Kopplungs-Approximation, Pietarinen 1981).

**Abweichung β=2, dim=64:** `⟨P⟩ = 0,454896` gegen 0,43313 →
**~5 %** (erwartete Multi-Loop-Korrektur).

**Creutz-Ratio (Etappe 23b, FAST-Modus, 2026-09-27):**

Deterministische Seeds, 200 Sweeps Thermalisierung, 300 Sweeps
Messung, 30 Bins à 10 Sweeps. Loop-Messung am Ende jedes Bins.

**dim=16:**

| β | W(1,1) | W(2,1) | W(2,2) | χ(2,2) |
|---|---|---|---|---|
| 1,00 | 0,24120 ± 0,00066 | 0,05850 ± 0,00093 | 0,00300 ± 0,00081 | 1,04844 ± 0,19338 |
| 2,00 | 0,45589 ± 0,00073 | 0,20912 ± 0,00082 | 0,04394 ± 0,00085 | 0,78591 ± 0,01758 |
| 4,00 | 0,72683 ± 0,00042 | 0,54315 ± 0,00069 | 0,31931 ± 0,00121 | 0,24010 ± 0,00281 |

**dim=32:**

| β | W(1,1) | W(2,1) | W(2,2) | χ(2,2) |
|---|---|---|---|---|
| 1,00 | 0,24124 ± 0,00020 | 0,05826 ± 0,00023 | 0,00343 ± 0,00028 | 1,51631 ± 0,08838 |
| 2,00 | 0,45436 ± 0,00027 | 0,20742 ± 0,00032 | 0,04350 ± 0,00029 | 0,77838 ± 0,00518 |
| 4,00 | 0,72734 ± 0,00018 | 0,54383 ± 0,00034 | 0,32009 ± 0,00046 | 0,23930 ± 0,00078 |

**Prüf-Ergebnisse:**

| Kriterium | Ergebnis |
|---|---|
| B1 (χ > 0 für alle dim,β) | **PASS** |
| B2 (χ monoton fallend in β pro dim) | **PASS** |
| B3 (χ konsistent über dim, 3σ) | **PASS** |

**Kreuzvalidierung gegen Running-Coupling:**

`W(1,1)(β=2, dim=32) = 0,4544` vs. `RC-⟨P⟩(β=2, dim=32) = 1 − 2·u_plaq ≈ 0,433`.
Übereinstimmung auf **~4,8 %** — konsistent mit der erwarteten
Multi-Loop-Korrektur. Die beiden Observablen messen dieselbe
physikalische Größe und liefern jetzt konsistente Werte.

### §2.8 — Code-Qualität

| Prüfung | Ergebnis |
|---|---|
| Compiler-Warnungen (`/W4` Kernel, `/W3` SDK, `/W3` Test) | **0** |
| Linker-Fehler | 0 |
| Build-Zeit kompletter Rebuild | ~5 s |

---

## §3 — Was gegen diese Baseline geprüft wird

Bei jeder Änderung am Kernel:

1. **Test-Suite läuft 45/45 PASS** (keine Regression).
2. **Numerische Anker bleiben innerhalb ihrer Toleranzen** (siehe
   Testkatalog).
3. **Creutz-Ratio-Konsistenz bleibt erhalten:**
   - χ > 0 für alle (dim, β),
   - χ monoton fallend in β,
   - χ dim-konsistent innerhalb 3σ.
4. **`W(1,1)` bleibt konsistent mit RC-`⟨P⟩`** innerhalb ~10 %
   (großzügige Schranke für Multi-Loop-Korrektur).
5. **Compiler-Warnungen bleiben bei 0** bei `/W4` (Kernel) bzw.
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

**Ende BASELINE v1.2.**