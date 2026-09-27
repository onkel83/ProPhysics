'use strict';

/**
 * BrainAI Documentation Portal Controller
 * Datei: 10_app.js
 * Version: 1.0.0 (Static UI Edition)
 * Minimaler Footprint. Keine aktive Krypto-Logik im Client.
 */

const app = {
    // --- APPLICATION STATE ---
    config: {
        startView: 'view_home'
    },
    state: {
        currentView: null,
        currentTheme: 'default'
    },

    // --- INITIALISIERUNG ---
    init: function () {
        console.log("[BrainAI] Documentation Portal Booting...");

        this.loadTheme();
        
        // System-Prüfung: Versteckt den Doku-Block, wenn keine .md Dateien geladen wurden
        const docsFlex = document.getElementById('nav-docs-flex');
        const docsGroup = document.getElementById('nav-docs-group');
        if (docsFlex && docsGroup && docsFlex.children.length === 0) {
            docsGroup.style.display = 'none';
        }
        // Navigation & History Events einrichten
        const hash = window.location.hash.replace('#', '');
        this.navigate(hash || this.config.startView);

        window.addEventListener('popstate', () => {
            const view = window.location.hash.replace('#', '') || 'view_home';
            this.navigate(view, false);
        });

        console.log("[BrainAI] System Ready. Architecture: STATIC.");
    },

    // --- NAVIGATION LOGIK (Router) ---
    navigate: function (viewId, updateHistory = true) {
        const targetEl = document.getElementById(viewId);
        
        
        if (!targetEl) {
            console.warn(`[Router] View '${viewId}' nicht gefunden. Fallback zu Home.`);
            if (viewId !== 'view_home') this.navigate('view_home');
            return;
        }

        
        const allViews = document.querySelectorAll('.view-content');
        if (allViews) {
            allViews.forEach(el => el.style.display = 'none');
        }
        
        targetEl.style.display = 'block';
        window.scrollTo(0, 0);

        this.state.currentView = viewId;
        this._updateNavHighlight(viewId);

        
        if (updateHistory) history.pushState(null, null, '#' + viewId);
    },

    setTheme: function (themeName) {
        const targetTheme = (themeName === 'default') ? 'proedc' : themeName;
        if (document.body) { 
            document.body.setAttribute('data-theme', targetTheme);
        }
        localStorage.setItem('prosuite_theme', themeName);
        this.state.currentTheme = themeName;
    },

    loadTheme: function () {
        const saved = localStorage.getItem('prosuite_theme');
        this.setTheme(saved ? saved : 'default');
    },
    
    _updateNavHighlight: function (activeId) {
        const btns = document.querySelectorAll('.nav-btn');
        if (btns) {
            btns.forEach(btn => {
                btn.classList.remove('active');
                const onClick = btn.getAttribute('onclick');
                if (onClick && onClick.includes(`'${activeId}'`)) {
                    btn.classList.add('active');
                }
            });
        }
    }
};

document.addEventListener('DOMContentLoaded', () => app.init());
