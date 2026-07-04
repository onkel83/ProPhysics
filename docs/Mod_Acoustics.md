# Modul-Handbuch: Akustik & Echtzeit-Feldgating (`Mod_Acoustics.c`)

Dieses Modul ist die offizielle Schnittstelle zur Analyse und interaktiven Manipulation von **akustischen Wellenfeldern, Druckoszillationen und Signalinterferenzen** innerhalb der zellulären Gitter-Anomalie (`MiniVers`). Es wertet das flache RAM-Substrat der `proPhysics.c` in Echtzeit aus und ermöglicht über das globale Gating-Register gezielte akustische Injektionen.

---

## 1. Physikalisches Mapping: "It from Bit"

Da das Universum auf einem diskreten FCC-Gitter (Face-Centered Cubic) operiert, existiert kein kontinuierliches Trägermedium für Schallwellen. Stattdessen emergieren akustische Phänomene direkt aus der **lokalen Bit-Dichte und Impulsübertragung**:

* **Schalldruck ($p$):** Die Abweichung der lokalen Knotendichte (`ProPhysics_Query_Local_Pressure`) vom statischen Hintergrund-Gleichgewichtswert ($1.0$).
* **Schallschnelle ($\vec{v}_s$):** Der vektorielle Netto-Fluss der kinetischen Photonen-Bits (`0x0FFF`) entlang der 12 axialen Flugkanäle pro Zeitschritt (Tick).
* **Schallgeschwindigkeit ($c_s$):** Im reinen Quanten-Vakuum wandern Druckimpulse streng invariant mit $c = 1.0$ (Zellen/Tick). Trifft die Wellenfront auf schwere Masseninseln (`0x1000`), wird der Impuls durch den elastischen Rückprall gestaut – die Schallgeschwindigkeit sinkt materialabhängig ab.

---

## 2. Visuelle Repräsentation im Dashboard

Wenn akustische Wellen durch die vier Viewports des `ProDiBatch`-Dashboards laufen, werden die lokalen Druckgradienten durch den `PhysicsCellRenderer` in folgende ASCII-Token übersetzt:

| Symbol | Druckbereich (Local Pressure) | Physikalischer Zustand |
| --- | --- | --- |
| **`**`** | *Kinetischer Fluss aktiv* | Akute Wellenfront / Fliegende Signal-Bits |
| **`@`** | $> 1.2$ | Extreme Hochdruckverdichtung (Wellenberg) |
| **`%`** | $> 0.7$ | Moderate Verdichtungsphase |
| **`:`** | $> 0.3$ | Diskrete Signal-Grenzschicht |
| **`.`** | $> 0.05$ | Minimale Druckfluktuation |
| **` `** | $\le 0.05$ | Akustisches Vakuum / Wellental |

---

## 3. Das interaktive Test-Labor (Command-Gating)

Über den fixierten Konsolen-Prompt (`MiniVers-Control`) kann das Akustik-Modul im laufenden Betrieb manipuliert werden. Der Befehl folgt der starr typisierten Syntax:

```bash
set acou <test_id> <intensity> [custom_param]

```

### Verfügbare Test-Konfigurationen:

| Test-ID | Bezeichnung | `intensity` (Bedeutung) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- |
| **`0`** | **Passive Diagnose** | *Keine Auswirkung* | *Keine Auswirkung* (Standardmodus) |
| **`1`** | **Schallgeschwindigkeits-Test** | Erwartete Soll-Geschwindigkeit | Messdistanz in Gitter-Sektoren |
| **`2`** | **Doppler-Validierung** | Oszillationskraft am Bug | Radius des Messfensters |
| **`3`** | **Tonsystem-Komparator** | Frequenz-Vorgabe (Hz-Äquivalent) | Akkord-Stufe ($1$ bis $8$) |

---

## 4. Funktionsweise der mathematischen Validatoren

Sobald ein Testmodus mit `test_id > 0` aktiviert wird, schaltet das Modul von der reinen Protokollierung in den **Formel-Abgleichs-Modus**.

### Test 1: Laufzeit- & Geschwindigkeits-Audit

Das Modul trackt die Ticks, die ein emittierter Druckpuls benötigt, um die Distanz (`custom_param`) zu durchqueren. Es berechnet live die emersierte Schallgeschwindigkeit:

$$c_{s,\text{ist}} = \frac{\Delta s}{\Delta t}$$

Das Ergebnis wird direkt gegen die Soll-Vorgabe (`intensity`) abgeglichen. Weicht das Gitter-Ergebnis ab, schlägt die Telemetrie wegen *Reaktionsdrift* an.

### Test 2: Doppler-Kompression

Misst die Stauchung der Wellenfronten vor und hinter einer beschleunigten `QuantumStateIsland`. Das Modul berechnet das theoretische Frequenzverhältnis nach der klassischen Doppler-Formel:

$$\text{Ratio}_{\text{theorie}} = \frac{1 + \frac{v}{c}}{1 - \frac{v}{c}}$$

Dieses mathematische Ideal wird live mit dem realen Druckverhältnis der Bug- und Heckwelle im Gitter verglichen, um den Grad der nichtlinearen Kompression im dichten Medium zu bestimmen.

### Test 3: Diatonischer Frequenzabgleich

Vergleicht die gemessene Schwingungsfrequenz der Gitterpunkte mit den mathematischen Verhältnissen der reinen Stimmung relativ zum vordefinierten Kammerton ($0.0440\,\text{Tick}^{-1}$).

---

## 5. Steuerungs-Beispiele im Live-Betrieb

1. **Rückkehr zum Normalbetrieb:**
```bash
set acou 0 0 0

```


*Deaktiviert alle künstlichen Berechnungen. Das Modul arbeitet wieder als passiver Wächter.*
2. **Schallgeschwindigkeit auf Prüfstand stellen:**
```bash
set acou 1 2.5000 100

```


*Weist das Modul an, eine Punkt-zu-Punkt-Messung über 100 Sektoren durchzuführen und gegen den mathematischen Soll-Wert von 2.5000 abzugleichen.*
3. **Doppler-Wellenstauchung analysieren:**
```bash
set acou 2 1.0 5

```


*Triggert den Doppler-Formel-Validator im Radius von 5 Sektoren um das primäre Masse-Island herum.*