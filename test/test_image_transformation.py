#!/usr/bin/env python3
"""
Test basic image transformation example from python/README.md
"""

import cv2
import numpy as np
import lpximage

# Create a dummy image instead of loading from file
width, height = 640, 480
img = np.random.randint(0, 255, (height, width, 3), dtype=np.uint8)
print("✓ Created dummy image")

# Convert BGR to RGB for processing (normally done with real image)
img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

# Initialize the LPX system
if not lpximage.initLPX("../ScanTables63", img.shape[1], img.shape[0]):
    print("❌ Failed to initialize LPX system")
    exit(1)
else:
    print("✓ Successfully initialized LPX system")

# Define center point (usually the center of the image)
center_x = img.shape[1] / 2
center_y = img.shape[0] / 2

# Convert to log-polar
lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)
print(f"✓ Created LPX image with {lpx_image.getLength()} cells")

# Initialize renderer
renderer = lpximage.LPXRenderer()
renderer.setScanTables(lpximage.LPXTables("../ScanTables63"))
print("✓ Successfully initialized renderer")

# Render back to standard format
rendered = renderer.renderToImage(lpx_image, 800, 600, 1.0)
print("✓ Successfully rendered LPX image back to standard format")

# Convert back to BGR for display with OpenCV
result_bgr = cv2.cvtColor(rendered, cv2.COLOR_RGB2BGR)
print("✓ Successfully converted rendered image back to BGR")

print("✓ All image transformation steps completed successfully")
