# ProPhysics — TODO

**Datei:** `TODO.md`
**Version:** 1.10
**Stand:** 2026-09-28 (Kernel-Version 1.23.0, Etappe 23 + 23b–23e —
release-ready, ProWB integriert, CI-Doku konsolidiert,
Creutz-Ratio implementiert, V&V-Anker zurückgenommen,
Etappe 23f Doku-Konsolidierung geplant)
**Zweck:** Zentrales Aufgaben-Register.

**Status-Marker:**

| Marker | Bedeutung |
|---|---|
| `[x]` | erledigt — Datei/Code ist im Repo |
| `[~]` | bereitgestellt — Inhalt liegt vor, Ausführung ausstehend |
| `[ ]` | offen |

**Änderung v1.9 → v1.10:**
- §8b neu: **Etappe 23f — Doku-Konsolidierung nach 1.23.14**
  (Torelon, Prio-99-Sektion, Registry v1.4, BUILD_SCRIPT-Korrektur).
- §11 neuer Punkt 10 (Etappe 23f ausführen, `[1.23.15]`-Tag).
- Kopf-Stand auf 23f-Vorbereitung gesetzt, Version v1.10.
- **Kein** alter Eintrag gelöscht oder inhaltlich verändert.

---

## §0 — Status heute

| Bereich | Status |
|---|---|
| Kernel-Version | **1.23.0** (Phase 1, Etappe 23) |
| Changelog-Version | **1.23.14** (Sampler-Korrektur Haar + 2D-Bessel) |
| Tests | **46/46 PASS** (Prio 1–8; Prio-All-Lauf in Vorbereitung) |
| Regression-Anker | Prio 1 (12/12), Prio 6 (1/1), Prio 7 (1/1) — grün |
| Compiler-Warnungen | **0** (`/W4` Kernel, `/W3` SDK/Test, `/W4` ProWB) |
| Link-Fehler | **0** |
| Kernel-Dateien auf `Kernel:`/`Etappe:` | ✅ |
| Modul-Konsolidierung 1.23.1–1.23.14 | ✅ abgeschlossen |
| SDK, Build, Sub-Makefiles, Build-Docs | ✅ auf Etappe 23 |
| Lizenz, VERSIONING, Repo-Hygiene | ✅ vollständig |
| BASELINE.md, Beispiel-BUILD_INFO | ✅ angelegt |
| **Git-Tags** | ✅ `v1.23.0` und `etappe-23`; `v1.23.13` offen; `v1.23.14`/`v1.23.15` geplant |
| **ProWB / Web-Docs** | ✅ implementiert (`1.23.11`) |
| **CI-Dokumentation** | ✅ vollständig (`1.23.12`) |
| **Creutz-Ratio-Konsistenz-Test** | ✅ FAST + FULL (`1.23.13`) |
| **V&V-Anker-Rücknahme** | ✅ dokumentiert (`1.23.13`/`1.23.14`) |
| **String-Tension Rev.2 / 2D-Metropolis** | ✅ (`1.23.14`) |
| **Torelon-Mass (Etappe 23d)** | ⚠ implementiert, Doku ausstehend (siehe §8b) |
| **Doku-Konsolidierung 8.1–8.8** | ✅ abgeschlossen |
| **Etappe 23f Doku-Konsolidierung** | ⏳ geplant (siehe §8b) |

