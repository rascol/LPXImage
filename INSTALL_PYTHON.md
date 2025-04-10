# Installing Python Bindings for LPXImage

This guide explains how to build and install the Python bindings for the LPXImage library, allowing you to use the high-performance log-polar image processing capabilities directly from Python.

## Prerequisites

Before building the Python bindings, make sure you have the following installed:

- CMake (version 3.10 or higher)
- A C++ compiler with C++14 support
- Python 3.6 or higher with development headers
- OpenCV 4.x with development headers
- Git (for downloading pybind11)

### macOS

On macOS, you can install the prerequisites using Homebrew:

```bash
brew install cmake
brew install opencv
brew install python
```

### Windows

On Windows, you'll need:

```batch
Visual Studio 2019 or 2022 with C++ Development Workload
cmake (you can install it via Visual Studio or download from cmake.org)
pip install opencv-python
pip install pybind11
```

### Linux (Ubuntu/Debian)

On Ubuntu or Debian-based distributions:

```bash
sudo apt update
sudo apt install cmake
sudo apt install libopencv-dev
sudo apt install python3-dev
sudo apt install git
```

## Python Dependencies

Install the required Python packages:

```bash
pip install -r requirements.txt
```

## Building and Installing

### Simple Build Process (All Platforms)

The simplified build process should work for most users:

1. Clone the repository:
```bash
git clone https://github.com/rascol/LPXImage.git
cd LPXImage
```

2. Create a build directory and run CMake:
```bash
mkdir -p build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
```

3. Build the library and Python module:
```bash
# macOS/Linux
make

# Windows
cmake --build . --config Release
```

4. Install the Python module:
```bash
# macOS/Linux
sudo make install

# Windows
cmake --install . --config Release
```

5. Verify the installation:
```bash
python -c "import lpximage; print('Module successfully imported!')"
```

### Platform-Specific Options

#### macOS Advanced Options

For macOS, you may want to use additional options to ensure proper library loading:

```bash
cmake .. -DBUILD_PYTHON_BINDINGS=ON \
         -DPython_EXECUTABLE=$(which python) \
         -DCMAKE_INSTALL_RPATH="/usr/local/lib" \
         -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
         -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
         -DCMAKE_MACOSX_RPATH=ON
```

#### Virtual Environment Installation

When using a Python virtual environment, you may want to install without sudo:

```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt

mkdir -p build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON -DPython_EXECUTABLE=$(which python)
make
make install  # No sudo needed for virtual environments
```

## Uninstalling and Cleaning

To clean both build artifacts and installed files, use the `cleanall` target:

```bash
# For macOS/Linux
cd build
make cleanall

# For Windows
cd build
cmake --build . --target cleanall
```

To just remove build artifacts without uninstalling:

```bash
# Standard clean command
cd build
make clean
```

If you only want to uninstall without cleaning build artifacts:

```bash
# For macOS/Linux
cd build
make uninstall

# For Windows
cd build
cmake --build . --target uninstall
```

## Troubleshooting

### Library Loading Issues on macOS

If you encounter errors about missing libraries when importing the module, such as:

```
ImportError: dlopen(..., 0x0002): Library not loaded: @rpath/liblpx_image.1.dylib
```

There are several ways to fix this:

1. **Use the fix_macos_paths.sh script** (Recommended)
   ```bash
   cd scripts
   chmod +x fix_macos_paths.sh
   ./fix_macos_paths.sh
   ```
   This script will automatically find and fix the library paths.

2. **Set the DYLD_LIBRARY_PATH environment variable**
   ```bash
   # Find the library location
   find /Users/ray/Desktop/LPXImage -name "liblpx_image*.dylib"
   
   # Set the environment variable
   export DYLD_LIBRARY_PATH=/path/to/directory/containing/library:$DYLD_LIBRARY_PATH
   ```

3. **Rebuild with updated CMake settings**
   ```bash
   cd build
   cmake .. -DBUILD_PYTHON_BINDINGS=ON -DPython_EXECUTABLE=$(which python)
   make
   sudo make install
   ```
   The updated CMakeLists.txt should now properly configure the library paths.

### Module Not Found Error

If you encounter `ModuleNotFoundError: No module named 'lpximage'` after installation:

1. Verify the module was installed by finding it:
   ```bash
   # macOS/Linux
   find /usr /usr/local /opt -name "lpximage*.so" 2>/dev/null
   
   # Windows
   dir /s C:\Python*\Lib\site-packages\lpximage*
   ```

2. Check your Python paths:
   ```python
   import sys
   print(sys.path)
   ```

3. Temporarily add the directory containing the module to your Python path:
   ```bash
   # macOS/Linux
   export PYTHONPATH=$PYTHONPATH:/path/to/directory/containing/module
   
   # Windows
   set PYTHONPATH=%PYTHONPATH%;C:\path\to\directory\containing\module
   ```

### Build Errors

1. **Missing pybind11**: If you encounter errors related to pybind11, try installing it manually:
   ```bash
   pip install pybind11
   ```

2. **OpenCV Not Found**: Ensure OpenCV is installed with development headers:
   ```bash
   # macOS
   brew install opencv
   
   # Ubuntu/Debian
   sudo apt install libopencv-dev
   ```

3. **Incorrect Python Version**: Make sure CMake is using the intended Python version:
   ```bash
   cmake .. -DBUILD_PYTHON_BINDINGS=ON -DPython_EXECUTABLE=$(which python)
   ```

### Missing Dependencies at Runtime

If you encounter errors about missing libraries when importing the module:

1. Check if all dependencies are installed:
   ```bash
   # macOS
   otool -L /path/to/lpximage.so
   
   # Linux
   ldd /path/to/lpximage.so
   
   # Windows
   dumpbin /dependents C:\path\to\lpximage.pyd
   ```

2. Install any missing libraries and ensure they're in your library path

## Next Steps

After installation, try running the example scripts in the `examples` directory:

```bash
cd examples
python lpx_demo.py
```

For more advanced usage, try the streaming examples:

```bash
# On the server computer
python lpx_server.py

# On the client computer
python lpx_renderer.py --host SERVER_IP_ADDRESS
```

Refer to the Python API documentation in `python/README.md` for detailed information on how to use the library.
