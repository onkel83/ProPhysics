# ProPhysics SDK Dokumentation

Diese Dokumentation deckt das ProPhysics SDK ab, bestehend aus `pro_sdk_interface.h` und `pro_sdk_interface.c`. Sie beschreibt die erweiterte 2D-Mesh-Simulation, das Plugin-System für plastische Topologien, Metriken, Szenario-Injektionen und CLI-Steuerungen.

---

## 1. Statische Werte & Makros

* **`GRID_DIM`**: `1000` (Ergibt ein 2D-Gitter von $1000 \times 1000 = 1.000.000$ Knoten).


* **`CHANNELS_MAX`**: `8` (Maximale Anzahl von Pointer-Kanälen pro Knoten gemäß Kern-Definition).


* **`PRO_SDK_EXPORT`**: `__declspec(dllexport)` (Windows-spezifisches Makro zum Exportieren von SDK-Funktionen für DLLs).



---

## 2. Konfigurationsstrukturen & Statische Zustände

### `SimulationScenario` (Struktur)

Steuert das experimentelle Setup, Injektionen, Barrieren und Datei-Exporte zur Laufzeit.

* **`pulse_active`** (`bool`): Schaltet den Impuls-Injektor ein oder aus.


* **`pulse_x`**, **`pulse_y`** (`uint32_t`): Koordinaten des Impuls-Startpunkts.


* **`pulse_type`** (`uint8_t`): Zu injizierender Zustand (z. B. `0x05U` für Photon, `0x02U` für Spin-Up, `0x01U` für Positron).


* **`pulse_length`** (`uint32_t`): Anzahl der Ticks, während der ein Puls aktiv ist.


* **`pulse_interval`** (`uint32_t`): Pausenlänge in Ticks zwischen zwei Pulswiederholungen.


* **`pulse_repeats`** (`uint32_t`): Gesamtzahl der Puls-Wiederholungen.


* **`wall_active`** (`bool`): Aktiviert oder deaktiviert die Vakuum-Barriere.


* **`wall_x`**, **`wall_y`**, **`wall_w`**, **`wall_h`** (`uint32_t`): Position ($x, y$) und Dimensionen (Breite $w$, Höhe $h$) der Barriere.


* **`wall_growth_rate`** (`uint32_t`): Wachstumsrate der Barriere ($0 = \text{statisch}$, $N > 0 = \text{wächst alle } N \text{ Ticks um 2 Einheiten in der Höhe}$).


* **`bmp_interval`** (`uint32_t`): Exportiert alle $X$ Ticks ein BMP-Bild als Zeitraffer ($0 = \text{Deaktiviert}$).


* **`total_ticks`** (`uint32_t`): Gesamtdauer der Simulation in Ticks (Default: `30`).


* **`export_final_bmp`** (`bool`): Schaltet den Export eines finalen BMP-Bildes am Simulationsende frei.



---

## 3. Funktionen & Schnittstellen (`pro_sdk_interface.h` & `pro_sdk_interface.c`)

### Typdefinition: `ProPhysics_ScientificRuleCallback`

Die Funktionszeiger-Signatur für benutzerdefinierte physikalische Regeln und Rewiring-Logiken.

```c
typedef void (*ProPhysics_ScientificRuleCallback)(
    uint8_t current_state, uint8_t target_state,
    uint64_t* current_channels, uint64_t* target_channels,
    uint8_t* out_next_state, uint8_t* out_next_target_state,
    uint64_t current_idx, uint64_t total_nodes
);

```

### SDK-Kernfunktionen

* **`ResearchPlugin_DynamicPlasticTopology`**
* **Typ:** Exportierte Plugin-Funktion (`PRO_SDK_EXPORT`).


* **Beschreibung:** Standard-Forschungsplugin für plastische Topologien. Handhabt Photonen-Paarerzeugung/Annihilation, Ising-Spins, magnetische Wechselwirkungen und Hebb'sches Rewiring entlang von Gradienten.




* **`ProPhysics_SDK_Execute_Plastizitaet_Tick`**
* **Parameter:** `ProUniverse* pu`, `ProPhysics_ScientificRuleCallback callback`

* **Beschreibung:** Führt einen vollständigen Simulationsschritt inklusive des plastischen Regel-Callbacks auf dem 2D-Mesh aus, tauscht die Register-Puffer (Double Buffering) aus und aktualisiert den globalen Entropie-/Interaktionsindex.




* **Interne Hilfsfunktionen (Visualisierung & Metriken):**
* `Export_Universe_To_BMP(ProUniverse* pu, const char* filename)`: Konvertiert das 2D-Gitter in ein unkomprimiertes 24-Bit-BMP-Bild mit Farbkodierung für Positronen (Rot), Negatronen (Blau), Photonen (Weiß) und Spins (Gelb/Grün).


* `Render_ASCII_Viewport(ProUniverse* pu)`: Gibt einen $40 \times 40$ Ausschnitt des Zentrums als ASCII-Art in der Konsole aus.


* `Calculate_Topological_Metrics(ProUniverse* pu)`: Berechnet den Anteil nicht-euklidischer Verbindungen (Wurmlöcher/Rewirings) im prozentualen Verhältnis zum Gesamtraum.


* `Measure_Energy_and_Coupling(ProUniverse* pu, uint32_t tick)`: Ermittelt die Erregungsdichte ($\rho_E$) sowie die Kopplungsmetrik ($\alpha_{eff}$).


* `Apply_Scenario_Injections(ProUniverse* pu, uint32_t current_tick)`: Steuert dynamisch Puls-Injektionen und wachsende Vakuum-Barrieren während der Laufzeit.

---

## 4. CLI-Befehlszeilenparameter (`main`)

Das SDK unterstützt direkte Steuerungsargumente über die Kommandozeile beim Start der Executable:

| Parameter | Argumente / Format | Beschreibung |
| --- | --- | --- |
| `--bmp` | *keine* | Speichert am Ende der Simulation das finale Gitter als `simulation_output.bmp`.|
| `--ticks` | `<Anzahl>` | Legt die Gesamtzahl der Simulations-Ticks fest (z. B. `--ticks 100`).|
| `--bmp-interval` | `<Intervall>` | Erstellt im festgelegten Tick-Abstand automatische Zeitraffer-BMPs (`frame_XXXX.bmp`).|
| `--pulse` | `x,y,type,length,interval,repeats` | Injiziert gezielt Signale. Beispiel: `--pulse 100,500,5,5,10,3`.|
| `--wall` | `x,y,w,h` | Erstellt eine statische Vakuum-Sperre (Barriere) im angegebenen Bereich.|
| `--wall-grow` | `<Rate>` | Lässt die Vakuum-Barriere alle $N$ Ticks um 2 Einheiten in der Höhe wachsen.|
