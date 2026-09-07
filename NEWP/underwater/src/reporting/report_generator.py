"""Geotagging + structured (JSON/CSV) anomaly report generation.

Converts pixel-space detections into real-world coordinates using sonar ping metadata
(one lat/lon/altitude sample per image row, or a resampled subset — interpolated here).
"""
from __future__ import annotations

import csv
import io
import json
from bisect import bisect_left

from ..detection.base import Detection

EARTH_RADIUS_M = 6371000.0


def _sorted_pings(metadata: list[dict]) -> list[dict]:
    return sorted(metadata, key=lambda p: p["row"])


def _interpolate_ping(pings: list[dict], row: int) -> dict:
    rows = [p["row"] for p in pings]
    idx = bisect_left(rows, row)
    if idx == 0:
        return pings[0]
    if idx >= len(pings):
        return pings[-1]
    p0, p1 = pings[idx - 1], pings[idx]
    if p1["row"] == p0["row"]:
        return p0
    t = (row - p0["row"]) / (p1["row"] - p0["row"])
    return {
        "row": row,
        "lat": p0["lat"] + t * (p1["lat"] - p0["lat"]),
        "lon": p0["lon"] + t * (p1["lon"] - p0["lon"]),
        "altitude_m": p0.get("altitude_m", 0.0) + t * (p1.get("altitude_m", 0.0) - p0.get("altitude_m", 0.0)),
    }


def _offset_latlon(lat: float, lon: float, north_m: float, east_m: float) -> tuple[float, float]:
    import math

    d_lat = (north_m / EARTH_RADIUS_M) * (180.0 / math.pi)
    d_lon = (east_m / (EARTH_RADIUS_M * math.cos(math.pi * lat / 180.0))) * (180.0 / math.pi)
    return lat + d_lat, lon + d_lon


def build_report(
    detections: list[Detection],
    metadata: list[dict] | None,
    image_shape: tuple[int, int],
    range_m: float = 30.0,
    source_image: str = "unknown.png",
) -> list[dict]:
    """Produce one structured record per detection: id, label, confidence, lat/lon, bbox, size."""
    height, width = image_shape[:2]
    m_per_px_x = (2 * range_m) / width  # sonar swath spans -range..+range across the image
    pings = _sorted_pings(metadata) if metadata else None

    records = []
    for i, det in enumerate(detections):
        cx = det.x + det.w / 2.0
        cy = det.y + det.h / 2.0

        lat, lon = None, None
        if pings:
            ping = _interpolate_ping(pings, int(cy))
            cross_track_m = (cx - width / 2.0) * m_per_px_x
            lat, lon = _offset_latlon(ping["lat"], ping["lon"], north_m=0.0, east_m=cross_track_m)

        records.append(
            {
                "id": f"det-{i:04d}",
                "label": det.label,
                "confidence": round(det.confidence, 4),
                "confidence_percent": round(det.confidence * 100, 1),
                "lat": round(lat, 7) if lat is not None else None,
                "lon": round(lon, 7) if lon is not None else None,
                "bbox_px": {"x": det.x, "y": det.y, "w": det.w, "h": det.h},
                "size_m": {
                    "width": round(det.w * m_per_px_x, 3),
                    "height": round(det.h * m_per_px_x, 3),
                },
                "range_m": range_m,
                "image": source_image,
                "features": det.features,
            }
        )
    return records


def to_json(records: list[dict]) -> str:
    return json.dumps({"detections": records, "count": len(records)}, indent=2)


def to_csv(records: list[dict]) -> str:
    buf = io.StringIO()
    fieldnames = [
        "id", "label", "confidence_percent", "lat", "lon",
        "bbox_x", "bbox_y", "bbox_w", "bbox_h", "size_width_m", "size_height_m", "range_m", "image",
    ]
    writer = csv.DictWriter(buf, fieldnames=fieldnames)
    writer.writeheader()
    for r in records:
        writer.writerow({
            "id": r["id"],
            "label": r["label"],
            "confidence_percent": r["confidence_percent"],
            "lat": r["lat"],
            "lon": r["lon"],
            "bbox_x": r["bbox_px"]["x"],
            "bbox_y": r["bbox_px"]["y"],
            "bbox_w": r["bbox_px"]["w"],
            "bbox_h": r["bbox_px"]["h"],
            "size_width_m": r["size_m"]["width"],
            "size_height_m": r["size_m"]["height"],
            "range_m": r["range_m"],
            "image": r["image"],
        })
    return buf.getvalue()
