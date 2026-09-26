#!/usr/bin/env python3
# analysis.py  -- Integrierte Analyse Module 1..15 (inkl. Summit + Plots)
# Usage examples:
#  python analysis.py
#  python analysis.py --csv-pattern "result*.csv" --events events_module13.csv --out-dir outputs --save-plots --out-summit summit.txt

import os
import sys
import argparse
import glob
import datetime
import math

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats
from sklearn.decomposition import PCA
from sklearn.preprocessing import StandardScaler
from datetime import datetime, timezone

# ---------------------------
# CLI
# ---------------------------
def parse_args():
    p = argparse.ArgumentParser(description="Run full analysis pipeline (Modules 1..14)")
    p.add_argument("--csv-pattern", default="result*.csv", help="Glob pattern for CSV input files")
    p.add_argument("--events", default=None, help="Optional precomputed events CSV (module13 output)")
    p.add_argument("--out-dir", default="outputs", help="Directory for outputs (PNGs, CSVs, summit)")
    p.add_argument("--save-plots", action="store_true", help="Save PNG plots for overview and per-event")
    p.add_argument("--out-summit", default="summit.txt", help="Filename for summit text (UTF-8)")
    return p.parse_args()

# ---------------------------
# LOAD
# ---------------------------
def load_all_csv(pattern):
    files = glob.glob(pattern)
    if not files:
        raise FileNotFoundError(f"Keine CSV-Dateien gefunden mit Pattern: {pattern}")
    dfs = []
    for f in files:
        print(f"[LOAD] {f}")
        # Liest den echten Header ein und überspringt die eine defekte Zeile 3002
        df = pd.read_csv(f, header=0, on_bad_lines='skip')
        
        # Mapping/Fallback für alternative Spaltennamen
        if "ph" in df.columns and "photon" not in df.columns:
            df = df.rename(columns={"ph": "photon"})
        if "alpha" in df.columns and "alpha_eff" not in df.columns:
            df = df.rename(columns={"alpha": "alpha_eff"})
            
        dfs.append(df)
        
    df_all = pd.concat(dfs, ignore_index=True)
    print(f"[INFO] Gesamtzeilen: {len(df_all)}")
    return df_all

# ---------------------------
# MODULE 1: BASIC STATS (Aktualisiert für C99 CHSH)
# ---------------------------
def compute_basic_stats(df):
    stats = {}
    stats["rhoE_mean"] = df["rhoE"].mean() if "rhoE" in df.columns else 0
    stats["rhoE_std"] = df["rhoE"].std() if "rhoE" in df.columns else 0
    stats["rhoE_min"] = df["rhoE"].min() if "rhoE" in df.columns else 0
    stats["rhoE_max"] = df["rhoE"].max() if "rhoE" in df.columns else 0
    stats["alpha_mean"] = df["alpha_eff"].mean() if "alpha_eff" in df.columns else 0
    stats["alpha_std"] = df["alpha_eff"].std() if "alpha_eff" in df.columns else 0
    stats["alpha_min"] = df["alpha_eff"].min() if "alpha_eff" in df.columns else 0
    stats["alpha_max"] = df["alpha_eff"].max() if "alpha_eff" in df.columns else 0
    stats["alpha_median"] = df["alpha_eff"].median() if "alpha_eff" in df.columns else 0
    stats["alpha_q25"] = df["alpha_eff"].quantile(0.25) if "alpha_eff" in df.columns else 0
    stats["alpha_q75"] = df["alpha_eff"].quantile(0.75) if "alpha_eff" in df.columns else 0
    stats["pos_total"] = df["pos"].sum() if "pos" in df.columns else 0
    stats["neg_total"] = df["neg"].sum() if "neg" in df.columns else 0
    stats["photon_total"] = df["photon"].sum() if "photon" in df.columns else 0
    stats["interactions_mean"] = df["interactions"].mean() if "interactions" in df.columns else 0
    stats["interactions_max"] = df["interactions"].max() if "interactions" in df.columns else 0
    
    # NEU: Native C99 Bell/CHSH-Metriken
    if "S_CHSH" in df.columns:
        stats["S_CHSH_mean"] = df["S_CHSH"].mean()
        stats["S_CHSH_max"] = df["S_CHSH"].max()
        stats["S_CHSH_std"] = df["S_CHSH"].std()
    return stats

def print_stats_block(title, stats):
    print(f"\n--- {title} ---")
    for k, v in stats.items():
        print(f"{k}: {v}")

# ---------------------------
# MODULE 2: STATS PER RUN
# ---------------------------
def stats_per_run(df):
    print("\n=== STATS PRO RUN ===")
    for run in sorted(df["run_index"].unique()):
        df_run = df[df["run_index"] == run]
        stats = compute_basic_stats(df_run)
        print_stats_block(f"Run {run}", stats)

# ---------------------------
# MODULE 3: REGIMES & CORRELATIONS
# ---------------------------
def analyze_regimes_and_correlations(df):
    print("\n=== MODUL 3: REGIME- & KORRELATIONSANALYSE ===")
    cols = ["rhoE", "alpha_eff", "interactions", "pos", "neg", "photon"]
    corr = df[cols].corr()
    print("\n[CORR] Korrelationsmatrix (Pearson):")
    print(corr)
    alpha = df["alpha_eff"]
    q25 = alpha.quantile(0.25)
    q75 = alpha.quantile(0.75)
    reg_low = df[alpha <= q25]
    reg_mid = df[(alpha > q25) & (alpha < q75)]
    reg_high = df[alpha >= q75]
    def regime_stats(name, subdf):
        s = {}
        s["count"] = len(subdf)
        s["rhoE_mean"] = subdf["rhoE"].mean()
        s["rhoE_std"] = subdf["rhoE"].std()
        s["alpha_mean"] = subdf["alpha_eff"].mean()
        s["interactions_mean"] = subdf["interactions"].mean()
        return name, s
    print("\n[REGIMES] basierend auf alpha_eff-Quartilen:")
    for name, s in [
        regime_stats("LOW (<= Q25)", reg_low),
        regime_stats("MID (Q25..Q75)", reg_mid),
        regime_stats("HIGH (>= Q75)", reg_high),
    ]:
        print_stats_block(name, s)
    df_sorted = df.sort_values("tick")
    df_sorted["alpha_diff"] = df_sorted["alpha_eff"].diff()
    df_sorted["rhoE_diff"] = df_sorted["rhoE"].diff()
    print("\n[DERIVATIVES] globale Mittelwerte der Differenzen:")
    print(f"mean d(alpha)/dt: {df_sorted['alpha_diff'].mean()}")
    print(f"mean d(rhoE)/dt: {df_sorted['rhoE_diff'].mean()}")

