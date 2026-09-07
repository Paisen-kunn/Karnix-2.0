"""Shared detection contract used by every detector backend."""
from __future__ import annotations

from dataclasses import dataclass, field


@dataclass
class Detection:
    """A single candidate anomaly found in a sonar image."""

    x: int
    y: int
    w: int
    h: int
    label: str = "unknown_object"
    confidence: float = 0.0
    features: dict = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {
            "bbox_px": {"x": self.x, "y": self.y, "w": self.w, "h": self.h},
            "label": self.label,
            "confidence": round(self.confidence, 4),
            "features": self.features,
        }


class BaseDetector:
    """Interface every detection backend (classical CV or learned model) must implement."""

    def detect(self, gray_image) -> list[Detection]:
        raise NotImplementedError
