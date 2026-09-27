# ProPhysics — TODO

**Datei:** `TODO.md`
**Version:** 1.6
**Stand:** 2026-09-27 (Kernel-Version 1.23.0, Etappe 23 — release-ready)
**Zweck:** Zentrales Aufgaben-Register.

**Status-Marker:**

| Marker | Bedeutung |
|---|---|
| `[x]` | erledigt — Datei/Code ist im Repo |
| `[~]` | bereitgestellt — Inhalt liegt vor, Ausführung ausstehend |
| `[ ]` | offen |

**Änderung v1.5 → v1.6:**
- Neuer **§5 — ProWB / Web-Docs Integration** (nächster großer Schritt).
- Ehemalige §5–§10 wurden zu §6–§11 — **Inhalt unverändert**.
- §0 um Statuszeile „Web-Docs / ProWB" ergänzt.
- §9 (Nächste konkrete Schritte) um **Punkt 0** (ProWB) ergänzt.
- §10 (Siehe auch) um Web-Docs-Einträge ergänzt.
- **Kein** alter Eintrag gelöscht oder inhaltlich verändert.

---

## §0 — Status heute

| Bereich | Status |
|---|---|
| Kernel-Version | **1.23.0** (Phase 1, Etappe 23) |
| Changelog-Version | **1.23.10** (Konsolidierung abgeschlossen) |
| Tests | **43/43 PASS** (Prio 1/6/7 nachgeprüft: 14/14) |
| Regression-Anker | Prio 1 (12/12), Prio 6 (1/1), Prio 7 (1/1) — grün |
| Compiler-Warnungen | **0** (`/W4` Kernel, `/W3` SDK/Test) |
| Link-Fehler | **0** |
| Kernel-Dateien auf `Kernel:`/`Etappe:` | ✅ |
| Modul-Konsolidierung 1.23.1–1.23.10 | ✅ abgeschlossen |
| SDK, Build, Sub-Makefiles, Build-Docs | ✅ auf Etappe 23 |
| Lizenz, VERSIONING, Repo-Hygiene | ✅ vollständig |
| BASELINE.md, Beispiel-BUILD_INFO | ✅ angelegt |
| **Git-Tags** | ✅ angelegt |
| **Web-Docs / ProWB** | 🟡 Konzept steht, Implementierung ausstehend — **siehe §5** |

**Netto:** Das Repo ist **release-ready**. Der nächste funktionale Schritt ist die
**ProWB / Web-Docs Integration** (§5) — sie ist **kein** Release-Blocker, sondern
eine publizierbare Zusatz-Dimension (Doku-Web-Portal direkt aus den MD-Quellen).

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
- [x] `tools\run_alpha_tests.ps1` — `-Prio`, `-Test`, `-DllDir`

### §1.3 — Sub-Makefiles ✅ erledigt

- [x] `build\prophysics\Makefile.nmake` — 12 Kernel-Module
- [x] `build\sdk\Makefile.sdk.nmake` — `check_core`
- [x] `build\test\Makefile.nmake` — 19 `.c`, `check_deps`

### §1.4 — Build-Dokumente ✅ erledigt

- [x] `docs\build\helper\build.md`, `export.md`,
      `write_build_info.md`
- [x] `docs\build\prophysics\Makefile.md`, `sdk\Makefile.md`,
      `test\Makefile.md`, `main\Makefile.md`
- [x] `docs\build\BUILD_SCRIPT.md`, `docs\build\pro_run.md`
- [x] `docs\test\run_alpha_tests.md`

### §1.5 — Lizenz ✅ erledigt

- [x] `LICENSE.md` — Copyright-Inhaber gesetzt
- [x] `LICENSE.md` — Repository-URL gesetzt
- [x] `LICENSE.md` §8 — Issue-URL gesetzt
- [x] `LICENSE.md` §3 — Zitierweise auf Kernel-Version `1.23.0`
- [x] `COMMERCIAL.md` — Kontakt gesetzt

### §1.6 — VERSIONING-Konzept ✅ erledigt

