# Modul-Handbuch: Kernphysik, Hochenergetik & Interaktiver Prüfstand (`Mod_Nuclear.c`)

Dieses Modul bildet das telemetrische Kontrollzentrum für die Erfassung, Analyse und interaktive Validierung hochenergetischer Kernprozesse innerhalb deines Repositoriums `onkel83/prophysics`. Es wertet das flache, multi-threaded simulierte Gitter-Substrat der `proPhysics.c` über branchlose Bit-Masken aus. Über das globale Steuerregister transformiert es den Observer in einen Live-Prüfstand, mit dem atomare Kritikalitäten, Fusionsbarrieren und Strahlungsabschirmungen direkt gegen klassische Lehrbuch-Formeln abgeglichen werden.

---

## 1. Das kernphysikalische Axiomen-Mapping: "It from Bit"

Innerhalb der toroidalen Raumzeit-Matrix deines Gitterautomaten emergieren kernphysikalische Phänomene direkt aus der Interaktion von diskreten Masse-Bits, lokalen Druckfeldern und Polaritätszuständen:

* **Uranspaltung (Fission):** Ein schwerer Kern-Cluster (`QuantumStateIsland`) gerät unter extremen Gitterkompressionsdruck. Übersteigt die lokale Dichte den kritischen Schwellenwert, dissoziieren die Massezellen (`0x1000`) deterministisch in leichtere Fragmente.
* **Kernfusion (Fusion):** Zwei beschleunigte, leichte Masse-Inseln überwinden die elektrostatische Abstoßungsbarriere (`repel`) und verschmelzen zu einem hyper-dichten Verbund. Die überschüssige strukturelle Bindungsenergie wird als massiver Schwall freier Photonen-Bits in die Flugkanäle entladen.
* **Neutronen-Analogie:** Freie, ungeladene, solitäre Flussbits (`0x0FFF` ohne Masse-Kopplung), die sich branchlos durch das Vakuum bewegen und als statistische Trigger für Folge-Spaltungen wirken.
* **Strahlungsabschirmung:** Dichte Ansammlungen von inaktiven Massezellen wirken als Absorptionsbarriere. Sie fangen emittierte Kinetikbits über elastische Rückprall-Kollisionen ab und dämpfen den Teilchenstrom im dahinterliegenden Sektor.

---

## 2. Das interaktive Test-Labor (Command-Gating)

Über den fixierten Sticky Prompt (`MiniVers-Control`) am unteren Konsolenrand steuerst du das nukleare Prüfstand-Register im laufenden Betrieb. Die Syntax lautet starr:

```bash
set nucl <test_id> <intensity> [custom_param]

```

### Die Test-Konfigurationen im Detail:

| Test-ID | Bezeichnung | `intensity` (Bedeutung) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- |
| **`0`** | **Passive Diagnose** | *Inaktiv* | *Inaktiv* (Standard-Hintergrundwächter)

 |
| **`1`** | **Kritikalitäts-Audit** | Erwarteter Multiplikationsfaktor $k$ | *Inaktiv*<br> |
| **`2`** | **Thermonukleares Gating** | Kinetische Fusions-Energiebarriere | *Inaktiv*<br> |
| **`3`** | **Abschirmungs-Prüfstand** | Schwächungskoeffizient $\mu$ | Dicke der Barriere $x$ (in Zellen)

 |

---

## 3. Mathematische Formel-Validatoren

Sobald ein Testmodus mit `test_id > 0` aktiviert wird, schaltet das Modul die standardmäßige Hintergrundtelemetrie ab und zwingt das Labor in den **Formel-Abgleichs-Modus**:

### Test 1: Kritikalität & Kettenreaktion ($k = \frac{N_{\text{neu}}}{N_{\text{alt}}}$)

Das Labor analysiert das exponentielle Verhalten der Kettenreaktion. Es berechnet live den realen Vermehrungsfaktor $k$ aus dem Verhältnis der aktuell gemessenen freien Trigger-Neutronenbits zum Zustand des vorherigen Ticks:

$$k = \frac{N_{\text{neutron, aktuell}}}{N_{\text{neutron, vorher}}}$$

* **Abgleich:** Das Ergebnis wird direkt gegen deine mathematische Vorgabe ($k_{\text{soll}}$, via `intensity`) abgeglichen. Ein stationärer, kontrollierter Prozess liegt bei exakt $k = 1.0$ vor. Werte über $1.0$ signalisieren eine überkritische, proportionale Expansion.



### Test 2: Thermonukleares Plasma-Gating (Coulomb-Wall)

Das Modul simuliert die energetischen Bedingungen, die für das Aufbrechen der abstoßenden Kernkräfte notwendig sind. Es vergleicht die geforderte energetische Mindestschwelle (`intensity`) mit der real emersierten kinetischen Plasma-Enthalpie nach dem Spaltprozess.

* **Abgleich:** Liegt die vorhandene kinetische Energie über dem geforderten Schwellenwert, meldet das Labor den Durchbruch der Barriere und die Freischaltung der Fusions-Zündung.

### Test 3: Lambert-Beersches Schwächungsgesetz ($I = I_0 \cdot e^{-\mu \cdot x}$)

Das Labor prüft die Effizienz von Strahlenschutzbarrieren im Fernfeld ($X = 900$). Es misst den ungeschützten Eingangsteilchenstrom $I_0$ und berechnet den theoretischen Durchlass hinter einer Barriere der Dicke $x$ (`custom_param`) unter Anwendung des linearen Schwächungskoeffizienten $\mu$ (`intensity`):

$$I_{\text{theorie}} = I_0 \cdot e^{-\mu \cdot x}$$

* **Abgleich:** Dieser theoretische Formelwert wird bitgenau mit der real durchgelassenen Bit-Intensität im geschützten Gittersektor verglichen, um die Absorptions-Konsistenz deines Vakuum-Mediums zu verifizieren.

---

## 4. Steuerungs-Beispiele im Live-Betrieb

1. **Zurückschalten auf passive Überwachung:**
```bash
set nucl 0 0 0

```


Deaktiviert den Formelprüfstand. Das Modul läuft wieder als reiner Hintergrund-Observer.


2. **Validierung der Reaktor-Kritikalität (Stationärer Zustand):**
```bash
set nucl 1 1.0000 0

```


Überwacht das Neutronen-Verhältnis im RAM und gleicht live ab, ob die Kettenreaktion exakt den kritischen Gleichgewichtsfaktor von 1.0000 einhält.


3. **Thermonukleare Fusionsschwelle kalibrieren:**
```bash
set nucl 2 500.0 0

```


Setzt eine energetische Barriere von 500.0 Einheiten an und prüft, ob die lokale Gitterkinetik ausreicht, um eine stabile Kernverschmelzung zu zünden.


4. **Strahlenschutz-Dämpfung auditieren:**
```bash
set nucl 3 0.6931 4

```


Prüft das zelluläre Absorptionsverhalten einer 4 Zellen dicken Blei-Analogie gegen das mathematische Lambert-Beersche Gesetz bei einem Soll-Dämpfungsfaktor von $\mu = 0.6931$.