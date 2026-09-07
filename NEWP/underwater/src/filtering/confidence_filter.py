"""False-positive suppression and confidence scoring for candidate detections.

Natural clutter (rock clusters, sand ripples) tends to produce irregular, shadow-less,
or implausibly-shaped blobs. This module re-weights/suppresses those before reporting.
"""
from __future__ import annotations

from ..detection.base import Detection

DEFAULT_MIN_CONFIDENCE = 0.35


def _iou(a: Detection, b: Detection) -> float:
    ax2, ay2 = a.x + a.w, a.y + a.h
    bx2, by2 = b.x + b.w, b.y + b.h
    ix1, iy1 = max(a.x, b.x), max(a.y, b.y)
    ix2, iy2 = min(ax2, bx2), min(ay2, by2)
    iw, ih = max(0, ix2 - ix1), max(0, iy2 - iy1)
    inter = iw * ih
    if inter == 0:
        return 0.0
    union = a.w * a.h + b.w * b.h - inter
    return inter / union if union else 0.0


def _non_max_suppress(detections: list[Detection], iou_threshold: float = 0.4) -> list[Detection]:
    ordered = sorted(detections, key=lambda d: d.confidence, reverse=True)
    kept: list[Detection] = []
    for det in ordered:
        if all(_iou(det, k) < iou_threshold for k in kept):
            kept.append(det)
    return kept


def filter_detections(
    detections: list[Detection],
    min_confidence: float = DEFAULT_MIN_CONFIDENCE,
    image_shape: tuple[int, int] | None = None,
) -> list[Detection]:
    """Suppress natural-clutter false positives, water-column noise, and duplicate/overlapping boxes.

    Rules applied (in order):
      1. Reject boxes below minimum confidence score threshold.
      2. Reject top water-column surface return noise and horizontal streaks.
      3. For classical/saliency detections: reject boxes with no acoustic shadow AND low fill ratio.
      4. Non-max suppression to collapse duplicate/overlapping boxes onto the best one.
    """
    survivors = []
    img_h, img_w = image_shape if image_shape else (None, None)

    for det in detections:
        if det.confidence < min_confidence:
            continue

        if img_h and img_w:
            # 1. Suppress water-column surface return strip at top of sonar scan
            if det.y < int(img_h * 0.10) and det.w > int(img_w * 0.35):
                continue

            # 2. Suppress horizontal dropout streaks spanning > 50% image width
            if det.w > int(img_w * 0.50):
                continue

            # 3. Suppress massive background boxes (> 25% total image area)
            if (det.w * det.h) > int(0.25 * img_h * img_w):
                continue

        # Physical acoustic clutter suppression
        shadow_ratio = det.features.get("shadow_ratio")
        fill_ratio = det.features.get("fill_ratio")
        contrast_ratio = det.features.get("contrast_ratio")

        if shadow_ratio is not None and fill_ratio is not None:
            if shadow_ratio < 0.04 and fill_ratio < 0.30 and (contrast_ratio is None or contrast_ratio < 1.20):
                continue  # looks like flat natural seafloor texture, not an object

        survivors.append(det)

    return _non_max_suppress(survivors, iou_threshold=0.25)


def confidence_percent(detections: list[Detection]) -> list[dict]:
    """Return detection dicts with confidence expressed as a 0-100% score for reporting."""
    out = []
    for det in detections:
        d = det.to_dict()
        d["confidence_percent"] = round(det.confidence * 100, 1)
        out.append(d)
    return out
