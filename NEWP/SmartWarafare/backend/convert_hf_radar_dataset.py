"""Convert downloaded Turing Synthetic Radar Dataset (TSRD) pulse-train
files (see download_hf_radar_dataset.py) into the same feature/label
schema used by generate_training_data.py, so real recorded emitter data
can be combined with the synthetic dataset when training the
MLPredictorScheduler's classifier.

Each .h5 file holds Pulse Descriptor Words (PDWs) with fields including
Time of Arrival (ToA, us) and Centre Frequency (MHz). We:
  1. Auto-detect the ToA / frequency fields inside each file (schema can
     vary; use --inspect to print the raw HDF5 structure if detection
     fails and adjust FIELD_ALIASES below).
  2. Bucket pulses into a (time-step x frequency-band) occupancy grid --
     analogous to the synthetic simulator's per-band/per-step truth.
  3. Replay a round-robin scan over that grid and emit rows in the exact
     schema as generate_training_data.py (features.py + label), so the
     output CSV can be concatenated with data/training_data.csv.

Usage:
    python3 download_hf_radar_dataset.py --split val_scan --num-files 5
    python3 convert_hf_radar_dataset.py
    python3 train_model.py --data data/training_data.csv data/training_data_hf.csv
"""
import argparse
import csv
import glob
import os
import random

import h5py
import numpy as np

from features import FEATURE_NAMES, extract_features

RAW_DIR = os.path.join(os.path.dirname(__file__), "data", "hf_radar_raw")
OUT_PATH = os.path.join(os.path.dirname(__file__), "data", "training_data_hf.csv")

FIELD_ALIASES = {
    "toa": ["toa", "time_of_arrival", "time"],
    "freq": ["freq", "frequency", "cf", "carrier"],
}


def _match_name(names, aliases):
    for name in names:
        low = name.lower()
        if any(alias in low for alias in aliases):
            return name
    return None


def find_pdw_arrays(h5file: h5py.File):
    """Return (toa_us, freq_mhz) numpy arrays for one pulse-train file."""
    datasets = []

    def visitor(name, obj):
        if isinstance(obj, h5py.Dataset):
            datasets.append((name, obj))

    h5file.visititems(visitor)
    if not datasets:
        raise ValueError("No datasets found in file")

    # Case 1: one compound/structured dataset holding all PDW fields per pulse.
    for _, ds in datasets:
        if ds.dtype.names:
            toa_key = _match_name(ds.dtype.names, FIELD_ALIASES["toa"])
            freq_key = _match_name(ds.dtype.names, FIELD_ALIASES["freq"])
            if toa_key and freq_key:
                data = ds[()]
                return np.asarray(data[toa_key], dtype=float), np.asarray(data[freq_key], dtype=float)

    # Case 2: separate flat datasets, matched by name.
    names = [n for n, _ in datasets]
    toa_name = _match_name(names, FIELD_ALIASES["toa"])
    freq_name = _match_name(names, FIELD_ALIASES["freq"])
    if toa_name and freq_name:
        by_name = dict(datasets)
        return np.asarray(by_name[toa_name][()], dtype=float), np.asarray(by_name[freq_name][()], dtype=float)

    raise ValueError(
        f"Could not auto-detect ToA / frequency fields. Datasets in file: {names}. "
        "Run with --inspect to see full structure and adjust FIELD_ALIASES in this script."
    )


def inspect(path: str) -> None:
    with h5py.File(path, "r") as f:
        print(f"--- {path} ---")

        def visitor(name, obj):
            if isinstance(obj, h5py.Dataset):
                extra = f"  fields={obj.dtype.names}" if obj.dtype.names else ""
                print(f"  {name}  shape={obj.shape} dtype={obj.dtype}{extra}")

        f.visititems(visitor)


def build_occupancy_grid(toa_us, freq_mhz, num_bands, freq_min_mhz, freq_max_mhz, num_steps):
    grid = np.zeros((num_steps, num_bands), dtype=bool)
    if toa_us.size == 0:
        return grid
    band_width = (freq_max_mhz - freq_min_mhz) / num_bands
    duration_us = max(toa_us.max() - toa_us.min(), 1e-6)
    time_step_us = duration_us / num_steps
    t_idx = np.floor((toa_us - toa_us.min()) / time_step_us).astype(int)
    b_idx = np.floor((freq_mhz - freq_min_mhz) / band_width).astype(int)
    valid = (t_idx >= 0) & (t_idx < num_steps) & (b_idx >= 0) & (b_idx < num_bands)
    grid[t_idx[valid], b_idx[valid]] = True
    return grid


def grid_to_training_rows(grid: np.ndarray, seed_tag: str) -> list:
    """Replay a round-robin scan over a real occupancy grid and emit rows
    in the same schema as generate_training_data.py."""
    num_steps, num_bands = grid.shape
    scan_history = {b: [] for b in range(num_bands)}
    last_scan_t = {b: None for b in range(num_bands)}
    last_hit_t = {b: None for b in range(num_bands)}
    rows = []
    for t in range(num_steps):
        band = t % num_bands
        feats = extract_features(t, scan_history[band], last_scan_t[band], last_hit_t[band])
        label = 1 if grid[t, band] else 0
        rows.append([seed_tag, t, band] + feats + [label])
        scan_history[band].append(label)
        last_scan_t[band] = t
        if label:
            last_hit_t[band] = t
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--raw-dir", default=RAW_DIR)
    parser.add_argument("--out", default=OUT_PATH)
    parser.add_argument("--num-bands", type=int, default=16, help="Must match the receiver's num_bands at train time.")
    parser.add_argument("--freq-min-mhz", type=float, default=0.0)
    parser.add_argument("--freq-max-mhz", type=float, default=18000.0, help="TSRD stare mode covers 0-18 GHz.")
    parser.add_argument("--num-steps", type=int, default=600, help="Time steps to bucket each pulse train into.")
    parser.add_argument("--inspect", action="store_true", help="Print HDF5 structure of the first files and exit.")
    args = parser.parse_args()

    files = sorted(glob.glob(os.path.join(args.raw_dir, "**", "*.h5"), recursive=True))
    if not files:
        raise SystemExit(f"No .h5 files found in {args.raw_dir} -- run download_hf_radar_dataset.py first.")

    if args.inspect:
        for path in files[:3]:
            inspect(path)
        return

    all_rows = []
    for path in files:
        with h5py.File(path, "r") as f:
            toa, freq = find_pdw_arrays(f)
        grid = build_occupancy_grid(toa, freq, args.num_bands, args.freq_min_mhz, args.freq_max_mhz, args.num_steps)
        rows = grid_to_training_rows(grid, seed_tag=os.path.basename(path))
        all_rows.extend(rows)
        print(f"  {os.path.basename(path)}: {toa.size} pulses -> {len(rows)} scan rows "
              f"({grid.sum()} active band-steps)")

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["seed", "t", "band"] + FEATURE_NAMES + ["label"])
        writer.writerows(all_rows)
    print(f"Wrote {len(all_rows)} rows from {len(files)} real pulse-train files -> {args.out}")


if __name__ == "__main__":
    main()
