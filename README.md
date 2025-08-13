# Log-Polar Image

This is a cross-platform implementation of the log-polar image transformation ([LogPolarImages.md](./LogPolarImages.md)) for computer vision applications. This system provides a way to convert standard video streams to log-polar format and back, mimicking aspects of vertebrate vision. This is the first of several modules that will ultimately provide a log-polar vision front end to an LLM.

## Features

- Convert streaming video to log-polar format in real-time
- Stream processed images over network connections
- Convert log-polar images back to standard format for visualization
- High-performance multithreaded C++ core
- Cross-platform support (macOS, Linux, Windows)

## Requirements

- CMake 3.10+
- C++14 compatible compiler
- OpenCV 4.x
- Python 3.6+ (for Python integration)

## Installation

### Quick Start (Recommended)

Install the `lpximage` Python module directly from GitHub. This automatically downloads the source, compiles the C++ library, and installs the module:

**Linux (Ubuntu 22.04+):**
```bash
# Install system dependencies
sudo apt update
sudo apt install cmake libopencv-dev python3-dev build-essential

# Create virtual environment and install
python3 -m venv lpximage-env
source lpximage-env/bin/activate
pip install git+https://github.com/rascol/LPXImage.git
```

**macOS and Windows:**
```bash
pip install git+https://github.com/rascol/LPXImage.git
```

After installation, you can use the module in Python:
```python
import lpximage
server = lpximage.WebcamLPXServer("ScanTables63")
```

### Development Installation

For developers who want to modify the code:

```bash
# Clone the repository
git clone https://github.com/rascol/LPXImage.git
cd LPXImage

# Install system dependencies (Linux only)
sudo apt install cmake libopencv-dev python3-dev build-essential

# Install system dependencies (Mac only)
brew install cmake opencv python@3.13

# Create virtual environment and install in development mode
python3 -m venv lpximage-env
source lpximage-env/bin/activate
pip install -e .
```

### Local Installation (Self-Contained)

For a completely self-contained installation that doesn't affect your system:

```bash
# Clone the repository
git clone https://github.com/rascol/LPXImage.git
cd LPXImage

# Run the local installation script
./install_local.sh

# Use with PYTHONPATH
PYTHONPATH=. python3 your_script.py
```

This installs all libraries locally in the project directory. See [INSTALL_LOCAL.md](./INSTALL_LOCAL.md) for detailed information.

## Cross-Computer Streaming

LPXImage includes support for streaming log-polar processed video between different computers.

### Server Computer

```bash
# Option 1: Stream from webcam
python examples/lpx_server.py

# Option 2: Stream from video file
python examples/lpx_file_server.py --file path/to/video.mp4 --loop

# Options are available for camera selection and resolution
python examples/lpx_server.py --camera 0 --width 640 --height 480
```

### Client Computer

```bash
# Run the renderer (receives and displays LPXImage frames)
python examples/lpx_renderer.py --host SERVER_IP_ADDRESS

# Options are available for window size and scaling
python examples/lpx_renderer.py --host SERVER_IP_ADDRESS --width 800 --height 600 --scale 1.0
```

See the [Streaming Demo README](examples/README_STREAMING.md) for detailed instructions on cross-computer streaming.

## Cleaning and Uninstalling

To clean both build artifacts and installed files:

```bash
# For macOS/Linux
cd build
make cleanall

# For Windows
cd build
cmake --build . --target cleanall
```

To only uninstall without cleaning build artifacts:

```bash
cd build
make uninstall
```

## Usage from C++

```cpp
#include "lpx_webcam_server.h"
#include <iostream>

int main() {
    // Create the webcam server with scan tables
    lpx::WebcamLPXServer server("ScanTables63");
    
    // Start capturing from webcam (camera ID, width, height)
    if (!server.start(0, 640, 480)) {
        std::cerr << "Failed to start webcam server" << std::endl;
        return 1;
    }
    
    std::cout << "Server running. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    // Stop the server
    server.stop();
    
    return 0;
}
```

## Usage from Python

```python
import lpximage
import threading
import time
import cv2
import numpy as np

# Function to run the webcam server in a background thread
def run_server():
    # Create the webcam server
    server = lpximage.WebcamLPXServer("ScanTables63")
    
    # Start the server (camera ID, width, height)
    if not server.start(0, 640, 480):
        print("Failed to start server")
        return
    
    print("Server started. Processing video stream...")
    
    # Keep the server running for 60 seconds
    time.sleep(60)
    
    # Stop the server
    server.stop()
    print("Server stopped")

# Function to run the client that displays the log-polar processed stream
def run_client():
    # Create the client
    client = lpximage.LPXDebugClient("ScanTables63")
    
    # Configure the client
    client.setWindowTitle("Log-Polar Video Stream")
    client.setWindowSize(800, 600)
    client.setScale(1.0)
    
    # Initialize the display window (must be on main thread for macOS)
    client.initializeWindow()
    
    # Connect to the server
    if not client.connect("127.0.0.1"):
        print("Failed to connect to server")
        return
    
    print("Connected to server, receiving video stream...")
    
    # Process events on main thread
    while client.isRunning():
        # Process UI events and update display
        if not client.processEvents():
            break
        time.sleep(0.01)
    
    # Disconnect when done
    client.disconnect()

# Start the server in a background thread
server_thread = threading.Thread(target=run_server)
server_thread.daemon = True
server_thread.start()

# Give the server a moment to start
time.sleep(1)

# Run the client on the main thread
run_client()
```

For complete Python API documentation, see [python/README.md](python/README.md).

## Future Development

This LPXImage module focuses solely on the log-polar transformation of video streams. Future modules (like LPXVision) will build upon this foundation to integrate with neural networks and other advanced vision processing systems.

## Scan Tables

The system requires scan tables that define the mapping between standard image coordinates and log-polar coordinates. A default scan table file `ScanTables63` is provided, which works for most cases.

## License

Log-Polar Vision Software
Copyright (c) 2025 Raymond S. Connell, Jr.
 
This software is dual-licensed:
 - Free for non-commercial and open-source use
 - Commercial use requires a paid license
 
See LICENSE.md for full terms.
