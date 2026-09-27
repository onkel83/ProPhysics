# ProPhysics — TODO

**Datei:** `TODO.md`
**Version:** 1.9
**Stand:** 2026-09-27 (Kernel-Version 1.23.0, Etappe 23 + 23b —
release-ready, ProWB integriert, CI-Doku konsolidiert,
Creutz-Ratio implementiert, V&V-Anker zurückgenommen)
**Zweck:** Zentrales Aufgaben-Register.

**Status-Marker:**

| Marker | Bedeutung |
|---|---|
| `[x]` | erledigt — Datei/Code ist im Repo |
| `[~]` | bereitgestellt — Inhalt liegt vor, Ausführung ausstehend |
| `[ ]` | offen |

**Änderung v1.8 → v1.9:**
- §0 um Statuszeilen „Creutz-Ratio", „V&V-Anker-Rücknahme" und
  „Doku-Konsolidierung 8.1–8.8" erweitert.
- §1.7 (Git-Tags) um `v1.23.13` erweitert (offen).
- §6.5 (CI Phase 2) um `Creutz-Ratio` als optionalen CI-Test
  ergänzt.
- §8.3 (Optional): 23b und 18d-B Status gesetzt, RC-Neulauf als
  nicht-blockierender Punkt.
- §11 (Nächste konkrete Schritte): Punkt 7 auf `v1.23.13`
  umgestellt, Punkt 8 um Doku-Konsolidierung ergänzt.
- §12 (Siehe auch) um `SU2.md` und `alpha_test_creutz_ratio.c`
  erweitert.
- **Kein** alter Eintrag gelöscht oder inhaltlich verändert.

---

## §0 — Status heute

| Bereich | Status |
|---|---|
| Kernel-Version | **1.23.0** (Phase 1, Etappe 23) |
| Changelog-Version | **1.23.13** (Creutz-Ratio + V&V-Anker-Rücknahme) |
| Tests | **45/45 PASS** (Prio 1/6/7 nachgeprüft: 14/14) |
| Regression-Anker | Prio 1 (12/12), Prio 6 (1/1), Prio 7 (1/1) — grün |
| Compiler-Warnungen | **0** (`/W4` Kernel, `/W3` SDK/Test, `/W4` ProWB) |
| Link-Fehler | **0** |
| Kernel-Dateien auf `Kernel:`/`Etappe:` | ✅ |
| Modul-Konsolidierung 1.23.1–1.23.13 | ✅ abgeschlossen |
| SDK, Build, Sub-Makefiles, Build-Docs | ✅ auf Etappe 23 |
| Lizenz, VERSIONING, Repo-Hygiene | ✅ vollständig |
| BASELINE.md, Beispiel-BUILD_INFO | ✅ angelegt |
| **Git-Tags** | ✅ `v1.23.0` und `etappe-23`; `v1.23.13` offen |
| **ProWB / Web-Docs** | ✅ implementiert (`1.23.11`) |
| **CI-Dokumentation** | ✅ vollständig (`1.23.12`) |
| **Creutz-Ratio-Konsistenz-Test** | ✅ FAST + FULL (`1.23.13`) |
| **V&V-Anker-Rücknahme** | ✅ dokumentiert (`1.23.13`) |
| **Doku-Konsolidierung 8.1–8.8** | ✅ abgeschlossen |

