"""Compare open-loop (round-robin) vs ML-based (UCB1) scan scheduling.

Runs the same synthetic RF environment against both strategies and plots
cumulative interception count and hit-rate over time to show how the
learning scheduler pulls ahead of the naive sweep.

Usage:
    python3 compare_schedulers.py
Produces: scheduler_comparison.png
"""
import random

import matplotlib.pyplot as plt

from scheduler import SCHEDULERS
from sensor import SENSOR_PD, SENSOR_PFA
from simulator import RFEnvironment

STEPS = 800
NUM_BANDS = 16
NUM_EMITTERS = 6
SEED = 42


def run(scheduler_name: str):
    random.seed(SEED)
    env = RFEnvironment(num_bands=NUM_BANDS, num_emitters=NUM_EMITTERS, seed=SEED)
    scheduler = SCHEDULERS[scheduler_name](NUM_BANDS)

    hits = 0
    cumulative_hits = []
    hit_rate_curve = []
    reward = 0.0
    reward_curve = []

    for t in range(STEPS):
        active = env.step(t)
        band = scheduler.choose(t)
        emitters_here = active.get(band, [])
        band_active = len(emitters_here) > 0

        if band_active and random.random() < SENSOR_PD:
            hits += 1
            step_reward = 1.0
            detected = True
        elif not band_active and random.random() < SENSOR_PFA:
            step_reward = -0.5
            detected = True
        elif band_active:
            step_reward = -0.3
            detected = False
        else:
            step_reward = -0.05
            detected = False

        scheduler.update(band, step_reward, detected)
        reward += step_reward

        cumulative_hits.append(hits)
        hit_rate_curve.append(hits / (t + 1))
        reward_curve.append(reward)

    return cumulative_hits, hit_rate_curve, reward_curve


def main():
    strategies = {
        "round_robin": "Round Robin (open-loop)",
        "random": "Random (open-loop)",
        "ucb1": "UCB1 Bandit (ML / smart scan)",
        "ml_predictor": "Learned Predictor (offline-trained ML)",
    }

    results = {name: run(name) for name in strategies}

    fig, axes = plt.subplots(1, 3, figsize=(16, 5))
    x = list(range(STEPS))

    for name, label in strategies.items():
        cumulative_hits, hit_rate_curve, reward_curve = results[name]
        axes[0].plot(x, cumulative_hits, label=label)
        axes[1].plot(x, [r * 100 for r in hit_rate_curve], label=label)
        axes[2].plot(x, reward_curve, label=label)

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

    fig.suptitle("Smart Scan (ML) vs Open-Loop Scan Scheduling Efficiency")
    fig.tight_layout()
    fig.savefig("scheduler_comparison.png", dpi=150)
    print("Saved scheduler_comparison.png")

    for name, label in strategies.items():
        cumulative_hits, hit_rate_curve, _ = results[name]
        print(f"{label:35s} final intercepts={cumulative_hits[-1]:4d}  final hit rate={hit_rate_curve[-1]*100:5.1f}%")


if __name__ == "__main__":
    main()
