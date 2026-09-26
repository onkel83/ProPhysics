# ProPhysics SDK — API-Referenz

**Dateien:** `src/sdk/pro_sdk_interface.h` + `src/sdk/pro_sdk_interface.c`
**Version:** 1.0
**Stand:** 2026-09-25 (SDK-Version 3.1, Etappe 22 + Refactoring 22)
**Zweck:** Verbindliche Schnittstellenbeschreibung des SDK-Interface.
Baut auf der Kernel-API (`docs/project/ProPhysics_API.md`) auf und ergänzt
sie um Plugin-Callbacks, einen Tick-Wrapper und einen CLI-Runner.

---

## §0 — Wie dieses Dokument zu lesen ist

Dieses Dokument beschreibt **nur** die Symbole aus `pro_sdk_interface.h`.
Die Kernel-API selbst (Funktionen mit Prefix `ProPhysics_`) ist in
`docs/project/ProPhysics_API.md` dokumentiert. Wer das SDK nutzt, braucht
**beide** Dokumente.

**Schichten-Modell:**

```
┌───────────────────────────────────────────────────────────────┐
│  Anwendung / Test-Harness                                     │
│  (nutzt Kernel-API + SDK-API)                                 │
├───────────────────────────────────────────────────────────────┤
│  SDK-Schicht  (pro_sdk_interface.h)                           │
│  - Callback-Typ-Alias                                         │
│  - Beispiel-Plugin                                            │
│  - Tick-Wrapper                                               │
│  - CLI-Runner (main)                                          │
│  - Visualisierung (BMP, ASCII)                                │
├───────────────────────────────────────────────────────────────┤
│  Kernel-Schicht  (ProPhysics.h)                               │
│  - ProUniverse, amp_grid, Topologie, Dynamik                  │
└───────────────────────────────────────────────────────────────┘
```

**Konvention:** Alle Symbole mit Prefix `PRO_SDK_API` sind öffentlich
(dll-exportiert). Alle Symbole mit Prefix `static` in der `.c`-Datei
sind **nur im SDK-Runner** sichtbar und **nicht** Teil der API.

---

## §1 — Übersicht

### §1.1 — Zweck des SDK-Interface

Das SDK-Interface hat **drei** Aufgaben:

| Aufgabe | Symbol | Status |
|---|---|---|
| **Kernel-Zugriff kapseln** | `ProPhysics_SDK_Execute_Plastizitaet_Tick` | stabil |
| **Plugin-Callback-Typ bereitstellen** | `ProPhysics_ScientificRuleCallback` | stabil (Alias) |
| **Beispiel-Plugin + CLI-Runner** | `ResearchPlugin_DynamicPlasticTopology`, `main` | Demo |

Die eigentliche Physik lebt im **Kernel**. Die SDK-Schicht bietet:

- einen **stabilen Tick-Einstiegspunkt** (falls die Kernel-Signatur
  sich ändert),
- einen **Callback-Typ-Alias** für Rückwärtskompatibilität,
- einen **Beispiel-Runner**, der zeigt, wie man den Kernel verwendet.

### §1.2 — Modul-Layout

```
src/sdk/
├── pro_sdk_interface.h    <- öffentliche SDK-API
└── pro_sdk_interface.c    <- Implementierung + CLI-Runner
```

**Build:** Wird von `build/sdk/Makefile.sdk.nmake` gebaut. Erzeugt:
- `bin/pro_sdk_interface.dll`
- `lib/pro_sdk_interface.lib`

**Linkt gegen:** `lib/ProPhysics.lib` (Kernel-Import-Lib).

### §1.3 — Include-Reihenfolge

**Für SDK-Nutzer:**

```c
#include "prophysics/ProPhysics.h"       /* Kernel-API */
#include "sdk/pro_sdk_interface.h"       /* SDK-API */
```

**Reihenfolge nicht optional:** `pro_sdk_interface.h` inkludiert
`ProPhysics.h` bereits. Wer die SDK-Header zuerst inkludiert, bekommt
den Kernel automatisch mit.

---

## §2 — Typen

### §2.1 — `ProPhysics_ScientificRuleCallback`

