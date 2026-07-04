# **Handbuch: ProPhysics-Observer**

**Version:** 1.2.0

**Repository-Referenz:** `onkel83/prophysics`

**Systemkomponente:** Laufzeit-Bootstrapper, Interaktive Gating-Zentrale & Multi-Axis UI

Der **Observer** ist die zentrale ausführbare Steueranwendung (Bootstrapper) des ProPhysics-Projekts aus dem Repository **onkel83/prophysics**. Seine Hauptaufgabe besteht darin, den parallelen, zellulären Simulationskern (`ProPhysics`) zu instanziieren, das asynchrone Visualisierungs-Framework (`ProDiBatch`) zu verwalten und über eine hardwarenahe Hauptschleife die sieben physikalisch-chemischen Analysemodule zu koordinieren.

---

## 1. Voraussetzungen & Systemarchitektur

Der Observer basiert auf einer strikt modularen, cache-optimierten Struktur. Die ausführbare Binärdatei kapselt lediglich die asynchrone I/O-Schleife und die interaktive Eingabeverarbeitung, während die rechenkritischen Subsysteme als dynamische Bibliotheken ausgelagert sind.

### ⚠️ Essenzieller Installationsschritt

Damit das System erfolgreich instanziiert werden kann, müssen die kompilierten Dynamic Link Libraries (DLLs) bzw. Shared Objects (SOs) zwingend **im selben Ausführungsverzeichnis** wie die Observer-Binärdatei liegen:

* **Windows-Umgebung:**
* `Observer.exe` (Das interaktive Hauptprogramm)
* `ProPhysics.dll` (Der LGA-Rechenkern – Gitterzustände, Thread-Barrieren)
* `ProDiBatch.dll` (Das UI-Framework – Doppelpufferung, asynchroner Log-Ringbuffer)


* **Linux / POSIX-Umgebung:**
* `Observer` (Ausführbare Steuerdatei)
* `libProPhysics.so`
* `libProDiBatch.so`



---

## 2. Kompilierung & Start

Die Übersetzung des Gesamtsystems erfolgt automatisiert über die im Root-Verzeichnis hinterlegten NMAKE-Skripte für den MSVC-Compiler-Toolchain.

### Bauprozess via Kommandozeile

```bash
# Wechsel in das Hauptverzeichnis des Repositories
cd ProPhysics-8af0aece6e4b99b5b9b9bf32919401510cd235ed/

# Kompilierung des Kerns und der Benutzeroberfläche über NMAKE
nmake -f Makefile.nmake

# Wechsel in den Observer-Kontrollraum und Start des Systems
cd Observer
Observer.exe

```

---

## 3. Das Dashboard-Layout (Flimmerfreie UI-Trennung)

Um optische Zeichenkorruption und Cursor-Flackern durch asynchron eintreffende Modul-Logs vollständig zu unterbinden, nutzt der Observer die Win32-Konsolen-API zur harten Partitionierung des Bildschirms. Der Eingabebereich (**Sticky Prompt**) ist unerschütterlich an die allerletzte Konsolenzeile fixiert.

```
+-----------------------------------------------------------------------------------------------+
| [Viewport 0: XY-Ebene]     [Viewport 1: XZ-Ebene]     [Viewport 2: YZ-Ebene]     [Viewport 3] |
|    . . ++ . . .               . . . . . . .              . . -- . . .               . . * . . |
|    . ++ ++ . . .              . . . . . . .              . -- -- . . .              . * * . . |
|    (10x10 Gitter)             (10x10 Gitter)             (10x10 Gitter)             (Raumdiag)|
+-----------------------------------------------------------------------------------------------+
| TELEMETRIE: Tick: 0000842 | Active Nodes: 12405 | Target Invariance: 4892100                  |
+-----------------------------------------------------------------------------------------------+
| LIVE LOG BUFFER (Scrollt autonom über der Eingabezeile weg)                                  |
| [PERF] Last Tick:  0.8412 ms | Total Run-Time:  12.50 Sek.                                    |
| [FIELD] Schalldruck p: 0.0412 | Schallschnelle v_s: 0.1245 | Schallstaerke I: 0.0051          |
| [CHEM] Elemente (Atome): 2 | Synthetisierte Verbindungen (Molekuele): 1                       |
+-----------------------------------------------------------------------------------------------+
| [MiniVers-Control]:> set acou 1 5.0 100_                                                     |
+-----------------------------------------------------------------------------------------------+

```

### Die Visualisierungs-Zellen (ASCII-Token)