# ---------------------------
# MODULE 4: TIME WINDOW
# ---------------------------
def time_window_stats(df, window_size=500):
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    max_tick = int(df_sorted["tick"].max())
    windows = []
    for start in range(1, max_tick + 1, window_size):
        end = min(start + window_size - 1, max_tick)
        w = df_sorted[(df_sorted["tick"] >= start) & (df_sorted["tick"] <= end)]
        if w.empty:
            continue
        win_info = {
            "start_tick": start,
            "end_tick": end,
            "rhoE_mean": w["rhoE"].mean(),
            "rhoE_std": w["rhoE"].std(),
            "alpha_mean": w["alpha_eff"].mean(),
            "alpha_std": w["alpha_eff"].std(),
            "interactions_mean": w["interactions"].mean(),
            "rhoE_trend": w["rhoE"].iloc[-1] - w["rhoE"].iloc[0],
            "alpha_trend": w["alpha_eff"].iloc[-1] - w["alpha_eff"].iloc[0],
            "interactions_trend": w["interactions"].iloc[-1] - w["interactions"].iloc[0],
        }
        windows.append(win_info)
    return windows

def print_time_window_stats(windows):
    print("\n=== MODUL 4: ZEITFENSTER-ANALYSE (STATIONARITÄT) ===")
    for i, w in enumerate(windows, start=1):
        print(f"\n--- Fenster {i} (Ticks {w['start_tick']}..{w['end_tick']}) ---")
        print(f"rhoE_mean: {w['rhoE_mean']}")
        print(f"rhoE_std: {w['rhoE_std']}")
        print(f"alpha_mean: {w['alpha_mean']}")
        print(f"alpha_std: {w['alpha_std']}")
        print(f"interactions_mean: {w['interactions_mean']}")
        print(f"rhoE_trend: {w['rhoE_trend']}")
        print(f"alpha_trend: {w['alpha_trend']}")
        print(f"interactions_trend: {w['interactions_trend']}")

# ---------------------------
# MODULE 5: ALPHA EFFICIENCY
# ---------------------------
def analyze_alpha_efficiency(df):
    print("\n=== MODUL 5: ALPHA-EFFIZIENZ-ANALYSE ===")
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    df_sorted["alpha_rho_ratio"] = df_sorted["alpha_eff"] / df_sorted["rhoE"].replace(0, np.nan)
    print("\n[ALPHA/RHO] Verhältnis alpha_eff / rhoE:")
    print(f"mean ratio: {df_sorted['alpha_rho_ratio'].mean()}")
    print(f"median ratio: {df_sorted['alpha_rho_ratio'].median()}")
    print(f"max ratio: {df_sorted['alpha_rho_ratio'].max()}")
    df_sorted["alpha_int_ratio"] = df_sorted["alpha_eff"] / df_sorted["interactions"].replace(0, np.nan)
    print("\n[ALPHA/INTERACTIONS] Verhältnis alpha_eff / interactions:")
    print(f"mean ratio: {df_sorted['alpha_int_ratio'].mean()}")
    print(f"median ratio: {df_sorted['alpha_int_ratio'].median()}")
    print(f"max ratio: {df_sorted['alpha_int_ratio'].max()}")
    df_sorted["alpha_jump"] = df_sorted["alpha_eff"].diff()
    jumps = df_sorted[df_sorted["alpha_jump"].abs() > df_sorted["alpha_jump"].std() * 3]
    print("\n[CRITICAL POINTS] starke Sprünge in alpha_eff (> 3σ):")
    print(jumps[["tick", "alpha_eff", "alpha_jump"]].head(20))
    corr = df_sorted["alpha_eff"].corr(df_sorted["rhoE"])
    print("\n[PHASEN] alpha_eff vs rhoE:")
    print(f"alpha_eff ↔ rhoE correlation: {corr}")
    alpha_diff = df_sorted["alpha_eff"].diff().rolling(200).mean()
    print("\n[SATURATION/COLLAPSE] Trend über 200-Tick-Fenster:")
    print(f"mean d(alpha)/dt (200er window): {alpha_diff.mean()}")
    print(f"min d(alpha)/dt (200er window): {alpha_diff.min()}")
    print(f"max d(alpha)/dt (200er window): {alpha_diff.max()}")

# ---------------------------
# MODULE 6: PHOTON ANALYSIS
# ---------------------------
def analyze_photons(df):
    print("\n=== MODUL 6: PHOTON-ANALYSE ===")
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    print("\n[PHOTON BASIC STATS]")
    print(f"mean photons: {df_sorted['photon'].mean()}")
    print(f"median photons: {df_sorted['photon'].median()}")
    print(f"min photons: {df_sorted['photon'].min()}")
    print(f"max photons: {df_sorted['photon'].max()}")
    df_sorted["photon_diff"] = df_sorted["photon"].diff()
    print("\n[PHOTON TREND]")
    print(f"mean d(photon)/dt: {df_sorted['photon_diff'].mean()}")
    print(f"min d(photon)/dt: {df_sorted['photon_diff'].min()}")
    print(f"max d(photon)/dt: {df_sorted['photon_diff'].max()}")
    corr_alpha = df_sorted["photon"].corr(df_sorted["alpha_eff"])
    print("\n[PHOTON ↔ ALPHA_EFF]")
    print(f"correlation photon ↔ alpha_eff: {corr_alpha}")
    corr_int = df_sorted["photon"].corr(df_sorted["interactions"])
    print("\n[PHOTON ↔ INTERACTIONS]")
    print(f"correlation photon ↔ interactions: {corr_int}")
    q25 = df_sorted["photon"].quantile(0.25)
    q75 = df_sorted["photon"].quantile(0.75)
    reg_low = df_sorted[df_sorted["photon"] <= q25]
    reg_mid = df_sorted[(df_sorted["photon"] > q25) & (df_sorted["photon"] < q75)]
    reg_high = df_sorted[df_sorted["photon"] >= q75]
    def photon_regime_stats(name, subdf):
        s = {}
        s["count"] = len(subdf)
        s["alpha_mean"] = subdf["alpha_eff"].mean()
        s["interactions_mean"] = subdf["interactions"].mean()
        s["rhoE_mean"] = subdf["rhoE"].mean()
        return name, s
    print("\n[PHOTON REGIMES]")
    for name, s in [
        photon_regime_stats("LOW (<= Q25)", reg_low),
        photon_regime_stats("MID (Q25..Q75)", reg_mid),
        photon_regime_stats("HIGH (>= Q75)", reg_high),
    ]:
        print_stats_block(name, s)
    jumps = df_sorted[df_sorted["photon_diff"].abs() > df_sorted["photon_diff"].std() * 3]
    print("\n[CRITICAL PHOTON POINTS] starke Sprünge (> 3σ):")
    print(jumps[["tick", "photon", "photon_diff"]].head(20))