```c
typedef ProPhysics_RuleCallback ProPhysics_ScientificRuleCallback;
```

**Zweck:** Alias für `ProPhysics_RuleCallback` aus `ProPhysics.h`. Beide
Namen sind strukturell **identisch** — der Compiler sieht denselben Typ.

**Warum der Alias existiert:** Vor dem Refactoring 22 hatte das SDK eine
eigene Callback-Definition. Nach dem Refactoring lebt der Typ im Kernel.
Der Alias erhält bestehenden Code (z.B. alte SDK-Demos), ohne dass
dieser angepasst werden muss.

**Neue Aufrufer** sollten direkt `ProPhysics_RuleCallback` verwenden.

**Vollständige Signatur (identisch zu `ProPhysics_RuleCallback`):**

```c
typedef void (*ProPhysics_RuleCallback)(
    uint8_t current_state,
    uint8_t target_state,
    uint64_t* current_channels,
    uint64_t* target_channels,
    uint8_t* out_next_state,
    uint8_t* out_next_target_state,
    uint64_t current_idx,
    uint64_t total_nodes);
```

**Parameter:**

| Parameter | Typ | Richtung | Bedeutung |
|---|---|---|---|
| `current_state` | `uint8_t` | in | `type_state` des aktuellen Knotens |
| `target_state` | `uint8_t` | in | `type_state` des Ziel-Knotens |
| `current_channels` | `uint64_t*` | in/out | Kanal-Array des aktuellen Knotens (mutierbar) |
| `target_channels` | `uint64_t*` | in/out | Kanal-Array des Ziel-Knotens (mutierbar) |
| `out_next_state` | `uint8_t*` | out | optional: neuer `type_state` (aktuell ignoriert) |
| `out_next_target_state` | `uint8_t*` | out | optional: neuer `type_state` (aktuell ignoriert) |
| `current_idx` | `uint64_t` | in | Index des aktuellen Knotens (Diagnose) |
| `total_nodes` | `uint64_t` | in | Gesamtzahl Knoten (Diagnose) |

**`current_channels` / `target_channels`:** Array der Länge
`CHANNELS_MAX` (16). Der Callback darf die Einträge **in-place
permutieren**. Die Änderungen werden nach dem Callback in `reg_target`
übernommen.

**`out_next_state` / `out_next_target_state`:** Werden aktuell
**ignoriert**. Die `type_state`-Werte werden im nächsten Tick aus
`amp_grid` via Guiding-Gleichung rekonstruiert. Die Parameter existieren
für zukünftige Erweiterungen.

**Aufruf-Kontext:** Der Kernel ruft den Callback **pro Kanal** eines
Knotens mit `type_state != UR_NEUTRAL` auf. Bei einem Universum mit `N`
Knoten und `C` Kanälen sind das bis zu `N · C` Aufrufe pro Tick.

**Beispiel:**

```c
void my_callback(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state,
    uint64_t current_idx, uint64_t total_nodes)
{
    /* Nur zwischen nicht-neutralen Knoten Plastizität erlauben */
    if (current_state == UR_NEUTRAL || target_state == UR_NEUTRAL) return;

    /* Tausche Kanal 0 und 2 */
    uint64_t tmp = current_channels[0];
    current_channels[0] = current_channels[2];
    current_channels[2] = tmp;
}
```

---

## §3 — Funktionen

### §3.1 — `ProPhysics_SDK_Execute_Plastizitaet_Tick`

```c
PRO_SDK_API void ProPhysics_SDK_Execute_Plastizitaet_Tick(
    ProUniverse* pu,
    ProPhysics_RuleCallback callback);
```

**Zweck:** Führt **einen vollständigen Tick** des Universums aus.
Dünner Wrapper um `ProPhysics_Tick`.

**Parameter:**
- `pu` — das Universum. Muss mit `ProPhysics_Initialize` initialisiert
  und mit einer Topologie versehen sein.
- `callback` — optionaler Plastizitäts-Callback. **Darf `NULL` sein.**

**Rückgabe:** keine.

**Nebenwirkungen:** Identisch zu `ProPhysics_Tick` — siehe
`docs/project/ProPhysics_API.md` §4.4.

