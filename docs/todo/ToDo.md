# ProPhysics — TODO

**Datei:** `TODO.md`
**Version:** 1.4
**Stand:** 2026-09-27 (Kernel-Version 1.23.0, Etappe 23 — Release-ready)
**Zweck:** Zentrales Aufgaben-Register.

**Status-Marker:**

| Marker | Bedeutung |
|---|---|
| `[x]` | erledigt — Datei/Code ist im Repo |
| `[~]` | bereitgestellt — Inhalt liegt vor, Ausführung ausstehend |
| `[ ]` | offen |

---

## §0 — Status heute

| Bereich | Status |
|---|---|
| Kernel-Version | **1.23.0** (Phase 1, Etappe 23) |
| Tests | **43/43 PASS** |
| Regression-Anker | Prio 1 (12/12), Prio 6 (1/1), Prio 7 (1/1) — grün |
| Kernel-Dateien auf `Kernel:`/`Etappe:` | ✅ erledigt |
| **Modul-Konsolidierung** | ✅ abgeschlossen (Patches 1.23.1–1.23.8) |
| **SDK, Build, Sub-Makefiles, Build-Docs** | ✅ auf Etappe 23 |
| **Lizenz** | ✅ vollständig |
| **VERSIONING.md** | ✅ angelegt |
| **Repo-Hygiene** (`.gitignore`, `.github/`) | ✅ angelegt |
| **BASELINE.md**, **Beispiel-BUILD_INFO** | ✅ angelegt |
| **CHANGELOG 1.23.9** | ✅ eingetragen |
| **C2/C3-Kommentare** | ✅ eingepflegt |
| **Git-Tags** | `[~]` ausstehend |

**Netto:** Das Repo ist inhaltlich **release-ready**. Nur die Git-Tags
fehlen noch.

---

## §1 — Blockiert Release

### §1.1 — SDK-Versionierung ✅ erledigt

- [x] `pro_sdk_interface.h` — Kopf, Doxygen, Version-Re-Export
- [x] `pro_sdk_interface.c` — Kopf auf Etappe 23

### §1.2 — Build-Skripte ✅ erledigt

- [x] Master-Makefile + Wrapper
- [x] `pro_run.ps1` / `pro_run.cmd`
- [x] `run_alpha_tests.ps1` erweitert
- [x] `export.ps1` mit `kit` + ZIP
- [x] `write_build_info.ps1` mit Etappe-Zeile

### §1.3 — Sub-Makefiles ✅ erledigt

- [x] `build\prophysics\Makefile.nmake` — 12 Kernel-Module
- [x] `build\sdk\Makefile.sdk.nmake` — `check_core`
- [x] `build\test\Makefile.nmake` — 19 `.c`, `check_deps`

### §1.4 — Build-Dokumente ✅ erledigt

- [x] Alle 10 Build-Docs auf Etappe 23

### §1.5 — Lizenz ✅ erledigt

- [x] `LICENSE.md` — Copyright, Repo-URL, Issue-URL
- [x] `LICENSE.md` §3 — Zitierweise auf `1.23.0`
- [x] `COMMERCIAL.md` — Kontakt gesetzt

### §1.6 — VERSIONING-Konzept ✅ erledigt

- [x] `docs\project\VERSIONING.md` angelegt

### §1.7 — Git-Tags `[~]` ausstehend

- [~] Tag **`v1.23.0`** — Befehl siehe §9
- [~] Tag **`etappe-23`** — Befehl siehe §9

---

## §2 — Sollte vor Release ✅ erledigt

### §2.1 — Versionsregister ✅ erledigt

- [x] `ProPhysics_VersionRegistry.md` auf neues Schema

### §2.2 — Doc-Versionen ✅ erledigt

- [x] Alle Projekt- und Test-Docs auf Etappe 23
- [x] `README.md` (Root) Kopf auf `Version: 1.0` / `Kernel: 1.23.0`

### §2.3 — CHANGELOG ✅ erledigt

- [x] Neues Versionsschema
- [x] Konsolidierungs-Serie `1.23.1`–`1.23.8`
- [x] Sammel-Patch `1.23.9` eingetragen

### §2.4 — Kleinere Inkonsistenzen ✅ erledigt

---

## §3 — Kann (Nice-to-have) ✅ erledigt

### §3.1 — Repo-Hygiene ✅

- [x] `.gitignore`
- [x] `CONTRIBUTORS.md`
- [x] `.github/ISSUE_TEMPLATE/bug_report.md`
- [x] `.github/ISSUE_TEMPLATE/feature_request.md`
- [x] `.github/PULL_REQUEST_TEMPLATE.md`
- [x] `.github/workflows/ci.yml`

### §3.2 — Beispiel-BUILD_INFO ✅

- [x] `docs\build\examples\BUILD_INFO.txt`

### §3.3 — Test-Baseline ✅

- [x] `docs\test\BASELINE.md`

---

## §4 — Refactoring-Backlog

### §4.1 — Doppelter Code (mechanisch)

