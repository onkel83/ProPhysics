# Modul-Handbuch: Elektrotechnik, Feldanalytik & Interaktiver Prüfstand (`Mod_Electrotech.c`)

Dieses Modul bildet das telemetrische Rückgrat für die Erfassung und Validierung sämtlicher elektrodynamischer Phänomene innerhalb deines Repositoriums `onkel83/prophysics`. Es analysiert das flache SoA-Array (Structure of Arrays) der `proPhysics.c` und transformiert den passiven Observer in einen interaktiven Echtzeit-Prüfstand, mit dem simulierte Netzwerke direkt gegen die klassischen Gesetze der Elektrotechnik abgeglichen werden.

---

## 1. Das elektrodynamische Axiomen-Mapping: "It from Bit"

Da die Simulation auf einem rein diskreten Lattice-Gas-Automaten (FCC-Gitter) basiert, werden makroskopische Maxwell-Größen direkt aus den binären Zuständen des RAM-Substrats abgeleitet:

* **Elektrischer Strom ($I$):** Definiert über den Netto-Massen- und Ladungsfluss der Photonenbits (`0x0FFF`) durch eine vordefinierten Sektorengrenze (X-Schnittstelle).


* **Elektrische Spannung ($U$):** Ergibt sich branchlos aus dem lokalen Druckgradienten (`ProPhysics_Query_Local_Pressure`) zwischen Quelle und Senke.


* **Elektrisches Feld ($E_x$):** Berechnet aus dem räumlichen Differenz-Gradienten der Polaritäten geladener Teilchen-Inseln entlang der Koordinatenachsen.


* **Magnetisches Feld ($B_z$):** Resultiert direkt aus der Spinausrichtung (`QUANTUM_SPIN_CW/CCW`) und der akkumulierten Masse der aktiven Knoten.



---

## 2. Das interaktive Test-Labor (Command-Gating)

Über den fixierten Sticky Prompt (`MiniVers-Control`) am unteren Konsolenrand kann das Elektrotechnik-Modul zur Laufzeit über das globale Kontrollregister gesteuert werden. Die Syntax lautet starr:

```bash
set elec <test_id> <intensity> [custom_param]

```

### Die Test-Konfigurationen im Detail:

| Test-ID | Bezeichnung | `intensity` (Bedeutung) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- |
| **`0`** | **Passive Diagnose** | *Inaktiv* | *Inaktiv* (Standard-Feldwächter)

 |
| **`1`** | **Ohmsches Last-Audit** | Erwarteter Soll-Widerstand $R$ | *Inaktiv*<br> |
| **`2`** | **Thomson-Resonanz** | Kapazitätswert $C$ | Induktivitätswert $L$<br> |
| **`3`** | **Transformator-Kopplung** | Soll-Übersetzungsverhältnis $u_e$ | *Inaktiv*<br> |

---

## 3. Mathematische Formel-Validatoren

Sobald ein Testmodus aktiviert wird (`test_id > 0`), berechnet das Labor die klassischen elektrotechnischen Kreisformeln im Hintergrund und vergleicht sie bitgenau mit der physikalischen Emergenz des Gitters:

### Test 1: Ohmsches Gesetz & Kirchhoff-Abgleich

Das Labor überwacht die Maschenkonformität des Netzwerks. Aus der im Gitter gemessenen Spannung $U$ und der Stromstärke $I$ wird der reale Widerstand ermittelt:

$$R_{\text{ist}} = \frac{U}{I}$$

* **Abgleich:** Das Ergebnis wird mit der Soll-Vorgabe ($R_{\text{soll}}$, übergeben via `intensity`) verglichen. Größere Differenzen demaskieren einen energetischen Netzwerkdrift im dichten Medium.



### Test 2: Thomsonsche Schwingungsgleichung

Beim Aufbau hochfrequenter Schwingkreise (Kondensator/Spule-Analogien) bestimmt das Modul die Oszillationsfrequenz des elektrischen Feldes über die Thomson-Formel:

$$f_{\text{theorie}} = \frac{1}{2\pi \sqrt{L \cdot C}}$$

* **Abgleich:** Die Werte für $C$ (`intensity`) und $L$ (`custom_param`) generieren die Soll-Frequenz, welche direkt gegen die real gemessenen Feld-Flips ($f_{\text{ist}}$) der kinetischen Bits im RAM abgeglichen wird.



### Test 3: Induktive Kopplung & Transformator-Übersetzung

Misst die drahtlose Spannungsübertragung zwischen dem Primärsektor ($X=256$) und dem Sekundärsektor ($X=768$) über den induzierten magnetischen Fluss. Das ideale Spannungsverhältnis folgt der Windungsgleichung:

$$u_e = \frac{U_2}{U_1} = \frac{N_2}{N_1}$$

* **Abgleich:** Der gemessene Transformationsfaktor wird direkt gegen die Soll-Übersetzung (`intensity`) geprüft, um Streuverluste des chiralen Spin-Feldes zu isolieren.



---

## 4. Steuerungs-Beispiele im Live-Betrieb

1. **Rückkehr zur passiven Feldüberwachung:**
```bash
set elec 0 0 0

```


Deaktiviert alle Labor-Validatoren. Das Modul läuft wieder als reiner Hintergrund-Observer.


2. **Validierung einer Ohmschen Lastleitung:**
```bash
set elec 1 12.5 0

```


Schaltet den Ohmschen Prüfstand scharf und vergleicht das Spannungs-Strom-Verhältnis im Gitter gegen den Soll-Widerstand von $12.5\,\Omega_{\text{bit}}$.


3. **LC-Resonanzprüfung zünden:**
```bash
set elec 2 0.047 10

```


Injiziert die Kreis-Parameter $C = 0.047$ und $L = 10$, berechnet die Thomson-Frequenz und überwacht die zelluläre Schwingungs-Invarianz.


4. **Transformator-Kopplungsgrad messen:**
```bash
set elec 3 0.5 0

```


Prüft, ob die induzierte Sekundärspannung exakt dem abtransformierten Verhältnis von 1:2 ($u_e = 0.5$) entspricht.