**Dispatch:** Die Funktion delegiert 1:1 an `ProPhysics_Tick`:

```c
ProPhysics_Tick(pu, (ProPhysics_RuleCallback)callback);
```

Der Cast ist ein **reiner Typhauch** — kein ABI-Unterschied.
`ProPhysics_ScientificRuleCallback` und `ProPhysics_RuleCallback` haben
dieselbe Signatur.

**`callback == NULL`:** Die Topologie bleibt unverändert. Der Rest läuft
**bit-identisch** zu einem No-Op-Callback (R7-konform).

**Warum dieser Wrapper existiert:**

- **Stabilität:** Die SDK-Signatur bleibt gleich, auch wenn der Kernel
  seine Tick-Signatur intern ändert.
- **Rückwärtskompatibilität:** Bestehende SDK-Nutzer (z.B.
  `pro_engine_tick`) müssen ihren Code nicht anpassen.
- **Sichtbarkeit:** SDK-Nutzer, die nur den SDK-Header einbinden,
  bekommen den Tick-Einstiegspunkt ohne die volle Kernel-API.

**Beispiel:**

```c
ProUniverse pu;
ProPhysics_Initialize(&pu, 1000 * 1000);
/* ... Topologie aufbauen ... */

/* Ohne Plastizität */
ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu, NULL);

/* Mit Beispiel-Plugin */
ProPhysics_SDK_Execute_Plastizitaet_Tick(
    &pu, ResearchPlugin_DynamicPlasticTopology);
```

**Wichtig:** Der Wrapper macht **nicht mehr** als `ProPhysics_Tick`.
Wer die volle Kernel-Kontrolle will, ruft direkt `ProPhysics_Tick` auf.

### §3.2 — `ResearchPlugin_DynamicPlasticTopology`

```c
PRO_SDK_API void ResearchPlugin_DynamicPlasticTopology(
    uint8_t current_state,
    uint8_t target_state,
    uint64_t* current_channels,
    uint64_t* target_channels,
    uint8_t* out_next_state,
    uint8_t* out_next_target_state,
    uint64_t current_idx,
    uint64_t total_nodes);
```

**Zweck:** Beispiel-Callback für Topologie-Plastizität. Demonstriert,
wie ein Plugin in den Tick eingreifen kann.

**Signatur:** Identisch zu `ProPhysics_RuleCallback`.

**Verhalten:**

```c
if (current_state != UR_NEUTRAL && target_state != UR_NEUTRAL) {
    const uint64_t tmp = current_channels[0];
    current_channels[0] = target_channels[1];
    target_channels[1] = tmp;
}
```

Wenn **beide** Knoten nicht-neutral sind, werden Kanal 0 des aktuellen
Knotens und Kanal 1 des Ziel-Knotens getauscht. Ansonsten passiert
nichts.

**Physikalische Interpretation:** Der Callback implementiert eine
einfache Topologie-Mutation. Er ist **kein** physikalisches Gesetz — er
ist eine Demonstration des Callback-Mechanismus.

**Verwendung:**

```c
ProPhysics_SDK_Execute_Plastizitaet_Tick(
    &pu, ResearchPlugin_DynamicPlasticTopology);
```

**Nicht geeignet für:** Produktive Physik-Simulationen. Wer echte
Plastizität braucht, schreibt einen eigenen Callback.

---

## §4 — CLI-Runner

Die `.c`-Datei enthält einen **kompletten CLI-Runner** mit `main()`.
Er ist **nicht Teil der API** — wer ihn nutzt, verwendet die SDK-DLL
als eigenständiges Programm.

**Ausführbar:** `bin/pro_sdk_interface.dll` wird von Windows nicht
direkt ausgeführt. Der Runner wird als eigenes EXE gebaut (nicht im
aktuellen Makefile enthalten — er ist als Referenz-Implementierung
gedacht).

### §4.1 — `main`-Funktion

```c
int main(int argc, char* argv[]);
```

**Ablauf:**

