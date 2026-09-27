# ProPhysics — TODO

**Datei:** `TODO.md`
**Version:** 1.2
**Stand:** 2026-09-27 (Kernel-Version 1.23.0, Etappe 23 — Konsolidierung abgeschlossen)
**Zweck:** Zentrales Aufgaben-Register. Zeigt auf einen Blick, was
erledigt ist, was noch offen ist, und in welcher Reihenfolge es
abgearbeitet wird.

**Regeln für dieses Dokument:**

- **Erledigte** Einträge bleiben stehen (mit `[x]`) — sie sind die
  Historie.
- **Offene** Einträge sind mit `[ ]` markiert.
- **Blockiert Release** heißt: solange offen, ist das Repo **nicht**
  öffentlich publizierbar.
- **Sollte** heißt: öffentlich publizierbar, aber mit sichtbarem
  Qualitätsverlust.
- **Kann** heißt: optionale Verbesserung, keine Pflicht.

---

## §0 — Status heute

| Bereich | Status |
|---|---|
| Kernel-Version | **1.23.0** (Phase 1, Etappe 23) |
| Tests | **43/43 PASS** |
| Regression-Anker | Prio 1 (12/12), Prio 6 (1/1), Prio 7 (1/1) — grün |
| Kernel-Dateien auf `Kernel:`/`Etappe:` | ✅ erledigt |
| **Modul-Konsolidierung** | ✅ **abgeschlossen** (Patches 1.23.1–1.23.8, alle 12 Module) |
| `ProPhysics_Version.h` auf 1.23.0 | ✅ erledigt |
| **SDK-Dateien auf 1.23.0** | ✅ erledigt |
| **Build-Skripte auf 1.23.0** | ✅ erledigt |
| **Sub-Makefiles auf Etappe 23** | ✅ erledigt (12 Kernel-Module, 19 Test-Dateien) |
| **Build-Docs auf Etappe 23** | ✅ erledigt |
| Lizenz-Platzhalter | ⚠️ teilweise (Issue-Tracker-URL fehlt) |
| VERSIONING.md | ❌ offen |
| Git-Tags | ❌ offen |

---

## §0.1 — Konsolidierungs-Serie 1.23.1–1.23.8 (✅ abgeschlossen)

**Ziel:** Alle Kernel-Module auf ein einheitliches Schema bringen
(Kernel/Etappe-Kopf, keine Etappen-Historie im Quellcode, Verweis
auf `CHANGELOG.md`, Auslagerung duplizierter Blöcke in `static`-Helfer,
Modul-Referenz in `docs/project/*.md`).

**Regel:** Keine Verhaltensänderung, keine API-Änderung. Verifikation
durch bestehende Test-Suite (`43/43 PASS`, bit-identisch).

### ✅ Abgeschlossen

**Header (Patch 1.23.1):**

- [x] `ProPhysics.h` — Sektionen gruppiert, Etappen-Historie raus
- [x] `ProPhysics_Config.h` — Verweis auf `CONFIG.md`
- [x] `ProPhysics_Exports.h` — Etappe 23, Build-Modus-Kommentar
- [x] `ProPhysics_Internal.h` — Etappe 23, Plattform-Fallback-Tabelle
- [x] `ProPhysics_Types.h` — Etappe 23, Layout-Hinweise

**Module (Patch 1.23.1):**

- [x] `ProPhysics_Amp.c` — 7 `static`-Helfer (Rotation, Norm-Quad, 4/6-Kanal, Auto-Sync, Transport-Cores, Wave-Step-Core)
- [x] `ProPhysics_Core.c` — `pro_registers_self_init`
- [x] `ProPhysics_Density.c` — `pro_density_find_free_slot`, `pro_lindblad_2x2`
- [x] `ProPhysics_Dirac.c` — `pro_gamma_op_init`, Massenterm auf `pro_transport_coeffs`

**Module (Patch 1.23.2):**

- [x] `ProPhysics_EPR.c` — `pro_epr_validate_pair`, `pro_epr_sharp_measure_pair`, `pro_epr_measure_born_collapse`
- [x] `ProPhysics_Fock.c` — `pro_fock_apply_jw_sign`, `ProFockOpKind`, `pro_fock_apply_op`, `pro_fock_anticomm_impl`

