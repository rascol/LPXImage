#!/bin/bash
# Make this script executable with: chmod +x fix_macos_paths.sh
# This script fixes the library paths in macOS Python module files
# Run after installation if you're experiencing library loading issues

# Check if Python is available
if ! command -v python &> /dev/null; then
    echo "Error: Python not found"
    exit 1
fi

# Get the site-packages directory
SITE_PACKAGES=$(python -c "import site; print(site.getsitepackages()[0])")
echo "Python site-packages directory: $SITE_PACKAGES"

# Find the module
MODULE=$(find "$SITE_PACKAGES" -name "lpximage*.so" -type f | head -1)
if [ -z "$MODULE" ]; then
    echo "Error: Could not find lpximage module in $SITE_PACKAGES"
    exit 1
fi
echo "Found module: $MODULE"

# Find the library
LIBRARY=$(find "$SITE_PACKAGES" -name "liblpx_image*.dylib" -type f | head -1)
if [ -z "$LIBRARY" ]; then
    # Look in the build directory as a fallback
    BUILD_DIR=$(dirname "$0")/../build
    LIBRARY=$(find "$BUILD_DIR" -name "liblpx_image*.dylib" -type f | head -1)
    
    if [ -z "$LIBRARY" ]; then
        echo "Error: Could not find liblpx_image dylib"
        exit 1
    fi
fi
echo "Found library: $LIBRARY"

# Get the library name
LIBRARY_NAME=$(basename "$LIBRARY")
echo "Library name: $LIBRARY_NAME"

# Fix the library install name in itself
echo "Fixing install name in the library..."
sudo install_name_tool -id "@loader_path/$LIBRARY_NAME" "$LIBRARY"

# Make sure the library is in the site-packages directory
if [[ "$LIBRARY" != "$SITE_PACKAGES"* ]]; then
    echo "Copying library to site-packages directory..."
    sudo cp "$LIBRARY" "$SITE_PACKAGES/"
    LIBRARY="$SITE_PACKAGES/$LIBRARY_NAME"
fi

# Fix the reference in the Python module
echo "Fixing reference in the Python module..."
sudo install_name_tool -change "@rpath/$LIBRARY_NAME" "@loader_path/$LIBRARY_NAME" "$MODULE"

# Check if it worked
echo "Checking if the fix worked..."
OTOOL_OUTPUT=$(otool -L "$MODULE")
echo "$OTOOL_OUTPUT"

# Check for @rpath references
if echo "$OTOOL_OUTPUT" | grep -q "@rpath/liblpx_image"; then
    echo "Warning: Module still contains @rpath references to the library"
    echo "You may need to manually fix the paths or use DYLD_LIBRARY_PATH"
else
    echo "Success! The module should now be able to find the library."
fi

echo "Done."
