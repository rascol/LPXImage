#!/bin/bash
# Script to perform a complete cleanup of build artifacts and installed files

# Change to the project root directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$SCRIPT_DIR/.."

# Check if build directory exists
if [ -d "build" ]; then
    echo "Cleaning everything (build artifacts and installed files)..."
    cd build
    
    # Run make clean, which now also handles uninstallation
    make clean
    
    # Go back to project root
    cd ..
else
    echo "Build directory not found. Creating a new build directory to run clean."
    mkdir -p build
    cd build
    cmake ..
    make clean
    cd ..
fi

echo "Cleanup complete!"