**Netto:** Das Repo ist **release-ready** und **publizierbar**.
Die Web-Docs sind unter `out\web\` generierbar und via CI auf
GitHub Pages deploybar. Alle drei CI-Workflows sind dokumentiert.
Der V&V-Anker aus `1.23.0` wurde mit `1.23.13` durch einen
Creutz-Ratio-Konsistenz-Test ersetzt.

---

## §1 — Blockiert Release

### §1.1 — SDK-Versionierung ✅ erledigt

- [x] `pro_sdk_interface.h` — Kopf auf `Kernel: 1.23.0` /
      `Etappe: 23`, Doxygen-`@file`-Block, Doku-Kommentare an
      beiden öffentlichen Symbolen, `PRO_SDK_EXPORTS`-Hinweis
- [x] `pro_sdk_interface.h` — Version-Re-Export
      (`PRO_SDK_VERSION_STRING`, `PRO_SDK_ETAPPE`)
- [x] `pro_sdk_interface.c` — Kopf auf `Kernel: 1.23.0` /
      `Etappe: 23`, Refactoring-22-Doku konsolidiert

### §1.2 — Build-Skripte ✅ erledigt

- [x] `build\main\Makefile.nmake` — Master-Orchestrierung
- [x] `build\main\build.ps1` / `build.cmd` — Wrapper, `-Config`
- [x] `build\main\export.ps1` / `export.cmd` — Export-Typen
      `exe|sdk|kit|all`, ZIP-Erzeugung
- [x] `build\main\write_build_info.ps1` — Etappe-Zeile, `-Config`,
      `-GitStamp`, `-GitNote`
- [x] `tools\pro_run.ps1` / `pro_run.cmd` — zentraler Einstieg
- [x] `tools\run_alpha_tests.ps1` — `-Prio`, `-Test`, `-DllDir`;
      Version 1.0.2 seit 23b

### §1.3 — Sub-Makefiles ✅ erledigt

- [x] `build\prophysics\Makefile.nmake` — 12 Kernel-Module
- [x] `build\sdk\Makefile.sdk.nmake` — `check_core`
- [x] `build\test\Makefile.nmake` — 19 `.c`, `check_deps`;
      Version 3.3 seit 23b
- [x] `build\prowb\Makefile.nmake` — ProWB-Builder (`1.23.11`)

### §1.4 — Build-Dokumente ✅ erledigt

- [x] `docs\build\helper\build.md`, `export.md`,
      `write_build_info.md`
- [x] `docs\build\prophysics\Makefile.md`, `sdk\Makefile.md`,
      `test\Makefile.md`, `main\Makefile.md`
- [x] `docs\build\BUILD_SCRIPT.md`, `docs\build\pro_run.md`
- [x] `docs\test\run_alpha_tests.md` (Version 1.2.0 seit 23b)
- [x] `docs\build\prowb\Makefile.md` (`1.23.11`)
- [x] `docs\build\web-docs-ci.md` (`1.23.11`, v1.0.1 in `1.23.12`)
- [x] `docs\build\ci.md` (`1.23.12`)
- [x] `docs\build\alpha-nightly.md` (`1.23.12`)

### §1.5 — Lizenz ✅ erledigt

- [x] `LICENSE.md` — Copyright-Inhaber gesetzt
- [x] `LICENSE.md` — Repository-URL gesetzt
- [x] `LICENSE.md` §8 — Issue-URL gesetzt
- [x] `LICENSE.md` §3 — Zitierweise auf Kernel-Version `1.23.0`
- [x] `COMMERCIAL.md` — Kontakt gesetzt

### §1.6 — VERSIONING-Konzept ✅ erledigt

- [x] `docs\project\VERSIONING.md` angelegt (Konzept-Dokument zur
      Etappen-Versionierung)

### §1.7 — Git-Tags

- [x] Tag **`v1.23.0`** — Befehl siehe §11
- [x] Tag **`etappe-23`** — Befehl siehe §11
- [ ] Tag **`v1.23.13`** — siehe §11 (nach dem nächsten Push)

---

## §2 — Sollte vor Release ✅ erledigt

### §2.1 — Versionsregister ✅ erledigt

- [x] `ProPhysics_VersionRegistry.md` auf neues Schema
- [x] Keine `3.1`/`3.2`-Reste mehr
- [x] ProWB-Sektionen (§5b, §5c, §6.3, §6.4, §7.5) eingetragen (`1.23.11`)
- [x] CI-Doku-Sektion (§6.5, `1.23.12`)
- [x] 23b-Eintrag in §1 (Kernel-Modul SU2.c), §2 (Header
      ProPhysics.h), §4.2 (Test-Modul creutz_ratio), §5 (Runner
      v1.0.2), §7.1 (SU2.md v1.1) und §8 (Changelog-Historie) —
      Version 1.3 seit 23b

### §2.2 — Doc-Versionen ✅ erledigt

- [x] Alle Projekt- und Test-Docs auf Etappe 23
- [x] `README.md` (Root) auf `Version: 1.0` / `Kernel: 1.23.0`
- [x] `docs\project\README.md` (Website-Version, `1.23.12`)
- [x] `docs\project\SU2.md` (Version 1.1 seit 23b)
- [x] `docs\test\ProPhysics_Testkatalog.md` (Version 2.0 seit 23b)
- [x] `docs\test\BASELINE.md` (Version 1.2 seit 23b)
- [x] `docs\test\run_alpha_tests.md` (Version 1.2.0 seit 23b)

### §2.3 — CHANGELOG ✅ erledigt

- [x] Neues Versionsschema
- [x] Konsolidierungs-Serie `1.23.1`–`1.23.8`
- [x] Sammel-Patch `1.23.9` (Release-Vorbereitung)
- [x] Bugfix-Patch `1.23.10` (B7 nachgeholt, Plaquette-Konjugation)
- [x] Infrastruktur-Patch `1.23.11` (ProWB / Web-Docs)
- [x] CI-Doku-Patch `1.23.12` (Standard-CI, Alpha-Nightly)
- [x] **Creutz-Ratio-Patch `1.23.13` (V&V-Anker-Rücknahme,
      `Wilson_Loop_Average`)**

### §2.4 — Kleinere Inkonsistenzen ✅ erledigt

- [x] `docs\build\helper\build.md` — Verweis korrekt
- [x] `docs\build\helper\write_build_info.md` — Verweis korrekt
- [x] `ProPhysics_Config.h` — `PRO_Q31_HALF_SQRT2` dokumentiert

---

## §3 — Kann (Nice-to-have) ✅ erledigt

### §3.1 — Repo-Hygiene ✅

- [x] `.gitignore`
- [x] `CONTRIBUTORS.md`
- [x] `.github\ISSUE_TEMPLATE\bug_report.md`
- [x] `.github\ISSUE_TEMPLATE\feature_request.md`
- [x] `.github\PULL_REQUEST_TEMPLATE.md`
- [x] `.github\workflows\ci.yml` (`1.23.9`)
- [x] `.github\workflows\web-docs.yml` (`1.23.11`)
- [x] `.github\workflows\alpha-nightly.yml` (`1.23.12`)

### §3.2 — Beispiel-BUILD_INFO ✅

- [x] `docs\build\examples\BUILD_INFO.txt`

### §3.3 — Test-Baseline ✅

- [x] `docs\test\BASELINE.md` (Version 1.2 seit 23b)

---

## §4 — Refactoring-Backlog

### §4.1 — Doppelter Code (mechanisch)

- [ ] **B5** — Q31-One (`2147483647.0`) und Q62-One
      (`4611686018427387904.0`) als benannte Konstanten (>50 Stellen)
- [ ] **B6** — Winkel-Konstanten (`PRO_PI`, `PRO_PI_HALF`,
      `PRO_PI_QUARTER`) einführen (>20 Stellen)
- [ ] **B1** — `pro_complex_matmul_q31`-Helfer für 8×8-Matrix-
      Multiplikation (3 Stellen)
- [x] **D** — `Apply_Dirac_Mass_Term` auf `pro_transport_coeffs`
      (in `1.23.1` erledigt)
- [x] **B3** — `pro_td_apply_2x2_lindblad` vs `pro_fock_2x2_kern`
      (`pro_lindblad_2x2` mit `1.23.1`)
- [ ] **B2** — `pro_td_*` vs `pro_fd_*` in `Density.c` vereinheitlichen
- [ ] **B4** — `pro_round_shift_q30` und `pro_round_shift_q31`
      zusammenführen
- [ ] **B8** — `pro_measure_sharp` und `pro_amp_to_lambda`
      Konsistenz
- [ ] **B9 (neu)** — `pro_su2_loop_sum_xy/_xz/_yz` könnten auf einen
      gemeinsamen parametrisierten Kern reduziert werden (3× ~80 LOC
      dupliziert). Rein mechanisch; Verhalten bit-identisch.
      Kandidat für eine spätere Konsolidierungs-Etappe.

### §4.2 — Struktur

- [ ] **C1** — `Apply_Amp_Step` aufteilen in 3–4 Sub-Funktionen
- [x] **C2** — `pro_su2_exp_apply` Kommentar präzisiert (`1.23.10`)
- [x] **C3** — `PRO_Q31_HALF_SQRT2` kommentiert (`1.23.10`)
- [ ] **C4** — `ProEdge` Layout reorganisieren (Breaking, nur
      Major-Änderung)
- [x] **B7** — SU(2)-Edge-Zugriff vereinheitlicht
      (`1.23.7`, Nachvollzug in `1.23.10` nachgeholt)

### §4.3 — Encoding-Bugs ✅

- [x] `ProPhysics_Exports.h` — Encoding-Check durchgeführt

### §4.4 — Compiler-Warnungen ✅

- [x] Unbenutzte Variablen in `ProPhysics_SU2.c` entfernt (`1.23.10`)
- [x] `uint64_t`→`uint32_t`-Casts in `ProPhysics_Density.c` (`1.23.10`)
- [x] `PRO_NODE_*_MASK` in `ProPhysics_Config.h` eingeführt (`1.23.10`)
- [x] Alle Warnungen auf **0** reduziert
- [x] ProWB-Builder mit 0 Warnungen bei `/W4` (`1.23.11`)
- [x] Creutz-Ratio-Modul mit 0 Warnungen bei `/W3` (`1.23.13`)

### §4.5 — Link-Fehler ✅

- [x] `LNK2001: pro_su2_edge / pro_su2_edge_mut` behoben (`1.23.10`)

---

## §5 — ProWB / Web-Docs Integration ✅ erledigt

**Status:** ✅ abgeschlossen (`1.23.11`).
**Ergebnis:** Web-Portal unter `out\web\index.html`, deploybar auf
GitHub Pages.
**Leitprinzip:** eingehalten — **keine MD-Kopien**, Quellen bleiben
an kanonischen Pfaden (`docs\project\`, `docs\test\`,
`docs\build\`, `docs\physics\`).

### §5.0 — Entscheidungen (eingefroren)

| # | Frage | Entscheidung | Status |
|---|---|---|---|
| 1 | Theme-Default | `proedc` (dark industrial, petroleum) | ✅ |
| 2 | Sektionen | **6** — Overview, Physics, Modules, API, Tests, Build | ✅ |
| 3 | Cross-Refs in MD | Phase 1: ignorieren | ✅ |
| 4 | Landing Page | Hero + Kurzfassung aus README + Sektions-Karten | ✅ |
| 5 | Nav-Struktur | flach, eine Ebene; Nav aus Manifest generiert | ✅ |
| 6 | Builder-API | **additiv** — `prowb_build()` (legacy) bleibt, `prowb_build_from_manifest()` neu | ✅ |

### §5.1 — Verzeichnis-Struktur ✅

- [x] `src/prowb/` angelegt (`prowb.c`, `md_parser.c`, Header, README.md)
- [x] `build/prowb/` angelegt (`Makefile.nmake`)
- [x] `docs/web/` angelegt (`manifest.txt`, `README.md`, `src/`)
- [x] `out/web/` in `.gitignore`
- [x] `bin/prowb/` in `.gitignore`

### §5.2 — Builder-Kern ✅

- [x] `prowb.h` — `prowb_build_from_manifest()`
- [x] `prowb.c` — Manifest-Parser
- [x] `prowb.c` — `_process_nav_template_manifest()`
- [x] `prowb.c` — `_process_view_file_manifest()`
- [x] `prowb.c` — `_process_docs_from_manifest()`
- [x] `prowb.c` — `main()` mit `--manifest`-Flag
- [x] `md_parser.c` — übernommen

### §5.3 — Build-Integration ✅

- [x] `build/prowb/Makefile.nmake`
- [x] Master-`prowb`-Target
- [x] `pro_run web`-Aktion
- [x] Master-`all` um `prowb` erweitert
- [x] `.github/workflows/web-docs.yml`

### §5.4 — Manifest & Web-Content ✅

- [x] `docs/web/manifest.txt` — 44 → **46 Einträge** (`1.23.12`)
- [x] `docs/web/src/parts/header.html`
- [x] `docs/web/src/parts/nav.html`
- [x] `docs/web/src/parts/footer.html`

### §5.5 — CSS & JS ✅

- [x] 8 CSS-Dateien
- [x] `00_bridge.js` + `10_app.js`

### §5.6 — Views ✅

- [x] 10 Views (Home, 6 Sektionen, Utility)

### §5.7 — Verifikation ✅

- [x] `pro_run web` läuft fehlerfrei
- [x] `out/web/index.html` ~1,2 MB
- [x] Alle Sektionen erreichbar
- [x] Theme-Umschaltung funktioniert
- [x] CI deployt auf GitHub Pages

### §5.8 — Doku ✅

- [x] `src/prowb/README.md`
- [x] `docs/build/prowb/Makefile.md`
- [x] `docs/web/README.md`
- [x] `docs/build/web-docs-ci.md`
- [x] `docs/project/README.md` (Website-Version, `1.23.12`)
- [x] `docs/project/ProPhysics_VersionRegistry.md` (§5b, §5c)
- [x] `docs/project/Project.md` (§7.8, §12e)
- [x] `CHANGELOG.md` (`[1.23.11]`)

### §5.9 — Offene Detailfragen (Phase 2 — nicht blockierend)

- [ ] Cross-Refs in MD-Dateien (`[link](other.md)`) → Phase 2
- [ ] Landing-Hero-Bild / ASCII-Art — optional
- [ ] Icons pro Sektion feinjustieren
- [ ] Volltextsuche im Portal — optional
- [ ] PDF-Export — optional
- [ ] i18n (Englisch) — optional

---

## §6 — CI / GitHub Actions ✅ erledigt

**Status:** ✅ abgeschlossen (`1.23.12`).
**Ergebnis:** Drei unabhängige GitHub-Actions-Workflows,
vollständig dokumentiert.

### §6.0 — Übersicht

| Workflow | Trigger | Laufzeit | Doku |
|---|---|---:|---|
| `ci.yml` | Push + PR auf `main` | ~1,5 min | `docs\build\ci.md` |
| `web-docs.yml` | Push auf `main` (Pfad-Filter) | ~1 min | `docs\build\web-docs-ci.md` |
| `alpha-nightly.yml` | **manuell** | ~23–64 min | `docs\build\alpha-nightly.md` |

### §6.1 — Standard-CI ✅

- [x] `.github/workflows/ci.yml` (Datei, `1.23.9`)
- [x] Prio 1, 6, 7, `SU2-Wilson-Loop`
- [x] Log-Upload (`test-logs`, 7 Tage)
- [x] `docs/build/ci.md` (`1.23.12`)

### §6.2 — Alpha-Nightly ✅

- [x] `.github/workflows/alpha-nightly.yml` (`1.23.12`)
- [x] `workflow_dispatch` (manuell)
- [x] Scope-Auswahl: `all-long` \| `prio-5` \| `running-coupling`
- [x] Log-Upload (`alpha-nightly-logs-*`, 30 Tage)
- [x] `docs/build/alpha-nightly.md` (`1.23.12`)

### §6.3 — Web-Docs-CI ✅

- [x] `.github/workflows/web-docs.yml` (`1.23.11`)
- [x] ProWB-Build + Web-Docs + Pages-Deploy
- [x] `docs/build/web-docs-ci.md` (`1.23.11`, v1.0.1 in `1.23.12`)

### §6.4 — CI-Doku-Konsolidierung ✅

- [x] Querverweise zwischen den drei CI-Docs
- [x] `docs/build/BUILD_SCRIPT.md` um §3.7 + §12.6 erweitert
- [x] `docs/test/run_alpha_tests.md` §16 neu geschrieben; Version
      1.2.0 seit 23b
- [x] `CONTRIBUTING.md` auf `pro_run`-Workflow umgestellt
- [x] `README.md` (Root) auf `pro_run`-Workflow umgestellt

### §6.5 — Offene CI-Themen (Phase 2)

- [ ] Nightly-Schedule (aktuell manuell — bewusst)
- [ ] Prio 2/3/4 in Standard-CI aufnehmen? (aktuell nicht)
- [ ] **`Creutz-Ratio` (Fast, ~110 s) in Standard-CI aufnehmen?**
      — passt zeitlich in `ci.yml`, ist aber bislang **nicht**
      eingebunden. Diskussion siehe `docs/build/ci.md` §7.
- [ ] **`Creutz-Ratio-Full` als Alpha-Nightly-Scope `creutz-full`
      verdrahten** — bislang nur lokal via `-Test Creutz-Ratio-Full`
      fahrbar. Siehe `docs/build/alpha-nightly.md`.
- [ ] `timeout-minutes` für `ci.yml` (aktuell Default 360 min)
- [ ] `concurrency` für `ci.yml` (aktuell keine)
- [ ] Pfad-Filter für `ci.yml` (aktuell kein Filter)

---

## §7 — SDK-Roadmap (optional, nach Etappe 24)

Bewusst offen gelassen — **keine Release-Blocker**, sondern zukünftige
Entwicklungsphasen.

### §7.1 — Sitzung A: SDK-Struktur trennen

- [ ] `main()` aus `pro_sdk_interface.c` auslagern →
      `pro_sdk_runner.c`
- [ ] `pro_sdk_interface.c` enthält nur Library-Code
- [ ] `build\sdk\Makefile.sdk.nmake` auf zwei Targets (DLL + Runner)
- [ ] `GRID_DIM` als CLI-Parameter

### §7.2 — Sitzung B: SDK-API ausbauen

- [ ] `ProSDK_Export_BMP`, `ProSDK_Render_ASCII` — public
- [ ] `ProSDK_Register_Plugin(pu, plugin, context)`
- [ ] `ProSDK_Setup_2D_Torus`, `ProSDK_Setup_3D_Torus`
- [ ] `ProSDK_Check_ABI`, `ProSDK_GetVersion`

### §7.3 — Sitzung C: SDK-Packaging

- [ ] `docs\project\SDK_PACKAGING.md`
- [ ] `prophysics.pc` für pkg-config
- [ ] CMake-Config
- [ ] `export.ps1 sdk -Version <tag>`
- [ ] `INSTALL`-Target

---

## §8 — Zukunft (Etappen 24+)

### §8.1 — Phase 1 abschließen (Etappe 24–27)

- [ ] **Etappe 24** — Euklidisches Pfadintegral
- [ ] **Etappe 25** — GHZ / Mermin
- [ ] **Etappe 26** — Universalität / T-Gate
- [ ] **Etappe 27** — Q61-Migration

**Nach Etappe 27:** Kernel-Version springt auf **2.x**.

### §8.2 — Phase 2 (Makrophysik)

- [ ] **Etappe M1** — U4' Bad
- [ ] **Etappe M2** — U5' Plastizität
- [ ] **Etappe M3** — Makrophysik-Konsistenz

**Nach M1–M3:** Kernel-Version springt auf **3.x**.

### §8.3 — Optional

- [x] **Etappe 23b** — Creutz-Ratio (erledigt in `1.23.13`,
      FAST + FULL-Modus, B1/B2/B3 PASS)
- [ ] **Etappe 18d-B** — Wasserstoff-Revision (adaptive Prep);
      weiterhin offen, quantitativ verfehlt (`rel_dev = 0,296`)
- [ ] **Etappe O1** — Cache-Optimierung (`CHANNELS_MAX` 16 → 8)
- [ ] **RC-Neulauf für β ≠ 2, dim=64** (nicht blockierend) — die
      bestehenden `u_plaq`-Werte außer β=2 sind vorläufig und um
      ca. −3 bis −4 % nach unten zu korrigieren (Größenordnung
      aus der β=2-Korrektur in `1.23.10`). Ein vollständiger
      Neulauf würde die RC-Tabelle im Testkatalog aktualisieren.
- [ ] **§4.1 B9** — `pro_su2_loop_sum_{xy,xz,yz}` zusammenführen
      (3× dupliziert, rein mechanisch)

---

## §9 — Publikation

- [ ] Preprint-Kandidat 1: „Quaternion-valued edges on a
      signed-permutation lattice" (arXiv:hep-lat)
- [ ] Preprint-Kandidat 2: „A computational exploration of quantum
      structures from discrete signed permutations" (arXiv:quant-ph)
- [ ] Software-Paper (CPC oder JOSS)
- [ ] Vergleich mit etablierten Lattice-QCD-Werten (Creutz-Ratio
      gegen publizierte `χ`-Werte, nicht nur Konsistenz-Test)
- [ ] Größere Gitter (dim ≥ 128)

---

## §10 — Wie dieses Dokument gepflegt wird

**Wann wird es aktualisiert?**

- Nach jedem Release oder jeder Etappe.
- Bei jedem PR, der einen Punkt abarbeitet.
- Wenn ein neuer Release-Blocker gefunden wird.

**Wer aktualisiert es?**

Der Autor der Änderung, im gleichen PR.

**Format-Regeln:**

- `[x]` erledigt — Datei/Code ist im Repo.
- `[~]` bereitgestellt — Inhalt liegt vor, Ausführung ausstehend.
- `[ ]` offen.
- **Kein** Eintrag wird gelöscht.

---

## §11 — Nächste konkrete Schritte

**1. Build + Prio-All (Pflicht vor Push):**

```cmd
cd tools
pro_run build -Mode all -Rebuild
pro_run test -Prio all
```

Erwartung: **45/45 PASS**, **0 Warnungen**, **0 Link-Fehler**,
~75,5 min Laufzeit.

**2. Web-Docs bauen und prüfen:**

```cmd
cd tools
pro_run web
```

Erwartung: `out\web\index.html` erzeugt (~1,2 MB, 46 Docs).

**3. Browser-Sichtprüfung:**

- `out\web\index.html` direkt öffnen (Doppelklick).
- Alle 6 Sektionen durchklicken.
- Theme-Umschaltung testen.
- Neue Einträge prüfen: `SU2.md` (Version 1.1).
- Testkatalog zeigt 45 Tests.

**4. Commit + Push:**

```cmd
cd C:\Users\koehn\source\repos\ProPhysics
git add -A
git status
git commit -m "1.23.13: Creutz-Ratio-Konsistenz-Test, V&V-Anker-Ruecknahme, Doku-Konsolidierung 23b"
git push origin main
```

**5. CI-Verifikation (Actions-Tab):**

- `ci.yml` — grün (~1,5 min).
- `web-docs.yml` — grün (~1 min).
- Live-URL prüfen: `https://onkel83.github.io/prophysics/`.