- [ ] **B5** — Q31-One / Q62-One als Konstanten (>50 Stellen)
- [ ] **B6** — Winkel-Konstanten (`PRO_PI`, `PRO_PI_HALF`, …)
- [ ] **B1** — `pro_complex_matmul_q31`-Helfer (3 Stellen)
- [x] **D** — `Apply_Dirac_Mass_Term` auf `pro_transport_coeffs`
- [x] **B3** — `pro_lindblad_2x2` vereinheitlicht
- [ ] **B2** — `pro_td_*` vs `pro_fd_*` in `Density.c`
- [ ] **B4** — `pro_round_shift_q30` / `_q31` zusammenführen
- [ ] **B8** — `pro_measure_sharp` / `pro_amp_to_lambda` Konsistenz

### §4.2 — Struktur

- [ ] **C1** — `Apply_Amp_Step` aufteilen (3–4 Sub-Funktionen)
- [x] **C2** — `pro_su2_exp_apply` Kommentar präzisiert
- [x] **C3** — `PRO_Q31_HALF_SQRT2` kommentiert
- [ ] **C4** — `ProEdge` Layout (Breaking, nur Major)
- [x] **B7** — SU(2)-Edge-Zugriff vereinheitlicht

### §4.3 — Encoding-Bugs ✅

---

## §5 — SDK-Roadmap (optional, nach Etappe 24)

- [ ] Sitzung A: SDK-Struktur trennen (DLL + Runner)
- [ ] Sitzung B: SDK-API ausbauen (`ProSDK_*`)
- [ ] Sitzung C: SDK-Packaging (pkg-config, CMake, INSTALL)

---

## §6 — Zukunft (Etappen 24+)

### §6.1 — Phase 1 abschließen

- [ ] Etappe 24 — Euklidisches Pfadintegral
- [ ] Etappe 25 — GHZ / Mermin
- [ ] Etappe 26 — Universalität / T-Gate
- [ ] Etappe 27 — Q61-Migration

### §6.2 — Phase 2 (Makrophysik)

- [ ] Etappe M1 — U4' Bad
- [ ] Etappe M2 — U5' Plastizität
- [ ] Etappe M3 — Makrophysik-Konsistenz

### §6.3 — Optional

- [ ] Etappe 23b — Creutz-Ratio
- [ ] Etappe 18d-B — Wasserstoff-Revision
- [ ] Etappe O1 — Cache-Optimierung

---

## §7 — Publikation

- [ ] Preprint 1: „Quaternion-valued edges…" (arXiv:hep-lat)
- [ ] Preprint 2: „A computational exploration…" (arXiv:quant-ph)
- [ ] Größere Gitter (dim ≥ 128)

---

## §8 — Pflege-Regeln

- `[x]` erledigt
- `[~]` bereitgestellt, Ausführung ausstehend
- `[ ]` offen
- Kein Eintrag wird gelöscht.

---

## §9 — Nächste konkrete Schritte

**1. Build + Regression:**

```cmd
cd build\main
build.cmd -Mode all -Rebuild

cd ..\..\tools
run_alpha_tests.cmd -Prio 1
run_alpha_tests.cmd -Prio 6
run_alpha_tests.cmd -Prio 7
run_alpha_tests.cmd -Test SU2-Wilson-Loop
```

Erwartung: alle PASS.

**2. Git-Tags setzen (schließt §1.7 ab):**

```cmd
cd H:\ProPhysics_SDK\ProPhysics
git add -A
git commit -m "Release 1.23.9: Doku-Konsolidierung, Lizenz, Repo-Hygiene"
git tag -a v1.23.0 -m "Kernel 1.23.0 / Etappe 23 (V&V-Anker 0,08%%)"
git tag -a etappe-23 -m "Etappe 23 abgeschlossen (43/43 PASS)"
git push origin main
git push origin v1.23.0
git push origin etappe-23
```

**3. Nach dem Push:**

- CI läuft via `.github/workflows/ci.yml` (~1,5 min).
- **Repo ist offiziell release-ready / publizierbar.**

**4. Optional (kann warten):**

- §4.1 — B1, B2, B4, B5, B6, B8 (Refactorings)
- §4.2 — C1, C4
- §5 — SDK-Roadmap
- §6 — Etappen 24+

---

## §10 — Siehe auch

| Thema | Datei |
|---|---|
| Projekt-Roadmap | `docs/project/Project.md` |
| Versions-Register | `docs/project/ProPhysics_VersionRegistry.md` |
| Changelog | `CHANGELOG.md` |
| Beitragen | `CONTRIBUTING.md` |
| Architektur | `docs/project/ARCHITECTURE.md` |
| Kernel-API | `docs/project/ProPhysics_API.md` |
| SDK-API | `docs/project/SDK_API.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Test-Baseline | `docs/test/BASELINE.md` |
| Konfiguration | `docs/project/CONFIG.md` |
| VERSIONING | `docs/project/VERSIONING.md` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende TODO v1.4.**