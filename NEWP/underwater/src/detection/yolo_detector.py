"""YOLO-based sonar debris detector for real-time object detection.

Integrates YOLOv8 (Ultralytics) for efficient marine debris detection.
Supports both CPU and GPU inference, with configurable confidence thresholds.
"""
from __future__ import annotations

import warnings

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

    def __init__(self, model_name: str = "yolov8n.pt", device: str = "cpu"):
        """
        Initialize YOLO detector.

        Args:
            model_name: YOLOv8 model variant ('yolov8n', 'yolov8s', 'yolov8m', etc.)
            device: 'cpu' or 'cuda' (auto-selects available GPU)
        """
        if not YOLO_AVAILABLE:
            raise ImportError(
                "YOLO not available. Install with: pip install ultralytics"
            )
        self.model = YOLO(f"{model_name}.pt")
        self.model.to(device)
        self.device = device
        self.sonar_classes = {
            "ghost_net": 0,
            "shipwreck": 1,
            "pipe": 2,
            "cylinder": 3,
            "debris": 4,
        }

    def detect(self, gray_image: np.ndarray) -> list[Detection]:
        """
        Run YOLO inference on sonar image.

        Args:
            gray_image: Grayscale sonar image (H x W x 1 or H x W)

        Returns:
            List of Detection objects with YOLO bounding boxes and confidence scores
        """
        # Convert grayscale to BGR for YOLO (expects 3-channel input)
        if len(gray_image.shape) == 2:
            bgr_image = cv2.cvtColor(gray_image, cv2.COLOR_GRAY2BGR)
        else:
            bgr_image = gray_image

        # Run YOLO inference
        results = self.model(bgr_image, verbose=False, conf=0.25)
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

                    # Get class name
                    class_name = (
                        self.model.names.get(cls_id, f"class_{cls_id}")
                        if hasattr(self.model, "names")
                        else f"class_{cls_id}"
                    )

                    # Map to sonar debris types
                    label = self._map_class_label(class_name)

                    detections.append(
                        Detection(
                            x=x,
                            y=y,
                            w=w,
                            h=h,
                            label=label,
                            confidence=conf,
                            features={
                                "yolo_class": class_name,
                                "yolo_class_id": cls_id,
                                "model": "yolov8",
                            },
                        )
                    )

        return detections

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
