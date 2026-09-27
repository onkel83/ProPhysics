# ProPhysics — Versionierung

**Datei:** `docs/project/VERSIONING.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Zweck:** Konzept-Dokument zur Etappen-Versionierung. Ergänzt
`CHANGELOG.md` §0 und §2 um eine eigenständige, kurze Referenz.

---

## §0 — Wozu dieses Dokument

ProPhysics verwendet ein **etappen-basiertes** Versionsschema, nicht
SemVer. Dieses Dokument erklärt das Schema, grenzt es gegen SemVer ab
und beschreibt die Kopplung zwischen Kernel, SDK und Dokumentation.

**Verwandte Dokumente:**

- `CHANGELOG.md` §0 — Format und Lese-Regeln
- `CHANGELOG.md` §2 — Versionsschema im Detail
- `docs/project/ProPhysics_VersionRegistry.md` — Versionen aller Dateien
- `src/prophysics/header/ProPhysics_Version.h` — semantischer Anker

---

## §1 — Das Schema

```
MAJOR . MINOR . PATCH
```

| Ziffer | Bedeutung | Wertebereich |
|---|---|---|
| **MAJOR** | Phase | `1`, `2`, `3` |
| **MINOR** | Etappen-Nummer | `1`, `2`, …, `23`, `24`, … |
| **PATCH** | Fix / Konsolidierung | `0`, `1`, `2`, … |

**Beispiele:**

| Kernel-Version | Bedeutung |
|---|---|
| `1.23.0` | Phase 1, Etappe 23, kein Fix |
| `1.23.1` | Phase 1, Etappe 23, Konsolidierungs-Fix 1 |
| `1.23.8` | Phase 1, Etappe 23, Konsolidierungs-Fix 8 |
| `2.0.0` | Phase 2, Etappe 24+ (geplant) |
| `3.0.0` | Phase 3, Makrophysik (geplant) |

---

## §2 — Phasen

| Phase | MAJOR | Bedeutung | Etappen | Status |
|---|:-:|---|---|---|
| **1** | `1.x.y` | Fundament + erste Validierung | 1–23 | **aktiv** |
| **2** | `2.x.y` | Komplette QM | 24–27 | geplant |
| **3** | `3.x.y` | Makrophysik | M1–M3 | geplant |

**Warum „Phase" als MAJOR?** Die Phase spiegelt den **physikalischen
Status** des Kernels, nicht den Code-Umfang.

- Phase 1 endet, wenn der Kernel gegen einen externen Referenzwert
  validiert ist (erreicht in Etappe 23, V&V-Anker 0,08 %).
- Phase 2 endet, wenn die vollständige QM abgedeckt ist (Pfadintegral,
  GHZ, Universalität, Q61).
- Phase 3 endet, wenn der klassische Limes und die Kopplung an
  Unsichtbares (U4'/U5') funktionieren.

---

## §3 — Abgrenzung zu SemVer

| Aspekt | SemVer | ProPhysics |
|---|---|---|
| MAJOR | API-Bruch | Phasenwechsel |
| MINOR | Feature | neue Etappe |
| PATCH | Bugfix | Fix / Konsolidierung |
| Sortierung | Bruch / Feature / Fix | Phase / Etappe / Fix |

**Konsequenzen:**

- Ein MAJOR-Sprung (`1.x → 2.x`) ist **kein API-Bruch**. API-Brüche
  sind durch R5 („keine stillen API-Brüche") ohnehin verboten.
- Innerhalb einer Phase ist die API stabil. Neue Funktionen kommen
  **additiv** hinzu.
- Ein MINOR-Sprung (`1.22 → 1.23`) entspricht einer neuen Etappe.
- Ein PATCH-Sprung (`1.23.0 → 1.23.1`) ist ein Fix oder eine
  Konsolidierung.

**Warum nicht SemVer?** SemVer sortiert nach „Major-Bruch, Feature,
Bugfix". ProPhysics sortiert nach „Phase, Etappe, Fix". Die Etappen-
Nummer ist die relevante Größe — sie spiegelt den physikalischen
Fortschritt, nicht die Code-Änderungen.

---

## §4 — Versions-Kopplung

| Komponente | Folgt | Begründung |
|---|---|---|
| **Kernel** | eigenständig | semantischer Anker |
| **SDK** | Kernel | beschreibt Kernel-API |
| **Dokumentation** | Kernel | beschreibt Kernel-Status |
| **Tests** | eigenständig (`1.0.0`) | wächst mit Test-Suite |
| **Build / Tools** | eigenständig (`1.0.0`) | wächst mit Infrastruktur |

**Regel:** Wenn die Kernel-Version sich ändert, müssen SDK und Doku
nachziehen. Tests und Build-Skripte **können**, müssen aber nicht.

**PATCH ohne Kernel-Bump:** Ein PATCH-Sprung in `CHANGELOG.md` **ohne**
Kernel-Bump ist erlaubt, wenn keine ABI-Änderung stattfindet (z. B.
reine Doku- und Header-Konsolidierung).

---

## §5 — Wer setzt welche Version?

| Datei | Trägt | Woher |
|---|---|---|
| `src/prophysics/header/ProPhysics_Version.h` | Kernel-Version | Autor, bei Freigabe |
| `src/sdk/header/pro_sdk_interface.h` | Kernel-Version | abgeleitet |
| `docs/project/*.md` | Kernel-Version + Doc-Version | abgeleitet |
| `CHANGELOG.md` | Changelog-Version | folgt Kernel-Patch |
| `ProPhysics_VersionRegistry.md` | alle Versionen | folgt Kernel |

**Semantischer Anker:** `ProPhysics_Version.h`. Bei Inkonsistenz gilt
diese Datei. `CHANGELOG.md` folgt ihr. Alle anderen Dokumente folgen
`CHANGELOG.md`.

---

## §6 — Historie der Schema-Umstellung

Vor dem 2026-09-26 folgte ProPhysics einer SemVer-ähnlichen Logik
(`1.x.y` bis `3.0.0`). Mit der Umstellung auf das Etappen-Schema
wurde die Kernel-Version ausschließlich durch Phase + Etappe + Fix
bestimmt. Die alte `3.0.0` entspricht jetzt `1.23.0`.

Die vollständige Abbildungstabelle steht in `CHANGELOG.md` §2.5.

---

## §7 — Wann springt die Version?

**MINOR:** Bei jeder neuen Etappe (`1.23.0 → 1.24.0`).

**PATCH:** Bei einem Fix, einer Unter-Etappe oder einer Konsolidierung.
Die Konsolidierungs-Serie `1.23.1`–`1.23.8` ist ein Beispiel.

**MAJOR:** Beim Phasenwechsel (`1.23.8 → 2.0.0` nach Etappe 27).

**Nicht:** Reine Doku-Änderungen ohne Code-Bezug, Test-Erweiterungen
ohne Kernel-Änderung, interne Refactorings ohne API-Bruch **und ohne
substanzielle Struktur-Änderungen**.

---

## §8 — Siehe auch

| Thema | Datei |
|---|---|
| Changelog | `CHANGELOG.md` |
| Versions-Register | `docs/project/ProPhysics_VersionRegistry.md` |
| Kernel-Version | `src/prophysics/header/ProPhysics_Version.h` |
| Projekt-Roadmap | `docs/project/Project.md` §18 |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende VERSIONING v1.0.**