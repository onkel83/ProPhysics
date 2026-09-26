# ProPhysics — Tests schreiben

**Datei:** `docs/test/WRITING_TESTS.md`
**Version:** 1.0
**Stand:** 2026-09-25 (Kernel-Version 3.0.0, Etappe 23)
**Zweck:** Verbindliche Anleitung zum Schreiben neuer Tests. Ergänzt
`CONTRIBUTING.md` §4 (Überblick) um die vollständige Praxis.

---

## §0 — Wozu dieses Dokument

`CONTRIBUTING.md` §4 erklärt in sechs Schritten, **wo** ein neuer Test
registriert wird. Dieses Dokument erklärt, **wie** ein Test geschrieben
wird — Konventionen, Patterns, Fehlerquellen, Beispiele.

**Wer dieses Dokument liest, sollte vorher gelesen haben:**

- `docs/project/Project.md` §2 (Regeln R1–R7)
- `docs/project/ARCHITECTURE.md` (Tick-Ablauf, Datenmodell)
- `docs/test/run_alpha_tests.md` (wie der Runner Tests auswertet)
- `docs/test/ProPhysics_Testkatalog.md` (bestehende Tests)

---

## §1 — Test-Philosophie

### §1.1 — Was ein Test in ProPhysics **ist**

Ein Test ist ein **Experiment**. Er hat:

1. **Eine Frage** — „Zeigt der Kernel Phänomen X?"
2. **Ein Kriterium** — „Wenn ja, dann muss Größe Y < Z sein."
3. **Ein Messverfahren** — „Ich messe Y, indem ich ..."
4. **Ein Ergebnis** — PASS oder FAIL.

**Kernaussage:** Ein Test ohne Kriterium ist kein Test, sondern eine
Demo. Ein Test ohne Frage ist Beschäftigungstherapie.

### §1.2 — Was ein Test **nicht** ist

- **Kein Benchmark.** Performance wird nicht getestet.
- **Kein Regressionstest im klassischen Sinne.** Der Vergleich
  zwischen Läufen läuft über Logs, nicht über automatische
  Differenz.
- **Keine Demo.** Wer nur zeigen will, dass etwas läuft, schreibt
  keine `alpha_test_*`-Datei.
- **Kein Konstrukt.** Der Test darf den Kernel **nicht** mit
  künstlichen Strukturen füttern, um ein Ergebnis zu erzwingen.

### §1.3 — Wann ein Test akzeptiert wird

| Kriterium | Erfüllt? |
|---|---|
| Neue emergente Struktur **oder** neue Messung **oder** Bug-Reproduktion | muss |
| Klares numerisches Kriterium | muss |
| Deterministisch (Seed oder deterministische Konstruktion) | muss |
| Läuft unter der Prio-Architektur | muss |
| Alle bestehenden Tests bleiben grün | muss |
| Dokumentiert im Testkatalog | muss |

---

## §2 — Test-Typen

ProPhysics-Tests werden in **Prios** (Prioritätsstufen) organisiert.
Jede Prio hat ein Thema.

| Prio | Thema | Typische Kriterien | Dauer |
|:-:|---|---|---|
| 1 | 2D-Basis | exakte Werte, χ²-Tests | Sekunden |
| 2 | Emergenz | χ²-Tests, Verteilungen | Minuten |
| 3 | Langlauf | Drift < Toleranz über N Ticks | Minuten |
| 4 | 3D-Torus | exakte Werte, Stabilität | Minuten |
| 5 | Hydrogen + Shared | Lokalisierung, Klassen | 30–40 min |
| 6 | Spin-1/2 | Algebra, exakte Werte | Sekunden |
| 7 | Dirac | Clifford-Algebra | Sekunden |
| 8 | SU(2) + Running-Coupling | Eichinvarianz, Metropolis | Sekunden bis 24 min |

**Prio-Zuordnung:** Ein neuer Test gehört zu der Prio, deren Thema er
erweitert. Ein neuer Test ohne passende Prio wird als **neue Prio**
eingeführt (z.B. Prio 9 für Pfadintegral in Etappe 24).

---

## §3 — Anatomie eines Tests

Ein ProPhysics-Test ist eine **Funktion**, die:

1. Ein `ProUniverse` initialisiert.
2. Eine Topologie aufbaut.
3. Eine Anfangskonfiguration setzt.
4. Die Dynamik laufen lässt.
5. Eine Observable misst.
6. Gegen ein Kriterium prüft.
7. PASS oder FAIL zurückgibt.

### §3.1 — Grundgerüst

