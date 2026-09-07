"""Generate a synthetic side-scan sonar image + ping metadata for demoing the pipeline
without needing a real (often restricted/licensed) SSS dataset.
"""
from __future__ import annotations

import json
from pathlib import Path

import cv2
import numpy as np

OUT_DIR = Path(__file__).resolve().parent.parent / "data" / "samples"
WIDTH, HEIGHT = 512, 768
BASE_LAT, BASE_LON = 13.0827, 80.2707  # Chennai coast, arbitrary demo origin


def _seafloor_texture(rng: np.random.Generator) -> np.ndarray:
    base = rng.normal(loc=110, scale=18, size=(HEIGHT, WIDTH))
    # sand ripple bands (natural clutter that detectors must reject)
    y = np.arange(HEIGHT).reshape(-1, 1)
    ripples = 10 * np.sin(y / 9.0)
    base += ripples
    return base


def _add_object(img: np.ndarray, cy: int, cx: int, w: int, h: int, brightness: int, shadow_len: int):
    y0, y1 = max(0, cy - h // 2), min(HEIGHT, cy + h // 2)
    x0, x1 = max(0, cx - w // 2), min(WIDTH, cx + w // 2)
    img[y0:y1, x0:x1] = brightness
    sy0, sy1 = y1, min(HEIGHT, y1 + shadow_len)
    img[sy0:sy1, x0:x1] *= 0.3


def generate(seed: int = 7) -> tuple[Path, Path]:
    rng = np.random.default_rng(seed)
    img = _seafloor_texture(rng)

    # a few natural rock clusters (irregular, low/no shadow) — should mostly be filtered out
    for _ in range(6):
        cy, cx = rng.integers(40, HEIGHT - 40), rng.integers(40, WIDTH - 40)
        r = rng.integers(8, 16)
        cv2.circle(img, (cx, cy), r, float(rng.integers(140, 170)), -1)

    # planted man-made anomalies: elongated "pipe/net" + compact "wreck part" + cylinder
    _add_object(img, 180, 150, w=90, h=14, brightness=225, shadow_len=22)   # pipe/net-like
    _add_object(img, 380, 340, w=40, h=40, brightness=235, shadow_len=30)   # compact wreck part
    _add_object(img, 560, 200, w=22, h=55, brightness=230, shadow_len=26)   # cylinder

    img = np.clip(img, 0, 255).astype(np.uint8)
    img = cv2.GaussianBlur(img, (3, 3), 0)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    image_path = OUT_DIR / "demo_sonar.png"
    cv2.imwrite(str(image_path), img)

    pings = []
    for row in range(0, HEIGHT, 32):
        pings.append({
            "row": row,
            "lat": BASE_LAT + row * 0.000006,
            "lon": BASE_LON + row * 0.000002,
            "altitude_m": 35.0 + float(rng.normal(0, 0.5)),
        })
    metadata_path = OUT_DIR / "demo_metadata.json"
    metadata_path.write_text(json.dumps(pings, indent=2))

    return image_path, metadata_path


if __name__ == "__main__":
    image_path, metadata_path = generate()
    print(f"Wrote {image_path}")
    print(f"Wrote {metadata_path}")
