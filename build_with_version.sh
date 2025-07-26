#!/bin/bash

# build_with_version.sh - Build script that increments build number
set -e

echo "=== LPXImage Build with Version Tracking ==="

# Get current build number from C++ constants
CURRENT_BUILD=$(grep "BUILD_NUMBER = " include/lpx_version.h | grep -o '[0-9]\+')
NEW_BUILD=$((CURRENT_BUILD + 1))

echo "Current build: $CURRENT_BUILD"
echo "New build: $NEW_BUILD"

# Update version header with C++ constants
sed -i '' "s/BUILD_NUMBER = [0-9]\+/BUILD_NUMBER = $NEW_BUILD/" include/lpx_version.h
sed -i '' "s/VERSION_STRING = \"[^\"]*\"/VERSION_STRING = \"1.0.$NEW_BUILD\"/" include/lpx_version.h

echo "Updated version files with build $NEW_BUILD"

# Clean previous build and all caches
echo "Cleaning previous build and caches..."

# Clear Python caches
echo "Clearing Python caches..."
python3 -c "
import sys
import importlib
import os

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
echo "Uninstalling existing module..."
pip3 uninstall lpximage -y --break-system-packages 2>/dev/null || true

# Remove build directory completely
make clean 2>/dev/null || true
rm -rf build 2>/dev/null || true

# Remove any cached Python bytecode
echo "Removing Python bytecode cache..."
find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "*.pyc" -delete 2>/dev/null || true

# Create build directory
mkdir -p build
cd build

# Configure and build
echo "Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building..."
make -j$(sysctl -n hw.ncpu)

echo "Installing Python module..."
cd ..
sudo cp build/python/lpximage.cpython-313-darwin.so /opt/homebrew/lib/python3.13/site-packages/
sudo cp build/liblpx_image.dylib /opt/homebrew/lib/python3.13/site-packages/liblpx_image.1.dylib

echo ""
echo "=== Build Complete ==="
echo "New build number: $NEW_BUILD"
echo "Python module location: $(python3 -c 'import lpximage; print(lpximage.__file__)')"
echo "Python version: $(python3 --version)"
echo ""
echo "You can now check the version with:"
echo "  python3 -c 'import lpximage; print(f\"Version: {lpximage.getVersionString()}, Build: {lpximage.getBuildNumber()}, Throttle: {lpximage.getKeyThrottleMs()}ms\")"