**Module (Patch 1.23.3):**

- [x] `ProPhysics_Gauge.c` — `pro_amp_vector_rotate_q16`, `pro_phase_q15_to_q16`

**Module (Patch 1.23.4):**

- [x] `ProPhysics_Observer.c` — `pro_amp_vector_abs2_sum`, `pro_observer_diffusion_pass`, `pro_observer_ping_pong`, `pro_observer_measure_axis`

**Module (Patch 1.23.5):**

- [x] `ProPhysics_Shared.c` — `pro_shared_pair_rotate_q31`, Union-Find-Helfer (`pro_shared_find`, `pro_shared_find_const`), Spin-Flip-Zugriff (`pro_node_spin_flipped`)

**Module (Patch 1.23.6):**

- [x] `ProPhysics_SU2.c` — `pro_su2_axis_angle_to_quat`, `pro_su2_normalize`; Wilson-Loop-Kommentar korrigiert (vorwärts)

**Module (Patch 1.23.7):**

- [x] `ProPhysics_SU2_Dynamics.c` — `su2_leapfrog_kick_E`, `su2_plaquette_action_at` zentral; **Backlog B7** gelöst (`pro_su2_edge*` nach `Internal.h`)

**Module (Patch 1.23.8):**

- [x] `ProPhysics_Tensor.c` — `pro_tensor_cmul_acc`, `pro_tensor_cmul_conj_acc`, `pro_rho_to_real16` zentralisiert

**Modul-Dokumentation (`docs/project/`):**

- [x] `CONFIG.md`
- [x] `Amp.md`
- [x] `Core.md`
- [x] `Density.md`
- [x] `Dirac.md`
- [x] `EPR.md`
- [x] `Fock.md`
- [x] `Gauge.md`
- [x] `Observer.md`
- [x] `Shared.md`
- [x] `SU2.md`
- [x] `SU2_Dynamics.md`
- [x] `Tensor.md`

### ✅ Definition of Done (alle erfüllt)

1. [x] Jede `.c`-Datei im Kernel hat `Kernel: 1.23.0` / `Etappe: 23`
   im Kopf und keine Etappen-Historie mehr.
2. [x] Jedes Modul hat eine `docs/project/<Name>.md`-Referenz.
3. [x] Keine der Konsolidierungen hat die Test-Suite verändert
   (`43/43 PASS`, bit-identische Ergebnisse).
4. [x] `CHANGELOG.md` enthält für jeden Patch einen Eintrag
   (`1.23.1`–`1.23.8`).

---

## §1 — Blockiert Release

### §1.1 — SDK-Versionierung ✅ erledigt

- [x] **`pro_sdk_interface.h`** — Kopf auf `Kernel: 1.23.0` / `Etappe: 23`
- [x] **`pro_sdk_interface.h`** — Version-Re-Export
      (`PRO_SDK_VERSION_STRING`, `PRO_SDK_ETAPPE`)
- [x] **`pro_sdk_interface.h`** — Doxygen-`@file`-Block
- [x] **`pro_sdk_interface.h`** — Doku-Kommentare an beiden
      öffentlichen Symbolen
- [x] **`pro_sdk_interface.h`** — `PRO_SDK_EXPORTS`-Hinweis
- [x] **`pro_sdk_interface.c`** — Kopf auf `Kernel: 1.23.0` /
      `Etappe: 23` (Dokumentation der Refactoring-22-Änderungen)

### §1.2 — Build-Skripte ✅ erledigt

- [x] **`build\main\Makefile.nmake`** — Kopf + Master-Orchestrierung
- [x] **`build\main\build.ps1`** — Wrapper um nmake, `-Config release|debug`
- [x] **`build\main\build.cmd`** — UTF-8-Wrapper
- [x] **`build\main\export.ps1`** — Export-Typen `exe|sdk|kit|all`, ZIP-Erzeugung
- [x] **`build\main\export.cmd`** — UTF-8-Wrapper
- [x] **`build\main\write_build_info.ps1`** — Etappe-Zeile, `-Config`, `-GitStamp`, `-GitNote`
- [x] **`tools\pro_run.ps1`** / **`pro_run.cmd`** — zentraler Einstiegspunkt
- [x] **`tools\run_alpha_tests.ps1`** — `-Prio`, `-Test`, `-DllDir`
- [x] **`write_build_info.ps1`** — `Etappe:`-Zeile

