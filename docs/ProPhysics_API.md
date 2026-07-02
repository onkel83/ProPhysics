```python

\# Content for ProPhysics API documentation (Standalone)

prophysics\_md = """# ProPhysics SDK – Core Engine API-Dokumentation

\*\*Version:\*\* 1.2.0  

\*\*Typ:\*\* Shared Library / DLL API-Schnittstellenbeschreibung  

\*\*Sprachstandard:\*\* C99  

\*\*Speicherarchitektur:\*\* Zero-Allocation Core, Cache-Line Aligned (Striktes 4-Byte Struct-Packing für Gitterknoten)



Diese Dokumentation beschreibt die eigenständige C-Schnittstelle des \*\*ProPhysics\*\*-Simulationskerns. Die Engine ist als eigenständige Dynamic Link Library (`.dll` unter Windows bzw. `.so` unter POSIX) konzipiert und implementiert ein hochparalleles Face-Centered Cubic (FCC) Lattice-Gas-Automaton (LGA) Modell („It from a Bit“ Simulation). Sie kann vollständig unabhängig von Visualisierungs- oder Bootstrapper-Schichten in beliebige C/C++-Projekte eingebunden werden.



\---



\## 1. Kompilierungs-Konfiguration \& Makros (`ProPhysics\_Config.h`)



Bevor die Bibliothek kompiliert wird, können über die Header-Datei `ProPhysics\_Config.h` grundlegende physikalische und strukturelle Schranken statisch festgelegt werden:



\* `PROPHYSICS\_X\_MAX` (`1024`): Die maximale Gitterauflösung entlang der X-Achse.

\* `PROPHYSICS\_Y\_MAX` (`512`): Die maximale Gitterauflösung entlang der Y-Achse.

\* `PROPHYSICS\_Z\_MAX` (`512`): Die maximale Gitterauflösung entlang der Z-Achse.

\* `FCC\_INDEX(x, y, z)`: Inline-Makro zur Abbildung von 3D-Koordinaten auf ein flaches 1D-RAM-Segment. Es implementiert implizit das periodische Gating für die Torus-Topologie:

&#x20;   ```

```text?code\_stdout\&code\_event\_index=2

Beide separaten API-Dokumentationen wurden erfolgreich erstellt.



```c

&#x20;   ((uint32\_t)(x) + ((uint32\_t)(y) \* PROPHYSICS\_X\_MAX) + ((uint32\_t)(z) \* PROPHYSICS\_X\_MAX \* PROPHYSICS\_Y\_MAX))

&#x20;   ```

\* `TOTAL\_ELEMENT\_AXIOMS` (`118`): Anzahl der maximal registrierbaren Element-Axiome.

\* `MAX\_MOLECULE\_ATOMS` (`128`): Maximale Anzahl von Atomen innerhalb einer strukturellen Molekülform.



\### Quantenkonfigurations-Bitmasken (64-Bit Aligned)

Dienen zur Analyse des `active\_flux`-Zustands oder der `charge\_spin`-Konfiguration:

\* `QUANTUM\_MASK\_POLARITY` (`0x0000000000000003ULL`): Filtert die Ladung heraus (Bits 0-1).

&#x20;   \* `QUANTUM\_POL\_NEUTRAL` (`0x0ULL`): Unpolarisiertes Feld / Vakuum.

&#x20;   \* `QUANTUM\_POL\_PLUS` (`0x1ULL`): Positive Quantenpolarität.

&#x20;   \* `QUANTUM\_POL\_MINUS` (`0x2ULL`): Negative Quantenpolarität.

\* `QUANTUM\_MASK\_SPIN\_CHIRAL` (`0x000000000000000CULL`): Filtert die Chiralität heraus (Bits 2-3).

&#x20;   \* `QUANTUM\_SPIN\_NONE` (`0x0ULL`): Kein intrinsischer Drehimpuls.

&#x20;   \* `QUANTUM\_SPIN\_CW` (`0x1ULL`): Clockwise (Rechtsdrehend).

&#x20;   \* `QUANTUM\_SPIN\_CCW` (`0x2ULL`): Counter-Clockwise (Linksdrehend).

\* `QUANTUM\_MASK\_SPIN\_AXIS` (`0x00000000000000F0ULL`): Rotationsachse (Bits 4-7; bildet auf die 12 FCC-Richtungen ab).



\---



\## 2. Zentrale Datentypen \& Speicher-Layout (`ProPhysics\_Types.h`)



