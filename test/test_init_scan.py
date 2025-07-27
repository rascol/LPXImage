#!/usr/bin/env python3
"""
Test initLPX and scanImage functions from python/README.md
"""

import lpximage
import numpy as np

# Initialize LPX system
width, height = 640, 480
if not lpximage.initLPX("../ScanTables63", width, height):
    print("❌ Failed to initialize LPX system")
    exit(1)
else:
    print("✓ Successfully initialized LPX system")

# Create a dummy RGB image (normally you'd use cv2.cvtColor from BGR)
img_rgb = np.random.randint(0, 255, (height, width, 3), dtype=np.uint8)

# Center of the image
center_x = width / 2
center_y = height / 2

# Scan the image to create an LPXImage
lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)
print(f"✓ Successfully created LPX image with {lpx_image.getLength()} cells")
