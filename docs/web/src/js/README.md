# JavaScript // Application Logic

In diesem Ordner liegt das "Gehirn" der Anwendung. Der `prowb` Builder liest alle `.js` Dateien in **alphabetischer Reihenfolge** ein und injiziert sie am Ende des `<body>` (direkt vor dem Footer).

Wir nutzen **Vanilla JS (ES6)** im `strict mode`. Keine Frameworks, keine externen Abhängigkeiten, volle W3C-Konformität.

---

## 📂 Ladereihenfolge & Architektur

Die Nummerierung der Dateien ist entscheidend, um Abhängigkeiten aufzulösen (z.B. muss die Bridge existieren, bevor die App sie nutzen kann).

| Datei | Zweck | Globales Objekt |
| :--- | :--- | :--- |
| **`00_bridge.js`** | **Der Kernel:** Schnittstelle zum Host (C#, WASM, WebView). Muss zwingend als erstes geladen werden. | `Bridge` |
| **`10_app.js`** | **Der Controller:** Steuert Navigation, UI-State und Initialisierung. | `app` |
| **`50_libs.js`** | **Helfer:** Eigene Bibliotheken (z.B. Chart-Rendering, Mathe-Funktionen). | - |
| **`90_view_*.js`** | **View-Logik:** Spezifischer Code für einzelne Seiten (z.B. `90_view_lager.js` für Lager-Events). | - |

---

## 🛠️ API Referenz

### 1. Navigation (SPA Routing)
Um zwischen Views zu wechseln, nutze niemals direkte Links (`<a href>`), sondern den Router.

```javascript
// Wechselt zur Settings-Ansicht und aktualisiert die URL/History
app.navigate('view_settings');

// Optionale Parameter: navigate(viewId, updateHistoryBoolean)
app.navigate('home', false); 

```

*Im HTML:* `<button onclick="app.navigate('view_settings')">Einstellungen</button>`

### 2. Daten an den Host senden (Bridge)

Um mit dem C++ Backend oder dem C# Wrapper zu kommunizieren:

```javascript
// Bridge.send(ActionString, PayloadObject)
Bridge.send('SAVE_PROFILE', { username: 'Admin', theme: 'dark' });

```

### 3. Daten vom Host empfangen

Das System feuert ein Custom Event, wenn Daten reinkommen.

```javascript
document.addEventListener('bridge-data', (e) => {
    const msg = e.detail;
    if (msg.type === 'ALARM') {
        alert("ALARM: " + msg.payload);
    }
});

```

---

## ⚠️ Coding Standards (W3C Strict)

Damit das System stabil und wartbar bleibt, gelten folgende Regeln:

1. **Use Strict:** Jede Datei muss mit `'use strict';` beginnen.
2. **Kein Global Pollution:** Kapsle deine Logik in Objekte oder IIFEs (Immediately Invoked Function Expressions), um den globalen Namespace sauber zu halten.
```javascript
// ✅ Gut:
const LagerLogik = {
    init: function() { ... }
};

// ❌ Schlecht:
function lagerInit() { ... }
var lagerCount = 0;

```


3. **Defensive DOM-Access:** Da Views ausgeblendet sein können, existieren Elemente oft nicht im sichtbaren Bereich. Prüfe immer auf `null`.
```javascript
const el = document.getElementById('output');
if (el) {
    el.innerText = 'Hallo';
}

```


4. **Performance:** Vermeide `setInterval` wenn möglich. Nutze Events. Wenn du Timer brauchst, bereinige sie, wenn die View gewechselt wird.

---

*ProWB JavaScript Core*
