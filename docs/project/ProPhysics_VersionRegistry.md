# ProPhysics — Versions-Register

**Datei:** `docs/project/ProPhysics_VersionRegistry.md`
**Version:** 1.2
**Kernel:** 1.23.0
**Etappe:** 23
**Zweck:** Zentrale Übersicht aller Versionen in allen Dateien.
Dient der Vereinheitlichung und dem schnellen Auffinden von
Inkonsistenzen.
**Stand:** 2026-09-28 (Kernel 1.23.0, Etappe 23, Konsolidierungs-Serie
1.23.1–1.23.12 abgeschlossen, Changelog-Version 1.23.12, ProWB
integriert, CI-Doku konsolidiert)

---

## §0 — Versionierungs-Philosophie

ProPhysics verwendet ein **etappen-basiertes** Schema, nicht SemVer.
Die vollständige Definition steht in `docs/project/VERSIONING.md`
und `CHANGELOG.md` §0 und §2.

### §0.1 — Drei Ebenen

| Ebene | Version | Was sie bedeutet |
|---|---|---|
| **Kernel** | `1.23.0` (MAJOR.MINOR.PATCH) | Phase.Etappe.Fix |
| **SDK** | `1.23.0` | folgt Kernel |
| **Module-Docs** | `1.0` pro Dokument | eigenständig; referenziert `Kernel:` |
| **CHANGELOG** | `1.23.12` | Kernel + Konsolidierungs-Patch-Serie |
| **Tests** | `1.0.0` | eigenständig (Test-Suite-Version) |
| **Build / Tools** | `1.0.0` | eigenständig (Infrastruktur-Version) |
| **ProWB / Web-Docs** | `1.0.0` | eigenständig (Web-Infrastruktur-Version) |
| **CI-Workflows** | `1.0.0` | eigenständig (CI-Infrastruktur-Version) |

**Regel:** Nur die Kernel-Version in `ProPhysics_Version.h` ist
**semantisch bindend**. Alle anderen Versionen sind Kommentare.

### §0.2 — Was bedeutet welche Ziffer?

| Ziffer | Bedeutung | Beispiel |
|---|---|---|
| MAJOR | Phase | `1` = Fundament, `2` = komplette QM, `3` = Makrophysik |
| MINOR | Etappen-Nummer | `1`, `2`, …, `23`, `24`, … |
| PATCH | Fix / Konsolidierung | `0`, `1`, `2`, …, `12` |

### §0.3 — Kein `3.0.0` mehr

Die früheren Versionen `1.x` bis `3.0.0` (SemVer-ähnlich) sind
mit `CHANGELOG.md` §2.5 auf das Etappen-Schema abgebildet.
`3.0.0` entspricht jetzt `1.23.0` (Phase 1, Etappe 23).

**Konsequenz für dieses Register:** Die Spalte „Aktuell" aus der
alten Fassung (z.B. `3.1 (Etappe 22)`) existiert nicht mehr —
alle Dateien tragen jetzt `Kernel: 1.23.0` / `Etappe: 23`.

---

## §1 — Kernel-Module (`src/prophysics/`)

**Status:** ✅ vollständig konsolidiert

Alle 12 Module tragen `Kernel: 1.23.0` / `Etappe: 23` und haben
Modul-Docs in `docs/project/`. Keine Etappen-Historie mehr in
Quellcode-Kommentaren; Verweise auf `CHANGELOG.md` und
`docs/project/<Name>.md`.

