# ProPhysics — TODO

**Datei:** `TODO.md`
**Version:** 1.1
**Stand:** 2026-09-26 (Kernel-Version 1.23.0, Etappe 23 — Konsolidierung läuft)
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
| **Modul-Konsolidierung** | 🔄 **läuft** — EPR + Fock fertig (siehe §0.1) |
| `ProPhysics_Version.h` auf 1.23.0 | ✅ erledigt (Verweis auf CHANGELOG offen, §0.1) |
| SDK-Dateien auf 1.23.0 | ❌ offen |
| Build-Skripte auf 1.23.0 | ❌ offen |
| Sub-Makefiles auf Etappe 23 | ❌ offen |
| Build-Docs auf Etappe 23 | ❌ offen |
| Lizenz-Platzhalter | ❌ offen |
| Git-Tags | ❌ offen |

---

## §0.1 — Aktueller Fokus: Modul-Konsolidierung (Etappe 23, Patch-Serie)

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

**Modul-Dokumentation (`docs/project/`):**

- [x] `CONFIG.md`
- [x] `Amp.md`
- [x] `Core.md`
- [x] `Density.md`
- [x] `Dirac.md`
- [x] `EPR.md`
- [x] `Fock.md`

### 🔄 Offen — Reihenfolge für die nächsten Sitzungen

**Sitzung 1 (direkt als Nächstes):**

- [ ] **`ProPhysics_Gauge.c`** — konsolidieren
  - [ ] Kopf auf `Kernel: 1.23.0` / `Etappe: 23`, Etappen-Historie raus, Verweis auf `CHANGELOG.md`
  - [ ] Duplizierte Blöcke in `static`-Helfer auslagern (prüfen: Wilson-Loop-Berechnung, Lambda-Feld-Aufbau, Gauge-Transformation)
  - [ ] Modul-Referenz `docs/project/Gauge.md` anlegen
  - [ ] Tests grün halten (`43/43 PASS`, bit-identisch)

- [ ] **`ProPhysics_Observer.c`** — konsolidieren
  - [ ] Kopf auf `Kernel: 1.23.0` / `Etappe: 23`
  - [ ] `static`-Helfer: Diffusion, chaotische Quelle, Dephasing-Tick (prüfen auf gemeinsame Sequenzen)
  - [ ] Modul-Referenz `docs/project/Observer.md` anlegen

**Sitzung 2:**

- [ ] **`ProPhysics_Shared.c`** — konsolidieren
  - [ ] Kopf auf `Kernel: 1.23.0` / `Etappe: 23`
  - [ ] Union-Find-Helfer (`find`/`union`) und Sync-Pfad auf gemeinsame `static`-Kerne
  - [ ] Modul-Referenz `docs/project/Shared.md` anlegen

- [ ] **`ProPhysics_Tensor.c`** — konsolidieren
  - [ ] Kopf auf `Kernel: 1.23.0` / `Etappe: 23`
  - [ ] `pro_complex_matmul_q31`-Helfer hier einführen (löst Backlog **B1** teilweise, siehe §4.1)
  - [ ] Gate-Anwendungen (Hadamard/X/Y/Z/CNOT/CZ/SWAP/√SWAP) auf gemeinsamen Kern prüfen
  - [ ] Modul-Referenz `docs/project/Tensor.md` anlegen

**Sitzung 3:**

- [ ] **`ProPhysics_SU2.c`** — konsolidieren
  - [ ] Kopf auf `Kernel: 1.23.0` / `Etappe: 23`
  - [ ] Edge-Zugriff vereinheitlichen → löst Backlog **B7** (siehe §4.2)
  - [ ] Modul-Referenz `docs/project/SU2.md` anlegen

- [ ] **`ProPhysics_SU2_Dynamics.c`** — konsolidieren
  - [ ] Kopf auf `Kernel: 1.23.0` / `Etappe: 23`
  - [ ] `su2_read_link` an `pro_su2_edge*` angleichen (siehe **B7**)
  - [ ] Modul-Referenz `docs/project/SU2_Dynamics.md` anlegen

**Abschluss der Konsolidierungs-Serie:**

- [ ] **`ProPhysics_Version.h`** — kurzer Verweis auf `CHANGELOG.md`
      (analog zu den anderen Headern; Kernel-Version bleibt `1.23.0`)
- [ ] **`docs/project/ProPhysics_VersionRegistry.md`** — auf neues
      Schema umstellen (nur `Kernel:` / `Etappe:` / Doc-Version,
      keine eigenen Versionvorschläge)
- [ ] **`CHANGELOG.md`** — bei nächstem Patch (z. B. `1.23.3`) die
      Restsitzungen bündeln; siehe §2.3
