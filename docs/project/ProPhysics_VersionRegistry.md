# ProPhysics — Versions-Register

**Datei:** `docs/project/ProPhysics_VersionRegistry.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Zweck:** Zentrale Übersicht aller Versionen in allen Dateien.
Dient der Vereinheitlichung und dem schnellen Auffinden von
Inkonsistenzen.
**Stand:** 2026-09-26 (Kernel 1.23.0, Konsolidierungs-Serie
1.23.1–1.23.8 abgeschlossen)

---

## §0 — Versionierungs-Philosophie

ProPhysics verwendet ein **etappen-basiertes** Schema, nicht SemVer.
Die vollständige Definition steht in `CHANGELOG.md` §0 und
`CHANGELOG.md` §2.

### §0.1 — Drei Ebenen

| Ebene | Version | Was sie bedeutet |
|---|---|---|
| **Kernel** | `1.23.0` (MAJOR.MINOR.PATCH) | Phase.Etappe.Fix |
| **SDK** | `1.23.0` | folgt Kernel |
| **Module-Docs** | `1.0` pro Dokument | eigenständig; referenziert `Kernel:` |
| **CHANGELOG** | `1.23.8` | Kernel + Konsolidierungs-Patch-Serie |
| **Tests** | `1.0.0` | eigenständig (Test-Suite-Version) |
| **Build / Tools** | `1.0.0` | eigenständig (Infrastruktur-Version) |

**Regel:** Nur die Kernel-Version in `ProPhysics_Version.h` ist
**semantisch bindend**. Alle anderen Versionen sind Kommentare.

### §0.2 — Was bedeutet welche Ziffer?

| Ziffer | Bedeutung | Beispiel |
|---|---|---|
| MAJOR | Phase | `1` = Fundament, `2` = komplette QM, `3` = Makrophysik |
| MINOR | Etappen-Nummer | `1`, `2`, …, `23`, `24`, … |
| PATCH | Fix / Konsolidierung | `0`, `1`, `2`, … |

### §0.3 — Kein `3.0.0` mehr

Die früheren Versionen `1.x` bis `3.0.0` (SemVer-ähnlich) sind
mit `CHANGELOG.md` §2.5 auf das Etappen-Schema abgebildet.
`3.0.0` entspricht jetzt `1.23.0` (Phase 1, Etappe 23).

**Konsequenz für dieses Register:** Die Spalte „Aktuell" aus der
alten Fassung (z.B. `3.1 (Etappe 22)`) existiert nicht mehr —
alle Dateien tragen jetzt `Kernel: 1.23.0` / `Etappe: 23`.

---

## §1 — Kernel-Module (`src/prophysics/kernel/`)

**Status:** ✅ vollständig konsolidiert

Alle 12 Module tragen `Kernel: 1.23.0` / `Etappe: 23` und haben
Modul-Docs in `docs/project/`. Keine Etappen-Historie mehr in
Quellcode-Kommentaren; Verweise auf `CHANGELOG.md` und
`docs/project/<Name>.md`.

| Datei | Kernel | Etappe | Konsolidiert mit | Wesentlicher Helfer |
|---|---|---|---|---|
| `ProPhysics_Amp.c` | 1.23.0 | 23 | 1.23.1 | 7 `static`-Helfer (Rotation, Norm, Transport-Cores) |
| `ProPhysics_Core.c` | 1.23.0 | 23 | 1.23.1 | `pro_registers_self_init` |
| `ProPhysics_Density.c` | 1.23.0 | 23 | 1.23.1 | `pro_lindblad_2x2` |
| `ProPhysics_Dirac.c` | 1.23.0 | 23 | 1.23.1 | `pro_gamma_op_init` |
| `ProPhysics_EPR.c` | 1.23.0 | 23 | 1.23.2 | 3 `static`-Helfer |
| `ProPhysics_Fock.c` | 1.23.0 | 23 | 1.23.2 | 4 `static`-Helfer |
| `ProPhysics_Gauge.c` | 1.23.0 | 23 | 1.23.3 | `pro_amp_vector_rotate_q16` |
| `ProPhysics_Observer.c` | 1.23.0 | 23 | 1.23.4 | 4 `static`-Helfer |
| `ProPhysics_Shared.c` | 1.23.0 | 23 | 1.23.5 | `pro_shared_pair_rotate_q31` |
| `ProPhysics_SU2.c` | 1.23.0 | 23 | 1.23.6 | `pro_su2_axis_angle_to_quat` |
| `ProPhysics_SU2_Dynamics.c` | 1.23.0 | 23 | 1.23.7 | `su2_leapfrog_kick_E` (löst B7) |
| `ProPhysics_Tensor.c` | 1.23.0 | 23 | 1.23.8 | 3 `static`-Helfer |

**Soll-Header pro Datei:**
```
* Kernel: 1.23.0
* Etappe: 23
```

---

## §2 — Kernel-Header (`src/prophysics/header/`)

**Status:** ✅ vollständig konsolidiert

| Datei | Kernel | Etappe | Konsolidiert mit | Anmerkung |
|---|---|---|---|---|
| `ProPhysics.h` | 1.23.0 | 23 | 1.23.1 | Öffentliche API, gruppierte Sektionen |
| `ProPhysics_Config.h` | 1.23.0 | 23 | 1.23.1 | Verweis auf `CONFIG.md` |
| `ProPhysics_Exports.h` | 1.23.0 | 23 | 1.23.1 | Build-Modus-Kommentar (dllexport/dllimport/statisch) |
| `ProPhysics_Internal.h` | 1.23.0 | 23 | 1.23.1 + 1.23.7 | `pro_su2_edge*` nach B7-Auflösung |
| `ProPhysics_Types.h` | 1.23.0 | 23 | 1.23.1 | Layout-Hinweise (`ProEdge` 40 B) |
| `ProPhysics_Version.h` | 1.23.0 | 23 | — | **Semantischer Anker** |

**Wichtig:** `ProPhysics_Version.h` ist die **einzige** Datei mit
einer aktiven `VERSION_MAJOR`/`VERSION_MINOR`/`VERSION_PATCH`-Trias.
Alle anderen Header tragen `Kernel: 1.23.0` / `Etappe: 23` nur als
Kommentar. Verweis auf `CHANGELOG.md` folgt mit dem Abschluss-Patch
`1.23.9` (siehe `TODO.md` §0.1).

---

## §3 — SDK (`src/sdk/`)

**Status:** ❌ offen (siehe `TODO.md` §1.1)

| Datei | Aktuell | Soll | Anmerkung |
|---|---|---|---|
| `pro_sdk_interface.c` | (alt, `3.1 (Etappe 22)`) | **`Kernel: 1.23.0` / `Etappe: 23`** | Wrapper um Kernel |
| `pro_sdk_interface.h` | (alt, `3.1 (Etappe 22)`) | **`Kernel: 1.23.0` / `Etappe: 23`** | Callback-Alias, `PRO_SDK_EXPORTS` |

**Zusätzlich geplant** (nicht Version, sondern Inhalt — `TODO.md` §1.1):

- `PRO_SDK_VERSION_STRING`, `PRO_SDK_ETAPPE` Re-Export
- Doxygen-`@file`-Block
- Usage-Beispiel (10 Zeilen)
- Kopfzeile in beiden Dateien

---

## §4 — Test-Module (`src/test/`)

**Status:** ❌ offen (siehe `TODO.md` §1.3)

### §4.1 — Test-Infrastruktur

| Datei | Aktuell | Soll |
|---|---|---|
| `alpha_test_common.h` | (alt, `3.1 (Etappe 22)`) | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `alpha_test_common.c` | (alt, `3.1 (Etappe 22)`) | **`Kernel: 1.23.0` / `Etappe: 23`** |

### §4.2 — Test-Module (18)

Alle folgenden Dateien bekommen den Kopf
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
| `alpha_test_qm_advanced.c` | 3 | QM erweitert |
| `alpha_test_qm_emergent.c` | 3 | Emergenz |
| `alpha_test_3d.c` | 4 | 3D-Torus |
| `alpha_test_hydrogen.c` | 5 | Coulomb/Hydrogen |
| `alpha_test_shared.c` | 6 | Shared-Reference |
| `alpha_test_spin.c` | 6 | Spin-1/2 |
| `alpha_test_dirac.c` | 7 | Dirac |
| `alpha_test_su2.c` | 8 | SU(2)-Wilson-Loop |
| `alpha_test_running_coupling.c` | 8 | **neu in Etappe 23** |
| `example_test_density.c` | — | Beispiel |
| `example_test_tensor.c` | — | Beispiel |

---

## §5 — Tools (`tools/`)

**Status:** ❌ offen (siehe `TODO.md` §1.2)

| Datei | Aktuell | Soll |
|---|---|---|
| `run_alpha_tests.ps1` | (alt, `3.1 (Etappe 22)`) | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `run_alpha_tests.cmd` | (keine) | **`Kernel: 1.23.0` / `Etappe: 23`** |

**Achtung:** `docs/test/run_alpha_tests.md` (Doc) hat
**Doc-Version 3.2**. Nach dem neuen Schema gehört sie auf `1.0`
(siehe §7.3).

---

## §6 — Build-Infrastruktur (`build/`)

**Status:** ❌ offen (siehe `TODO.md` §1.2, §1.3)

### §6.1 — Master (`build/main/`)

| Datei | Aktuell | Soll |
|---|---|---|
| `Makefile.nmake` | `3.0` | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `build.ps1` | (keine) | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `build.cmd` | (keine) | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `export.ps1` | (alt, `3.1 (Etappe 22)`) | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `export.cmd` | (keine) | **`Kernel: 1.23.0` / `Etappe: 23`** |
| `write_build_info.ps1` | (keine) | **`Kernel: 1.23.0` / `Etappe: 23`** + `Etappe:`-Zeile |

### §6.2 — Sub-Makefiles

| Datei | Aktuell | Soll | Anmerkung |
|---|---|---|---|
| `build/prophysics/Makefile.nmake` | `3.1`, **12 `.c`** | **`Kernel: 1.23.0` / `Etappe: 23`, 13 `.c`** | `SU2.c` + `SU2_Dynamics.c` fehlen |
| `build/sdk/Makefile.sdk.nmake` | `3.0` | **`Kernel: 1.23.0` / `Etappe: 23`** | — |
| `build/test/Makefile.nmake` | `3.1`, **17 `.c`** | **`Kernel: 1.23.0` / `Etappe: 23`, 18 `.c`** | `running_coupling.c` fehlt |

**Kritisch:** `build/prophysics/Makefile.nmake` zählt aktuell
12 Module; korrekt sind **13** (mit `ProPhysics_SU2_Dynamics.c`).

---

## §7 — Modul-Dokumentation (`docs/project/`)

**Status:** ⚠️ überwiegend fertig; Nachzügler bei den in `1.23.1`
erstellten Docs (Schema-Prüfung) und den in `1.23.2` erstellten
Docs (`Version:`-Feld)

### §7.1 — Modul-Docs

| Datei | Doc-Version | Kernel | Etappe | Fock-Schema §0–§8 | Status |
|---|---|---|---|:-:|---|
| `CONFIG.md` | 1.0 | 1.23.0 | 23 | — | ⚠️ Sonderfall (kein Modul); Schema prüfen |
| `Amp.md` | 1.0 | 1.23.0 | 23 | ⚠️ | Schema auf §0–§8 prüfen |
| `Core.md` | 1.0 | 1.23.0 | 23 | ⚠️ | dito |
| `Density.md` | 1.0 | 1.23.0 | 23 | ⚠️ | dito |
| `Dirac.md` | 1.0 | 1.23.0 | 23 | ⚠️ | dito |
| `EPR.md` | **1.23.0** | 1.23.0 | 23 | ✅ | ❌ Kopf auf `Version: 1.0` |
| `Fock.md` | **1.23.0** | 1.23.0 | 23 | ✅ | ❌ Kopf auf `Version: 1.0` |
| `Gauge.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ fertig |
| `Observer.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ fertig |
| `Shared.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ fertig |
| `SU2.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ fertig |
| `SU2_Dynamics.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ fertig |
| `Tensor.md` | 1.0 | 1.23.0 | 23 | ✅ | ✅ fertig |

