# Modul-Handbuch: Thermodynamik, Kinetische Gastheorie & Energetischer Prüfstand (`Mod_Thermodynamics.c`)

Dieses Modul dient der Echtzeit-Analyse, Klassifizierung und mathematischen Verifikation aller thermodynamischen Zustandsänderungen, Phasenübergänge und statistischen Teilchenbewegungen innerhalb deines Universums. Durch die direkte Kopplung an das branchlose Gating-Register der `Observer_Config.h` wandelt sich dieses Modul bei Bedarf in ein interaktives Energielabor. Damit lassen sich Gasgesetze, Carnot-Wirkungsgrade und Stoßparameter direkt zur Laufzeit gegen die emersierte zelluläre LGA-Physik validieren.

---

## 1. Physikalisches Mapping: "It from Bit"

Da das System auf einem diskreten FCC-Gitter (Face-Centered Cubic) operiert, emergieren thermodynamische Größen rein statistisch aus der Verteilung der kinetischen Photonen-Bits (`0x0FFF`) und Materie-Bits (`0x1000`):

* **Temperatur ($T$):** Die lokale oder globale Bit-Dichte (Teilchenanzahl pro Einheitsvolumen $V$). Höhere Bit-Konzentrationen in den Flugkanälen repräsentieren hochenergetisches Plasma.
* **Druck ($p$):** Die zelluläre Kraftwirkung auf die Umgebung, ermittelt über die Richtungs-Flussfluktuationen der Netzknoten (`ProPhysics_Query_Local_Pressure`).
* **Volumen ($V$):** Der geometrische Raumabschnitt der toroidalen Matrix, berechnet über branchlose Bit-Masken (`& 0x3FF`, `>> 10`, `>> 19`) zur Vermeidung von Divisions-Overhead.


* **Innere Energie ($U$):** Die Summe der kinetischen Zustände aller aktiven Flusskomponenten.
* **Mittlere freie Weglänge ($\lambda$):** Die durchschnittliche Taktanzahl, die ein masseloses Kinetik-Bit ungestört durchlaufen kann, bevor es auf einem Netzknoten mit einem anderen Bit kombinatorisch kollidiert.

---

## 2. Das interaktive Test-Labor (Command-Gating)

Über den fixierten Sticky Prompt (`MiniVers-Control`) am unteren Konsolenrand steuerst du das thermodynamische Prüfstand-Register zur Laufzeit. Die Syntax folgt dem starr typisierten Format:

```bash
set thermo <test_id> <intensity> [custom_param]

```

### Die Test-Konfigurationen im Detail:

| Test-ID | Bezeichnung | `intensity` (Bedeutung) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- |
| **`0`** | **Passive Diagnose** | *Inaktiv* | *Inaktiv* (Standard-Thermoverschluss)

 |
| **`1`** | **Realgas-Zustand (Z)** | Erwarteter Kompressibilitätsfaktor $Z$ | *Inaktiv* |
| **`2`** | **Carnot-Prozess ($\eta$)** | Erwarteter Wirkungsgrad $\eta_{\text{soll}}$ | Optionale Kalt-Temperatur $T_{\text{cold}}$ |
| **`3`** | **Lambert-Beer-Stoßgesetz** | Erwartete Weglänge $\lambda_{\text{soll}}$ | *Inaktiv* |

---

## 3. Mathematische Formel-Validatoren

Sobald ein Testmodus aktiviert wird (`test_id > 0`), schaltet das Modul die standardmäßige Hintergrundtelemetrie ab und erzwingt das thermodynamische Formellabor:

### Test 1: Realgas-Zustandsgleichung ($Z = \frac{p \cdot V}{n \cdot R \cdot T}$)

Das Labor extrahiert den aktuellen Druck $p$, das effektive Zellvolumen $V$ und die lokale Teilchenanzahl $n$, um den realen Kompressibilitätsfaktor des Gitters zu bestimmen:

$$Z_{\text{ist}} = \frac{p \cdot V}{n \cdot R \cdot T_{\text{local}}}$$

* **Abgleich:** Das Ergebnis wird direkt mit deiner Soll-Vorgabe ($Z_{\text{soll}}$, via `intensity`) abgeglichen. Ideale Gaszustände liegen im LGA bei exakt $1.0000$. Abweichungen demaskieren die Eigenviskosität und den dichten Staudruck des Vakuums unter Kompression.

### Test 2: Thomsonscher Carnot-Wirkungsgrad

Das Labor trackt die im zellulären System verrichtete Volumenarbeit ($dW$) sowie die absorbierte Wärme ($dQ$) über die Ticks hinweg und ermittelt den realen energetischen Ertrag:

$$\eta_{\text{ist}} = \frac{|W_{\text{zyklus}}|}{Q_{\text{absorbiert}}}$$

* **Abgleich:** Dieser Wert wird mit dem theoretischen Carnot-Limit abgeglichen, das sich aus der maximalen Kernplasma-Temperatur ($T_{\text{hot}}$) und der Senken-Temperatur ($T_{\text{cold}}$, via `custom_param`) berechnet:

$$\eta_{\text{carnot}} = 1 - \frac{T_{\text{cold}}}{T_{\text{hot}}}$$

### Test 3: Kinetisches Stoßgesetz ($\lambda = \frac{1}{\sqrt{2} \cdot n \cdot \sigma}$)

Validiert die mikroskopische Stoßtheorie des Lattice-Gases. Das Modul berechnet die mittlere freie Weglänge der Photonenbits basierend auf der aktuellen Bit-Dichte pro Zelle.

* **Abgleich:** Das emersierte Ergebnis wird direkt mit deiner theoretischen Laufweg-Vorgabe (`intensity`) verglichen, um Streu- und Kaskadierungseffekte im dichten Transportmedium mathematisch zu isolieren.

---

## 4. Steuerungs-Beispiele im Live-Betrieb

1. **Rückkehr zur passiven Temperaturüberwachung:**
```bash
set thermo 0 0 0

```


Deaktiviert den Prüfstand. Das Modul arbeitet wieder als reiner Hintergrund-Observer für Entropie und Phasenübergänge.


2. **Validierung der Realgas-Kompressibilität:**
```bash
set thermo 1 1.0000 0

```


*Schaltet den Kompressibilitäts-Komparator scharf, um Abweichungen zum idealen Gasgesetz im komprimierten Gitter-Plasma aufzudecken.*
3. **Carnot-Limit im Kreisprozess prüfen:**
```bash
set thermo 2 0.6500 5

```


*Gibt einen thermodynamischen Soll-Wirkungsgrad von $65.0\,\%$ vor und vergleicht diesen live mit den realen Energieflips zwischen Wärmequelle und Kältesenke.*
4. **Mittlere freie Weglänge messen:**
```bash
set thermo 3 12.4500 0

```


*Überprüft bitgenau, ob die emersierte freie Flugstrecke der masselosen Kinetikbits im RAM mit dem theoretischen Stoßgesetz übereinstimmt.*