### §1.3 — Sub-Makefiles ✅ erledigt

- [x] **`build\prophysics\Makefile.nmake`** — Kopf + **12 Kernel-Module**
      (Core, Amp, Dirac, SU2, SU2_Dynamics, Gauge, EPR, Observer,
      Tensor, Fock, Density, Shared)
- [x] **`build\sdk\Makefile.sdk.nmake`** — Kopf + `check_core`-Vorprüfung
- [x] **`build\test\Makefile.nmake`** — Kopf + **19 `.c`-Dateien**
      (17 Alpha + 2 Example), `check_deps`-Vorprüfung

### §1.4 — Build-Dokumente inhaltlich auf Etappe 23 ✅ erledigt

- [x] **`docs\build\helper\export.md`** — Etappe 23, Export-Typen inkl. `kit`, ZIP-Erzeugung, Runner `tools\`
- [x] **`docs\build\helper\build.md`** — Etappe 23, `-Config`-Support
- [x] **`docs\build\helper\write_build_info.md`** — Etappe 23, `-Config`, `-Version`, `-OutFile`, Git-Optionen
- [x] **`docs\build\prophysics\Makefile.md`** — 12 Kernel-Module
- [x] **`docs\build\test\Makefile.md`** — 19 `.c`, Prio 1–8
- [x] **`docs\build\sdk\Makefile.md`** — Etappe 23, `check_core`
- [x] **`docs\build\main\Makefile.md`** — Master-Makefile, `rebuild_*`
- [x] **`docs\build\BUILD_SCRIPT.md`** — Übersicht, `pro_run`
- [x] **`docs\build\pro_run.md`** — zentraler Einstiegspunkt
- [x] **`docs\test\run_alpha_tests.md`** — Version 1.0.0, Etappe 23

### §1.5 — Lizenz und Kontakt ⚠️ teilweise erledigt

- [x] **`LICENSE.md`** — Copyright-Inhaber gesetzt
      (`Sascha Alexander Köhne / BrainAI(Inhaber Sascha Alexander Köhne)`)
- [x] **`LICENSE.md`** — Repository-URL gesetzt
      (`https://github.com/onkel83/prophysics`)
- [ ] **`LICENSE.md`** — `[ISSUE-TRACKER-URL]` in §8 ersetzen
- [x] **`COMMERCIAL.md`** — Kontakt-E-Mail gesetzt

### §1.6 — VERSIONING-Konzept ❌ offen

- [ ] **`docs\project\VERSIONING.md`** — Konzept-Dokument
      festschreiben (Etappen-Versionierung, Abgrenzung zu SemVer,
      Major=Phase, Minor=Etappe, Patch=Fix). **Hinweis:** Der Inhalt
      ist in `CHANGELOG.md` §0 und §2 bereits ausführlich dokumentiert;
      die eigene Datei ist optional, aber im Projekt-Doc-Struktur-Plan
      (`Project.md` §1.2) aufgeführt.

### §1.7 — Git-Tags ❌ offen

- [ ] Tag **`v1.23.0`** setzen (Kernel-Version)
- [ ] Tag **`etappe-23`** setzen (Etappen-Markierung)

---

## §2 — Sollte vor Release

### §2.1 — Versionsregister ✅ erledigt

- [x] **`docs\project\ProPhysics_VersionRegistry.md`** — auf neues Schema
      umgestellt (Kernel / Etappe / Doc-Version)
- [x] Versionsregister abarbeiten: keine `3.1`/`3.2`-Reste mehr in
      den referenzierten Dokumenten (§7 dokumentiert die Nachzügler)

### §2.2 — Doc-Versionen vereinheitlichen ⚠️ teilweise

- [x] **`docs\project\Project.md`** — `Version: 1.0` + Kopf
- [x] **`docs\test\ProPhysics_Testkatalog.md`** — `Version: 1.8`,
      `Kernel:` + `Etappe:` (Doc-Version ist eigenständig)
