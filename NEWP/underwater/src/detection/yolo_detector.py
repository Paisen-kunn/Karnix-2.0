"""YOLO-based sonar debris detector for real-time object detection.

Integrates YOLOv8 (Ultralytics) for efficient marine debris detection.
Supports both CPU and GPU inference, with configurable confidence thresholds.
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
    """YOLO detector for marine debris in side-scan sonar imagery."""

    def __init__(self, model_name: str | None = None, device: str = "cpu"):
        """
        Initialize YOLO detector.

        Args:
            model_name: Path or name of YOLO model. If None, checks for trained model weights
                       at 'models/best_sonar_yolo.pt' before falling back to 'yolov8n.pt'.
            device: 'cpu' or 'cuda' (auto-selects available GPU)
        """
        if not YOLO_AVAILABLE:
            raise ImportError(
                "YOLO not available. Install with: pip install ultralytics"
            )

        if model_name is None:
            # Check for custom trained model in workspace
            project_root = Path(__file__).resolve().parent.parent.parent
            custom_best = project_root / "models" / "best_sonar_yolo.pt"
            if custom_best.exists():
                model_path = str(custom_best)
            else:
                model_path = "yolov8n.pt"
        else:
            model_path = model_name if model_name.endswith(".pt") else f"{model_name}.pt"

        self.model = YOLO(model_path)
        self.model.to(device)
        self.device = device
        self.sonar_classes = {
            "ghost_net": 0,
            "shipwreck": 1,
            "pipe": 2,
            "cylinder": 3,
            "debris": 4,
        }

    def detect(self, gray_image: np.ndarray, augment: bool = False) -> list[Detection]:
        """
        Run YOLO inference on sonar image with acoustic feature extraction.

        Args:
            gray_image: Grayscale sonar image (H x W x 1 or H x W)
            augment: Enable Test-Time Augmentation (TTA) for higher detection accuracy

        Returns:
            List of Detection objects with YOLO bounding boxes, acoustic features, and confidence scores
        """
        # Convert grayscale to BGR for YOLO (expects 3-channel input)
        if len(gray_image.shape) == 2:
            bgr_image = cv2.cvtColor(gray_image, cv2.COLOR_GRAY2BGR)
            gray_2d = gray_image
        else:
            bgr_image = gray_image
            gray_2d = cv2.cvtColor(gray_image, cv2.COLOR_BGR2GRAY)

        # Run YOLO inference with optimal confidence threshold
        results = self.model(bgr_image, verbose=False, conf=0.20, augment=augment)
        detections: list[Detection] = []

        if results and len(results) > 0:
            result = results[0]
            if result.boxes is not None:
                for box in result.boxes:
                    x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
                    conf = float(box.conf[0].cpu().numpy())
                    cls_id = int(box.cls[0].cpu().numpy())

                    # Convert to x, y, w, h format
                    x, y = int(x1), int(y1)
                    w, h = int(x2 - x1), int(y2 - y1)

                    # Calculate local sonar acoustic physics metrics
                    ac_features = self._compute_acoustic_features(gray_2d, x, y, w, h)

                    # Get class name
                    class_name = (
                        self.model.names.get(cls_id, f"class_{cls_id}")
                        if hasattr(self.model, "names")
                        else f"class_{cls_id}"
                    )

                    # Map to sonar debris types
                    label = self._map_class_label(class_name)

                    features = {
                        "yolo_class": class_name,
                        "yolo_class_id": cls_id,
                        "model": "yolov8",
                    }
                    features.update(ac_features)

                    detections.append(
                        Detection(
                            x=x,
                            y=y,
                            w=w,
                            h=h,
                            label=label,
                            confidence=conf,
                            features=features,
                        )
                    )

        return detections

    @staticmethod
    def _compute_acoustic_features(gray: np.ndarray, x: int, y: int, w: int, h: int) -> dict:
        """Calculates local acoustic backscatter, shadow ratio, and signal-to-noise ratio."""
        img_h, img_w = gray.shape[:2]
        x0, y0 = max(0, x), max(0, y)
        x1, y1 = min(img_w, x + w), min(img_h, y + h)

        patch = gray[y0:y1, x0:x1]
        if patch.size == 0:
            return {"shadow_ratio": 0.5, "contrast_ratio": 1.0, "snr_dB": 0.0}

        target_mean = float(np.mean(patch))
        target_max = float(np.max(patch))

        # Local background region
        bg_x0, bg_y0 = max(0, x - w), max(0, y - h)
        bg_x1, bg_y1 = min(img_w, x + 2 * w), min(img_h, y + 2 * h)
        bg_patch = gray[bg_y0:bg_y1, bg_x0:bg_x1]
        bg_mean = float(np.mean(bg_patch)) if bg_patch.size > 0 else 1.0
        bg_std = float(np.std(bg_patch)) if bg_patch.size > 0 and np.std(bg_patch) > 1e-5 else 1.0

        # Acoustic shadow patch directly behind object
        sy0, sy1 = y1, min(img_h, y1 + h)
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
        """Map YOLO class names to sonar debris types."""
        class_lower = yolo_class.lower()
        if "net" in class_lower or "ghost" in class_lower:
            return "ghost_net"
        if "wreck" in class_lower or "ship" in class_lower:
            return "shipwreck"
        if "pipe" in class_lower or "tube" in class_lower:
            return "pipe"
        if "cylinder" in class_lower or "drum" in class_lower:
            return "cylinder"
        if "rope" in class_lower or "cable" in class_lower:
            return "entangled_debris_net"
        return "debris"