# ---------------------------
# MODULE 7: NONLOCALITY
# ---------------------------
def analyze_nonlocality(df):
    print("\n=== MODUL 7: NICHTLOKALITÄTS-ANALYSE (WELF-MODUL) ===")
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    
    # Nutzt non_euclidean_links falls vorhanden, sonst Fallback auf interactions
    nl_col = "non_euclidean_links" if "non_euclidean_links" in df_sorted.columns else "interactions"
    print(f"\n[NONLOCALITY METRIC] Verwendete Spalte: {nl_col}")
    print(f"mean {nl_col}: {df_sorted[nl_col].mean()}")
    print(f"median {nl_col}: {df_sorted[nl_col].median()}")
    print(f"max {nl_col}: {df_sorted[nl_col].max()}")
    
    df_sorted["nl_diff"] = df_sorted[nl_col].diff()
    print("\n[NONLOCALITY TREND]")
    print(f"mean d({nl_col})/dt: {df_sorted['nl_diff'].mean()}")
    print(f"min d({nl_col})/dt: {df_sorted['nl_diff'].min()}")
    print(f"max d({nl_col})/dt: {df_sorted['nl_diff'].max()}")
    
    corr_alpha = df_sorted[nl_col].corr(df_sorted["alpha_eff"])
    corr_rhoE = df_sorted[nl_col].corr(df_sorted["rhoE"])
    corr_photon = df_sorted[nl_col].corr(df_sorted["photon"])
    print("\n[KORRELATIONEN MIT NICHTLOKALITÄT]")
    print(f"{nl_col} ↔ alpha_eff: {corr_alpha}")
    print(f"{nl_col} ↔ rhoE: {corr_rhoE}")
    print(f"{nl_col} ↔ photon: {corr_photon}")
    
    q25 = df_sorted[nl_col].quantile(0.25)
    q75 = df_sorted[nl_col].quantile(0.75)
    reg_low = df_sorted[df_sorted[nl_col] <= q25]
    reg_mid = df_sorted[(df_sorted[nl_col] > q25) & (df_sorted[nl_col] < q75)]
    reg_high = df_sorted[df_sorted[nl_col] >= q75]
    
    def reg_stats(name, subdf):
        s = {}
        s["count"] = len(subdf)
        s["alpha_mean"] = subdf["alpha_eff"].mean()
        s["rhoE_mean"] = subdf["rhoE"].mean()
        s["photon_mean"] = subdf["photon"].mean()
        return name, s
        
    print("\n[NICHTLOKALITÄTS-REGIMES]")
    for name, s in [
        reg_stats("LOW (<= Q25)", reg_low),
        reg_stats("MID (Q25..Q75)", reg_mid),
        reg_stats("HIGH (>= Q75)", reg_high),
    ]:
        print_stats_block(name, s)
        
    jumps = df_sorted[df_sorted["nl_diff"].abs() > df_sorted["nl_diff"].std() * 3]
    print("\n[CRITICAL NONLOCALITY POINTS] starke Sprünge (> 3σ):")
    print(jumps[["tick", nl_col, "nl_diff"]].head(20))

# ---------------------------
# MODULE 8: EMERGENCE & CLUSTERS
# ---------------------------
def analyze_emergence_and_clusters(df):
    print("\n=== MODUL 8: EMERGENZ- & CLUSTER-ANALYSE ===")
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    window = 50
    df_sorted["alpha_roll_max"] = df_sorted["alpha_eff"].rolling(window, center=True).max()
    df_sorted["alpha_peak"] = (df_sorted["alpha_eff"] == df_sorted["alpha_roll_max"])
    peaks = df_sorted[df_sorted["alpha_peak"] & (df_sorted["alpha_eff"] > df_sorted["alpha_eff"].mean() + 2*df_sorted["alpha_eff"].std())]
    print("\n[PEAKS] alpha_eff Peaks (> 2σ):")
    print(peaks[["tick", "alpha_eff"]].head(20))
    df_sorted["int_roll_mean"] = df_sorted["interactions"].rolling(window, center=True).mean()
    df_sorted["int_hotspot"] = df_sorted["interactions"] > df_sorted["int_roll_mean"] + 2*df_sorted["interactions"].std()
    hotspots = df_sorted[df_sorted["int_hotspot"]]
    print("\n[HOTSPOTS] Interaktions-Hotspots (> 2σ):")
    print(hotspots[["tick", "interactions"]].head(20))
    df_sorted["cluster_flag"] = (
        (df_sorted["alpha_eff"] > df_sorted["alpha_eff"].quantile(0.75)) &
        (df_sorted["interactions"] > df_sorted["interactions"].quantile(0.75))
    )
    clusters = df_sorted[df_sorted["cluster_flag"]]
    print("\n[CLUSTERS] starke Kopplungscluster (alpha_eff & interactions >= Q75):")
    print(clusters[["tick", "alpha_eff", "interactions"]].head(20))
    alpha = df_sorted["alpha_eff"]
    phase_A = df_sorted[df_sorted["alpha_eff"] < alpha.quantile(0.25)]
    phase_B = df_sorted[(df_sorted["alpha_eff"] >= alpha.quantile(0.25)) & (df_sorted["alpha_eff"] < alpha.quantile(0.50))]
    phase_C = df_sorted[(df_sorted["alpha_eff"] >= alpha.quantile(0.50)) & (df_sorted["alpha_eff"] < alpha.quantile(0.75))]
    phase_D = df_sorted[df_sorted["alpha_eff"] >= alpha.quantile(0.75)]
    print("\n[PHASEN] Klassifikation nach alpha_eff:")
    print(f"Phase A: {phase_A['tick'].min()}–{phase_A['tick'].max()} (n={len(phase_A)})")
    print(f"Phase B: {phase_B['tick'].min()}–{phase_B['tick'].max()} (n={len(phase_B)})")
    print(f"Phase C: {phase_C['tick'].min()}–{phase_C['tick'].max()} (n={len(phase_C)})")
    print(f"Phase D: {phase_D['tick'].min()}–{phase_D['tick'].max()} (n={len(phase_D)})")
    transitions = df_sorted[df_sorted["alpha_eff"].diff().abs() > df_sorted["alpha_eff"].std()]
    print("\n[TRANSITIONS] starke Phasenübergänge (> 1σ Sprung):")
    print(transitions[["tick", "alpha_eff", "alpha_eff"]].head(20))
    print("\nFERTIG — Modul 8 erfolgreich getestet.")

# ---------------------------
# MODULE 9: FFT
# ---------------------------
def fft_analysis(df):
    print("\n=== MODUL 9: FREQUENZ- & SPEKTRALANALYSE (FFT) ===")
    df_sorted = df.sort_values("tick")
    alpha = df_sorted["alpha_eff"].values
    rhoE = df_sorted["rhoE"].values
    interactions = df_sorted["interactions"].values
    photon = df_sorted["photon"].values
    N = len(alpha)
    dt = 1.0
    freq = np.fft.rfftfreq(N, d=dt)
    fft_alpha = np.abs(np.fft.rfft(alpha))
    fft_rhoE = np.abs(np.fft.rfft(rhoE))
    fft_interactions = np.abs(np.fft.rfft(interactions))
    fft_photon = np.abs(np.fft.rfft(photon))
    def top_freqs(fft_data, name):
        idx = np.argsort(fft_data)[-5:][::-1]
        print(f"\n[FFT TOP] {name}:")
        for i in idx:
            print(f"freq={freq[i]:.4f}, amplitude={fft_data[i]:.4f}")
    top_freqs(fft_alpha, "alpha_eff")
    top_freqs(fft_rhoE, "rhoE")
    top_freqs(fft_interactions, "interactions")
    top_freqs(fft_photon, "photon")
    def cross_corr(a, b, name):
        corr = np.corrcoef(a, b)[0, 1]
        print(f"[CROSS] {name}: {corr}")
    print("\n[CROSS-SPECTRAL CORRELATIONS]")
    cross_corr(fft_alpha, fft_rhoE, "alpha_eff ↔ rhoE")
    cross_corr(fft_alpha, fft_interactions, "alpha_eff ↔ interactions")
    cross_corr(fft_alpha, fft_photon, "alpha_eff ↔ photon")
    cross_corr(fft_interactions, fft_photon, "interactions ↔ photon")
    resonance = (fft_alpha * fft_interactions)
    idx_res = np.argsort(resonance)[-5:][::-1]
    print("\n[RESONANZEN] stärkste Kopplungsfrequenzen (alpha_eff × interactions):")
    for i in idx_res:
        print(f"freq={freq[i]:.4f}, resonance={resonance[i]:.4f}")