- [x] **`docs\test\run_alpha_tests.md`** — `Version: 1.0.0` + Kopf
- [x] **`docs\build\BUILD_SCRIPT.md`** — `Version: 1.0` + Kopf
- [x] **`docs\project\ProPhysics_Differentiators.md`** — `Version: 1.0`,
      `Kernel:` + `Etappe:`
- [ ] **`README.md`** (Root) — Kopf mit `Version:` / `Kernel:` /
      `Etappe:` ergänzen (aktuell informell: „Version: 1.23.0,
      Stand: 2026-09-25")

### §2.3 — `CHANGELOG.md` umschreiben ✅ erledigt

- [x] Auf neues Versionsschema umstellen (`[1.23.0]`, `[1.22.1]`, …)
- [x] Alte `[3.0.0]` / `[2.9.0]` / … durch neue Versionen ersetzen
- [x] Etappen-Nummern bleiben
- [x] Abgrenzung zu SemVer dokumentieren (§0, §2.2)
- [x] Konsolidierungs-Serie `1.23.1`–`1.23.8` eingetragen
- [ ] Optional: Sammel-Patch `1.23.9` für die Aufräumarbeiten
      (SDK, Build-Skripte, Lizenzen, VERSIONING.md), oder direkt
      `1.24.0` bei Etappe 24

### §2.4 — Kleinere Inkonsistenzen ⚠️ teilweise

- [x] **`docs\build\helper\build.md`** — Verweis auf
      `docs\test\run_alpha_tests.md` korrekt
- [x] **`docs\build\helper\write_build_info.md`** — gleicher Verweis korrekt
- [x] **`ProPhysics_Config.h`** — `PRO_Q31_HALF_SQRT2` in der
      Überschreib-Tabelle dokumentiert (`CONFIG.md` §8.6)

---

## §3 — Kann (Nice-to-have)

### §3.1 — Repo-Hygiene

- [ ] **`.gitignore`** prüfen und ergänzen (`out\`, `.vs\`,
      `.vscode\`, `.idea\`, `bin\logs\`)
- [ ] **`docs\project\CONTRIBUTORS.md`** anlegen (mit dem ersten
      externen Beitrag)
- [ ] **`.github\ISSUE_TEMPLATE\bug_report.md`** anlegen
- [ ] **`.github\ISSUE_TEMPLATE\feature_request.md`** anlegen
- [ ] **`.github\PULL_REQUEST_TEMPLATE.md`** anlegen
- [ ] **`.github\workflows\ci.yml`** anlegen (Prio 1, 6, 7 auf
      jedem Push)

### §3.2 — Beispiel-BUILD_INFO

- [ ] **`docs\build\examples\BUILD_INFO.txt`** — Beispiel-Datei
      mitliefern

### §3.3 — Test-Baseline dokumentieren

- [ ] **`docs\test\BASELINE.md`** — den aktuellen
      43/43-PASS-Lauf mit Rohwerten als Referenz

---

## §4 — Refactoring-Backlog

Code-Verbesserungen. Kein Release-Blocker, aber sinnvoll für
Wartbarkeit.

### §4.1 — Doppelter Code (mechanisch)

- [ ] **B5** — Q31-One (`2147483647.0`) und Q62-One
      (`4611686018427387904.0`) als benannte Konstanten in
      `ProPhysics_Config.h` (>50 Stellen)
- [ ] **B6** — Winkel-Konstanten (`PRO_PI`, `PRO_PI_HALF`,
      `PRO_PI_QUARTER`) einführen (>20 Stellen)
- [ ] **B1** — `pro_complex_matmul_q31`-Helfer für 8×8-Matrix-
      Multiplikation (3 Stellen: `Amp.c`, `Density.c`, `Tensor.c`)
- [x] **D** — `Apply_Dirac_Mass_Term` auf `pro_transport_coeffs`
      umstellen (in `1.23.1` erledigt)
- [x] **B3** — `pro_td_apply_2x2_lindblad` vs `pro_fock_2x2_kern`
      vereinheitlichen (`pro_lindblad_2x2` mit `1.23.1`)
- [ ] **B2** — `pro_td_*` vs `pro_fd_*` in `Density.c` vereinheitlichen
- [ ] **B4** — `pro_round_shift_q30` und `pro_round_shift_q31`
      zusammenführen (Makro oder Parameter)
- [ ] **B8** — `pro_measure_sharp` und `pro_amp_to_lambda`
      Konsistenz (beide `static inline` oder beide `extern`)

### §4.2 — Struktur

- [ ] **C1** — `Apply_Amp_Step` aufteilen in 3–4 Sub-Funktionen
      (aktuell ~200 Zeilen Dispatch + Standard-Pfad)
- [ ] **C2** — `pro_su2_exp_apply` Kommentar vs. Implementierung
      angleichen (sagt „kein div/mod“, nutzt `(double)dt/32768.0`)
- [ ] **C3** — `PRO_Q31_HALF_SQRT2` kommentieren (kurze Erklärung)
- [ ] **C4** — `ProEdge` Layout reorganisieren (Breaking, nur in
      einer Major-Änderung)
- [x] **B7** — SU(2)-Edge-Zugriff vereinheitlichen
      (`pro_su2_edge*` nach `ProPhysics_Internal.h`, `1.23.7`)

### §4.3 — Encoding-Bugs

- [x] **`ProPhysics_Exports.h`** — Encoding-Check (keine Mojibake)

---

## §5 — SDK-Roadmap

Nach §1.1 (Versionierung) sind die SDK-Lücken in drei Sitzungen
gegliedert.

### §5.1 — Sitzung A: SDK-Struktur trennen

- [ ] `main()` aus `pro_sdk_interface.c` auslagern →
      `pro_sdk_runner.c` (eigene `.exe`)
- [ ] `pro_sdk_interface.c` enthält nur Library-Code
- [ ] `build\sdk\Makefile.sdk.nmake` auf zwei Targets erweitern
      (DLL + Runner)
- [ ] `GRID_DIM` als CLI-Parameter (`--dim <N>`)

### §5.2 — Sitzung B: SDK-API ausbauen

- [ ] `ProSDK_Export_BMP(pu, filename)` — public
- [ ] `ProSDK_Render_ASCII(pu, out_buffer, size)` — public
- [ ] `ProSDK_Register_Plugin(pu, plugin, context)` mit Kontext
- [ ] `ProSDK_Setup_2D_Torus(pu, dim)` — Convenience
- [ ] `ProSDK_Setup_3D_Torus(pu, dim)` — Convenience
- [ ] `ProSDK_Check_ABI(pu)` — Versionsvergleich
- [ ] `ProSDK_GetVersion()` — Version zur Laufzeit

### §5.3 — Sitzung C: SDK-Packaging

- [ ] `docs\project\SDK_PACKAGING.md` — Paket-Struktur
- [ ] `prophysics.pc` für pkg-config
- [ ] CMake-Config
- [ ] `export.ps1 sdk -Version <tag>` — Tag im Paketnamen
- [ ] `INSTALL`-Target

---

## §6 — Zukunft (Etappen 24+)

Geplante Entwicklungsphasen. Nicht Release-relevant, aber für die
Roadmap sichtbar.

### §6.1 — Phase 1 abschließen (Etappe 24–27)

- [ ] **Etappe 24** — Euklidisches Pfadintegral
      (`test_path_integral_equivalence`)
- [ ] **Etappe 25** — GHZ / Mermin (`test_ghz_mermin`, Hypergraph)
- [ ] **Etappe 26** — Universalität / T-Gate (`test_universality`)
- [ ] **Etappe 27** — Q61-Migration (`test_q61_drift`, int128)

**Nach Etappe 27:** Kernel-Version springt auf **2.x**.

### §6.2 — Phase 2 (Makrophysik)

- [ ] **Etappe M1** — U4' Bad (`test_thermalization`)
- [ ] **Etappe M2** — U5' Plastizität (`test_gravitational_attraction`)
- [ ] **Etappe M3** — Makrophysik-Konsistenz (`test_classical_limit`)

**Nach M1–M3:** Kernel-Version springt auf **3.x**.

### §6.3 — Optional

- [ ] **Etappe 23b** — Creutz-Ratio (`Wilson_Loop_Average`)
- [ ] **Etappe 18d-B** — Wasserstoff-Revision (adaptive Prep)
- [ ] **Etappe O1** — Cache-Optimierung (`CHANNELS_MAX` 16 → 8,
      SoA-Layout)

---

## §7 — Publikation

- [ ] Preprint-Kandidat 1: „Quaternion-valued edges on a
      signed-permutation lattice" (Ziel: arXiv:hep-lat)
- [ ] Preprint-Kandidat 2: „A computational exploration of quantum
      structures from discrete signed permutations"
      (Ziel: arXiv:quant-ph)
- [ ] Vergleich mit etablierten Lattice-QCD-Werten (nach
      Etappe 23b/24)
- [ ] Größere Gitter (dim ≥ 128)

---

## §8 — Wie dieses Dokument gepflegt wird

**Wann wird es aktualisiert?**

- Nach jedem Release oder jeder Etappe.
- Bei jedem PR, der einen Punkt aus §0.1–§3 abarbeitet.
- Wenn ein neuer Release-Blocker gefunden wird.

**Wer aktualisiert es?**

Der Autor der Änderung, im gleichen PR.

**Format-Regeln:**

- Erledigte Einträge: `- [x]` + kurze Notiz mit Datum.
- Offene Einträge: `- [ ]`.
- Neue Einträge kommen in die passende Kategorie.
- **Kein** Eintrag wird gelöscht — Erledigtes bleibt als Historie.
- **§0.1** wird bei jedem Konsolidierungs-Patch nachgeführt
  (Checkbox umsetzen, neue Module ergänzen).

**Beispiel für einen erledigten Eintrag:**

```markdown
- [x] **`ProPhysics_Version.h`** auf 1.23.0 umgestellt (2026-09-26)
```

**Beispiel für einen Konsolidierungs-Eintrag (§0.1):**

```markdown
- [x] `ProPhysics_EPR.c` — konsolidiert mit `1.23.2` (2026-09-26)
```

---

## §9 — Reihenfolge für die nächsten Sitzungen

**Aktueller Pfad:**

1. ✅ **§0.1 — Konsolidierungs-Serie abgeschlossen** (Patches 1.23.1–1.23.8)
2. ✅ **§1.1 — SDK-Versionierung erledigt**
3. ✅ **§1.2 — Build-Skripte erledigt**
4. ✅ **§1.3 — Sub-Makefiles erledigt**
5. ✅ **§1.4 — Build-Docs erledigt**
6. ⚠️ **§1.5 — Lizenz-URL ergänzen** (kleine Lücke)
7. ❌ **§1.6 — `VERSIONING.md` anlegen** (optional, siehe §1.6)
8. ❌ **§1.7 — Git-Tags setzen** (`v1.23.0`, `etappe-23`)
9. ⚠️ **§2.1 — Versionsregister auf neuen Stand bringen**
   (Doc-Nachzügler; §7 der Registry listet sie)
10. ⚠️ **§2.2 — Doc-Versionen vereinheitlichen**
    (README.md-Kopf, `Testkatalog.md` `Kernel:`/`Etappe:` ergänzen)
11. ✅ **§2.3 — CHANGELOG umgeschrieben**
12. ⚠️ **§2.4 — Kleinere Inkonsistenzen** (weitgehend erledigt)
13. ❌ **§3 — Repo-Hygiene** (kann)
14. ❌ **§4 — Refactoring-Backlog** (kann)
15. ❌ **§5 — SDK-Roadmap** (nach §1.1 abgeschlossen)
16. ❌ **§6 — Etappen 24+** (Roadmap)

**Nach Schritt 8 ist das Repo öffentlich publizierbar.**

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
| Konfiguration | `docs/project/CONFIG.md` |
| Amp-Modul | `docs/project/Amp.md` |
| Core-Modul | `docs/project/Core.md` |
| Density-Modul | `docs/project/Density.md` |
| Dirac-Modul | `docs/project/Dirac.md` |
| EPR-Modul | `docs/project/EPR.md` |
| Fock-Modul | `docs/project/Fock.md` |
| Repository | https://github.com/onkel83/prophysics |

---

**Ende TODO v1.2.**