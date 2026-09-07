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

        # 3. Acoustic Salient Target Extraction (Finds prominent aircraft, shipwrecks, pipes, cylinders)
        salient_detections = self._extract_acoustic_salient_targets(gray_2d, norm_gray)
        
        # Merge salient detections with YOLO detections
        all_detections = self._merge_proposals(detections, salient_detections)

        return all_detections

    def _extract_acoustic_salient_targets(self, gray: np.ndarray, norm_gray: np.ndarray) -> list[Detection]:
        """
        Extracts salient acoustic targets on the seafloor by identifying bright highlights
        casting dark acoustic shadows behind them.
        """
        h, w = gray.shape[:2]
        img_area = h * w
        salient: list[Detection] = []

        # Threshold prominent acoustic returns (top 5% intensity)
        high_thresh = max(180, int(np.percentile(norm_gray, 95)))
        _, highlight_mask = cv2.threshold(norm_gray, high_thresh, 255, cv2.THRESH_BINARY)

        # Morphological grouping to merge structure fragments (e.g. wings + fuselage)
        kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (7, 7))
        highlight_mask = cv2.morphologyEx(highlight_mask, cv2.MORPH_CLOSE, kernel)

        contours, _ = cv2.findContours(highlight_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        for cnt in contours:
            area = cv2.contourArea(cnt)
            # Minimum target area threshold (350 px) to exclude tiny rock clutter
            if area < 350 or area > img_area * 0.20:
                continue

            x, y, bw, bh = cv2.boundingRect(cnt)

            # Skip water-column surface return strip at top of scan
            if y < int(h * 0.08) and bw > int(w * 0.30):
                continue
            # Skip horizontal streak artifacts
            if bw > int(w * 0.45) or (bh > 0 and bw / bh > 5.0 and bw > int(w * 0.25)):
                continue

            ac_features = self._compute_acoustic_features(gray, x, y, bw, bh)
            shadow_ratio = ac_features["shadow_ratio"]
            contrast_ratio = ac_features["contrast_ratio"]
            snr_dB = ac_features["snr_dB"]

            # Must cast a real acoustic shadow or have high SNR/contrast
            if shadow_ratio < 0.10 and contrast_ratio < 1.30 and snr_dB < 7.0:
                continue

            aspect = bw / float(bh) if bh else 1.0
            fill = area / float(bw * bh) if bw * bh else 0.5

            # Classify target structure
            if (aspect > 1.2 or aspect < 0.8) and area > 1000 and shadow_ratio > 0.12:
                label = "shipwreck_plane"  # Aircraft / large structure
            elif aspect > 3.0 or aspect < 0.33:
                label = "pipe"
            elif 0.7 <= aspect <= 1.4 and fill > 0.55:
                label = "cylinder"
            elif shadow_ratio > 0.25 and fill < 0.45:
                label = "ghost_net"
            else:
                label = "shipwreck"

            # Compute physical confidence score
            conf = min(0.98, 0.35 + 0.35 * shadow_ratio + 0.15 * min(1.0, snr_dB / 15.0) + 0.15 * min(1.0, contrast_ratio / 2.0))

            features = {
                "detector": "acoustic_saliency",
                "area_px": float(area),
                "aspect_ratio": round(aspect, 3),
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

    def _merge_proposals(self, yolo_dets: list[Detection], salient_dets: list[Detection]) -> list[Detection]:
        """Merges YOLO proposals with acoustic salient proposals."""
        merged = list(yolo_dets)
        
        for s_det in salient_dets:
            matched = False
            for i, y_det in enumerate(merged):
                # Calculate IoU
                ix1, iy1 = max(s_det.x, y_det.x), max(s_det.y, y_det.y)
                ix2, iy2 = min(s_det.x + s_det.w, y_det.x + y_det.w), min(s_det.y + s_det.h, y_det.y + y_det.h)
                iw, ih = max(0, ix2 - ix1), max(0, iy2 - iy1)
                inter = iw * ih
                union = s_det.w * s_det.h + y_det.w * y_det.h - inter
                iou = inter / float(union) if union > 0 else 0.0

                if iou > 0.15:
                    # Enclose both boxes cleanly
                    nx = min(s_det.x, y_det.x)
                    ny = min(s_det.y, y_det.y)
                    nw = max(s_det.x + s_det.w, y_det.x + y_det.w) - nx
                    nh = max(s_det.y + s_det.h, y_det.y + y_det.h) - ny
                    
                    # Boost confidence when both detectors agree
                    boosted_conf = min(0.99, max(s_det.confidence, y_det.confidence) + 0.15)
                    best_label = s_det.label if "plane" in s_det.label or y_det.label == "debris" else y_det.label

                    features = dict(y_det.features)
                    features.update(s_det.features)
                    features["fusion_agreed"] = True

                    merged[i] = Detection(
                        x=nx, y=ny, w=nw, h=nh,
                        label=best_label,
                        confidence=boosted_conf,
                        features=features,
                    )
                    matched = True
                    break

            if not matched:
                merged.append(s_det)

        return merged

    @staticmethod
    def _compute_acoustic_features(gray: np.ndarray, x: int, y: int, w: int, h: int) -> dict:
        """Calculates local acoustic backscatter, shadow ratio, and signal-to-noise ratio."""
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

        # Acoustic shadow patch directly behind object
        sy0, sy1 = y1, min(img_h, y1 + int(h * 1.2))
        shadow_patch = gray[sy0:sy1, x0:x1]
        shadow_ratio = 0.0
        if shadow_patch.size > 0:
            dark_pixels = np.sum(shadow_patch < 0.5 * bg_mean)
            shadow_ratio = float(dark_pixels) / shadow_patch.size

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
