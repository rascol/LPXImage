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

## Building and Installing

### macOS and Linux

1. First, clone the LPXImage repository if you haven't already:

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
make
```

4. Install the Python module (may require administrator privileges):

```bash
make install
```

   Or, to install for the current user only:

```bash
make install DESTDIR=~/.local
```

### Windows

1. First, clone the LPXImage repository if you haven't already:

```batch
git clone https://github.com/rascol/LPXImage.git
cd LPXImage
```

2. Create a build directory and run CMake:

```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_PYTHON_BINDINGS=ON
```

3. Build the library and Python module:

```batch
cmake --build . --config Release
```

4. Install the Python module:

```batch
cmake --install . --config Release
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

## Verifying the Installation

To verify that the Python module was correctly installed, run Python and try to import it:

```python
import lpximage
print(lpximage.__doc__)
```

You should see the documentation string: "Python bindings for LPX Image Processing Library"

## Running the Example

The LPXImage package includes an example script that demonstrates how to use the library. To run it:

```bash
cd examples
python lpx_demo.py
```

Make sure you have a sample image named "sample_image.jpg" in the current directory, or modify the script to use a different image.

## Troubleshooting

### Module Not Found

If Python cannot find the module, you may need to add the installation directory to your Python path:

```bash
export PYTHONPATH=$PYTHONPATH:/usr/local/lib/python3.x/site-packages
```

Replace "python3.x" with your Python version.

### Missing Dependencies

If you encounter errors about missing libraries when importing the module, make sure all dependencies (especially OpenCV) are correctly installed and in your library path.

### Build Errors

If you encounter build errors related to pybind11, try installing it manually:

```bash
pip install pybind11
```

Then modify the CMakeLists.txt in the python directory to use the installed version:

```cmake
find_package(pybind11 REQUIRED)
```

## Using with Virtual Environments

It's often best to use Python virtual environments to avoid conflicts between different projects:

```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install opencv-python numpy
# Build and install as above
```

## Next Steps

After installation, refer to the Python API documentation in `python/README.md` for detailed information on how to use the library from Python.

For integration with other Python libraries and custom applications, refer to the Python API documentation in `python/README.md`.