| Datei | Kernel | Etappe | Konsolidiert mit | Wesentlicher Helfer |
|---|---|---|---|---|
| `ProPhysics_Amp.c` | 1.23.0 | 23 | 1.23.1 | 7 `static`-Helfer (Rotation, Norm, Transport-Cores) |
| `ProPhysics_Core.c` | 1.23.0 | 23 | 1.23.1 | `pro_registers_self_init` |
| `ProPhysics_Density.c` | 1.23.0 | 23 | 1.23.1 + 1.23.10 | `pro_lindblad_2x2`; Cast-Fix C4244 |
| `ProPhysics_Dirac.c` | 1.23.0 | 23 | 1.23.1 | `pro_gamma_op_init` |
| `ProPhysics_EPR.c` | 1.23.0 | 23 | 1.23.2 | 3 `static`-Helfer |
| `ProPhysics_Fock.c` | 1.23.0 | 23 | 1.23.2 | 4 `static`-Helfer |
| `ProPhysics_Gauge.c` | 1.23.0 | 23 | 1.23.3 | `pro_amp_vector_rotate_q16` |
| `ProPhysics_Observer.c` | 1.23.0 | 23 | 1.23.4 | 4 `static`-Helfer |
| `ProPhysics_Shared.c` | 1.23.0 | 23 | 1.23.5 + 1.23.10 | `pro_shared_pair_rotate_q31`; `PRO_NODE_*_MASK` |
| `ProPhysics_SU2.c` | 1.23.0 | 23 | 1.23.6 + 1.23.10 | `pro_su2_axis_angle_to_quat`; Edge-Zugriff nach Internal.h |
| `ProPhysics_SU2_Dynamics.c` | 1.23.0 | 23 | 1.23.7 + 1.23.10 | `su2_leapfrog_kick_E`; Plaquette-Konjugations-Fix |
| `ProPhysics_Tensor.c` | 1.23.0 | 23 | 1.23.8 | 3 `static`-Helfer |

**Soll-Header pro Datei:**
```
* Kernel: 1.23.0
* Etappe: 23
```

**Anmerkung zu 1.23.10:** Der Patch hat `pro_su2_edge` /
`pro_su2_edge_mut` nach `ProPhysics_Internal.h` verschoben
(nachgeholt aus 1.23.7) und den Forward-Plaquette-Konjugations-Bug
in `su2_plaquette_action_at` gefixt.

**Anmerkung zu 1.23.11 und 1.23.12:** Die ProWB- und CI-Patches
haben **keinen** Kernel-Code angefasst. Alle 12 Kernel-Module
bleiben unverändert.

---

## §2 — Kernel-Header (`src/prophysics/header/`)

**Status:** ✅ vollständig konsolidiert

| Datei | Kernel | Etappe | Konsolidiert mit | Anmerkung |
|---|---|---|---|---|
| `ProPhysics.h` | 1.23.0 | 23 | 1.23.1 | Öffentliche API, gruppierte Sektionen |
| `ProPhysics_Config.h` | 1.23.0 | 23 | 1.23.1 + 1.23.10 | `PRO_NODE_*_MASK` eingeführt; `PRO_Q31_HALF_SQRT2` kommentiert |
| `ProPhysics_Exports.h` | 1.23.0 | 23 | 1.23.1 | Build-Modus-Kommentar |
| `ProPhysics_Internal.h` | 1.23.0 | 23 | 1.23.1 + 1.23.7 + 1.23.10 | `pro_su2_edge*` final verschoben; C2-Kommentar präzisiert |
| `ProPhysics_Types.h` | 1.23.0 | 23 | 1.23.1 | Layout-Hinweise (`ProEdge` 40 B) |
| `ProPhysics_Version.h` | 1.23.0 | 23 | — | **Semantischer Anker** |

**Wichtig:** `ProPhysics_Version.h` ist die **einzige** Datei mit
einer aktiven `VERSION_MAJOR`/`VERSION_MINOR`/`VERSION_PATCH`-Trias.
Alle anderen Header tragen `Kernel: 1.23.0` / `Etappe: 23` nur als
Kommentar.

---

## §3 — SDK (`src/sdk/`)

**Status:** ✅ erledigt (Patch 1.23.9)

| Datei | Kernel | Etappe | Anmerkung |
|---|---|---|---|
| `pro_sdk_interface.c` | 1.23.0 | 23 | Wrapper um Kernel-Tick; Refactoring-22-Doku konsolidiert |
| `pro_sdk_interface.h` | 1.23.0 | 23 | Version-Re-Export (`PRO_SDK_VERSION_STRING`, `PRO_SDK_ETAPPE`); Doxygen-`@file`; `PRO_SDK_EXPORTS`-Hinweis |

**Geplant (Sitzung A, §7.1 in TODO):**

- `main()` aus `pro_sdk_interface.c` auslagern → `pro_sdk_runner.c`
- `GRID_DIM` als CLI-Parameter

