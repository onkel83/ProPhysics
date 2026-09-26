# ProPhysics — Versions-Register

**Zweck:** Zentrale Übersicht aller Versionen in allen Dateien. Dient
der Vereinheitlichung und dem schnellen Auffinden von Inkonsistenzen.

**Stand:** 2026-09-25 (nach Etappe 23)

---

## §0 — Versionierungs-Philosophie

Es gibt **zwei getrennte Versionierungsebenen**:

### Kernel-Version (`ProPhysics_Version.h`)

| Version | Bedeutung | Wann |
|---|---|---|
| **3.0.0** | Kernel vollständig, physikalisch validiert | **jetzt** |
| 3.1.0 | Komplette QM (nach Etappe 24–27) | nach Pfadintegral/GHZ/Universalität/Q61 |
| 3.2.0 | Makrophysik (nach M1–M3) | nach U4'/U5'/klassischer Limes |

**Regel:** Kernel-Version springt **nur** bei einem **physikalischen
Paradigmenwechsel**. Algorithmus-Verbesserungen, neue Tests,
V&V-Anker, read-only Funktionen rechtfertigen **keinen** Sprung.

### Doc-Versionen (pro Dokument)

Jedes Doc hat eine **eigene Versionsnummer**, weil sie unabhängig
fortgeschrieben werden. Doc-Versionen dürfen **nicht** mit der
Kernel-Version kollidieren, um Verwechslungen zu vermeiden.

**Empfehlung:** Doc-Versionen bleiben < 3.0, solange Kernel noch 3.0.0.

---

## §1 — Kernel `.c`-Dateien (`src/prophysics/`)

