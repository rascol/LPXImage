#!/bin/bash
# Script to perform a complete cleanup of build artifacts and installed files

# Change to the project root directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$SCRIPT_DIR/.."

# Check if build directory exists
if [ -d "build" ]; then
    echo "Cleaning build directory..."
    cd build
    
    # Run uninstall if the target exists
    if [ -f "cmake_uninstall.cmake" ]; then
        echo "Running uninstall..."
        cmake -P cmake_uninstall.cmake
    else
        echo "Uninstall script not found. Will only clean build artifacts."
    fi
    
    # Run clean
    echo "Running make clean..."
    make clean
    
    # Go back to project root
    cd ..
else
    echo "Build directory not found. Nothing to clean."
fi

# Additional cleanup for common install locations
echo "Checking for installed files in common locations..."

# Library files
COMMON_LIB_PATHS=(
    "/usr/local/lib"
    "/usr/lib"
    "$HOME/.local/lib"
)

# Include files
COMMON_INCLUDE_PATHS=(
    "/usr/local/include"
    "/usr/include" 
    "$HOME/.local/include"
)

# Python module paths
PYTHON_PATHS=()
if command -v python3 >/dev/null 2>&1; then
    PYTHON_PATHS+=($(python3 -c "import site; print(site.getsitepackages()[0])"))
    PYTHON_PATHS+=($(python3 -c "import site; print(site.getusersitepackages())"))
fi

# Check for library files
for path in "${COMMON_LIB_PATHS[@]}"; do
    if [ -f "$path/liblpx_image.dylib" ] || [ -f "$path/liblpx_image.so" ]; then
        echo "Found library in $path"
        if [ -w "$path" ]; then
            echo "Removing liblpx_image* from $path"
            sudo rm -f "$path/liblpx_image"*
        else
            echo "Need sudo permissions to remove from $path"
            sudo rm -f "$path/liblpx_image"*
        fi
    fi
done

# Check for include files
for path in "${COMMON_INCLUDE_PATHS[@]}"; do
    if [ -f "$path/lpx_image.h" ]; then
        echo "Found header files in $path"
        if [ -w "$path" ]; then
            echo "Removing lpx_*.h from $path"
            rm -f "$path/lpx_"*.h
        else
            echo "Need sudo permissions to remove from $path"
            sudo rm -f "$path/lpx_"*.h
        fi
    fi
done

# Check for Python module
for path in "${PYTHON_PATHS[@]}"; do
    if [ -d "$path/lpximage" ] || [ -f "$path/lpximage.so" ]; then
        echo "Found Python module in $path"
        if [ -w "$path" ]; then
            echo "Removing lpximage* from $path"
            rm -rf "$path/lpximage"*
        else
            echo "Need sudo permissions to remove from $path"
            sudo rm -rf "$path/lpximage"*
        fi
    fi
done

echo "Cleanup complete!"
