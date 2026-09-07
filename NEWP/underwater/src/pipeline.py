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

BOX_COLOR = (0, 0, 255)  # red, BGR
TEXT_COLOR = (0, 255, 255)


def draw_overlay(original_bgr: np.ndarray, records: list[dict]) -> np.ndarray:
    overlay = original_bgr.copy()
    for r in records:
        b = r["bbox_px"]
        x, y, w, h = b["x"], b["y"], b["w"], b["h"]
        cv2.rectangle(overlay, (x, y), (x + w, y + h), BOX_COLOR, 2)
        label = f"{r['label']} {r['confidence_percent']:.0f}%"
        cv2.putText(overlay, label, (x, max(0, y - 6)), cv2.FONT_HERSHEY_SIMPLEX, 0.45, TEXT_COLOR, 1, cv2.LINE_AA)
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

    kept_detections = filter_detections(raw_detections, min_confidence=min_confidence)
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