**Soll-Header pro Modul-Doc:**
```
**Datei:** `docs/project/<Name>.md`
**Version:** 1.0
**Kernel:** 1.23.0
**Etappe:** 23
**Quelldatei:** `src/prophysics/kernel/ProPhysics_<Name>.c`
**Zweck:** …
```

### §7.2 — Projekt-Docs (nicht Modul)

| Datei | Aktuell | Soll |
|---|---|---|
| `Project.md` | 3.0 | **1.0** (Doc-Version) + `Kernel: 1.23.0` / `Etappe: 23` |
| `ProPhysics_Differentiators.md` | 1.0 | `Kernel:` / `Etappe:` ergänzen |
| `ProPhysics_VersionRegistry.md` | **1.0** (diese Datei) | ✅ |
| `VERSIONING.md` | — | **neu anlegen** (`TODO.md` §1.6) |

### §7.3 — Test-Docs

| Datei | Aktuell | Soll |
|---|---|---|
| `docs/test/ProPhysics_Testkatalog.md` | 1.8 | `Kernel:` / `Etappe:` ergänzen |
| `docs/test/run_alpha_tests.md` | 3.2 | **1.0** + `Kernel:` / `Etappe:` |

### §7.4 — Build-Docs

| Datei | Aktuell | Soll |
|---|---|---|
| `docs/build/BUILD_SCRIPT.md` | 3.3 | **1.0** + `Kernel:` / `Etappe:` |
| `docs/build/main/Makefile.md` | unbekannt | **1.0** + Etappe 23 |
| `docs/build/prophysics/Makefile.md` | 11 `.c` | **13 `.c`** + Etappe 23 |
| `docs/build/sdk/Makefile.md` | unbekannt | **1.0** + Etappe 23 |
| `docs/build/test/Makefile.md` | 17 `.c` | **18 `.c`** + Etappe 23 |
| `docs/build/helper/build.md` | unbekannt | Etappe 23 + Pfad-Korrektur (`docs/test/run_alpha_tests.md`) |
| `docs/build/helper/export.md` | unbekannt | Etappe 23, 41 → 43 Tests, `bin\` → `tools\` |
| `docs/build/helper/write_build_info.md` | unbekannt | Etappe 23 + Pfad-Korrektur |

---

## §8 — Changelog-Historie

**Aktuelle Changelog-Version:** `1.23.8`

| Version | Datum | Fokus |
|---|---|---|
| `1.23.8` | 2026-09-26 | Modul-Konsolidierung Tensor |
| `1.23.7` | 2026-09-26 | Modul-Konsolidierung SU2_Dynamics; B7 gelöst |
| `1.23.6` | 2026-09-26 | Modul-Konsolidierung SU2 |
| `1.23.5` | 2026-09-26 | Modul-Konsolidierung Shared |
| `1.23.4` | 2026-09-26 | Modul-Konsolidierung Observer |
| `1.23.3` | 2026-09-26 | Modul-Konsolidierung Gauge |
| `1.23.2` | 2026-09-26 | Modul-Konsolidierung EPR + Fock |
| `1.23.1` | 2026-09-26 | Header + Amp, Core, Density, Dirac |
| `1.23.0` | 2026-09-26 | Etappe 23 (SU(2)-Metropolis, V&V-Anker 0,08 %) |

**Nächster geplanter Eintrag:** `1.23.9` als Abschluss-Patch
(`ProPhysics_Version.h`-Verweis + `ProPhysics_VersionRegistry.md`-
Umstellung), oder direkt `1.24.0` bei Etappe 24.

---

## §9 — Zusammenfassung: Was ist fertig, was ist offen

### §9.1 — Fertig (Stand heute)

1. ✅ **12 Kernel-Module** auf `Kernel: 1.23.0` / `Etappe: 23`
2. ✅ **5 Kernel-Header** auf `Kernel: 1.23.0` / `Etappe: 23`
3. ✅ **7 Modul-Docs** im Fock-Schema §0–§8
   (`Gauge`, `Observer`, `Shared`, `SU2`, `SU2_Dynamics`, `Tensor`
   sowie `Fock`/`EPR` aus `1.23.2`)
4. ✅ **`CHANGELOG.md`** auf `1.23.8` mit vollständiger Patch-Serie
5. ✅ **Backlog B7** (SU(2)-Edge-Zugriff) gelöst
6. ✅ **Backlog D, B3** (Dirac-Massenterm, Lindblad-2x2) gelöst

### §9.2 — Offen (Release-Vorbereitung)

7. ❌ **SDK-Versionierung** (`TODO.md` §1.1)
8. ❌ **Build-Skripte** (`TODO.md` §1.2)
9. ❌ **Sub-Makefiles** (`TODO.md` §1.3) — 12 → 13 `.c`, 17 → 18 `.c`
10. ❌ **Build-Docs** inhaltlich auf Etappe 23 (`TODO.md` §1.4)
11. ❌ **Lizenz-Platzhalter** (`TODO.md` §1.5)
12. ❌ **`VERSIONING.md`** — Konzept-Dokument (`TODO.md` §1.6)
13. ❌ **Git-Tags** `v1.23.0`, `etappe-23` (`TODO.md` §1.7)
14. ❌ **Modul-Doc-Nachzügler** (Schema-Prüfung für `Amp.md`,
    `Core.md`, `Density.md`, `Dirac.md`, `CONFIG.md`;
    `Version:`-Kopf für `EPR.md`, `Fock.md`)
15. ❌ **`ProPhysics_Version.h`** — Verweis auf `CHANGELOG.md`

### §9.3 — Empfohlene Reihenfolge

1. **Jetzt:** `ProPhysics_Version.h` + Registry (dieser Patch)
2. **Danach:** `TODO.md` §1.1 (SDK) und §1.2 (Build-Skripte)
3. **Danach:** `TODO.md` §1.3 (Sub-Makefiles) und §1.4 (Build-Docs)
4. **Danach:** `TODO.md` §1.5–§1.7 (Lizenz, VERSIONING.md, Tags)
5. **Danach:** `TODO.md` §2 (Versionsregister, CHANGELOG, Konsistenz)

**Nach Schritt 4 ist das Repo öffentlich publizierbar.**

---

## §10 — Änderungshistorie dieses Dokuments

| Datum | Version | Änderung |
|---|:-:|---|
| 2026-09-25 | 0.9 | Erste Fassung (alte SemVer-Logik, Anker `3.0.0`) |
| 2026-09-26 | 1.0 | Auf Etappen-Schema umgestellt; Kernel `1.23.0`; Konsolidierungs-Serie `1.23.1`–`1.23.8` eingetragen; §7 (Modul-Docs) und §8 (Changelog-Historie) neu; §9 (Status) neu |

---

**Ende Versions-Register v1.0.**