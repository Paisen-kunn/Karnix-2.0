#!/usr/bin/env bash
set -e

echo "=================================================="
echo "🌊 AquaSentry ML & Pipeline Setup"
echo "=================================================="

# 1. Check Python version
PYTHON_CMD="python3"
if ! command -v $PYTHON_CMD &> /dev/null; then
    echo "Error: python3 is required but not found."
    exit 1
fi

echo "[1/4] Setting up Python virtual environment (.venv)..."
if [ ! -d ".venv" ]; then
    $PYTHON_CMD -m venv .venv
fi

source .venv/bin/activate

echo "[2/4] Installing Python dependencies..."
pip install --upgrade pip
pip install -r requirements.txt

echo "[3/4] Downloading & preparing benchmark sonar datasets..."
python3 scripts/download_sonar_datasets.py --dataset sctd

echo "[4/4] Training high-accuracy YOLO ML Sonar Detector..."
# Train on prepared SCTD dataset for 10 epochs (or synthetic if preferred)
python3 scripts/train.py --dataset sctd --epochs 10 --batch 8 --out-dir models

echo "=================================================="
echo "🎉 AquaSentry Setup & Training Complete!"
echo "Trained weights saved to: models/best_sonar_yolo.pt"
echo "To start the backend dashboard:"
echo "   cd dashboard && ../.venv/bin/python3 app.py"
echo "To start the frontend UI:"
echo "   cd frontend && npm install && npm run dev"
echo "=================================================="