- [x] `docs\project\VERSIONING.md` angelegt (Konzept-Dokument zur
      Etappen-Versionierung)

### §1.7 — Git-Tags `[~]` ausstehend

- [x] Tag **`v1.23.0`** — Befehl siehe §9
- [x] Tag **`etappe-23`** — Befehl siehe §9

---

## §2 — Sollte vor Release ✅ erledigt

### §2.1 — Versionsregister ✅ erledigt

- [x] `ProPhysics_VersionRegistry.md` auf neues Schema
- [x] Keine `3.1`/`3.2`-Reste mehr

### §2.2 — Doc-Versionen ✅ erledigt

- [x] Alle Projekt- und Test-Docs auf Etappe 23
- [x] `README.md` (Root) Kopf auf `Version: 1.0` / `Kernel: 1.23.0`

### §2.3 — CHANGELOG ✅ erledigt

- [x] Neues Versionsschema
- [x] Konsolidierungs-Serie `1.23.1`–`1.23.8`
- [x] Sammel-Patch `1.23.9` (Release-Vorbereitung)
- [x] Bugfix-Patch `1.23.10` (B7 nachgeholt, Plaquette-Konjugation)

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
- [x] `.github\workflows\ci.yml` (Prio 1, 6, 7 + `SU2-Wilson-Loop`)

### §3.2 — Beispiel-BUILD_INFO ✅

- [x] `docs\build\examples\BUILD_INFO.txt`

### §3.3 — Test-Baseline ✅

- [x] `docs\test\BASELINE.md`

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

### §4.5 — Link-Fehler ✅

- [x] `LNK2001: pro_su2_edge / pro_su2_edge_mut` behoben (`1.23.10`)

---

## §5 — ProWB / Web-Docs Integration (NEU, nächster Schritt)

**Status:** 🟡 Konzept steht; Implementierung ausstehend.
**Ziel:** ProWB als neues Build-Werkzeug in ProPhysics integrieren; die
vorhandenen Markdown-Dokumente als statisches Web-Portal (`out/web/index.html`)
publizierbar machen.
**Leitprinzip:** **Keine MD-Kopien.** Der Builder liest die Doku-Quellen von
ihren **kanonischen Pfaden** über ein Manifest.
**Nicht release-blockierend.** Kann unabhängig von Etappe 24+ laufen.

### §5.0 — Entscheidungen (eingefroren)

| # | Frage | Entscheidung |
|---|---|---|
| 1 | Theme-Default | `proedc` (dark industrial, petroleum) |
| 2 | Sektionen | **7** — Overview, Physics, Modules, API, Tests, Build, Settings |
| 3 | Cross-Refs in MD | Phase 1: ignorieren |
| 4 | Landing Page | Hero + Kurzfassung aus README + 7 Sektions-Karten |
| 5 | Nav-Struktur | flach, eine Ebene; Nav wird aus Manifest generiert |
| 6 | Builder-API | **additiv** — `prowb_build()` (legacy) bleibt, `prowb_build_from_manifest()` kommt neu |

### §5.1 — Verzeichnis-Struktur [ ]

- [ ] `src/prowb/` anlegen
  - [ ] `prowb.c`
  - [ ] `prowb.h` (mit `prowb_build_from_manifest`)
  - [ ] `md_parser.c`
  - [ ] `md_parser.h`
  - [ ] `README.md` (kurz: was ist das, wie bauen)
- [ ] `build/prowb/` anlegen
  - [ ] `Makefile.nmake` (`clean`, `rebuild`, `help`, `CONFIG=release|debug`)
- [ ] `docs/web/` anlegen
  - [ ] `manifest.txt`
  - [ ] `README.md` (Manifest-Pflege-Anleitung)
  - [ ] `src/parts/header.html`
  - [ ] `src/parts/nav.html`
  - [ ] `src/parts/footer.html`
- [ ] `out/web/` in `.gitignore` aufnehmen

### §5.2 — Builder-Kern [ ]

- [ ] `prowb.h` — `prowb_build_from_manifest()` deklarieren
- [ ] `prowb.c` — Manifest-Parser (`Manifest`, `ManifestEntry`,
      `_load_manifest`, Sektions-Cluster)