```c
/* ==========================================================================
 * ProPhysics - Alpha-Test: <Thema> (Etappe NN)
 * File: alpha_test_<thema>.c
 * Architecture: <kurze Beschreibung>
 * Version: <Version> (Etappe NN)
 *
 * <Was wird getestet, warum>
 * ========================================================================== */

#include "alpha_test_common.h"

int alpha_test_<thema>_run(void)
{
    /* --- 1. Setup --- */
    ProUniverse pu;
    ProPhysics_Initialize(&pu, N_NODES);
    pu.grid_dim = DIM;
    pu.grid_ndim = 2;

    /* --- 2. Topologie --- */
    /* ... Link_Nodes aufrufen ... */

    /* --- 3. Anfangszustand --- */
    /* ... Set_Node_Amplitude / Inject_Momentum ... */

    /* --- 4. Dynamik --- */
    for (int t = 0; t < N_TICKS; ++t) {
        ProPhysics_Tick(&pu, NULL);
    }

    /* --- 5. Messung --- */
    double result = /* ... */;
    double expected = /* ... */;
    double rel_dev = fabs(result - expected) / fabs(expected);

    /* --- 6. Ausgabe --- */
    printf("[XXX] Ergebnis:  %.9e\n", result);
    printf("[XXX] Erwartet:  %.9e\n", expected);
    printf("[XXX] rel_dev = %.3e (Schwelle %.1e)\n", rel_dev, TOL);

    /* --- 7. Bewertung --- */
    ProPhysics_Free(&pu);

    if (rel_dev < TOL) {
        printf("[XXX] -> PASSED (<Kurzbeschreibung>)\n");
        return 1;
    }
    printf("[XXX] -> FAILED\n");
    return 0;
}
```

### §3.2 — Rückgabewert

| Rückgabe | Bedeutung |
|:-:|---|
| `1` | PASS |
| `0` | FAIL |

**Nicht** `bool` — die Konvention ist `int` mit `1`/`0`, weil der
Runner über den Marker in der Ausgabe (nicht über den Rückgabewert)
entscheidet. Der Rückgabewert wird nur vom `main`-Wrapper
(`alpha_test_main.c`) in den Exit-Code übersetzt.

### §3.3 — Ausgabe-Präfix

Jeder Test verwendet ein **kurzes, eindeutiges Präfix** in eckigen
Klammern:

| Test | Präfix |
|---|---|
| `test_amp` | `[AMP]` |
| `test_born` | `[BORN]` |
| `test_chsh` | `[CHSH]` |
| `test_su2_wilson_loop` | `[SU2]` |
| `test_running_coupling` | `[RC]` |
| `test_hydrogen` | `[H2]` |

**Konvention:** 2–4 Großbuchstaben, thematisch. Nicht mit anderen
Tests kollidieren.

### §3.4 — PASS/FAIL-Zeile

**Letzte Zeile:**

```
[XXX] -> PASSED (<kurze Begründung>)
```

oder

```
[XXX] -> FAILED
```

**Zwingend:** Der Runner sucht genau nach dieser Zeile
(siehe `run_alpha_tests.md` §7.5). Andere Formate werden nicht
erkannt.

**Beispiele:**

```
[SU2] -> PASSED (SU(2)-Eichfeld implementiert)
[RC] -> PASSED (Rohdaten, weitere Analyse extern)
[BORN] -> PASSED
```

---

## §4 — Ausgabe-Konventionen

Der Runner erkennt PASS/FAIL über **Regex-Muster**. Wer die Konvention
nicht einhält, bekommt `UNKNOWN` als Ergebnis.

### §4.1 — Die erkannten Muster (Priorität von oben nach unten)

| # | Muster | Verwender |
|:-:|---|---|
| 1 | `Ergebnis: N PASS, M FAIL` | Sammeltests mit Nummer |
| 2 | `Ergebnis: HELD` / `Ergebnis: BROKEN` | No-Signaling |
| 3 | `[PASS]` / `[FAIL]` gezählt | Einzel-Check-Marker |
| 4 | `Ergebnis: PASSED` / `Ergebnis: FAILED` | Einzelne Tests |
| 5 | `-> PASSED` / `-> FAILED` | **Standard** |
| 6 | Sondermarker | CHSH-Varianten, Superdet |

### §4.2 — Wenn dein Test keinen Marker liefert

Der Runner fällt zurück auf:

- `ExitCode != 0` → FAIL
- `ExitCode == 0` → UNKNOWN (wird als Fehler gewertet)

**Konsequenz:** Der Test muss **explizit** einen der Marker ausgeben.
Sonst wird er als `UNKNOWN` gewertet, auch wenn er „eigentlich"
grün ist.

### §4.3 — Standard: `-> PASSED` / `-> FAILED`

Das ist das **empfohlene** Muster für neue Tests.

```c
printf("[XXX] -> PASSED\n");
printf("[XXX] -> FAILED\n");
```

### §4.4 — Mehrere Marker in einem Test

