#!/bin/bash

# force_fresh_build.sh - Aggressively rebuild with no caching
set -e

echo "=== Force Fresh Build Script ==="
echo "Build started at: $(date)"

# Clear all possible caches
echo "1. Clearing Python caches..."
python3 -c "
import sys
import importlib
import os
import shutil

# Clear module from memory
if 'lpximage' in sys.modules:
    del sys.modules['lpximage']

# Clear import caches
importlib.invalidate_caches()

# Clear pip cache
os.system('pip3 cache purge --break-system-packages 2>/dev/null || true')

print('Python caches cleared')
"

# Force uninstall existing module
echo "2. Uninstalling existing module..."
pip3 uninstall lpximage -y --break-system-packages 2>/dev/null || true

# Remove build directory completely
echo "3. Removing build directory..."
rm -rf build

# Remove any cached Python bytecode
echo "4. Removing Python bytecode cache..."
find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "*.pyc" -delete 2>/dev/null || true

# Create fresh build directory
echo "5. Creating fresh build..."
mkdir build
cd build

# Configure with fresh timestamps
echo "6. Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with verbose output
echo "7. Building with make..."
make VERBOSE=1

# Check the version in the built library
echo "8. Checking built library version..."
echo "Shared library version strings:"
strings liblpx_image.dylib | grep -E "(Jul.*24.*2025|19:5[0-9]|20:0[0-9])" || echo "No recent timestamps found"

# Force install with no cache
echo "9. Installing Python module with no cache..."
cd ..
PYTHONDONTWRITEBYTECODE=1 pip3 install -e . --break-system-packages --force-reinstall --no-cache-dir --no-deps

echo "10. Testing installation..."
python3 -c "
import sys
import importlib

# Clear any cached imports
importlib.invalidate_caches()
if 'lpximage' in sys.modules:
    del sys.modules['lpximage']

print('Fresh import test:')
import lpximage
print('Version:', lpximage.getVersionString())
print('Build:', lpximage.getBuildNumber())
print('All available functions:', sorted([f for f in dir(lpximage) if not f.startswith('_')]))
"

echo ""
echo "=== Build Complete ==="
echo "Build finished at: $(date)"