- [ ] `prowb.c` — `_process_nav_template_manifest()`
      (Platzhalter `{{NAV_DOCS}}` → Sektions-Buttons mit Icons)
- [ ] `prowb.c` — `_process_view_file_manifest()` inkl.
      Platzhalter-Expansion `{{SECTION_CARDS:<Sektion>}}`
- [ ] `prowb.c` — `_process_docs_from_manifest()`
      (Existenz-Check, stderr-Warnung bei fehlender Quelle)
- [ ] `prowb.c` — Standalone-`main()` mit `--manifest`-Flag
      (Legacy-Aufruf bleibt unverändert)
- [ ] `md_parser.c` — unverändert übernehmen (GFM-Parser)

### §5.3 — Build-Integration [ ]

- [ ] `build/prowb/Makefile.nmake`
- [ ] `build/main/Makefile.nmake` — Target `prowb` ergänzen
- [ ] `tools/pro_run.ps1` — neue Aktion `web` (`-Rebuild`-Support)
- [ ] `tools/pro_run.cmd` — Wrapper aktualisieren
- [ ] Master-`all`-Target um `prowb` erweitern (Reihenfolge: **nach** `test`)

### §5.4 — Manifest & Web-Content [ ]

- [ ] `docs/web/manifest.txt` — alle Sektionen aus aktueller
      Doku-Landschaft (siehe Vorschlag in §5.0 Session)
- [ ] `docs/web/src/parts/header.html` — ProPhysics-Branding
      (`⚛️ ProPhysics // KERNEL DOCS`, `KERNEL: 1.23.0 / ETAPPE 23`)
- [ ] `docs/web/src/parts/nav.html` — `{{NAV_DOCS}}` Platzhalter,
      System-Sektion, Settings-Button
- [ ] `docs/web/src/parts/footer.html` — ProPhysics-Footer
      (`© 2026 Sascha Alexander Köhne // BrainAI`)

### §5.5 — CSS & JS (Web-Docs) [ ]

Übernahme aus ProWB-Vorlage, rebranded für ProPhysics:

- [ ] `docs/web/src/css/00_reset.css`
- [ ] `docs/web/src/css/00_vars.css`
- [ ] `docs/web/src/css/10_layout.css`
- [ ] `docs/web/src/css/50_prophysics.css` (ersetzt `50_proedc.css`)
- [ ] `docs/web/src/css/99_home.css`
- [ ] `docs/web/src/css/99_theme_light.css`
- [ ] `docs/web/src/css/99_theme_matrix.css`
- [ ] `docs/web/src/css/99_theme_prophysics.css` (default = `proedc`)
- [ ] `docs/web/src/js/00_bridge.js`
- [ ] `docs/web/src/js/10_app.js` — Router, Theme-Default `proedc`

### §5.6 — Views [ ]

Sektions-Übersichts-Views (mit `{{SECTION_CARDS:<Sektion>}}`):

- [ ] `docs/web/src/views/00_view_home.html` — Hero + 7 Sektions-Karten
- [ ] `docs/web/src/views/01_view_overview.html`
- [ ] `docs/web/src/views/02_view_physics.html`
- [ ] `docs/web/src/views/03_view_modules.html`
- [ ] `docs/web/src/views/04_view_api.html`
- [ ] `docs/web/src/views/05_view_tests.html`
- [ ] `docs/web/src/views/06_view_build.html`

Utility-Views (kein Manifest-Bezug):

- [ ] `docs/web/src/views/96_view_lizenz.html` — **Dual-Licensing**
      (frei für privat/Forschung/Lehre; kommerziell nur mit
      separater Lizenz → Verweis `COMMERCIAL.md` §6)
- [ ] `docs/web/src/views/97_view_datenschutz.html` — DSGVO-Block
      (Netlify, statisch, keine Personendaten)
- [ ] `docs/web/src/views/99_view_settings.html` — Theme-Umschaltung,
      Reset, Links zu Lizenz/Datenschutz