Der `PhysicsCellRenderer` übersetzt die anonymen Gitterzustände und lokalen Druckgradienten flimmerfrei in diskrete ASCII-Zeichen:

* ` ` (Leerzeichen): Reines Quanten-Vakuum. Keine Ladungen, Ströme oder Massen aktiv.
* `++` / `--`: Lokale Konzentrationen positiver (Plus-Polarität) oder negativer (Minus-Polarität) Ladungen.
* `##`: Neutrale Ladungszustände schwerer Materie-Inseln.
* ``: Kinetischer Photonen-Fluss im freien Raum.
* `@` / `%` / `:` / `.`: Lokale Druckverdichtungen (Wellenberge) und abfallende Signal-Grenzschichten.

### Das unbestechliche Telemetrie-Register

* **Tick:** Der aktuelle evolutionäre Zeitschritt der Simulation.
* **Active Nodes:** Die Kardinalität des *Sparse-Tracking-Puffers*. Nur geänderte Netzknoten belegen Rechenzeit.
* **Target Invariance:** Die bitgenaue Erhaltungssumme aller Quanten-Bits beim Urknall. Jede Abweichung demaskiert sofort ein Informationsleck im mathematischen Modell.

---

## 4. Die 7 Physikalischen Überwachungsmodule

Über das compile-time optimierte X-Macro-Verfahren (`OBSERVER_MODULE_LIST`) speist der Observer die Simulationsdaten im Takt-Intervall sequentiell und ohne Funktionszeiger-Overhead durch sieben spezialisierte Diagnose-Engines:

1. **`mech` (Mechanik):** Evaluiert Massen-Baryzentren, Translationen, Rotationen, Newtonsche Impulserhaltungen sowie Strömungsviskositäten.
2. **`acou` (Akustik):** Misst Schalldruck, Schallschnelle, Wellen-Superpositionen (Schwebungen), Doppler-Effekte und diatonische Tonsysteme.
3. **`thermo` (Thermodynamik):** Überwacht polytrope Zustandsänderungen, Entropieproduktion, Carnot-Wirkungsgrade und Maxwell-Boltzmann-Geschwindigkeitsverteilungen.
4. **`optics` (Optik):** Analysiert fotometrische Lichtstärken, Reflexionen, Brechungsindizes (Prismen-Dispersion) sowie gravitative Lichtablenkungen.
5. **`elec` (Elektrotechnik):** Trackt Ladungs- und Verschiebungsströme, Ohmsche Lastwiderstände, Induktionsgesetze sowie die Resonanzfrequenzen von LC-Schwingkreisen.
6. **`chem` (Chemie):** Identifiziert molekulare Clusterstrukturen, offene Oberflächen-Valenzen, Reaktionsenthalpien ($\Delta H$) und chirale Spinkatalysatoren.
7. **`nucl` (Kernphysik):** Bilanziert Kernumwandlungen, atomare Massendefekte, Neutronen-Multiplikationsfaktoren ($k$) und absorbiere Strahlungsdosierungen.

---

## 5. Das interaktive Live-Gating (Formelprüfstand-Labor)

Das mächtigste Feature des modernisierten Observers ist die **Transformation zum mathematischen Echtzeit-Labor**. Über das Kontrollregister können Sie den Modulen im laufenden Betrieb physikalische Randbedingungen injizieren. Die Triebwerke berechnen daraufhin die klassischen Lehrbuch-Formeln und validieren diese direkt gegen die emersierte LGA-Gitterphysik des RAMs.

### Die Befehlssyntax (Eingabe in der Sticky Line)

```bash
set <mod> <test_id> <intensity> [custom_param]

```

### Das globale Labor-Prüfregister