Manche Tests haben **intern** mehrere Untertests (z.B.
`SU2-Wilson-Loop` mit T1–T18). Diese Tests sammeln intern:

```c
int fail_count = 0;
/* ... */
if (kriterium_1) printf("[SU2] T1 PASS\n");
else { printf("[SU2] T1 FAIL\n"); fail_count++; }
/* ... */
if (fail_count == 0) {
    printf("[SU2] -> PASSED\n");
    return 1;
}
printf("[SU2] -> FAILED\n");
return 0;
```

Der Runner erkennt nur den **letzten** Marker (`-> PASSED`).

### §4.5 — Zahlen-Formatierung

| Typ | Format | Beispiel |
|---|---|---|
| `double` (Standard) | `%.6e` oder `%.6f` | `1.234567e-08` |
| `double` (hohe Präzision) | `%.9e` | `1.234567890e-08` |
| `int` | `%d` | `18` |
| `uint64_t` | `%" PRIu64` | `262144` |
| Verhältnis | `%.3f` | `0.987` |

**Kein `%g`.** Das wechselt zwischen fester und exponentieller
Darstellung und ist schwer lesbar.

---

## §5 — Determinismus und RNG

### §5.1 — Zwei Kategorien von Tests

| Kategorie | RNG? | Beispiel |
|---|---|---|
| **Deterministisch** | kein RNG | `test_unitary`, `test_amp` |
| **Stochastisch** | mit festem Seed | `test_chsh`, `test_running_coupling` |

**Deterministische Tests sind bevorzugt.** Sie sind reproduzierbar,
schneller und einfacher zu debuggen.

### §5.2 — Wann RNG nötig ist

Nur wenn:

- Eine **Verteilung** gemessen wird (Born, Boltzmann, CHSH-Statistik).
- **Viele Trials** nötig sind, um Statistik zu erhalten.

Auch dann: **Seed immer fix.**

### §5.3 — RNG-State

Der Kernel verwendet `xoshiro256**`. Der State ist ein `uint64_t[4]`:

```c
uint64_t rng[4] = { 0x1234567890ABCDEFULL, 0xDEADBEEFCAFEBABEULL,
                    0x0F0F0F0F0F0F0F0FULL, 0xAAAAAAAA55555555ULL };
```

**Konvention für Tests:** Der Seed ist **eine Konstante**, nicht
`time(NULL)`.

```c
/* Gut: fixer Seed */
uint64_t rng[4] = { 1, 2, 3, 4 };

/* Schlecht: nicht reproduzierbar */
uint64_t rng[4] = { time(NULL), 0, 0, 0 };
```

### §5.4 — Zufallszahlen ziehen

```c
double u = pro_uniform01(rng);   /* Gleichverteilung [0, 1) */
```

`pro_uniform01` ist in `ProPhysics_Internal.h` definiert.

### §5.5 — Warum fester Seed wichtig ist

Bei einem Bug ist die **Reihenfolge** der RNG-Aufrufe entscheidend.
Mit festem Seed ist der Fehler reproduzierbar.

**Regel:** Wenn ein Test mit fixem Seed FAIL liefert, liefert er bei
jedem Lauf FAIL — bis der Bug gefixt ist.

---

## §6 — Kriterien

### §6.1 — Kriterien-Typen

| Typ | Beispiel | Anwendung |
|---|---|---|
| **Absolut** | `rel_dev < 1e-6` | wenn Sollwert bekannt |
| **Relativ** | `drift / norm < 1e-5` | wenn Sollwert 0 |
| **Chi²** | `chi² < threshold(dof, p)` | Verteilungen |
| **Invarianz** | `before == after` | Erhaltungssätze |
| **Monotonie** | `u_plaq(β=1) > u_plaq(β=2)` | Physik |
| **Symmetrie** | `A_perm == B_perm` | algebraisch |

### §6.2 — Toleranz-Wahl

**Empfehlung:** Toleranz **so eng wie möglich, so weit wie nötig.**

| Situation | Toleranz |
|---|---|
| Exakt (Integer-Arithmetik) | `==` |
| Q31-Rundung über 1 Tick | `1e-9` |
| Q31-Rundung über 100 Ticks | `1e-7` |
| Q31-Rundung über 10⁴ Ticks | `1e-6` |
| Floating-Point in Wave-Step | `1e-5` |
| Statistik (10³ Trials) | `1e-2` |
| Statistik (10⁴ Trials) | `1e-3` |
| V&V gegen externe Physik | `1e-3` (0,1 %) |

**Nicht akzeptabel:**

```c
/* Zu locker: jeder Test wäre grün */
if (rel_dev < 1.0) { /* PASS */ }

/* Zu eng: Test wird flaky */
if (rel_dev < 1e-15) { /* PASS */ }
```