### §5.7 — Verifikation [ ]

- [ ] `pro_run web` — Bau läuft fehlerfrei durch
- [ ] `out/web/index.html` existiert, Größe > 0
- [ ] Browser-Sichtprüfung: alle 7 Sektionen erreichbar
- [ ] Alle Doc-Views erreichbar (keine `missing`-Warnung auf stderr)
- [ ] Theme-Umschaltung funktioniert (`default` / `light` / `matrix`)
- [ ] Keine Konsolen-Fehler im Browser
- [ ] CI: `prowb`-Target in `all` bricht Prio-1/6/7-Tests nicht

### §5.8 — Doku [ ]

- [ ] `src/prowb/README.md`
- [ ] `docs/build/prowb/Makefile.md`
- [ ] `docs/build/pro_run.md` — Aktion `web` dokumentieren
- [ ] `docs/web/README.md` — Manifest-Pflege-Anleitung
- [ ] `TODO.md` §11 — Verweis auf `docs/web/manifest.txt`

### §5.9 — Offene Detailfragen (Phase 1 — nicht blockierend)

- [ ] Cross-Refs in MD-Dateien (`[link](other.md)`) → spätere Phase
- [ ] Landing-Hero-Bild / ASCII-Art — optional
- [ ] Icons pro Sektion feinjustieren (aktuell Vorschlag:
      Overview 📋, Physics ⚛️, Modules 🧩, API 🔌, Tests 🧪, Build 🏗️)

---

## §6 — SDK-Roadmap (optional, nach Etappe 24)

Bewusst offen gelassen — **keine Release-Blocker**, sondern zukünftige
Entwicklungsphasen.

### §6.1 — Sitzung A: SDK-Struktur trennen

- [ ] `main()` aus `pro_sdk_interface.c` auslagern →
      `pro_sdk_runner.c`
- [ ] `pro_sdk_interface.c` enthält nur Library-Code
- [ ] `build\sdk\Makefile.sdk.nmake` auf zwei Targets (DLL + Runner)
- [ ] `GRID_DIM` als CLI-Parameter

### §6.2 — Sitzung B: SDK-API ausbauen

- [ ] `ProSDK_Export_BMP`, `ProSDK_Render_ASCII` — public
- [ ] `ProSDK_Register_Plugin(pu, plugin, context)`
- [ ] `ProSDK_Setup_2D_Torus`, `ProSDK_Setup_3D_Torus`
- [ ] `ProSDK_Check_ABI`, `ProSDK_GetVersion`

### §6.3 — Sitzung C: SDK-Packaging

- [ ] `docs\project\SDK_PACKAGING.md`
- [ ] `prophysics.pc` für pkg-config
- [ ] CMake-Config
- [ ] `export.ps1 sdk -Version <tag>`
- [ ] `INSTALL`-Target

---

## §7 — Zukunft (Etappen 24+)

### §7.1 — Phase 1 abschließen (Etappe 24–27)

- [ ] **Etappe 24** — Euklidisches Pfadintegral
- [ ] **Etappe 25** — GHZ / Mermin
- [ ] **Etappe 26** — Universalität / T-Gate
- [ ] **Etappe 27** — Q61-Migration

**Nach Etappe 27:** Kernel-Version springt auf **2.x**.

### §7.2 — Phase 2 (Makrophysik)

- [ ] **Etappe M1** — U4' Bad
- [ ] **Etappe M2** — U5' Plastizität
- [ ] **Etappe M3** — Makrophysik-Konsistenz

**Nach M1–M3:** Kernel-Version springt auf **3.x**.

### §7.3 — Optional

- [ ] **Etappe 23b** — Creutz-Ratio
- [ ] **Etappe 18d-B** — Wasserstoff-Revision
- [ ] **Etappe O1** — Cache-Optimierung (`CHANNELS_MAX` 16 → 8)

---

## §8 — Publikation

- [ ] Preprint-Kandidat 1: „Quaternion-valued edges on a
      signed-permutation lattice" (arXiv:hep-lat)