1. CLI-Argumente parsen (`Parse_CLI_Args`).
2. `ProUniverse` mit `GRID_DIM × GRID_DIM` Knoten initialisieren.
3. 2D-Torus-Topologie verdrahten.
4. Zufällige Anfangszustände setzen (Hash-basiert).
5. Tick-Schleife:
   - `Apply_Scenario_Injections`
   - `ProPhysics_SDK_Execute_Plastizitaet_Tick` mit
     `ResearchPlugin_DynamicPlasticTopology`
   - Diagnose alle 10 Ticks
   - BMP-Export falls konfiguriert
6. Finale Ausgabe (Metriken, ASCII-Viewport oder BMP).
7. `ProPhysics_Free`.

**GRID_DIM:** Kompiliert als `1000`. Das bedeutet **1 000 000 Knoten** —
also 2 MB `ProAmpVector` + 16 MB `ProRegister` + 40 MB `ProEdge` +
usw. Insgesamt ~80 MB RAM. Der Runner ist auf einem Standard-PC
lauffähig, aber die Tick-Rate ist niedrig.

### §4.2 — CLI-Parameter

| Parameter | Typ | Default | Beschreibung |
|---|---|---|---|
| `--bmp` | Flag | aus | Finale `simulation_output.bmp` exportieren |
| `--ticks N` | `uint32_t` | `30` | Anzahl Ticks |
| `--bmp-interval N` | `uint32_t` | `0` | BMP alle N Ticks (0 = aus) |
| `--pulse x,y,type,len,interval,repeats` | Komma-Liste | aus | Puls-Sequenz |
| `--wall x,y,w,h` | Komma-Liste | aus | Statische Wand |
| `--wall-grow N` | `uint32_t` | `0` | Wand wächst alle N Ticks |

**Beispiele:**

```cmd
:: 30 Ticks, ASCII-Viewport am Ende
pro_sdk_interface.exe

:: 100 Ticks, finale BMP
pro_sdk_interface.exe --ticks 100 --bmp

:: 200 Ticks, BMP alle 20 Ticks
pro_sdk_interface.exe --ticks 200 --bmp-interval 20

:: Mit Puls-Injektion
pro_sdk_interface.exe --ticks 50 --pulse 100,500,5,5,10,3

:: Mit Wand
pro_sdk_interface.exe --ticks 100 --wall 500,200,20,600

:: Mit wachsender Wand
pro_sdk_interface.exe --ticks 100 --wall 500,200,20,600 --wall-grow 10
```

### §4.3 — Interne Funktionen (nicht API)

Diese Funktionen sind **static** und **nicht** Teil der öffentlichen
SDK-API. Sie werden hier nur dokumentiert, weil sie den Runner
verständlich machen.

#### §4.3.1 — `Export_Universe_To_BMP`

```c
static void Export_Universe_To_BMP(ProUniverse* pu, const char* filename);
```

**Zweck:** Exportiert das Universum als 24-Bit-BMP.

**Größe:** `GRID_DIM × GRID_DIM` Pixel (`1000 × 1000`).

**Farbcodierung:**

| `type_state` | Farbe | RGB |
|---|---|---|
| `UR_NEUTRAL` | Dunkelgrau | (10, 10, 15) |
| `UR_POSITRON_CW` | Rot | (255, 50, 50) |
| `UR_NEGATRON_CW` | Blau | (50, 50, 255) |
| `UR_PHOTON` | Weiß | (255, 255, 255) |
| `UR_POSITRON_CCW` | Gelb | (255, 200, 0) |
| `UR_NEGATRON_CCW` | Grün | (0, 255, 100) |

**Dateiformat:** Standard-BMP, 54-Byte-Header + Zeilen-Padding auf
4-Byte-Grenzen.

#### §4.3.2 — `Render_ASCII_Viewport`

```c
static void Render_ASCII_Viewport(ProUniverse* pu);
```

**Zweck:** Gibt einen 40×40-Ausschnitt aus der Mitte als ASCII aus.

**Symbole:**

| Symbol | `type_state` |
|:-:|---|
| `.` | `UR_NEUTRAL` |
| `+` | `UR_POSITRON_CW` |
| `-` | `UR_NEGATRON_CW` |
| `*` | `UR_PHOTON` |
| `^` | `UR_POSITRON_CCW` |
| `v` | `UR_NEGATRON_CCW` |