- [ ] Regression prüfen: `43/43 PASS` unverändert
- [ ] Alle neuen Modul-Docs in §10 („Siehe auch") verlinken

**Definition of Done für diese Serie:**

1. Jede `.c`-Datei im Kernel hat `Kernel: 1.23.0` / `Etappe: 23`
   im Kopf und keine Etappen-Historie mehr.
2. Jedes Modul hat eine `docs/project/<Name>.md`-Referenz.
3. Keine der Konsolidierungen hat die Test-Suite verändert
   (`43/43 PASS`, bit-identische Ergebnisse).
4. `CHANGELOG.md` enthält für jeden Patch einen Eintrag.

---

## §1 — Blockiert Release

Solange diese Punkte offen sind, ist das Repo **nicht** sauber
publizierbar.

### §1.1 — SDK-Versionierung

- [ ] **`pro_sdk_interface.h`** — Kopf auf `Kernel: 1.23.0` /
      `Etappe: 23`
- [ ] **`pro_sdk_interface.h`** — Version-Re-Export
      (`PRO_SDK_VERSION_STRING`, `PRO_SDK_ETAPPE`)
- [ ] **`pro_sdk_interface.h`** — Doxygen-`@file`-Block
- [ ] **`pro_sdk_interface.h`** — Doku-Kommentare an beiden
      öffentlichen Symbolen
- [ ] **`pro_sdk_interface.h`** — Usage-Beispiel (10 Zeilen)
- [ ] **`pro_sdk_interface.h`** — `PRO_SDK_EXPORTS`-Hinweis
- [ ] **`pro_sdk_interface.c`** — Kopf auf `Kernel: 1.23.0` /
      `Etappe: 23`

### §1.2 — Build-Skripte

- [ ] **`build\main\Makefile.nmake`** — Kopf auf `Kernel:` / `Etappe:`
- [ ] **`build\main\build.ps1`** — Kopf
- [ ] **`build\main\build.cmd`** — Kopf
- [ ] **`build\main\export.ps1`** — Kopf
- [ ] **`build\main\export.cmd`** — Kopf
- [ ] **`build\main\write_build_info.ps1`** — Kopf
- [ ] **`write_build_info.ps1`** — um `Etappe:`-Zeile erweitern

### §1.3 — Sub-Makefiles

- [ ] **`build\prophysics\Makefile.nmake`** — Kopf + **11 → 13
      `.c`-Dateien** (SU2.c und SU2_Dynamics.c fehlen)
- [ ] **`build\sdk\Makefile.sdk.nmake`** — Kopf
- [ ] **`build\test\Makefile.nmake`** — Kopf + **17 → 18
      `.c`-Dateien** (running_coupling.c fehlt)

### §1.4 — Build-Dokumente inhaltlich auf Etappe 23

- [ ] **`docs\build\helper\export.md`** — Etappe 21 → 23,
      41 → 43 Tests, Runner `bin\` → `tools\`
- [ ] **`docs\build\helper\build.md`** — Etappe 21 → 23
- [ ] **`docs\build\helper\write_build_info.md`** — Etappe 21 → 23,
      Verweis auf `docs\build\helper\run_alpha_tests.md` →
      `docs\test\run_alpha_tests.md`
- [ ] **`docs\build\prophysics\Makefile.md`** — 11 → 13 `.c`
- [ ] **`docs\build\test\Makefile.md`** — 17 → 18 `.c`,
      41 → 43 Tests, Prio 1–7 → 1–8
- [ ] **`docs\build\sdk\Makefile.md`** — Etappe 21 → 23

### §1.5 — Lizenz und Kontakt

- [ ] **`LICENSE.md`** — `[DEIN NAME / ORGANISATION]` ersetzen
- [ ] **`LICENSE.md`** — `[ISSUE-TRACKER-URL]` ersetzen
- [ ] **`LICENSE.md`** — Repository-URL ersetzen
- [ ] **`COMMERCIAL.md`** — Repository-/Issue-URLs ersetzen

### §1.6 — VERSIONING-Konzept

- [ ] **`docs\project\VERSIONING.md`** — Konzept-Dokument
      festschreiben (Etappen-Versionierung, Abgrenzung zu SemVer,
      Major=Phase, Minor=Etappe, Patch=Fix)

### §1.7 — Git-Tags

- [ ] Tag **`v1.23.0`** setzen (Kernel-Version)
- [ ] Tag **`etappe-23`** setzen (Etappen-Markierung)

---

## §2 — Sollte vor Release

Nicht blockierend, aber sichtbar für den Leser.

### §2.1 — Versionsregister

- [ ] **`docs\project\ProPhysics_VersionRegistry.md`** — auf neues
      Schema umstellen (nur `Kernel:` / `Etappe:` / Doc-Version,
      keine eigenen Versionvorschläge mehr) *(siehe §0.1, Abschluss)*
- [ ] Versionsregister abarbeiten: alle `3.1`/`3.2`-Reste in den
      Dokumenten entfernen

### §2.2 — Doc-Versionen vereinheitlichen

Nach neuem Schema: Doc-Version auf `1.x`, zusätzlich `Kernel:` und
`Etappe:`.

- [ ] **`docs\project\Project.md`** — `Version: 3.0` → `1.0` + Kopf
- [ ] **`docs\test\ProPhysics_Testkatalog.md`** — `1.8` bleibt,
      `Kernel:` + `Etappe:` ergänzen
- [ ] **`docs\test\run_alpha_tests.md`** — `3.2` → `1.0` + Kopf
- [ ] **`docs\build\BUILD_SCRIPT.md`** — `3.3` → `1.0` + Kopf
- [ ] **`docs\project\ProPhysics_Differentiators.md`** — `Kernel:` +
      `Etappe:` ergänzen
- [ ] **`README.md`** — Kopf mit `Version:` / `Kernel:` / `Etappe:`
      ergänzen

### §2.3 — `CHANGELOG.md` umschreiben

- [x] Auf neues Versionsschema umstellen
      (`[1.23.0]`, `[1.22.1]`, `[1.22.0]`, …) — erledigt mit
      `1.23.1`/`1.23.2`
- [x] Alte `[3.0.0]` / `[2.9.0]` / … durch neue Versionen ersetzen
- [x] Etappen-Nummern bleiben
- [x] Abgrenzung zu SemVer dokumentieren (§0, §2.2)
- [ ] Bei Abschluss der Konsolidierungs-Serie (§0.1) einen
      Sammel-Patch `1.23.3` (oder direkt `1.24.0` bei Etappe 24)
      eintragen

### §2.4 — Kleinere Inkonsistenzen

- [ ] **`docs\build\helper\build.md`** — Verweis auf
      `docs\build\helper\run_alpha_tests.md` korrigieren
      (richtig: `docs\test\run_alpha_tests.md`)
- [ ] **`docs\build\helper\write_build_info.md`** — gleicher Verweis
- [ ] **`ProPhysics_Config.h`** — `PRO_Q31_HALF_SQRT2` kommentieren
      (wie andere Konstanten)

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
      *(wird teilweise durch §0.1 Sitzung 2 erledigt)*
- [x] **D** — `Apply_Dirac_Mass_Term` auf `pro_transport_coeffs`
      umstellen (2026-09-26, mit `1.23.1`)
- [x] **B3** — `pro_td_apply_2x2_lindblad` vs `pro_fock_2x2_kern`
      vereinheitlichen (2026-09-26, `pro_lindblad_2x2` mit
      `1.23.1`)
- [ ] **B2** — `pro_td_*` vs `pro_fd_*` in `Density.c` vereinheitlichen
- [ ] **B4** — `pro_round_shift_q30` und `pro_round_shift_q31`
      zusammenführen (Makro oder Parameter)
- [ ] **B8** — `pro_measure_sharp` und `pro_amp_to_lambda`
      Konsistenz (beide `static inline` oder beide `extern`)

### §4.2 — Struktur

- [ ] **C1** — `Apply_Amp_Step` aufteilen in 3–4 Sub-Funktionen
      (aktuell ~200 Zeilen)
- [ ] **C2** — `pro_su2_exp_apply` Kommentar vs. Implementierung
      angleichen (sagt „kein div/mod", nutzt `(double)dt/32768.0`)
- [ ] **C3** — `PRO_Q31_HALF_SQRT2` kommentieren
- [ ] **C4** — `ProEdge` Layout reorganisieren (Breaking, nur in
      einer Major-Änderung)
- [ ] **B7** — SU(2)-Edge-Zugriff vereinheitlichen
      (`pro_su2_edge*` in `SU2.c` vs `su2_read_link` in
      `SU2_Dynamics.c`) *(wird durch §0.1 Sitzung 3 erledigt)*

### §4.3 — Encoding-Bugs

- [ ] **`ProPhysics_Exports.h`** — Encoding-Check (Mojibake in
      Kommentaren wurde behoben, aber Datei mit UTF-8 ohne BOM
      prüfen)

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

**Aktueller Pfad (Konsolidierung abschließen):**

1. **Jetzt:** §0.1 Sitzung 1 — `ProPhysics_Gauge.c`,
   `ProPhysics_Observer.c` (+ Modul-Docs `Gauge.md`, `Observer.md`)
2. **Danach:** §0.1 Sitzung 2 — `ProPhysics_Shared.c`,
   `ProPhysics_Tensor.c` (+ `Shared.md`, `Tensor.md`)
3. **Danach:** §0.1 Sitzung 3 — `ProPhysics_SU2.c`,
   `ProPhysics_SU2_Dynamics.c` (+ `SU2.md`, `SU2_Dynamics.md`)
4. **Abschluss:** `ProPhysics_Version.h` (CHANGELOG-Verweis),
   `ProPhysics_VersionRegistry.md` (neues Schema),
   `CHANGELOG.md`-Eintrag `1.23.3`, Regression prüfen

**Danach (Release-Vorbereitung):**

5. §1.1 (SDK-Versionierung) und §1.2 (Build-Skripte)
6. §1.3 (Sub-Makefiles) und §1.4 (Build-Docs)
7. §1.5 (Lizenz), §1.6 (VERSIONING.md), §1.7 (Tags)
8. §2 (Versionsregister, CHANGELOG, Konsistenz)
9. §3 (Repo-Hygiene)
10. §4 (Refactoring), §5 (SDK-Roadmap), §6 (Etappen 24+)

**Nach Schritt 3 ist die Konsolidierungs-Serie abgeschlossen.**
**Nach Schritt 7 ist das Repo öffentlich publizierbar.**

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

**Ende TODO v1.1.**