- [ ] Preprint-Kandidat 2: „A computational exploration of quantum
      structures from discrete signed permutations" (arXiv:quant-ph)
- [ ] Vergleich mit etablierten Lattice-QCD-Werten
- [ ] Größere Gitter (dim ≥ 128)

---

## §9 — Wie dieses Dokument gepflegt wird

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

## §10 — Nächste konkrete Schritte

**0. Vorbereitung für §5 — ProWB / Web-Docs Integration:**

```cmd
:: 1. Verzeichnisse anlegen
mkdir src\prowb
mkdir build\prowb
mkdir docs\web\src\parts
mkdir docs\web\src\css
mkdir docs\web\src\js
mkdir docs\web\src\views

:: 2. .gitignore erweitern
echo out/ >> .gitignore

:: 3. Erste Bausteine (in dieser Reihenfolge):
::    docs\web\manifest.txt
::    docs\web\src\parts\header.html
::    docs\web\src\parts\nav.html
::    docs\web\src\parts\footer.html
::    src\prowb\prowb.h  (mit prowb_build_from_manifest)
::    src\prowb\prowb.c  (neue Funktion + Manifest-Parser)
::    src\prowb\md_parser.h
::    src\prowb\md_parser.c
::    build\prowb\Makefile.nmake
::    tools\pro_run.ps1  (Aktion "web")
::    build\main\Makefile.nmake  (Target "prowb")
::    docs\web\src\css\*.css
::    docs\web\src\js\10_app.js
::    docs\web\src\views\*.html
```

**1. Build + Regression (falls nicht schon gelaufen):**

```cmd
cd C:\Users\koehn\source\repos\ProPhysics\build\main
build.cmd -Mode all -Rebuild

cd ..\..\tools
run_alpha_tests.cmd -Prio 1,6,7
```

Erwartung: alle PASS, **0 Warnungen**, **0 Link-Fehler**.

**2. Alle Doku-Änderungen committen:**

```cmd
cd C:\Users\koehn\source\repos\ProPhysics
git add -A
git status
git commit -m "1.23.10: B7 nachgeholt, SU2-Plaquette-Konjugation gefixt, 0 Warnings"
```

**3. Git-Tags setzen (schließt §1.7 ab):**

```cmd
git tag -a v1.23.0 -m "Kernel 1.23.0 / Etappe 23 (V&V-Anker 0,08%%)"
git tag -a etappe-23 -m "Etappe 23 abgeschlossen (43/43 PASS)"
```

**4. Merge `rewrite` → `origin` (falls noch nicht geschehen):**

```cmd
:: Sicherheitsnetz
git tag pre-merge-backup

:: rewrite → origin/main (hart, überschreibt origin)
git push origin rewrite:main --force

:: Lokalen Branch nachziehen
git fetch origin
git branch -u origin/main
```

**5. Tags pushen:**

```cmd
git push origin v1.23.0
git push origin etappe-23
```

**6. Nach dem Push:**

- `.github/workflows/ci.yml` läuft und führt Prio 1, 6, 7 +
  `SU2-Wilson-Loop` aus (~1,5 min).
- **Phase 1 ist formal abgeschlossen. Repo ist publizierbar.**

**7. §5 — ProWB / Web-Docs Integration umsetzen:**

- Schritte §5.1 → §5.8 in dieser Reihenfolge abarbeiten.
- Verifikation §5.7 am Ende.
- Doku §5.8 nachziehen.

**8. Optional (kann warten):**

- §4.1 — verbleibende Refactorings (B1, B2, B4, B5, B6, B8)
- §4.2 — `C1` (`Apply_Amp_Step` splitten), `C4` (`ProEdge` Layout)
- §6 — SDK-Roadmap
- §7 — Etappen 24+

---

## §11 — Siehe auch

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
| **ProWB Builder (C)** | `src/prowb/README.md` *(folgt in §5.8)* |
| **Web-Docs Manifest** | `docs/web/manifest.txt` *(folgt in §5.4)* |
| **Web-Docs Pflege** | `docs/web/README.md` *(folgt in §5.8)* |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende TODO v1.6.**