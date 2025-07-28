#!/bin/bash
# clean_all_lpx.sh - Comprehensive cleanup of all LPX library artifacts
# This script removes all LPX-related files from the system to prevent conflicts

set -e

echo "🧹 Starting comprehensive LPX library cleanup..."

# Function to safely remove files with sudo if needed
safe_remove() {
    local path="$1"
    if [ -e "$path" ]; then
        echo "  🗑️  Removing: $path"
        if [ -w "$(dirname "$path")" ]; then
            rm -rf "$path"
        else
            sudo rm -rf "$path"
        fi
        echo "     ✅ Removed successfully"
    else
        echo "     ℹ️  Not found (OK): $path"
    fi
}

# 1. Remove system-wide C++ libraries and headers
echo ""
echo "=== Cleaning system-wide C++ libraries ==="
safe_remove "/usr/local/lib/liblpx_image.1.0.0.dylib"
safe_remove "/usr/local/lib/liblpx_image.1.dylib"
safe_remove "/usr/local/lib/liblpx_image.dylib"
safe_remove "/usr/local/include/lpx_image.h"
safe_remove "/usr/local/include/lpx_renderer.h"
safe_remove "/usr/local/include/lpx_mt.h"
safe_remove "/usr/local/include/lpx_webcam_server.h"
safe_remove "/usr/local/include/lpx_file_server.h"

# 2. Remove Homebrew libraries (both regular and Python site-packages)
echo ""
echo "=== Cleaning Homebrew libraries ==="
safe_remove "/opt/homebrew/lib/liblpx_image.1.0.0.dylib"
safe_remove "/opt/homebrew/lib/liblpx_image.1.dylib"
safe_remove "/opt/homebrew/lib/liblpx_image.dylib"

# 3. Remove ALL Python versions (3.11, 3.12, 3.13, etc.)
echo ""
echo "=== Cleaning Python libraries (all versions) ==="
for python_dir in /opt/homebrew/lib/python*/site-packages; do
    if [ -d "$python_dir" ]; then
        echo "Checking Python directory: $python_dir"
        
        # Remove main library files
        safe_remove "$python_dir/lpximage.cpython-*.so"
        safe_remove "$python_dir/liblpx_image.1.0.0.dylib"
        safe_remove "$python_dir/liblpx_image.1.dylib"
        safe_remove "$python_dir/liblpx_image.dylib"
        
        # Remove editable install artifacts (development installs)
        safe_remove "$python_dir/__editable__.lpximage-*.pth"
        safe_remove "$python_dir/__editable___lpximage_*_finder.py"
        safe_remove "$python_dir/lpximage-*.dist-info"
        
        # Remove pycache for editable installs
        safe_remove "$python_dir/__pycache__/__editable___lpximage_*"
    fi
done

# 4. Remove user-specific Python libraries
echo ""
echo "=== Cleaning user Python libraries ==="
for user_python_dir in /Users/ray/Library/Python/*/lib/python/site-packages; do
    if [ -d "$user_python_dir" ]; then
        echo "Checking user Python directory: $user_python_dir"
        safe_remove "$user_python_dir/lpximage.cpython-*.so"
        safe_remove "$user_python_dir/liblpx_image.*"
        safe_remove "$user_python_dir/lpximage-*.dist-info"
    fi
done

# 5. Clear Python import caches
echo ""
echo "=== Clearing Python caches ==="
echo "  🧹 Clearing Python import caches..."
python3 -c "import sys; print('Python executable:', sys.executable)"
python3 -c "
import sys
import importlib.util
# Clear any cached modules
for module_name in list(sys.modules.keys()):
    if 'lpx' in module_name.lower():
        print(f'Removing cached module: {module_name}')
        del sys.modules[module_name]
"

# 6. Check for any remaining LPX files
echo ""
echo "=== Verification: Checking for remaining LPX files ==="
remaining_files=$(find /usr/local /opt/homebrew /Users/ray/Library/Python -name "*lpx*" -o -name "*LPX*" 2>/dev/null | grep -v "Dropbox\|Firefox\|lpxScanner.cpp" || true)

if [ -z "$remaining_files" ]; then
    echo "✅ All LPX libraries have been successfully removed!"
else
    echo "⚠️  Some LPX files still remain:"
    echo "$remaining_files"
    echo ""
    echo "These may need manual cleanup or could be unrelated files."
fi

echo ""
echo "🎉 LPX library cleanup completed!"
echo ""
echo "Next steps:"
echo "1. Run a fresh build: cd build && cmake .. && make -j4"
echo "2. Install: sudo make install"
echo "3. Test the installation"