---

## §4 — Test-Module (`src/test/`)

**Status:** ✅ erledigt (Patch 1.23.9)

### §4.1 — Test-Infrastruktur

| Datei | Kernel | Etappe |
|---|---|---|
| `alpha_test_common.h` | 1.23.0 | 23 |
| `alpha_test_common.c` | 1.23.0 | 23 |

### §4.2 — Test-Module (18)

Alle folgenden Dateien tragen den Kopf
`Kernel: 1.23.0` / `Etappe: 23`:

| Datei | Prio | Anmerkung |
|---|:-:|---|
| `alpha_test_main.c` | — | CLI-Einstieg |
| `alpha_test_basic.c` | 1 | 2D-Basis |
| `alpha_test_born.c` | 1 | Born-Regel |
| `alpha_test_chsh.c` | 2 | CHSH |
| `alpha_test_invariance.c` | 2 | U5-Invariante |
| `alpha_test_soliton.c` | 2 | GP-Soliton |
| `alpha_test_qm_basics.c` | 2 | QM-Grundlagen |
| `alpha_test_qm_advanced.c` | 2 | QM erweitert |
| `alpha_test_qm_emergent.c` | 3 | Emergenz |
| `alpha_test_3d.c` | 4 | 3D-Torus |
| `alpha_test_hydrogen.c` | 5 | Coulomb/Hydrogen |
| `alpha_test_shared.c` | 6 | Shared-Reference |
| `alpha_test_spin.c` | 6 | Spin-1/2 |
| `alpha_test_dirac.c` | 7 | Dirac |
| `alpha_test_su2.c` | 8 | SU(2)-Wilson-Loop |
| `alpha_test_running_coupling.c` | 8 | Running-Coupling |
| `example_test_density.c` | — | Beispiel |
| `example_test_tensor.c` | — | Beispiel |

---

## §5 — Tools (`tools/`)

**Status:** ✅ erledigt (Patch 1.23.9, `web`-Aktion in 1.23.11)

| Datei | Kernel | Etappe | Anmerkung |
|---|---|---|---|
| `pro_run.ps1` | 1.23.0 | 23 | Zentraler Einstiegspunkt (Aktion `web` neu in 1.23.11) |
| `pro_run.cmd` | 1.23.0 | 23 | UTF-8-Wrapper |
| `run_alpha_tests.ps1` | 1.23.0 | 23 | Test-Runner |
| `run_alpha_tests.cmd` | 1.23.0 | 23 | UTF-8-Wrapper |

**Doc:** `docs/test/run_alpha_tests.md` (Version 1.1.0, Etappe 23,
erweitert um CI-Integration in `1.23.12`).

---

## §5b — ProWB-Quellen (`src/prowb/`)

**Status:** ✅ neu in `1.23.11`

| Datei | Version | Kernel | Etappe | Anmerkung |
|---|---|---|---|---|
| `prowb.c` | 1.0.0 | 1.23.0 | 23 | Builder-Kern (Manifest-Parser, Template-Expansion) |
| `header/prowb.h` | 1.0.0 | 1.23.0 | 23 | Öffentliche Builder-API (`prowb_build`, `prowb_build_from_manifest`) |
| `md_parser.c` | 1.0.0 | 1.23.0 | 23 | Eigenständiger GFM-Parser |
| `header/md_parser.h` | 1.0.0 | 1.23.0 | 23 | Parser-API |
| `README.md` | 1.0 | 1.23.0 | 23 | Builder-Übersicht, Manifest-Format, Sektionen, CLI, Fallstricke |

**Soll-Header pro Datei:**
```
* Kernel: 1.23.0
* Etappe: 23
```

**Anmerkung:** ProWB ist **kein** Kernel-Modul. Es ist ein
Standalone-Build-Tool. Die Versionsangabe `Kernel: 1.23.0` ist ein
Kompatibilitäts-Kommentar, kein Abhängigkeits-Ausdruck.

---

## §5c — Web-Docs-Quellen (`docs/web/`)

**Status:** ✅ neu in `1.23.11`, Manifest erweitert in `1.23.12`

