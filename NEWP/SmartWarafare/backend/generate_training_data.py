"""Generate supervised training data for the learned MLPredictorScheduler.

Runs the synthetic RF environment (simulator.py) under a round-robin scan
policy across many randomized episodes -- so every band gets scanned
regularly -- and records, for each scan event, the receiver-observable
features (this band's own past scan outcomes + elapsed time, see
features.py) together with the noisy sensor outcome (hit / no-hit) for
that scan. This mirrors exactly what MLPredictorScheduler sees in the
field, just collected offline over many episodes so a classifier can learn
which bands / times tend to be busy.

Usage:
    python3 generate_training_data.py
Produces: data/training_data.csv
"""
import csv
import os
import random

from features import FEATURE_NAMES, extract_features
from sensor import SENSOR_PD, SENSOR_PFA
from simulator import RFEnvironment

STEPS_PER_EPISODE = 600
NUM_EPISODES = 60


def run_episode(seed: int, out_rows: list) -> None:
    rng = random.Random(seed)
    num_bands = rng.choice([8, 12, 16, 20, 24])
    num_emitters = rng.randint(3, 10)
    env = RFEnvironment(num_bands=num_bands, num_emitters=num_emitters, seed=seed)

    scan_history = {b: [] for b in range(num_bands)}
    last_scan_t = {b: None for b in range(num_bands)}
    last_hit_t = {b: None for b in range(num_bands)}

    for t in range(STEPS_PER_EPISODE):
        active = env.step(t)
        band = t % num_bands  # round-robin sweep -> uniform per-band coverage for data collection

        feats = extract_features(t, scan_history[band], last_scan_t[band], last_hit_t[band])
        band_active = band in active
        detected = (rng.random() < SENSOR_PD) if band_active else (rng.random() < SENSOR_PFA)
        label = 1 if detected else 0

        out_rows.append([seed, t, band] + feats + [label])

        scan_history[band].append(label)
        last_scan_t[band] = t
        if label:
            last_hit_t[band] = t


def main():
    out_path = os.path.join(os.path.dirname(__file__), "data", "training_data.csv")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)

    rows: list = []
    for ep in range(NUM_EPISODES):
        run_episode(seed=1000 + ep, out_rows=rows)

    with open(out_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["seed", "t", "band"] + FEATURE_NAMES + ["label"])
        writer.writerows(rows)

    print(f"Wrote {len(rows)} rows ({NUM_EPISODES} episodes x up to {STEPS_PER_EPISODE} steps) to {out_path}")


if __name__ == "__main__":
    main()
