# CSS // Styling & Design System

In diesem Ordner liegen alle Stylesheets. Der `prowb` Builder liest diese Dateien in **alphabetischer Reihenfolge** ein und injiziert sie direkt in den `<head>` der `index.html`.

Wir nutzen **keine Preprozessoren** (Sass/Less), sondern modernes, natives CSS mit **CSS Custom Properties (Variables)**.

---

## 📂 Dateistruktur & Ladereihenfolge

Damit das Kaskadieren (das "C" in CSS) korrekt funktioniert, nutzen wir numerische Präfixe:

| Präfix | Zweck | Beispiel |
| :--- | :--- | :--- |
| `00_` | **Core & Variablen:** Hier werden Farben, Fonts und Resets definiert. | `00_vars.css`, `00_reset.css` |
| `10_` | **Layout:** Globales Raster, Header, Footer, Navigation. | `10_layout.css` |
| `50_` | **Komponenten:** Styles für spezifische Elemente (Cards, Tabellen). | `50_components.css` |
| `90_` | **View Specifics:** Styles, die nur für eine bestimmte View nötig sind. | `90_view_home.css` |
| `99_` | **Overrides:** "Notfall"-Styles, die alles andere überschreiben. | `99_hacks.css` |

---

## 🎨 Rebranding Guide (Design anpassen)

Das gesamte Farbschema wird zentral gesteuert. Du musst nicht hunderte Zeilen Code suchen.

Öffne **`00_vars.css`** und ändere die Werte im `:root` Block:

```css
:root {
    /* 1. Hauptfarben (Industrial Look) */
    --color-header-bg: #2d3436;   /* Dunkler Hintergrund (Header/Footer) */
    --color-bg:        #f0f2f5;   /* Heller Hintergrund (Body) */

    /* 2. Akzentfarbe (Das "Neon Grün") */
    --color-accent:     #00ffcc;  /* Leuchtend (Buttons, Status ON) */
    --color-accent-dim: #00b894;  /* Gedimmt (Progress Bars) */

    /* 3. Effekte */
    --shadow-metallic: 2px 2px 0px #0984e3; /* Der blaue Schlagschatten */
}

```

Änderungen hier wirken sich sofort auf die **gesamte App** aus.

---

## 📐 Layout-Prinzipien

Wir bauen **Responsive** und **Mobile First**.

1. **Grid & Flexbox:**
Nutze CSS Grid für Seitenlayouts und Flexbox für Komponenten-Ausrichtung.
* *Beispiel:* `src/css/10_layout.css` definiert das Grundgerüst.


2. **Embedded Fonts:**
Da das System offline/embedded laufen muss:
* ❌ Keine Links zu Google Fonts (`<link href="...">`).
* ✅ Nutze System-Fonts (`Segoe UI`, `Roboto`, `Helvetica`) oder binde Fonts als Base64 direkt im CSS ein (wenn zwingend nötig).


3. **Markdown Styling:**
Dokumentations-Styles (für Tabellen, Listen aus `.md` Dateien) werden meist über die Klasse `.markdown-body` gesteuert.

---

## ⚠️ Regeln für Entwickler

1. **Keine Inline-Styles:** Schreibe CSS in diesen Ordner, nicht in die HTML-Views.
2. **Namenskonvention:** Wenn du Styles für `view_lager.html` schreibst, nenne die CSS-Datei `50_lager.css`.
3. **Sauberkeit:** Entferne nicht genutzte CSS-Dateien, um die `index.html` klein zu halten.

---

*ProWB Design Team*