#### §4.3.3 — `Calculate_Topological_Metrics`

```c
static void Calculate_Topological_Metrics(ProUniverse* pu);
```

**Zweck:** Zählt Kanäle, die **nicht** der initialen 2D-Torus-Verdrahtung
entsprechen. Gibt den Prozentsatz aus.

**Interpretation:** Ein hoher Prozentsatz bedeutet viele
Plastizitäts-Ereignisse.

#### §4.3.4 — `Measure_Energy_and_Coupling`

```c
static void Measure_Energy_and_Coupling(ProUniverse* pu, uint32_t tick);
```

**Zweck:** Gibt Diagnose pro Tick aus:

```
[METRIC] Tick 100 | rho_E = 0.123456 | alpha_eff ~ 12.5 | 
         pos=100 neg=80 prot=8 | interactions=42
```

| Feld | Bedeutung |
|---|---|
| `rho_E` | Anteil nicht-neutraler Knoten |
| `alpha_eff` | Verhältnis `(pos+neg)/photon` |
| `pos` | Anzahl Positronen |
| `neg` | Anzahl Negatronen |
| `prot` | Anzahl Photonen |
| `interactions` | `pu->global_entropy_index` |

**Hinweis:** Diese Metriken sind **Demo-Größen**, keine physikalischen
Observablen im strengen Sinne.

#### §4.3.5 — `Apply_Scenario_Injections`

```c
static void Apply_Scenario_Injections(ProUniverse* pu, uint32_t current_tick);
```

**Zweck:** Wendet konfigurierte Puls- und Wand-Injektionen an.

**Puls-Modus:** Setzt an `(pulse_x, pulse_y)` für `pulse_length` Ticks
den Zustand `pulse_type`, dann `pulse_interval` Ticks Pause. Wiederholt
`pulse_repeats` mal.

**Wand-Modus:** Setzt einen `wall_w × wall_h`-Block bei
`(wall_x, wall_y)` auf `UR_NEUTRAL`. Bei `wall_growth_rate > 0` wächst
die Wand alle N Ticks um 2 Zeilen.

#### §4.3.6 — `Parse_CLI_Args`

```c
static void Parse_CLI_Args(int argc, char* argv[]);
```

**Zweck:** Parst die Kommandozeile in `g_config`.

**`g_config`-Struct:**

```c
typedef struct {
    bool     pulse_active;
    uint32_t pulse_x, pulse_y;
    uint8_t  pulse_type;
    uint32_t pulse_length;
    uint32_t pulse_interval;
    uint32_t pulse_repeats;

    bool     wall_active;
    uint32_t wall_x, wall_y;
    uint32_t wall_w, wall_h;
    uint32_t wall_growth_rate;

    uint32_t bmp_interval;
    uint32_t total_ticks;
    bool     export_final_bmp;
} SimulationScenario;
```

**Default-Werte:**

| Feld | Default |
|---|---|
| `pulse_active` | `false` |
| `pulse_x, pulse_y` | `100, 500` |
| `pulse_type` | `0x05` (Photon) |
| `pulse_length` | `5` |
| `pulse_interval` | `10` |
| `pulse_repeats` | `3` |
| `wall_active` | `false` |
| `wall_x, wall_y` | `500, 200` |
| `wall_w, wall_h` | `20, 600` |
| `wall_growth_rate` | `0` |
| `bmp_interval` | `0` |
| `total_ticks` | `30` |
| `export_final_bmp` | `false` |

---

## §5 — DLL-Export und Linken

### §5.1 — Export-Macro

```c
#ifdef _WIN32
#ifdef PRO_SDK_EXPORTS
#define PRO_SDK_API __declspec(dllexport)
#elif defined(PRO_SDK_DLL_IMPORT)
#define PRO_SDK_API __declspec(dllimport)
#else
#define PRO_SDK_API
#endif
#else
#if __GNUC__ >= 4
#define PRO_SDK_API __attribute__((visibility("default")))
#else
#define PRO_SDK_API
#endif
#endif
```

