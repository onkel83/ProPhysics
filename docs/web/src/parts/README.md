
# HTML Parts // The Skeleton

Dieser Ordner enthält das **statische Gerüst** der Anwendung. Der `prowb` Builder nimmt diese drei Dateien und "klebt" sie mit den dynamischen Inhalten (CSS, JS, Views) zusammen.

Wenn du das Layout, das Logo oder die Meta-Daten ändern willst, bist du hier richtig.

---

## 1. `header.html` (Der Kopf)
Startet das HTML-Dokument. Hier definierst du die technischen Meta-Daten.

* **Zuständig für:**
    * `<!DOCTYPE html>` & `<html>`
    * SEO (`meta description`)
    * Mobile Optimierung (`viewport`)
    * Browser-Farbe (`theme-color`)
    * Titel im Browser-Tab (`<title>`)
* **⚠️ WICHTIG:**
    * Diese Datei darf **kein** `</head>` Tag enthalten!
    * Der Builder injiziert **direkt nach dieser Datei** automatisch alle CSS-Dateien.

---

## 2. `nav.html` (Die Steuerung)
Hier passiert der Übergang vom technischen Kopf zum sichtbaren Körper.

* **Zuständig für:**
    * Schließt den `<head>` und öffnet den `<body>`.
    * Enthält den sichtbaren **Header** (Logo/Titel).
    * Enthält die **Navigation Bar**.
* **Die Magie (Template Tags):**
    Der Builder sucht in dieser Datei nach zwei speziellen Platzhaltern und ersetzt sie automatisch:
    * `{{NAV_VIEWS}}` → Erzeugt Buttons für alle `.html` Dateien aus `/src/views`.
    * `{{NAV_DOCS}}` → Erzeugt Buttons für alle `.md` Dateien aus `/src/docs`.
* **Rebranding:**
    * Ändere `<h1 class="brand-title">...</h1>`, um den Produktnamen anzupassen.

---

## 3. `footer.html` (Der Fuß)
Der Abschluss der Seite.

* **Zuständig für:**
    * Schließt den `<main>` Container.
    * Enthält Copyright und statische Links (z.B. zu Lizenz/Readme).
    * Schließt `<body>` und `<html>`.
* **⚠️ WICHTIG:**
    * Der Builder injiziert **direkt vor dieser Datei** automatisch alle JavaScript-Dateien.
* **Anpassung:**
    * Hier kannst du das Copyright-Jahr und den Firmennamen ändern.
    * Die Buttons nutzen `onclick="app.navigate(...)"`, um ohne Neuladen zu navigieren.

---

## 🏗️ Wie der Builder alles zusammensetzt

Damit du verstehst, warum die Dateien so aussehen, wie sie aussehen – so baut `prowb` die `index.html`:

1.  Inhalt von **`header.html`**
2.  `<style> ... Inhalt von src/css/*.css ... </style>`
3.  Inhalt von **`nav.html`** (Platzhalter werden ersetzt)
4.  Inhalt von **`src/views/*.html`**
5.  Inhalt von **`src/docs/*.md`** (konvertiert zu HTML)
6.  `<script> ... Inhalt von src/js/*.js ... </script>`
7.  Inhalt von **`footer.html`**

---

*ProWB Documentation - Internal Use Only*
