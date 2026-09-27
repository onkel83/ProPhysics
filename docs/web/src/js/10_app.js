'use strict';

/**
 * ProPhysics Documentation Portal Controller
 * Datei: 10_app.js
 * Version: 1.1.0 (Etappe 23 / ProWB)
 * Minimaler Footprint. Keine aktive Krypto-Logik im Client.
 */

const app = {

    // --- STATE ---
    config: {
        startView: 'view_home',
        defaultTheme: 'prophysics'
    },

    state: {
        currentView: null,
        currentTheme: 'prophysics'
    },

    // --- INITIALISIERUNG ---
    init: function () {
        console.log("[ProPhysics] Documentation Portal Booting...");

        this.loadTheme();

        // System-Prüfung: Verstecke Doku-Gruppe, wenn Manifest keine Sektionen hat
        const docsFlex = document.getElementById('nav-docs-flex');
        const docsGroup = document.getElementById('nav-docs-group');
        if (docsFlex && docsGroup && docsFlex.children.length === 0) {
            docsGroup.style.display = 'none';
        }

        // Initiale View aus Hash oder Home
        const hash = window.location.hash.replace('#', '');
        this.navigate(hash || this.config.startView);

        // Browser-History (Vor / Zurück)
        window.addEventListener('popstate', () => {
            const view = window.location.hash.replace('#', '') || this.config.startView;
            this.navigate(view, false);
        });

        // Hash-Änderung abfangen (für Anker-Klicks in Doku)
        window.addEventListener('hashchange', () => {
            const hash = window.location.hash;
            if (!hash) return;
            const targetId = hash.substring(1);
            const el = document.getElementById(targetId);
            if (el) {
                // Wenn es eine View ist, router nutzen; sonst Browser scrollen lassen
                if (el.classList.contains('view-content')) {
                    this.navigate(targetId, false);
                }
            }
        });

        console.log("[ProPhysics] System Ready.");
    },

    // --- ROUTER ---
    navigate: function (viewId, updateHistory = true) {
        if (!viewId) {
            viewId = this.config.startView;
        }

        // Anker in doc_...? → wir haben evtl. <section id="doc_Modules_SU2">,
        // aber der Anker-Link kann auf "doc_Modules_SU2-someheading" zeigen.
        let baseViewId = viewId;
        let anchorFragment = null;
        const el = document.getElementById(viewId);

        if (!el && viewId.startsWith('doc_')) {
            // Suche Präfix-View: doc_<Section>_<DocID> bis zum letzten Bindestrich
            const dashIdx = viewId.lastIndexOf('-');
            if (dashIdx > 0) {
                baseViewId = viewId.substring(0, dashIdx);
                anchorFragment = viewId;
            }
        }

        const targetEl = el || document.getElementById(baseViewId);

        if (!targetEl) {
            console.warn(`[Router] View '${viewId}' nicht gefunden. Fallback zu Home.`);
            if (viewId !== 'view_home') this.navigate('view_home');
            return;
        }

        // Alle Views verstecken
        document.querySelectorAll('.view-content').forEach(v => {
            v.style.display = 'none';
        });

        // Ziel-View zeigen
        targetEl.style.display = 'block';
        this.state.currentView = baseViewId;

        // Nav-Highlight aktualisieren
        this._updateNavHighlight(baseViewId);

        // Scrollen: entweder zum Anker oder nach oben
        if (anchorFragment) {
            // Verzögert, damit Layout fertig ist
            setTimeout(() => {
                const anchor = document.getElementById(anchorFragment);
                if (anchor) {
                    anchor.scrollIntoView({ behavior: 'smooth', block: 'start' });
                } else {
                    window.scrollTo(0, 0);
                }
            }, 30);
        } else {
            window.scrollTo(0, 0);
        }

        // History
        if (updateHistory) {
            const newHash = '#' + viewId;
            if (window.location.hash !== newHash) {
                history.pushState(null, null, newHash);
            }
        }
    },

    // --- THEME ---
    setTheme: function (themeName) {
        const valid = ['prophysics', 'light', 'matrix'];
        const target = valid.includes(themeName) ? themeName : this.config.defaultTheme;

        if (document.body) {
            document.body.setAttribute('data-theme', target);
        }
        try {
            localStorage.setItem('prophysics_theme', target);
        } catch (e) {
            console.warn("[Theme] localStorage nicht verfügbar:", e);
        }
        this.state.currentTheme = target;
    },

    loadTheme: function () {
        let saved = null;
        try {
            saved = localStorage.getItem('prophysics_theme');
        } catch (e) { /* ignore */ }
        this.setTheme(saved || this.config.defaultTheme);
    },

    toggleTheme: function () {
        // Reihenfolge: prophysics → light → matrix → prophysics
        const order = ['prophysics', 'light', 'matrix'];
        const cur = order.indexOf(this.state.currentTheme);
        const next = order[(cur + 1) % order.length];
        this.setTheme(next);
    },

    // --- NAV HIGHLIGHT ---
    _updateNavHighlight: function (activeId) {
        const btns = document.querySelectorAll('.nav-btn');
        if (!btns) return;

        // Wenn doc_xxx_yyy aktiv ist, auch die Sektions-View highlighten
        let effectiveId = activeId;
        if (activeId && activeId.startsWith('doc_')) {
            const parts = activeId.split('_');
            if (parts.length >= 3) {
                effectiveId = 'view_' + parts[1].toLowerCase();
            }
        }

        btns.forEach(btn => {
            btn.classList.remove('active');
            const onClick = btn.getAttribute('onclick');
            if (!onClick) return;

            if (onClick.includes(`'${activeId}'`) || onClick.includes(`'${effectiveId}'`)) {
                btn.classList.add('active');
            }
        });
    }
};

document.addEventListener('DOMContentLoaded', () => app.init());