### §6.3 — Beispiele für gute Kriterien

**Aus `test_unitary`:**

```c
if (drift == 0.0) {
    printf("[UNIT] -> PASSED (exakt unitär)\n");
    return 1;
}
```

**Aus `test_born`:**

```c
/* chi² mit dof=1, p=0.01 → threshold=6.635 */
if (chi2 < 6.635) { /* PASS */ }
```

**Aus `test_running_coupling` (V&V-Anker):**

```c
const double ref = 0.43313;
const double dev = fabs(P_measured - ref) / ref;
if (dev < 1e-3) { /* PASS */ }   /* 0,1 % */
```

**Aus `test_su2_wilson_loop` (Invarianz):**

```c
if (fabs(tr_after - tr_before) < 1e-9) { /* PASS */ }
```

---

## §7 — Zeit und Timeout

### §7.1 — Wie lange darf ein Test dauern?

| Prio | Typische Dauer |
|:-:|---|
| 1 | < 10 s |
| 2 | 10 s – 5 min |
| 3 | 10 s – 1 min |
| 4 | 10 s – 1 min |
| 5 | 30 s – 40 min |
| 6 | < 1 s |
| 7 | < 20 s |
| 8 | 2 s – 25 min |

**Faustregel:** < 5 s bevorzugt. Nur wenn die Physik es erfordert,
darf ein Test länger laufen.

### §7.2 — Timeout-Konvention

Der Timeout im Runner ist **3–4× größer** als die erwartete Dauer:

| Test-Dauer | Timeout |
|---:|---:|
| < 1 s | 60 s |
| 1–10 s | 120 s |
| 10–60 s | 300 s |
| 1–10 min | 900 s |
| 10–30 min | 2400 s |
| 30–60 min | 7200 s |

**Grund:** Ein langsamerer Rechner (Faktor 1,5–2) soll durchlaufen.

**Nicht:** `timeout == erwartete Dauer`. Der Test würde bei kleinsten
Schwankungen failen.

### §7.3 — Timeout-Verhalten

Bei Timeout:

- Prozess wird via `Kill()` beendet.
- Log enthält bis dahin gesammelte Ausgabe.
- Ergebnis ist `TIMEOUT`.
- Timeout wird als Fehler gewertet (Exit-Code 1).

**Konsequenz:** Ein Test, der oft in den Timeout läuft, wurde zu eng
konfiguriert.

---

## §8 — Test registrieren

Die sechs Schritte aus `CONTRIBUTING.md` §4, hier mit Details.

### §8.1 — Schritt 1: Datei anlegen

Datei: `src/test/alpha_test_<thema>.c`.

**Kopf-Kommentar:**

```c
/* ==========================================================================
 * ProPhysics - Alpha-Test: <Thema> (Etappe NN)
 * File: alpha_test_<thema>.c
 * Architecture: <kurze Beschreibung>
 * Version: <Version> (Etappe NN)
 *
 * <Was wird getestet>
 * <Warum ist das relevant>
 * ========================================================================== */
```

### §8.2 — Schritt 2: In `alpha_test_main.c` registrieren

Öffne `src/test/alpha_test_main.c`. Zwei Stellen:

**Am Dateianfang, Deklaration:**

```c
extern int alpha_test_<thema>_run(void);
```

**Im Dispatcher (`main`), neuer Zweig:**

```c
else if (strcmp(argv[1], "--test-<thema>") == 0) {
    return alpha_test_<thema>_run() ? 0 : 1;
}
```

**Wichtig:** Die Reihenfolge der Zweige ist nicht kritisch (CLI-Vergleich
ist exakt), aber eine alphabetische Sortierung ist üblich.

### §8.3 — Schritt 3: In `build/test/Makefile.nmake`

Öffne `build/test/Makefile.nmake`. Füge die Datei in die
`ALPHA_SOURCES`-Liste ein:

```
ALPHA_SOURCES = \
    $(SRC_DIR)\alpha_test_main.c \
    $(SRC_DIR)\alpha_test_common.c \
    $(SRC_DIR)\alpha_test_basic.c \
    ...
    $(SRC_DIR)\alpha_test_<thema>.c
```

**Reihenfolge:** Alphabetisch oder logisch gruppiert. `alpha_test_main.c`
steht immer am Anfang.

### §8.4 — Schritt 4: In `tools/run_alpha_tests.ps1`

Öffne `tools/run_alpha_tests.ps1`. Füge den Test in `$TestCatalog` ein.

**Für einen bestehenden Prio:**

```powershell
'1' = @(
    # ... bestehende Einträge ...
    @{ Name = 'My-New-Physics'
       Exe = 'example_alpha_test.exe'
       Args = '--test-my-new-physics'
       Timeout = 60 }
)
```