**Wichtig für den Build:** In `pro_sdk_interface.c` muss
`PRO_SDK_EXPORTS` **vor** dem Include von `pro_sdk_interface.h` gesetzt
werden:

```c
#define PRO_SDK_EXPORTS
#include "pro_sdk_interface.h"
```

Andernfalls meldet MSVC **C2375** („Neudefinition; unterschiedliche
Bindung").

### §5.2 — Wer nutzt welches Macro?

| Nutzer | Macro | Bedeutung |
|---|---|---|
| SDK-DLL-Build | `PRO_SDK_EXPORTS` | `dllexport` |
| SDK-Nutzer (EXE gegen DLL) | `PRO_SDK_DLL_IMPORT` | `dllimport` |
| SDK-Nutzer (statisch) | (keine) | leeres Macro |

**In der Praxis:** Da `pro_sdk_interface.c` die einzige Datei ist, die
`PRO_SDK_EXPORTS` setzt, müssen Nutzer, die die SDK-DLL verwenden,
`PRO_SDK_DLL_IMPORT` in ihren Build-Flags haben. Die meisten
Anwendungen linken jedoch statisch gegen `pro_sdk_interface.lib` — dann
ist **kein** Macro nötig.

### §5.3 — Import-Lib

Der Link gegen die SDK-DLL erfolgt über `lib/pro_sdk_interface.lib`.
Sie wird vom SDK-Makefile erzeugt und in `lib/` abgelegt.

**Link-Reihenfolge:**

```
link.exe ... pro_sdk_interface.lib ProPhysics.lib
```

`pro_sdk_interface.lib` **vor** `ProPhysics.lib`. Der SDK-Wrapper
verwendet Symbole aus dem Kernel, daher muss die Kernel-Lib rechts
stehen.

---

## §6 — Verwendungsszenarien

### §6.1 — Kernel-Direkt (ohne SDK)

Wer nur den Kernel nutzt, braucht die SDK-DLL **nicht**:

```c
#include "ProPhysics.h"

ProUniverse pu;
ProPhysics_Initialize(&pu, 1000 * 1000);
/* Topologie aufbauen */
for (int t = 0; t < 100; ++t) {
    ProPhysics_Tick(&pu, NULL);
}
ProPhysics_Free(&pu);
```

**Vorteil:** Weniger Abhängigkeiten, direkter Zugriff auf alle
Kernel-Funktionen.

**Nachteil:** Tick-Orchestrierung muss der Nutzer selbst verstehen.

### §6.2 — SDK-Tick (mit Wrapper)

Wer die SDK-DLL nutzt, verwendet den Wrapper:

```c
#include "ProPhysics.h"
#include "pro_sdk_interface.h"

ProUniverse pu;
ProPhysics_Initialize(&pu, 1000 * 1000);
/* Topologie aufbauen */

void my_plugin(...) {
    /* eigene Plastizitäts-Regel */
}

for (int t = 0; t < 100; ++t) {
    ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu, my_plugin);
}
ProPhysics_Free(&pu);
```

**Vorteil:** SDK-API ist stabiler als Kernel-API. Zukünftige
Kernel-Änderungen brechen den Code nicht.

**Nachteil:** Ein zusätzlicher Wrapper-Aufruf pro Tick.

### §6.3 — SDK-Runner als Referenz

Wer den Runner als Beispiel nehmen will:

```cmd
pro_sdk_interface.exe --ticks 100 --bmp --pulse 100,500,5,5,10,3
```

Das erzeugt:
- `simulation_output.bmp` (finale Visualisierung)
- Konsolen-Diagnose pro 10 Ticks
- Topologische Metrik am Ende

### §6.4 — SDK in eigenes Projekt einbetten

**Kopiere:**

- `pro_sdk_interface.h`
- `pro_sdk_interface.c` (falls du den CLI-Runner nicht brauchst: nur
  die Regionen 0 und 1 relevant)
- `ProPhysics.h` + alle Kernel-Header
- `lib/ProPhysics.lib` + `lib/pro_sdk_interface.lib`

**Linke gegen:**

```
pro_sdk_interface.lib ProPhysics.lib
```

**Setze Include-Pfade:**

```
/I<prophysics-header> /I<sdk-header>
```

---

## §7 — Was das SDK-Interface nicht tut

- **Keine eigene Physik.** Alle Physik ist im Kernel.
- **Kein Objekt-Management.** `ProUniverse` wird vom Nutzer allokiert.
- **Kein Threading.** Der Callback wird im selben Thread wie der Tick
  aufgerufen.
- **Keine Netzwerk- oder Datei-IO außer BMP.** Kein JSON, kein CSV.
- **Keine Logging-Framework.** Ausgaben gehen direkt auf stdout.
- **Kein Memory-Management für den Nutzer.** `ProUniverse` muss explizit
  initialisiert und freigegeben werden.
- **Kein CLI-Framework.** Der Parser ist Minimal-Handarbeit.
- **Keine Persistenz.** Kein Save/Load.

---

## §8 — Migrationshinweise (Refactoring 22)

### §8.1 — Was sich geändert hat

| Vorher | Nachher |
|---|---|
| SDK hatte eigene Callback-Definition | `ProPhysics_ScientificRuleCallback` ist Alias für `ProPhysics_RuleCallback` |
| Tick-Body lag in `pro_sdk_interface.c` | Tick-Body liegt in `ProPhysics_Core.c` (Kernel) |
| `ProPhysics_SDK_Execute_Plastizitaet_Tick` machte alles selbst | delegiert an `ProPhysics_Tick` |

### §8.2 — Was unverändert bleibt

- **Signatur** von `ProPhysics_SDK_Execute_Plastizitaet_Tick`:
  identisch.
- **Callback-Typ**: strukturell identisch (nur Alias).
- **Verhalten bei `callback == NULL`**: bit-identisch.
- **Symbol-Name**: derselbe.

### §8.3 — Was Nutzer tun müssen

**Nichts.** Bestehender Code, der die SDK-API verwendet, funktioniert
unverändert weiter. Der Cast im Wrapper ist transparent.

**Optional:** Wer den Alias nicht braucht, kann direkt
`ProPhysics_RuleCallback` verwenden.

---

## §9 — Fehlersuche

| Symptom | Ursache | Fix |
|---|---|---|
| `C2375: Neudefinition; unterschiedliche Bindung` | `PRO_SDK_EXPORTS` nicht vor Include gesetzt | in `.c` vor `#include "pro_sdk_interface.h"` setzen |
| `unresolved external symbol ProPhysics_Tick` | Kernel-Lib fehlt in Link-Zeile | `lib/ProPhysics.lib` hinzufügen |
| `unresolved external symbol ProPhysics_SDK_...` | SDK-Lib fehlt | `lib/pro_sdk_interface.lib` hinzufügen |
| Callback wird nicht aufgerufen | `type_state == UR_NEUTRAL` bei beiden Knoten | Callback wird nur bei nicht-neutralen Knoten aufgerufen |
| `--pulse` funktioniert nicht | Format falsch | `--pulse x,y,type,len,interval,repeats` |
| BMP ist schwarz | `GRID_DIM` passt nicht zu `total_nodes` | `ProPhysics_Initialize(&pu, GRID_DIM * GRID_DIM)` |
| `pro_sdk_interface.exe` läuft nicht | Runner nicht als EXE gebaut | `cl.exe` mit `main` bauen, nicht als DLL |

---

## §10 — Versions-Historie dieses Dokuments

| Version | Datum | Änderung |
|---|---|---|
| 1.0 | 2026-09-25 | Erste Fassung, SDK-Version 3.1 |

---

## §11 — Siehe auch

| Thema | Datei |
|---|---|
| Kernel-API | `docs/project/ProPhysics_API.md` |
| Kernel-Header | `src/prophysics/header/ProPhysics.h` |
| SDK-Header | `src/sdk/header/pro_sdk_interface.h` |
| SDK-Implementierung | `src/sdk/pro_sdk_interface.c` |
| SDK-Build | `docs/build/sdk/Makefile.md` |
| Testkatalog | `docs/test/ProPhysics_Testkatalog.md` |
| Projekt-Übersicht | `docs/project/Project.md` |

---

**Ende SDK-API-Referenz v1.0.**