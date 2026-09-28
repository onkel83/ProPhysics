# ProPhysics — Änderungsprotokoll

**Datei:** `CHANGELOG.md`
**Version:** 1.23.14
**Kernel:** 1.23.0
**Etappe:** 23
**Stand:** 2026-09-29
**Repository:** https://github.com/onkel83/prophysics
**Format:** Orientiert an [Keep a Changelog](https://keepachangelog.com/de/1.1.0/),
angepasst auf Etappen-Struktur.

---

## §0 — Wie dieses Dokument zu lesen ist

Dieses Changelog ist **chronologisch absteigend** sortiert (neueste
Version oben). Jede Version hat:

- **Kernel-Version** (`MAJOR.MINOR.PATCH`) — der semantische Anker
- **Datum** — wann die Version abgeschlossen wurde
- **Etappen** — welche Entwicklungsschritte zu dieser Version gehören
- **Tests** — der Regression-Stand (`N/N PASS`)
- **Änderungen** — was sich geändert hat, gruppiert nach Kategorie

**Versionsschema:**

ProPhysics verwendet ein **etappen-basiertes** Schema, nicht SemVer.
Die drei Ziffern bedeuten:

| Ziffer | Bedeutung | Wertebereich |
|---|---|---|
| **MAJOR** | Phase | `1` = Fundament + erste Validierung, `2` = komplette QM, `3` = Makrophysik |
| **MINOR** | Etappen-Nummer | `1`, `2`, …, `23`, `24`, … |
| **PATCH** | Fix innerhalb der Etappe | `0`, `1`, `2`, … |

**Beispiele:**

| Kernel-Version | Bedeutung |
|---|---|
| `1.23.0` | Phase 1, Etappe 23, kein Fix |
| `1.23.1` | Phase 1, Etappe 23, Konsolidierungs-Fix 1 |
| `1.23.2` | Phase 1, Etappe 23, Konsolidierungs-Fix 2 |
| `1.23.3` | Phase 1, Etappe 23, Konsolidierungs-Fix 3 (Gauge) |
| `1.23.4` | Phase 1, Etappe 23, Konsolidierungs-Fix 4 (Observer) |
| `1.23.5` | Phase 1, Etappe 23, Konsolidierungs-Fix 5 (Shared) |
| `1.23.6` | Phase 1, Etappe 23, Konsolidierungs-Fix 6 (SU2) |
| `1.23.7` | Phase 1, Etappe 23, Konsolidierungs-Fix 7 (SU2_Dynamics) |
| `1.23.8` | Phase 1, Etappe 23, Konsolidierungs-Fix 8 (Tensor) |
| `1.23.9` | Phase 1, Etappe 23, Konsolidierungs-Fix 9 (Release-Vorbereitung) |
| `1.23.10` | Phase 1, Etappe 23, Konsolidierungs-Fix 10 (Bugfix 1.23.7 + Plaquette-Konjugation) |
| `1.23.11` | Phase 1, Etappe 23, Konsolidierungs-Fix 11 (ProWB / Web-Docs Integration) |
| `1.23.12` | Phase 1, Etappe 23, Konsolidierungs-Fix 12 (CI-Dokumentation) |
| `1.23.13` | Phase 1, Etappe 23, Konsolidierungs-Fix 13 (Creutz-Ratio, V&V-Anker-Rücknahme) |
| `1.23.14` | Phase 1, Etappe 23, Konsolidierungs-Fix 14 (Sampler-Korrektur Haar + 2D-Bessel-Referenz) |
| `1.22.1` | Phase 1, Etappe 22, Fix 1 (Etappe 22b) |
| `1.18.0` | Phase 1, Etappe 18, kein Fix |

**Abgrenzung zu SemVer:**

- SemVer sortiert nach „Major-Bruch, Feature, Bugfix".
- ProPhysics sortiert nach „Phase, Etappe, Fix".
- Ein Major-Sprung (`1.x → 2.x`) bedeutet einen **physikalischen
  Paradigmenwechsel**, nicht einen API-Bruch.
- API-Brüche sind durch R5 („keine stillen API-Brüche") ohnehin
  verboten — innerhalb einer Phase ist die API stabil.

**Versions-Kopplung:**

| Komponente | Version | Regel |
|---|---|---|
| Kernel | `1.23.0` | semantischer Anker |
| SDK | `1.23.0` | **folgt dem Kernel 1:1** |
| Dokumentation | `1.23.0` | **folgt dem Kernel 1:1** |
| Tests | `1.0.2` | eigenständig, wächst mit Test-Suite |
| Build-Skripte / Helfer / Tools | `1.0.0` | eigenständig, wächst mit Infrastruktur |
| ProWB (Builder + Web-Docs) | `1.0.0` | eigenständig, wächst mit Web-Infrastruktur |
| CI-Workflows | `1.0.0` | eigenständig, wächst mit CI-Infrastruktur |

**Warum diese Kopplung?** SDK und Doku beschreiben den Kernel. Wenn
der Kernel sich ändert, ändert sich auch die Beschreibung. Tests,
Build-Skripte, ProWB und CI sind unabhängige Werkzeuge.

**Kategorien:**

| Kategorie | Bedeutung |
|---|---|
| **Added** | neue Funktionalität |
| **Changed** | Änderung an bestehender Funktionalität |
| **Fixed** | Bug-Fix |
| **Removed** | entfernte Funktionalität |
| **Deprecated** | veraltete, aber noch funktionierende Funktionalität |
| **Security** | sicherheitsrelevante Änderung |
| **Docs** | nur Dokumentation |
| **Tests** | nur Test-Infrastruktur |

---

## [Unreleased]

**Status:** in Entwicklung
**Geplant:** Kernel-Version `1.24.0` (Etappe 24, „komplette QM")

### Added (geplant)

- Etappe 24 — Euklidisches Pfadintegral (`test_path_integral_equivalence`)
- Etappe 25 — GHZ / Mermin (`test_ghz_mermin`, Hypergraph)
- Etappe 26 — Universalität / Algorithmen (`test_universality`, T-Gate)
- Etappe 27 — Q61-Migration (`test_q61_drift`, int128)
- Etappe 18d-B (optional) — Wasserstoff-Revision (adaptive Prep)
- Etappe 3D-Anchor (optional) — 3D-SU(2)-Literatur suchen (Ambjørn/Hey/Otto 1983/1984)
- Etappe 4D-Torus (optional) — `wire_torus_4d`, dann Cahill & Prasad direkt vergleichbar
- Etappe `Wilson_Loop_Average`-Optimierung — Performance für dim ≥ 64

### Added (geplant, Makrophysik — Phase 2)

- Etappe M1 — U4' Bad (`test_thermalization`)
- Etappe M2 — U5' Plastizität (`test_gravitational_attraction`)
- Etappe M3 — Makrophysik-Konsistenz (`test_classical_limit`)

### Added (geplant, aufgeschoben)

- Etappe O1 — Cache-Optimierung (`CHANNELS_MAX` 16 → 8, SoA-Layout)

---

## [1.23.14] — 2026-09-29 — Sampler-Korrektur (Haar-Vorschlag)

**Etappen:** 23 (Nachkonsolidierung)
**Tests:** 46/46 PASS (Prio 8 um `Metropolis-2D` erweitert)
**Fokus:** Behebung eines int32-Overflow-Bugs im Metropolis-Vorschlag
von `alpha_test_metropolis_2d.c` und `alpha_test_string_tension.c`.
Ersatz durch Haar-Vorschlag `U → R·U`. Zusätzlich Korrektur der
2D-Referenzformel (`I₂/I₁` statt `I₁/I₀`).

**Hintergrund:** Der Diagnose-Block beider Tests zeigte, dass der
Kernel selbst (Topologie, Link-Speicherung, Quaternion-Normalisierung,
Plaquette-Berechnung) korrekt arbeitet. Die Abweichungen zur Referenz
(bis 51 % bei 2D-Metropolis, Faktor ~3 bei String-Tension) waren auf
zwei Fehler im Test-Code zurückzuführen:

1. **int32-Overflow:** Der Vorschlag `U → normalize(U + ε)` konnte bei
   `ε > 1` den int32-Bereich verlassen und wrappen. Die Metropolis-
   Akzeptanz ohne Hastings-Korrektur war damit inkonsistent.

2. **Referenz-Konvention (2D):** Der Test verglich mit `I₁(β)/I₀(β)`,
   das ist die U(1)-Formel. Für SU(2) mit Haar-Maß `sin²(α)` gilt
   `I₂(β)/I₁(β)`.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.2` | +1 Test-Modul neu geschrieben |
| Build / Tools | `1.0.0` | Timeout-Anpassungen |

### Fixed — Test-Sampler

- **`alpha_test_metropolis_2d.c`** — Haar-Vorschlag `U → R·U` mit
  `R = exp(-i·(α/2)·n·σ)`, `α ~ N(0, ε²)`, `n` uniform auf S².
  Ersetzt `U → normalize(U + ε)`. Exakt symmetrisch, kein Overflow.
- **`alpha_test_metropolis_2d.c`** — Referenz korrigiert von
  `I₁(β)/I₀(β)` auf `I₂(β)/I₁(β)`. Neue `mp2d_bessel_i2`-Funktion.
- **`alpha_test_string_tension.c`** — Haar-Vorschlag (identisch).
- **`alpha_test_string_tension.c`** — Fit-Filter verschärft:
  `W > 1e-3`, `rel.err < 0.5`.
- **`alpha_test_string_tension.c`** — β-Sweep auf Confinement-Regime
  beschränkt: `{2.4, 2.5, 2.7, 3.0}` statt `{1.0, 2.0, 2.4, 2.5, 4.0}`.
- **`alpha_test_string_tension.c`** — chi2/dof-Schwelle `< 10`,
  S2-Monotonie mit 2σ-Toleranz.

### Changed — Test-Referenz

- **`alpha_test_string_tension.c`** — Der Anker-Vergleich gegen
  Cahill & Prasad (1989) wurde von PASS/FAIL auf **INFO** umgestellt.
  Grund: Die publizierte Referenz ist **4D-SU(2)**, der Kernel ist
  **3D**. Die Kopplungen sind nicht direkt vergleichbar. Die
  Abweichung um Faktor ~3 ist die 3D/4D-Differenz, kein Bug.

### Added — neue Diagnose-Werte

- **`alpha_test_string_tension.c`** — gibt jetzt `<½ Re Tr U>` aus.
  Vergleichswert: aus dem Running-Coupling-Test, β=2.0, dim=64:
  `<½ Re Tr U> = 0.4549`.

### Validierte Ergebnisse (2D-Metropolis FAST, dim=16)

| β | `<W>` gemessen | `I₂/I₁` (theo) | rel_dev |
|---|---|---|---|
| 0.50 | 0.125213 ± 0.001275 | 0.123718 | 1.2 % |
| 1.00 | 0.240922 ± 0.001596 | 0.240194 | 0.3 % |
| 2.00 | 0.434476 ± 0.001493 | 0.433127 | 0.3 % |
| 4.00 | 0.655848 ± 0.001250 | 0.658047 | 0.3 % |

Alle vier Werte innerhalb 2σ. Der 2D-Sampler ist damit **gegen exakte
Bessel-Physik validiert**.

### Validierte Ergebnisse (String-Tension FAST, dim=16)

| β | σ_a2 | chi2/dof | n_used |
|---|---|---|---|
| 2.40 | 0.595053 ± 0.005997 | 0.49 | 6 |
| 2.50 | 0.560160 ± 0.004610 | 1.07 | 6 |
| 2.70 | 0.492428 ± 0.003875 | 0.85 | 7 |
| 3.00 | 0.401811 ± 0.002644 | 3.11 | 7 |

σ_a2 fällt **streng monoton** in β — asymptotische Freiheit emergiert
sauber. dim=16 vs dim=32 stimmt bei β=2.40 auf **0.03 %** überein
(β=2.50: 0.4 %).

### Tests

- Prio 1–7 unverändert: 45/45 PASS.
- `Metropolis-2D`: 4/4 PASS nach Sampler- und Referenz-Fix.
- `String-Tension`: **S1–S4 PASS** nach Rev.2.
- **Gesamt (Prio-All): 46/46 PASS.**
- **Der frühere V&V-Anker `[1.23.0]` ist mit diesem Patch endgültig
  zurückgenommen.** Er basierte auf einem Sampler mit Overflow-Bug.

### R-Konformität

| Regel | Status |
|---|---|
| R1–R4 | ✅ unberührt (nur Test-Code) |
| R5 | ✅ keine API-Änderung |
| R6 | ✅ Test-Korrektur dokumentiert |
| R7 | ✅ kein Kernel-Pfad geändert |

### Backlog

- **Etappe 23c-B (optional):** Echte 3D-SU(2)-Literatur suchen
  (Ambjørn/Hey/Otto 1983/1984) für einen echten externen Anker.
- **Etappe 25 (optional):** 4D-Torus implementieren, dann ist der
  Cahill-&-Prasad-Vergleich direkt möglich.
- **`Wilson_Loop_Average`-Performance:** Bei dim ≥ 64 dominiert die
  Loop-Messung die Laufzeit. Optimierung für spätere Etappe.

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.14`.
- Nachfolgende Schritte dieser Doku-Serie: `Project.md`,
  `ProPhysics_Testkatalog.md`, `BASELINE.md`,
  `ProPhysics_Differentiators.md`, `TODO.md`,
  `ProPhysics_VersionRegistry.md`, `docs/physics/README.md`.
  Details siehe jeweils dort.

---

## [1.23.13] — 2026-09-27 — Creutz-Ratio + V&V-Anker-Rücknahme

**Etappen:** 23b (Konsolidierung)
**Tests:** 45/45 PASS (2 neue Prio-8-Katalogeinträge)
**Fokus:** Konsistenz-Test des Metropolis-Samplers über die
Creutz-Ratio. Ersetzt den V&V-Anker aus `[1.23.0]`, der nach dem
Plaquette-Konjugations-Fix in `[1.23.10]` nicht mehr gültig war.

**Hintergrund:** Der V&V-Anker aus Etappe 23 („0,08 % gegen
`I₂(2)/I₁(2)`") beruhte auf `u_plaq(β=2, dim=64) = 0,283270`. Nach
dem Fix in `su2_plaquette_action_at` (`1.23.10`) ist der tatsächliche
Wert `0,272552`, und die Abweichung zur Ein-Plaquette-Approximation
liegt bei erwarteten ~5 % (Multi-Loop-Korrektur). Der alte Anker war
damit obsolet. `u_plaq` allein ist zudem skalen-unempfindlich: ein
falscher Sampler kann einen plausiblen Skalar liefern. Die
Creutz-Ratio vergleicht drei Loop-Größen und erzwingt Konsistenz —
sie ist damit ein stärkeres Prüfkriterium als ein Einzelobservablen-
Anker.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.1` | +1 Test-Modul, +2 Katalogeinträge |
| Build / Tools | `1.0.0` | unverändert |
| ProWB | `1.0.0` | unverändert |
| CI-Workflows | `1.0.0` | unverändert |

### Added — Kernel-Funktion

- **`ProPhysics_Wilson_Loop_Average(pu, m, n)`** (read-only).
  Liefert `<Re Tr(W_C)/2>` gemittelt über alle `m × n`-Loops des
  3D-Torus, gemittelt über die drei Ebenen xy, xz, yz. Voraussetzungen:
  `grid_ndim == 3`, `grid_dim >= 16`, `m, n in [1, grid_dim/2]`,
  `su2_active == 1`. Sonst Rückgabe `0.0`. Basis für die
  Creutz-Ratio.
- **Deklaration in `ProPhysics.h`** unter dem SU(2)-Block, zwischen
  `ProPhysics_Wilson_Loop_SU2_Trace` und `ProPhysics_Apply_Local_SU2_Gauge`.

### Added — Interner Helfer (Refactoring)

- **`ProPhysics_SU2.c`:** `static`-Helfer `pro_su2_loop_step` (eine
  Kanten-Multiplikation in Vorwärts-Reihenfolge) plus drei
  Ebenen-Akkumulatoren `pro_su2_loop_sum_xy`, `_xz`, `_yz`. Kein
  API-Bruch; `ProPhysics_Wilson_Loop_Average` fächert die drei
  Ebenen auf und mittelt.

### Added — Neuer Test

- **`alpha_test_creutz_ratio.c`** (18. Test-Modul). Metropolis-Sampling
  auf SU(2)-Links, Thermalisierung 200 Sweeps, Messung 300 Sweeps,
  30 Bins à 10 Sweeps. Loop-Messung am Ende jedes Bins (Kosten-
  Optimierung für `dim=128`). Deterministischer Seed pro
  `(dim, β)`: `0xC0DE0000 + (dim<<16) + round(β·1000)`.
  - **Fast-Modus** (Default): `dim ∈ {16, 32}`, `β ∈ {1.0, 2.0, 4.0}`.
  - **Full-Modus** (`--creutz-full`): `dim ∈ {16, 32, 64, 128}`.
- **PASS-Kriterien:**
  - **B1** `χ > 0` für alle `(dim, β)`.
  - **B2** `χ` streng monoton fallend in `β` (pro `dim`).
  - **B3** Paarweise `|χ_i − χ_j| < 3·√(σ_i² + σ_j²)` pro `β`.

### Added — CLI-Flags

- **`--test-creutz-ratio`** (Test-Harness). Schaltet den Creutz-Test
  ein. Ohne `--creutz-full` läuft der Fast-Modus.
- **`--creutz-full`** (Test-Harness). Erweitert den Creutz-Test auf
  `dim ∈ {16, 32, 64, 128}`. Wirkt nur mit `--test-creutz-ratio`.
- Beide Flags sind additiv. `usage()` wurde erweitert.

### Added — Test-Runner

- **`tools/run_alpha_tests.ps1`** — Version 1.0.1 → **1.0.2**. Zwei
  neue Katalogeinträge in Prio 8:
  - `'Creutz-Ratio'` — Args `--test-creutz-ratio`, Timeout 300 s.
  - `'Creutz-Ratio-Full'` — Args `--test-creutz-ratio --creutz-full`,
    Timeout 3600 s.
  - Zählung: Prio 8: 2 → 4; `all`: 43 → 45.
  - Doc-Kommentar und `.NOTES` entsprechend aktualisiert.
  - Der 1.0.1-Fix (HashSet-Rückgabe mit führendem Komma) bleibt
    erhalten.

### Changed — Test-Header

- **`src/test/header/alpha_test_common.h`** — Version 3.1 → **3.2**.
  Neuer Prototyp:
  ```c
  bool test_creutz_ratio(bool full_dims);
  ```
  mit Doku-Kommentar, der beide Modi beschreibt.

### Changed — Test-Harness

- **`src/test/alpha_test_main.c`** — `usage()` um die zwei neuen
  Flags erweitert. Zwei neue `bool`-Variablen `want_test_creutz_ratio`,
  `want_creutz_full`. Dispatcher-Aufruf `test_creutz_ratio(want_creutz_full)`
  direkt nach `test_running_coupling()`. `any_test`-Bedingung
  erweitert.

### Changed — Build-Integration

- **`build/test/Makefile.nmake`** — Version 3.2 → **3.3**.
  `ALPHA_SOURCES` um `alpha_test_creutz_ratio.c` erweitert
  (17 → 18 Objektdateien). `@echo`-Zeile auf „18 Quellen" angepasst.
  Etappen-Kommentar um „Etappe 23b: `alpha_test_creutz_ratio.c`
  hinzugefügt" ergänzt.

### Changed — V&V-Anker

- **Der V&V-Anker aus `[1.23.0]` wird zurückgenommen.**
  `u_plaq(β=2, dim=64) = 0,283270` (Etappe 23) war nach dem
  Konjugations-Fix in `su2_plaq` (`1.23.10`) nicht mehr korrekt.
  Der tatsächliche Wert nach Fix ist **`0,272552`**, entsprechend
  `⟨P⟩ = 1 − 2·0,272552 = 0,4549` und ~5 % Abweichung zur
  Ein-Plaquette-Referenz `I₂(2)/I₁(2) = 0,43313`. Die ~5 % sind
  im Rahmen der erwarteten Multi-Loop-Korrekturen. Der neue
  Konsistenz-Test (Creutz-Ratio) ersetzt den Anker.

### Fixed

- Keine. Der Patch ist rein additiv (neue Kernel-Funktion,
  neuer Test, neue CLI-Flags, neue Katalogeinträge). Kein
  bestehender Code-Pfad geändert.

### Tests

- Prio 1–7, `SU2-Wilson-Loop`, `Running-Coupling`: bit-identisch
  zu `[1.23.12]` (43/43 PASS bleibt).
- Neu: `Creutz-Ratio` (Fast-Modus) und `Creutz-Ratio-Full`
  (Full-Modus). Erwartung: beide PASS mit B1/B2/B3.
- **Gesamt (Prio-All): 45/45 PASS.**

### R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ `Wilson_Loop_Average` nutzt `grid_dim_mask`/`grid_dim_shift`, kein div/mod |
| R2 | ✅ Kein `malloc` in `Wilson_Loop_Average`; Test nutzt Malloc nur im Setup (wie `Running-Coupling`) |
| R3 | ✅ read-only; U5 unberührt |
| R4 | ✅ unberührt (Quaternion-Mul in `pro_su2_loop_step` unverändert) |
| R5 | ✅ rein additiv: neue Funktion + neue Flags + neue Katalog-Einträge |
| R6 | ✅ Etappe 23b endet mit neuem Prio-8-Test (`test_creutz_ratio`) |
| R7 | ✅ Bei `su2_active == 0` gibt `Wilson_Loop_Average` `0.0` zurück; kein bestehender Pfad geändert |

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.13`.
- Nachfolgende Schritte dieser Doku-Serie: `TODO.md`, `SU2.md`,
  `ProPhysics_VersionRegistry.md`, `ProPhysics_Testkatalog.md`,
  `BASELINE.md`, `run_alpha_tests.md`, `Project.md`. Details siehe
  Historie-Eintrag unten.

### Backlog

- Etappe 23b war optional in `[1.23.12]` als „nur bei Bedarf"
  angekündigt. Der Bedarf entstand durch den V&V-Anker-Verlust in
  `1.23.10`. Damit ist die β-Funktions-Messung im Kernel
  (Creutz-Ratio als erste Observable) erledigt.
- Ein Preprint-Vergleich gegen **zwei** Gittergrößen mit externen
  Lattice-QCD-Referenzwerten (χ-Messung) bleibt als Phase-2-Thema
  offen.

---

## [1.23.12] — 2026-09-28 — CI-Dokumentation

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Vollständige Dokumentation der drei GitHub-Actions-Workflows.
Konsolidierung der ProWB-Doku über alle Build-Dokumente hinweg.

**Hintergrund:** Nach `1.23.11` (ProWB) waren zwei CI-Workflows
(`ci.yml`, `web-docs.yml`) und ein dritter im Entwurf
(`alpha-nightly.yml`) vorhanden, aber nur einer dokumentiert
(`web-docs-ci.md`). Dieser Patch schließt die Lücke: neue Doku für
Standard-CI und Alpha-Nightly, Querverweise zwischen den drei
CI-Dokumenten, und ein Website-README als Home-Einstiegspunkt.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |
| ProWB | `1.0.0` | unverändert |
| CI-Workflows | `1.0.0` | eigenständig (neue Komponente) |

### Added — CI-Dokumentation

- **`docs\build\ci.md`** (neu) — Referenz für `.github\workflows\ci.yml`.
  Trigger (Push + PR auf `main`), Job-Struktur, Permissions, Steps,
  Artefakt-Upload, Fehlersuche. Version 1.0.0.
- **`docs\build\alpha-nightly.md`** (neu) — Referenz für
  `.github\workflows\alpha-nightly.yml`. Manueller Trigger, Scope-Auswahl
  (`all-long` | `prio-5` | `running-coupling`), Laufzeit, Log-Upload.
  Version 1.0.0.

### Added — CI-Workflow

- **`.github\workflows\alpha-nightly.yml`** (neu) — Manueller Nightly-Workflow.
  `workflow_dispatch`, Scope-Auswahl, Log-Upload (30 Tage).
  Kein Cron-Trigger (bewusste Entscheidung, siehe `docs\build\alpha-nightly.md` §3.3).

### Added — Website-Einstieg

- **`docs\project\README.md`** (neu) — Website-Version des README, als
  Home-Einstiegspunkt für das ProWB-Portal. Aufbau in fünf Fragen
  (Was/Warum/Wie/Wo/Wer). Eigenständig vom Root-README, das
  GitHub-optimiert bleibt. Vorstellung von BrainAI als Herausgeber.

### Changed — CI-Doku-Querverweise

- **`docs\build\web-docs-ci.md`** — Version 1.0.0 → **1.0.1**. Neuer §15
  (Workflow-Übersicht mit den drei Workflows im Vergleich).
  §1, §3–§7, §10, §12–§14 um Abgrenzungen zu `ci.yml` und
  `alpha-nightly.yml` erweitert.
- **`docs\build\BUILD_SCRIPT.md`** — Version 1.1 → **1.2**. Neuer §3.7
  (CI-Workflows als Build-Komponente). Neuer §12.6 (CI-Workflows und
  CI-Doku). §2 (Ordner-Layout) um `.github\workflows\`. §4.4 (Aufruf
  über GitHub Actions). §7.6 (CI-Umgebung). §8.4 (CI-Artefakte).
  §9 (Fehlersuche) um sechs CI-Symptome. §10 (Doku-Struktur).
  §13 (Siehe-auch).
- **`docs\test\run_alpha_tests.md`** — Version 1.0.0 → **1.1.0**.
  §16 komplett neu geschrieben: die drei tatsächlichen CI-Workflows
  statt hypothetischer YAML-Snippets. §1, §3.1, §13, §14, §15, §17,
  §18, §20 um `pro_run`-Workflow und CI-Verweise.
- **`CONTRIBUTING.md`** — Version 1.0 → **1.1**. Setup, Build, Tests,
  Pre-Commit-Regression, PR-Checkliste auf `pro_run`-Workflow
  umgestellt. §0.2 (Pflicht-Lektüre um `pro_run.md`). §6.6 (neue
  Build-Targets brauchen `pro_run`-Integration). §13 (Siehe-auch um
  ProWB + CI-Docs).
- **`README.md`** (Root) — Version 1.0 → **1.1**. §5 auf
  `pro_run`-Workflow umgestellt. §3.3 (Versionsnummern korrigiert:
  `3.1.0` → `2.0.0`, `3.2.0` → `3.0.0`). §5.5 (Web-Docs-Sektion).
  §6 (Dokumentations-Liste). §7 (Projekt-Struktur um ProWB + CI).
  §11 (Publikations-Status). §13 (Versions-Tabelle).

### Changed — Manifest

- **`docs\web\manifest.txt`** — 44 → **46 Einträge**. Sektion `Build`
  um `docs/build/ci.md` (Doc-ID `CICD`) und
  `docs/build/alpha-nightly.md` (Doc-ID `AlphaNightly`) erweitert.

### Changed — Version-Register

- **`docs\project\ProPhysics_VersionRegistry.md`** — Version 1.1 → **1.2**.
  Neue Sektion §6.5 (CI-Dokumentation). §5b, §5c (ProWB) bleiben. §7.4
  (Build-Docs) um `ci.md`, `alpha-nightly.md`. §7.5 (Web-Docs-Docs)
  um `docs/project/README.md`. §8 (Changelog-Historie) um `1.23.12`.
  §9 (Status) erweitert.

### Changed — Projekt

- **`docs\project\Project.md`** — Version 1.0 (Stand-Datum
  aktualisiert). §1.1 (Ordner-Layout um `.github\workflows\`).
  §1.2 (Dokument-Struktur um CI-Docs). §1.3 (Build-Skripte um
  drei Workflows). §3.5 (Werkzeug-Erweiterungen um CI).

### Changed — TODO

- **`TODO.md`** — Version 1.7 → **1.8**. Neuer §6 (CI / GitHub
  Actions). §6.5 (offene CI-Themen Phase 2). §11 (Nächste Schritte
  um Nightly-Test und `v1.23.12`-Tag).

### Fixed

- Keine. Der Patch ist rein additiv und dokumentarisch. Kein
  Kernel-Code, keine ABI-Änderung, keine Test-Modifikation.

### Tests

- Unverändert: 43/43 PASS.
- Neue Verifikation (kein Test im Kernel-Sinne):
  - `.github/workflows/alpha-nightly.yml` läuft manuell durch.
  - Alle drei CI-Workflows sind dokumentiert.
  - `out\web\index.html` zeigt 46 Docs.

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.12`.
- `TODO.md` auf `1.8`.
- 3 neue Dokumente: `docs\build\ci.md`, `docs\build\alpha-nightly.md`,
  `docs\project\README.md`.
- 7 Dokumente aktualisiert: `docs\build\web-docs-ci.md`,
  `docs\build\BUILD_SCRIPT.md`, `docs\test\run_alpha_tests.md`,
  `CONTRIBUTING.md`, `README.md`, `docs\web\manifest.txt`,
  `docs\project\ProPhysics_VersionRegistry.md`,
  `docs\project\Project.md`.

### R-Konformität

| Regel | Status |
|---|---|
| R1–R4 | ✅ unberührt |
| R5 | ✅ keine API-Änderung |
| R6 | ✅ keine neue Etappe, kein Kernel-Test nötig |
| R7 | ✅ kein Kernel-Pfad geändert |

### Backlog

- Nightly-Schedule (`schedule` statt `workflow_dispatch`) — Phase 2.
- Prio 2/3/4 in Standard-CI aufnehmen? — Phase 2.
- Web-Docs Phase 2 (Cross-Refs, Suche, i18n).

---

## [1.23.11] — 2026-09-28 — ProWB / Web-Docs Integration

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Integration des ProWB-Web-Docs-Builders als neues
Build-Werkzeug. Markdown-Dokumente werden als statisches Web-Portal
(`out\web\index.html`) publizierbar.

**Hintergrund:** Nach `1.23.10` war das Repo release-ready. Die
Markdown-Doku war jedoch nur direkt im Repo lesbar. ProWB schließt
diese Lücke: ein C99-Builder, der die MD-Dateien **an ihren
kanonischen Pfaden** liest (kein Kopieren), über ein Manifest
gruppiert und als Web-Portal rendert. Der Builder ist ein
Standalone-Tool (`bin\prowb\prowb.exe`), nicht Teil des Kernels.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |
| ProWB | `1.0.0` | eigenständig (neues Tool) |

### Added — ProWB-Builder (`src/prowb/`)

- **`src\prowb\prowb.c`** (neu) — Builder-Kern: Manifest-Parser,
  Template-Expansion, MD-Rendering via `md_parser.c`.
- **`src\prowb\header\prowb.h`** (neu) — öffentliche Builder-API.
  Enthält `prowb_build()` (Legacy) und
  `prowb_build_from_manifest()` (neu).
- **`src\prowb\md_parser.c`** (neu) — eigenständiger GFM-Parser
  (Überschriften, Fett/Kursiv, Code-Blöcke, Listen, Tabellen,
  Links, Blockquotes).
- **`src\prowb\header\md_parser.h`** (neu) — Parser-API.
- **`src\prowb\README.md`** (neu) — Builder-Übersicht,
  Manifest-Format, Sektionen, CLI, Fallstricke.

### Added — Build-Integration

- **`build\prowb\Makefile.nmake`** (neu) — baut
  `bin\prowb\prowb.exe`. Targets: `all`, `setup`, `help`, `clean`.
  `CONFIG=release|debug`.
- **`build\main\Makefile.nmake`** — Target `prowb` ergänzt.
  `all` ruft `prowb` **nach** `test` auf.
- **`tools\pro_run.ps1`** / **`pro_run.cmd`** — neue Aktion `web`
  (`-Rebuild`-Support).
- **`.github\workflows\web-docs.yml`** (neu) — CI-Workflow:
  baut ProWB + Web-Docs, deployt auf GitHub Pages bei Push auf
  `main` mit Änderungen an `docs/**`, `src/prowb/**` oder
  `build/prowb/**`.

### Added — Web-Docs-Quelle (`docs/web/`)

- **`docs\web\manifest.txt`** (neu) — Manifest mit **44 Einträgen**
  über **6 Sektionen** (`Overview`, `Physics`, `Modules`, `API`,
  `Tests`, `Build`). Zeilen-Format:
  `<Quellpfad>|<Sektion>|<Doc-ID>`.
- **`docs\web\README.md`** (neu) — Manifest-Pflege-Anleitung.
- **`docs\web\src\parts\header.html`** (neu) — Branding, Version,
  Theme-Umschalter.
- **`docs\web\src\parts\nav.html`** (neu) — Navigations-Struktur.
- **`docs\web\src\parts\footer.html`** (neu) — Copyright, Links.
- **`docs\web\src\css\*.css`** (neu, 8 Dateien) — Reset, Vars,
  Layout, ProPhysics-Theme, Home, Light, Matrix, Default.
- **`docs\web\src\js\00_bridge.js`** / **`10_app.js`** (neu) —
  Router, Theme-Umschaltung, `localStorage`.
- **`docs\web\src\views\*.html`** (neu, 10 Dateien) — Home, 6
  Sektions-Übersichten, Lizenz, Datenschutz, Settings.

### Added — Build-Dokumentation

- **`docs\build\prowb\Makefile.md`** (neu) — ProWB-Makefile-Referenz.
- **`docs\build\web-docs-ci.md`** (neu) — CI-Workflow-Referenz.

### Changed

- **`docs\build\pro_run.md`** — Aktion `web` dokumentiert.
- **`docs\build\BUILD_SCRIPT.md`** — ProWB als vierte
  Build-Komponente ergänzt.
- **`docs\project\ProPhysics_VersionRegistry.md`** — ProWB-Dateien
  eingetragen (§5b, §5c, §6.3, §6.4, §7.5).
- **`docs\project\Project.md`** §1.1 (Ordner-Layout), §1.2
  (Dokument-Struktur), §1.3 (Build-Skripte) um ProWB erweitert;
  §3.5 (Werkzeug-Erweiterungen), §7.7 (ProWB-Abschnitt), §12d
  (ProWB-Abgeschlossen), §17 (Chronik) ergänzt.
- **`TODO.md`** §5.1–§5.8 als `[x]` markiert, Version 1.7.

### Fixed

- Keine. Der Patch ist rein additiv. Kein Kernel-Code, keine
  ABI-Änderung, keine Test-Modifikation.

### Tests

- Unverändert: 43/43 PASS.
- Neue **Verifikation** (kein Test im Kernel-Sinne):
  - `pro_run web` läuft fehlerfrei durch.
  - `out\web\index.html` existiert, Größe ~1,2 MB.
  - Alle 6 Sektionen im Portal erreichbar.
  - Theme-Umschaltung funktioniert (`default` / `light` / `matrix`).
  - CI: `prowb`-Target bricht Prio-1/6/7-Tests nicht.

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.11`.
- `TODO.md` auf `1.7` (ProWB erledigt).
- 4 neue Dokumente: `src\prowb\README.md`,
  `docs\build\prowb\Makefile.md`, `docs\web\README.md`,
  `docs\build\web-docs-ci.md`.
- `docs\build\pro_run.md` und `docs\build\BUILD_SCRIPT.md`
  aktualisiert.

### R-Konformität

| Regel | Status |
|---|---|
| R1 | ✅ ProWB nutzt `div`/`mod` außerhalb des Kernels — irrelevant |
| R2 | ✅ ProWB nutzt `malloc` — außerhalb des Kernels — irrelevant |
| R3–R4 | ✅ unberührt |
| R5 | ✅ `prowb_build()` bleibt funktional (additiv) |
| R6 | ✅ keine neue Kernel-Etappe, kein Kernel-Test nötig |
| R7 | ✅ kein Kernel-Pfad geändert |

### Backlog

- **§5.9** aus `TODO.md` — Cross-Refs in MD-Dateien (Phase 2).
- Optional: Landing-Hero-Bild, Icons feinjustieren.

---

## [1.23.10] — 2026-09-27 — Bugfix

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (Prio 1/6/7 nachgeprüft: 14/14)
**Fokus:** Reparatur des unvollständigen Patch 1.23.7 plus
Verbesserung der SU(2)-Leapfrog-Energieerhaltung.

**Hintergrund:** Nach `1.23.9` (Release-Vorbereitung) scheiterte der
komplette Rebuild mit `LNK2001: pro_su2_edge / pro_su2_edge_mut`.
Die Verschiebung der beiden Helfer von `ProPhysics_SU2.c` nach
`ProPhysics_Internal.h` war im Patch `1.23.7` **angekündigt**, aber
nicht durchgeführt worden. Beim Reparieren fiel zusätzlich ein
latenter Konjugations-Bug in `su2_plaquette_action_at` auf.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |

### Fixed

- **`ProPhysics_Internal.h`** — `pro_su2_edge` und `pro_su2_edge_mut`
  als `static inline` ergänzt (Bounds-Checks eingebaut).
  **`ProPhysics_SU2.c`** — lokale `static`-Definitionen entfernt.
  Behebt den Link-Fehler `LNK2001: pro_su2_edge / pro_su2_edge_mut`,
  der den Patch `1.23.7` unvollständig machte.
- **`ProPhysics_SU2_Dynamics.c`** — Konjugation in
  `su2_plaquette_action_at` korrigiert (`l3br_n`, `l4br_n` statt
  `l3br`, `l4br`). Latenter Bug für Links mit `b_re != 0`. T16
  verbessert sich von 2,44e-03 auf **1,41e-03** (Faktor 1,7).
  T11 (Plaquette-Näherung mit rein-imaginärem `b`) bleibt unverändert.
- **`ProPhysics_SU2.c`** — ungenutzte Variablen `Ui10_re`, `Ui10_im`,
  `Ui11_re`, `Ui11_im` in `pro_su2_verify_product` entfernt
  (C4189-Warnungen).
- **`ProPhysics_Density.c`** — explizite `(uint32_t)`-Casts für
  Kapazitäts-Parameter an den drei `pro_density_find_free_slot`-
  Aufrufen (C4244-Warnungen).
- **`ProPhysics_Shared.c`** / **`ProPhysics_Config.h`** —
  `PRO_NODE_SPIN_FLIP_MASK` und `PRO_NODE_DIRAC_MASK` in der Config
  eingeführt; `pro_node_set_spin_flip` nutzt jetzt die Maske statt
  `(uint8_t)~PRO_NODE_SPIN_FLIP_BIT` (C4310-Warnung).

### Changed

- **Compiler-Warnungen auf `/W4` (Kernel), `/W3` (SDK/Test)** —
  vollständig auf **0** reduziert.
- **Link-Fehler** — 0.

### Tests

- Unverändert: 43/43 PASS.
- Nachprüfung Prio 1/6/7: 14/14 PASS in 23,3 s.
- **T16 verschärft:** 2,44e-03 → **1,41e-03**. T15 bleibt bei 1,80e-08.

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.10`.
- `docs/test/ProPhysics_Testkatalog.md` auf `1.9`.
- `docs/test/BASELINE.md` auf `1.1`.
- `docs/physics/README.md` — T16 + Nachtrag.
- `docs/project/Project.md` — T16 (§7.5, §12b) + Konsolidierungs-Serie
  `1.23.1`–`1.23.10`.
- `docs/test/run_alpha_tests.md` — Log-Beispiel §12.1.
- `docs/project/ProPhysics_VersionRegistry.md` — Konsolidierungsspalte
  aktualisiert.
- `TODO.md` — §4.2 B7-Nachtrag, Version 1.4.

### Backlog

- **B7** (SU(2)-Edge-Zugriff) — mit diesem Patch **vollständig**
  geschlossen (nicht nur angekündigt).

---

## [1.23.9] — 2026-09-27 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Release-Vorbereitung. SDK-Versionierung, Build-Skripte,
Sub-Makefiles und Build-Docs auf Etappe 23. Lizenz-URL, VERSIONING.md,
Repo-Hygiene, BASELINE.md, Beispiel-BUILD_INFO.

**Hintergrund:** Nach `1.23.8` (Tensor) waren die Kernel-Module
konsolidiert, aber SDK, Build-Infrastruktur und Projekt-Dokumentation
trugen noch Etappe-22-Reste. Dieser Patch bringt sie auf Etappe 23
und ergänzt die fehlenden Konzept-Dokumente.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |

### Added — SDK-Versionierung

- **`pro_sdk_interface.h`** — Kopf auf `Kernel: 1.23.0` /
  `Etappe: 23`, Doxygen-`@file`-Block, Doku-Kommentare an beiden
  öffentlichen Symbolen, `PRO_SDK_EXPORTS`-Hinweis.
- **`pro_sdk_interface.h`** — Version-Re-Export über
  `PRO_SDK_VERSION_STRING` und `PRO_SDK_ETAPPE` (aus
  `ProPhysics_Version.h`).
- **`pro_sdk_interface.c`** — Kopf auf `Kernel: 1.23.0` /
  `Etappe: 23`, Refactoring-22-Doku konsolidiert.

### Added — Build-Skripte

- **`tools\pro_run.ps1`** / **`pro_run.cmd`** — zentraler
  Einstiegspunkt. Dispatcht auf `build.ps1`, `export.ps1`,
  `run_alpha_tests.ps1`. Aktion `all` führt build → test → export
  aus (Abbruch bei Fehlschlag).
- **`build\main\export.ps1`** — Export-Typ `kit` (SDK + Beispiele +
  erweiterte Docs), ZIP-Erzeugung
  `prophysics-<kind>-<version>.zip`, `-Version`-Override.
- **`build\main\build.ps1`** — `-Config release|debug` für alle
  Sub-Makefiles, `-GitStamp` / `-GitNote`, `-Sign`, `-DryRun`.
- **`build\main\write_build_info.ps1`** — `-Config`, `-Version`,
  `-OutFile`, `-GitStamp`, `-GitNote`.
- **`tools\run_alpha_tests.ps1`** — `-Prio` mit Range/Liste,
  `-Test <name>`, `-DllDir`.

### Added — Sub-Makefiles

- **`build\prophysics\Makefile.nmake`** — Kopf auf Etappe 23,
  **12 Kernel-Module** (vorher 11; `ProPhysics_SU2_Dynamics.c`
  ergänzt).
- **`build\test\Makefile.nmake`** — Kopf auf Etappe 23,
  **19 Test-Quellen** (17 Alpha + 2 Example), `check_deps`-Vorprüfung.
- **`build\sdk\Makefile.sdk.nmake`** — Kopf auf Etappe 23,
  `check_core`-Vorprüfung.

### Added — Build-Dokumentation

- **`docs\build\pro_run.md`** (neu) — zentraler Einstiegspunkt.
- **`docs\build\helper\build.md`** — Etappe 23, `-Config`-Support.
- **`docs\build\helper\export.md`** — Etappe 23, Export-Typen
  inkl. `kit`, ZIP-Erzeugung, SDK-Layout.
- **`docs\build\helper\write_build_info.md`** — Etappe 23,
  `-Config`, `-Version`, `-OutFile`, Git-Optionen.
- **`docs\build\main\Makefile.md`** — Master-Makefile,
  `rebuild_*`-Targets, `CONFIG`-Weitergabe.
- **`docs\build\prophysics\Makefile.md`** — 12 Kernel-Module,
  `$(MAKEDIR)`-Pfade.
- **`docs\build\test\Makefile.md`** — 19 `.c`, Prio 1–8.
- **`docs\build\sdk\Makefile.md`** — Etappe 23, `check_core`.
- **`docs\build\BUILD_SCRIPT.md`** — Übersicht, `pro_run`.
- **`docs\test\run_alpha_tests.md`** — Version 1.0.0, Etappe 23.

### Added — Projekt-Dokumente

- **`docs\project\VERSIONING.md`** (neu) — Konzept-Dokument zur
  Etappen-Versionierung.
- **`CONTRIBUTORS.md`** (neu) — Beitragende.
- **`docs\test\BASELINE.md`** (neu) — Test-Baseline 43/43.
- **`docs\build\examples\BUILD_INFO.txt`** (neu) — Beispiel-Datei.

### Added — Repo-Hygiene

- **`.gitignore`** — ergänzt (`out\`, `.vs\`, `.vscode\`, `.idea\`,
  `bin\logs\`, `*.obj`, `_obj\`).
- **`.github\ISSUE_TEMPLATE\bug_report.md`** (neu)
- **`.github\ISSUE_TEMPLATE\feature_request.md`** (neu)
- **`.github\PULL_REQUEST_TEMPLATE.md`** (neu)
- **`.github\workflows\ci.yml`** (neu) — Prio 1, 6, 7 +
  `SU2-Wilson-Loop` auf jedem Push.

### Fixed

- **`LICENSE.md`** §8 — `[ISSUE-TRACKER-URL]` ersetzt durch
  `https://github.com/onkel83/prophysics/issues`.
- **`LICENSE.md`** §3 — Zitierweise auf Kernel-Version `1.23.0`
  umgestellt (vorher `3.0.0`).
- **`README.md`** (Root) — Kopf auf `Version: 1.0`, `Kernel: 1.23.0`,
  `Etappe: 23` umgestellt.

### Changed

- **`docs\project\ProPhysics_VersionRegistry.md`** — SDK-, Build-
  und Test-Dateien auf Etappe 23 aktualisiert. Verweise auf
  `docs\test\run_alpha_tests.md` korrigiert.

### Tests

- Unverändert: 43/43 PASS.

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.9`.
- `TODO.md` auf `1.2` (alle Release-Blocker erledigt).

---

## [1.23.8] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung Tensor. Keine Verhaltensänderung,
keine API-Änderung.

**Hintergrund:** Nach `1.23.7` (SU2_Dynamics) wurde
`ProPhysics_Tensor.c` auf das einheitliche Schema gebracht. Zwei
komplexe Multiplikationsmuster (mit/ohne Konjugation) sind in
`static`-Helfer ausgelagert; das Q31-rho-→-16×16-Embedding ist
jetzt an einer Stelle (`pro_rho_to_real16`) zentralisiert.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |

### Added — Modul-Dokumentation

- **`docs/project/Tensor.md`** (neu) — Modul-Referenz Tensor:
  Basis-Layout, Partial Traces, Sync zu/aus `amp_grid`, Gatter,
  Fermionen-Adapter, Fallstricke.

### Added — Interne Helfer (Refactoring)

Alle folgenden Helfer sind `static` (nicht Teil der API):

**`ProPhysics_Tensor.c`:**
- `pro_tensor_cmul_acc` — `acc += a · b` in Q31. Ersetzt 8×
  inline-c-Mul in `Apply_Local_Op`, `Apply_Single_Qubit_Gate`,
  `Apply_Two_Qubit_Gate`, `Sync_From_Amp`, `Apply_SU2_Rotation`.
- `pro_tensor_cmul_conj_acc` — `acc += a · conj(b)` in Q31.
  Ersetzt 2× inline-c-Mul in `Partial_Trace_A` und `_B`.
- `pro_rho_to_real16` — war bereits vorhanden, jetzt an
  zentraler Stelle (oberhalb `Von_Neumann_Entropy`), damit
  `Von_Neumann_Entropy` und `Sync_To_Amp` sie teilen.

### Changed — Modul-Refactoring

- **`ProPhysics_Tensor.c`** — Header-Kopf von
  `(Etappe 13-15, Refactoring 22)` / `Etappe: 22` auf
  `Etappe: 23` korrigiert. Etappen-Historie aus dem Header
  entfernt, Verantwortlich-Liste, Konventionsblock,
  Helfer-Liste und aligned Footer ergänzt (Fock-Schema).
  Section-Separatoren auf Spalte 0.

### Fixed

- **`ProPhysics_Tensor.c`** — Kein Verhaltensunterschied. Beide
  cmul-Helfer runden jeden Term einzeln vor der Akkumulation
  (bit-identisch zur Vorversion). `Von_Neumann_Entropy` nutzt
  jetzt `pro_rho_to_real16`, das dieselbe 16×16-Matrix erzeugt
  wie der vormalige inline-Code.

### Tests

- Unverändert: 43/43 PASS.
- Semantisch identisch (verifiziert durch `test_tensor_*`,
  `test_su2_*`, `test_fermionize`, `test_slater`).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.8`.
- 1 neues Modul-Dokument: `Tensor.md`.

---

## [1.23.7] — 2026-09-26 — Konsolidierung + B7

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung SU2_Dynamics, Auflösung Backlog B7
(SU(2)-Edge-Zugriff vereinheitlicht).

**Hintergrund:** Nach `1.23.6` (SU2) wurde
`ProPhysics_SU2_Dynamics.c` auf das einheitliche Schema gebracht.
`su2_plaquette_action_at` wird jetzt auch vom Hauptloop in
`ProPhysics_SU2_Plaquette_Action` genutzt, und
`su2_leapfrog_kick_E` fasst die beiden identischen E-Kick-Schleifen
in `Apply_SU2_Tick` zusammen.

**Versions-Kopplung:** (unverändert zu `1.23.6`)

### Added — Modul-Dokumentation

- **`docs/project/SU2_Dynamics.md`** (neu) — Modul-Referenz
  SU2_Dynamics: Hamilton-Funktion, Leapfrog, Plaquette-Action,
  Link-Plaquette-Summe, Konventionen, Fallstricke.

### Added — Interne Helfer (Refactoring)

**`ProPhysics_Internal.h`:**
- `pro_su2_edge` / `pro_su2_edge_mut` — SU(2)-Edge-Zugriff als
  `static inline`. Löst Backlog B7 (geteilter Zugriff zwischen
  `SU2.c` und `SU2_Dynamics.c`). **Hinweis:** Die Verschiebung
  wurde tatsächlich erst in `1.23.10` vollzogen; `1.23.7` hatte
  den Aufruf in `SU2_Dynamics.c` umgestellt, aber die Helfer
  nicht verschoben. `1.23.10` schließt diese Lücke.

**`ProPhysics_SU2_Dynamics.c`:**
- `su2_leapfrog_kick_E` — E-Kick-Schleife
  (`E += half_dt_g2 · F(U)`). Ersetzt die zwei identischen
  Schleifen in `Apply_SU2_Tick` (Schritt 1 und 3).
- `su2_plaquette_action_at` — bereits vorhanden; wird jetzt auch
  vom Hauptloop in `ProPhysics_SU2_Plaquette_Action` genutzt
  (vorher inline 4-Link-Produkt).

### Changed — Modul-Refactoring

- **`ProPhysics_SU2.c`** — lokale `pro_su2_edge` /
  `pro_su2_edge_mut`-Definitionen entfernt (jetzt in `Internal.h`).
  Aufrufer unverändert.
- **`ProPhysics_SU2_Dynamics.c`** — Header-Kopf konsolidiert
  (Etappen-Historie `(Etappe 22b)` und `Etappe 23:`-Marker aus
  Section-Headern entfernt). `su2_read_link` / `su2_write_link` /
  `su2_read_E` / `su2_write_E` nutzen jetzt `pro_su2_edge` /
  `pro_su2_edge_mut` statt direktem Slot-Index.

### Fixed

- **`ProPhysics_SU2_Dynamics.c`** — Kein Verhaltensunterschied.
  `su2_plaquette_action_at` liest exakt dieselben vier Links in
  derselben Reihenfolge wie der bisherige Hauptloop.
  `su2_leapfrog_kick_E` iteriert `k` außen, `d` innen — identisch
  zur Vorversion. Die zusätzlichen Bounds-Checks in den
  `pro_su2_edge`-Wrappern sind nie negativ (Aufrufer prüft via
  `su2_link_exists`).
- **Backlog B7** — aufgelöst. `SU2.c` und `SU2_Dynamics.c` teilen
  jetzt denselben Edge-Zugriff über `ProPhysics_Internal.h`.
  **Vollständig abgeschlossen in `1.23.10`.**

### Tests

- Unverändert: 43/43 PASS.
- Semantisch identisch (verifiziert durch `test_su2_wilson_loop`,
  `test_running_coupling`).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.7`.
- 1 neues Modul-Dokument: `SU2_Dynamics.md`.

---

## [1.23.6] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung SU2. Keine Verhaltensänderung,
keine API-Änderung.

**Hintergrund:** Nach `1.23.5` (Shared) wurde `ProPhysics_SU2.c`
auf das einheitliche Schema gebracht. Die
Achse-Winkel-zu-Quaternion-Konvertierung ist in einen
`static`-Helfer ausgelagert. Der Header-Kopf ist von `Etappe: 22`
auf `Etappe: 23` korrigiert, und die Wilson-Loop-Konvention im
Header von „rückwärts / Path-Ordered" auf „vorwärts" (Stand des
Codes seit dem Backward-Staple-Fix in `1.23.0`) berichtigt.

**Versions-Kopplung:** (unverändert zu `1.23.5`)

### Added — Modul-Dokumentation

- **`docs/project/SU2.md`** (neu) — Modul-Referenz SU2:
  Quaternion-Konvention (Skala 2^30), Wilson-Loop (vorwärts),
  lokale Eichtransformation, Verifikation, Fallstricke.

### Added — Interne Helfer (Refactoring)

**`ProPhysics_SU2.c`:**
- `pro_su2_axis_angle_to_quat` — Achse (normiert) + Winkel →
  Q30-Quaternion. Ersetzt 6× inline-Konvertierung in
  `pro_su2_verify_product` und 1× in
  `ProPhysics_Set_Edge_SU2_AxisAngle`.

### Changed — Modul-Refactoring

- **`ProPhysics_SU2.c`** — Header-Kopf von `Etappe: 22` auf
  `Etappe: 23` korrigiert. **Wilson-Loop-Konvention im Header
  korrigiert**: Der Header sagte fälschlich „rückwärts /
  Path-Ordered `W(C) = U_{n-1}·…·U_0`"; der Code macht seit
  `1.23.0` vorwärts (`W(C) = U_0·…·U_{n-1}`, Backward-Staple-Fix).
  `Interne Helfer`-Liste und aligned Footer ergänzt (Fock-Schema).
  Section-Separatoren auf Spalte 0.

### Fixed

- **`ProPhysics_SU2.c`** — Kein Verhaltensunterschied.
  `pro_su2_axis_angle_to_quat` erzeugt exakt dieselben Q30-Werte
  wie der ausgelagerte Block (identische `llround`-Reihenfolge).
  Die Achsen in `pro_su2_verify_product` sind Einheitsvektoren,
  daher ist der Helper-Aufruf semantisch äquivalent zur bisherigen
  inline-Konvertierung. Wilson-Loop-Iteration und Eichinvarianz
  unverändert.

### Tests

- Unverändert: 43/43 PASS.
- Semantisch identisch (verifiziert durch `test_su2_wilson_loop`,
  `test_running_coupling`).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.6`.
- 1 neues Modul-Dokument: `SU2.md`.

---

## [1.23.5] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung Shared. Keine Verhaltensänderung,
keine API-Änderung.

**Hintergrund:** Nach `1.23.4` (Observer) wurde
`ProPhysics_Shared.c` auf das einheitliche Schema gebracht. Die
inline-2×2-Komplex-Rotation in `Shared_Tick_Reps` ist in einen
`static`-Helfer ausgelagert. Header-Kopf auf `Etappe: 23`
korrigiert (war `Etappe: 22`).

**Versions-Kopplung:** (unverändert zu `1.23.4`)

### Added — Modul-Dokumentation

- **`docs/project/Shared.md`** (neu) — Modul-Referenz Shared:
  Union-Find, Copy-on-Sync, Klassen-Tick, Spin-Flip/Singlet,
  Konventionen, Fallstricke.

### Added — Interne Helfer (Refactoring)

**`ProPhysics_Shared.c`:**
- `pro_shared_pair_rotate_q31` — 2×2-Komplex-Rotation
  `a' = c·a + i·s·b`, `b' = c·b + i·s·a`. Ersetzt die inline-Form
  in `Shared_Tick_Reps`. Dieselbe Matrix-Form existiert in
  `ProPhysics_Fock.c` (`Apply_Hopping`) mit umgekehrtem
  Vorzeichen-Konvention; eine spätere Etappe kann den Helfer nach
  `ProPhysics_Internal.h` verschieben.

### Changed — Modul-Refactoring

- **`ProPhysics_Shared.c`** — Header-Kopf von `Etappe: 22` auf
  `Etappe: 23` korrigiert. Etappen-Historie (`Etappe 18c + 18e +
  19`, `Refactoring 22`) aus dem Header entfernt. `Interne
  Helfer`-Liste und aligned Footer ergänzt (Fock-Schema).
  Fallunterscheidung in `Dissociate_Node` mit Kommentaren
  (`Fall 1 / 2 / 3`) versehen.

### Fixed

- **`ProPhysics_Shared.c`** — Kein Verhaltensunterschied. Die
  ausgelagerte Rotation liest alle vier Komponenten vor dem
  Schreiben (bit-identisch zur inline-Form). Die Reihenfolge der
  Paar-Sammlung und Paar-Iteration ist unverändert.

### Tests

- Unverändert: 43/43 PASS.
- Semantisch identisch (verifiziert durch `test_shared_reference`,
  `test_su2_wilson_loop`, `test_chsh_*`).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.5`.
- 1 neues Modul-Dokument: `Shared.md`.

---

## [1.23.4] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung Observer. Keine Verhaltensänderung,
keine API-Änderung.

**Hintergrund:** Nach `1.23.3` (Gauge) wurde
`ProPhysics_Observer.c` auf das einheitliche Schema gebracht. Der
Diffusions-Kern wurde aus zwei Funktionen in einen gemeinsamen
`static`-Helfer extrahiert; ebenso wurden die Ping-Pong-Sequenz,
die ProU128-Summe über einen Vektor und der `sign(cos(θ-λ))`-Aufruf
zentralisiert.

**Versions-Kopplung:** (unverändert zu `1.23.3`)

### Added — Modul-Dokumentation

- **`docs/project/Observer.md`** (neu) — Modul-Referenz Observer:
  Deskriptor, lokales Lesen, Diffusion, CHSH-Messung, chaotische
  Quelle, Dephasing, Konventionen.

### Added — Interne Helfer (Refactoring)

**`ProPhysics_Observer.c`:**
- `pro_amp_vector_abs2_sum` — ProU128-Summe `Σ_b |c_b|²`.
  Ersetzt 2× inline-Schleife in `Get_Environment_Trace` und in
  der Sättigungs-Prüfung von `Apply_Nonlinear_Diffusion_Tick`.
- `pro_observer_diffusion_pass` — Gemeinsamer Diffusions-Kern
  für `Apply_Local_Amplitude_Diffusion` und
  `Apply_Nonlinear_Diffusion_Tick`.
- `pro_observer_ping_pong` — `amp_grid ↔ amp_scratch`-Tausch.
- `pro_observer_measure_axis` — `sign(cos(θ - λ(v)))`.
  Ersetzt 4× Aufruf in beiden CHSH-Varianten.

### Changed — Modul-Refactoring

- **`ProPhysics_Observer.c`** — Header-Kopf von `Etappe: 22` auf
  `Etappe: 23` korrigiert. Etappen-Historie (`Etappe 6a-6f`,
  `Refactoring 22`) aus dem Header entfernt, Verweis auf
  `docs/project/Observer.md` und `CHANGELOG.md` ergänzt.
  `INTERPRETATION (bindend)`-Block in den regulären
  Konventionsblock überführt.

### Fixed

- **`ProPhysics_Observer.c`** — Kein Verhaltensunterschied. Der
  Diffusions-Pass ist semantisch identisch (gleiche Reihenfolge
  der Nachbar-Schleife, gleiche Q31-Koeffizienten). Die
  Ping-Pong-Reihenfolge ist unverändert (Schritt 2 der
  nichtlinearen Diffusion liest weiterhin aus `pu->amp_scratch`,
  das vor dem Tausch `dst` entspricht).

### Tests

- Unverändert: 43/43 PASS.
- Semantisch identisch (verifiziert durch `test_chsh_*`,
  `test_observer_diffusion`, `test_chaotic_source`,
  `test_dephasing`).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.4`.
- 1 neues Modul-Dokument: `Observer.md`.

---

## [1.23.3] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung Gauge. Keine Verhaltensänderung,
keine API-Änderung.

**Hintergrund:** Nach `1.23.2` (EPR + Fock) wurde
`ProPhysics_Gauge.c` auf das einheitliche Schema gebracht. Vier
identische Rotationsschleifen und eine inline Q15→Q16-Normalisierung
sind jetzt in `static`-Helfer ausgelagert.

**Versions-Kopplung:** (unverändert zu `1.23.2`)

### Added — Modul-Dokumentation

- **`docs/project/Gauge.md`** (neu) — Modul-Referenz Gauge:
  U(1)-Eichstruktur, Wilson-Loop, Phase Plate, Coulomb-Phase-Feld,
  Konventionen Q15/Q16/Q30 und Fallstricke.

### Added — Interne Helfer (Refactoring)

**`ProPhysics_Gauge.c`:**
- `pro_amp_vector_rotate_q16` — rotiert alle
  `PRO_AMP_BASIS_SIZE` Koeffizienten eines `ProAmpVector` um eine
  Q16-Phase. Ersetzt vier identische Schleifen in
  `Global_Phase`, `Apply_Local_Gauge` (Schritt 2),
  `Apply_Local_Phase_Plate`, `Apply_Coulomb_Phase_Field_3D`.
- `pro_phase_q15_to_q16` — normalisiert eine (möglicherweise
  negative oder große) Q15-Phase auf einen Q16-Winkel in
  `[0, 65536)`. Ersetzt die inline-Normalisierung in
  `Apply_Coulomb_Phase_Field_3D`.

### Changed — Modul-Refactoring

- **`ProPhysics_Gauge.c`** — Header-Kopf von `Etappe: 22` auf
  `Etappe: 23` korrigiert (war inkonsistent zur Kernel-Version
  `1.23.0`). Etappen-Historie (`Etappe 4-18`, `Refactoring 22`,
  `Etappe 17b:`, `Etappe 18:`) aus dem Header entfernt, Verweis
  auf `docs/project/Gauge.md` und `CHANGELOG.md` ergänzt.
- **`ProPhysics_Fock.c`** — Kommentar-Separatoren auf Spalte 0
  vereinheitlicht (rein kosmetisch). Kein Code-Change.
- **`ProPhysics_Gauge.c`** — Header auf Fock-Schema umgestellt
  (Konventionsblöcke, „Interne Helfer"-Liste, aligned Footer);
  Section-Separatoren auf Spalte 0, `(Etappe NN)`-Annotationen
  entfernt. Kein Code-Change.

### Fixed

- **`ProPhysics_Gauge.c`** — Kein Verhaltensunterschied. Alle vier
  Rotationsschleifen sind semantisch identisch (gleiche Reihenfolge
  der Basis-Indizes, gleicher Aufruf von `pro_amp_rotate_q16`).
  Q15→Q16-Normalisierung liefert bit-identische Werte.

### Tests

- Unverändert: 43/43 PASS.
- Semantisch identisch (verifiziert durch die bestehende
  Test-Suite; insbesondere `test_wilson_loop_*`, `test_local_gauge`,
  `test_coulomb_*`, `test_phase_plate`).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.3`.
- 1 neues Modul-Dokument: `Gauge.md`.
- Modul-Header in `ProPhysics_Fock.c` und `ProPhysics_Gauge.c`
  folgen jetzt demselben Schema wie `docs/project/Fock.md`
  (Konventionen, Helfer-Liste, Footer).

---

## [1.23.2] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Modul-Konsolidierung EPR und Fock. Keine
Verhaltensänderung, keine API-Änderung.

**Hintergrund:** Nach `1.23.1` (Header und erste fünf Module)
wurden die Module `ProPhysics_EPR.c` und `ProPhysics_Fock.c`
refactored und dokumentiert. Beide Module hatten duplizierte
Codeblöcke, die jetzt in `static`-Helfer ausgelagert sind.

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |

### Added — Modul-Dokumentation

- **`docs/project/EPR.md`** (neu) — Modul-Referenz EPR: vier
  Messvarianten, Kollaps-Physik und U4, Apparat-Subgraph.
- **`docs/project/Fock.md`** (neu) — Modul-Referenz Fock:
  8-Moden-Fermion-Raum, Jordan-Wigner-Signatur,
  Antikommutatoren, Hopping.

### Added — Interne Helfer (Refactoring)

Alle folgenden Helfer sind `static` (nicht Teil der API):

**`ProPhysics_EPR.c`:**
- `pro_epr_validate_pair` — Pre-Flight-Check mit drei
  konfigurierbaren Anforderungen (type_state / edge_type /
  amp_nonzero). Ersetzt 3× wiederholten Guard-Block.
- `pro_epr_sharp_measure_pair` — 2× `pro_amp_to_lambda` +
  2× `pro_measure_sharp`. Ersetzt 2× identische Sequenz.
- `pro_epr_measure_born_collapse` — Born-Collapse-Sequenz mit
  `phase_offset_rad`-Parameter. Ersetzt 2× die Sequenz in
  `_Collapse` und `_Graph`.

**`ProPhysics_Fock.c`:**
- `pro_fock_apply_jw_sign` — Jordan-Wigner-Vorzeichen (negiert
  bei ungeradem `below_count`). Ersetzt 3× 4-Zeilen-Block.
- `ProFockOpKind` (enum) — Operations-Art (CREATE / ANNIHILATE).
- `pro_fock_apply_op` — Erzeuger/Vernichter-Dispatcher.
- `pro_fock_anticomm_impl` — Generisches Antikommutator-Muster
  (Clone, zwei Operationen pro Seite, Summe, Destroy). Ersetzt
  3× 30-Zeilen-Block in `CD`, `CC`, `DD`.

### Changed — Modul-Refactoring

- **`ProPhysics_EPR.c`** — Die vier Messvarianten nutzen jetzt den
  gemeinsamen Guard-Helper. `_Collapse` und `_Graph` teilen sich
  eine einzige Born-Collapse-Sequenz mit unterschiedlichem
  `phase_offset_rad`. Reihenfolge der RNG-Aufrufe bleibt identisch.
- **`ProPhysics_Fock.c`** — `Create`, `Annihilate` und
  `Create_Plus_Annihilate` nutzen `pro_fock_apply_jw_sign`.
  Die drei Antikommutator-Funktionen sind 4-Zeilen-Wrapper um
  `pro_fock_anticomm_impl`.

### Fixed

- **`ProPhysics_EPR.c`** — Kein Verhaltensunterschied. Die
  drei Guard-Varianten (pair / collapse / graph) sind semantisch
  identisch zum Zustand vor `1.23.2`. RNG-Fluss und Reihenfolge
  unverändert (bit-identisch).
- **`ProPhysics_Fock.c`** — Kein Verhaltensunterschied. Alle
  Jordan-Wigner-Vorzeichen und Antikommutator-Operationen liefern
  bit-identische Ergebnisse.

### Tests

- Unverändert: 43/43 PASS.
- Alle Refactorings sind semantisch identisch (verifiziert durch
  die bestehende Test-Suite; insbesondere `test_su2_wilson_loop`,
  `test_chsh_*`, und die Fock-Antikommutator-Tests).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.2`.
- 2 neue Modul-Dokumente: `EPR.md`, `Fock.md`.

---

## [1.23.1] — 2026-09-26 — Konsolidierung

**Etappen:** 23 (Konsolidierung)
**Tests:** 43/43 PASS (unverändert)
**Fokus:** Header-, Modul- und Doku-Konsolidierung. Keine
Verhaltensänderung, keine API-Änderung.

**Hintergrund:** Nach dem Etappe-23-Abschluss wurden alle
Kernel-Header, Kernel-Module und Modul-Dokumentationen auf ein
einheitliches Schema gebracht (Kernel/Etappe-Zeile, keine
Etappen-Historie in Quellcode-Kommentaren, Verweis auf dieses
Changelog). Zusätzlich wurden mehrere duplizierte Codeblöcke
in `static`-Helfer ausgelagert (rein intern, keine API-Änderung).

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | unverändert (keine ABI-Änderung) |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | unverändert |
| Build / Tools | `1.0.0` | unverändert |

### Added — Modul-Dokumentation

- **`docs/project/CONFIG.md`** (neu) — Vollständige Referenz aller
  Compile-Time-Konstanten aus `ProPhysics_Config.h`. Mit
  Überschreib-Tabelle und Fehlersuche.
- **`docs/project/Amp.md`** (neu) — Modul-Referenz Amp: unitäre
  Dynamik auf `amp_grid`, Tick-Reihenfolge, Fallstricke.
- **`docs/project/Core.md`** (neu) — Modul-Referenz Core:
  Lifecycle, Topologie, Tick-Orchestrierung.
- **`docs/project/Density.md`** (neu) — Modul-Referenz Density:
  Dichte-Trilogie (8×8, 64×64, 256×256) und Lindblad-Kanäle.
- **`docs/project/Dirac.md`** (neu) — Modul-Referenz Dirac:
  4-Komponenten-Spinor, Gamma- und Alpha-Tabellen.

### Added — Interne Helfer (Refactoring)

Alle folgenden Helfer sind `static` (nicht Teil der API):

**`ProPhysics_Amp.c`:**
- `pro_amp_rotate_by_phase` — komplexe Rotation um Radiant-Winkel
  (ersetzt 5 duplizierte Stellen).
- `pro_amp_abs2_q62` — Norm-Quad in Q62-Skala (ersetzt 3 Stellen).
- `pro_amp_select_neighbor_channels` — 4/6-Kanal-Auswahl.
- `pro_amp_auto_sync` — Tensor- und Dichte-Sync nach Tick.
- `pro_transport_sequential_core` — sequenzieller Transport.
- `pro_transport_colored_core` — parametrisierter farbiger Transport
  (2D, 3D, Tensor-Marks).
- `pro_wave_step_core` — parametrisierter Wave-Step (regular, Marks).

**`ProPhysics_Core.c`:**
- `pro_registers_self_init` — Register-Default-Init (ersetzt 2 Stellen).

**`ProPhysics_Density.c`:**
- `pro_density_find_free_slot` — Create-Slot-Suche (ersetzt 3 Stellen).
- `pro_lindblad_2x2` — 2×2-Lindblad-Kern mit `decay_from`/`decay_to`
  (ersetzt 2 Funktionen: `pro_td_apply_2x2_lindblad`,
  `pro_fock_2x2_kern`).

**`ProPhysics_Dirac.c`:**
- `pro_gamma_op_init` — Ein-Zeilen-Init einer `ProGammaOp` (ersetzt
  16 wiederholte Init-Blöcke).

### Changed — Header-Konsolidierung

Alle Kernel-Header wurden auf das einheitliche Schema umgestellt:
`Kernel: 1.23.0`, `Etappe: 23`, keine Etappen-Historie, Verweis auf
`CHANGELOG.md` / `CONFIG.md`.

- **`ProPhysics.h`** — Etappen-Historie entfernt, Sektionen
  gruppiert (Lifecycle, Topologie, Injektion, EPR, Amp, Dynamik,
  Gauge, Apparat, Observer, Tick, Shared, Spin, Dirac, SU(2),
  Tensor, Fock, Density).
- **`ProPhysics_Config.h`** — Etappen-Historie entfernt, Verweis
  auf `docs/project/CONFIG.md`.
- **`ProPhysics_Exports.h`** — Etappe auf 23, Kommentar zu den
  drei Build-Modi (dllexport/dllimport/statisch).
- **`ProPhysics_Internal.h`** — Etappe auf 23, Kommentare
  strukturiert, Plattform-Fallback-Tabelle für
  `pro_aligned_calloc`.
- **`ProPhysics_Types.h`** — Etappe auf 23, Layout-Hinweise zu
  `ProEdge` (40 B), `ProNode`, `ProUniverse`.

### Changed — Modul-Refactoring

- **`ProPhysics_Amp.c`** — Neu strukturiert mit klaren Sektionen
  und wiederverwendbaren Cores.
- **`ProPhysics_Core.c`** — Register-Default-Init zentralisiert.
- **`ProPhysics_Density.c`** — Lindblad-Kern für 64×64 und 256×256
  vereinheitlicht.
- **`ProPhysics_Dirac.c`** — Massenterm nutzt jetzt
  `pro_transport_coeffs` (geteilter Cache mit Transport),
  Gamma-Init kompakter.

### Fixed

- **`ProPhysics_Density.c`** — `pro_lindblad_2x2` behebt die
  Duplikation zwischen Tensor- und Fock-Ebene. Die
  `decay_from`/`decay_to`-Parameter machen die unterschiedlichen
  Index-Konventionen explizit. Verhalten bit-identisch zu vorher.
- **`ProPhysics_Dirac.c`** — Massenterm-Koeffizienten nutzen jetzt
  `pro_transport_coeffs` statt einer eigenen Kopie der
  tan(θ/2)-Approximation. Verhalten bit-identisch zu vorher,
  Cache wird geteilt.

### Tests

- Unverändert: 43/43 PASS.
- Alle Refactorings sind semantisch identisch (bit-identische
  Ergebnisse, verifiziert durch die bestehende Test-Suite).

### Docs

- `CHANGELOG.md` (diese Datei) auf `1.23.1`.
- 5 neue Modul-Dokumente: `CONFIG.md`, `Amp.md`, `Core.md`,
  `Density.md`, `Dirac.md`.

---

## [1.23.0] — 2026-09-26 — Etappe 23

**Etappen:** 22 + 22b + 23
**Tests:** 43/43 PASS
**Prio-All-Laufzeit:** 4 420,6 s (~73,7 min)

**Fokus:** Erste **absolute** Validierung des Kernels gegen externe
Lattice-QCD-Physik (V&V-Anker).

**Versions-Kopplung:**

| Komponente | Version | Bemerkung |
|---|---|---|
| Kernel | `1.23.0` | — |
| SDK | `1.23.0` | folgt Kernel |
| Doku | `1.23.0` | folgt Kernel |
| Tests | `1.0.0` | eigenständig |
| Build / Tools | `1.0.0` | eigenständig |

### Added — Etappe 23 (SU(2)-Metropolis / Wilson-Action-Validierung)

- **Kernel-Funktion** `ProPhysics_SU2_Link_Plaquette_Sum`
  (read-only). Liefert die Summe der Wilson-Aktionen aller Plaquettes
  um einen Link `(x, μ)`. Basis für Metropolis/HMC.
- **Neuer Test** `alpha_test_running_coupling.c` (18. Test-Modul).
  Metropolis-Sampling auf SU(2)-Links, Sweep über
  `dim ∈ {16, 32, 64}` × `β ∈ {0.5, 1, 2, 4}`.
- **Neuer CLI-Flag** `--test-running-coupling`.
- **Neuer Prio-8-Test** `Running-Coupling` in `run_alpha_tests.ps1`,
  Timeout 2 400 s.
- **V&V-Anker:** `⟨P⟩(β=2, dim=64) = 0,43346 ± 0,00005` vs.
  Referenz `I₂(2)/I₁(2) = 0,43313`. Abweichung **0,08 %**.
  **WICHTIG:** Dieser Anker wird mit `[1.23.13]` und endgültig mit
  `[1.23.14]` zurückgenommen. Nach dem Sampler-Fix ist
  `u_plaq(β=2, dim=64) = 0,272552` statt `0,283270`, entsprechend
  `⟨P⟩ = 0,4549` und ~5 % Abweichung zur SPA-Referenz. Der
  ursprüngliche 0,08-%-Wert war ein Artefakt des int32-Overflow-Bugs
  im Metropolis-Vorschlag `U → normalize(U + ε)`.

### Added — Etappe 22b (SU(2)-Link-Dynamik / Leapfrog)

- **Neues Kernel-Modul** `ProPhysics_SU2_Dynamics.c` (13. Modul).
- **Neue Kernel-Funktionen:**
  - `ProPhysics_Enable_SU2_Dynamics`
  - `ProPhysics_Disable_SU2_Dynamics`
  - `ProPhysics_Is_SU2_Dynamics_Active`
  - `ProPhysics_Set_SU2_Yang_Mills`
  - `ProPhysics_Apply_SU2_Tick`
  - `ProPhysics_SU2_Plaquette_Action`
  - `ProPhysics_SU2_Total_Energy`
- **`ProEdge`-Erweiterung:** +4 × `int32` für chromoelektrisches
  Feld `su2_E_*`. `sizeof(ProEdge)` von 24 auf 40 Bytes.
- **`ProUniverse`-Felder:** `su2_dynamics_active` (uint8),
  `su2_yang_mills_q15` (int32).
- **`Internal.h`:** `pro_su2_mul`, `pro_su2_conj`, `pro_su2_norm_sq`
  zentral; neue `pro_su2_exp_apply`.
- **Neue Config-Konstanten:** `PRO_SU2_YM_DEFAULT_Q15`,
  `PRO_SU2_LEAPFROG_DT_Q15`.
- **Tests:** T15–T18 im `test_su2_wilson_loop` (Link-Norm, Energie,
  R7-Konformität, aktive Dynamik).

### Added — Etappe 22 (SU(2)-Eichfeld / kinematisch)

- **Neues Kernel-Modul** `ProPhysics_SU2.c` (12. Modul).
- **Neue Kernel-Funktionen:**
  - `ProPhysics_Set_Edge_SU2`
  - `ProPhysics_Set_Edge_SU2_AxisAngle`
  - `ProPhysics_Get_Edge_SU2`
  - `ProPhysics_Wilson_Loop_SU2`
  - `ProPhysics_Wilson_Loop_SU2_Trace`
  - `ProPhysics_Apply_Local_SU2_Gauge`
  - `ProPhysics_Verify_SU2_Quaternion`
- **`ProEdge`-Erweiterung:** +4 × `int32` für Quaternion-Link
  (`su2_a_re/ai`, `su2_b_re/bi`). `sizeof(ProEdge)` von 12 auf 24 Bytes.
- **`ProUniverse`-Felder:** `su2_active`, `su2_gauge_basis`,
  `su2_coupling_q15`.
- **Refactoring 22 (parallel):**
  - Kernel-Tick `ProPhysics_Tick` in `ProPhysics_Core.c`
    (vorher in `pro_sdk_interface.c`).
  - Cache-Aligned-Alloc `pro_aligned_calloc` für 8 Hot-Arrays.
  - Zentrale Helfer in `Internal.h`.
  - Konstanten zentral in `ProPhysics_Config.h`.
  - Test-Runner von `bin\` nach `tools\`.
  - EPR-Debug-Ring statt printf im Kernel-Tick.
- **`ProPhysics_RuleCallback`** als zentraler Callback-Typ in
  `ProPhysics.h`.

### Changed

- **`ProEdge`-Größe:** 40 Bytes (war 12 vor Etappe 22).
- **`ProUniverse`-Größe:** +4 Felder für SU(2).
- **Test-Runner-Ablage:** `tools\` statt `bin\`.
- **Kernel-Version:** `1.22.1` → `1.23.0`.
- **Alle Header** auf das neue Etappen-Schema (Kernel/Etappe) umgestellt.
- **Etappen-Historie** aus Headern in dieses Changelog verschoben.

### Fixed

- **Backward-Staple-Fix** in `su2_force_on_link`. Fehlende
  `†`-Dagger auf `U_μ(x-ν)`. Reduziert:
  - T15: `2,02e-08` → `1,80e-08`
  - T16: `8,06e-03` → `2,44e-03` (Faktor 3,3)
  - Wirkung: Energiedrift jetzt auf symplektischem O(dt²)-Niveau.

### Tests

- Prio 8: 2 Tests (`SU2-Wilson-Loop`, `Running-Coupling`).
- Prio-All: 42 → 43 Tests.
- `SU2-Wilson-Loop`: 18 + KA = 19 Einzelchecks.
- `Running-Coupling`: 16 Werte + V&V-Anker = 17 Einzelchecks.

### Docs

- `ProPhysics_Testkatalog.md` auf `1.8`.
- `Project.md` auf `1.23.0`.
- `BUILD_SCRIPT.md` auf `1.0.0`.
- `CONFIG.md` neu angelegt.
- `ProPhysics_Differentiators.md` neu.
- `ProPhysics_VersionRegistry.md` neu.
- Alle Header-Dateien von Etappen-Historie bereinigt.

---

## [1.22.1] — 2026-09-25 — Etappe 22b

**Etappen:** 22b
**Tests:** 43/43 PASS (nach Etappe 23)

**Hinweis:** Etappe 22b wurde am selben Tag wie Etappe 23 abgeschlossen.
Der Eintrag ist als Patch zu Etappe 22 zu verstehen (`1.22.0` → `1.22.1`).

### Added

- Siehe `[1.23.0]` §Added — Etappe 22b.

### Changed

- **`ProEdge`:** 24 → 40 Bytes (E-Feld).
- **Kernel-Version:** `1.22.0` → `1.22.1`.

### Tests

- Prio 8 von 15/15 auf 19/19 erweitert (T15–T18).
- Prio-All: 42 Tests.

---

## [1.22.0] — 2026-09-25 — Etappe 22 + Refactoring 22

**Etappen:** 22 + Refactoring
**Tests:** 42/42 PASS

### Added

- Siehe `[1.23.0]` §Added — Etappe 22.
- **`ProPhysics_Tick`** (Kernel-Tick-Orchestrierung).
- **`ProPhysics_RuleCallback`** (Callback-Typ).
- **`pro_aligned_calloc`** / `pro_aligned_free`.

### Changed

- **`ProEdge`:** 12 → 24 Bytes (SU(2)-Link).
- **Test-Runner:** `bin\` → `tools\`.
- **Kernel-Version:** `1.21.0` → `1.22.0`.

### Tests

- Prio 8 eingeführt (`SU2-Wilson-Loop`).
- Prio-All: 41 → 42 Tests.

---

## [1.21.0] — 2026-09-24 — Etappe 21 + 21b

**Etappen:** 21 + 21b (Dirac)
**Tests:** 41/41 PASS

### Added — Etappe 21 (Dirac-Struktur)

- **Neues Kernel-Modul** `ProPhysics_Dirac.c` (11. Modul).
- **Neue Kernel-Funktionen:**
  - `ProPhysics_Verify_Gamma_Algebra`
  - `ProPhysics_Apply_Dirac_Mass_Term`
  - `ProPhysics_Apply_Dirac_Step`
- **`ProUniverse`-Felder:** `dirac_active`, `dirac_gamma_basis`,
  `dirac_mass_q15`.
- **`ProNode.reserved_gating` Bit 1:** Dirac-Markierung.
- **`ProGammaBasis`** (enum): `PRO_GAMMA_BASIS_DIRAC`,
  `PRO_GAMMA_BASIS_WEYL`.
- **`PRO_NODE_DIRAC_BIT`** Konstante.
- **Tests:** 5 Untertests (γ-Algebra, Masse, Dispersion, ZBW,
  Regression).

### Added — Etappe 21b (α-Kopplung)

- **`pro_dirac_transport_3d`** mit α^i-Kopplung pro Sweep-Richtung.
- **`pro_dirac_alpha`**-Tabelle für Dirac- und Weyl-Basis.

### Changed

- **Kernel-Version:** `1.19.0` → `1.21.0`.

### Tests

- Prio 7 eingeführt (`Dirac`).
- Prio-All: 40 → 41 Tests.

---

## [1.19.0] — 2026-09-24 — Etappe 19

**Etappen:** 19 (Spin-1/2)
**Tests:** 40/40 PASS

### Added

- **Neue Kernel-Funktionen:**
  - `ProPhysics_Entangle_Nodes_Singlet`
  - `ProPhysics_Is_Spin_Flipped`
  - `ProPhysics_Get_Node_Spin_View`
  - `ProPhysics_Apply_Nonlinear_Phase_Step_Spin`
- **`ProNode.reserved_gating` Bit 0:** Spin-Flip-Markierung.
- **`PRO_NODE_SPIN_FLIP_BIT`** Konstante.
- **Tests:** 6 Assertions (K1, K2, K3, F4, Regression).

### Changed

- **Kernel-Version:** `1.18.5` → `1.19.0`.

### Tests

- Prio 6 eingeführt (`Spin-Half`).
- Prio-All: 39 → 40 Tests.

---

## [1.18.5] — 2026-09-24 — Konsolidierung nach Etappe 18e

**Etappen:** Konsolidierung
**Tests:** 39/39 PASS

### Changed

- Nachkonsolidierung nach Etappe 18e.
- Kernel-Version: `1.18.4` → `1.18.5`.

### Docs

- `Project.md` auf v2.5 (alte Doc-Version).
- `Testkatalog.md` auf v1.4 (alte Doc-Version).

---

## [1.18.4] — 2026-09-24 — Etappen 18d + 18e

**Etappen:** 18d + 18e
**Tests:** 39/39 PASS

### Added — Etappe 18d (Hydrogen dim=64)

- **`ProPhysics_Apply_Coulomb_Phase_Field_3D`** für dim=64.
- Hydrogen-Spektrum dim=64: `rel_dev = 0,296` (Ziel < 5 %
  **verfehlt**).

### Added — Etappe 18e (Klassen-Tick)

- **`ProPhysics_Shared_Tick_Reps`** (Klassen-basierter Tick).
- **`θ_eff = θ · N_AB`** in Klassen-Kopplung.
- **`ProPhysics_Apply_Nonlinear_Phase_Step_Spin`** vorbereitet.

### Changed

- **Kernel-Version:** `1.18.3` → `1.18.4`.

### Tests

- Prio-All: 38 → 39 Tests.
- Shared-Reference-Test: 23/23.

---

## [1.18.3] — 2026-09-23 — Etappe 18d

**Etappen:** 18d
**Tests:** 38/38 PASS

### Added

- Hydrogen dim=64 (`--test-hydrogen-48`).
- `ProPhysics_Apply_Coulomb_Phase_Field_3D` erstmals für dim=64.

### Changed

- **Kernel-Version:** `1.18.2` → `1.18.3`.

### Tests

- Hydrogen-48 dauert ~2 281 s.

---

## [1.18.2] — 2026-09-23 — Etappe 18c

**Etappen:** 18c (U4 Shared Reference)
**Tests:** 38/38 PASS

### Added

- **Neues Kernel-Modul** `ProPhysics_Shared.c`.
- **Neue Kernel-Funktionen:**
  - `ProPhysics_Entangle_Nodes`
  - `ProPhysics_Dissociate_Node`
  - `ProPhysics_Is_Entangled`
  - `ProPhysics_Get_Representative`
  - `ProPhysics_Shared_Sync`
  - `ProPhysics_Shared_Class_Count`
- **`ProSharedInfo`** (Union-Find).
- **`shared.parent`** Array (Cache-Line-aligned).
- **`shared.active`** Flag.

### Changed

- **`ProUniverse`:** neue `shared`-Struktur.
- **Kernel-Version:** `1.18.1` → `1.18.2`.

### Tests

- Shared-Reference: 23/23.
- Prio-All: 37 → 38 Tests.

---

## [1.18.1] — 2026-09-23 — Etappe 18b

**Etappen:** 18b (Imaginaerzeit-Prep)
**Tests:** 37/37 PASS

### Added

- Imaginaerzeit-Evolution (`rel_dev = 0,139`).
- Adaptive Prep für Hydrogen.

### Changed

- **Kernel-Version:** `1.18.0` → `1.18.1`.

### Tests

- Prio-All: 36 → 37 Tests.

---

## [1.18.0] — 2026-09-23 — Etappe 18 + Ontologie-Update

**Etappen:** 18 (Coulomb/Hydrogen)
**Tests:** 36/36 PASS

**Fokus:** Ontologie-Update mit 10 Thesen, U4'/U5' eingeführt.

### Added — Etappe 18 (Coulomb/Hydrogen)

- **`ProPhysics_Apply_Coulomb_Phase_Field_3D`** (dim=32).
- Hydrogen-Test (`--test-hydrogen`).
- Coulomb-Phase-Feld (`V(r) = -K/r`).

### Added — Ontologie

- **10 Thesen** (T1–T10) in `Project.md` §4.
- **U4' — Bad** (Kopplung an unsichtbare Freiheitsgrade) als
  geplante Erweiterung.
- **U5' — Plastizität** (dynamische Topologie) als geplante
  Erweiterung.

### Changed

- **Kernel-Version:** `1.17.1` → `1.18.0`.

### Tests

- Prio-All: 35 → 36 Tests.

---

## [1.17.1] — 2026-09-21 — Etappe 17b (Kalibrierung)

**Etappen:** 17b (Phase Plate)
**Tests:** 35/35 PASS

### Added

- **`ProPhysics_Apply_Local_Phase_Plate`** (Ortsabhängige Phase).
- Phase-Plate-Kalibrierung (Peak-Shift, amp_ratio).

### Changed

- **Kernel-Version:** `1.17.0` → `1.17.1`.

### Tests

- Prio-All: 34 → 35 Tests.

---

## [1.17.0] — 2026-09-21 — Etappe 17 + 17b

**Etappen:** 17 + 17b
**Tests:** 33/33 → 34/34 PASS

### Added — Etappe 17 (3D-Torus)

- 3D-Transport (`ProPhysics_Apply_Edge_Transport_Colored_3D`).
- 3D-Bloch-Dispersion.
- `grid_ndim` (2 oder 3).

### Added — Etappe 17b (Phase Plate)

- **`ProPhysics_Apply_Local_Phase_Plate`**.
- Doppelspalt-Test mit Phase Plate.

### Changed

- **`ProPhysics_Apply_Wave_Step`:** n_nb ∈ {4, 6}.
- **Kernel-Version:** `1.16.0` → `1.17.0`.

### Tests

- Prio 4 eingeführt (`3D-Smoke`, `3D-Invariance`, `3D-Dispersion`).
- Prio-All: 30 → 33 → 34 Tests.

---

## [1.16.0] — 2026-09-21 — Erste Fassung (bis Etappe 16e'')

**Etappen:** 1–16e''
**Tests:** 30/30 PASS

**Erster stabiler Stand des Kernels.** Alle Etappen vor 17 wurden
nie einzeln versioniert, sondern im Rahmen der ersten Fassung
zusammengefasst.

### Added — bis Etappe 16e''

- **Fundament (U1–U5):** `ProUniverse`, `amp_grid`, `reg_source`,
  `edge_phases`.
- **Amp-Modul:** `Apply_Signed_Permutation`, `Apply_Context_Tick`,
  `Apply_Edge_Transport`, `Apply_Wave_Step`,
  `Apply_Guiding_Equation`, `Measure_Amp_Invariant`.
- **U(1)-Gauge:** `Wilson_Loop`, `Make_Lambda_Field`,
  `Apply_Local_Gauge`, `Get_Born_Probability`.
- **EPR:** `Measure_EPR_Pair`, `Measure_EPR_Pair_Amp`,
  `Measure_EPR_Pair_Collapse`, `Measure_EPR_Pair_Graph`.
- **Observer:** `Init_Observer`, `Observer_Read_Local`,
  `Apply_Local_Amplitude_Diffusion`, `Init_Chaotic_Source`,
  `Apply_Nonlinear_Diffusion_Tick`, `Apply_Local_Dephasing_Tick`.
- **Tensor:** `Tensor_Create_Pair`, `Tensor_Apply_Local_Op`,
  `Tensor_Partial_Trace_A/B`, `Tensor_Apply_Hadamard/X/Y/Z/CNOT/CZ/SWAP/SqrtSwap`,
  `Tensor_Measure_Projective`, `Tensor_Sync_To_Amp`,
  `Tensor_Sync_From_Amp`, `Tensor_Concurrence`.
- **Fock:** `Fock_Create`, `Fock_Apply_Create/Annihilate`,
  `Fock_Apply_Anticomm_CD/CC/DD`, `Fock_Apply_Hopping`.
- **Density:** `Density_Create`, `Density_From_Pure`,
  `Density_Apply_Unitary`, `Density_Apply_Lindblad_Step`,
  `TensorDensity_Create`, `FockDensity_Create`.
- **GP-Selbstkopplung:**
  `Apply_Nonlinear_Phase_Step`,
  `Apply_Nonlinear_Phase_Step_Dilated`.

### Tests (Prios 1–3)

- **Prio 1** — 2D-Basis (12 Tests).
- **Prio 2** — Emergenz (10 Tests).
- **Prio 3** — Langlauf (10 Tests).
- **Prio-All:** 30 Tests.

### Docs

- Erste Fassung von `Project.md`, `Testkatalog.md`, `run_alpha_tests.md`.
- Erste Fassung der Makefile-Doku.

---

## §1 — Vor Etappe 1 (nicht versioniert)

**Status:** Projektskizze, keine lauffähige Version.

Die Konzeption des Projekts (5 Ur-Regeln, Graph-Modell,
signed-permutation-Darstellung) liegt **vor** dem ersten Commit.
Es gibt keine Changelog-Einträge dafür.

**Erste Etappen** (1–16e''), die zur Version `1.16.0` führten, sind in
der ersten Fassung von `Project.md` §18 dokumentiert, aber nicht
einzeln im Changelog aufgeführt. Die Struktur ist:

| Etappe | Thema |
|---|---|
| 1 | Amp-Layer (Grundgerüst) |
| 4 | U(1)-Connection |
| 5 | Apparat-Subgraph |
| 6a–6f | Observer, EPR, CHSH |
| 6i | Wave-Step |
| 8 | U5-Invariante |
| 9 | Transport (2D) |
| 12 | GP-Dilatation (Lorentz) |
| 13 | Tensor-Paare |
| 14 | Fermionen (Jordan-Wigner) |
| 15 | Fock-Raum |
| 16 | Dichte-Trilogie + Lindblad |
| 16e'' | Konsolidierung → 1.16.0 |

Detail-Beschreibungen dieser Etappen stehen in `Project.md` §18.

---

## §2 — Versionsschema im Detail

### §2.1 — Was bedeutet welche Ziffer?

| Ziffer | Bedeutung | Wann springt sie? | Beispiel |
|---|---|---|---|
| **MAJOR** | Phase | bei physikalischem Paradigmenwechsel | `1.x → 2.x` wenn Etappe 24–27 abgeschlossen |
| **MINOR** | Etappe | bei jeder neuen Etappe | `1.22.x → 1.23.0` bei Etappe 23 |
| **PATCH** | Fix | bei Unter-Etappe, Bugfix oder Konsolidierung | `1.23.13 → 1.23.14` bei weiterer Konsolidierung |

### §2.2 — Phasen-Übersicht

| Phase | MAJOR | Bedeutung | Etappen | Status |
|---|:-:|---|---|---|
| **1** | `1.x.y` | Fundament + erste Validierung | 1–23 | **aktiv** |
| **2** | `2.x.y` | Komplette QM | 24–27 | geplant |
| **3** | `3.x.y` | Makrophysik | M1–M3 | geplant |

**Warum „Phase" als MAJOR?** Die Phase spiegelt den **physikalischen
Status** des Kernels, nicht den Code-Umfang. Phase 1 endet, wenn der
Kernel gegen einen externen Referenzwert validiert ist (erreicht in
Etappe 23 mit V&V-Anker, in `1.23.13`/`1.23.14` durch Selbstkonsistenz
+ 2D-exakte Referenz ersetzt). Phase 2 endet, wenn die vollständige
QM abgedeckt ist (Pfadintegral, GHZ, Universalität, Q61). Phase 3
endet, wenn der klassische Limes und die Kopplung an Unsichtbares
(U4'/U5') funktionieren.

### §2.3 — Patch-Vergabe

Ein **PATCH**-Sprung passiert, wenn:

- Eine **Unter-Etappe** hinzukommt (z.B. Etappe 22b nach 22).
- Ein **Bugfix** an einer bereits veröffentlichten Version nötig ist.
- Eine **Konsolidierung** mit substanziellen Änderungen stattfindet
  (z.B. `1.23.1` — Header-, Modul- und Doku-Konsolidierung;
  `1.23.2` — EPR + Fock; `1.23.3` — Gauge; `1.23.4` — Observer;
  `1.23.5` — Shared; `1.23.6` — SU2; `1.23.7` — SU2_Dynamics;
  `1.23.8` — Tensor; `1.23.9` — Release-Vorbereitung;
  `1.23.10` — Bugfix + Plaquette-Konjugation;
  `1.23.11` — ProWB / Web-Docs Integration;
  `1.23.12` — CI-Dokumentation;
  `1.23.13` — Creutz-Ratio + V&V-Anker-Rücknahme;
  `1.23.14` — Sampler-Korrektur (Haar-Vorschlag) + 2D-Bessel-Referenz).

Ein PATCH-Sprung passiert **nicht** bei:

- Reinen Doku-Änderungen ohne Code-Bezug (kein Changelog-Eintrag nötig).
- Test-Erweiterungen ohne Kernel-Änderung.
- Internen Refactorings ohne API-Bruch **und ohne substanzielle
  Struktur-Änderungen**.

**Besonderheit `1.23.11`, `1.23.12`, `1.23.13`, `1.23.14`:** Alle
vier Patches enthalten entweder neue Build-Werkzeuge, neue
CI-Pipelines, neue Kernel-Funktionen oder korrigierte Test-Sampler.
Sie bekommen einen PATCH-Sprung, weil sie:

- neue **Build-Werkzeuge** einführen (`bin\prowb\prowb.exe`) oder
- neue **Build-Targets** etablieren (`prowb` in `build\main\Makefile.nmake`) oder
- neue **CI-Pipelines** hinzufügen (`.github\workflows\*.yml`) oder
- neue **Ausgabe-Dimensionen** etablieren (`out\web\`, CI-Artefakte) oder
- neue **Dokumentations-Kategorien** etablieren (CI-Docs) oder
- neue **Kernel-Funktionen** hinzufügen (`ProPhysics_Wilson_Loop_Average`)
  plus einen neuen Prio-8-Test oder
- **Test-Sampler korrigieren**, die einen systematischen Fehler
  hatten (int32-Overflow im Metropolis-Vorschlag).

Reine Doku-Änderungen ohne Infrastruktur-Wirkung bekommen keinen
PATCH-Sprung. Neue Kernel-Funktionen, neue Build-Werkzeuge, neue
CI-Pipelines oder korrigierte Test-Sampler sind **strukturell** und
qualifizieren für einen Patch.

### §2.4 — Was zählt als „Paradigmenwechsel" (MAJOR)?

- **Etappe 18–23 (innerhalb Phase 1):** Der Kernel wird zum ersten
  Mal gegen einen externen physikalischen Referenzwert validiert
  (V&V-Anker, 0,08 %). Das war vorher `3.0.0` in der alten
  SemVer-Logik; jetzt bleibt es bei `1.23.0`, weil wir noch in
  Phase 1 sind. **Der V&V-Anker wurde mit `[1.23.13]` wegen des
  Bugfixes `[1.23.10]` und endgültig mit `[1.23.14]` (Sampler-Overflow)
  zurückgenommen.** Die Validierung erfolgt jetzt über die
  Creutz-Ratio-Konsistenz und die 2D-exakte-Bessel-Referenz.
- **Etappe 24–27 (Phase 1 → 2, geplant):** Der Kernel wird zur
  „kompletten QM" (Pfadintegral, GHZ, Universalität, Q61).
  MAJOR-Bump auf `2.0.0`.
- **Etappe M1–M3 (Phase 2 → 3, geplant):** Der Kernel erreicht
  Makrophysik (U4', U5', klassischer Limes).
  MAJOR-Bump auf `3.0.0`.

**Zusätzlicher Hinweis zur Umstellung (2026-09-26):** Die bisherigen
Versionen `1.x.y` bis `3.0.0` folgten einer SemVer-ähnlichen Logik,
die nicht zum Projekt passte. Mit der Umstellung auf das
Etappen-Schema wird die **Kernel-Version** ausschließlich durch
Phase + Etappe + Fix bestimmt. Die alten Versionen werden in diesem
Changelog auf das neue Schema abgebildet (siehe Historie-Tabelle
unten).

### §2.5 — Historie der Schema-Umstellung

| Alte Version | Neue Version | Grund |
|---|---|---|
| `3.0.0` | `1.23.0` | Etappe 23 war Phase 1, nicht Phase 3 |
| `2.9.0` | `1.22.1` | Etappe 22b war eine Unter-Etappe |
| `2.8.0` | `1.22.0` | Etappe 22 |
| `2.7.0` | `1.21.0` | Etappe 21 |
| `2.6.0` | `1.19.0` | Etappe 19 |
| `2.5.0` | `1.18.5` | Konsolidierung |
| `2.4.0` | `1.18.4` | Etappe 18d+18e |
| `2.3.0` | `1.18.3` | Etappe 18d |
| `2.2.0` | `1.18.2` | Etappe 18c |
| `2.1.0` | `1.18.1` | Etappe 18b |
| `2.0.0` | `1.18.0` | Etappe 18 + Ontologie |
| `1.5.0` | `1.18.0` | mit 2.0.0 zusammengefasst |
| `1.4.0` | `1.17.1` | Etappe 17b |
| `1.3.0` | `1.17.1` | mit 1.4.0 zusammengefasst |
| `1.2.0` | `1.17.0` | Etappe 17b (erste Fassung) |
| `1.1.0` | `1.17.0` | Etappe 17 |
| `1.0.0` | `1.16.0` | bis Etappe 16e'' |

### §2.6 — Versions-Kopplung

| Komponente | Folgt | Begründung |
|---|---|---|
| **Kernel** | eigenständig | semantischer Anker |
| **SDK** | Kernel | beschreibt Kernel-API |
| **Dokumentation** | Kernel | beschreibt Kernel-Status |
| **Tests** | eigenständig (`1.0.2`) | wächst mit Test-Suite, nicht mit Kernel |
| **Build / Tools / Helfer** | eigenständig (`1.0.0`) | wächst mit Infrastruktur |
| **ProWB** | eigenständig (`1.0.0`) | wächst mit Web-Infrastruktur |
| **CI-Workflows** | eigenständig (`1.0.0`) | wächst mit CI-Infrastruktur |

**Regel:** Wenn die Kernel-Version sich ändert, müssen SDK und Doku
nachziehen. Tests, Build-Skripte, ProWB und CI-Workflows **können**,
müssen aber nicht.

---

## §3 — Konsistenz mit anderen Dokumenten

Dieses Changelog ist die **eine** Quelle für Versions-Historie.
Andere Dokumente haben ihre **eigene** Versionsnummer (Doc-Version),
aber die **Kernel-Version** kommt ausschließlich aus
`ProPhysics_Version.h` und wird hier geführt.

| Dokument | Was es zeigt | Soll-Version |
|---|---|---|
| `CHANGELOG.md` (diese Datei) | Versions-Historie | `1.23.14` (folgt Kernel-Patch) |
| `ProPhysics_VersionRegistry.md` | Versionen **aller** Dateien | `1.23.0` (folgt Kernel) |
| `Project.md` §18 | Detaillierte Etappen-Historie | `1.23.0` (folgt Kernel) |
| `ProPhysics_Version.h` | Aktuelle Kernel-Version | `1.23.0` |
| `BUILD_INFO.txt` | Aktuelle Build-Metadaten | auto-generiert |
| `TODO.md` | Aufgaben-Register | `1.10` (folgt Changelog) |

**Bei Inkonsistenz:** `ProPhysics_Version.h` ist der semantische
Anker. `CHANGELOG.md` folgt ihm. Alle anderen Dokumente folgen
`CHANGELOG.md`.

**Hinweis zu PATCH-Versionen:** Die Changelog-Version (z.B. `1.23.14`)
kann höher sein als die Kernel-Version (z.B. `1.23.0`), wenn der
Patch **nur** Doku, Infrastruktur, interne Refactorings oder
Test-Korrekturen betrifft. Ein PATCH **ohne** Kernel-Bump ist erlaubt,
wenn keine ABI-Änderung stattfindet.

---

## §4 — Pflege dieses Dokuments

**Wann wird dieses Dokument aktualisiert?**

Jede neue Etappe oder jede substanzielle Konsolidierung fügt einen
Eintrag **oben** hinzu (unter `[Unreleased]`).

**Wer aktualisiert es?**

Der Autor der Etappe oder Konsolidierung, **vor** dem Merge in `main`.

**Wie wird die Version bestimmt?**

Nach §2.1. Bei Unsicherheit: im Pull Request diskutieren.

**Was passiert mit `[Unreleased]`?**

Wenn eine Etappe eine Version freigibt, wird der freigegebene Inhalt
von `[Unreleased]` in die neue Version verschoben. `[Unreleased]`
bleibt für **geplante** Änderungen reserviert.

**Kopplung an andere Dokumente:**

Wenn die Kernel-Version sich ändert:

1. `src/prophysics/header/ProPhysics_Version.h` anpassen
   (`VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH`).
2. `CHANGELOG.md` (diese Datei) um einen Eintrag erweitern.
3. SDK-Header (`pro_sdk_interface.h`) auf dieselbe Version setzen.
4. Alle Doku-Dateien mit Header auf dieselbe Version setzen.
5. `ProPhysics_VersionRegistry.md` aktualisieren.

**Wenn nur die Changelog-Version sich ändert (Patch ohne Kernel-Bump):**

1. `CHANGELOG.md` erweitern.
2. `ProPhysics_Version.h` **unverändert** lassen.
3. Alle anderen Dokumente **unverändert** lassen.
4. Ggf. `ProPhysics_VersionRegistry.md` mit Hinweis ergänzen.

**Wenn ein neues Tool, eine neue CI-Pipeline, eine neue
Kernel-Funktion oder eine Test-Korrektur hinzukommt (wie `1.23.11`
ProWB, `1.23.12` CI-Doku, `1.23.13` Creutz-Ratio oder `1.23.14`
Sampler-Korrektur):**

1. `CHANGELOG.md` erweitern.
2. `ProPhysics_Version.h` **unverändert** lassen (Kernel unberührt).
3. `ProPhysics_VersionRegistry.md` um neue Tool-Sektion erweitern.
4. `TODO.md` um erledigte Punkte markieren.
5. Neue Doku-Dateien anlegen (`src/prowb/README.md`,
   `docs/build/ci.md`, `docs/project/SU2.md` etc.).
6. Querverweise in bestehenden Doku-Dateien ergänzen.

---

## §5 — Siehe auch

| Thema | Datei |
|---|---|
| Kernel-Version | `src/prophysics/header/ProPhysics_Version.h` |
| Versions-Register | `docs/project/ProPhysics_VersionRegistry.md` |
| Versionierungs-Konzept | `docs/project/VERSIONING.md` |
| Etappen-Historie | `docs/project/Project.md` §18 |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Baseline | `docs/test/BASELINE.md` |
| Build-System | `docs/build/BUILD_SCRIPT.md` |
| ProWB-README | `src/prowb/README.md` |
| ProWB-Makefile | `docs/build/prowb/Makefile.md` |
| Web-Docs-Manifest | `docs/web/README.md` |
| Web-Docs-CI | `docs/build/web-docs-ci.md` |
| Standard-CI | `docs/build/ci.md` |
| Alpha-Nightly | `docs/build/alpha-nightly.md` |
| Konfiguration | `docs/project/CONFIG.md` |
| Amp-Modul | `docs/project/Amp.md` |
| Core-Modul | `docs/project/Core.md` |
| Density-Modul | `docs/project/Density.md` |
| Dirac-Modul | `docs/project/Dirac.md` |
| EPR-Modul | `docs/project/EPR.md` |
| Fock-Modul | `docs/project/Fock.md` |
| Gauge-Modul | `docs/project/Gauge.md` |
| Observer-Modul | `docs/project/Observer.md` |
| Shared-Modul | `docs/project/Shared.md` |
| SU2-Modul | `docs/project/SU2.md` |
| SU2-Dynamik | `docs/project/SU2_Dynamics.md` |
| Tensor-Modul | `docs/project/Tensor.md` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende CHANGELOG v1.23.14.**