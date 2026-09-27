# Views // Content Modules

In diesem Ordner liegen die **HTML-Fragmente**, die den eigentlichen Inhalt der Applikation bilden. Der `prowb` Builder fügt diese Dateien automatisch in die `index.html` ein und generiert passende Buttons in der Navigation.

---

## ⚡ Schnellstart: Neue View anlegen

1.  **Datei erstellen:**
    Erstelle eine neue Datei mit dem Präfix `view_`, z.B. `view_lager_bestand.html`.

2.  **Code einfügen (Template):**
    Kopiere diesen Block in die Datei:

    ```html
    <section id="view_lager_bestand" class="view-content" style="display:none;">
        
        <div class="view-header">
            <h2>LAGERBESTAND</h2>
        </div>

        <div class="view-body">
            <p>Hier kommt dein W3C konformer HTML5 Inhalt rein.</p>
        </div>

    </section>
    ```

3.  **Builden:**
    Führe `make` aus.
    * **Ergebnis:** Ein Button **"LAGER BESTAND"** erscheint automatisch in der Navigation.

---

## 📐 Regeln für W3C Konformität

Da wir ein "Embedded Web System" bauen, werden diese Dateien in den bestehenden `<body>` injiziert.

### 1. KEINE Struktur-Tags verwenden!
Deine Datei darf **NIEMALS** enthalten:
* ❌ `<!DOCTYPE html>`
* ❌ `<html>`, `<head>`, `<body>`
* ❌ `<script>` oder `<style>` (Diese gehören in die Ordner `/src/js` bzw. `/src/css`)

### 2. Semantisches HTML5 nutzen
Nutze Tags, die den Inhalt beschreiben:
* ✅ `<article>` für Textblöcke
* ✅ `<aside>` für Seitenleisten
* ✅ `<table>` mit `<thead>` und `<tbody>` für Daten
* ✅ `<h2>` bis `<h6>` für Überschriften (`<h1>` ist für den App-Titel reserviert)

### 3. IDs und Klassen
* **Root-ID:** Das umschließende `<section>` Tag muss die ID `view_DATEINAME` haben.
* **Klasse:** Es muss die Klasse `view-content` haben (damit das JS Umschalten funktioniert).
* **Style:** `style="display:none;"` verhindert, dass beim Laden kurz alle Seiten gleichzeitig zu sehen sind (FOUC).

---

## 🤖 Wie die Auto-Navigation funktioniert

Der Builder (`prowb.c`) scannt diesen Ordner. Der Dateiname bestimmt, wie der Button in der Navigation aussieht:

| Dateiname | Generierter Button Text | Generierte ID |
| :--- | :--- | :--- |
| `view_home.html` | **HOME** | `home` |
| `view_user_login.html` | **USER LOGIN** | `view_user_login` |
| `view_settings.html` | **SETTINGS** | `view_settings` |

*Tipp: Unterstriche `_` im Dateinamen werden zu Leerzeichen im Button.*

---

## 🎨 Styling

Schreibe kein CSS in diese HTML-Dateien (Inline-Styles vermeiden).
1.  Gebe deinen Elementen Klassen (z.B. `<div class="lager-kachel">`).
2.  Erstelle eine passende CSS-Datei in `src/css/`, z.B. `50_lager.css`.
3.  Der Builder verknüpft alles automatisch.

---

*ProWB Dev Guide*