\### `RelativeAtom`

Beschreibt ein Atom in Relation zu einem molekularen Schwerpunkt.

```c

typedef struct {

&#x20;   int8\_t   dx, dy, dz;    // Relative 3D-Gitterabstände

&#x20;   uint8\_t  atomic\_number; // Ordnungszahl / Element-Token

} RelativeAtom;



```



\### `StructuralForm`



Beschreibt die geometrische Verteilung von Molekülen.



```c

typedef struct {

&#x20;   uint64\_t hash;

&#x20;   RelativeAtom atoms\[MAX\_MOLECULE\_ATOMS];

&#x20;   uint32\_t     atom\_count;

} StructuralForm;



```



\### `QuantumStateIsland`



Das zentrale Register für emergente, lokalisierte Massenstrukturen (z. B. Himmelskörper oder Elementarpartikel).



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



\### `FCCNode` (Strikte Ausrichtung)



Ein einzelner Punkt im Face-Centered Cubic Raumgitter. Wird über `#pragma pack(push, 4)` auf exakt 4-Byte-Grenzen ausgerichtet, um Speicherlöcher beim CPU-Cache-Line-Interleaving zu verhindern.



```c

typedef struct {

&#x20;   uint32\_t neighbor\_idx\[12];  // Die vorberechneten Indizes der 12 FCC-Nachbarzellen

&#x20;   uint32\_t active\_flux;       // Bitmaske der 12 Geschwindigkeitsvektoren (Bits 0-11) + Masse-Bit (Bit 12)

&#x20;   uint32\_t state\_island\_idx;  // Index in den globalen Datenpool (0 = ungebundenes Vakuum)

&#x20;   uint32\_t local\_anisotropy;  // Lokaler struktureller Richtungsvektor

&#x20;   uint32\_t reserved\_gating;   // Reservierter Alignment-Speicher

} FCCNode;



```



\### `ProUniverse`



Das Master-Zustandsregister, welches den gesamten Speicherkontext eines simulierten Kosmos umschließt. DLL-Konsumenten allokieren diese Struktur und übergeben sie per Zeiger an die API.



```c

typedef struct {

&#x20;   FCCNode\* \_\_restrict grid;                       // Flaches Array aller Gitterknoten (Größe: X\*Y\*Z)

&#x20;   QuantumStateIsland\* \_\_restrict data\_pool;       // Globaler Pool für Massen-Zustandsinseln

&#x20;   uint32\_t\* active\_nodes\_current;                 // Sparse-Liste: Indizes der aktuell aktiven Zellen

&#x20;   uint32\_t\* active\_nodes\_next;                    // Sparse-Liste: Arbeits-Puffer für die nächste Generation

&#x20;   uint32\_t\* active\_nodes\_kinetic;                 // Schnelles, komprimiertes Array für direkte Flux-Updates

&#x20;   uint64\_t  active\_count\_kinetic;                 // Zähler der kinetisch relevanten Operationen

&#x20;   uint64\_t  active\_count\_current;                 // Anzahl der Einträge in active\_nodes\_current

&#x20;   uint64\_t  active\_count\_next;                    // Anzahl der Einträge in active\_nodes\_next

&#x20;   uint8\_t\* node\_active\_bitset;                   // Bitfeld zur O(1)-Duplikatsvermeidung bei der Gitterwanderung

&#x20;   uint64\_t  current\_cpu\_tick;                     // Akkumulierter Zeitschritt-Zähler (Simulationstakt)

&#x20;   uint32\_t  observer\_node\_idx;                    // Fokus-Index des virtuellen Beobachters

&#x20;   uint32\_t  active\_element\_count;                 // Aktuelle Belegung des data\_pool

&#x20;   bool      is\_hardened;                          // Status-Flag für Modifikationsschutz

&#x20;   uint64\_t  dynamic\_invariance\_target;            // Informationeller Sollwert (Bitsumme zur Erhaltung)

&#x20;   double    global\_entropy\_index;                 // Berechnetes Entropiemaß des Gesamtsystems

&#x20;   double    observer\_field\_vx;                    // Makroskopischer System-Schwerpunkt-Impuls X

&#x20;   double    observer\_field\_vy;                    // Makroskopischer System-Schwerpunkt-Impuls Y

&#x20;   double    observer\_field\_vz;                    // Makroskopischer System-Schwerpunkt-Impuls Z

} ProUniverse;



```



\---



\## 3. API-Funktionsreferenz (`ProPhysics.h`)



