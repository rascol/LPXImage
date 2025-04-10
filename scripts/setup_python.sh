#!/bin/bash
# Setup script for LPXImage Python bindings

# Create directories if they don't exist
mkdir -p python/pybind11

# Clone pybind11 (alternative to FetchContent in CMake)
echo "Downloading pybind11..."
git clone https://github.com/pybind/pybind11.git python/pybind11 --depth 1 --branch v2.12.0

echo "Setup complete!"
echo "Now you can build the project with Python bindings:"
echo "  mkdir -p build"
echo "  cd build"
echo "  cmake .. -DBUILD_PYTHON_BINDINGS=ON"
echo "  make"