| Datei | Aktuell | Soll | Anmerkung |
|---|---|---|---|
| `ProPhysics_Amp.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | Refactoring 22 integriert |
| `ProPhysics_Core.c` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** | Kernel-Tick integriert |
| `ProPhysics_Density.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_Dirac.c` | 3.1 (Etappe 21 + 21b) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_EPR.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_Fock.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_Gauge.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_Observer.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_Shared.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |
| `ProPhysics_SU2.c` | (keine Version) | **3.0 (Etappe 23)** | 12. Modul |
| `ProPhysics_SU2_Dynamics.c` | (keine Version, nur „Etappe 22b") | **3.0 (Etappe 23)** | 13. Modul |
| `ProPhysics_Tensor.c` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | stabil |

**Soll-Header pro Datei:**
```
* Version: 3.0 (Etappe 23)
```

---

## §2 — Kernel-Header (`src/prophysics/header/`)

| Datei | Aktuell | Soll | Anmerkung |
|---|---|---|---|
| `ProPhysics.h` | 3.1 (Etappe 22) | **3.0 (Etappe 23)** | Öffentliche API |
| `ProPhysics_Config.h` | 3.2 (Etappe 22b) | **3.0 (Etappe 23)** | Zentrale Konstanten |
| `ProPhysics_Exports.h` | (keine Version) | **3.0 (Etappe 23)** | DLL-Gates |
| `ProPhysics_Internal.h` | 3.2 (Etappe 22b) | **3.0 (Etappe 23)** | Kernel-interne API |
| `ProPhysics_Types.h` | 3.2 (Etappe 22b) | **3.0 (Etappe 23)** | Structs/Enums |
| `ProPhysics_Version.h` | **3.0.0** | **3.0.0** | **Semantischer Anker** |

**Wichtig:** `ProPhysics_Config.h`, `ProPhysics_Internal.h` und
`ProPhysics_Types.h` tragen aktuell **3.2**, obwohl die Kernel-Version
3.0.0 ist. Das ist inkonsistent — die „3.2" war eine irreführende
Etappen-Markierung, keine Version.

---

## §3 — SDK (`src/sdk/`)

| Datei | Aktuell | Soll | Anmerkung |
|---|---|---|---|
| `pro_sdk_interface.c` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** | Wrapper um Kernel |
| `pro_sdk_interface.h` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** | Callback-Alias |

---

## §4 — Test-Module (`src/test/`)

### 4.1 — Mit Header-Version

| Datei | Aktuell | Soll |
|---|---|---|
| `alpha_test_common.h` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** |
| `alpha_test_common.c` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** |

### 4.2 — Ohne Header-Version (nur Etappen-Hinweis)

Alle folgenden Dateien haben **keine explizite „Version:"-Zeile**.
Option: entweder Version hinzufügen oder als „Etappen-modul" belassen.

| Datei | Aktuell | Soll |
|---|---|---|
| `alpha_test_main.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_basic.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_born.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_chsh.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_invariance.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_soliton.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_qm_basics.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_qm_advanced.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_qm_emergent.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_3d.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_hydrogen.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_shared.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_spin.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_dirac.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_su2.c` | (keine) | **3.0 (Etappe 23)** |
| `alpha_test_running_coupling.c` | (keine, **NEU in Etappe 23**) | **3.0 (Etappe 23)** |
| `example_test_density.c` | (keine) | **3.0 (Etappe 23)** |
| `example_test_tensor.c` | (keine) | **3.0 (Etappe 23)** |

---

## §5 — Tools (`tools/`)

| Datei | Aktuell | Soll |
|---|---|---|
| `run_alpha_tests.ps1` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** |
| `run_alpha_tests.cmd` | (keine) | **3.0 (Etappe 23)** |

**Achtung:** `run_alpha_tests.md` (Doc) hat **Doc-Version 3.2** —
das ist korrekt und unabhängig von der Tool-Version.

---

## §6 — Build-Infrastruktur (`build/`)

### 6.1 — Master (`build/main/`)

| Datei | Aktuell | Soll |
|---|---|---|
| `Makefile.nmake` | 3.0 | **3.0** (passt) |
| `build.ps1` | (keine) | **3.0 (Etappe 23)** |
| `build.cmd` | (keine) | **3.0 (Etappe 23)** |
| `export.ps1` | 3.1 (Etappe 22 + Refactoring) | **3.0 (Etappe 23)** |
| `export.cmd` | (keine) | **3.0 (Etappe 23)** |
| `write_build_info.ps1` | (keine) | **3.0 (Etappe 23)** |

### 6.2 — Sub-Makefiles

| Datei | Aktuell | Soll |
|---|---|---|
| `build/prophysics/Makefile.nmake` | 3.1 — Multi-Modul-Kernel (12 .c) | **3.0 (Etappe 23) — 13 .c** |
| `build/sdk/Makefile.sdk.nmake` | 3.0 — SDK-Interface | **3.0** (passt) |
| `build/test/Makefile.nmake` | 3.1 — Alpha-Test + Example | **3.0 (Etappe 23) — 18 Module** |

**Wichtig:** `build/prophysics/Makefile.nmake` sagt aktuell „12 .c-Dateien",
aber es sind **13** (inkl. `ProPhysics_SU2_Dynamics.c`). Das sollte
korrigiert werden.

---

## §7 — Dokumentation (`docs/`)

### 7.1 — Projekt- und Test-Docs

| Datei | Aktuell | Soll | Begründung |
|---|---|---|---|
| `project/Project.md` | 3.0 | **2.10** | Doc-Version < Kernel-Version |
| `test/ProPhysics_Testkatalog.md` | 1.8 | **1.8** (passt) | unabhängiges Schema |
| `test/run_alpha_tests.md` | 3.2 | **1.0 oder 3.2** | eigenes Schema |
| `build/BUILD_SCRIPT.md` | 3.3 | **1.0 oder 3.3** | eigenes Schema |

**Empfehlung:** Entweder alle Docs auf **1.x-Schema** (1.0, 1.1, ...)
umbenennen, oder konsequent **pro Doc** hochzählen. Aktuell ist es
gemischt (`Project.md` bei 3.0, `Testkatalog` bei 1.8).

### 7.2 — Build-Detail-Docs

| Datei | Aktuell | Soll |
|---|---|---|
| `build/main/Makefile.md` | unbekannt | **1.0** |
| `build/prophysics/Makefile.md` | unbekannt | **1.0** |
| `build/sdk/Makefile.md` | unbekannt | **1.0** |
| `build/test/Makefile.md` | unbekannt | **1.0** |
| `build/helper/build.md` | unbekannt | **1.0** |
| `build/helper/export.md` | unbekannt | **1.0** |
| `build/helper/write_build_info.md` | unbekannt | **1.0** |

---

## §8 — Vorgeschlagene Vereinheitlichung

### 8.1 — Drei Kategorien, drei Schemata

| Kategorie | Schema | Beispiel |
|---|---|---|
| **Kernel** | `3.0 (Etappe 23)` für alle `.c`/`.h` | `ProPhysics_Amp.c` |
| **Tests / Tools / SDK** | `3.0 (Etappe 23)` | `alpha_test_main.c` |
| **Dokumentation** | `1.x` (unabhängig) | `Project.md` 1.0 |

**Regel:** Nur die Kernel-Version in `ProPhysics_Version.h` ist
**semantisch bindend**. Alle anderen Versionen sind Kommentare.

### 8.2 — Konkrete Änderungen

**Zu ändern:**

1. **12 Kernel-`.c`-Dateien**: `3.1 (Etappe 22)` → `3.0 (Etappe 23)`
2. **5 Kernel-Header**: `3.1/3.2` → `3.0 (Etappe 23)`
3. **2 SDK-Dateien**: `3.1` → `3.0 (Etappe 23)`
4. **2 Test-Module** (common.h/.c): `3.1` → `3.0 (Etappe 23)`
5. **15 Test-Module ohne Version**: Version hinzufügen
6. **2 Tools**: `3.1` → `3.0 (Etappe 23)`
7. **4 Build-Skripte**: Version hinzufügen
8. **2 Sub-Makefiles**: `3.1` → `3.0 (Etappe 23)`
9. **`build/prophysics/Makefile.nmake`**: „12 .c" → „13 .c"

**Zu belassen:**

- `ProPhysics_Version.h`: `3.0.0` (semantischer Anker)
- `Testkatalog.md`: `1.8`
- Docs-Struktur (mit Ausnahme `Project.md` → 2.10)

**Zu diskutieren:**

- Ob `Project.md` bei `3.0` bleibt oder auf `2.10` geht
- Ob alle Docs ein einheitliches `1.x`-Schema bekommen
- Ob `run_alpha_tests.md` bei `3.2` bleibt oder auf `1.0` geht

---

## §9 — Änderungshistorie dieses Dokuments

| Datum | Änderung |
|---|---|
| 2026-09-25 | Erste Fassung, 43 Dateien erfasst |

---

**Ende Versions-Register.**