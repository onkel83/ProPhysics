'use strict';

/**
 * BRIDGE MODULE
 * Schnittstelle zwischen Web-UI und C/C++/C# Host.
 */
const Bridge = {
    
    // Simuliert den Status der Verbindung
    isConnected: false,

    init: function() {
        console.log("[Bridge] Initializing connection...");
        // Hier würde der Handshake mit WASM/MAUI stattfinden
        this.isConnected = true;
        this.log("Bridge Online");
    },

    // Sendet Daten an den Host (Generic)
    send: function(action, payload = {}) {
        if (!this.isConnected) {
            console.warn("[Bridge] Offline. Cannot send:", action);
            return;
        }

        const message = { action: action, data: payload, timestamp: Date.now() };
        
        // ECHTE IMPLEMENTIERUNG WÄRE HIER:
        // window.chrome.webview.postMessage(message); // für MAUI/WebView2
        // Module.ccall(...) // für WASM
        
        console.log(`[Bridge] >> OUT: ${action}`, payload);
    },

    // Empfängt Daten vom Host (wird vom Host aufgerufen)
    receive: function(jsonString) {
        try {
            const msg = JSON.parse(jsonString);
            console.log(`[Bridge] << IN:`, msg);
            // Event feuern, damit app.js darauf reagieren kann
            const event = new CustomEvent('bridge-data', { detail: msg });
            document.dispatchEvent(event);
        } catch (e) {
            console.error("[Bridge] Error parsing incoming message", e);
        }
    },

    // Hilfsfunktion für Logs im Dashboard
    log: function(txt) {
        console.log(`[Bridge] ${txt}`);
        // Optional: Log auch im HTML-Terminal anzeigen, falls vorhanden
    }
};

// Starten
Bridge.init();
