KARNIX SPINE SCREENING - AI MODELS
===================================

Required Models:
---------------

1. resnet50_mura.onnx (Primary Model)
   - Architecture: ResNet50
   - Training Dataset: MURA (Musculoskeletal Radiographs)
   - Input: 224x224x3 RGB
   - Output: 2 classes (Normal/Abnormal)
   - Size: ~98 MB
   
   Download from:
   - MURA dataset: https://stanfordmlgroup.github.io/competitions/mura/
   - Pre-trained ResNet50 ONNX: https://github.com/onnx/models/tree/main/vision/classification/resnet
   
   Training:
   - Train on MURA dataset using PyTorch/TensorFlow
   - Export to ONNX format: torch.onnx.export()
   - Place in this directory as: resnet50_mura.onnx

2. efficientnet_b0.onnx (Alternative Model)
   - Architecture: EfficientNet-B0
   - More efficient for edge deployment
   - Input: 224x224x3 RGB
   - Output: 2 classes
   - Size: ~21 MB
   
   Download from:
   - EfficientNet ONNX: https://github.com/onnx/models/tree/main/vision/classification/efficientnet-lite4

3. densenet121.onnx (Medical Imaging Specialist)
   - Architecture: DenseNet121
   - Trained specifically for bone abnormality detection
   - Input: 224x224x3 RGB
   - Output: 2 classes
   - Size: ~32 MB

Model Training Instructions:
---------------------------

If you want to train your own models:

1. Download MURA Dataset:
   wget https://cs.stanford.edu/group/mlgroup/MURA-v1.1.zip
   
2. Train with PyTorch:
   ```python
   import torch
   import torchvision.models as models
   
   # Load pre-trained ResNet50
   model = models.resnet50(pretrained=True)
   
   # Modify final layer for binary classification
   model.fc = torch.nn.Linear(2048, 2)
   
   # Train on MURA dataset...
   # (Training code here)
   
   # Export to ONNX
   dummy_input = torch.randn(1, 3, 224, 224)
   torch.onnx.export(model, dummy_input, "resnet50_mura.onnx",
                     input_names=['input'], output_names=['output'],
                     dynamic_axes={'input': {0: 'batch_size'},
                                   'output': {0: 'batch_size'}})
   ```

3. Optimize for inference:
   ```bash
   python -m onnxruntime.tools.convert_onnx_models_to_ort resnet50_mura.onnx
   ```

UCI Vertebral Dataset:
---------------------
The system also uses features from the UCI Vertebral Column dataset for
structured ML risk scoring. This dataset is automatically integrated into
the analysis pipeline.

Dataset: https://archive.ics.uci.edu/ml/datasets/Vertebral+Column

Clinical Scoring System:
------------------------
The rule-based clinical scoring doesn't require model files. It uses:
- Cobb angle classification (scoliosis)
- Vertebral compression fracture detection
- FRAX-inspired fracture risk assessment
- WHO bone density classification
- Spinal instability scoring

Simulation Mode:
---------------
If model files are not present, the system will run in simulation mode
with sophisticated feature-based inference. This is suitable for:
- Testing the system architecture
- Development and debugging
- Demonstrations

For production clinical use, download and use the actual trained models.

