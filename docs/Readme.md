# ProPhysics

**Ein C99-Kernel, der Quantenmechanik aus fünf Regeln entstehen lässt.**

Kein Kontinuumslimes. Keine Quantengravitation. Kein Produkt. Ein
Experimentierfeld — mit einem Validierungsanker, 43 Tests, und einem
Web-Portal, das du gerade liest.

**Kernel-Version:** 1.23.0 · **Etappe:** 23 · **Status:** validierter
Forschungs-Prototyp · **Stand:** 2026-09-28

---

## Was ist ProPhysics?

Ein **topologischer Graph-Kernel** in C99. Auf einem diskreten Gitter
sitzt pro Knoten ein 8-dimensionaler komplexer Vektor. Darüber laufen:

- **signed permutations** statt dichter unitärer Matrizen
- **Q31-Integer-Arithmetik** statt Gleitkomma
- **Union-Find** für Verschränkung (shared reference)
- **Quaternion-Links** für nicht-abelsche Eichtheorie (Skala 2³⁰)

Was dabei herauskommt: unitäre Dynamik, Bloch-Dispersion, Dirac-Spinor,
Jordan-Wigner-Fermionen, SU(2)-Eichtheorie (kinematisch, dynamisch und
thermalisiert), Born-Verteilung, Tsirelson-Korrelation, Lorentz-
Zeitdilatation. Alles auf ≤ 128³-Gittern, alles in Integer-Arithmetik.

---

## Warum?

**Die Frage:** Reichen einfache, diskrete Regeln, um Phänomene zu
erzeugen, die wir als Quantenmechanik kennen — ohne dass QM explizit
eincodiert ist?

**Die fünf Regeln:**

```
U1:  U = {0,1} × {00,01,10,11}          (8 Basis-Zustände)
U2:  A : G → U ∪ G, G ⊆ ℤ³              (Interaktion pro Tick)
U3:  S_{t+1} = f(S_t)                   (Tick-Iteration)
U4:  Ψ(x,y) ⟺ A(x) = A(y)               (shared reference)
U5:  Σ_{x∈G} A_t(x) = Konstante         (Bit-Erhaltung)
```

**Was „emergent" hier heißt:** Der Code enthält **keine** Formel wie
`P = |ψ|²` oder `ω² = k² + m²`. Diese Strukturen entstehen aus dem
Zusammenspiel der fünf Regeln.

**Was „emergent" hier nicht heißt:** Der Kernel ist keine neue Physik.
Er ist eine **andere Darstellung** bekannter Physik. Die Frage ist,
ob diese Darstellung **sparsamer** oder **tiefer** ist.

---

## Wie?

**Sparsam.** Keine externen Abhängigkeiten außer der C-Standard-
Bibliothek. Kein `malloc` im Hotpath, kein `div`/`mod` im Hotpath.
Alles in Q31, deterministisch, auf jeder x86-64-CPU bit-identisch.

**Testbar.** 43 Tests über 8 Prioritätsstufen. Jede Etappe endet mit
einem grünen Regressionstest. Wer etwas ändert, kann es sofort
verifizieren.

**Validierbar.** Der Kernel ist gegen **externe Lattice-QCD-Physik**
validiert — der erste absolute Vergleich mit einer publizierten Zahl:

| Größe | Wert |
|---|---|
| ⟨P⟩(β=2, dim=64) gemessen | 0,43346 ± 0,00005 |
| Referenz `I₂(2)/I₁(2)` | 0,43313 |
| **Abweichung** | **0,08 %** |

Ein falscher Metropolis-Sampler würde auf Prozent-Ebene abweichen,
nicht auf 0,08 %.

---

## Wo?

**Der Kernel** — 12 Module in `src\prophysics\`, ~36 000 Zeilen C99.
Vollständig dokumentiert in der Sektion **Modules** dieses Portals.

**Die Tests** — 43 Test-Module in `src\test\`. Ergebnis, Kriterien
und Rohwerte in der Sektion **Tests**.

**Die API** — Kernel-Referenz und SDK-Interface in der Sektion **API**.

**Das Build-System** — `pro_run` als zentraler Einstiegspunkt, vier
Sub-Makefiles, ein Web-Docs-Builder. Alles in der Sektion **Build**.

**Die Physik** — kompakte Einführung in die Sektion **Physics**.

---

## Wer?

**BrainAI** ist der Herausgeber von ProPhysics. Inhaber: Sascha
Alexander Köhne.

BrainAI entwickelt und pflegt den Kernel, die Test-Suite, die
Dokumentation und dieses Web-Portal. Das Projekt ist ein
Forschungsprototyp — kein Produkt — und wird als Open-Source-Projekt
für Forschung, Lehre und nicht-kommerzielle Nutzung bereitgestellt.

**Beiträge sind willkommen.** Siehe **Contributing** in der Sektion
*Overview*. Kommerzielle Nutzung ist über eine separate Lizenz
geregelt (`COMMERCIAL.md`).

---

## In fünf Minuten?

1. **Was es ist:** Lese diese Seite zu Ende. (~2 min)
2. **Was es kann:** Öffne die Sektion **Physics**. (~3 min)
3. **Ob es stimmt:** Öffne die Sektion **Tests**, schau dir den
   V&V-Anker an.
4. **Wie man es baut:** Öffne die Sektion **Build**, lies `pro_run`.
5. **Wie es funktioniert:** Öffne die Sektion **Modules**, fang mit
   `Core` oder `Amp` an.

Wenn dich nach diesen fünf Minuten etwas überzeugt hat — super. Wenn
nicht, auch okay. Die Zahlen bleiben stehen.

---

## Was ProPhysics nicht ist

- Keine Quantengravitation
- Keine Simulation echter QCD
- Kein Kontinuumslimes
- Kein Bell-Bruch (S > 2 wäre ein Superdeterminismus-Loop)
- Keine Behauptung, dass die Urregeln „wahr" sind
- Keine Alternative zu ITensor, QDP++, MILC

ProPhysics ist eine **andere Darstellung**, kein Ersatz. Wer QCD auf
großen Gittern simulieren will, nimmt MILC. Wer emergente QM
untersuchen will, kann hier anfangen.

---

## Status

| | |
|---|---|
| Kernel-Version | **1.23.0** (Phase 1, Etappe 23) |
| Tests | **43/43 PASS** |
| V&V-Anker | **0,08 %** gegen externe Lattice-QCD |
| Compiler-Warnungen | **0** (`/W4` Kernel, `/W3` SDK/Test) |
| Externe Abhängigkeiten | **keine** (außer `msvcrt`) |

**Nächste Etappe:** Euklidisches Pfadintegral (24).
**Danach:** GHZ / Mermin (25), Universalität (26), Q61 (27),
Makrophysik (M1–M3).

---

## Loslegen

Dieses Portal hat sechs Sektionen. Die Navigation links zeigt sie.
Für den Anfang reichen zwei:

- **Overview** — Projekt, Architektur, Versionierung.
- **Modules** — was der Kernel im Detail tut.

Für Mitwirkende:

- **Build** — wie man baut, testet, exportiert.
- **Contributing** (in *Overview*) — wie man beiträgt.

---

**ProPhysics. 36 000 Zeilen. 43 Tests. Eine Zahl: 0,08 %.**

---

*Lizenz: siehe `LICENSE.md` — frei für privat, Forschung und Lehre.
Kommerziell nur mit separater Lizenz (`COMMERCIAL.md`).*