**6. Optional — Alpha-Nightly testen:**

- Actions-Tab → „Alpha-Nightly" → „Run workflow".
- Scope: `running-coupling` (schneller Test, ~23 min).
- Danach: `all-long` (voller Lauf, ~64 min).
- Optional nach §6.5: Scope `creutz-full` verdrahten und einmal
  durchlaufen.

**7. Git-Tag `v1.23.13` (nach dem Push):**

```cmd
git tag -a v1.23.13 -m "Creutz-Ratio-Konsistenz-Test + V&V-Anker-Ruecknahme (Etappe 23b)"
git push origin v1.23.13
```

**8. Danach:**

- **Phase 1 ist formal abgeschlossen.** Repo ist publizierbar.
- **Doku-Konsolidierung 8.1–8.8 abgeschlossen:**
  - `CHANGELOG.md` auf 1.23.13.
  - `SU2.md` auf v1.1.
  - `ProPhysics_VersionRegistry.md` auf v1.3.
  - `ProPhysics_Testkatalog.md` auf v2.0.
  - `BASELINE.md` auf v1.2.
  - `run_alpha_tests.md` auf v1.2.0.
  - `Project.md` auf Stand 23b.
  - `TODO.md` auf v1.9 (diese Datei).
- Web-Docs sind live.
- Nächster funktionaler Schritt: **Etappe 24 — Euklidisches
  Pfadintegral.**