**Netto:** Das Repo ist **release-ready** und **publizierbar**.
Die Web-Docs sind unter `out\web\` generierbar und via CI auf
GitHub Pages deploybar. Alle drei CI-Workflows sind dokumentiert.
Der V&V-Anker aus `1.23.0` wurde mit `1.23.13`/`1.23.14` durch
einen Creutz-Ratio-Konsistenz-Test und eine 2D-Bessel-Referenz
ersetzt. Offen ist die Doku-Konsolidierung für Etappe 23d/23e
(Torelon, Prio 99) — siehe §8b.

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
      Version 1.0.4 seit 23d/23e

### §1.3 — Sub-Makefiles ✅ erledigt

- [x] `build\prophysics\Makefile.nmake` — 12 Kernel-Module
- [x] `build\sdk\Makefile.sdk.nmake` — `check_core`
- [x] `build\test\Makefile.nmake` — 20 `.c` (18 Alpha + 2 Example),
      `check_deps`; Version 3.6 seit 23e
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
- [ ] Tag **`v1.23.14`** — siehe §11 (Sampler-Korrektur)
- [ ] Tag **`v1.23.15`** — siehe §11 (Etappe 23f, Doku-Konsolidierung)

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
- [ ] 23c/23d/23e-Einträge (Test-Module, Runner v1.0.4,
      Changelog `[1.23.14]`) — siehe §8b.D1

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
- [x] **Creutz-Ratio-Patch `1.23.13` (V&V-Anker-Rücknahme,**
      **`Wilson_Loop_Average`)**
- [x] **Sampler-Korrektur `1.23.14` (Haar-Vorschlag, 2D-Bessel)**
- [ ] **Doku-Patch `1.23.15` (Etappe 23f)** — siehe §8b.D2

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

- [x] `docs\test\BASELINE.md` (Version 1.2 seit 23b; v1.3 in `1.23.14`)

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
- [ ] **`Torelon-Mass-Full` und `Metropolis-2D-Full` als
      Nightly-Scopes verdrahten** — siehe §8b.
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

## §8b — Etappe 23f: Doku-Konsolidierung nach 1.23.14

**Version geplant:** CHANGELOG `[1.23.15]` (Doku-Patch ohne Kernel-Bump).
**Ziel:** Alle Doku an den tatsächlichen Stand von Code, Runner und
Registrierung angleichen. Kein Kernel-Change, kein neuer Test.

### §8b.0 — Aktueller Ist-Zustand (aus Querlesen)

| Bereich | Doku sagt | Tatsächlich |
|---|---|---|
| Test-Anzahl Prio 1-8 | 46 (Katalog v2.1) | **47** (Runner v1.0.4+ hat 6 Prio-8-Tests) |
| Test-Anzahl Prio 99 | nicht dokumentiert | **5** (`Creutz-Ratio-Full`, `String-Tension-Full`, `String-Tension-Huge`, `Torelon-Mass-Full`, `Metropolis-2D-Full`) |
| `Torelon-Mass` | „in Entwicklung" (Katalog §10.4) | **aktiv in Prio 8** (Runner) |
| `run_alpha_tests.ps1` | v1.0.2 in Registry | **v1.0.4+** |
| `alpha_test_common.h` | v3.2 in Registry | **v3.5** (mit falschem I1/I0-Kommentar) |
| `build/test/Makefile.nmake` | v3.3 in Registry | **v3.6** (20 Quellen, 21 in echo) |
| `BUILD_SCRIPT.md` | Master-`all` baut ProWB | Master-`all` = `setup prophysics sdk test info` (kein `prowb`) |
| `alpha_test_common.h` | Prototyp `test_torelon_mass` fehlt | wird dennoch aufgerufen |

### §8b.1 — Arbeitspakete

**A — Code + Kommentar-Korrekturen (klein, isoliert)**

- [ ] **A1 — `src/test/header/alpha_test_common.h`**
  - Kommentar `test_metropolis_2d`: `I1(beta)/I0(beta)` → `I2(beta)/I1(beta)`.
  - Prototyp ergänzen: `bool test_torelon_mass(bool full_dims, bool anchor);`
  - Version 3.5 → 3.6; Header-Kommentar auf v3.6.
  - **Input:** `src/test/header/alpha_test_common.h`

- [ ] **A2 — `build/test/Makefile.nmake`**
  - Echo-Zeile für `$(EXE_ALPHA)`: `(21 Quellen, ...)` → `(20 Quellen, ...)`.
  - **Input:** `build/test/Makefile.nmake`

- [ ] **A3 — `tools/run_alpha_tests.ps1`**
  - `.NOTES`-Block korrigieren: `Zaehlung all 45 → 46, Prio 99: 3 → 4`
    → `Zaehlung all 47, Prio 99: 5`.
  - Versionskopf auf `1.0.5 (Etappe 23d + 23e)` setzen.
  - **Input:** `tools/run_alpha_tests.ps1`

**B — Test-Dokumentation**

- [ ] **B1 — `docs/test/ProPhysics_Testkatalog.md`** (v2.1 → v2.2)
  - §0: Prio 8 = **6** Tests, Prio 99 = **5** (neue Zeile), Gesamt = **47**
    (Prio 1-8); 52 inkl. Prio 99.
  - §8: **T8.6 — Torelon-Mass (Etappe 23d)** neu.
  - §9: **Prio-99-Sektion** mit 5 Tests neu.
  - §0 Laufzeit + CI-Empfehlung aktualisieren.
  - §10.4 offene Punkte: Torelon-Status klären.
  - §11 Historie v2.2.
  - **Input:** `docs/test/ProPhysics_Testkatalog.md`

- [ ] **B2 — `docs/test/BASELINE.md`** (v1.3 → v1.4)
  - §1 Prio-Übersicht: Prio 8 = **6**, Gesamt = **47**.
  - §2.7 um Torelon erweitern.
  - Stand-Zeile aktualisieren.
  - **Input:** `docs/test/BASELINE.md`

- [ ] **B3 — `docs/test/run_alpha_tests.md`** (v1.2.0 → v1.3.0)
  - §5 Prios: Prio 8 = 6, Prio 99 = 5, Gesamt 47.
  - §6 Prio 8: Zeile `Torelon-Mass` ergänzen.
  - §6 neue **Prio-99-Sektion** mit 5 Tests.
  - §9 Timeouts: `Torelon-Mass` (1200 s) + `Torelon-Mass-Full` (3600 s).
  - §21 Änderungshistorie v1.3.0.
  - **Input:** `docs/test/run_alpha_tests.md`

**C — Build-Doku**

- [ ] **C1 — `docs/build/BUILD_SCRIPT.md`** (v1.2 → v1.3)
  - §3.2 Master-Targets: `all: setup prophysics sdk test info`
    (kein `prowb`).
  - §3.6 Position ProWB: explizit „nicht im Master-`all`,
    separat via `pro_run web`".
  - §12.5 Build-Kette: ProWB **neben** `test`, nicht danach.
  - **Input:** `docs/build/BUILD_SCRIPT.md`

- [ ] **C2 — `docs/build/ci.md`** (v1.0.0 → v1.0.1)
  - Runner-Version auf v1.0.5 referenzieren.
  - **Input:** `docs/build/ci.md`

- [ ] **C3 — `docs/build/alpha-nightly.md`** (v1.0.0 → v1.0.1)
  - Scope-Tabelle um Prio 99 (`torelon-full`, `metropolis-2d-full`)
    erweitern.
  - Runner-Version v1.0.5.
  - **Input:** `docs/build/alpha-nightly.md`

**D — Meta-Doku**

- [ ] **D1 — `docs/project/ProPhysics_VersionRegistry.md`** (v1.3 → v1.4)
  - §4.2: 3 fehlende Test-Module eintragen
    (`alpha_test_string_tension.c`, `alpha_test_torelon.c`,
    `alpha_test_metropolis_2d.c`).
  - §5: `run_alpha_tests.ps1` v1.0.2 → **v1.0.5**.
  - §5: `run_alpha_tests.md` v1.2.0 → **v1.3.0**.
  - §6.2: test-Makefile v3.3 → **v3.6**.
  - §7.3: Soll-Versionen (Katalog v2.2, BASELINE v1.4).
  - §8: `[1.23.14]` und `[1.23.15]` ergänzen.
  - §0.1 PATCH-Wertebereich bis 15.
  - §10 Historie v1.4.
  - **Input:** `docs/project/ProPhysics_VersionRegistry.md`

- [ ] **D2 — `CHANGELOG.md`** (v1.23.14 → v1.23.15)
  - Neuer Eintrag `[1.23.15]` — Doku-Konsolidierung nach Etappe 23d/23e.
  - Kein Kernel-Change. Test-Zählung 47/47 (Prio 1-8), +5 Prio 99.
  - Anschließend `[Unreleased]` prüfen.
  - **Input:** `CHANGELOG.md`

- [ ] **D3 — `TODO.md`** (v1.10 → v1.11)
  - Nach Abschluss von Etappe 23f: §8b-Punkte als `[x]` markieren,
    Status in §0 auf „Etappe 23f abgeschlossen" setzen.
  - **Input:** diese Datei (nach Bearbeitung)

**E — Optional (Alternative zu C1)**

- [ ] **E1 — `build/main/Makefile.nmake`**
  - Alternative zu C1: Master-`all`-Target um `prowb` erweitern:
    `all: setup prophysics sdk test prowb info`
  - Dann bleibt `BUILD_SCRIPT.md` wie es ist.
  - **Entscheidung nötig:** Doku korrigieren (C1) vs. Build erweitern (E1).
  - **Input:** `build/main/Makefile.nmake`

### §8b.2 — Eingabe-Liste (was der User liefern muss)

| # | Datei (Input) |
|---|---|
| A1 | `src/test/header/alpha_test_common.h` |
| A2 | `build/test/Makefile.nmake` |
| A3 | `tools/run_alpha_tests.ps1` |
| B1 | `docs/test/ProPhysics_Testkatalog.md` |
| B2 | `docs/test/BASELINE.md` |
| B3 | `docs/test/run_alpha_tests.md` |
| C1 | `docs/build/BUILD_SCRIPT.md` |
| C2 | `docs/build/ci.md` |
| C3 | `docs/build/alpha-nightly.md` |
| D1 | `docs/project/ProPhysics_VersionRegistry.md` |
| D2 | `CHANGELOG.md` |
| D3 | `TODO.md` (nach Bearbeitung) |
| E1 | `build/main/Makefile.nmake` (nur wenn E1 gewählt) |

**Reihenfolge-Empfehlung für Bearbeitung:**

1. A1, A2, A3 (Kommentar + Prototyp)
2. D1, D2 (Registry + Changelog)
3. B1, B2, B3 (Test-Doku)
4. C1, C2, C3 (Build-Doku)
5. D3 (ToDo-Bump)
6. E1 (optional)

**R-Konformität:** Kein Kernel-Code, kein neuer Test, keine
Signaturänderung. Reine Doku- und Kommentar-Korrektur. R7-konform.

### §8b.3 — Offene Entscheidungen

| # | Frage | Optionen |
|---|---|---|
| 1 | Master-`all` erweitern (E1) vs. Doku korrigieren (C1)? | Empfehlung: **C1** (Trennung gewollt) |
| 2 | Torelon-Mass-Status: PASS oder FAIL? | Re-Run nötig. Katalog §10.2 nennt „Timeout, Korrelation fällt auf Rauschen bei z=1". Wenn das noch gilt → Torelon als **FAIL** markieren; sonst als **PASS**. |
| 3 | Runner-Version für den nächsten Bump: 1.0.5 oder 1.1.0? | Empfehlung: **1.0.5** (kleiner Patch, keine neue Test-Struktur) |

**Ende §8b.**

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

Erwartung: **47/47 PASS** (Prio 1–8), **0 Warnungen**,
**0 Link-Fehler**, ~80 min Laufzeit inkl. String-Tension, Metropolis-2D.

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
- Testkatalog zeigt 47 Tests (nach §8b).

**4. Commit + Push:**

```cmd
cd C:\Users\koehn\source\repos\ProPhysics
git add -A
git status
git commit -m "1.23.14: Sampler-Korrektur Haar + 2D-Bessel-Referenz"
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