**Für eine neue Prio:**

```powershell
'9' = @(
    @{ Name = 'My-New-Physics'
       Exe = 'example_alpha_test.exe'
       Args = '--test-my-new-physics'
       Timeout = 300 }
)
```

**Timeout-Wahl:** siehe §7.2.

**Bei neuer Prio:** Auch im Prio-Wähler (`-Prio`) ergänzen, falls das
Skript das über `ValidateSet` erzwingt.

### §8.5 — Schritt 5: In `ProPhysics_Testkatalog.md`

Füge einen neuen Abschnitt hinzu:

```markdown
### T<N>.<M> — <Testname>

**Ziel:** Was wird geprüft.

**Methode:** Wo und was wird gemessen.

**Ergebnis:** Rohwerte des letzten vollständigen Laufs.

**Beweis:** Was damit gezeigt ist.

**Emergenz:** Ja / Nein / Teilweise.
```

Und in §0 (Regression-Status):

| Prio | Thema | Tests | Status |
|---|---|---|---|
| 9 | Pfadintegral | 1 | **1/1** |

Auch die **Laufzeit-Tabelle** und die **Gesamtzahl** aktualisieren.

### §8.6 — Schritt 6: In `run_alpha_tests.md`

- §5 (Prios im Überblick): Zeile für neue Prio.
- §6 (Test-Katalog): Eintrag für neuen Test.
- Gesamt-Test-Anzahl aktualisieren.
- Falls relevant: CI-Empfehlungen anpassen.

### §8.7 — Checkliste

- [ ] Neue Datei `src/test/alpha_test_<thema>.c` erstellt
- [ ] `extern`-Deklaration in `alpha_test_main.c`
- [ ] CLI-Zweig in `alpha_test_main.c`
- [ ] Eintrag in `build/test/Makefile.nmake` `ALPHA_SOURCES`
- [ ] Eintrag in `tools/run_alpha_tests.ps1` `$TestCatalog`
- [ ] Abschnitt in `ProPhysics_Testkatalog.md`
- [ ] Eintrag in `run_alpha_tests.md`
- [ ] `build.cmd -Mode all -Rebuild` läuft
- [ ] `run_alpha_tests.cmd -Prio <N>` zeigt neuen Test
- [ ] Neuer Test PASS
- [ ] Alle bestehenden Tests weiter PASS

---

## §9 — Häufige Fehler

### §9.1 — Fehler im Test-Code

| Fehler | Symptom | Fix |
|---|---|---|
| `grid_dim` keine Zweierpotenz | Test hängt oder falsches Ergebnis | `dim ∈ {16, 32, 64, 128}` |
| `total_nodes != dim × dim` | Out-of-Range, Crash | `Initialize(&pu, dim*dim)` |
| `grid_ndim` nicht gesetzt | 2D-Pfad statt 3D | `pu.grid_ndim = 3` |
| Nachbarschaft nicht aufgebaut | Self-Loops, keine Dynamik | `ProPhysics_Link_Nodes` aufrufen |
| Kein `ProPhysics_Free` | Memory-Leak im Runner | `Free` vor `return` |
| Kein PASS/FAIL-Marker | UNKNOWN-Ergebnis | `printf("[XXX] -> PASSED\n")` |
| Umlaute in Ausgabe | Log kaputt | UTF-8 ohne Umlaute, oder `-` statt `ü` |
| Zeitstempel im Log | Nicht reproduzierbar | Nicht machen |
| `time(NULL)` als Seed | Nicht reproduzierbar | Fixer Seed |
| `printf` mit `%g` | Schwer lesbar | `%.6e` verwenden |

### §9.2 — Fehler in der Registrierung

| Fehler | Symptom | Fix |
|---|---|---|
| Datei nicht in `ALPHA_SOURCES` | Linker-Fehler (`unresolved external`) | Makefile-Eintrag |
| CLI-Zweig fehlt | Runner: `MISSING` | `alpha_test_main.c` prüfen |
| Timeout zu klein | `TIMEOUT` | Timeout verdoppeln |
| Runner-Eintrag fehlt | Test läuft nie | `$TestCatalog` prüfen |
| Testkatalog-Eintrag fehlt | Doku inkonsistent | Doku nachziehen |

### §9.3 — Fehler im Kriterium

| Fehler | Symptom |
|---|---|
| Kriterium tautologisch | Test läuft immer grün, aber nichts geprüft |
| Kriterium zu eng | Test ist flaky (grün/schwarz) |
| Kriterium zu locker | Test läuft immer grün, aber jeder Wert akzeptiert |
| Kriterium unklar | Wer den Test liest, versteht nicht, was geprüft wird |

**Tautologie-Beispiel (schlecht):**