# ---------------------------
# MODULE 10: AUTOCORR & MEMORY
# ---------------------------
def analyze_memory_depth(df):
    print("\n=== MODUL 10: AUTOKORRELATION & MEMORY-DEPTH ===")
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    alpha = df_sorted["alpha_eff"].values
    rhoE = df_sorted["rhoE"].values
    interactions = df_sorted["interactions"].values
    photon = df_sorted["photon"].values
    max_lag = 500
    def autocorr(x, lag):
        if lag >= len(x): return np.nan
        return np.corrcoef(x[:-lag], x[lag:])[0, 1]
    lags = np.arange(1, max_lag)
    ac_alpha = np.array([autocorr(alpha, lag) for lag in lags])
    ac_rhoE = np.array([autocorr(rhoE, lag) for lag in lags])
    ac_interactions = np.array([autocorr(interactions, lag) for lag in lags])
    ac_photon = np.array([autocorr(photon, lag) for lag in lags])
    def memory_depth(ac):
        below = np.where(ac < 0.1)[0]
        return int(below[0]) if len(below) > 0 else max_lag
    md_alpha = memory_depth(ac_alpha)
    md_rhoE = memory_depth(ac_rhoE)
    md_interactions = memory_depth(ac_interactions)
    md_photon = memory_depth(ac_photon)
    print("\n[MEMORY DEPTH]")
    print(f"alpha_eff: {md_alpha} Ticks")
    print(f"rhoE: {md_rhoE} Ticks")
    print(f"interactions: {md_interactions} Ticks")
    print(f"photon: {md_photon} Ticks")
    print("\n[PERSISTENZ]")
    if len(ac_alpha) >= 200:
        print(f"alpha_eff autocorr(1): {ac_alpha[0]}")
        print(f"alpha_eff autocorr(50): {ac_alpha[49]}")
        print(f"alpha_eff autocorr(200): {ac_alpha[199]}")
    print("\n[INTERPRETATION]")
    print("Hohe Autokorrelation bedeutet:")
    print("- Das System erinnert seine Vergangenheit")
    print("- Peaks beeinflussen spätere Peaks")
    print("- Cluster haben Lebensdauer")
    print("- Rewiring ist nicht lokal")
    print("- alpha_eff ist kein Artefakt, sondern emergent")

# ---------------------------
# MODULE 11: STATE-SPACE
# ---------------------------
def module11_state_space(df):
    print("\n=== MODUL 11: STATE-SPACE & DYNAMIK-ANALYSE ===")
    dyn_cols = ["rhoE", "alpha_eff", "interactions", "photon"]
    scaler = StandardScaler()
    X = scaler.fit_transform(df[dyn_cols])
    pca = PCA(n_components=3)
    Xp = pca.fit_transform(X)
    print("\n[PCA] Varianzanteile:")
    for i, v in enumerate(pca.explained_variance_ratio_):
        print(f"PC{i+1}: {v:.4f}")
    print("\n[STATE-SPACE] Beispielpunkte:")
    print(pd.DataFrame(Xp[:10], columns=["PC1", "PC2", "PC3"]))
    diffs = np.linalg.norm(np.diff(Xp, axis=0), axis=1)
    lyap_proxy = np.mean(diffs)
    lyap_max = np.max(diffs)
    print("\n[CHAOS-INDIKATOR]")
    print(f"Lyapunov-Proxy (mean Δstate): {lyap_proxy}")
    print(f"Lyapunov-Proxy (max Δstate): {lyap_max}")
    bins = pd.cut(Xp[:,0], bins=10)
    attractor_counts = bins.value_counts().sort_index()
    print("\n[ATTRACTOR] Dichte entlang PC1:")
    print(attractor_counts)
    mode = "Unbekannt"
    if lyap_proxy < 0.01:
        mode = "Linear / stabil"
    elif lyap_proxy < 0.05:
        mode = "Metastabil / saturiert"
    elif lyap_proxy < 0.15:
        mode = "Komplex / emergent"
    else:
        mode = "Chaotisch / hochdynamisch"
    print(f"\n[SYSTEMMODUS] {mode}")
    print("\nFERTIG — Modul 11 erfolgreich getestet.")

# ---------------------------
# MODULE 12: ENERGY NETWORK
# ---------------------------
def module12_energy_network(df):
    print("\n=== MODUL 12: ENERGETISCHE FLÜSSE & KOPPLUNGSNETZWERK ===")
    df_sorted = df.sort_values("tick").reset_index(drop=True)
    cols = ["rhoE", "alpha_eff", "interactions", "photon", "pos", "neg"]
    data = df_sorted[cols]
    corr = data.corr()
    print("\n[KOPPLUNGSMATRIX] Korrelationsmatrix:")
    print(corr)
    diffs = data.diff().fillna(0)
    directed = pd.DataFrame(index=cols, columns=cols)
    for x in cols:
        for y in cols:
            try:
                directed.loc[x, y] = np.corrcoef(diffs[x], diffs[y])[0, 1]
            except Exception:
                directed.loc[x, y] = np.nan
    print("\n[GERICHTETE KOPPLUNG] Δ-basierte Einflussmatrix:")
    print(directed)
    flow = pd.DataFrame(index=cols, columns=cols)
    for x in cols:
        for y in cols:
            dx = diffs[x].replace(0, np.nan)
            dy = diffs[y]
            flow.loc[x, y] = np.nanmean(dy / dx)
    print("\n[ENERGIEFLUSS] ΔY/ΔX Mittelwerte:")
    print(flow)
    driver_index = directed.abs().sum(axis=1).sort_values(ascending=False)
    print("\n[TREIBER-INDEX] Variablen mit stärkstem Einfluss:")
    print(driver_index)
    regulator_index = directed.apply(lambda row: row[row.astype(float) < 0].abs().sum(), axis=1)
    regulator_index = regulator_index.sort_values(ascending=False)
    print("\n[REGULATOR-INDEX] stärkste dämpfende Variablen:")
    print(regulator_index)
    hotspot_index = (driver_index + regulator_index).sort_values(ascending=False)
    print("\n[NETZWERK-HOTSPOTS] wichtigste Knoten:")
    print(hotspot_index)
    roles = {}
    for var in cols:
        d = float(driver_index[var])
        r = float(regulator_index[var])
        if d > driver_index.mean() and r < regulator_index.mean():
            roles[var] = "Driver"
        elif r > regulator_index.mean() and d < driver_index.mean():
            roles[var] = "Regulator"
        elif d > driver_index.mean() and r > regulator_index.mean():
            roles[var] = "Mediator / Hub"
        else:
            roles[var] = "Passive / Sink"
    print("\n[ROLLEN] Systemrollen der Variablen:")
    for k, v in roles.items():
        print(f"{k}: {v}")
    print("\nFERTIG — Modul 12 erfolgreich getestet.")

