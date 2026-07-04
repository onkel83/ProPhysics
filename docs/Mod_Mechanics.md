# Modul-Handbuch: Klassische Mechanik, Kinematik & Interaktiver Prüfstand (`Mod_Mechanics.c`)

Dieses Modul bildet das fundamentale Fundament zur Überwachung und mathematischen Verifikation aller mechanischen, kinematischen und statischen Phänomene innerhalb deines Universums. Es liest das flache, cache-optimierte Gitter-Substrat der `proPhysics.c` über branchlose Bit-Masken aus und transformiert den Observer in ein interaktives Live-Labor. Dadurch können Newtonsche Axiome, Wurfbahnen und Drehmomente direkt zur Laufzeit gegen die emersierte Physik validiert werden.

---

## 1. Das mechanische Axiomen-Mapping: "It from Bit"

Da das System auf einem diskreten Lattice-Gas-Automaten (FCC-Gitter) operiert, werden kontinuierliche klassische Erhaltungsgrößen direkt aus den binären Gitterzuständen berechnet:

* **Träge Masse ($m$):** Wird durch das Vorhandensein des schweren Materie-Bits (`0x1000`) an einem Netzknoten repräsentiert und über das `mass_accumulator`-Register im Datenpool gewichtet.


* **Raumkoordinaten ($x, y, z$):** Werden zur Vermeidung von Divisions-Overhead über hardwarenahe Bit-Shifts und Bit-Masken aus dem flachen Positionsindex extrahiert:


* $x = \text{idx} \,\,\& \,\,0\text{x3FF}$ (10 Bits)


* $y = (\text{idx} \gg 10) \,\,\& \,\,0\text{x1FF}$ (9 Bits)


* $z = \text{idx} \gg 19$ (9 Bits)




* **Impuls ($\vec{p}$) & Kinetische Energie ($E_{\text{kin}}$):** Berechnen sich direkt aus den im Datenpool der `QuantumStateIsland`-Strukturen hinterlegten Geschwindigkeitsvektoren ($v_x, v_y, v_z$) multipliziert mit der akkumulierten Insel-Masse.


* **Lokale Kraft ($\vec{F}$):** Wird im zellulären Raum über die räumliche Differenz der Druckgradienten (`ProPhysics_Query_Local_Pressure`) zwischen benachbarten Gitterzellen ermittelt.



---

## 2. Das interaktive Test-Labor (Command-Gating)

Über den sticky Konsolen-Prompt (`MiniVers-Control`) der `Observer.c` steuerst du das mechanische Prüfstand-Register zur Laufzeit. Die Befehlssyntax folgt dem starr typisierten Format:

```bash
set mech <test_id> <intensity> [custom_param]

```

### Die Test-Konfigurationen im Detail:

| Test-ID | Bezeichnung | `intensity` (Bedeutung) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- |
| **`0`** | **Passive Diagnose** | *Inaktiv* | *Inaktiv* (Standardwächter-Modus)|
| **`1`** | **Arbeit-Energie-Theorem** | Erwartete $\Delta E_{\text{kin}}$-Änderung | *Inaktiv*<br> |
| **`2`** | **Ballistischer Wurf** | Erwarteter Wurfwinkel in Grad | *Inaktiv*<br> |
| **`3`** | **Archimedischer Hebel** | Erwartetes Drehmoment-Delta | *Inaktiv*<br> |

---

## 3. Mathematische Formel-Validatoren

Sobald du einen Testmodus aktivierst (`test_id > 0`), schaltet das Modul die standardmäßigen Hintergrundberichte ab und zwingt das System in den **Formel-Vergleichs-Modus**:

### Test 1: Arbeit-Energie-Theorem ($W = \Delta E_{\text{kin}}$)

Das Labor aggregiert die verrichtete Druck-Arbeit $W$ aller bewegten Masseknoten durch die vektorielle Multiplikation von lokaler Feldkraft und realer Inselgeschwindigkeit pro Zeitschritt:

$$W = \sum (\vec{F} \cdot \vec{v})$$

* **Abgleich:** Das Modul berechnet die reale Differenz der kinetischen Gesamtenergie zwischen dem aktuellen und dem vorherigen Takt ($\Delta E_{\text{kin}} = E_{\text{kin, aktuell}} - E_{\text{kin, vorher}}$). Weicht die berechnete Feldarbeit vom kinetischen Energie-Wechsel ab, schlägt das Labor Alarm.



### Test 2: Ballistische Trajektorien-Analyse

Beim Abschuss oder der Beschleunigung von Materie-Islands isoliert das Modul den horizontalen Geschwindigkeitsvektor ($v_{\text{horiz}} = \sqrt{v_x^2 + v_y^2}$) und berechnet über die Umkehrfunktion des Tangens den exakten emergenten Abschusswinkel:

$$\theta = \arctan2(v_z, v_{\text{horiz}}) \cdot \frac{180}{\pi}$$

* **Abgleich:** Dieser Wert wird direkt mit deiner numerischen Winkelvorgabe (`intensity`) verglichen, um den gravitativen Einfluss des umgebenden Quantenrauschens auf die Wurfparabel zu bestimmen.



### Test 3: Archimedisches Hebelgesetz

Das Labor berechnet das Drehmoment-Gleichgewicht bezogen auf das globale Massen-Baryzentrum (Zentralbaryzentrum) der Simulation. Es bilanziert die Drehmomente der linken und rechten Systemhälfte:

$$\Delta M = (m_{\text{left}} \cdot s_{\text{left}}) - (m_{\text{right}} \cdot s_{\text{right}})$$

* **Abgleich:** Das emersierte Drehmoment-Delta wird live gegen deine Soll-Vorgabe abgeglichen. Ein stabiles Gleichgewicht (Zweiseitiger Hebel) liegt bei exakt `0.0000` vor.



---

## 4. Steuerungs-Beispiele im Live-Betrieb

1. **Zurückschalten auf passive Telemetrie:**
```bash
set mech 0 0 0

```


Deaktiviert den Formelprüfstand. Das Modul protokolliert wieder die mechanischen Standard-Zustände im Hintergrund.


2. **Validierung der mechanischen Energieerhaltung:**
```bash
set mech 1 0.0 0

```


Schaltet den integrierten Energie-Arbeits-Komparator scharf und deckt instantan Energieverluste oder dissipative Effekte im Gitter auf.


3. **Prüfung einer $45^{\circ}$ Wurfparabel:**
```bash
set mech 2 45.0 0

```


Zwingt das ballistische Analysezentrum zum permanenten Abgleich der realen Flugbahn gegen den idealen theoretischen Abschusswinkel von $45.0\,\text{Grad}$.


4. **Hebelgesetz-Gleichgewicht überwachen:**
```bash
set mech 3 0.0 0

```


Aktiviert das statische Drehmoment-Audit, um asymmetrische Masseverteilungen an den Flanken der toroidalen Raumzeit-Matrix zu detektieren.