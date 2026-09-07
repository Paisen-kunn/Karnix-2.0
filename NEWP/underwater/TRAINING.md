# AquaSentry ML Training Guide 🌊🤖

AquaSentry features a complete machine learning training pipeline designed to train and fine-tune object detection models on Side-Scan Sonar (SSS) and Forward-Looking Sonar (FLS) imagery using resources from **[Awesome-Sonar-Image-Resources](https://github.com/Jorwnpay/Awesome-Sonar-Image-Resources)**.

---

## 🎯 Supported Sonar Datasets

The training pipeline integrates open-source benchmark sonar datasets listed in *Awesome-Sonar-Image-Resources*:

| Dataset Key | Dataset Name | Description |
|---|---|---|
| `sctd` | **SCTD** | 497 high-res SSS/FLS/SAS sonar images with 596 target annotations |
| `seabed` | **SeabedObjects-KLSG** | 385 ship sonar images + 62 airplane sonar images |
| `fls_detection` | **FLS Detection** | Oculus M1200d sonar dataset with victims, boats, and planes |
| `marine_debris` | **Marine Debris FLS** | ARIS Explorer 3000 sonar marine debris dataset |
| `synthetic` | **Synthetic Sonar** | Realistic generated SSS/FLS imagery with highlight/shadow physics |

---

## 🚀 Quick Start Training

### 1. Train on Synthetic Sonar Dataset (Offline / Instant Start)

Generate synthetic sonar images with acoustic highlight/shadow modeling and train YOLOv8:

```bash
cd NEWP/underwater
.venv/bin/python3 scripts/train.py --dataset synthetic --epochs 10 --batch 8
```

### 2. Train on Open-Source Benchmark Sonar Datasets

Download, format, and train directly on benchmark datasets from *Awesome-Sonar-Image-Resources*:

```bash
# Train on SCTD dataset
.venv/bin/python3 scripts/train.py --dataset sctd --epochs 20 --batch 8

# Train on SeabedObjects dataset
.venv/bin/python3 scripts/train.py --dataset seabed --epochs 20 --batch 8

# Train on Marine Debris FLS dataset
.venv/bin/python3 scripts/train.py --dataset marine_debris --epochs 20 --batch 8
```

### 3. Download & Format Datasets Separately

To only download and format datasets without immediately starting training:

```bash
# Download a single dataset
.venv/bin/python3 scripts/download_sonar_datasets.py --dataset sctd

# Download all supported benchmark datasets
.venv/bin/python3 scripts/download_sonar_datasets.py --dataset all
```

---

## 🛠️ Advanced Training Options

```bash
.venv/bin/python3 scripts/train.py \
  --dataset sctd \
  --model yolov8n.pt \
  --epochs 30 \
  --batch 16 \
  --img-size 640 \
  --device cpu \
  --out-dir models
```

### Command Line Arguments:

- `--dataset`: `synthetic` | `sctd` | `seabed` | `fls_detection` | `marine_debris`
- `--data-yaml`: Path to an existing custom YOLO `data.yaml` file
- `--model`: Base model weights (`yolov8n.pt`, `yolov8s.pt`, `yolov8m.pt`)
- `--epochs`: Number of training epochs
- `--batch`: Batch size
- `--img-size`: Resolution (default: 640)
- `--device`: `cpu`, `cuda`, or `mps`
- `--out-dir`: Output directory for trained weights (`models/best_sonar_yolo.pt`)

---

## 📊 Model Deployment & Auto-Loading

Once training completes:
1. Trained weights are automatically saved to `models/best_sonar_yolo.pt`.
2. The AquaSentry detector (`src/detection/yolo_detector.py`) automatically detects and loads `models/best_sonar_yolo.pt`.
3. The dashboard and CLI pipeline immediately use your trained model for real-time sonar anomaly detection!
