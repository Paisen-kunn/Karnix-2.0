"""YOLO-based sonar debris detector for real-time object detection in SSS/FLS imagery.

Combines YOLOv8 deep learning object detection with acoustic physics saliency analysis
(highlight backscatter return + acoustic shadow projection) for high-accuracy target
localization (shipwrecks, aircraft, pipes, cylinders, ghost nets).
"""
from __future__ import annotations

import warnings
from pathlib import Path

import cv2
import numpy as np

from .base import BaseDetector, Detection

# Suppress Ultralytics telemetry and warnings
warnings.filterwarnings('ignore')

try:
    from ultralytics import YOLO
    YOLO_AVAILABLE = True
except ImportError:
    YOLO_AVAILABLE = False


class YOLOSonarDetector(BaseDetector):
    """High-accuracy YOLO sonar detector with acoustic physics feature fusion."""

    def __init__(self, model_name: str | None = None, device: str = "cpu"):
        """
        Initialize YOLO detector.

        Args:
            model_name: Path or name of YOLO model.
            device: 'cpu' or 'cuda' or 'mps'
        """
        if not YOLO_AVAILABLE:
            raise ImportError(
                "YOLO not available. Install with: pip install ultralytics"
            )

        # Default to yolov8n.pt unless a valid custom model path is given
        if model_name is None or not Path(model_name).exists():
            model_path = "yolov8n.pt"
        else:
            model_path = str(model_name)

        self.model = YOLO(model_path)
        self.model.to(device)
        self.device = device

    def detect(self, gray_image: np.ndarray, augment: bool = False) -> list[Detection]:
        """
        Run YOLO detection with acoustic saliency extraction.

        Args:
            gray_image: Grayscale sonar image (H x W)
            augment: Enable Test-Time Augmentation

        Returns:
            List of Detection objects
        """
        if len(gray_image.shape) == 3:
            gray_2d = cv2.cvtColor(gray_image, cv2.COLOR_BGR2GRAY)
        else:
            gray_2d = gray_image.copy()

        h, w = gray_2d.shape[:2]

        # 1. Prepare 3-channel sonar-optimized composite image for YOLO
        # Channel 0: CLAHE normalized grayscale
        # Channel 1: High-pass acoustic highlight return
        # Channel 2: Acoustic shadow contrast map
        clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
        norm_gray = clahe.apply(gray_2d)
        
        blur = cv2.GaussianBlur(norm_gray, (15, 15), 0)
        high_pass = cv2.absdiff(norm_gray, blur)
        shadow_map = cv2.bitwise_not(norm_gray)

        composite_bgr = cv2.merge([norm_gray, high_pass, shadow_map])

        # 2. Run YOLO inference on composite and grayscale
        detections: list[Detection] = []
        raw_results = self.model(composite_bgr, verbose=False, conf=0.15, augment=augment)

        if raw_results and len(raw_results) > 0:
            res = raw_results[0]
            if res.boxes is not None:
                for box in res.boxes:
                    x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
                    conf = float(box.conf[0].cpu().numpy())
                    cls_id = int(box.cls[0].cpu().numpy())

                    bx = max(0, int(x1))
                    by = max(0, int(y1))
                    bw = min(w - bx, int(x2 - x1))
                    bh = min(h - by, int(y2 - y1))

                    if bw < 12 or bh < 12:
                        continue

                    class_name = (
                        self.model.names.get(cls_id, f"class_{cls_id}")
                        if hasattr(self.model, "names")
                        else f"class_{cls_id}"
                    )

                    label = self._map_class_label(class_name)
                    ac_features = self._compute_acoustic_features(gray_2d, bx, by, bw, bh)

                    features = {
                        "yolo_class": class_name,
                        "yolo_class_id": cls_id,
                        "model": "yolov8",
                    }
                    features.update(ac_features)

                    detections.append(
                        Detection(
                            x=bx, y=by, w=bw, h=bh,
                            label=label,
                            confidence=conf,
                            features=features,
                        )
                    )

        # 3. Acoustic Salient Structure Extraction
        salient_detections = self._extract_acoustic_salient_targets(gray_2d, norm_gray)

        # 4. Proximity & Structure Clustering (Unifies fragmented hull/wings/mast boxes into ONE complete box)
        all_candidates = detections + salient_detections
        unified_detections = self._cluster_and_unify_structures(all_candidates, gray_2d)

        return unified_detections

    def _extract_acoustic_salient_targets(self, gray: np.ndarray, norm_gray: np.ndarray) -> list[Detection]:
        """
        Extracts salient acoustic targets on the seafloor by identifying bright highlights
        casting dark acoustic shadows behind them.
        """
        h, w = gray.shape[:2]
        img_area = h * w
        salient: list[Detection] = []

        bg_mean = np.mean(gray) or 1.0

        # High-intensity highlight returns
        high_thresh = max(160, int(np.percentile(norm_gray, 92)))
        _, highlight_mask = cv2.threshold(norm_gray, high_thresh, 255, cv2.THRESH_BINARY)

        # Morphological closing to group structure highlights
        kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (7, 7))
        saliency_field = cv2.morphologyEx(highlight_mask, cv2.MORPH_CLOSE, kernel)

        contours, _ = cv2.findContours(saliency_field, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        for cnt in contours:
            area = cv2.contourArea(cnt)
            if area < 120 or area > img_area * 0.35:
                continue

            x, y, bw, bh = cv2.boundingRect(cnt)

            # Skip water-column surface return strip at top of scan
            if y < int(h * 0.08) and bw > int(w * 0.35):
                continue
            # Skip horizontal streak artifacts
            if bw > int(w * 0.55) or (bh > 0 and bw / bh > 6.0 and bw > int(w * 0.30)):
                continue

            ac_features = self._compute_acoustic_features(gray, x, y, bw, bh)
            shadow_ratio = ac_features["shadow_ratio"]
            contrast_ratio = ac_features["contrast_ratio"]
            snr_dB = ac_features["snr_dB"]

            # Must have shadow, contrast, or SNR
            if shadow_ratio < 0.08 and contrast_ratio < 1.20 and snr_dB < 6.0:
                continue

            label = self._classify_structure(bw, bh, area, shadow_ratio)
            conf = min(0.98, 0.40 + 0.35 * shadow_ratio + 0.15 * min(1.0, snr_dB / 15.0) + 0.10 * min(1.0, contrast_ratio / 2.0))

            features = {
                "detector": "acoustic_saliency",
                "area_px": float(area),
                "aspect_ratio": round(bw / float(bh) if bh else 1.0, 3),
            }
            features.update(ac_features)

            salient.append(
                Detection(
                    x=x, y=y, w=bw, h=bh,
                    label=label,
                    confidence=float(conf),
                    features=features,
                )
            )

        return salient

    def _cluster_and_unify_structures(self, candidates: list[Detection], gray: np.ndarray) -> list[Detection]:
        """
        Clusters spatially close or overlapping candidate boxes (e.g. fragments of a large
        shipwreck or aircraft) into a single, unified bounding box.
        """
        if not candidates:
            return []

        img_h, img_w = gray.shape[:2]
        max_merged_area = 0.50 * img_h * img_w

        clusters: list[dict] = []
        for det in candidates:
            clusters.append({
                "x1": det.x,
                "y1": det.y,
                "x2": det.x + det.w,
                "y2": det.y + det.h,
                "conf": det.confidence,
                "label": det.label,
                "features": det.features,
            })

        changed = True
        while changed:
            changed = False
            i = 0
            while i < len(clusters):
                j = i + 1
                while j < len(clusters):
                    c1, c2 = clusters[i], clusters[j]

                    # Potential merged bounding box
                    nx1 = min(c1["x1"], c2["x1"])
                    ny1 = min(c1["y1"], c2["y1"])
                    nx2 = max(c1["x2"], c2["x2"])
                    ny2 = max(c1["y2"], c2["y2"])
                    merged_area = (nx2 - nx1) * (ny2 - ny1)

                    # Distance between two boxes
                    dx = max(0, max(c1["x1"], c2["x1"]) - min(c1["x2"], c2["x2"]))
                    dy = max(0, max(c1["y1"], c2["y1"]) - min(c1["y2"], c2["y2"]))

                    # Merge if close (dx < 30 and dy < 30) AND merged area isn't the entire image
                    if dx < 30 and dy < 30 and merged_area < max_merged_area:
                        c1["x1"], c1["y1"] = nx1, ny1
                        c1["x2"], c1["y2"] = nx2, ny2
                        c1["conf"] = min(0.99, max(c1["conf"], c2["conf"]) + 0.10)
                        
                        if "shipwreck" in c2["label"] or "plane" in c2["label"]:
                            c1["label"] = c2["label"]

                        clusters.pop(j)
                        changed = True
                    else:
                        j += 1
                i += 1

        unified: list[Detection] = []
        for cl in clusters:
            bx, by = cl["x1"], cl["y1"]
            bw, bh = cl["x2"] - cl["x1"], cl["y2"] - cl["y1"]

            ac_features = self._compute_acoustic_features(gray, bx, by, bw, bh)
            label = self._classify_structure(bw, bh, bw * bh, ac_features["shadow_ratio"], cl["label"])

            features = dict(cl["features"])
            features.update(ac_features)

            unified.append(
                Detection(
                    x=bx, y=by, w=bw, h=bh,
                    label=label,
                    confidence=cl["conf"],
                    features=features,
                )
            )

        return unified

    @staticmethod
    def _classify_structure(bw: int, bh: int, area: float, shadow_ratio: float, default_label: str = "shipwreck") -> str:
        """Determines target classification based on structure scale and shape."""
        # Any large target (> 1200 px or bw > 70 & bh > 70) is a Shipwreck / Aircraft structure!
        if area > 1200 or (bw > 70 and bh > 70) or (bw > 120 or bh > 120) or shadow_ratio > 0.35:
            if "plane" in default_label or "aircraft" in default_label:
                return "shipwreck_plane"
            return "shipwreck"

        aspect = bw / float(bh) if bh else 1.0

        # Pipes must be thin and elongated with small overall area
        if (aspect > 3.2 or aspect < 0.31) and area < 1200 and max(bw, bh) < 180:
            return "pipe"

        # Cylinders must be compact
        if 0.65 <= aspect <= 1.5 and area < 1500 and shadow_ratio < 0.25:
            return "cylinder"

        if shadow_ratio > 0.20 and area < 1000:
            return "ghost_net"

        return "shipwreck"

    @staticmethod
    def _compute_acoustic_features(gray: np.ndarray, x: int, y: int, w: int, h: int) -> dict:
        """Calculates multi-directional acoustic backscatter, shadow ratio, and signal-to-noise ratio."""
        img_h, img_w = gray.shape[:2]
        x0, y0 = max(0, x), max(0, y)
        x1, y1 = min(img_w, x + w), min(img_h, y + h)

        patch = gray[y0:y1, x0:x1]
        if patch.size == 0:
            return {"shadow_ratio": 0.0, "contrast_ratio": 1.0, "snr_dB": 0.0}

        target_mean = float(np.mean(patch))
        target_max = float(np.max(patch))

        # Local background region around patch
        bg_x0, bg_y0 = max(0, x - w), max(0, y - h)
        bg_x1, bg_y1 = min(img_w, x + 2 * w), min(img_h, y + 2 * h)
        bg_patch = gray[bg_y0:bg_y1, bg_x0:bg_x1]
        bg_mean = float(np.mean(bg_patch)) if bg_patch.size > 0 else 1.0
        bg_std = float(np.std(bg_patch)) if bg_patch.size > 0 and np.std(bg_patch) > 1e-5 else 1.0

        bg_thresh = 0.50 * bg_mean
        shadow_ratios = []

        # 1. Shadow to the Right (Starboard transducer shadow)
        rx0, rx1 = x1, min(img_w, x1 + max(w, 80))
        if rx1 > rx0:
            r_patch = gray[y0:y1, rx0:rx1]
            if r_patch.size > 0:
                shadow_ratios.append(float(np.sum(r_patch < bg_thresh)) / r_patch.size)

        # 2. Shadow Below (Waterfall track shadow)
        sy0, sy1 = y1, min(img_h, y1 + max(h, 80))
        if sy1 > sy0:
            b_patch = gray[sy0:sy1, x0:x1]
            if b_patch.size > 0:
                shadow_ratios.append(float(np.sum(b_patch < bg_thresh)) / b_patch.size)

        # 3. Shadow to the Left (Port transducer shadow)
        lx0, lx1 = max(0, x0 - max(w, 80)), x0
        if lx1 > lx0:
            l_patch = gray[y0:y1, lx0:lx1]
            if l_patch.size > 0:
                shadow_ratios.append(float(np.sum(l_patch < bg_thresh)) / l_patch.size)

        shadow_ratio = max(shadow_ratios) if shadow_ratios else 0.0

        contrast_ratio = target_mean / (bg_mean + 1e-5)
        snr_dB = 20.0 * np.log10((target_max - bg_mean) / bg_std + 1.0) if target_max > bg_mean else 0.0

        return {
            "shadow_ratio": round(shadow_ratio, 3),
            "contrast_ratio": round(contrast_ratio, 3),
            "snr_dB": round(float(snr_dB), 2),
            "peak_intensity": int(target_max),
        }

    @staticmethod
    def _map_class_label(yolo_class: str) -> str:
        """Map YOLO class names to sonar target types."""
        c = yolo_class.lower()
        if "plane" in c or "airplane" in c or "aircraft" in c or "jet" in c:
            return "shipwreck_plane"
        if "net" in c or "ghost" in c:
            return "ghost_net"
        if "wreck" in c or "ship" in c or "boat" in c or "vessel" in c:
            return "shipwreck"
        if "pipe" in c or "tube" in c or "cable" in c:
            return "pipe"
        if "cylinder" in c or "drum" in c or "container" in c or "tank" in c:
            return "cylinder"
        return "debris"