**7. Git-Tag `v1.23.14` (nach dem Push):**

```cmd
git tag -a v1.23.14 -m "Sampler-Korrektur Haar + 2D-Bessel-Referenz"
git push origin v1.23.14
```

**8. Danach:**

- **Phase 1 ist formal abgeschlossen.** Repo ist publizierbar.
- Web-Docs sind live.
- Nächster funktionaler Schritt: **Etappe 24 — Euklidisches
  Pfadintegral.**

**9. Etappe 23f — Doku-Konsolidierung (siehe §8b):**

Nach Abschluss der Dokumenten-Runde `[1.23.15]`:

```cmd
git add -A
git commit -m "1.23.15: Doku-Konsolidierung 23f (Torelon, Prio 99, Registry v1.4)"
git push origin main

git tag -a v1.23.15 -m "Doku-Konsolidierung 23f"
git push origin v1.23.15
```

**10. Optional (kann warten):**

- §4.1 — verbleibende Refactorings (B1, B2, B4, B5, B6, B8, B9)
- §4.2 — `C1` (`Apply_Amp_Step` splitten), `C4` (`ProEdge` Layout)
- §5.9 — Web-Docs Phase 2 (Cross-Refs, Suche, i18n)
- §6.5 — CI Phase 2 (Nightly-Schedule, Prio 2/3/4, Creutz-Ratio
  in CI, `creutz-full`-Scope, Torelon-Full-Scope)
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
| Tests schreiben | `docs/test/WRITING_TESTS.md` |
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
| String-Tension-Test | `src/test/alpha_test_string_tension.c` |
| Torelon-Mass-Test | `src/test/alpha_test_torelon.c` |
| 2D-Metropolis-Test | `src/test/alpha_test_metropolis_2d.c` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende TODO v1.10.**