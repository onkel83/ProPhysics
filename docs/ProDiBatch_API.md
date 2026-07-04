\# ProDiBatch SDK – UI Engine API-Dokumentation

\*\*Version:\*\* 1.0.0



\*\*Typ:\*\* Shared Library / DLL API-Schnittstellenbeschreibung



\*\*Sprachstandard:\*\* C99



\*\*Visualisierungstyp:\*\* Win32 High-Performance ASCII-Textmodus (Echtzeit-Doppelpufferung im RAM)



Diese Dokumentation beschreibt die öffentliche Programmierschnittstelle des \*\*ProDiBatch\*\*-Dashboard-Frameworks. ProDiBatch ist eine plattform- und modellunabhängige Benutzeroberfläche, die als Dynamic Link Library (`.dll` bzw. `.so`) kompiliert wird. Sie entkoppelt mathematische oder physikalische Rechenkerne vollständig von der Ausgabeebene, indem sie ein zeilenbasiertes Interleaving-Rendering über asynchrone Callbacks in einem separaten Win32-Thread realisiert.



\---



\## 1. Dashboard-Konfiguration \& Grenzwerte (`ProDiBatch\_Config.h`)



Die grafische Anordnung des Dashboards wird statisch über folgende Compile-Time-Konstanten vordefiniert:



\* `PRODIBATCH\_VIEWPORTS\_COUNT` (`4`): Die Anzahl parallel nebeneinander dargestellter Überwachungsfenster (Schnittansichten).

\* `PRODIBATCH\_VIEWPORT\_WIDTH` (`10`): Die Breite eines einzelnen Viewports in Gitterzellen.

\* `PRODIBATCH\_VIEWPORT\_HEIGHT` (`10`): Die Höhe eines einzelnen Viewports in Gitterzellen.

\* `PRODIBATCH\_CELL\_CHAR\_LEN` (`2`): Jede Zelle belegt exakt zwei ASCII-Zeichen (z. B. `\\"++\\"` für positive Ladung, `\\"--\\"` für negative Ladung, `\\"  \\"` für Vakuum).

\* `PRODIBATCH\_LOG\_LINES` (`12`): Die maximale Zeilenanzahl des asynchronen unteren Live-Protokoll-Puffers.

\* `PRODIBATCH\_LOG\_LINE\_LEN` (`125`): Die maximale Breite einer einzelnen Log-Zeile vor dem Abschneiden.

\* `PRODIBATCH\_REFRESH\_MS` (`33`): Die Aktualisierungsfrequenz des UI-Threads (ca. 30 Bilder pro Sekunde). Schützt die CPU vor unnötigem Zeichnungs-Overhead.



\---



\## 2. Funktionsverträge \& Datenstrukturen (`ProDiBatch.h`)



\### `ProDiBatch\_CellRenderCallback`



Dies ist die funktionale Brücke zum Anwendungsmodell. Der DLL-Konsument muss diese Funktion implementieren und der UI-Engine übergeben.



```c

typedef void (\*ProDiBatch\_CellRenderCallback)(

&#x20;   void\* user\_context, // Anonymisierter Zeiger auf das Kernmodell (z. B. ProUniverse\*)

&#x20;   int view\_idx,       // Aktueller Viewport-Index (0 bis 3)

&#x20;   int local\_x,        // Lokale X-Koordinate innerhalb des Viewports (0 bis 9)

&#x20;   int local\_y,        // Lokale Y-Koordinate innerhalb des Viewports (0 bis 9)

&#x20;   char\* out\_chars     // Zielpuffer für exakt 2 ASCII-Zeichen (KEINE Nullterminierung vornehmen!)

);



```



\### `ProDiBatch\_LogField`



Ein interner, thread-sicherer Ringpuffer zur Speicherung des asynchronen Logging-Stroms.



```c

typedef struct {

&#x20;   char lines\[PRODIBATCH\_LOG\_LINES]\[PRODIBATCH\_LOG\_LINE\_LEN];

&#x20;   volatile long write\_index; // Atomar verwalteter Schreibzeiger

} ProDiBatch\_LogField;



```



\### `ProDiBatch\_Engine`



