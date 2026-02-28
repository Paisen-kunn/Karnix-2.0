#!/bin/bash

# create_test_images.sh
# Create sample test images for Karnix Spine Screening

echo "Creating test X-ray images..."

cd data/images

# Create a simple grayscale PPM image (normal spine)
cat > test_normal_01.ppm << 'EOF'
P5
224 224
255
EOF

# Generate random grayscale data (simulate normal spine)
python3 << 'PYTHON'
import sys
import random

# Generate 224x224 grayscale image
for y in range(224):
    for x in range(224):
        # Add some structure (vertical spinal column in center)
        center_x = 112
        if abs(x - center_x) < 20:
            # Darker values for spine
            intensity = int(120 + random.gauss(0, 15))
        else:
            # Lighter background
            intensity = int(180 + random.gauss(0, 20))
        
        # Clamp to 0-255
        intensity = max(0, min(255, intensity))
        sys.stdout.buffer.write(bytes([intensity]))
PYTHON

# Create abnormal spine image
cat > test_abnormal_01.ppm << 'EOF'
P5
224 224
255
EOF

python3 << 'PYTHON'
import sys
import random
import math

# Generate 224x224 grayscale image with abnormality
for y in range(224):
    for x in range(224):
        center_x = 112
        
        # Add curvature to simulate scoliosis
        curve_offset = int(10 * math.sin(y / 30.0))
        curved_center = center_x + curve_offset
        
        if abs(x - curved_center) < 20:
            # More variation and darker patches (fracture simulation)
            intensity = int(100 + random.gauss(0, 30))
            
            # Add bright spots (abnormality simulation)
            if random.random() < 0.05:
                intensity = int(255)
        else:
            intensity = int(170 + random.gauss(0, 25))
        
        intensity = max(0, min(255, intensity))
        sys.stdout.buffer.write(bytes([intensity]))
PYTHON

# Create another normal image
cat > test_normal_02.ppm << 'EOF'
P5
224 224
255
EOF

python3 << 'PYTHON'
import sys
import random

for y in range(224):
    for x in range(224):
        center_x = 112
        if abs(x - center_x) < 18:
            intensity = int(130 + random.gauss(0, 10))
        else:
            intensity = int(190 + random.gauss(0, 15))
        
        intensity = max(0, min(255, intensity))
        sys.stdout.buffer.write(bytes([intensity]))
PYTHON

echo "✓ Created test_normal_01.ppm"
echo "✓ Created test_abnormal_01.ppm"
echo "✓ Created test_normal_02.ppm"
echo ""
echo "You can now run: ./spine_cli"
