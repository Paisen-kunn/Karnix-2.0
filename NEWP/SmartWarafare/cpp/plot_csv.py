"""Plot scheduler_comparison.csv produced by the C++ compare_schedulers binary.

Usage:
    python3 plot_csv.py [csv_path]
Produces: scheduler_comparison_cpp.png
"""
import csv
import sys
from collections import defaultdict

import matplotlib.pyplot as plt

path = sys.argv[1] if len(sys.argv) > 1 else "scheduler_comparison.csv"

series = defaultdict(lambda: {"step": [], "hits": [], "hit_rate": [], "reward": []})

with open(path, newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        s = series[row["strategy"]]
        s["step"].append(int(row["step"]))
        s["hits"].append(int(row["cumulative_hits"]))
        s["hit_rate"].append(float(row["hit_rate"]) * 100)
        s["reward"].append(float(row["cumulative_reward"]))

fig, axes = plt.subplots(1, 3, figsize=(16, 5))
for label, s in series.items():
    axes[0].plot(s["step"], s["hits"], label=label)
    axes[1].plot(s["step"], s["hit_rate"], label=label)
    axes[2].plot(s["step"], s["reward"], label=label)

axes[0].set_title("Cumulative Intercepts")
axes[0].set_xlabel("Time step")
axes[0].set_ylabel("Total successful intercepts")
axes[0].legend()

axes[1].set_title("Intercept Hit Rate Over Time")
axes[1].set_xlabel("Time step")
axes[1].set_ylabel("Hit rate (%)")
axes[1].legend()

axes[2].set_title("Cumulative Reward (scheduler quality)")
axes[2].set_xlabel("Time step")
axes[2].set_ylabel("Cumulative reward")
axes[2].legend()

fig.suptitle("Smart Scan (ML) vs Open-Loop Scan Scheduling Efficiency [C++ engine]")
fig.tight_layout()
fig.savefig("scheduler_comparison_cpp.png", dpi=150)
print("Saved scheduler_comparison_cpp.png")