**9. Optional (kann warten):**

- §4.1 — verbleibende Refactorings (B1, B2, B4, B5, B6, B8, B9)
- §4.2 — `C1` (`Apply_Amp_Step` splitten), `C4` (`ProEdge` Layout)
- §5.9 — Web-Docs Phase 2 (Cross-Refs, Suche, i18n)
- §6.5 — CI Phase 2 (Nightly-Schedule, Prio 2/3/4, Creutz-Ratio
  in CI, `creutz-full`-Scope)
- §7 — SDK-Roadmap
- §8 — Etappen 24+
- §8.3 — RC-Neulauf für β ≠ 2

---

## §12 — Siehe auch

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
| Test-Runner | `docs/test/run_alpha_tests.md` |
| Konfiguration | `docs/project/CONFIG.md` |
| VERSIONING | `docs/project/VERSIONING.md` |
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
| SU2_Dynamics-Modul | `docs/project/SU2_Dynamics.md` |
| Tensor-Modul | `docs/project/Tensor.md` |
| Zentraler Einstiegspunkt | `docs/build/pro_run.md` |
| Build-Übersicht | `docs/build/BUILD_SCRIPT.md` |
| Standard-CI | `docs/build/ci.md` |
| Alpha-Nightly | `docs/build/alpha-nightly.md` |
| Web-Docs-CI | `docs/build/web-docs-ci.md` |
| ProWB Builder | `src/prowb/README.md` |
| ProWB-Makefile | `docs/build/prowb/Makefile.md` |
| Web-Docs Manifest | `docs/web/manifest.txt` |
| Web-Docs Pflege | `docs/web/README.md` |
| Website-README | `docs/project/README.md` |
| Creutz-Ratio-Test | `src/test/alpha_test_creutz_ratio.c` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende TODO v1.9.**