```c
double x = compute_x();
if (x == x) { /* PASS */ }   /* immer wahr */
```

**Zu eng (schlecht):**

```c
if (fabs(result - expected) < 1e-16) { /* PASS */ }
/* Q31 hat nur ~2^-31 Genauigkeit → immer FAIL */
```

---

## §10 — Beispiele

### §10.1 — Deterministischer Test (analytisches Kriterium)

**Test:** U5-Invariante über 1000 Ticks.

```c
int alpha_test_u5_invariance_run(void)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 32 * 32);
    pu.grid_dim = 32;
    pu.grid_ndim = 2;

    /* 2D-Torus */
    for (uint32_t y = 0; y < 32; ++y) {
        for (uint32_t x = 0; x < 32; ++x) {
            const uint64_t k = y * 32 + x;
            ProPhysics_Link_Nodes(&pu, k, y * 32 + ((x + 1) % 32),
                PRO_NEIGHBOR_X_PLUS);
            ProPhysics_Link_Nodes(&pu, k, y * 32 + ((x + 31) % 32),
                PRO_NEIGHBOR_X_MINUS);
            ProPhysics_Link_Nodes(&pu, k, ((y + 1) % 32) * 32 + x,
                PRO_NEIGHBOR_Y_PLUS);
            ProPhysics_Link_Nodes(&pu, k, ((y + 31) % 32) * 32 + x,
                PRO_NEIGHBOR_Y_MINUS);
        }
    }

    /* Uniforme Anfangskonfiguration */
    for (uint64_t k = 0; k < pu.total_nodes; ++k) {
        ProPhysics_Set_Node_Amplitude(&pu, k, UR_POSITRON_CW, INT32_MAX, 0);
    }

    const ProU128 n0 = ProPhysics_Measure_Amp_Invariant(&pu);

    for (int t = 0; t < 1000; ++t) {
        ProPhysics_Tick(&pu, NULL);
    }

    const ProU128 n1 = ProPhysics_Measure_Amp_Invariant(&pu);

    const double drift =
        fabs(pro_u128_to_double(n1) - pro_u128_to_double(n0))
        / pro_u128_to_double(n0);

    printf("[U5] drift = %.3e (Schwelle 1e-5)\n", drift);

    ProPhysics_Free(&pu);

    if (drift < 1e-5) {
        printf("[U5] -> PASSED (U5-Invariante erhalten)\n");
        return 1;
    }
    printf("[U5] -> FAILED\n");
    return 0;
}
```

### §10.2 — Statistischer Test (mit Seed)

**Test:** Born-Verteilung gegen `0.25:0.75`.

```c
int alpha_test_born_statistics_run(void)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    pu.grid_dim = 8;
    pu.grid_ndim = 2;

    /* Superposition im Knoten 0: alpha=0.5, beta=0.866 */
    ProPhysics_Set_Node_Amplitude(&pu, 0, UR_POSITRON_CW,  INT32_MAX / 2, 0);
    ProPhysics_Set_Node_Amplitude(&pu, 0, UR_NEGATRON_CCW, (int32_t)(INT32_MAX * 0.866), 0);

    /* 10 000 Born-Samples mit fixem Seed */
    uint64_t rng[4] = { 0xCAFEBABE, 0xDEADBEEF, 0x12345678, 0x9ABCDEF0 };
    int count_up = 0;
    const int N = 10000;

    for (int i = 0; i < N; ++i) {
        const double p_up = ProPhysics_Get_Born_Probability(&pu, 0, UR_POSITRON_CW);
        const double u = pro_uniform01(rng);
        if (u < p_up) count_up++;
    }

    const double p_meas = (double)count_up / N;
    const double p_theo = 0.25;
    const double chi2 = (p_meas - p_theo) * (p_meas - p_theo)
                        * N / (p_theo * (1.0 - p_theo));

    printf("[BORN] p_meas = %.4f, p_theo = %.4f\n", p_meas, p_theo);
    printf("[BORN] chi2 = %.4f (Schwelle 6.635, dof=1, p=0.01)\n", chi2);

    ProPhysics_Free(&pu);

    if (chi2 < 6.635) {
        printf("[BORN] -> PASSED (Born-Statistik konform)\n");
        return 1;
    }
    printf("[BORN] -> FAILED\n");
    return 0;
}
```

### §10.3 — Multi-Check-Test (SU(2)-Stil)

**Test:** SU(2)-Algebra mit T1–T18.

