# ProPhysics Core - Technische Dokumentation (C99 Kern)

Diese Dokumentation beschreibt die Architektur, Funktionen, Konfigurationswerte und statischen Typen des **ProPhysics Core** (Version 2.0.0). ProPhysics ist ein rein topologischer Physik-Kern, der auf Graph-basierten Zeiger-Registern ohne feste Raumkoordinaten (Zero-Coordinate Core) aufbaut.

---

## 1. Statische Werte & Makros (`ProPhysics_Config.h`, `ProPhysics_Version.h`, `ProPhysics_Types.h`)

### 1.1 Netzwerkkonstanten
* `NODE_COUNT` (`1048576ULL` / $2^{20}$): Standard-Standardgittergröße bzw. Knotenanzahl als Zweierpotenz.
* `PRO_NODE_COUNT` / `MAX_NODES`: Legacy-Aliase für `NODE_COUNT`.
* `MAX_SPARSE_TRACKING_NODES` (`64000000ULL`): Maximale Speicher-Schranke für das Pointer-Register im 4-GB-Limit.
* `CHANNELS_MAX` (`8`): Anzahl der maximalen Pointer-Kanäle pro Knoten für komplexe Moleküle und Feld-Verschränkungen.
* `SYMMETRY_CHANNELS`: Legacy-Alias für `CHANNELS_MAX`.

### 1.2 Versions-Token (`ProPhysics_Version.h`)
* `PROPHYSICS_VERSION_MAJOR`: `2`
* `PROPHYSICS_VERSION_MINOR`: `0`
* `PROPHYSICS_VERSION_PATCH`: `0`

### 1.3 Urzustände (`ProUrState` Enum)
* `UR_NEUTRAL` (`0x00`): Neutraler Raum-Knoten.
* `UR_POSITRON_CW` (`0x01`): Positron mit Spin $+1/2$ (Clockwise).
* `UR_POSITRON_CCW` (`0x02`): Positron mit Spin $-1/2$ (Counter-Clockwise).
* `UR_NEGATRON_CW` (`0x03`): Negatron mit Spin $+1/2$.
* `UR_NEGATRON_CCW` (`0x04`): Negatron mit Spin $-1/2$.
* `UR_PHOTON` (`0x05`): Energie-Quant / Welle.

---

## 2. Datenstrukturen (`ProPhysics_Types.h`)

* **`ProNode`** (Struktur):
  * `uint8_t type_state`: Bit-Zustand bzw. Chiralität (`ProUrState`).
  * `uint8_t field_helicity`: Topologische Feld-Ausrichtung ($0 = \text{neutral}$, $>0 = \text{magnetischer Drall}$).
  * `uint8_t momentum_phase`: Lokaler Phasengradient für Impuls / kinetischen Boost (Neu V2.0).
  * `uint8_t reserved_gating`: Reserviert für ältere SDK-Demos.

* **`ProRegister`** (Struktur):
  * `uint64_t channels[CHANNELS_MAX]`: Zeiger-Array auf Nachbar-Knoten im Topologie-Graphen.

* **`ProUniverse`** (Struktur):
  * `uint64_t total_nodes`: Gesamtzahl der Knoten im Universum.
  * `uint64_t dynamic_invariance_target`: Zielwert zur Invarianz-Verifizierung.
  * `ProNode* ur_grid`: Array der Knoten (topologischer Raum).
  * `ProRegister* reg_source`: Quell-Verschränkungs-Register.
  * *Legacy-Altlasten:* `reg_target`, `current_cpu_tick`, `global_entropy_index`.

---

## 3. API-Funktionen (`ProPhysics.c` / `ProPhysics.h`)

### 3.1 Lifecycle & Steuerung
* **`ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count)`**
  * *Beschreibung:* Initialisiert das Universum, alloziert den Graphen (`ur_grid`) sowie die Register (`reg_source`) und setzt Standard-Verschränkungen (Self-Loops).
* **`ProPhysics_Free(ProUniverse* pu)`**
  * *Beschreibung:* Gibt alle alloziierten Speicherbereiche des Universums frei und setzt Pointer auf `NULL`.
* **`ProPhysics_Execute_Tick(ProUniverse* pu)`**
  * *Beschreibung:* Führt einen Simulationsschritt (Tick) aus. Beinhaltet die Phasen: *Field Induction*, *Dynamic Kinematics & Momentum Routing*, *State Propagation & Fusion* sowie *Graph Synchronization*.
* **`ProPhysics_Verify_Invariance(const ProUniverse* pu)`** -> `bool`
  * *Beschreibung:* Überprüft die Erhaltungssätze im Universum über die Summe der gewichteten Zustände.

### 3.2 Topologie & Manipulation
* **`ProPhysics_Link_Nodes(ProUniverse* pu, uint64_t src, uint64_t target, uint8_t channel_idx)`**
  * *Beschreibung:* Verknüpft einen Quellknoten über einen spezifischen Kanal mit einem Zielknoten.
* **`ProPhysics_Unlink_Node(ProUniverse* pu, uint64_t src, uint8_t channel_idx)`**
  * *Beschreibung:* Trennt einen Kanal auf (setzt den Zeiger auf den Quellknoten zurück).
* **`ProPhysics_Inject_Momentum(ProUniverse* pu, uint64_t node_idx, double velocity_ratio)`**
  * *Beschreibung:* Prägt einem Knoten einen lokalen Phasengradienten / kinetischen Boost auf (skaliert auf `uint8_t`).
* **`ProPhysics_Spawn_Body(ProUniverse* pu, uint64_t node_idx, uint8_t state, uint8_t helicity)`**
  * *Beschreibung:* Erzeugt bzw. platziert einen physikalischen Körper direkt auf einem bestimmten Knoten mit definiertem Zustand und Helizität.

---

## 4. Konfigurations-Makros für Export-Gates (`ProPhysics_Exports.h`)
* `PROPHYSICS_API`: Regelt plattformübergreifend (`__declspec(dllexport)` unter Windows bzw. Visibility-Attribute unter GCC/Clang) die Sichtbarkeit der Symbole.