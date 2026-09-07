"""Preprocessing for side-scan sonar (SSS) imagery.

Handles the acoustic-imagery-specific challenges called out in the problem statement:
speckle noise, varying pixel resolution, and motion-induced (heave/pitch/roll) row
dropouts/streaks.
"""
from __future__ import annotations

import cv2
import numpy as np


def load_grayscale(image: np.ndarray) -> np.ndarray:
    """Normalize an input image (any channel count) to single-channel uint8 grayscale."""
    if image.ndim == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    if image.dtype != np.uint8:
        image = cv2.normalize(image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
    return image


def despeckle(gray: np.ndarray) -> np.ndarray:
    """Reduce multiplicative speckle noise typical of acoustic backscatter.

    A median filter removes salt-and-pepper speckle while a small-window bilateral
    filter preserves debris edges (needed for accurate bounding boxes later).
    """
    median = cv2.medianBlur(gray, 5)
    return cv2.bilateralFilter(median, d=7, sigmaColor=50, sigmaSpace=50)


def correct_motion_dropouts(gray: np.ndarray) -> np.ndarray:
    """Repair row-wise dropout/streaking artifacts caused by AUV heave/pitch/roll.

    Rows whose mean intensity deviates sharply from their neighbors (dead pings /
    saturation) are replaced by an interpolation of the surrounding rows.
    """
    row_means = gray.mean(axis=1)
    if len(row_means) < 3:
        return gray
    diffs = np.abs(np.diff(row_means))
    threshold = diffs.mean() + 3 * diffs.std() if diffs.std() > 0 else np.inf
    bad_rows = np.where(np.abs(row_means - np.median(row_means)) > threshold)[0]

    if len(bad_rows) == 0:
        return gray

    fixed = gray.copy()
    good_rows = np.setdiff1d(np.arange(gray.shape[0]), bad_rows)
    if len(good_rows) < 2:
        return gray
    for row in bad_rows:
        prev_candidates = good_rows[good_rows < row]
        next_candidates = good_rows[good_rows > row]
        prev_row = prev_candidates[-1] if len(prev_candidates) else next_candidates[0]
        next_row = next_candidates[0] if len(next_candidates) else prev_candidates[-1]
        fixed[row] = ((gray[prev_row].astype(np.uint16) + gray[next_row].astype(np.uint16)) // 2).astype(np.uint8)
    return fixed


def normalize_range(gray: np.ndarray) -> np.ndarray:
    """Compensate for across-track intensity falloff (near-range vs far-range) via CLAHE."""
    clahe = cv2.createCLAHE(clipLimit=2.5, tileGridSize=(8, 8))
    return clahe.apply(gray)


def preprocess(image: np.ndarray) -> np.ndarray:
    """Full preprocessing chain: grayscale -> despeckle -> motion repair -> range normalize."""
    gray = load_grayscale(image)
    gray = correct_motion_dropouts(gray)
    gray = despeckle(gray)
    gray = normalize_range(gray)
    return gray