Das zentrale Zustandsregister der UI-Bibliothek. Es enthält alle Steuerungselemente und Echtzeit-Metriken, die direkt auf dem Bildschirm ausgegeben werden.



```c

typedef struct {

&#x20;   void\* user\_context;     // Zeiger auf das externe Datenmodell

&#x20;   ProDiBatch\_CellRenderCallback cell\_renderer;    // Registrierter Render-Callback



&#x20;   // Echtzeit-Telemetrie-Schnittstellen (Können direkt vom Rechenkern beschrieben werden)

&#x20;   volatile uint64\_t             metric\_sim\_tick;          // Aktueller Simulations-Schritt

&#x20;   volatile uint64\_t             metric\_active\_nodes;      // Anzahl aktiver Rechenknoten

&#x20;   volatile uint64\_t             metric\_target\_invariance; // Soll-Erhaltungswert



&#x20;   ProDiBatch\_LogField           log\_buffer;       // Lokaler Protokollpuffer

&#x20;   volatile bool                 is\_running;       // Kontroll-Flag für den UI-Thread

&#x20;   void\* thread\_handle;    // Internes Win32-Thread-HANDLE

} ProDiBatch\_Engine;



```



\---



\## 3. Öffentliche API-Schnittstellen (`ProDiBatch.h`)



\### `ProDiBatch\_Initialize`



```c

PRODIBATCH\_API bool ProDiBatch\_Initialize(ProDiBatch\_Engine\* engine, void\* user\_context, ProDiBatch\_CellRenderCallback callback);



```



\* \*\*Beschreibung:\*\* Initialisiert das übergebene Zustandsregister der UI-Engine. Bereinigt alle Speicherbereiche, verknüpft das anwenderspezifische Kernmodell (`user\_context`) und brennt den Rendering-Callback fest ein.

\* \*\*Parameter:\*\*

\* `engine`: Zeiger auf eine allokierte `ProDiBatch\_Engine`-Struktur.

\* `user\_context`: Beliebiger Zeiger auf die Datenstrukturen des Simulationskerns.

\* `callback`: Funktionszeiger zum Zell-Renderer.





\* \*\*Rückgabewert:\*\* `true` bei Erfolg, `false` wenn `engine` oder `callback` `NULL` sind.



\### `ProDiBatch\_Start`



```c

PRODIBATCH\_API bool ProDiBatch\_Start(ProDiBatch\_Engine\* engine);



```



\* \*\*Beschreibung:\*\* Aktiviert das Dashboard. Löscht den Terminal-Bildschirm, deaktiviert den blinkenden Win32-Konsolencursor vollständig (zur Vermeidung von Tearing/Flimmern) und spawnt den asynchronen Hintergrundthread (`ProDiBatch\_ThreadProc`).

\* \*\*Funktionsweise des UI-Threads:\*\* Der Thread generiert im Abstand von 33ms ein exaktes Text-Abbild des Dashboards in einem RAM-basierten `back\_buffer`. Anschließend vergleicht er diesen zeichenweise mit dem sichtbaren `front\_buffer`. Nur geänderte Zeichen werden mittels punktueller Win32-Cursorpositionierung (`SetConsoleCursorPosition`) direkt im Terminal überschrieben. Dies ermöglicht eine extrem ressourcenschonende und flimmerfreie Darstellung mit 0% CPU-Verschwendung.

\* \*\*Rückgabewert:\*\* `true`, wenn der Thread erfolgreich erzeugt wurde; andernfalls `false`.



\### `ProDiBatch\_Log`



```c

PRODIBATCH\_API void ProDiBatch\_Log(ProDiBatch\_Engine\* engine, const char\* format, ...);



```



\* \*\*Beschreibung:\*\* Ein absolut thread-sicherer, sperrenfreier (lock-free) Logger. Kann aus beliebigen Threads (z.B. den parallelen Rechen-Workern der Physik-Engine) simultan aufgerufen werden. Nutzt die atomare CPU-Instruktion `\_InterlockedIncrement`, um Kollisionen im Ringpuffer auszuschließen. Formatiert Texte analog zu `printf`.



\### `ProDiBatch\_Stop`



```c

PRODIBATCH\_API void ProDiBatch\_Stop(ProDiBatch\_Engine\* engine);



```