```c
int alpha_test_su2_run(void)
{
    int fail_count = 0;
    int test_count = 0;

    /* T1: Quaternion-Unit */
    const double err_t1 = ProPhysics_Verify_SU2_Quaternion();
    test_count++;
    if (err_t1 < 1e-9) {
        printf("[SU2] T1 PASS (err = %.3e)\n", err_t1);
    } else {
        printf("[SU2] T1 FAIL (err = %.3e > 1e-9)\n", err_t1);
        fail_count++;
    }

    /* T2: Wilson-Loop-Eichinvarianz */
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 4 * 4 * 4);
    pu.grid_dim = 4;
    pu.grid_ndim = 3;
    /* ... Topologie + Link ... */

    /* ... 18 Untertests ... */

    printf("[SU2] Ergebnis: %d / %d\n", test_count - fail_count, test_count);

    ProPhysics_Free(&pu);

    if (fail_count == 0) {
        printf("[SU2] -> PASSED (SU(2)-Eichfeld implementiert)\n");
        return 1;
    }
    printf("[SU2] -> FAILED\n");
    return 0;
}
```

### §10.4 — V&V-Test (gegen externe Physik)

**Test:** Wilson-Action gegen `I₂(2)/I₁(2)`.

```c
int alpha_test_running_coupling_run(void)
{
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64 * 64 * 64);
    pu.grid_dim = 64;
    pu.grid_ndim = 3;

    /* Topologie */
    /* ... */

    /* Metropolis-Sampling */
    uint64_t rng[4] = { 1, 2, 3, 4 };
    const double beta = 2.0;
    /* ... thermalisieren, messen ... */

    const double P_measured = 0.433460;   /* aus Metropolis */
    const double u_err      = 0.000027;

    /* V&V-Anker */
    const double P_ref = 0.43313;         /* I₂(2)/I₁(2), Pietarinen 1981 */
    const double dev = fabs(P_measured - P_ref) / P_ref;

    printf("[RC] dim=64, beta=2.0\n");
    printf("[RC] P_meas = %.6f +- %.6f\n", P_measured, u_err);
    printf("[RC] P_ref  = %.6f\n", P_ref);
    printf("[RC] rel_dev = %.4e (Schwelle 1e-3 = 0,1%%)\n", dev);

    ProPhysics_Free(&pu);

    if (dev < 1e-3) {
        printf("[RC] -> PASSED (V&V-Anker erfuellt)\n");
        return 1;
    }
    printf("[RC] -> FAILED\n");
    return 0;
}
```

**Wichtig:** Bei V&V-Tests muss die **Referenz** im Kommentar stehen
(Quelle, Jahr). Wer die Referenz nicht nennen kann, hat keine V&V.

---

## §11 — Was **nicht** akzeptabel ist

### §11.1 — Tautologische Tests

```c
/* Schlecht: prüft nichts */
double x = compute();
if (x == x) { printf("-> PASSED\n"); return 1; }
```

### §11.2 — Konstruierte Tests

```c
/* Schlecht: erzwingt das Ergebnis */
Set_Node_Amplitude(&pu, 0, UR_POSITRON_CW, INT32_MAX, 0);
double p = Get_Born_Probability(&pu, 0, UR_POSITRON_CW);
if (p == 1.0) { /* PASS */ }
/* Das ist trivial — die Amplitude ist ja schon |1| */
```

### §11.3 — Nicht-reproduzierbare Tests

```c
/* Schlecht */
uint64_t seed = time(NULL);
/* Bei jedem Lauf anderes Ergebnis */
```

### §11.4 — Tests mit externen Ressourcen

```c
/* Schlecht */
FILE* f = fopen("test_data.csv", "r");
/* Der Runner hat keine externen Files */
```

### §11.5 — Tests, die bestehende brechen

Ein neuer Test darf **nie** bestehende Tests brechen. Wenn der neue
Test eine Änderung am Kernel erfordert, die einen bestehenden Test
bricht, ist die Änderung falsch (R7).

### §11.6 — Tests mit versteckten Sollwerten

```c
/* Schlecht: Sollwert nicht nachvollziehbar */
const double expected = 0.4334567;   /* woher? */
if (fabs(result - expected) < 1e-6) { /* PASS */ }
```

**Regel:** Jeder Sollwert braucht eine Quelle (analytisch,
publiziert, Referenzimplementierung). Im Kommentar.

---

## §12 — Test-Debugging

### §12.1 — Einzelnen Test direkt laufen lassen

```cmd
cd bin
example_alpha_test.exe --test-<thema>
```

Ausgabe ist direkt sichtbar. Der Runner leitet nur um.

### §12.2 — Log lesen

```cmd
type bin\logs\<timestamp>_<name>.log
```

Die Log enthält **alle** `printf`-Ausgaben des Tests.

### §12.3 — Bei unerwartetem FAIL

**Checkliste:**

1. **Ist der Kernel neu gebaut?**
   `build.cmd -Mode all -Rebuild`
2. **Ist der Test korrekt registriert?**
   `--test-<thema>` muss funktionieren.