| Modul (`mod`) | Test-ID | Physikalische Modell-Formel | `intensity` (Soll-Vorgabe) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- | --- |
| **`mech`** | **1** | Arbeit-Energie-Theorem: $W = \Delta E_{\text{kin}}$ | Erwartete $\Delta E_{\text{kin}}$-Änderung | *Inaktiv* |
|  | **2** | Ballistische Trajektorie: $\theta = \arctan2(v_z, v_{\text{horiz}})$ | Erwarteter Wurfwinkel in Grad | *Inaktiv* |
|  | **3** | Archimedisches Hebelgesetz: $\Delta M = (m_1 \cdot s_1) - (m_2 \cdot s_2)$ | Erwartetes Drehmoment-Delta | *Inaktiv* |
| **`acou`** | **1** | Diskrete Signal-Laufzeit: $c_s = \frac{\Delta s}{\Delta t}$ | Erwartete Schallgeschwindigkeit | Messdistanz in Sektoren |
|  | **2** | Wellenfront-Stauchung: $\text{Ratio} = \frac{1 + v/c}{1 - v/c}$ | Oszillationskraft am Bug | Radius des Messfensters |
|  | **3** | Diatonische Tonskalen: $f_n = f_0 \cdot \text{Ratio}_n$ | Soll-Frequenz relativ zum Kammerton | Akkord-Stufe ($1$ bis $8$) |
| **`thermo`** | **1** | Realgas-Zustandsgleichung: $Z = \frac{p \cdot V}{n \cdot R \cdot T}$ | Erwarteter Kompressibilitätsfaktor | *Inaktiv* |
|  | **2** | Carnot-Kreisprozess-Limit: $\eta = 1 - \frac{T_{\text{cold}}}{T_{\text{hot}}}$ | Erwartete Kühleffizienz $\eta$ | Kalt-Temperatur $T_{\text{cold}}$ |
|  | **3** | Kinetisches Stoßgesetz: $\lambda = \frac{1}{\sqrt{2} \cdot n \cdot \sigma}$ | Erwartete mittlere Weglänge | *Inaktiv* |
| **`optics`** | **1** | Fotometrische Absorption: $I = I_0 \cdot e^{-\mu \cdot x}$ | Dämpfungskoeffizient $\mu$ | Geometrische Materialdicke |
|  | **2** | Geometrische Abbildungsgleichung: $\frac{1}{f} = \frac{1}{g} + \frac{1}{b}$ | Erwartete Brennweite $f$ | *Inaktiv* |
|  | **3** | Gravitative Lichtablenkung (Einstein-Lensing) | Erwarteter Ablenkwinkel in rad | *Inaktiv* |
| **`elec`** | **1** | Ohmsches Maschen-Netzwerk: $R = \frac{U}{I}$ | Erwarteter Lastwiderstand $R$ | *Inaktiv* |
|  | **2** | Thomsonsche Schwingungsgleichung: $f = \frac{1}{2\pi \sqrt{L \cdot C}}$ | Kapazitätswert $C$ | Induktivitätswert $L$ |
|  | **3** | Induktive Transformator-Kopplung: $u_e = \frac{U_2}{U_1}$ | Soll-Übersetzungsverhältnis | *Inaktiv* |
| **`chem`** | **1** | Stoffmengen-Invarianz (Lavoisier-Massenbilanz) | *Inaktiv* (Erhaltungs-Audit) | *Inaktiv* |
|  | **2** | Arrhenius-Reaktionskinetik: $k = A \cdot e^{-\frac{E_a}{T}}$ | Aktivierungsenergie $E_a$ | Externe Test-Temperatur $T$ |
|  | **3** | Faradaysches Elektrolyse-Gesetz: $m = \left(\frac{M}{z \cdot F}\right) \cdot I \cdot t$ | Injizierter Strom $I$ | Einwirkungszeit $t$ in Ticks |
| **`nucl`** | **1** | Neutronen-Multiplikationsfaktor: $k = \frac{N_{\text{neu}}}{N_{\text{alt}}}$ | Erwarteter Kritikalitätsfaktor | *Inaktiv* |
|  | **2** | Thermonukleares Plasma-Gating (Coulomb-Wall) | Kinetische Fusionsbarriere | *Inaktiv* |
|  | **3** | Lambert-Beersches Teilchen-Schwächungsgesetz | Schwächungskoeffizient $\mu$ | Dicke der Schutzbarriere |

### Praktische Anwendungsbeispiele

* **`set acou 1 5.0 100`**
Aktiviert die Schallgeschwindigkeitsmessung. Das Modul stoppt die Ticks, die ein Signal über 100 Gitterzellen benötigt, und gleicht das Ergebnis gegen die Soll-Vorgabe von 5.0 ab.
* **`set mech 2 45.0 0`**
Zwingt das ballistische Analysezentrum, die Flugbahn fliegender Masseninseln auszuwerten und den realen, emergenten Auswurfwinkel gegen das mathematische Ideal von $45.0^{\circ}$ zu prüfen.
* **`set thermo 2 0.65 5`**
Simuliert einen thermodynamischen Carnot-Prozess und gleicht die im Gitter emersierte Volumenarbeit live gegen einen Soll-Wirkungsgrad von $65.0\,\%$ bei einer Kältesenke von 5 Einheiten ab.
* **`set nucl 0 0 0`**
Setzt das nukleare Modul wieder in den passiven Standardwächter-Modus zurück.