\* \*\*Beschreibung:\*\* Fährt das UI-System geordnet herunter. Setzt das Flag `is\_running` auf `false`, wartet über das Betriebssystem auf das Ende des Threads (`WaitForSingleObject`), schließt das Win32-Handle und stellt den Standardzustand der Konsole wieder her.



\---



\## 4. DLL-Integrationsbeispiel (Standalone UI mit Dummy-Rechenkern)



Dieses Beispiel demonstriert, wie ProDiBatch als eigenständige DLL eingebunden wird, um ein beliebiges mathematisches Raster (hier ein einfaches Wellenmuster) asynchron darzustellen.



```c

\#include <windows.h>

\#include <stdio.h>

\#include <math.h>

\#include "ProDiBatch.h"



// Ein einfacher, benutzerdefinierter Datenkontext (Rechenkern)

typedef struct {

&#x20;   double phase;

&#x20;   uint64\_t operations;

} MyCustomModel;



// Implementierung des Render-Vertrags

void MyCustomCellRenderer(void\* user\_context, int view\_idx, int local\_x, int local\_y, char\* out\_chars) {

&#x20;   MyCustomModel\* model = (MyCustomModel\*)user\_context;

&#x20;   

&#x20;   // Generiere ein mathematisches Muster basierend auf Koordinaten und Phase

&#x20;   double val = sin((double)local\_x \* 0.5 + model->phase) \* cos((double)local\_y \* 0.5 + (double)view\_idx);

&#x20;   

&#x20;   if (val > 0.4) {

&#x20;       out\_chars\[0] = '\*'; out\_chars\[1] = '\*';

&#x20;   } else if (val < -0.4) {

&#x20;       out\_chars\[0] = 'o'; out\_chars\[1] = 'o';

&#x20;   } else {

&#x20;       out\_chars\[0] = ' '; out\_chars\[1] = ' ';

&#x20;   }

}



int main(void) {

&#x20;   // 1. Lokales Modell initialisieren

&#x20;   MyCustomModel model = { 0.0, 0 };



&#x20;   // 2. ProDiBatch Engine-Kontext deklarieren und initialisieren

&#x20;   ProDiBatch\_Engine ui;

&#x20;   if (!ProDiBatch\_Initialize(\&ui, \&model, MyCustomCellRenderer)) {

&#x20;       printf("Fehler bei UI-Initialisierung!\\\\n");

&#x20;       return 1;

&#x20;   }



&#x20;   // Statische Metadaten an das Dashboard übergeben

&#x20;   ui.metric\_target\_invariance = 9999; 



&#x20;   // 3. Asynchrones Dashboard starten

&#x20;   ProDiBatch\_Start(\&ui);

&#x20;   ProDiBatch\_Log(\&ui, "Standalone Dashboard erfolgreich gestartet.");

&#x20;   ProDiBatch\_Log(\&ui, "Modell angebunden. Berechne Wellenkontext...");



&#x20;   // 4. Haupt-Rechenschleife

&#x20;   for (uint64\_t tick = 1; tick <= 300; tick++) {

&#x20;       // Simuliere mathematische Berechnung

&#x20;       model.phase += 0.05;

&#x20;       model.operations += (local\_x \* local\_y);

&#x20;       Sleep(50); // Simuliere Rechenzeit



&#x20;       // Telemetrie live aktualisieren (Hintergrund-Thread liest dies automatisch)

&#x20;       ui.metric\_sim\_tick = tick;

&#x20;       ui.metric\_active\_nodes = model.operations % 500;



&#x20;       if (tick == 50)  ProDiBatch\_Log(\&ui, "Meilenstein erreicht: 50 Iterationen.");

&#x20;       if (tick == 150) ProDiBatch\_Log(\&ui, "System stabil bei ca. 20 Hz.");

&#x20;   }



&#x20;   // 5. Ordentlicher Shutdown

&#x20;   ProDiBatch\_Log(\&ui, "Berechnung abgeschlossen. Schließe Dashboard...");

&#x20;   Sleep(1000);

&#x20;   ProDiBatch\_Stop(\&ui);



&#x20;   printf("UI erfolgreich beendet. Prozess ordnungsgemaess geschlossen.\\\\n");

&#x20;   return 0;

}



```





