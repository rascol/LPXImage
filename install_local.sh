#!/bin/bash
# install_local.sh - Build and install LPXImage libraries locally

set -e  # Exit on any error

echo "=== LPXImage Local Installation Script ==="
echo "This script will build and install the LPXImage libraries locally."
echo ""

# Get the project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Project root: $PROJECT_ROOT"

# Create and enter build directory
BUILD_DIR="$PROJECT_ROOT/build"
echo "Build directory: $BUILD_DIR"

if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "=== Configuring CMake for local installation ==="
cmake -DCMAKE_INSTALL_PREFIX="$PROJECT_ROOT" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_PYTHON_BINDINGS=ON \
      "$PROJECT_ROOT"

echo ""
echo "=== Building libraries ==="
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=== Installing libraries locally ==="
make install

echo ""
echo "=== Installation Summary ==="
echo "Libraries installed to:"
if [ -d "$PROJECT_ROOT/lib" ]; then
    ls -la "$PROJECT_ROOT/lib/"
else
    echo "Warning: lib directory not found at $PROJECT_ROOT/lib"
fi

echo ""
echo "Python module installed to:"
PYTHON_MODULE="$PROJECT_ROOT/build/python/lpximage.cpython-*.so"
if ls $PYTHON_MODULE 1> /dev/null 2>&1; then
    ls -la $PYTHON_MODULE
    # Copy to project root for easy access
    cp $PYTHON_MODULE "$PROJECT_ROOT/"
    echo "Python module copied to project root."
    
    # Also copy shared libraries to project root for Python module to find them
    if [ -d "$PROJECT_ROOT/lib" ]; then
        echo "Copying shared libraries to project root for Python module..."
        cp "$PROJECT_ROOT/lib/liblpx_image."*.dylib "$PROJECT_ROOT/" 2>/dev/null || \
        cp "$PROJECT_ROOT/lib/liblpx_image."*.so "$PROJECT_ROOT/" 2>/dev/null || echo "No shared libraries found"
    fi
else
    echo "Warning: Python module not found"
fi

echo ""
echo "=== Testing Installation ==="
cd "$PROJECT_ROOT"

# Test library loading
echo "Testing library loading..."
if [ -f "$PROJECT_ROOT/lib/liblpx_image.dylib" ] || [ -f "$PROJECT_ROOT/lib/liblpx_image.so" ]; then
    echo "✅ C++ library installed successfully"
else
    echo "❌ C++ library not found"
fi

# Test Python module
echo "Testing Python module..."
if PYTHONPATH="$PROJECT_ROOT" python3 -c "import lpximage; print('✅ Python module imported successfully')" 2>/dev/null; then
    echo "✅ Python module working"
else
    echo "❌ Python module not working"
fi

echo ""
echo "=== Installation Complete ==="
echo "To use the libraries:"
echo "1. C++ projects should link against: $PROJECT_ROOT/lib/liblpx_image.*"
echo "2. Python scripts should use: PYTHONPATH=$PROJECT_ROOT python3 your_script.py"
echo "3. Or run: export PYTHONPATH=$PROJECT_ROOT before running Python scripts"
