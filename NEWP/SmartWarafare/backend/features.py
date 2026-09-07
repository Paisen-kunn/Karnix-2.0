"""Shared feature engineering for the learned scan-priority model
(`MLPredictorScheduler` in scheduler.py).

Features are built *only* from information a real receiver could actually
observe: the outcomes of its own past scans of a given band, plus elapsed
wall-clock time (so the model can pick up on periodic hop patterns). They
never depend on ground-truth activity of bands that weren't scanned.
"""
from typing import List, Optional

FEATURE_NAMES = [
    "scans_so_far",
    "hit_rate",
    "recent_hit_rate",
    "last_scan_hit",
    "steps_since_last_scan",
    "steps_since_last_hit",
    "t_mod2", "t_mod3", "t_mod4", "t_mod5", "t_mod8",
]

MAX_GAP = 50  # cap for "steps since" features so the model never sees unbounded values


def extract_features(
    t: int,
    scan_history: List[int],
    last_scan_t: Optional[int],
    last_hit_t: Optional[int],
) -> List[float]:
    """Build a feature vector for one band at time `t`.

    scan_history: 1/0 detector outcomes of this band's past scans, oldest first.
    last_scan_t / last_hit_t: time index of this band's most recent scan /
        most recent positive detection (None if it has never happened).
    """
    scans_so_far = len(scan_history)
    hit_rate = sum(scan_history) / scans_so_far if scans_so_far else 0.5
    recent = scan_history[-5:]
    recent_hit_rate = sum(recent) / len(recent) if recent else 0.5
    last_scan_hit = float(scan_history[-1]) if scan_history else 0.5
    steps_since_last_scan = min(t - last_scan_t, MAX_GAP) if last_scan_t is not None else MAX_GAP
    steps_since_last_hit = min(t - last_hit_t, MAX_GAP) if last_hit_t is not None else MAX_GAP
    return [
        float(scans_so_far),
        hit_rate,
        recent_hit_rate,
        last_scan_hit,
        steps_since_last_scan / MAX_GAP,
        steps_since_last_hit / MAX_GAP,
        (t % 2) / 2.0,
        (t % 3) / 3.0,
        (t % 4) / 4.0,
        (t % 5) / 5.0,
        (t % 8) / 8.0,
    ]