| Datei | Version | Kernel | Etappe | Anmerkung |
|---|---|---|---|---|
| `manifest.txt` | 1.0 | 1.23.0 | 23 | 46 Einträge, 6 Sektionen (44 in 1.23.11, +2 CI-Doku in 1.23.12) |
| `README.md` | 1.0 | 1.23.0 | 23 | Manifest-Pflege, Sektionen, Templates, Themes |
| `src/parts/header.html` | 1.0 | 1.23.0 | 23 | Branding, Version, Theme-Umschalter |
| `src/parts/nav.html` | 1.0 | 1.23.0 | 23 | Navigations-Struktur |
| `src/parts/footer.html` | 1.0 | 1.23.0 | 23 | Copyright, Links |
| `src/css/00_reset.css` | 1.0 | 1.23.0 | 23 | CSS-Reset |
| `src/css/00_vars.css` | 1.0 | 1.23.0 | 23 | CSS-Variablen |
| `src/css/10_layout.css` | 1.0 | 1.23.0 | 23 | Layout |
| `src/css/50_prophysics.css` | 1.0 | 1.23.0 | 23 | ProPhysics-spezifische Stile |
| `src/css/99_home.css` | 1.0 | 1.23.0 | 23 | Home-View-Stile |
| `src/css/99_theme_light.css` | 1.0 | 1.23.0 | 23 | Light-Theme |
| `src/css/99_theme_matrix.css` | 1.0 | 1.23.0 | 23 | Matrix-Theme |
| `src/css/99_theme_prophysics.css` | 1.0 | 1.23.0 | 23 | Default-Theme (= `proedc`) |
| `src/js/00_bridge.js` | 1.0 | 1.23.0 | 23 | Daten-Bridge |
| `src/js/10_app.js` | 1.0 | 1.23.0 | 23 | Router, Theme-Umschaltung, `localStorage` |
| `src/views/00_view_home.html` | 1.0 | 1.23.0 | 23 | Hero + Sektions-Karten |
| `src/views/01_view_overview.html` | 1.0 | 1.23.0 | 23 | Overview-Sektion |
| `src/views/02_view_physics.html` | 1.0 | 1.23.0 | 23 | Physics-Sektion |
| `src/views/03_view_modules.html` | 1.0 | 1.23.0 | 23 | Modules-Sektion |
| `src/views/04_view_api.html` | 1.0 | 1.23.0 | 23 | API-Sektion |
| `src/views/05_view_tests.html` | 1.0 | 1.23.0 | 23 | Tests-Sektion |
| `src/views/06_view_build.html` | 1.0 | 1.23.0 | 23 | Build-Sektion |
| `src/views/96_view_lizenz.html` | 1.0 | 1.23.0 | 23 | Lizenz (Dual-Licensing) |
| `src/views/97_view_datenschutz.html` | 1.0 | 1.23.0 | 23 | Datenschutz (DSGVO) |
| `src/views/99_view_settings.html` | 1.0 | 1.23.0 | 23 | Settings (Theme-Umschaltung) |

**Anmerkung:** `docs/web/` enthält **keine** Markdown-Dokumentation.
Es ist reine Konfiguration (Manifest) und Layout (Templates,
Assets, Views). Die MD-Dateien liegen an ihren kanonischen Pfaden
in `docs/project/`, `docs/test/`, `docs/build/`, `docs/physics/`
und im Repo-Root (`README.md`, `CHANGELOG.md`, `TODO.md`,
`CONTRIBUTING.md`, `CONTRIBUTORS.md`, `COMMERCIAL.md`).