3. **Ist das Kriterium zu eng?**
   Vergleich mit bestehenden Tests.
4. **Ist die Anfangskonfiguration korrekt?**
   Log zeigt die Rohwerte.
5. **Ist die Nachbarschaft korrekt?**
   Bei unerwartet kleinen Zahlen: oft falsche Kanäle.
6. **Ist der RNG-Seed korrekt?**
   Bei statistischen Tests.
7. **Ist `Free` aufgerufen?**
   Bei Speicherproblemen.
8. **Wurde der Kernel geändert?**
   `git diff` prüfen.

### §12.4 — Bei „Test läuft nie"

- Check `$TestCatalog` in `run_alpha_tests.ps1`.
- Check `ALPHA_SOURCES` in `Makefile.nmake`.
- Check die Prio-Zahl (`-Prio <N>`).

### §12.5 — Bei „UNKNOWN"

- Check die PASS/FAIL-Zeile. Endet sie mit `-> PASSED\n`?
- Check, ob der Test überhaupt `printf` schreibt.
- Check, ob der Exit-Code korrekt ist.

---

## §13 — Test-Erweiterung vs. Test-Ersatz

### §13.1 — Wann erweitern?

Wenn ein bestehender Test **dasselbe Phänomen** prüft, aber
**unvollständig** ist:

- Neue Untertests (T19, T20, ...) im selben Test.
- Erweiterte Kriterien (engerere Toleranz, zusätzliche Größen).

**Beispiel:** `test_su2_wilson_loop` wurde in Etappe 22b um T15–T18
erweitert (Leapfrog-Dynamik), ohne den Test umzubenennen.

### §13.2 — Wann neu anlegen?

Wenn ein Test ein **neues Phänomen** prüft, das **nicht** in den
bestehenden passt:

- Neuer Name (`test_<neues_thema>`).
- Neue Datei (`alpha_test_<neues_thema>.c`).
- Neue Prio, falls das Thema nicht in eine bestehende passt.

**Beispiel:** `test_running_coupling` (Etappe 23) wurde als **neuer
Test** angelegt, nicht als Erweiterung von `test_su2_wilson_loop`.
Grund: Metropolis-Sampling ist ein anderes Phänomen als Leapfrog.

### §13.3 — Test-Ersatz

Ein Test wird **nie** ersetzt. Wenn ein Kriterium falsch war,
wird er **korrigiert** — mit dokumentierter Begründung im
Testkatalog.

**Grund:** R6 verlangt, dass jede Etappe mit einem Test endet. Der
Verlauf der Tests ist die Geschichte des Projekts.

---

## §14 — Dokumentationspflicht

Jeder Test **muss** im Testkatalog stehen. Folgende Felder sind
Pflicht:

| Feld | Inhalt |
|---|---|
| **Ziel** | Was wird geprüft |
| **Methode** | Wie wird gemessen |
| **Ergebnis** | Rohwerte des letzten Laufs |
| **Beweis** | Was damit gezeigt ist |
| **Emergenz** | Ja / Nein / Teilweise |

**Rohwerte-Pflicht:** Der Testkatalog muss **Zahlen** zeigen, keine
Adjektive. „Der Test war grün" ist kein Ergebnis.

**Quellen-Pflicht:** Bei V&V-Tests: Quelle der Referenz (Paper,
Jahr).

---

## §15 — Zusammenfassung

Ein guter ProPhysics-Test ist:

- **deterministisch** (fester Seed oder kein RNG)
- **klar** (ein Kriterium, nicht zehn)
- **nachvollziehbar** (Sollwert mit Quelle)
- **schnell** (< 5 s bevorzugt)
- **reproduzierbar** (gleiches Ergebnis bei jedem Lauf)
- **dokumentiert** (Testkatalog, Runner, Makefile)

Ein schlechter Test ist:

- **flaky** (grün/schwarz je nach Lauf)
- **tautologisch** (prüft nichts)
- **konstruiert** (erzwingt das Ergebnis)
- **langsam** (Timeout knapp)
- **undokumentiert** (nicht im Katalog)
- **unklar** (Sollwert ohne Quelle)

---

## §16 — Siehe auch

| Thema | Datei |
|---|---|
| Testkatalog (Rohwerte) | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Runner | `docs/test/run_alpha_tests.md` |
| Projekt-Regeln | `docs/project/Project.md` §2 |
| Architektur | `docs/project/ARCHITECTURE.md` |
| Kernel-API | `docs/project/ProPhysics_API.md` |
| Beitragen (Überblick) | `CONTRIBUTING.md` |
| Physik-Übersicht | `docs/physics/README.md` |
| Changelog | `CHANGELOG.md` |

---

**Ende WRITING_TESTS v1.0.**
