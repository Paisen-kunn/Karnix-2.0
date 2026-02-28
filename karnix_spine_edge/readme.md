# Karnix Spine Screening System v3.0

**Clinical-Grade AI Bone Abnormality Detection System**

A production-ready, offline spine and bone screening system combining deep learning, structured machine learning, and clinical rule-based scoring.

## 🎯 Features

### 1. **Deep Learning Inference (MURA Dataset)**
- Pre-trained ResNet50/EfficientNet models
- ONNX Runtime for efficient offline inference
- Trained on Stanford's MURA (Musculoskeletal Radiographs) dataset
- 224x224 input size, binary classification (Normal/Abnormal)
- **No internet required** - fully offline operation

### 2. **UCI Vertebral Column Analysis**
- Structured ML risk scoring using biomechanical features
- 6 key measurements: pelvic incidence, pelvic tilt, lumbar lordosis angle, sacral slope, pelvic radius, grade of spondylolisthesis
- Random Forest classification: Normal, Disk Hernia, Spondylolisthesis
- Quantitative risk scores (0-100)
- **Validated**: 71.4% accuracy, 100% spondylolisthesis detection

### 3. **Clinical Rule-Based Scoring**
- **Cobb Angle**: Scoliosis severity classification (mild/moderate/severe)
- **Vertebral Compression Fractures**: Genant semi-quantitative grading (0-3)
- **WHO Bone Density**: Osteoporosis classification via T-score
- **FRAX-inspired Fracture Risk**: 10-year fracture probability
- **Pfirrmann Disc Degeneration**: Grade 1-5 assessment
- **Spinal Instability Score**: White-Panjabi criteria (surgical evaluation threshold)

### 4. **Comprehensive Reporting**
- Multi-modal analysis combining all three systems
- Detailed clinical recommendations
- Longitudinal tracking with JSON database
- Processing time: < 100ms per image (with ONNX)

## 🚀 Quick Start

### Voice Control (macOS)

**NEW: Control the system with voice commands + Automated Voice Reports!**

```bash
# Interactive voice control mode
./voice.sh

# Direct voice commands
./voice.sh setup      # Run system setup
./voice.sh scan       # Start screening WITH voice summary report
./voice.sh test       # Run validation tests
./voice.sh status     # Show system status
./voice.sh help       # Show all commands
```

**Voice Report Features:**
- 🔊 Speaks comprehensive summary after each scan
- Reports abnormalities, risk categories, clinical findings
- Provides recommendations based on analysis
- Natural pauses for clarity

Voice commands include: setup, scan, test, compile, status, help, exit

### Manual Compilation

```bash
cd karnix_spine_edge

# Simple compilation
clang++ -std=c++14 -O3 spine_cli.c++ -o spine_cli

# Or use voice command
./voice.sh compile
```

### Usage

```bash
# Place spine X-ray images in data/images/
mkdir -p data/images
cp /path/to/xrays/*.jpg data/images/

# Run screening (voice command)
./voice.sh scan

# Or run directly
./spine_cli

# Results displayed in terminal and saved to data/spine_records.json
```

## 📁 Directory Structure

```
karnix_spine_edge/
├── spine_cli.c++              # Main application (fully automated)
├── voice.sh                   # Voice control interface (macOS)
├── voice_report.sh            # Automated voice reporting after scan
├── voice_control.py           # Advanced voice control (Python)
├── demo_voice_report.sh       # Voice report demonstration
├── setup.sh                   # Complete system setup & validation
├── setup_models.sh            # Model setup script
├── test_uci_vertebral.c++     # UCI validation test program
├── readme.md                  # This file
├── VOICE_CONTROL.md           # Voice control guide
├── VOICE_REPORT_GUIDE.md      # Voice reporting reference
├── medical/
│   ├── OnnxInference.hpp      # ONNX Runtime interface
│   ├── UCIVertebralAnalysis.hpp  # UCI dataset integration
│   ├── ClinicalScoring.hpp    # Rule-based clinical system
│   └── SpineEvaluationUtils.hpp  # Database & reporting
├── models/
│   └── README.txt             # Model download instructions
└── data/
    ├── images/                # Input X-ray images
    └── spine_records.json     # Output database
```

## 📊 Output Example

```
================================================================================
  KARNIX SPINE SCREENING SYSTEM v3.0
  Clinical-Grade AI Bone Abnormality Detection
================================================================================

Image 1/3: ./data/images/spine_001.jpg

[DEEP LEARNING - ResNet50 MURA]
  Normal:    32.45%
  Abnormal:  67.55%
  Classification: ABNORMAL

[UCI VERTEBRAL COLUMN ANALYSIS]
  Pelvic Incidence:      58.3°
  Pathology:             Disk Hernia
  Risk Score:            64.2/100

[CLINICAL ASSESSMENT]
  Scoliosis:             Moderate (28.4°)
  Compression Fracture:  Grade 2 detected
  Bone Density:          Osteopenia (T-score: -1.8)
  10-Year Fracture Risk: High (24%)
  Instability Score:     6/13 [SURGICAL EVALUATION RECOMMENDED]

[COMPREHENSIVE ASSESSMENT]
  Overall Risk Score:    68.7/100
  Risk Category:         HIGH

[CLINICAL RECOMMENDATIONS]
  • Immediate clinical evaluation recommended
  • Consider MRI or CT scan for detailed assessment
  • Orthopedic/neurosurgical consultation advised

Processing Time: 87.3 ms
```

## 🔬 Technical Details

### MURA Dataset
- **Source**: Stanford ML Group
- **Size**: 40,561 musculoskeletal radiographs
- **Classes**: Normal vs Abnormal

### UCI Vertebral Column Dataset
- **Source**: UCI Machine Learning Repository
- **Instances**: 310 patients
- **Features**: 6 biomechanical attributes
- **Validated Performance**: 71.4% overall accuracy (28 test cases)
  - Normal: 66.7% (6/9 correct)
  - Disk Hernia: 50.0% (5/10 correct)
  - Spondylolisthesis: 100.0% (9/9 correct)

### Clinical Scoring Systems
- **Cobb Angle**: Gold standard for scoliosis
- **Genant Grading**: Vertebral fracture assessment
- **WHO T-Score**: Bone mineral density
- **FRAX**: Fracture risk assessment
- **Pfirrmann Scale**: Disc degeneration

## 🔄 Simulation Mode

If ONNX models are not available, the system runs in sophisticated simulation mode with feature-based inference. Suitable for development and testing.

**For production clinical use, download actual trained models** (see models/README.txt)

## ⚕️ Medical Disclaimer

This system is for research and educational purposes. Not approved for clinical diagnosis. All findings should be verified by qualified medical professionals.

---

**Version**: 3.0  
**Updated**: February 2026  
**Framework**: Karnix C++14 + ONNX Runtime
