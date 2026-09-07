"""Classical CV detector: works with zero training data.

Combines adaptive thresholding (to isolate high-backscatter returns from debris) with
acoustic-shadow analysis (man-made objects on a flat seafloor cast a distinctive dark
shadow behind them) and contour geometry to flag candidate anomalies.
"""
from __future__ import annotations

import cv2
import numpy as np

from .base import BaseDetector, Detection

MIN_AREA_PX = 40
MAX_AREA_FRACTION = 0.25  # reject blobs that are implausibly large (likely noise band)


class ClassicalSonarDetector(BaseDetector):
    def __init__(self, block_size: int = 35, c: int = -7):
        # block_size/C tuned for typical 8-bit SSS waterfall imagery
        self.block_size = block_size if block_size % 2 == 1 else block_size + 1
        self.c = c

    def detect(self, gray_image: np.ndarray) -> list[Detection]:
        h, w = gray_image.shape[:2]
        image_area = h * w

        thresh = cv2.adaptiveThreshold(
            gray_image, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
            cv2.THRESH_BINARY, self.block_size, self.c,
        )
        thresh = cv2.morphologyEx(thresh, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
        thresh = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))

        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        detections: list[Detection] = []
        for contour in contours:
            area = cv2.contourArea(contour)
            if area < MIN_AREA_PX or area > image_area * MAX_AREA_FRACTION:
                continue

            x, y, bw, bh = cv2.boundingRect(contour)
            aspect_ratio = bw / bh if bh else 0.0
            fill_ratio = area / (bw * bh) if bw * bh else 0.0

            shadow_ratio = self._shadow_ratio(gray_image, x, y, bw, bh)
            label = self._classify(aspect_ratio, fill_ratio, shadow_ratio)
            confidence = self._score(area, image_area, fill_ratio, shadow_ratio)

            detections.append(
                Detection(
                    x=x, y=y, w=bw, h=bh,
                    label=label,
                    confidence=confidence,
                    features={
                        "area_px": float(area),
                        "aspect_ratio": round(aspect_ratio, 3),
                        "fill_ratio": round(fill_ratio, 3),
                        "shadow_ratio": round(shadow_ratio, 3),
                    },
                )
            )
        return detections

    @staticmethod
    def _shadow_ratio(gray: np.ndarray, x: int, y: int, w: int, h: int) -> float:
        """Fraction of dark (shadow) pixels immediately behind (below) the return.

        A strong shadow is the key discriminator between a proud man-made object and a
        flat natural texture patch of similar brightness.
        """
        img_h = gray.shape[0]
        shadow_y0 = min(y + h, img_h)
        shadow_y1 = min(y + h + h, img_h)
        if shadow_y1 <= shadow_y0:
            return 0.0
        shadow_patch = gray[shadow_y0:shadow_y1, x:x + w]
        if shadow_patch.size == 0:
            return 0.0
        local_mean = gray[max(0, y - h):min(img_h, y + h), x:x + w].mean() or 1.0
        dark_pixels = np.sum(shadow_patch < 0.5 * local_mean)
        return float(dark_pixels) / shadow_patch.size

    @staticmethod
    def _classify(aspect_ratio: float, fill_ratio: float, shadow_ratio: float) -> str:
        if aspect_ratio > 3.5 or aspect_ratio < 0.28:
            return "elongated_object_pipe_or_net"
        if fill_ratio < 0.45 and shadow_ratio > 0.3:
            return "entangled_debris_net"
        if 0.7 <= aspect_ratio <= 1.4 and fill_ratio > 0.6:
            return "compact_object_cylinder_or_wreck_part"
        return "unclassified_anomaly"

    @staticmethod
    def _score(area: float, image_area: int, fill_ratio: float, shadow_ratio: float) -> float:
        """Heuristic 0-1 confidence: rewards a well-defined shape plus a real acoustic shadow."""
        size_score = min(1.0, area / (image_area * 0.01))
        shape_score = min(1.0, fill_ratio + 0.2)
        shadow_score = min(1.0, shadow_ratio * 1.5)
        return float(np.clip(0.25 * size_score + 0.35 * shape_score + 0.40 * shadow_score, 0.0, 1.0))
