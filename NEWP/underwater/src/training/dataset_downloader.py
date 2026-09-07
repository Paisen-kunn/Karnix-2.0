"""Downloader and converter for benchmark sonar image datasets from Awesome-Sonar-Image-Resources.

Supports downloading and preparing open-source datasets:
- SCTD (Sonar Common Target Detection Dataset)
- SeabedObjects-KLSG (Ship & Airplane dataset)
- FLS-Detection (Forward-looking Sonar Detection Dataset)
- Marine-Debris-FLS (Turntable and Water-tank datasets)
"""
from __future__ import annotations

import os
import shutil
import subprocess
import tarfile
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path

import cv2
import numpy as np
import yaml

DATASET_REPOS = {
    "sctd": {
        "name": "SCTD (Sonar Common Target Detection Dataset)",
        "url": "https://github.com/freepoet/SCTD.git",
        "description": "497 high-resolution side-scan, FLS, and SAS sonar images with 596 target annotations.",
    },
    "seabed": {
        "name": "SeabedObjects-KLSG (Ship and Airplane)",
        "url": "https://github.com/huoguanying/SeabedObjects-Ship-and-Airplane-dataset.git",
        "description": "385 ship sonar images and 62 airplane sonar images.",
    },
    "fls_detection": {
        "name": "Forward-looking Sonar Detection Dataset",
        "url": "https://github.com/XingYZhu/Forward-looking-Sonar-Detection-Dataset.git",
        "description": "FLS Oculus M1200d sonar dataset with victims, boats, and planes.",
    },
    "marine_debris": {
        "name": "Marine Debris FLS Dataset",
        "url": "https://github.com/mvaldenegro/marine-debris-fls-datasets.git",
        "description": "Forward-looking sonar marine debris dataset captured with ARIS Explorer 3000.",
    },
}

STANDARD_CLASSES = [
    "ghost_net",
    "shipwreck",
    "pipe",
    "cylinder",
    "debris",
    "boat",
    "plane",
]


def _clone_repo(repo_url: str, target_dir: Path) -> bool:
    """Clones a GitHub repository if not already present."""
    if target_dir.exists() and any(target_dir.iterdir()):
        print(f"Repository already downloaded at {target_dir}")
        return True

    target_dir.parent.mkdir(parents=True, exist_ok=True)
    print(f"Cloning dataset from {repo_url} into {target_dir}...")
    try:
        subprocess.run(
            ["git", "clone", "--depth", "1", repo_url, str(target_dir)],
            check=True,
            capture_output=True,
            text=True,
        )
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error cloning {repo_url}: {e.stderr}")
        return False


def _convert_voc_xml_to_yolo(xml_path: Path, img_width: int, img_height: int) -> list[str]:
    """Parses Pascal VOC XML bounding box annotations and converts to YOLO format lines."""
    yolo_lines = []
    try:
        tree = ET.parse(xml_path)
        root = tree.getroot()

        for obj in root.findall("object"):
            name = obj.find("name")
            if name is None:
                continue
            obj_name = name.text.lower().strip()

            # Map class name to standard index
            cls_id = 4  # default to 'debris'
            if "net" in obj_name:
                cls_id = 0
            elif "ship" in obj_name or "wreck" in obj_name:
                cls_id = 1
            elif "pipe" in obj_name:
                cls_id = 2
            elif "cylin" in obj_name or "cube" in obj_name or "ball" in obj_name:
                cls_id = 3
            elif "boat" in obj_name or "victim" in obj_name:
                cls_id = 5
            elif "plane" in obj_name or "aircraft" in obj_name:
                cls_id = 6

            bndbox = obj.find("bndbox")
            if bndbox is None:
                continue

            xmin = float(bndbox.find("xmin").text)
            ymin = float(bndbox.find("ymin").text)
            xmax = float(bndbox.find("xmax").text)
            ymax = float(bndbox.find("ymax").text)

            cx = (xmin + xmax) / (2.0 * img_width)
            cy = (ymin + ymax) / (2.0 * img_height)
            w = (xmax - xmin) / float(img_width)
            h = (ymax - ymin) / float(img_height)

            cx = min(max(0.0, cx), 1.0)
            cy = min(max(0.0, cy), 1.0)
            w = min(max(0.0, w), 1.0)
            h = min(max(0.0, h), 1.0)

            yolo_lines.append(f"{cls_id} {cx:.6f} {cy:.6f} {w:.6f} {h:.6f}")
    except Exception as e:
        print(f"Warning: Failed to parse XML {xml_path}: {e}")

    return yolo_lines


def _extract_archives(raw_dir: Path):
    """Automatically extracts .zip, .tar, .tar.gz, .tgz archives found in raw_dir."""
    zip_files = list(raw_dir.rglob("*.zip"))
    for zf in zip_files:
        dest_dir = zf.parent / zf.stem
        if not dest_dir.exists() or not any(dest_dir.iterdir()):
            print(f"Extracting zip archive: {zf.name}...")
            try:
                with zipfile.ZipFile(zf, "r") as z_ref:
                    z_ref.extractall(dest_dir)
            except Exception as e:
                print(f"Warning: Failed to extract zip {zf}: {e}")

    tar_files = list(raw_dir.rglob("*.tar")) + list(raw_dir.rglob("*.tar.gz")) + list(raw_dir.rglob("*.tgz"))
    for tf in tar_files:
        dest_dir = tf.parent / tf.stem
        if not dest_dir.exists() or not any(dest_dir.iterdir()):
            print(f"Extracting tar archive: {tf.name}...")
            try:
                with tarfile.open(tf, "r:*") as t_ref:
                    t_ref.extractall(dest_dir)
            except Exception as e:
                print(f"Warning: Failed to extract tar {tf}: {e}")


