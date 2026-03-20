import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
from matplotlib.ticker import FuncFormatter

# The two folders we are comparing
DIR_BASELINE = "results_baseline"
DIR_ISOLATED = "results"

# The queues we want to compare
queues = {
    "FAA": "FaaTicketQueueAlignas",
    "Vyukov": "VyukovStateMachineAlignas",
    "Mutex": "BasicMutexQueueAlignas",
    "Spinlock": "SpinlockQueueAlignas"
}

# Ensure output directory exists
os.makedirs("plots", exist_ok=True)

def get_dataset(directory, keyword):
    """Helper to safely load the latency arrays."""
    if not os.path.exists(directory):
        return False, []
        
    files = [f for f in os.listdir(directory) if keyword in f and "_2" in f and "latencies.csv" in f]
    if not files:
        # Fallback if the naming is slightly different
        files = [f for f in os.listdir(directory) if keyword in f and "latencies.csv" in f]
        
    if not files:
        return False, []
        
    filepath = os.path.join(directory, files[0])
    latencies = pd.read_csv(filepath, header=None)[0].values
    
    if len(latencies) == 0:
        return False, []
        
    return True, latencies


for label, keyword in queues.items():
    print(f"\nProcessing {label}...")
    
    has_base, lat_base = get_dataset(DIR_BASELINE, keyword)
    has_iso, lat_iso = get_dataset(DIR_ISOLATED, keyword)
    
    if not has_base and not has_iso:
        print(f"  -> Skipping {label}, no data found in either directory.")
        continue
        
    plt.figure(figsize=(12, 7))
    
    # Combine the arrays just to find the global min/max for the log bins 
    # so both histograms align perfectly on the same scale
    all_lats = np.concatenate([l for l in (lat_base, lat_iso) if len(l) > 0])
    min_val = max(1, all_lats.min())
    max_val = max(10000, all_lats.max())
    log_bins = np.logspace(np.log10(min_val), np.log10(max_val), 150)
    
    # Plot Baseline (Red/Orange)
    if has_base:
        sns.histplot(lat_base, bins=log_bins, color='#e07a5f', alpha=0.5, element="step", linewidth=0)
        median_base = np.median(lat_base)
        plt.axvline(median_base, color='#e07a5f', linestyle='--', linewidth=2, label=f"Baseline Median: {median_base:.1f} ns")
        
    # Plot Isolated (Blue)
    if has_iso:
        sns.histplot(lat_iso, bins=log_bins, color='#8ebad9', alpha=0.5, element="step", linewidth=0)
        median_iso = np.median(lat_iso)
        plt.axvline(median_iso, color='#8ebad9', linestyle='--', linewidth=2, label=f"Isolated Median: {median_iso:.1f} ns")

    # Formatting
    plt.title(f"The Impact of Core Isolation on {label} Queue (2 Producers)", fontsize=16)
    plt.xlabel("Latency (ns)", fontsize=14)
    plt.ylabel("Frequency", fontsize=14)
    plt.legend(fontsize=12, loc="upper right")
    
    # Log-Log scale
    plt.xscale('log')
    plt.yscale('log')
    plt.gca().xaxis.set_major_formatter(FuncFormatter(lambda x, pos: f"{int(x):,}"))
    
    plt.tight_layout()
    
    out_path = f"plots/5_Isolation_Comparison_{label}.png"
    plt.savefig(out_path)
    plt.close() # Close figure to free memory
    print(f"  -> Saved {out_path}!")