Alle Funktionen sind plattformübergreifend exportiert. Unter Windows wird das Makro `PROPHYSICS\_API` zu `\_\_declspec(dllexport)` expandiert (wenn `PROPHYSICS\_EXPORTS` definiert ist), andernfalls bleibt es für statisches Linken frei oder steuert unter GCC/Clang die `visibility("default")`.



\### `ProPhysics\_Initialize`



```c

PROPHYSICS\_API void ProPhysics\_Initialize(ProUniverse\* pu);



```



\* \*\*Beschreibung:\*\* Initialisiert den Speicherkontext des übergebenen Universums. Allokiert die internen Gitter-Arrays (`grid`, `active\_nodes\_kinetic`, `node\_active\_bitset` sowie die Sparse-Listen) im RAM, sofern diese noch nicht zugewiesen sind. Setzt alle Felder auf Null zurück.

\* \*\*Wichtig:\*\* Diese Funktion startet im Hintergrund automatisch das native Windows-Multithreading-System (`NUM\_WORKERS = 10`), bindet die Threads per CPU-Affinitätsmaske (`SetThreadAffinityMask`) an feste CPU-Kerne ab Core 2 und versetzt sie in ein lock-freies `\_mm\_pause()`-Spinning, um Latenzen bei der Thread-Aktivierung zu minimieren.

\* \*\*Preconditions:\*\* `pu` darf nicht `NULL` sein.



\### `ProPhysics\_Inject\_Elements`



```c

PROPHYSICS\_API void ProPhysics\_Inject\_Elements(ProUniverse\* pu);



```



\* \*\*Beschreibung:\*\* Flutet das Vakuum des Gitters mit einem pseudozufälligen Quantenrauschen (Samen: `1337`). Zudem werden zwei makroskopische Himmelskörper (Planet A und Planet B) mit einem Radius von 10 Zellen an den Koordinatenachsen in den RAM gestanzt, um Gravitations- und Migrationsszenarien zu simulieren.

\* \*\*Preconditions:\*\* Das Universum muss zuvor mit `ProPhysics\_Initialize` initialisiert worden sein.



\### `ProPhysics\_Execute\_Tick`



```c

PROPHYSICS\_API void ProPhysics\_Execute\_Tick(ProUniverse\* pu);



```



\* \*\*Beschreibung:\*\* Das Herzstück der Engine. Berechnet einen vollständigen globalen Simulationszeitschritt. Die Funktion koordiniert die 10 Worker-Threads über ein hochoptimiertes, dreiphasiges atomares Barrieren-Modell (`\_InterlockedExchange`, `\_InterlockedIncrement`):

1\. \*\*Phase 1 (Parallel Flux Update):\*\* Jeder Thread verarbeitet ein gleich großes Daten-Chunk der aktiven Knoten. Es wird die Funktion `ProPhysics\_Update\_Quantum\_Flux` aufgerufen, welche Paar-Streuung (LGA-Kollisionen), Achsen-Reflexionen bei entgegengesetzten Ladungen sowie die Spin-Ablenkungen (Chiralität) berechnet.

2\. \*\*Inertial Mass Migration (Main Thread):\*\* Der Haupt-Thread übernimmt exklusiv die Trägheits-Massen-Migration. Wenn ein Gitterknoten schwere Ruhemasse enthält (`Bit 12 / 0x1000`) und die akkumulierten Impulse einer Zustandsinsel eine definierte Trägheitsschranke überschreiten, wandert die Masse entlang der FCC-Richtungen weiter. Hierbei greift die \*\*Pauli-Exklusion\*\*: Ein Feld darf nur betreten werden, wenn es frei von anderer schwerer Masse ist. Bei Bewegung wird verbrauchte Bewegungsenergie vom Vektor abgezogen (Impulserhaltung).

3\. \*\*Phase 2 \& 3 (Parallel Gather \& Cleanup):\*\* Die Threads erwachen erneut, um die gestreuten Fluss-Bits bijektiv aus den Nachbarknoten einzusammeln (`Gather`) und die transienten Bitsets für den Folgetakt zu bereinigen.





\* \*\*Preconditions:\*\* `pu` initialisiert; `active\_count\_current > 0`.



\### `ProPhysics\_Verify\_Invariance`



```c

PROPHYSICS\_API bool ProPhysics\_Verify\_Invariance(const ProUniverse\* pu, uint64\_t expected\_initial\_bits);



```



