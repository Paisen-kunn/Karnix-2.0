"""YOLO model training and fine-tuning pipeline for sonar anomaly detection."""
from __future__ import annotations

import json
import shutil
import sys
import time
from pathlib import Path

try:
    from ultralytics import YOLO
    YOLO_AVAILABLE = True
except ImportError:
    YOLO_AVAILABLE = False


def train_yolo_model(
    data_yaml_path: str | Path,
    model_name: str = "yolov8n.pt",
    epochs: int = 10,
    batch_size: int = 8,
    img_size: int = 640,
    device: str = "cpu",
    output_dir: str | Path = "models",
    lr0: float = 0.01,
) -> dict:
    """
    Train or fine-tune a YOLO object detection model on sonar imagery.

    Args:
        data_yaml_path: Path to dataset data.yaml file
        model_name: Base model weight or architecture ('yolov8n.pt', 'yolov8s.pt', etc.)
        epochs: Number of training epochs
        batch_size: Training batch size
        img_size: Input image resolution
        device: 'cpu', 'cuda', or 'mps'
        output_dir: Path to save trained weights and metrics
        lr0: Initial learning rate

    Returns:
        Dict containing training summary, best weight path, and performance metrics
    """
    if not YOLO_AVAILABLE:
        raise ImportError(
            "Ultralytics is required for YOLO training. Install with: pip install ultralytics"
        )

    yaml_path = Path(data_yaml_path).resolve()
    if not yaml_path.exists():
        raise FileNotFoundError(f"data.yaml not found at {yaml_path}")

    out_path = Path(output_dir).resolve()
    out_path.mkdir(parents=True, exist_ok=True)

    print(f"==================================================")
    print(f"🚀 Starting AquaSentry YOLO Sonar Detector Training")
    print(f"==================================================")
    print(f"Dataset YAML: {yaml_path}")
    print(f"Base Model: {model_name}")
    print(f"Epochs: {epochs} | Batch Size: {batch_size} | Image Size: {img_size}")
    print(f"Device: {device} | Learning Rate: {lr0}")
    print(f"Output Directory: {out_path}")
    print(f"--------------------------------------------------")

    start_time = time.time()

    # Load YOLO model
    model = YOLO(model_name)

    # Train model with optimal data augmentation and learning schedule for high precision
    results = model.train(
        data=str(yaml_path),
        epochs=epochs,
        batch=batch_size,
        imgsz=img_size,
        device=device,
        lr0=lr0,
        lrf=0.01,
        cos_lr=True,
        patience=25,
        mosaic=1.0,
        mixup=0.15,
        degrees=15.0,
        translate=0.1,
        scale=0.5,
        fliplr=0.5,
        flipud=0.5,
        hsv_h=0.015,
        hsv_s=0.7,
        hsv_v=0.4,
        box=7.5,
        cls=0.5,
        dfl=1.5,
        project=str(out_path / "runs"),
        name="sonar_yolo_experiment",
        exist_ok=True,
        verbose=True,
    )

    training_time = time.time() - start_time

    # Find trained weights
    exp_dir = out_path / "runs" / "sonar_yolo_experiment"
    weights_best = exp_dir / "weights" / "best.pt"
    weights_last = exp_dir / "weights" / "last.pt"

    target_best = out_path / "best_sonar_yolo.pt"
    target_last = out_path / "last_sonar_yolo.pt"

    if weights_best.exists():
        shutil.copy(weights_best, target_best)
        print(f"✅ Saved best trained model weights to: {target_best}")

    if weights_last.exists():
        shutil.copy(weights_last, target_last)

    # Extract metrics if available
    metrics = {}
    if hasattr(results, "results_dict"):
        metrics = results.results_dict
    elif hasattr(model, "metrics"):
        metrics = model.metrics

    summary = {
        "status": "completed",
        "model_name": model_name,
        "epochs": epochs,
        "batch_size": batch_size,
        "img_size": img_size,
        "device": device,
        "training_time_seconds": round(training_time, 2),
        "dataset_yaml": str(yaml_path),
        "best_weights_path": str(target_best),
        "metrics": metrics,
    }

    summary_file = out_path / "training_summary.json"
    summary_file.write_text(json.dumps(summary, indent=2, default=str))

    print(f"--------------------------------------------------")
    print(f"🎉 Training finished in {training_time:.1f}s")
    print(f"Summary saved to: {summary_file}")
    print(f"==================================================")

    return summary


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Train YOLO on sonar target dataset")
    parser.add_argument("--data", required=True, help="Path to data.yaml")
    parser.add_argument("--model", default="yolov8n.pt", help="Base model")
    parser.add_argument("--epochs", type=int, default=5, help="Number of epochs")
    parser.add_argument("--batch", type=int, default=8, help="Batch size")
    parser.add_argument("--device", default="cpu", help="Device (cpu, cuda, mps)")
    parser.add_argument("--out", default="models", help="Output directory")

    args = parser.parse_args()

    train_yolo_model(
        data_yaml_path=args.data,
        model_name=args.model,
        epochs=args.epochs,
        batch_size=args.batch,
        device=args.device,
        output_dir=args.out,
    )