**Soll-Header pro MD-Datei (nur `README.md`):**
```
**Datei:** `docs/web/README.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
```

HTML- und CSS/JS-Dateien tragen keinen `Kernel:`/`Etappe:`-Header
(kein Kommentar-Format in HTML üblich, siehe Konvention).

---

## §5d — Website-README (`docs/project/`)

**Status:** ✅ neu in `1.23.12`

| Datei | Version | Kernel | Etappe | Anmerkung |
|---|---|---|---|---|
| `docs/project/README.md` | 1.0 | 1.23.0 | 23 | Website-Einstiegspunkt (Home). Eigenständig vom Root-README. Vorstellung in fünf Fragen (Was/Warum/Wie/Wo/Wer). Vorstellung von BrainAI als Herausgeber. |

**Abgrenzung zum Root-README:**

| Datei | Zweck | Empfänger |
|---|---|---|
| `README.md` (Root) | GitHub-Anzeige, Repository-Info | Entwickler auf GitHub |
| `docs/project/README.md` | Website-Home im ProWB-Portal | Besucher im Web |

Beide Dateien dürfen divergieren. Sie erfüllen unterschiedliche
Zwecke.

---

## §6 — Build-Infrastruktur (`build/`)

**Status:** ✅ erledigt (Patch 1.23.9 + 1.23.11 + 1.23.12)

### §6.1 — Master (`build/main/`)

| Datei | Kernel | Etappe | Anmerkung |
|---|---|---|---|
| `Makefile.nmake` | 1.23.0 | 23 | Target `prowb` ergänzt (1.23.11) |
| `build.ps1` | 1.23.0 | 23 | — |
| `build.cmd` | 1.23.0 | 23 | — |
| `export.ps1` | 1.23.0 | 23 | — |
| `export.cmd` | 1.23.0 | 23 | — |
| `write_build_info.ps1` | 1.23.0 | 23 | — |

### §6.2 — Sub-Makefiles

| Datei | Kernel | Etappe | Anmerkung |
|---|---|---|---|
| `build/prophysics/Makefile.nmake` | 1.23.0 | 23 | **12 Kernel-Module** |
| `build/sdk/Makefile.sdk.nmake` | 1.23.0 | 23 | `check_core`-Vorprüfung |
| `build/test/Makefile.nmake` | 1.23.0 | 23 | **19 `.c`** (17 Alpha + 2 Example) |

**Kritisch geprüft:** `build/prophysics/Makefile.nmake` zählt 12
Module (mit `ProPhysics_SU2_Dynamics.c`). `build/test/Makefile.nmake`
zählt 19 Quellen (mit `alpha_test_running_coupling.c` und den beiden
Example-Tests).

### §6.3 — ProWB-Makefile (`build/prowb/`)

**Status:** ✅ neu in `1.23.11`

| Datei | Kernel | Etappe | Anmerkung |
|---|---|---|---|
| `Makefile.nmake` | 1.23.0 | 23 | Baut `bin\prowb\prowb.exe`. Targets: `all`, `setup`, `help`, `clean`. `CONFIG=release\|debug`. |

**Doc:** `docs/build/prowb/Makefile.md` (Version 1.0.0, Etappe 23).

### §6.4 — CI-Workflows (`.github/workflows/`)

| Datei | Kernel | Etappe | Anmerkung |
|---|---|---|---|
| `ci.yml` | 1.23.0 | 23 | Standard-CI: Prio 1, 6, 7 + `SU2-Wilson-Loop`. Push + PR auf `main`. (~1,5 min) |
| `web-docs.yml` | 1.23.0 | 23 | NEU in 1.23.11. Baut ProWB + Web-Docs, deployt auf GitHub Pages. Pfad-Filter auf `docs/**`, `src/prowb/**`, `build/prowb/**`. (~1 min) |
| `alpha-nightly.yml` | 1.23.0 | 23 | NEU in 1.23.12. Lange Tests (Prio 5 + `Running-Coupling`), **manuell** (`workflow_dispatch`). Scope-Auswahl: `all-long` \| `prio-5` \| `running-coupling`. (~23–64 min) |

**Docs:**

- `docs/build/ci.md` (Version 1.0.0, Etappe 23, NEU in 1.23.12)
- `docs/build/web-docs-ci.md` (Version 1.0.1, Etappe 23, NEU in 1.23.11, erweitert in 1.23.12)
- `docs/build/alpha-nightly.md` (Version 1.0.0, Etappe 23, NEU in 1.23.12)

### §6.5 — CI-Dokumentation (`docs/build/`)

**Status:** ✅ neu in `1.23.12`

| Datei | Doc-Version | Kernel | Etappe | Anmerkung |
|---|---|---|---|---|
| `docs/build/ci.md` | 1.0.0 | 1.23.0 | 23 | Standard-CI (`ci.yml`) |
| `docs/build/alpha-nightly.md` | 1.0.0 | 1.23.0 | 23 | Alpha-Nightly (`alpha-nightly.yml`) |
| `docs/build/web-docs-ci.md` | 1.0.1 | 1.23.0 | 23 | Web-Docs-CI (`web-docs.yml`) |

---

## §7 — Modul-Dokumentation (`docs/project/`)

**Status:** ✅ vollständig

### §7.1 — Modul-Docs

| Datei | Doc-Version | Kernel | Etappe | Fock-Schema §0–§8 | Status |
|---|---|---|---|:-:|---|
| `CONFIG.md` | 1.0 | 1.23.0 | 23 | — | ⚠️ Sonderfall (kein Modul) |
| `Amp.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Core.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Density.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Dirac.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `EPR.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Fock.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Gauge.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Observer.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Shared.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `SU2.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `SU2_Dynamics.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |
| `Tensor.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ |

**Soll-Header pro Modul-Doc:**
```
**Datei:** `docs/project/<Name>.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/ProPhysics_<Name>.c`
**Zweck:** …
```

### §7.2 — Projekt-Docs (nicht Modul)

| Datei | Doc-Version | Kernel | Etappe | Status |
|---|---|---|---|---|
| `Project.md` | 1.0 | 1.23.0 | 23 | ✅ (Layout um ProWB + CI erweitert in 1.23.11/1.23.12) |
| `ProPhysics_API.md` | 1.0 | 1.23.0 | 23 | ✅ |
| `ProPhysics_Differentiators.md` | 1.0 | 1.23.0 | 23 | ✅ |
| `ProPhysics_VersionRegistry.md` | 1.2 | 1.23.0 | 23 | ✅ (diese Datei) |
| `ARCHITECTURE.md` | 1.0 | 1.23.0 | 23 | ✅ |
| `VERSIONING.md` | 1.0 | 1.23.0 | 23 | ✅ (Patch 1.23.9) |
| `SDK_API.md` | 1.0 | 1.23.0 | 23 | ✅ |
| `README.md` (Root) | 1.1 | 1.23.0 | 23 | ✅ (auf `pro_run`-Workflow umgestellt, 1.23.12) |
| `docs/project/README.md` (Website) | 1.0 | 1.23.0 | 23 | ✅ (neu in 1.23.12) |

### §7.3 — Test-Docs

| Datei | Doc-Version | Kernel | Etappe |
|---|---|---|---|
| `docs/test/ProPhysics_Testkatalog.md` | 1.9 | 1.23.0 | 23 |
| `docs/test/run_alpha_tests.md` | 1.1.0 | 1.23.0 | 23 |
| `docs/test/BASELINE.md` | 1.1 | 1.23.0 | 23 |
| `docs/test/WRITING_TESTS.md` | 1.0 | 1.23.0 | 23 |

### §7.4 — Build-Docs

| Datei | Doc-Version | Kernel | Etappe |
|---|---|---|---|
| `docs/build/BUILD_SCRIPT.md` | 1.2 | 1.23.0 | 23 |
| `docs/build/pro_run.md` | 1.1.0 | 1.23.0 | 23 |
| `docs/build/ci.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/alpha-nightly.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/web-docs-ci.md` | 1.0.1 | 1.23.0 | 23 |
| `docs/build/main/Makefile.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/prophysics/Makefile.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/sdk/Makefile.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/test/Makefile.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/prowb/Makefile.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/helper/build.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/helper/export.md` | 1.0.0 | 1.23.0 | 23 |
| `docs/build/helper/write_build_info.md` | 1.0.0 | 1.23.0 | 23 |

### §7.5 — Web-Docs-Docs

| Datei | Doc-Version | Kernel | Etappe |
|---|---|---|---|
| `docs/web/README.md` | 1.0 | 1.23.0 | 23 |
| `src/prowb/README.md` | 1.0 | 1.23.0 | 23 |

---

## §8 — Changelog-Historie

**Aktuelle Changelog-Version:** `1.23.12`

| Version | Datum | Fokus |
|---|---|---|
| `1.23.12` | 2026-09-28 | CI-Dokumentation, Alpha-Nightly, Website-README |
| `1.23.11` | 2026-09-28 | ProWB / Web-Docs Integration |
| `1.23.10` | 2026-09-27 | Bugfix: 1.23.7 nachgeholt (B7); SU(2)-Plaquette-Konjugations-Fix (T16 1,41e-03); 0 Warnungen |
| `1.23.9` | 2026-09-27 | Release-Vorbereitung: SDK, Build-Skripte, Sub-Makefiles, Build-Docs; Lizenz, VERSIONING, Repo-Hygiene |
| `1.23.8` | 2026-09-26 | Modul-Konsolidierung Tensor |
| `1.23.7` | 2026-09-26 | Modul-Konsolidierung SU2_Dynamics; B7 angekündigt |
| `1.23.6` | 2026-09-26 | Modul-Konsolidierung SU2 |
| `1.23.5` | 2026-09-26 | Modul-Konsolidierung Shared |
| `1.23.4` | 2026-09-26 | Modul-Konsolidierung Observer |
| `1.23.3` | 2026-09-26 | Modul-Konsolidierung Gauge |
| `1.23.2` | 2026-09-26 | Modul-Konsolidierung EPR + Fock |
| `1.23.1` | 2026-09-26 | Header + Amp, Core, Density, Dirac |
| `1.23.0` | 2026-09-26 | Etappe 23 (SU(2)-Metropolis, V&V-Anker 0,08 %) |

**Nächster geplanter Eintrag:** `1.24.0` bei Etappe 24.

---

## §9 — Zusammenfassung: Was ist fertig, was ist offen

### §9.1 — Fertig (Stand 2026-09-28)

1. ✅ **12 Kernel-Module** auf `Kernel: 1.23.0` / `Etappe: 23`
2. ✅ **6 Kernel-Header** auf `Kernel: 1.23.0` / `Etappe: 23`
3. ✅ **13 Modul-Docs** (`Amp`, `Core`, `Density`, `Dirac`, `EPR`,
   `Fock`, `Gauge`, `Observer`, `Shared`, `SU2`, `SU2_Dynamics`,
   `Tensor`, `CONFIG`)
4. ✅ **`CHANGELOG.md`** auf `1.23.12` mit vollständiger Patch-Serie
5. ✅ **Backlog B7** (SU(2)-Edge-Zugriff) gelöst (1.23.10)
6. ✅ **Backlog D, B3** (Dirac-Massenterm, Lindblad-2x2) gelöst
7. ✅ **Compiler-Warnungen 0** (Patch 1.23.10)
8. ✅ **Link-Fehler 0** (Patch 1.23.10)
9. ✅ **SDK-Versionierung** (`pro_sdk_interface.h`/`.c`)
10. ✅ **Build-Skripte** (`build.ps1`, `export.ps1`, `pro_run.ps1`)
11. ✅ **Sub-Makefiles** (12 Kernel-Module, 19 Test-Dateien)
12. ✅ **Build-Docs** auf Etappe 23
13. ✅ **Lizenz** (Issue-URL, Zitierweise)
14. ✅ **VERSIONING.md** angelegt
15. ✅ **Repo-Hygiene** (`.gitignore`, `.github/`, CI-Workflow)
16. ✅ **BASELINE.md** und **Beispiel-BUILD_INFO.txt**
17. ✅ **TODO.md** auf v1.8
18. ✅ **ProWB-Builder** (`src/prowb/`, `1.23.11`)
19. ✅ **ProWB-Makefile** (`build/prowb/`, `1.23.11`)
20. ✅ **Web-Docs-Quelle** (`docs/web/`, `1.23.11`, Manifest erweitert in `1.23.12`)
21. ✅ **Web-Docs CI** (`.github/workflows/web-docs.yml`, `1.23.11`, Doku v1.0.1 in `1.23.12`)
22. ✅ **Build-Doku** (`docs/build/prowb/Makefile.md`, `1.23.11`)
23. ✅ **Web-Docs Manifest-Doku** (`docs/web/README.md`, `1.23.11`)
24. ✅ **Web-Docs CI-Doku** (`docs/build/web-docs-ci.md`, `1.23.11`, v1.0.1 in `1.23.12`)
25. ✅ **ProWB-README** (`src/prowb/README.md`, `1.23.11`)
26. ✅ **CI-Dokumentation** (`docs/build/ci.md`, `1.23.12`)
27. ✅ **Alpha-Nightly-Workflow** (`.github/workflows/alpha-nightly.yml`, `1.23.12`)
28. ✅ **Alpha-Nightly-Doku** (`docs/build/alpha-nightly.md`, `1.23.12`)
29. ✅ **Website-README** (`docs/project/README.md`, `1.23.12`)
30. ✅ **CI-Doku-Querverweise** (`web-docs-ci.md`, `BUILD_SCRIPT.md`, `run_alpha_tests.md`, `CONTRIBUTING.md`, `README.md`, `1.23.12`)

### §9.2 — Offen

| Punkt | Priorität | Sektion |
|---|---|---|
| Refactoring B1, B2, B4, B5, B6, B8 | kann | TODO §4.1 |
| Refactoring C1, C4 | kann | TODO §4.2 |
| Web-Docs Phase 2 (Cross-Refs, Suche, i18n) | optional | TODO §5.9 |
| CI Phase 2 (Nightly-Schedule, Prio 2/3/4) | optional | TODO §6.5 |
| SDK-Roadmap | optional | TODO §7 |
| Etappen 24+ | Roadmap | TODO §8 |

### §9.3 — Empfohlene Reihenfolge

1. **Jetzt:** Web-Docs bauen und prüfen (`pro_run web`).
2. **Danach:** Commit + Push auf `main`.
3. **Danach:** CI prüfen (Actions-Tab, zwei Workflows).
4. **Optional:** Alpha-Nightly manuell starten.
5. **Danach:** Tag `v1.23.12` setzen und pushen.
6. **Danach:** Repo ist offiziell release-ready und publizierbar.
7. **Optional:** Refactorings (§4), CI Phase 2 (§6.5), SDK-Roadmap
   (§7), Etappen 24+ (§8).

---

## §10 — Änderungshistorie dieses Dokuments

| Datum | Version | Änderung |
|---|:-:|---|
| 2026-09-25 | 0.9 | Erste Fassung (alte SemVer-Logik, Anker `3.0.0`) |
| 2026-09-26 | 1.0 | Auf Etappen-Schema umgestellt; Kernel `1.23.0`; Konsolidierungs-Serie `1.23.1`–`1.23.8` eingetragen; §7 (Modul-Docs) und §8 (Changelog-Historie) neu; §9 (Status) neu |
| 2026-09-27 | 1.0 | Konsolidierungs-Serie `1.23.9` + `1.23.10` eingetragen; §1 Konsolidierungsspalte SU2/SU2_Dynamics auf `1.23.7 + 1.23.10` aktualisiert; §2 Header-Konsolidierung ergänzt; §9 Status-Liste auf 17 fertige Punkte erweitert, nur Git-Tags offen |
| 2026-09-28 | 1.1 | ProWB-Integration (`1.23.11`): §5b (ProWB-Quellen), §5c (Web-Docs-Quellen), §6.3 (ProWB-Makefile), §6.4 (CI-Workflows), §7.5 (Web-Docs-Docs) neu; §1 Anmerkung zu 1.23.11 ergänzt; §5 Tools-Aktion `web` markiert; §6.1 Master-Makefile um `prowb`-Target ergänzt; §7.4 Build-Docs um ProWB-Einträge und Version-Updates erweitert; §8 Changelog-Historie um `1.23.11` ergänzt; §9 Status-Liste auf 25 fertige Punkte erweitert |
| 2026-09-28 | 1.2 | CI-Doku-Konsolidierung (`1.23.12`): §5c Manifest-Anmerkung (44 → 46 Einträge); §5d (Website-README) neu; §6.4 CI-Workflows um `alpha-nightly.yml` und Doku-Verweise; §6.5 (CI-Dokumentation) neu; §7.2 Projekt-Docs um `docs/project/README.md` (Website); §7.4 Build-Docs um `ci.md` und `alpha-nightly.md`; §8 Changelog-Historie um `1.23.12`; §9 Status-Liste auf 30 fertige Punkte erweitert; §0.1 Versions-Ebenen um CI-Workflows; §0.2 PATCH-Wertebereich bis 12 |

---

**Ende Versions-Register v1.2.**