\* \*\*Beschreibung:\*\* Führt eine forensische Überprüfung der informationellen Gittererhaltung durch. Es zählt die Popcounts aller aktiven Photonen-Flussbits und addiert für jedes gesetzte Masse-Bit exakt `3` Informationseinheiten hinzu.

\* \*\*Rückgabewert:\*\* `true`, wenn die aktuelle Summe der Bits exakt dem Wert `expected\_initial\_bits` entspricht (Fehlen von Rundungsfehlern oder Datenverlust). `false`, falls eine informationelle Anomalie detektiert wurde.



\### `ProPhysics\_Update\_Observer`



```c

PROPHYSICS\_API void ProPhysics\_Update\_Observer(ProUniverse\* pu, uint64\_t expected\_initial\_bits);



```



\* \*\*Beschreibung:\*\* Berechnet makroskopische Observablen des Gesamtsystems durch statistische Reduktion des Gitters. Ermittelt den virtuellen Massen-Schwerpunkt (`observer\_field\_vx/vy/vz`) sowie den globalen Netto-Impuls (`global\_entropy\_index`).



\### `ProPhysics\_Reset`



```c

PROPHYSICS\_API void ProPhysics\_Reset(ProUniverse\* pu, uint64\_t\* initial\_bit\_tracker);



```



\* \*\*Beschreibung:\*\* Setzt das Universum komplett zurück, sofern das Feld `is\_hardened` nicht auf `true` steht. Initialisiert den Speicher neu, flutet das System mit Elementen und speichert die exakte Anzahl der generierten Initial-Bits im Zeiger `initial\_bit\_tracker` ab.



\### `ProPhysics\_Query\_Anisotropy`



```c

PROPHYSICS\_API double ProPhysics\_Query\_Anisotropy(const ProUniverse\* pu, int32\_t x, int32\_t y, int32\_t z);



```



\* \*\*Beschreibung:\*\* Platzhalter für lokale Feldmessungen an einer expliziten Koordinate (Standardmäßig `0.0`).



\---



\## 4. DLL-Integrationsbeispiel (Standalone C)



```c

\#include <stdio.h>

\#include <stdlib.h>

\#include "ProPhysics.h"



int main(void) {

&#x20;   // 1. Universum-Kontext auf dem Heap allokieren

&#x20;   ProUniverse\* my\_cosmos = (ProUniverse\*)calloc(1, sizeof(ProUniverse));

&#x20;   if (!my\_cosmos) return 1;



&#x20;   // 2. Speicher-Strukturen und Threads über die DLL initialisieren

&#x20;   ProPhysics\_Initialize(my\_cosmos);

&#x20;   printf("\[SDK] Speicher allokiert. Core-Engine-Takt steht bei: %llu\\n", my\_cosmos->current\_cpu\_tick);



&#x20;   // 3. Quanten- und Objektstrukturen injizieren

&#x20;   ProPhysics\_Inject\_Elements(my\_cosmos);



&#x20;   // 4. Initialen Erhaltungs-Sollwert ermitteln

&#x20;   uint64\_t initial\_bits = 0;

&#x20;   ProPhysics\_Reset(my\_cosmos, \&initial\_bits);

&#x20;   printf("\[SDK] Kosmologischer Horizont fixiert auf: %llu Informationseinheiten\\\\n", initial\_bits);



&#x20;   // 5. Unabhängige Simulationsschleife (z.B. 1000 Ticks)

&#x20;   for (int i = 0; i < 1000; i++) {

&#x20;       ProPhysics\_Execute\_Tick(my\_cosmos);



&#x20;       if (i % 100 == 0) {

&#x20;           // Invarianz-Prüfung über die API abfragen

&#x20;           bool intact = ProPhysics\_Verify\_Invariance(my\_cosmos, initial\_bits);

&#x20;           printf("\[Takt %04d] Informationserhaltung: %s\\n", i, intact ? "INTAKT" : "ANOMALIE DETEKTIERT!");

&#x20;       }

&#x20;   }



&#x20;   // Speicherbereinigung (Hinweis: Worker-Threads laufen bis zum Prozessende)

&#x20;   free(my\_cosmos->grid);

&#x20;   free(my\_cosmos->active\_nodes\_kinetic);

&#x20;   free(my\_cosmos->node\_active\_bitset);

&#x20;   free(my\_cosmos->active\_nodes\_current);

&#x20;   free(my\_cosmos->active\_nodes\_next);

&#x20;   free(my\_cosmos);



&#x20;   return 0;

}



```

