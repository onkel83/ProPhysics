\# ProPhysics API-Dokumentation



\*\*Version:\*\* 1.2.0 (Core Engine) / 1.0.0 (UI Engine)  

\*\*Architektur:\*\* C99, Lock-Free Gather (Pull) Engine, Zero-Branching Torus \& Asynchrones Win32 ASCII UI  



Diese Dokumentation beschreibt die Programmierschnittstellen (APIs), Datenstrukturen und Konfigurationsaxiome des ProPhysics-Simulationsframeworks. Das Gesamtsystem ist modular aufgebaut und gliedert sich in den physikalischen Simulationskern (\*\*ProPhysics\*\*) und das asynchrone Visualisierungs-Dashboard (\*\*ProDiBatch\*\*).



\---



\## Inhaltsverzeichnis

1\. \[Core Simulation Engine (ProPhysics)](#1-core-simulation-engine-prophysics)

&#x20;  - \[Konfigurations-Axiome \& Makros](#konfigurations-axiome--makros)

&#x20;  - \[Zentrale Datentypen \& Strukturen](#zentrale-datentypen--strukturen)

&#x20;  - \[Exportierte API-Funktionen](#exportierte-api-funktionen)

2\. \[UI \& Dashboard Engine (ProDiBatch)](#2-ui--dashboard-engine-prodibatch)

&#x20;  - \[Konfiguration \& Grenzen](#konfiguration--grenzen)

&#x20;  - \[Funktions-Schnittstellen (Callbacks)](#funktions-schnittstellen-callbacks)

&#x20;  - \[Exportierte API-Funktionen](#exportierte-api-funktionen-1)

3\. \[Architektur- \& Integrationsleitfaden](#3-architektur---integrationsleitfaden)



\---



\## 1. Core Simulation Engine (ProPhysics)



Der Simulationskern basiert auf einem Face-Centered Cubic (FCC) Lattice-Modell („It from Bit“-Ansatz). Er implementiert ein hochoptimiertes Gitter-Netzwerk im RAM zur parallelen Evolution von Quantenflüssen und trägen Massen.



\### Konfigurations-Axiome \& Makros

Definiert in `ProPhysics\_Config.h` und `ProPhysics\_Types.h`:



\* `PROPHYSICS\_X\_MAX`: `1024` (Breite des Gitters)

\* `PROPHYSICS\_Y\_MAX`: `512` (Höhe des Gitters)

\* `PROPHYSICS\_Z\_MAX`: `512` (Tiefe des Gitters)

\* `FCC\_INDEX(x, y, z)`: Makro zur Berechnung des flachen eindimensionalen RAM-Index aus den 3D-Koordinaten unter Berücksichtigung der Torus-Topologie:

&#x20;   ```

```text?code\_stdout\&code\_event\_index=5

API-Dokumentation erfolgreich generiert.



```c

&#x20;   ((uint32\_t)(x) + ((uint32\_t)(y) \* PROPHYSICS\_X\_MAX) + ((uint32\_t)(z) \* PROPHYSICS\_X\_MAX \* PROPHYSICS\_Y\_MAX))

&#x20;   ```

\* `QUANTUM\_MASK\_POLARITY` (`0x0000000000000003ULL`): Bitmaske für Quantenladung/Polarität.

&#x20;   \* `QUANTUM\_POL\_NEUTRAL` (`0x0ULL`)

&#x20;   \* `QUANTUM\_POL\_PLUS` (`0x1ULL`)

&#x20;   \* `QUANTUM\_POL\_MINUS` (`0x2ULL`)

\* `QUANTUM\_MASK\_SPIN\_CHIRAL` (`0x000000000000000CULL`): Bitmaske für Chiralität/Drehrichtung.

&#x20;   \* `QUANTUM\_SPIN\_NONE` (`0x0ULL`)

&#x20;   \* `QUANTUM\_SPIN\_CW` (`0x1ULL`) (Rechtsdrehend)

&#x20;   \* `QUANTUM\_SPIN\_CCW` (`0x2ULL`) (Linksdrehend)



\---



\### Zentrale Datentypen \& Strukturen



\#### 1. `QuantumStateIsland`

Repräsentiert eine emergente lokale Masseneinheit oder ein physikalisches Objekt im Datenpool.

```c

typedef struct {

&#x20;   uint64\_t charge\_spin;       // Kombinierte Spin- und Ladungskonfiguration

&#x20;   uint64\_t mass\_accumulator;  // Emergente lokale Dichte-Masse

&#x20;   uint32\_t element\_token;     // Axiom-Identifikator

&#x20;   uint32\_t tick\_counter;      // Relativistischer Takt-Teiler

&#x20;   int64\_t vx;                 // Kinetischer Impuls-Akkumulator X-Achse

&#x20;   int64\_t vy;                 // Kinetischer Impuls-Akkumulator Y-Achse

&#x20;   int64\_t vz;                 // Kinetischer Impuls-Akkumulator Z-Achse

} QuantumStateIsland;



```



\#### 2. `FCCNode`



Ein einzelner Gitterknoten im 3D-Raum mit striktem 4-Byte-Alignment für maximale Cache-Dichte.



```c

\#pragma pack(push, 4)

typedef struct {

&#x20;   uint32\_t neighbor\_idx\[12];  // Indizes der 12 direkten Nachbarn in der FCC-Topologie

&#x20;   uint32\_t active\_flux;       // Aktuelle kinetische Bit-Flüsse (Bits 0-11) + Masse-Bit (Bit 12)

&#x20;   uint32\_t state\_island\_idx;  // Zeiger/Index auf zugeordnete QuantumStateIsland (0 = Vakuum)

&#x20;   uint32\_t local\_anisotropy;  // Lokaler berechneter Anisotropie-Wert

&#x20;   uint32\_t reserved\_gating;   // Reserviertes Gating-Feld für zukünftige Erweiterungen

} FCCNode;

\#pragma pack(pop)



```



\#### 3. `ProUniverse`



Das monolithische Hauptregister, welches den vollständigen Zustand des Kosmos verwaltet.



```c

typedef struct {

&#x20;   FCCNode\* \_\_restrict grid;                       // Array aller Gitterknoten

&#x20;   QuantumStateIsland\* \_\_restrict data\_pool;       // Dynamischer Pool für lokalisierte Quantenzustände

&#x20;   uint32\_t\* active\_nodes\_current;                 // Liste aktuell aktiver Knotenindizes (Sparse Tracking)

&#x20;   uint32\_t\* active\_nodes\_next;                    // Puffer für die nächste Generation aktiver Knoten

&#x20;   uint32\_t\* active\_nodes\_kinetic;                 // Flaches komprimiertes Array für direkte Flux-Berechnungen

&#x20;   uint64\_t  active\_count\_kinetic;                 // Zähler kinetischer Flüsse

&#x20;   uint64\_t  active\_count\_current;                 // Anzahl der aktuell im Fokus befindlichen Knoten

&#x20;   uint64\_t  active\_count\_next;                    // Anzahl der Knoten für den nächsten Zeitschritt

&#x20;   uint8\_t\* node\_active\_bitset;                   // Bit-Array zur schnellen Duplikaterkennung

&#x20;   uint64\_t  current\_cpu\_tick;                     // Globaler Simulationszeitschritt (Takt)

&#x20;   uint32\_t  observer\_node\_idx;                    // Index des virtuellen Beobachterknotens

&#x20;   uint32\_t  active\_element\_count;                 // Anzahl registrierter Zustandsinseln

&#x20;   bool     is\_hardened;                          // Schreibschutz für Invarianz-Sicherung

&#x20;   uint64\_t  dynamic\_invariance\_target;            // Kosmologischer Horizont-Sollwert (Informationelle Erhaltung)

&#x20;   double   global\_entropy\_index;                 // Globaler Netto-Impuls- und Entropiewert

&#x20;   double   observer\_field\_vx;                    // Berechnete Schwerpunktgeschwindigkeit X

&#x20;   double   observer\_field\_vy;                    // Berechnete Schwerpunktgeschwindigkeit Y

&#x20;   double   observer\_field\_vz;                    // Berechnete Schwerpunktgeschwindigkeit Z

} ProUniverse;



```



\---



\### Exportierte API-Funktionen



\#### `ProPhysics\_Initialize`



```c

PROPHYSICS\_API void ProPhysics\_Initialize(ProUniverse\* pu);



```



\* \*\*Beschreibung:\*\* Allokiert die Speicherstrukturen für das Gitter (`grid`), den kinetischen Beschleuniger (`active\_nodes\_kinetic`), das Bitset und die transienten Aktivitätslisten. Startet intern die Multithreading-Worker-Threads (`NUM\_WORKERS = 10`) und bindet diese an CPU-Kerne.

\* \*\*Parameter:\*\* `pu` - Zeiger auf eine zu initialisierende `ProUniverse`-Struktur.



\#### `ProPhysics\_Inject\_Elements`



```c

PROPHYSICS\_API void ProPhysics\_Inject\_Elements(ProUniverse\* pu);



```



\* \*\*Beschreibung:\*\* Initiiert das System, flutet das Vakuum mit zufälligem Quantenrauschen und stanzt vordefinierte physikalische Objekte (z. B. Planet A mit positiver Polarität und Planet B mit negativer Polarität) in das Gitter.



\#### `ProPhysics\_Execute\_Tick`



```c

PROPHYSICS\_API void ProPhysics\_Execute\_Tick(ProUniverse\* pu);



```



\* \*\*Beschreibung:\*\* Führt einen vollständigen, parallelen Simulations-Zeitschritt aus. Die Ausführung erfolgt über ein dreiphasiges Barrier-Modell unter den Threads:

1\. \*Phase 1 (Scatter Prep \& Quantum Flux):\* Interne Kollisionsmatrix-Berechnung, Paar-Streuung und Chiralitäts-Spin-Ablenkungen auf den Gitterachsen.

2\. \*Consolidation Step (Main Thread):\* Verarbeitung der Massenmigration (`Inertial Mass Migration Engine`) unter Einhaltung des Pauli-Prinzips und der Impulserhaltung.

3\. \*Phase 2 \& 3 (Gather \& Cleanup):\* Zusammenführung der gestreuten Fluss-Bits aus den Nachbarknoten und Synchronisation für den nächsten Takt.







\#### `ProPhysics\_Verify\_Invariance`



```c

PROPHYSICS\_API bool ProPhysics\_Verify\_Invariance(const ProUniverse\* pu, uint64\_t expected\_initial\_bits);



```



\* \*\*Beschreibung:\*\* Validiert die absolute informationelle Erhaltung (Energieerhaltungssatz) im Universum, indem die Summe aller Photonen-Bits und Massenäquivalente (Masse-Bit = 3 Bits) mit dem Zielwert abgeglichen wird.

\* \*\*Rückgabewert:\*\* `true`, wenn die Information exakt erhalten blieb; `false` bei Anomalien.



\#### `ProPhysics\_Update\_Observer`



```c

PROPHYSICS\_API void ProPhysics\_Update\_Observer(ProUniverse\* pu, uint64\_t expected\_initial\_bits);



```



\* \*\*Beschreibung:\*\* Berechnet die makroskopischen Schwerpunktskoordinaten, Systemgeschwindigkeiten und den globalen Entropieindex aus der Summe aller mikroskopischen Fluss-Vektoren.



\---



\## 2. UI \& Dashboard Engine (ProDiBatch)



ProDiBatch ist ein asynchrones, thread-sicheres Win32-Visualisierungs-Framework, das im Konsolen-Kontext läuft und Flimmern mittels In-Place-Double-Buffering verhindert.



\### Konfiguration \& Grenzen



Definiert in `ProDiBatch\_Config.h`:



\* `PRODIBATCH\_VIEWPORTS\_COUNT`: `4` (Gleichzeitige Perspektiven: XY, XZ, YZ und Raumdiagonale)

\* `PRODIBATCH\_VIEWPORT\_WIDTH`: `10` (Zellen pro Viewport-Breite)

\* `PRODIBATCH\_VIEWPORT\_HEIGHT`: `10` (Zellen pro Viewport-Höhe)

\* `PRODIBATCH\_CELL\_CHAR\_LEN`: `2` (Breite einer Visualisierungszelle in Zeichen, z. B. `"++"`, `"--"`)

\* `PRODIBATCH\_LOG\_LINES`: `12` (Größe des unteren Live-Protokoll-Puffers)

\* `PRODIBATCH\_REFRESH\_MS`: `33` (Aktualisierungsfrequenz, entspricht ca. 30 FPS zur Vermeidung von CPU-Bloat)



\---



\### Funktions-Schnittstellen (Callbacks)



\#### `ProDiBatch\_CellRenderCallback`



```c

typedef void (\*ProDiBatch\_CellRenderCallback)(

&#x20;   void\* user\_context, // Anonymisierter Zeiger auf das Datenmodell (z. B. ProUniverse\*)

&#x20;   int view\_idx,       // Index des Viewports (0 bis 3)

&#x20;   int local\_x,        // Lokale X-Koordinate innerhalb des Zeichenfensters (0-9)

&#x20;   int local\_y,        // Lokale Y-Koordinate innerhalb des Zeichenfensters (0-9)

&#x20;   char\* out\_chars     // Zielpuffer für exakt 2 ASCII-Zeichen (darf nicht nullterminiert werden)

);



```



\* \*\*Beschreibung:\*\* Ein vom Entwickler bereitzustellender Callback, der von der UI-Engine für jede Bildschirmzelle asynchron aufgerufen wird, um die physikalischen Zustände in darstellbare ASCII-Token zu übersetzen.



\---



\### Exportierte API-Funktionen



\#### `ProDiBatch\_Initialize`



```c

PRODIBATCH\_API bool ProDiBatch\_Initialize(ProDiBatch\_Engine\* engine, void\* user\_context, ProDiBatch\_CellRenderCallback callback);



```



\* \*\*Beschreibung:\*\* Initialisiert das Zustandsregister der UI-Engine und verknüpft das Datenmodell sowie den Render-Vertrag.



\#### `ProDiBatch\_Start`



```c

PRODIBATCH\_API bool ProDiBatch\_Start(ProDiBatch\_Engine\* engine);



```



\* \*\*Beschreibung:\*\* Setzt die Konsole zurück, verbirgt den Cursor und startet den asynchronen Win32-UI-Thread zur fortlaufenden Pufferaktualisierung.



\#### `ProDiBatch\_Log`



```c

PRODIBATCH\_API void ProDiBatch\_Log(ProDiBatch\_Engine\* engine, const char\* format, ...);



```



\* \*\*Beschreibung:\*\* Thread-sicherer, lock-freier Logger. Nutzt atomare Interlocked-Inkremente (`\_InterlockedIncrement`), um Log-Nachrichten formatkonform in einen Ringpuffer einzuspeisen, der im Dashboard dargestellt wird.



\#### `ProDiBatch\_Stop`



```c

PRODIBATCH\_API void ProDiBatch\_Stop(ProDiBatch\_Engine\* engine);



```



\* \*\*Beschreibung:\*\* Signalisiert dem UI-Thread den Abbruch, wartet geordnet auf dessen Beendigung (`WaitForSingleObject`) und gibt die Win32-Handles frei.



\---



\## 3. Architektur- \& Integrationsleitfaden



Um beide Komponenten zu verknüpfen, wird in der Hauptanwendung (`Observer.c`) das Universum initialisiert, an die Dashboard-Engine gekoppelt und in einer deterministischen Zeitschleife berechnet:



```c

// Minimale Bootstrapper-Struktur

int main(void) {

&#x20;   ProUniverse universe = { 0 };

&#x20;   ProPhysics\_Initialize(\&universe);

&#x20;   ProPhysics\_Inject\_Elements(\&universe);



&#x20;   ProDiBatch\_Engine ui\_engine;

&#x20;   // PhysicsCellRenderer implementiert den ProDiBatch\_CellRenderCallback

&#x20;   ProDiBatch\_Initialize(\&ui\_engine, \&universe, PhysicsCellRenderer);

&#x20;   

&#x20;   // Telemetrie verknüpfen

&#x20;   ui\_engine.metric\_target\_invariance = universe.dynamic\_invariance\_target;

&#x20;   ProDiBatch\_Start(\&ui\_engine);



&#x20;   for (uint64\_t tick = 1; tick <= 5000; tick++) {

&#x20;       ProPhysics\_Execute\_Tick(\&universe);

&#x20;       

&#x20;       // Live-Metriken an UI übermitteln

&#x20;       ui\_engine.metric\_sim\_tick = tick;

&#x20;       ui\_engine.metric\_active\_nodes = universe.active\_count\_current;

&#x20;       

&#x20;       if (tick % 1000 == 0) {

&#x20;           ProDiBatch\_Log(\&ui\_engine, "Generation %llu stabil berechnet.", tick);

&#x20;       }

&#x20;   }



&#x20;   ProDiBatch\_Stop(\&ui\_engine);

&#x20;   return 0;

}



```

