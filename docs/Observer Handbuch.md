# **Handbuch: ProPhysics-Observer**

**Version:** 1.0.0

**Repository-Referenz:** onkel83/prophysics

**Systemkomponente:** Laufzeit-Bootstrapper & Visualisierungs-Überwachung

Der **Observer** ist die zentrale ausführbare Anwendung (Bootstrapper) des ProPhysics-Projekts aus dem Repository **onkel83/prophysics**. Seine Aufgabe ist es, den physikalischen Simulationskern (ProPhysics) zu instanziieren, die asynchrone Visualisierungskonsole (ProDiBatch) zu steuern und beide Komponenten in einer echtzeitfähigen Hauptschleife zu koordinieren.

## **1\. Voraussetzungen & wichtige DLL/SO-Abhängigkeiten**

Der Observer ist extrem modular aufgebaut. Das bedeutet, dass die ausführbare Datei (Observer.exe unter Windows bzw. Observer unter Linux) **keine** physikalischen Berechnungen oder Rendering-Logiken selbst enthält. Sie lädt diese Funktionalitäten dynamisch zur Laufzeit.

### **⚠️ Essenzieller Installationsschritt**

Damit die Anwendung startet, müssen sich die kompilierten Dynamic Link Libraries (DLLs) bzw. Shared Objects (SOs) der Sub-Module zwingend **im selben Verzeichnis** wie die ausführbare Datei des Observers befinden (oder im globalen System-Suchpfad registriert sein):

* **Unter Windows:**  
  * Observer.exe (Die Hauptanwendung)  
  * ProPhysics.dll (Der Rechenkern – Physik, Multithreading-Barrieren)  
  * ProDiBatch.dll (Das UI-Framework – Win32-Doppelpufferung, Logger)  
* **Unter Linux / POSIX:**  
  * Observer (Die ausführbare Datei)  
  * libProPhysics.so  
  * libProDiBatch.so

*Sollten diese Dateien fehlen, bricht das Betriebssystem den Start des Observers sofort mit einer Fehlermeldung ab (z. B. "Die Codeausführung kann nicht fortgesetzt werden, da ProPhysics.dll nicht gefunden wurde").*

## **2\. Kompilierung & Start**

### **Kompilierung (NMAKE / Make)**

Navigieren Sie im Root-Verzeichnis des Repositories **onkel83/prophysics** in die jeweiligen Verzeichnisse, um die Komponenten zu bauen.

**Beispiel mit MSVC NMAKE (Windows):**

1. Kompilieren Sie die DLLs in ihren Unterordnern:  
   cd Observer/.. \# Hauptverzeichnis  
   nmake \-f Makefile.nmake

   *(Dadurch werden die nötigen Bibliotheken in ihren Verzeichnissen gebaut. Kopieren Sie die ProPhysics.dll und ProDiBatch.dll anschließend in den Ordner des Observers).*

### **Starten der Anwendung**

Führen Sie den Observer direkt über die Kommandozeile aus:

cd Observer  
Observer.exe

## **3\. Was man sieht (Das Dashboard-Layout)**

Nach dem Start öffnet sich ein optimiertes Text-Terminal. Das Interface ist in drei logische Zonen unterteilt, um eine flimmerfreie Überwachung der Quanten-Ströme in Echtzeit zu gewährleisten:

\+-----------------------------------------------------------------------------+  
| \[Viewport 0: XY\]  \[Viewport 1: XZ\]  \[Viewport 2: YZ\]  \[Viewport 3: Diag\]   |  
|   . . \++ . . .      . . . . . . .      . . \-- . . .      . . . . . . .      |  
|   . \++ \++ . . .      . . . . . . .      . \-- \-- . . .      . . . . . . .      |  
|   . . \++ . . .      . . . . . . .      . . \-- . . .      . . . . . . .      |  
|   (10x10 Gitter)    (10x10 Gitter)    (10x10 Gitter)    (10x10 Gitter)     |  
\+-----------------------------------------------------------------------------+  
| TELEMETRIE: Tick: 0000412 | Active: 2481 | Target Invariance: 582910      |  
\+-----------------------------------------------------------------------------+  
| LIVE LOG:                                                                   |  
| \[14:02:11\] \[System\] Gitter-Netzwerk erfolgreich initialisiert.             |  
| \[14:02:12\] \[Physics\] Stern-Objekt 'Alpha' mit Radius 10 implantiert.       |  
| \[14:02:13\] \[Invariance\] Erhaltungssatz verifiziert: 100.00% intakt.        |  
\+-----------------------------------------------------------------------------+

### **Die Visualisierungs-Zellen (ASCII-Token)**

Jede Zelle in den vier Viewports stellt einen diskreten Zustand des Face-Centered Cubic (FCC) Gitters dar:

* (Leerzeichen): Reines Quanten-Vakuum. Es sind keine Ströme oder Massen aktiv.  
* \++ (Doppel-Plus): Lokale Konzentration positiver Quantenladung (Proton-Äquivalente).  
* \-- (Doppel-Minus): Lokale Konzentration negativer Quantenladung (Elektron-Äquivalente).  
* \* / \* (Sterne / Flüsse): Freie Photonen-Fluss-Vektoren, die sich mit Lichtgeschwindigkeit durch das Gitter bewegen.

### **Die Telemetrie-Bar**

* **Tick (Simulationsschritt):** Der aktuelle evolutionäre Takt des Universums.  
* **Active (Aktive Rechenknoten):** Zeigt die Anzahl der Zellen an, die aktuell im *Sparse-Tracking-Puffer* liegen und aktiv berechnet werden müssen (schont die CPU im Vergleich zu statischen Gittern).  
* **Target Invariance (Erhaltungswert):** Die exakte Summe aller Quanten-Bits beim Urknall. Dieser Wert darf sich während der gesamten Laufzeit nicht um ein einziges Bit verändern.

## **4\. Aktuelle Features & physikalische Möglichkeiten**

Mit der aktuellen Version des Observers können folgende physikalische Phänomene beobachtet und evaluiert werden:

1. **Parallele Quanten-Evolution:** 10 CPU-Worker-Threads berechnen das Gitter parallel. Jede Zelle interagiert mit ihren 12 FCC-Nachbarzellen.  
2. **Gravitative Trägheits-Migration:** Durch die Kollision freier Fluss-Bits (Photonen) mit schweren Masseninseln baut sich ein kinetischer Impuls auf. Überschreitet dieser die Trägheitsschranke der Masse, wandert die Masse im Gitter weiter.  
3. **Pauli-Prinzip (Ausschließungsprinzip):** Zwei schwere Massen können niemals dieselbe Gitterkoordinate zur selben Zeit besetzen. Es kommt zu elastischen Stößen oder zur Ablenkung.  
4. **Echtzeit-Invarianzprüfung:** Der Observer verifiziert nach jedem Rechenschritt die absolute Energieerhaltung (Informationserhaltung). Jedes Bit, das "verschwindet" oder fehlerhaft generiert wird, führt sofort zu einem kritischen Log-Eintrag.  
5. **Asynchrones Logging:** Protokolle aus den rechenintensiven Hintergrund-Worker-Threads werden über atomare Operationen blockierungsfrei direkt in den sichtbaren Log-Puffer geschleust.