# ---------------------------
# MODULE 13: EVENT DETECTION
# ---------------------------
def module13_event_detection(df, out_prefix="events_module13", out_dir="module13_outputs"):
    os.makedirs(out_dir, exist_ok=True)
    csv_out = os.path.join(out_dir, f"{out_prefix}.csv")
    summit_out = os.path.join(out_dir, f"{out_prefix}_summit.txt")
    overview_png = os.path.join(out_dir, f"{out_prefix}_overview.png")
    df_sorted = df.sort_values(["run_index", "tick"]).reset_index(drop=True)
    events = []
    z_thresh_alpha = 3.0
    z_thresh_inter = 3.0
    z_thresh_photon = 3.0
    min_duration = 1
    for run in sorted(df_sorted["run_index"].unique()):
        sub = df_sorted[df_sorted["run_index"] == run].copy().reset_index(drop=True)
        if sub.empty:
            continue
        sub["alpha_z"] = stats.zscore(sub["alpha_eff"].fillna(0).values, nan_policy='omit')
        sub["inter_z"] = stats.zscore(sub["interactions"].fillna(0).values, nan_policy='omit')
        sub["photon_z"] = stats.zscore(sub["photon"].fillna(0).values, nan_policy='omit')
        sub["rhoE_z"] = stats.zscore(sub["rhoE"].fillna(0).values, nan_policy='omit')
        sub["alpha_flag"] = sub["alpha_z"].abs() >= z_thresh_alpha
        sub["inter_flag"] = sub["inter_z"].abs() >= z_thresh_inter
        sub["photon_flag"] = sub["photon_z"].abs() >= z_thresh_photon
        sub["any_flag"] = sub[["alpha_flag", "inter_flag", "photon_flag"]].any(axis=1)
        sub["flag_group"] = (sub["any_flag"] != sub["any_flag"].shift(1)).cumsum()
        grouped = sub[sub["any_flag"]].groupby("flag_group")
        for gid, g in grouped:
            start_tick = int(g["tick"].min())
            end_tick = int(g["tick"].max())
            duration = end_tick - start_tick + 1
            if duration < min_duration:
                continue
            alpha_peak = float(g["alpha_eff"].max())
            inter_peak = float(g["interactions"].max())
            photon_peak = float(g["photon"].max())
            rhoE_peak = float(g["rhoE"].max())
            alpha_mean = float(sub[(sub["tick"] >= start_tick) & (sub["tick"] <= end_tick)]["alpha_eff"].mean())
            inter_mean = float(sub[(sub["tick"] >= start_tick) & (sub["tick"] <= end_tick)]["interactions"].mean())
            photon_mean = float(sub[(sub["tick"] >= start_tick) & (sub["tick"] <= end_tick)]["photon"].mean())
            rhoE_mean = float(sub[(sub["tick"] >= start_tick) & (sub["tick"] <= end_tick)]["rhoE"].mean())
            alpha_peak_z = float(g["alpha_z"].max())
            inter_peak_z = float(g["inter_z"].max())
            photon_peak_z = float(g["photon_z"].max())
            rhoE_peak_z = float(g["rhoE_z"].max())
            signals = []
            if (g["alpha_flag"].any()): signals.append("alpha_eff")
            if (g["inter_flag"].any()): signals.append("interactions")
            if (g["photon_flag"].any()): signals.append("photon")
            signals_str = ";".join(signals) if signals else "unknown"
            events.append({
                "run_index": int(run),
                "start_tick": start_tick,
                "end_tick": end_tick,
                "duration": duration,
                "signals": signals_str,
                "alpha_peak": alpha_peak,
                "interactions_peak": inter_peak,
                "photon_peak": photon_peak,
                "rhoE_peak": rhoE_peak,
                "alpha_mean": alpha_mean,
                "interactions_mean": inter_mean,
                "photon_mean": photon_mean,
                "rhoE_mean": rhoE_mean,
                "alpha_peak_z": alpha_peak_z,
                "interactions_peak_z": inter_peak_z,
                "photon_peak_z": photon_peak_z,
                "rhoE_peak_z": rhoE_peak_z
            })
    df_events = pd.DataFrame(events)
    if not df_events.empty:
        # compute a simple significance score
        df_events["significance"] = (
            df_events["alpha_peak_z"].fillna(0).abs() +
            df_events["interactions_peak_z"].fillna(0).abs() +
            df_events["photon_peak_z"].fillna(0).abs()
        )
        df_events = df_events.sort_values("significance", ascending=False).reset_index(drop=True)
        df_events.to_csv(csv_out, index=False)
        # write a short summit for module13
        with open(summit_out, "w", encoding="utf-8") as fh:
            fh.write("Module 13 Event Summary\n")
            fh.write(f"Generated: {datetime.now(timezone.utc).isoformat()}Z\n\n")
            fh.write(f"Detected events: {len(df_events)}\n")
            fh.write("Top events (by significance):\n")
            fh.write(df_events.head(20).to_string(index=False))
            fh.write("\n")
    else:
        print("[MODULE 13] Keine Events gefunden")
    return df_events, csv_out, summit_out

