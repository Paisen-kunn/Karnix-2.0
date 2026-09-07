#!/usr/bin/env python3
"""Unified training CLI for AquaSentry ML Sonar Object Detector.

Supports training on datasets from Awesome-Sonar-Image-Resources or synthetic sonar data.

Examples:
    # 1. Train on synthetic sonar dataset (quick offline start):
    python3 scripts/train.py --dataset synthetic --epochs 5

    # 2. Download & train on SCTD benchmark sonar dataset:
    python3 scripts/train.py --dataset sctd --epochs 10

    # 3. Train on SeabedObjects dataset:
    python3 scripts/train.py --dataset seabed --epochs 10

    # 4. Train on custom data.yaml:
    python3 scripts/train.py --data-yaml path/to/data.yaml --epochs 10
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Ensure project root is in sys.path
PROJECT_ROOT = Path(__file__).resolve().parent.parent
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from src.training.dataset_downloader import DATASET_REPOS, download_and_prepare_dataset
from src.training.synthetic_generator import generate_synthetic_yolo_dataset
from src.training.train_yolo import train_yolo_model


def main() -> int:
    parser = argparse.ArgumentParser(
        description="AquaSentry ML Sonar Detector Training CLI"
    )

    dataset_group = parser.add_mutually_exclusive_group(required=False)
    dataset_group.add_argument(
        "--dataset",
        choices=["synthetic"] + list(DATASET_REPOS.keys()),
        default="synthetic",
        help="Choose dataset source: 'synthetic' or open-source benchmark key ('sctd', 'seabed', 'fls_detection', 'marine_debris')",
    )
    dataset_group.add_argument(
        "--data-yaml",
        help="Path to an existing YOLO data.yaml file",
    )

    parser.add_argument(
        "--high-accuracy",
        action="store_true",
        help="Enable high-accuracy preset (yolov8s.pt backbone, 25 epochs, expanded dataset, data augmentations)",
    )
    parser.add_argument(
        "--model",
        default="yolov8n.pt",
        help="Base YOLO model weight or architecture (default: yolov8n.pt)",
    )
    parser.add_argument(
        "--epochs",
        type=int,
        default=10,
        help="Number of training epochs (default: 10)",
    )
    parser.add_argument(
        "--batch",
        type=int,
        default=8,
        help="Batch size (default: 8)",
    )
    parser.add_argument(
        "--img-size",
        type=int,
        default=640,
        help="Training image resolution (default: 640)",
    )
    parser.add_argument(
        "--device",
        default="cpu",
        help="Hardware execution device: 'cpu', 'cuda', or 'mps' (default: cpu)",
    )
    parser.add_argument(
        "--out-dir",
        default="models",
        help="Directory to save trained model weights and reports (default: models)",
    )
    parser.add_argument(
        "--synth-train-size",
        type=int,
        default=200,
        help="Number of synthetic training images if --dataset synthetic (default: 200)",
    )
    parser.add_argument(
        "--synth-val-size",
        type=int,
        default=40,
        help="Number of synthetic validation images if --dataset synthetic (default: 40)",
    )

    args = parser.parse_args()

    # Apply high-accuracy preset if requested
    if args.high_accuracy:
        print("🎯 High-Accuracy preset enabled!")
        if args.model == "yolov8n.pt":
            args.model = "yolov8s.pt"
        if args.epochs == 10:
            args.epochs = 25
        if args.synth_train_size == 200:
            args.synth_train_size = 300
        if args.synth_val_size == 40:
            args.synth_val_size = 60

    # Determine dataset data.yaml path
    if args.data_yaml:
        yaml_path = Path(args.data_yaml).resolve()
        if not yaml_path.exists():
            print(f"Error: Specified data.yaml not found at {yaml_path}", file=sys.stderr)
            return 1
    elif args.dataset == "synthetic":
        print("Generating synthetic sonar dataset for ML training...")
        synth_dir = PROJECT_ROOT / "data" / "dataset_synthetic"
        yaml_path = generate_synthetic_yolo_dataset(
            output_dir=synth_dir,
            num_train=args.synth_train_size,
            num_val=args.synth_val_size,
            img_size=(args.img_size, args.img_size),
        )
    else:
        print(f"Preparing dataset '{args.dataset}' from Awesome-Sonar-Image-Resources...")
        yaml_path = download_and_prepare_dataset(
            dataset_key=args.dataset,
            data_dir=PROJECT_ROOT / "data",
        )

    # Launch YOLO training
    summary = train_yolo_model(
        data_yaml_path=yaml_path,
        model_name=args.model,
        epochs=args.epochs,
        batch_size=args.batch,
        img_size=args.img_size,
        device=args.device,
        output_dir=PROJECT_ROOT / args.out_dir,
    )

    print(f"\nTraining session complete!")
    print(f"Best Weights: {summary['best_weights_path']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
