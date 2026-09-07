"""Synthetic side-scan and forward-looking sonar image dataset generator.

Creates realistic multi-class sonar imagery with acoustic highlight and shadow effects,
speckle noise, and seafloor clutter, exported directly to YOLO dataset format.
"""
from __future__ import annotations

import json
import random
from pathlib import Path

import cv2
import numpy as np
import yaml

SONAR_CLASSES = [
    "ghost_net",
    "shipwreck",
    "pipe",
    "cylinder",
    "debris",
]


def _create_seafloor_background(
    rng: np.random.Generator, height: int = 640, width: int = 640
) -> np.ndarray:
    """Generate realistic sonar seafloor texture with ripple patterns and Rayleigh speckle noise."""
    # Base intensity with variation
    base_val = rng.uniform(80, 120)
    bg = rng.normal(loc=base_val, scale=15, size=(height, width))

    # Add acoustic gain gradient (typical in SSS/FLS across range)
    y_coords = np.linspace(0, 1, height).reshape(-1, 1)
    gain = 0.8 + 0.4 * np.sin(y_coords * np.pi)
    bg = bg * gain

    # Add seafloor ripples (sine waves with random angle)
    angle = rng.uniform(-0.5, 0.5)
    x = np.arange(width)
    y = np.arange(height)
    xx, yy = np.meshgrid(x, y)
    freq = rng.uniform(0.02, 0.06)
    ripples = 12 * np.sin(xx * np.cos(angle) * freq + yy * np.sin(angle) * freq)
    bg += ripples

    # Add Rayleigh multiplicative acoustic speckle noise
    speckle = rng.rayleigh(scale=0.25, size=(height, width)) + 0.75
    bg = bg * speckle

    # Clip to valid uint8 range
    bg = np.clip(bg, 0, 255).astype(np.uint8)
    return bg


