# ProPhysics — Test-Baseline

**Datei:** `docs/test/BASELINE.md`
**Version:** 1.1
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-27
**Zweck:** Referenz-Lauf der 43 Tests mit Rohwerten. Dient als
Vergleichsanker für Regressionstests.

---

## §0 — Was dieses Dokument ist

Ein **Snapshot** des aktuellen Test-Suite-Ergebnisses (Kernel 1.23.0,
Etappe 23). Wer eine Änderung am Kernel vornimmt, kann seine
Ergebnisse gegen diese Werte vergleichen.

**Nicht** in diesem Dokument:

- Test-Methodik → `docs/test/ProPhysics_Testkatalog.md`
- Test-Runner-Bedienung → `docs/test/run_alpha_tests.md`
- Changelog → `CHANGELOG.md`

**Quelle:** `bin\logs\<timestamp>_*.log` des letzten Prio-All-Laufes.

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
| 8 | SU(2) + Running-Coupling | 2 | 2/2 |
| **Gesamt** | | **43** | **43/43** |

**Prio-All-Laufzeit:** 4 420,6 s (~73,7 min).

**Laufzeit-Treiber:**

- Hydrogen-48 (~2 281 s = 51,6 %)
- Running-Coupling (~1 398 s = 31,6 %)
- Alle anderen (~742 s = 16,8 %)

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

### §2.7 — V&V-Anker (Running-Coupling)

| β | dim=64 `u_plaq` | `u_err` | ⟨P⟩ |
|---|---|---|---|
| 0,50 | 0,438134 | 0,000025 | 0,123732 |
| 1,00 | 0,379875 | 0,000021 | 0,240250 |
| **2,00** | **0,283270** | **0,000027** | **0,433460** |
| 4,00 | 0,170344 | 0,000019 | 0,659312 |

**Referenz** (β=2, dim=64): `I₂(2)/I₁(2) = 0,43313`.

**Abweichung:** **0,08 %**.

### §2.8 — Code-Qualität

| Prüfung | Ergebnis |
|---|---|
| Compiler-Warnungen (`/W4` Kernel, `/W3` SDK, `/W3` Test) | **0** |
| Linker-Fehler | 0 |
| Build-Zeit kompletter Rebuild | ~5 s |

---

## §3 — Was gegen diese Baseline geprüft wird

Bei jeder Änderung am Kernel:

1. **Test-Suite läuft 43/43 PASS** (keine Regression).
2. **Numerische Anker bleiben innerhalb ihrer Toleranzen** (siehe
   Testkatalog).
3. **V&V-Anker bleibt < 0,1 %** (Running-Coupling).
4. **Compiler-Warnungen bleiben bei 0** bei `/W4` (Kernel) bzw.
   `/W3` (SDK/Test).

**Bit-Identität** ist nur bei deterministischen Tests garantiert
(Wave-Step und SU(2)-exp nutzen `cos`/`sin` — IEEE-754-konform auf
x86-64, aber nicht plattformübergreifend bit-identisch).

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
| Repository | https://github.com/onkel83/prophysics |

---

**Ende BASELINE v1.1.**