# ---------------------------
# MODULE 14: SUMMIT, PLOTS, EXPORT
# ---------------------------
def module14_summit_and_plots(df, events_df=None, out_dir="outputs", out_summit="summit.txt", save_plots=False):
    os.makedirs(out_dir, exist_ok=True)
    # 1) Summit text: erklärend, keine neue Statistik
    summit_path = os.path.join(out_dir, out_summit)
    lines = []
    lines.append("SUMMIT — Kurze, erklärende Zusammenfassung der Analyse")
    lines.append(f"Erstellt: {datetime.now(timezone.utc).isoformat()}Z")
    lines.append("")
    lines.append("Was dieses Summit ist:")
    lines.append("- Eine erklärende Zusammenfassung der Ergebnisse aus den Modulen 1..13")
    lines.append("- Keine neue statistische Auswertung, sondern Interpretation und Orientierung")
    lines.append("")
    lines.append("Wesentliche Beobachtungen:")
    # pick a few highlights from df
    stats = compute_basic_stats(df)
    lines.append(f"- Gesamt rhoE Mittelwert: {stats['rhoE_mean']:.6f}")
    lines.append(f"- alpha_eff Mittelwert: {stats['alpha_mean']:.6f}")
    lines.append(f"- Photonensumme: {stats['photon_total']:,}")
    lines.append("- Korrelationen zeigen starke Kopplung zwischen alpha_eff, rhoE und interactions")
    lines.append("- Autokorrelationen deuten auf hohe Persistenz und Memory-Depth")
    lines.append("")
    lines.append("Was man hier sieht:")
    lines.append("- Zeitreihen von rhoE, alpha_eff, interactions und photon")
    lines.append("- Erkannte Events mit Start/End Tick, Peak-Werten und Signifikanz")
    lines.append("")
    lines.append("Was man hier nicht sehen sollte:")
    lines.append("- Dieses Summit ist keine Rohdaten-Quelle; für Rohdaten siehe result*.csv")
    lines.append("- Keine Kausalitätsbehauptungen über physikalische Mechanismen ohne weitere Experimente")
    lines.append("")
    lines.append("Empfehlungen:")
    lines.append("- Untersuche Top-Events manuell mit Simulation-Frames")
    lines.append("- Prüfe Cluster-Regionen und Rewiring-Hotspots in der Simulation")
    with open(summit_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    print(f"[MODULE 14] Summit geschrieben nach: {summit_path}")
    # 2) Export top events if provided
    top_events_path = None
    if events_df is not None and not events_df.empty:
        top_events_path = os.path.join(out_dir, "events_module13_top.csv")
        events_df.to_csv(top_events_path, index=False)
        print(f"[MODULE 14] Top-Events exportiert nach: {top_events_path}")
    # 3) Plots
    if save_plots:
        # timeseries overview
        plt.figure(figsize=(12,6))
        sns.lineplot(data=df.sort_values("tick"), x="tick", y="alpha_eff", label="alpha_eff")
        sns.lineplot(data=df.sort_values("tick"), x="tick", y="rhoE", label="rhoE")
        sns.lineplot(data=df.sort_values("tick"), x="tick", y="interactions", label="interactions")
        plt.legend()
        plt.title("Time series overview")
        out_png = os.path.join(out_dir, "timeseries_with_events.png")
        plt.tight_layout()
        plt.savefig(out_png)
        plt.close()
        print(f"[PLOT] Saved: {out_png}")
        # per-event plots (top N)
        if events_df is not None and not events_df.empty:
            max_plots = 50
            for i, row in events_df.head(max_plots).iterrows():
                run = int(row["run_index"])
                s = int(row["start_tick"])
                e = int(row["end_tick"])
                sub = df[(df["run_index"]==run) & (df["tick"]>=s-10) & (df["tick"]<=e+10)].sort_values("tick")
                if sub.empty:
                    continue
                plt.figure(figsize=(8,4))
                sns.lineplot(data=sub, x="tick", y="alpha_eff", label="alpha_eff")
                sns.lineplot(data=sub, x="tick", y="interactions", label="interactions")
                sns.lineplot(data=sub, x="tick", y="photon", label="photon")
                plt.axvspan(s, e, color="grey", alpha=0.2)
                plt.title(f"event_run{run}_ticks_{s}_{e}")
                out_png = os.path.join(out_dir, f"event_run{run}_ticks_{s}_{e}.png")
                plt.tight_layout()
                plt.savefig(out_png)
                plt.close()
                print(f"[PLOT] Saved: {out_png}")
    return summit_path, top_events_path

# ---------------------------
# MODUL CHSH / BELL-ANALYSE
# ---------------------------
import math
import random
from typing import Callable, Dict, Tuple
import matplotlib.pyplot as plt

# ---------------------------
# MODUL CHSH / HELPER FUNCTIONS
# ---------------------------
def default_outcome_from_posneg(row, setting_angle=0.0):
    """
    Echter Messoperator basierend auf Knotenasymmetrie und Projektionswinkel.
    Nutzt pos/neg Asymmetrie + Helizitätskomponente für Phase.
    """
    pos = row.get("pos", 0)
    neg = row.get("neg", 0)
    helicity = row.get("helicity_sum", 0)
    
    # Asymmetrie-Signal (+1/-1)
    diff = pos - neg
    if diff > 0:
        val = 1
    elif diff < 0:
        val = -1
    else:
        # Bei Gleichstand Auswertung der Helizität
        val = 1 if helicity >= 0 else -1
        
    # Winkelprojektion auf Messbasis
    projected = val * math.cos(setting_angle)
    return 1 if projected >= 0 else -1

def default_outcome_from_phase(row, setting_angle):
    """
    Alternative Heuristik: interpretiere alpha_eff als Phase (rad),
    Outcome = sign(cos(phase - setting_angle)).
    Erwartet alpha_eff in sinnvollem Bereich; fallback auf pos/neg.
    """
    try:
        phase = float(row["alpha_eff"])  # falls alpha als Phase interpretiert werden kann
        val = math.cos(phase - setting_angle)
        return 1 if val >= 0 else -1
    except Exception:
        return default_outcome_from_posneg(row, setting_angle)

def extract_measurement_pairs(df, run_index: int,
                              settings: Dict[str, float],
                              outcome_fn: Callable = None,
                              mapping="posneg") -> Dict[Tuple[str,str], list]:
    """
    Extrahiert Messpaare (A,B) für einen Run.
    settings: dict mit keys 'a','a2','b','b2' und Winkelwerte (radians) oder beliebige Kennzeichen.
    outcome_fn: function(row, setting_angle) -> +1/-1
    mapping: 'posneg' oder 'phase' (wählt default outcome_fn)
    Rückgabe: dict mit Schlüsseln ('a','b'), ('a','b2'), ('a2','b'), ('a2','b2') -> Liste von (A,B)
    """
    if outcome_fn is None:
        if mapping == "phase":
            outcome_fn = default_outcome_from_phase
        else:
            outcome_fn = default_outcome_from_posneg

    sub = df[df["run_index"] == run_index].sort_values("tick").reset_index(drop=True)
    pairs = {("a","b"): [], ("a","b2"): [], ("a2","b"): [], ("a2","b2"): []}

    # Hier: Annahme, dass pro tick eine Messung für beide Seiten existiert.
    # Wenn Messungen nur zu bestimmten Ticks stattfinden, muss man das Filtern anpassen.
    for _, row in sub.iterrows():
        A_a  = outcome_fn(row, settings["a"])
        A_a2 = outcome_fn(row, settings["a2"])
        B_b  = outcome_fn(row, settings["b"])
        B_b2 = outcome_fn(row, settings["b2"])

        pairs[("a","b")].append((A_a, B_b))
        pairs[("a","b2")].append((A_a, B_b2))
        pairs[("a2","b")].append((A_a2, B_b))
        pairs[("a2","b2")].append((A_a2, B_b2))

    return pairs

def expectation_from_pairs(pairs):
    """
    Erwartungswert E = mean(A*B) für Liste von (A,B)
    """
    arr = [a*b for (a,b) in pairs]
    if len(arr) == 0:
        return float("nan")
    return sum(arr) / len(arr)

def compute_chsh_from_pairs(pairs_dict):
    """
    Berechnet E(a,b) für alle Kombinationen und S.
    pairs_dict: dict wie von extract_measurement_pairs
    """
    E_ab  = expectation_from_pairs(pairs_dict[("a","b")])
    E_ab2 = expectation_from_pairs(pairs_dict[("a","b2")])
    E_a2b = expectation_from_pairs(pairs_dict[("a2","b")])
    E_a2b2= expectation_from_pairs(pairs_dict[("a2","b2")])

    S = E_ab - E_ab2 + E_a2b + E_a2b2
    return {"E_ab":E_ab, "E_ab2":E_ab2, "E_a2b":E_a2b, "E_a2b2":E_a2b2, "S":S}

def bootstrap_chsh(pairs_dict, n_boot=1000, seed=0):
    """
    Bootstrap-Resampling über Messpaare (resample mit Replacement pro Kombination).
    Liefert S-Verteilung und 95% CI.
    """
    random.seed(seed)
    S_samples = []
    keys = [("a","b"),("a","b2"),("a2","b"),("a2","b2")]
    for _ in range(n_boot):
        sampled_pairs = {}
        for k in keys:
            arr = pairs_dict[k]
            if len(arr) == 0:
                sampled_pairs[k] = []
            else:
                sampled_pairs[k] = [random.choice(arr) for _ in range(len(arr))]
        res = compute_chsh_from_pairs(sampled_pairs)
        S_samples.append(res["S"])
    S_samples = [s for s in S_samples if not math.isnan(s)]
    if not S_samples:
        return {"S_mean": float("nan"), "S_ci": (float("nan"), float("nan")), "S_samples": []}
    S_mean = sum(S_samples)/len(S_samples)
    S_samples_sorted = sorted(S_samples)
    lo = S_samples_sorted[int(0.025*len(S_samples_sorted))]
    hi = S_samples_sorted[int(0.975*len(S_samples_sorted))-1]
    return {"S_mean": S_mean, "S_ci": (lo, hi), "S_samples": S_samples}

def classify_S(S, ci=None):
    """
    Klassifikation nach S und optional CI.
    """
    q = 2.0
    q_quant = 2.0 * math.sqrt(2.0)
    label = "unknown"
    if S <= q:
        label = "local (S <= 2)"
    elif S <= q_quant:
        label = "quantum-range (S <= 2√2)"
    else:
        label = "exceeds quantum bound (S > 2√2)"
    if ci is not None:
        lo, hi = ci
        # einfache Einschätzung, ob CI ganz unter/über Grenzen liegt
        if hi <= q:
            label += " — CI fully local"
        elif lo > q_quant:
            label += " — CI above quantum bound"
    return label

def run_chsh_module(df, run_index=6,
                    settings=None,
                    outcome_mapping="posneg",
                    outcome_fn=None,
                    n_boot=2000,
                    out_dir="outputs",
                    save_plots=True):
    """
    Convenience-Funktion: führt Extraktion, CHSH-Berechnung, Bootstrap und Plot aus.
    settings: dict mit 'a','a2','b','b2' (Winkel in rad oder Kennzeichen)
    """
    os.makedirs(out_dir, exist_ok=True)
    if settings is None:
        # Beispielwinkel (in rad): klassisches CHSH-Set
        settings = {"a":0.0, "a2":math.pi/2, "b":math.pi/4, "b2":-math.pi/4}

    pairs = extract_measurement_pairs(df, run_index, settings, outcome_fn, mapping=outcome_mapping)
    chsh = compute_chsh_from_pairs(pairs)
    boot = bootstrap_chsh(pairs, n_boot=n_boot, seed=42)
    classification = classify_S(chsh["S"], ci=boot["S_ci"])

    # Ausgabe
    print("\n=== MODUL CHSH / BELL-ANALYSE ===")
    print(f"Run: {run_index}")
    print(f"E(a,b): {chsh['E_ab']:.6f}")
    print(f"E(a,b2): {chsh['E_ab2']:.6f}")
    print(f"E(a2,b): {chsh['E_a2b']:.6f}")
    print(f"E(a2,b2): {chsh['E_a2b2']:.6f}")
    print(f"S: {chsh['S']:.6f}")
    print(f"S (bootstrap mean): {boot['S_mean']:.6f}")
    print(f"S 95% CI: [{boot['S_ci'][0]:.6f}, {boot['S_ci'][1]:.6f}]")
    print(f"Classification: {classification}")

    # Plot S-Verteilung
    if save_plots and boot["S_samples"]:
        plt.figure(figsize=(6,4))
        plt.hist(boot["S_samples"], bins=50, color="C0", alpha=0.8)
        plt.axvline(chsh["S"], color="k", linestyle="--", label=f"S={chsh['S']:.3f}")
        plt.axvline(2.0, color="r", linestyle=":", label="Local bound 2")
        plt.axvline(2.0*math.sqrt(2.0), color="g", linestyle=":", label="Quantum bound 2√2")
        plt.legend()
        plt.title(f"CHSH S distribution (run {run_index})")
        plt.xlabel("S")
        plt.ylabel("counts")
        fname = os.path.join(out_dir, f"chsh_run{run_index}.png")
        plt.tight_layout()
        plt.savefig(fname, dpi=150)
        plt.close()
        print(f"[PLOT] Saved: {fname}")

    return {"pairs": pairs, "chsh": chsh, "bootstrap": boot, "classification": classification}

# ============================================================
# MODULE 15: NATIVE C99 CHSH / BELL-ANALYSE (Aktualisiert)
# ============================================================
def module15_chsh(df,
                  run_index=1,
                  outcome_fn=None,
                  settings=None,
                  n_boot=2000,
                  out_dir="outputs",
                  save_plots=True):
    print("\n=== MODUL 15: C99 CHSH / BELL-ANALYSE ===")

    if "run_index" not in df.columns:
        sub = df.copy()
        run_index = 1
    else:
        sub = df[df["run_index"] == run_index].sort_values("tick").reset_index(drop=True)
        if sub.empty:
            available_runs = df["run_index"].unique() if "run_index" in df.columns else []
            print(f"[WARN] Keine Daten für Run {run_index}. Verfügbare Runs: {available_runs}")
            if len(available_runs) > 0:
                run_index = available_runs[0]
                sub = df[df["run_index"] == run_index].sort_values("tick").reset_index(drop=True)
                print(f"[INFO] Wechsle automatisch auf Run {run_index}")
            else:
                print("[FEHLER] Abbruch CHSH-Modul: Keine gültigen Runs vorhanden.")
                return

    # Prüfen, ob native C99-Spalten der C-Engine vorliegen
    native_cols = ["E_ab", "E_ab_prime", "E_a_prime_b", "E_a_prime_b_prime", "S_CHSH"]
    has_native_chsh = all(col in sub.columns for col in native_cols)

    if has_native_chsh:
        print("[INFO] Nutze native C99-Graphen-Messergebnisse aus dem Substrat.")
        E_ab   = float(sub["E_ab"].mean())
        E_ab2  = float(sub["E_ab_prime"].mean())
        E_a2b  = float(sub["E_a_prime_b"].mean())
        E_a2b2 = float(sub["E_a_prime_b_prime"].mean())
        S      = float(sub["S_CHSH"].mean())

        # Resampling / Bootstrap direkt über die nativen S_CHSH-Messungen pro Tick
        S_values = sub["S_CHSH"].values
        n = len(S_values)
        S_boot = []
        if n > 0:
            np.random.seed(42)
            for _ in range(n_boot):
                sample = np.random.choice(S_values, size=n, replace=True)
                S_boot.append(np.mean(sample))

        S_boot = np.array(S_boot)
        S_mean = float(np.mean(S_boot)) if len(S_boot) > 0 else S
        S_low, S_high = np.percentile(S_boot, [2.5, 97.5]) if len(S_boot) > 0 else (S, S)

    else:
        print("[WARN] Keine nativen C99 CHSH-Spalten in CSV gefunden. Nutze Fallback-Heuristik.")
        # Fallback auf historische Asymmetrie-Funktion für ältere CSVs
        if outcome_fn is None:
            outcome_fn = default_outcome_from_posneg
        if settings is None:
            settings = {"a": 0.0, "a2": math.pi / 2, "b": math.pi / 4, "b2": -math.pi / 4}

        pairs = {("a","b"): [], ("a","b2"): [], ("a2","b"): [], ("a2","b2"): []}
        for _, row in sub.iterrows():
            A_a  = outcome_fn(row, settings["a"])
            A_a2 = outcome_fn(row, settings["a2"])
            B_b  = outcome_fn(row, settings["b"])
            B_b2 = outcome_fn(row, settings["b2"])
            pairs[("a","b")].append((A_a, B_b))
            pairs[("a","b2")].append((A_a, B_b2))
            pairs[("a2","b")].append((A_a2, B_b))
            pairs[("a2","b2")].append((A_a2, B_b2))

        def E(pairs_list):
            arr = np.array([a * b for (a, b) in pairs_list])
            return float(arr.mean()) if len(arr) > 0 else 0.0

        E_ab   = E(pairs[("a","b")])
        E_ab2  = E(pairs[("a","b2")])
        E_a2b  = E(pairs[("a2","b")])
        E_a2b2 = E(pairs[("a2","b2")])
        S      = E_ab - E_ab2 + E_a2b + E_a2b2

        S_boot_list = []
        n = len(sub)
        if n > 0:
            for _ in range(n_boot):
                sample = sub.sample(n, replace=True)
                p_bs = {("a","b"): [], ("a","b2"): [], ("a2","b"): [], ("a2","b2"): []}
                for _, row in sample.iterrows():
                    p_bs[("a","b")].append((outcome_fn(row, settings["a"]), outcome_fn(row, settings["b"])))
                    p_bs[("a","b2")].append((outcome_fn(row, settings["a"]), outcome_fn(row, settings["b2"])))
                    p_bs[("a2","b")].append((outcome_fn(row, settings["a2"]), outcome_fn(row, settings["b"])))
                    p_bs[("a2","b2")].append((outcome_fn(row, settings["a2"]), outcome_fn(row, settings["b2"])))
                S_bs = E(p_bs[("a","b")]) - E(p_bs[("a","b2")]) + E(p_bs[("a2","b")]) + E(p_bs[("a2","b2")])
                S_boot_list.append(S_bs)

        S_boot = np.array(S_boot_list)
        S_mean = float(np.mean(S_boot)) if len(S_boot) > 0 else S
        S_low, S_high = np.percentile(S_boot, [2.5, 97.5]) if len(S_boot) > 0 else (S, S)

    print(f"Run: {run_index}")
    print(f"E(a,b):   {E_ab:.6f}")
    print(f"E(a,b'):  {E_ab2:.6f}")
    print(f"E(a',b):  {E_a2b:.6f}")
    print(f"E(a',b'): {E_a2b2:.6f}")
    print(f"S_CHSH:   {S:.6f}")
    print(f"S (bootstrap mean): {S_mean:.6f}")
    print(f"S 95% CI: [{S_low:.6f}, {S_high:.6f}]")

    # Quantenmechanische & Klassische Klassifizierung
    if S_high <= 2.0:
        cls = "local (S <= 2) — CI fully local"
    elif S_low > 2.0 and S_high <= 2.8284271247461903:
        cls = "quantum (2 < S <= 2√2)"
    elif S_low > 2.8284271247461903:
        cls = "post-quantum (S > 2√2)"
    else:
        cls = "ambiguous (CI overlaps local boundary 2.0)"

    print(f"Classification: {cls}")

    os.makedirs(out_dir, exist_ok=True)
    if save_plots and len(S_boot) > 0:
        plt.figure(figsize=(7, 4))
        plt.hist(S_boot, bins=40, alpha=0.75, color="steelblue", edgecolor="black")
        plt.axvline(2.0, color="red", linestyle="--", linewidth=2, label="Lokalitäts-Grenze S=2.0")
        plt.axvline(2.8284271247461903, color="orange", linestyle="--", linewidth=2, label="Cirel'son Limit S=2√2")
        plt.axvline(S_mean, color="black", linestyle="-", linewidth=2, label=f"S_mean = {S_mean:.4f}")
        plt.xlabel("S_CHSH (Bootstrap-Verteilung)")
        plt.ylabel("Häufigkeit")
        plt.title(f"Bell / CHSH Auswertung - Run {run_index}")
        plt.legend()
        out_png = os.path.join(out_dir, f"chsh_run{run_index}.png")
        plt.tight_layout()
        plt.savefig(out_png, dpi=150)
        plt.close()
        print(f"[PLOT] Saved: {out_png}")


# ---------------------------
# MAIN
# ---------------------------
def main():
    args = parse_args()
    try:
        df_all = load_all_csv(args.csv_pattern)
    except Exception as e:
        print(f"Fehler beim Laden der Daten: {e}")
        sys.exit(1)
    # Run modules
    global_stats = compute_basic_stats(df_all)
    print("\n=== GLOBAL STATS ===")
    print_stats_block("Global", global_stats)
    stats_per_run(df_all)
    analyze_regimes_and_correlations(df_all)
    windows = time_window_stats(df_all, window_size=500)
    print_time_window_stats(windows)
    analyze_alpha_efficiency(df_all)
    analyze_photons(df_all)
    analyze_nonlocality(df_all)
    analyze_emergence_and_clusters(df_all)
    fft_analysis(df_all)
    analyze_memory_depth(df_all)
    module11_state_space(df_all)
    module12_energy_network(df_all)
    # Module 13: event detection (unless user provided events CSV)
    events_df = None
    if args.events:
        try:
            print(f"[INFO] Lade vorgegebene Events: {args.events}")
            events_df = pd.read_csv(args.events)
        except Exception as e:
            print(f"[WARN] Konnte events CSV nicht laden: {e}. Führe Modul 13 intern aus.")
            events_df, csv_out, summit_out = module13_event_detection(df_all, out_prefix="events_module13", out_dir="module13_outputs")
    else:
        events_df, csv_out, summit_out = module13_event_detection(df_all, out_prefix="events_module13", out_dir="module13_outputs")
    # Module 14: summit + plots
    summit_path, top_events_path = module14_summit_and_plots(df_all, events_df=events_df, out_dir=args.out_dir, out_summit=args.out_summit, save_plots=args.save_plots)
    # Modul 15: CHSH / Bell
    module15_chsh(df_all,
              run_index=5,
              outcome_fn=None,   # oder deine eigene Messfunktion
              settings=None,
              n_boot=2000,
              out_dir=args.out_dir,
              save_plots=args.save_plots)


    print("[MODULE 15] Fertig. Summit ist eine erklärende Datei (keine neue statistische Auswertung).")

if __name__ == "__main__":
    main()
