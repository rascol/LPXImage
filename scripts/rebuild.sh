#!/bin/bash
# Script to perform a complete rebuild from scratch

# Change to the project root directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$SCRIPT_DIR/.."

# Remove the build directory if it exists
if [ -d "build" ]; then
    echo "Removing existing build directory..."
    rm -rf build
fi

# Create a fresh build directory
echo "Creating new build directory..."
mkdir -p build
cd build

# Configure and build the core C++ library first
echo "Configuring and building core C++ library..."
cmake .. -DBUILD_PYTHON_BINDINGS=OFF
make lpx_image

# Now configure and build everything including Python bindings
echo "Configuring and building Python bindings..."
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make

echo "Build completed!"
