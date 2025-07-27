#!/usr/bin/env python3
"""
Test the corrected LPXRenderer example from python/README.md
"""

import lpximage
import numpy as np

# Initialize scan tables first
tables = lpximage.LPXTables("../ScanTables63")

if not tables.isInitialized():
    print("❌ Failed to initialize scan tables")
    exit(1)

print("✓ Successfully initialized scan tables")

# Initialize LPX system to create a proper LPXImage
if not lpximage.initLPX("../ScanTables63", 640, 480):
    print("❌ Failed to initialize LPX system")
    exit(1)

# Create a dummy RGB image and scan it to create an LPXImage
img_rgb = np.random.randint(0, 255, (480, 640, 3), dtype=np.uint8)
lpx_image = lpximage.scanImage(img_rgb, 320, 240)

print("✓ Created LPXImage for testing")

# Initialize renderer (this is the corrected documentation example)
renderer = lpximage.LPXRenderer()
renderer.setScanTables(tables)

print("✓ Successfully initialized renderer with scan tables")

# Render a log-polar image to a standard RGB image
# Parameters: lpx_image, width, height, scale
standard_img = renderer.renderToImage(lpx_image, 800, 600, 1.0)
print("✓ Successfully rendered LPX image to standard format")
print("✓ Documentation example now works correctly!")
