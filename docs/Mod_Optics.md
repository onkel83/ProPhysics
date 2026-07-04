# Modul-Handbuch: Optik, Fotometrie & Relativistischer Prüfstand (`Mod_Optics.c`)

Dieses Modul bildet die Schnittstelle zur tiefgehenden Analyse und Steuerung wellen- und geometrieoptischer Phänomene innerhalb deines Repositoriums `onkel83/prophysics`. Es demaskiert die diskreten Bewegungsmuster von Photonenbits (`0x0FFF`) im RAM-Substrat der `proPhysics.c` vollkommen branchlos über hardwarebeschleunigte Bit-Masken. Über das interaktive Kontrollregister transformiert es sich in ein optisches Messlabor, das Absorptionsraten, Linsenbrennweiten und sogar geodätische Lichtablenkungen live gegen die klassischen Feldformeln prüft.

---

## 1. Das optische Axiomen-Mapping: "It from Bit"

Da die Simulation auf einem diskreten FCC-Lattice (Face-Centered Cubic) operiert, emergieren elektromagnetische Lichtwellen direkt aus den rein quantisierten Zuständen der Zellkanäle:

* **Das Photon (Lichtbit):** Ein masseloses, kinetisches Bit (`0x0FFF`), das sich invariant mit der absoluten Gitter-Lichtgeschwindigkeit $c = 1.0$ (Sektoren/Tick) durch das zelluläre Vakuum bewegt.


* **Die Reflexion (Spiegelung):** Trifft ein Lichtbit auf dichte Trägheitsmaterie (`0x1000`), blockiert der elastische LGA-Rückprall die Vorwärtsachse. Die Umkehrung der Bit-Richtung im flachen RAM-Substrat verifiziert das Reflexionsgesetz (Einfallswinkel = Reflexionswinkel) auf zellulärer Ebene.


* **Die Brechung & Dispersion (Prisma):** Durch dichte Massecluster hindurch verlangsamt sich der effektive Vorwärtsstrom der Bits, da Kaskadenkollisionen Mikroverzögerungen erzeugen. Unterschiedliche Bit-Muster (Frequenzanalogien) reagieren anisotrop – das führt zur optischen Aufspaltung (Dispersion).


* **Die Gravitationslinse (Einstein-Ablenkung):** Schwere Planetenkörper (`0x1000`) erzeugen asymmetrische Sektordrücke in ihrer unmittelbaren Peripherie. Photonenbits, die diese Hochdruckfronten tangential passieren, erfahren eine topologische Richtungsänderung aus ihrer primären Achse (geodätische Bit-Krümmung).



---

## 2. Das interaktive Test-Labor (Command-Gating)

Über den fixierten Sticky Prompt (`MiniVers-Control`) der `Observer.c` steuerst du das optische Gating-Register im laufenden Betrieb. Die Syntax folgt der unbiegsamen Struktur:

```bash
set optics <test_id> <intensity> [custom_param]

```

### Die Test-Konfigurationen im Detail:

| Test-ID | Bezeichnung | `intensity` (Bedeutung) | `custom_param` (Bedeutung) |
| --- | --- | --- | --- |
| **`0`** | **Passive Diagnose** | *Inaktiv* | *Inaktiv* (Standard-Optikwächter)

 |
| **`1`** | **Lambert-Beer-Transmission** | Dämpfungskoeffizient $\mu$ | Geometrische Dicke $x$ (in Zellen)

 |
| **`2`** | **Abbildungsgleichung** | Soll-Brennweite $f$ | *Inaktiv*<br> |
| **`3`** | **Gravitationslinsen-Audit** | Erwarteter Ablenkwinkel in rad | *Inaktiv*<br> |

---

## 3. Mathematische Formel-Validatoren

Sobald du ein Experiment über eine `test_id > 0` zündest, blendet das Modul die standardmäßigen Telemetriemeldungen aus und gleicht das Gitter-Verhalten live gegen die mathematische Theorie ab:

### Test 1: Fotometrisches Dämpfungsgesetz

Das Labor überprüft die Lichtdurchlässigkeit von künstlich platzierten Materialstrukturen. Der theoretische Transmissionswert folgt dem klassischen Lambert-Beerschen Abschwächungsgesetz:

$$I_{\text{theorie}} = I_0 \cdot e^{-\mu \cdot x}$$

* **Abgleich:** Der Dämpfungsfaktor $\mu$ (`intensity`) und die Dicke der Barriere $x$ (`custom_param`) definieren das mathematische Ideal. Das Modul misst die reale Bit-Anisotropie hinter dem Hindernis im RAM und wirft den absoluten Differenzfehler aus.



### Test 2: Geometrische Abbildungsgleichung

Misst die geometrische Brennpunkt-Konvergenz von zellulär emersierten Linsen- und Hohlspiegelstrukturen im Zentrum des Gitters. Das ideale System gehorcht der optischen Linsengleichung:

$$\frac{1}{f} = \frac{1}{g} + \frac{1}{b}$$

* **Abgleich:** Das Modul scannt die Fokuszone asynchron über die lokalen Druckmaxima, bestimmt die reale Brennweite $f_{\text{ist}}$ und spiegelt die exakte Sektoren-Abweichung zur Soll-Vorgabe (`intensity`) im Kontrollraum.



### Test 3: Relativistische Lichtablenkung

Das Labor wertet die photonischen Richtungs-Flips in der unmittelbaren Flugperipherie schwerer Masseninseln aus. Es berechnet die mittlere Krümmung der emersierten Bit-Geodäten.

* **Abgleich:** Der reale, akkumulierte Ablenkwinkel der abgelenkten Photonenströme wird direkt mit deiner radianten Formelvorgabe (`intensity`, z.B. **0.0872 rad** $\approx$ **5°**) verglichen, um den Brechungsindex des reinen Quantenvakuums unter Gravitationsdruck zu isolieren.



---

## 4. Steuerungs-Beispiele im Live-Betrieb

1. **Rückkehr zur passiven Feldüberwachung:**
```bash
set optics 0 0 0

```


Deaktiviert den Formelprüfstand. Das Modul läuft wieder als reiner Hintergrundwächter für Spektren und Polarisation.


2. **Hitzeprüfung der Lichtdurchlässigkeit:**
```bash
set optics 1 0.15 8

```


Schaltet den Transmissions-Prüfstand scharf und vergleicht den Photonenstrom hinter einer 8 Zellen dicken Struktur mit dem theoretischen Lambert-Beer-Absorptionswert bei $\mu = 0.15$.


3. **Linsenbrennweite kalibrieren:**
```bash
set optics 2 25.0 0

```


Zwingt das Analysezentrum zum permanenten Abgleich der realen Gitter-Brennpunktkonvergenz gegen den Soll-Wert von 25.0 Sektoreinheiten.


4. **Einstein-Lensing simulieren und validieren:**
```bash
set optics 3 0.0872 0

```


Aktiviert das relativistische Überwachungszentrum und misst die gravitative Ablenkungsrate der geodätischen Bit-Flüge im dichten Druckfeld.