def format_dataset_to_yolo(raw_dir: Path, output_dir: Path) -> Path:
    """
    Scans raw downloaded dataset directory for images and annotations (XML / TXT)
    and structures them into a standard YOLO dataset directory with data.yaml.
    """
    output_dir = output_dir.resolve()
    images_train = output_dir / "images" / "train"
    images_val = output_dir / "images" / "val"
    labels_train = output_dir / "labels" / "train"
    labels_val = output_dir / "labels" / "val"

    for p in [images_train, images_val, labels_train, labels_val]:
        p.mkdir(parents=True, exist_ok=True)

    # Extract any archives first
    _extract_archives(raw_dir)

    image_exts = {".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff"}
    image_files = [f for f in raw_dir.rglob("*") if f.is_file() and f.suffix.lower() in image_exts]

    if not image_files:
        raise ValueError(f"No image files found in {raw_dir}")

    print(f"Found {len(image_files)} images in raw dataset {raw_dir}")

    np.random.seed(42)
    indices = np.random.permutation(len(image_files))
    split_point = int(len(image_files) * 0.8)
    train_indices = set(indices[:split_point])

    processed_count = 0

    for idx, img_path in enumerate(image_files):
        is_train = idx in train_indices
        target_img_dir = images_train if is_train else images_val
        target_lbl_dir = labels_train if is_train else labels_val

        img = cv2.imread(str(img_path))
        if img is None:
            continue

        h, w = img.shape[:2]
        file_stem = f"sonar_{idx:05d}"
        dest_img_path = target_img_dir / f"{file_stem}.jpg"
        cv2.imwrite(str(dest_img_path), img)

        # Look for corresponding annotation XML or TXT
        xml_candidates = list(raw_dir.rglob(f"{img_path.stem}.xml"))
        yolo_lines = []

        if xml_candidates:
            yolo_lines = _convert_voc_xml_to_yolo(xml_candidates[0], w, h)
        else:
            txt_candidates = list(raw_dir.rglob(f"{img_path.stem}.txt"))
            if txt_candidates:
                try:
                    yolo_lines = txt_candidates[0].read_text().strip().splitlines()
                except Exception:
                    pass

        # If no explicit bounding box annotation found, create central target box
        if not yolo_lines:
            cls_id = 1 if "ship" in img_path.stem.lower() else 4
            yolo_lines = [f"{cls_id} 0.500000 0.500000 0.400000 0.400000"]

        dest_lbl_path = target_lbl_dir / f"{file_stem}.txt"
        dest_lbl_path.write_text("\n".join(yolo_lines))
        processed_count += 1

    # Write data.yaml
    yaml_data = {
        "path": str(output_dir),
        "train": "images/train",
        "val": "images/val",
        "names": {i: name for i, name in enumerate(STANDARD_CLASSES)},
    }

    yaml_path = output_dir / "data.yaml"
    with open(yaml_path, "w") as f:
        yaml.dump(yaml_data, f, default_flow_style=False)

    print(f"Formatted {processed_count} images to YOLO dataset at {output_dir}")
    return yaml_path


def download_and_prepare_dataset(dataset_key: str, data_dir: str | Path = "data") -> Path:
    """
    Downloads a sonar dataset by key (sctd, seabed, fls_detection, marine_debris)
    and prepares it as a YOLO dataset.

    Args:
        dataset_key: Key in DATASET_REPOS ('sctd', 'seabed', etc.)
        data_dir: Base directory to store dataset files

    Returns:
        Path to generated data.yaml
    """
    if dataset_key not in DATASET_REPOS:
        raise ValueError(
            f"Unknown dataset key '{dataset_key}'. Supported: {list(DATASET_REPOS.keys())}"
        )

    base_dir = Path(data_dir).resolve()
    repo_info = DATASET_REPOS[dataset_key]
    raw_dir = base_dir / "raw" / dataset_key
    processed_dir = base_dir / f"dataset_{dataset_key}"

    print(f"=== Preparing {repo_info['name']} ===")
    print(f"Description: {repo_info['description']}")

    success = _clone_repo(repo_info["url"], raw_dir)
    if not success:
        raise RuntimeError(f"Failed to download repository for {dataset_key}")

    yaml_path = format_dataset_to_yolo(raw_dir, processed_dir)
    return yaml_path


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Download & format sonar datasets")
    parser.add_argument(
        "--dataset",
        choices=list(DATASET_REPOS.keys()),
        default="sctd",
        help="Dataset to download",
    )
    parser.add_argument("--out", default="data", help="Output base directory")
    args = parser.parse_args()

    download_and_prepare_dataset(args.dataset, data_dir=args.out)
