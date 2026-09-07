"""End-to-end pipeline: preprocess -> detect -> filter -> geotag/report -> overlay image.

Usable both as a CLI (`python3 -m src.pipeline <image> [--metadata meta.json]`) and as a
library import from the dashboard API.

Supports both classical CV and YOLO detection backends.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import cv2
import numpy as np

from .detection.classical_detector import ClassicalSonarDetector
from .filtering.confidence_filter import filter_detections
from .preprocessing.sonar_preprocessing import preprocess
from .reporting.report_generator import build_report, to_csv, to_json

# Attempt to import YOLO detector
try:
    from .detection.yolo_detector import YOLOSonarDetector
    YOLO_AVAILABLE = True
except ImportError:
    YOLO_AVAILABLE = False

COLOR_MAP = {
    "shipwreck_plane": (255, 229, 0),  # Cyan / Yellow
    "shipwreck": (102, 255, 0),        # Bright Green
    "pipe": (0, 153, 255),             # Bright Orange
    "cylinder": (0, 204, 255),         # Bright Yellow
    "ghost_net": (204, 0, 255),        # Magenta
    "debris": (51, 51, 255),           # Bright Red
}

LABEL_DISPLAY_MAP = {
    "shipwreck_plane": "✈️ Aircraft Wreck",
    "shipwreck": "🚢 Shipwreck Target",
    "pipe": "⛽ Underwater Pipe",
    "cylinder": "🛢️ Cylinder / Container",
    "ghost_net": "🕸️ Ghost Net / Rope",
    "debris": "🔍 Marine Debris Anomaly",
}


def draw_overlay(original_bgr: np.ndarray, records: list[dict]) -> np.ndarray:
    """Draw tactical, high-contrast bounding boxes with filled text labels."""
    overlay = original_bgr.copy()
    h, w = overlay.shape[:2]
    
    # Scale box border and text size dynamically based on image resolution
    thickness = max(2, int(min(h, w) / 300))
    font_scale = max(0.45, min(h, w) / 1000.0)

    for r in records:
        b = r["bbox_px"]
        x, y, bw, bh = b["x"], b["y"], b["w"], b["h"]
        raw_label = r.get("label", "debris")
        color = COLOR_MAP.get(raw_label, (0, 255, 255))
        display_name = LABEL_DISPLAY_MAP.get(raw_label, raw_label.replace("_", " ").title())
        conf_pct = r.get("confidence_percent", r.get("confidence", 0) * 100)

        # 1. Main Bounding Box
        cv2.rectangle(overlay, (x, y), (x + bw, y + bh), color, thickness)

        # 2. Corner Crosshairs
        corner_len = max(6, int(min(bw, bh) * 0.2))
        cv2.line(overlay, (x, y), (x + corner_len, y), color, thickness + 1)
        cv2.line(overlay, (x, y), (x, y + corner_len), color, thickness + 1)
        cv2.line(overlay, (x + bw, y), (x + bw - corner_len, y), color, thickness + 1)
        cv2.line(overlay, (x + bw, y), (x + bw, y + corner_len), color, thickness + 1)
        cv2.line(overlay, (x, y + bh), (x + corner_len, y + bh), color, thickness + 1)
        cv2.line(overlay, (x, y + bh), (x, y + bh - corner_len), color, thickness + 1)
        cv2.line(overlay, (x + bw, y + bh), (x + bw - corner_len, y + bh), color, thickness + 1)
        cv2.line(overlay, (x + bw, y + bh), (x + bw, y + bh - corner_len), color, thickness + 1)

        # 3. Label Text Badge
        text_str = f"{display_name} ({conf_pct:.0f}%)"
        (tw, th), baseline = cv2.getTextSize(text_str, cv2.FONT_HERSHEY_SIMPLEX, font_scale, 1)

        # Badge position (above box, or inside top if near top border)
        by1 = max(0, y - th - 8)
        by2 = y if y >= th + 8 else y + th + 8
        ty = y - 4 if y >= th + 8 else y + th + 4

        # Filled badge background
        cv2.rectangle(overlay, (x, by1), (x + tw + 10, by2), (20, 20, 20), -1)
        cv2.rectangle(overlay, (x, by1), (x + tw + 10, by2), color, 1)

        # White text inside badge
        cv2.putText(overlay, text_str, (x + 5, ty), cv2.FONT_HERSHEY_SIMPLEX, font_scale, (255, 255, 255), 1, cv2.LINE_AA)

    return overlay


def run_pipeline(
    image_bgr: np.ndarray,
    metadata: list[dict] | None = None,
    range_m: float = 30.0,
    min_confidence: float = 0.35,
    source_image: str = "unknown.png",
    detector: str = "yolo",
) -> dict:
    """
    Run the sonar anomaly detection pipeline.

    Args:
        image_bgr: Input BGR image
        metadata: Optional metadata dict
        range_m: Sonar range in meters
        min_confidence: Minimum confidence threshold
        source_image: Source image filename
        detector: 'yolo' or 'classical'

    Returns:
        Dict with records, overlay_bgr, raw_count, kept_count
    """
    gray = preprocess(image_bgr)

    # Select detector
    if detector.lower() == "yolo":
        if not YOLO_AVAILABLE:
            print("Warning: YOLO not available, falling back to classical detector", file=sys.stderr)
            print("Install with: pip install ultralytics", file=sys.stderr)
            raw_detections = ClassicalSonarDetector().detect(gray)
        else:
            try:
                raw_detections = YOLOSonarDetector().detect(gray)
            except Exception as e:
                print(f"Warning: YOLO detection failed ({e}), falling back to classical", file=sys.stderr)
                raw_detections = ClassicalSonarDetector().detect(gray)
    else:
        # Use classical detector
        raw_detections = ClassicalSonarDetector().detect(gray)

    kept_detections = filter_detections(raw_detections, min_confidence=min_confidence, image_shape=gray.shape[:2])
    records = build_report(
        kept_detections, metadata, gray.shape, range_m=range_m, source_image=source_image,
    )
    overlay_bgr = draw_overlay(image_bgr if image_bgr.ndim == 3 else cv2.cvtColor(image_bgr, cv2.COLOR_GRAY2BGR), records)
    return {
        "records": records,
        "overlay_bgr": overlay_bgr,
        "raw_count": len(raw_detections),
        "kept_count": len(kept_detections),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="AquaSentry sonar anomaly detection pipeline")
    parser.add_argument("image", help="Path to side-scan sonar image")
    parser.add_argument("--metadata", help="Path to ping metadata JSON", default=None)
    parser.add_argument("--range-m", type=float, default=30.0, help="Sonar range (m) per side of swath")
    parser.add_argument("--min-confidence", type=float, default=0.35)
    parser.add_argument("--detector", choices=["yolo", "classical"], default="yolo", help="Detection backend")
    parser.add_argument("--out-dir", default="output")
    args = parser.parse_args()

    image_path = Path(args.image)
    image_bgr = cv2.imread(str(image_path))
    if image_bgr is None:
        print(f"Could not read image: {image_path}", file=sys.stderr)
        return 1

    metadata = None
    if args.metadata:
        metadata = json.loads(Path(args.metadata).read_text())

    result = run_pipeline(
        image_bgr, metadata, range_m=args.range_m,
        min_confidence=args.min_confidence, source_image=image_path.name,
        detector=args.detector,
    )

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "report.json").write_text(to_json(result["records"]))
    (out_dir / "report.csv").write_text(to_csv(result["records"]))
    cv2.imwrite(str(out_dir / "overlay.png"), result["overlay_bgr"])

    print(f"Detections: {result['raw_count']} raw -> {result['kept_count']} after filtering")
    print(f"Report written to {out_dir}/report.json and {out_dir}/report.csv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
