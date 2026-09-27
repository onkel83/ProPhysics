'use strict';

/**
 * ProPhysics Bridge Module
 * Schnittstelle zwischen Web-UI und C/C++/C# Host.
 *
 * Version: 1.0.0 (Etappe 23)
 */
const Bridge = {

    isConnected: false,

    init: function () {
        console.log("[Bridge] Initializing connection...");
        this.isConnected = true;
        this.log("Bridge Online");
    },

    /**
     * Sendet Daten an den Host.
     * @param {string} action  Aktions-Identifier
     * @param {object} payload Nutzdaten
     */
    send: function (action, payload = {}) {
        if (!this.isConnected) {
            console.warn("[Bridge] Offline. Cannot send:", action);
            return;
        }

        const message = {
            action: action,
            data: payload,
            timestamp: Date.now()
        };

        // ECHTE IMPLEMENTIERUNG:
        // window.chrome.webview.postMessage(message); // MAUI / WebView2
        // Module.ccall(...)                          // WASM

        console.log(`[Bridge] >> OUT: ${action}`, payload);
    },

    /**
     * Empfängt Daten vom Host (wird vom Host aufgerufen).
     * @param {string} jsonString
     */
    receive: function (jsonString) {
        try {
            const msg = JSON.parse(jsonString);
            console.log(`[Bridge] << IN:`, msg);
            const event = new CustomEvent('bridge-data', { detail: msg });
            document.dispatchEvent(event);
        } catch (e) {
            console.error("[Bridge] Error parsing incoming message", e);
        }
    },

    log: function (txt) {
        console.log(`[Bridge] ${txt}`);
    }
};

Bridge.init();