# LPXImage Local Installation Guide

This document describes how to build and install the LPXImage libraries locally for development and testing.

## Quick Start

The easiest way to build and install locally is to use the provided installation script:

```bash
./install_local.sh
```

This script will:
1. Clean any existing build directory
2. Configure CMake for local installation 
3. Build all libraries and Python bindings
4. Install C++ libraries to `./lib/`
5. Install Python module to project root
6. Copy required libraries to project root for Python module compatibility
7. Test the installation

## Manual Installation

If you prefer to install manually:

```bash
# Clean and create build directory
rm -rf build
mkdir build && cd build

# Configure for local installation
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/.. \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_PYTHON_BINDINGS=ON \
      ..

# Build
make -j$(nproc)

# Install locally
make install

# Copy libraries to project root for Python module
cd ..
cp lib/liblpx_image.*.dylib . 2>/dev/null || cp lib/liblpx_image.*.so . 2>/dev/null
cp build/python/lpximage.cpython-*.so .
```

## Usage After Installation

### C++ Projects
Link against the locally installed libraries:
```bash
g++ -I./include -L./lib -llpx_image your_program.cpp
```

### Python Scripts
Set PYTHONPATH to the project root:
```bash
PYTHONPATH=/path/to/LPXImage python3 your_script.py
```

Or export it for the session:
```bash
export PYTHONPATH=/path/to/LPXImage
python3 your_script.py
```

### Test the Installation

```python
import lpximage

# Test basic functionality
print("LPXImage version:", lpximage.getVersionString())
success = lpximage.initLPX('ScanTables63', 640, 480)
print("Initialization successful:", success)
```

## Directory Structure After Installation

```
LPXImage/
├── lib/                          # C++ libraries
│   ├── liblpx_image.1.0.0.dylib  # Main library
│   ├── liblpx_image.1.dylib      # Symlink
│   └── liblpx_image.dylib        # Symlink
├── include/                      # C++ headers
├── liblpx_image.*.dylib          # Libraries (copied for Python)
├── lpximage.cpython-*.so         # Python module
└── install_local.sh              # Installation script
```

## Key Features of Local Installation

1. **Self-contained**: All dependencies in project directory
2. **No system pollution**: No files installed to `/usr/local`
3. **Easy cleanup**: Just delete the project directory
4. **Development friendly**: Libraries and Python module updated together
5. **RPATH configured**: Libraries can find each other automatically

## Troubleshooting

### Python Module Can't Find Libraries
If you get `Library not loaded` errors, ensure:
1. The `.dylib` files are in the project root directory
2. PYTHONPATH is set correctly
3. Re-run `./install_local.sh` to refresh installation

### CMake Configuration Issues
If CMake can't find OpenCV:
```bash
# For Homebrew installations
export OpenCV_DIR=/opt/homebrew/lib/cmake/opencv4

# Then re-run installation
./install_local.sh
```

### Permission Errors
The installation script may warn about existing system files it cannot remove. This is normal and expected - the local installation will work correctly regardless.

## Testing Examples

After installation, test with:
```bash
# Test saccade mode
PYTHONPATH=. python3 test_saccade_mode.py

# Test color fix
PYTHONPATH=. python3 test_color_fix.py

# Test simple client
PYTHONPATH=. python3 test_client_simple.py
```