def _add_sonar_target(
    img: np.ndarray,
    cls_id: int,
    rng: np.random.Generator,
    height: int = 640,
    width: int = 640,
) -> tuple[dict | None, np.ndarray]:
    """
    Plants a target on the seafloor with realistic highlight texture and acoustic shadow.

    Returns:
        Bounding box dict in normalized YOLO format: {'class': int, 'cx': float, 'cy': float, 'w': float, 'h': float}
        and updated image array.
    """
    # Random object dimensions depending on class type
    if cls_id == 0:  # ghost_net (irregular, net-like mesh)
        obj_w, obj_h = rng.integers(50, 120), rng.integers(20, 60)
    elif cls_id == 1:  # shipwreck (large, prominent shadow)
        obj_w, obj_h = rng.integers(70, 150), rng.integers(40, 90)
    elif cls_id == 2:  # pipe (elongated horizontal/diagonal)
        obj_w, obj_h = rng.integers(80, 160), rng.integers(12, 28)
    elif cls_id == 3:  # cylinder (compact vertical/horizontal)
        obj_w, obj_h = rng.integers(25, 55), rng.integers(25, 55)
    else:  # debris (general compact anomaly)
        obj_w, obj_h = rng.integers(30, 70), rng.integers(30, 70)

    # Position target away from edges
    margin = 80
    if width <= 2 * margin or height <= 2 * margin:
        return None, img

    cx = rng.integers(margin, width - margin)
    cy = rng.integers(margin, height - margin)

    x1 = max(0, cx - obj_w // 2)
    y1 = max(0, cy - obj_h // 2)
    x2 = min(width, cx + obj_w // 2)
    y2 = min(height, cy + obj_h // 2)

    actual_w = x2 - x1
    actual_h = y2 - y1

    if actual_w < 10 or actual_h < 10:
        return None, img

    # 1. Acoustic Highlight with Class-Specific Structural Physics
    patch = np.zeros((actual_h, actual_w), dtype=np.float32)
    
    if cls_id == 0:  # ghost_net: mesh pattern
        grid_x, grid_y = np.meshgrid(np.arange(actual_w), np.arange(actual_h))
        mesh = (np.sin(grid_x / 3.0) * np.cos(grid_y / 3.0) > 0.1).astype(np.float32)
        patch = 180 + 70 * mesh
    elif cls_id == 1:  # shipwreck: strong metallic hull reflection + structural ribs
        patch.fill(210)
        ribs = (np.arange(actual_h) % 8 < 3).reshape(-1, 1)
        patch += ribs * 40
    elif cls_id == 2:  # pipe: specular ridge reflection along longitudinal center line
        y_center = actual_h / 2.0
        dist_from_center = np.abs(np.arange(actual_h) - y_center).reshape(-1, 1)
        patch = 250 - (dist_from_center / (actual_h / 2.0 + 1e-5)) * 100
    elif cls_id == 3:  # cylinder: curved specular highlight
        x_center = actual_w / 2.0
        dist = np.abs(np.arange(actual_w) - x_center)
        patch = 245 - (dist / (actual_w / 2.0 + 1e-5)) * 90
    else:  # debris: irregular high-backscatter cluster
        patch = rng.uniform(190, 255, size=(actual_h, actual_w))

    # Add acoustic speckle noise to highlight
    speckle = rng.uniform(0.85, 1.15, size=(actual_h, actual_w))
    patch = np.clip(patch * speckle, 0, 255).astype(np.uint8)

    img[y1:y2, x1:x2] = patch

    # 2. Acoustic Shadow (dark region behind object relative to sonar beam)
    shadow_length = int(rng.uniform(1.2, 2.5) * actual_h)
    sy1 = y2
    sy2 = min(height, y2 + shadow_length)
    if sy2 > sy1:
        shadow_patch = rng.uniform(10, 35, size=(sy2 - sy1, actual_w)).astype(np.uint8)
        img[sy1:sy2, x1:x2] = shadow_patch

    # Calculate normalized YOLO bbox (center_x, center_y, w, h)
    norm_cx = (x1 + x2) / (2.0 * width)
    norm_cy = (y1 + y2) / (2.0 * height)
    norm_w = actual_w / float(width)
    norm_h = actual_h / float(height)

    bbox = {
        "class": cls_id,
        "cx": float(norm_cx),
        "cy": float(norm_cy),
        "w": float(norm_w),
        "h": float(norm_h),
    }
    return bbox, img


def generate_synthetic_yolo_dataset(
    output_dir: str | Path,
    num_train: int = 200,
    num_val: int = 40,
    img_size: tuple[int, int] = (640, 640),
    seed: int = 42,
) -> Path:
    """
    Generates a full synthetic sonar dataset formatted for YOLO training.

    Args:
        output_dir: Path where dataset will be created
        num_train: Number of training images
        num_val: Number of validation images
        img_size: (width, height)
        seed: Random seed for reproducibility

    Returns:
        Path to data.yaml file ready for YOLO training
    """
    output_path = Path(output_dir).resolve()
    images_train = output_path / "images" / "train"
    images_val = output_path / "images" / "val"
    labels_train = output_path / "labels" / "train"
    labels_val = output_path / "labels" / "val"

    for p in [images_train, images_val, labels_train, labels_val]:
        p.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(seed)
    w, h = img_size

    def _generate_split(split_name: str, count: int, img_dir: Path, lbl_dir: Path):
        for i in range(count):
            img = _create_seafloor_background(rng, height=h, width=w)

            # Add 1 to 4 target anomalies
            num_targets = rng.integers(1, 5)
            boxes = []

            for _ in range(num_targets):
                cls_id = int(rng.integers(0, len(SONAR_CLASSES)))
                bbox, img = _add_sonar_target(img, cls_id, rng, height=h, width=w)
                if bbox is not None:
                    boxes.append(bbox)

            # Convert to 3-channel BGR image
            bgr_img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)

            # Apply slight blur to simulate acoustic resolution limit
            bgr_img = cv2.GaussianBlur(bgr_img, (3, 3), 0)

            # Save image
            img_filename = f"sonar_{split_name}_{i:05d}.jpg"
            lbl_filename = f"sonar_{split_name}_{i:05d}.txt"

            cv2.imwrite(str(img_dir / img_filename), bgr_img)

            # Save YOLO format label txt file
            lbl_lines = []
            for b in boxes:
                lbl_lines.append(
                    f"{b['class']} {b['cx']:.6f} {b['cy']:.6f} {b['w']:.6f} {b['h']:.6f}"
                )
            (lbl_dir / lbl_filename).write_text("\n".join(lbl_lines))

    print(f"Generating synthetic train split ({num_train} images)...")
    _generate_split("train", num_train, images_train, labels_train)

    print(f"Generating synthetic val split ({num_val} images)...")
    _generate_split("val", num_val, images_val, labels_val)

    # Write data.yaml for Ultralytics YOLO
    yaml_data = {
        "path": str(output_path),
        "train": "images/train",
        "val": "images/val",
        "names": {i: name for i, name in enumerate(SONAR_CLASSES)},
    }

    yaml_path = output_path / "data.yaml"
    with open(yaml_path, "w") as f:
        yaml.dump(yaml_data, f, default_flow_style=False)

    print(f"Synthetic dataset generation complete! YOLO config saved to: {yaml_path}")
    return yaml_path


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Generate synthetic sonar YOLO dataset")
    parser.add_argument("--out", default="data/dataset_synthetic", help="Output directory")
    parser.add_argument("--train", type=int, default=100, help="Train count")
    parser.add_argument("--val", type=int, default=20, help="Val count")
    args = parser.parse_args()

    generate_synthetic_yolo_dataset(args.out, num_train=args.train, num_val=args.val)
