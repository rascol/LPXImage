# LPXImage Python API

This document provides documentation for the Python bindings to the LPXImage C++ library. These bindings allow you to use the high-performance log-polar image processing capabilities of LPXImage directly from Python.

## Installation

To build and install the Python bindings:

```bash
# From the LPXImage root directory
mkdir -p build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make
make install  # May require sudo depending on your Python installation
```

## Quick Start

```python
import numpy as np
import cv2
import lpximage

# Load an image
img = cv2.imread("sample_image.jpg")
img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

# Initialize scan tables
tables = lpximage.LPXTables("path/to/ScanTables63")
if tables.isInitialized():
    print(f"Spiral period: {tables.spiralPer}")

# Process the image using LPX
center_x = img.shape[1] / 2
center_y = img.shape[0] / 2
lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)

# Render the image back to standard format
renderer = lpximage.LPXRenderer()
renderer.setScanTables(tables)
rendered = renderer.renderToImage(lpx_image, 800, 600, 1.0)

# Display the result
cv2.imshow("LPX Rendered", cv2.cvtColor(rendered, cv2.COLOR_RGB2BGR))
cv2.waitKey(0)
```

## API Reference

### LPXTables

Represents the scan tables that define the log-polar mapping.

#### Constructor

```python
tables = lpximage.LPXTables(scan_table_file)
```

- `scan_table_file`: Path to the scan tables file

#### Methods

- `isInitialized()`: Returns True if the tables were successfully initialized
- `printInfo()`: Prints detailed information about the scan tables

#### Properties

- `spiralPer`: The spiral period in cells per revolution
- `length`: Length of the scan table arrays

### LPXImage

Represents an image in log-polar format.

#### Constructor

```python
image = lpximage.LPXImage(tables, width, height)
```

- `tables`: A shared pointer to an LPXTables object
- `width`: The width of the original image
- `height`: The height of the original image

#### Methods

- `getWidth()`: Get the width of the original image
- `getHeight()`: Get the height of the original image
- `getLength()`: Get the number of cells in the log-polar representation
- `getMaxCells()`: Get the maximum number of cells allowed
- `getSpiralPeriod()`: Get the spiral period
- `getXOffset()`: Get the X offset of the center point
- `getYOffset()`: Get the Y offset of the center point
- `setPosition(x, y)`: Set the position of the center point
- `saveToFile(filename)`: Save the LPXImage to a file
- `loadFromFile(filename)`: Load an LPXImage from a file

### LPXRenderer

Renders LPXImage objects back to standard image format.

#### Constructor

```python
renderer = lpximage.LPXRenderer()
```

#### Methods

- `setScanTables(tables)`: Set the scan tables to use for rendering
- `renderToImage(lpx_image, width, height, scale)`: Render an LPXImage to a numpy array
  - `lpx_image`: The LPXImage to render
  - `width`: The width of the output image
  - `height`: The height of the output image
  - `scale`: Scale factor for rendering
  - Returns: A numpy array containing the rendered image

### Global Functions

- `scanImage(image, centerX, centerY)`: Convert a standard image to an LPXImage
  - `image`: A numpy array containing the image (RGB format)
  - `centerX`: X coordinate of the center point
  - `centerY`: Y coordinate of the center point
  - Returns: An LPXImage object

### WebcamLPXServer

A server that captures webcam images and converts them to LPXImage format for clients.

#### Constructor

```python
server = lpximage.WebcamLPXServer(scan_table_file, port=5050)
```

- `scan_table_file`: Path to the scan tables file
- `port`: Port to listen on (default: 5050)

#### Methods

- `start(cameraId=0, width=640, height=480)`: Start the server
  - `cameraId`: ID of the camera to use
  - `width`: Width of the captured frames
  - `height`: Height of the captured frames
- `stop()`: Stop the server
- `setSkipRate(min, max, motionThreshold=5.0)`: Set the frame skip rate
  - `min`: Minimum skip rate
  - `max`: Maximum skip rate
  - `motionThreshold`: Motion threshold for adaptive skipping
- `getClientCount()`: Get the number of connected clients

### LPXDebugClient

A client that connects to a WebcamLPXServer and displays the received images.

#### Constructor

```python
client = lpximage.LPXDebugClient(scan_table_file)
```

- `scan_table_file`: Path to the scan tables file

#### Methods

- `connect(serverAddress, port=5050)`: Connect to a server
  - `serverAddress`: Address of the server
  - `port`: Port to connect to
- `disconnect()`: Disconnect from the server
- `setWindowTitle(title)`: Set the window title
- `setWindowSize(width, height)`: Set the window size
- `setScale(scale)`: Set the rendering scale
- `initializeWindow()`: Initialize the display window (must be called from main thread)
- `processEvents()`: Process UI events (must be called from main thread)
- `isRunning()`: Check if the client is still running

## Performance Tips

1. Use the `scanImage` function for best performance, as it leverages the multithreaded C++ implementation.
2. Reuse LPXTables and LPXRenderer objects to avoid initialization overhead.
3. For real-time applications, consider using the WebcamLPXServer/LPXDebugClient approach to offload processing to a separate thread or machine.

When using the LPXDebugClient in Python, it's important to ensure all UI operations occur on the main thread. Here's the recommended pattern:

```python
import lpximage
import threading
import time

def run_server():
    server = lpximage.WebcamLPXServer("path/to/ScanTables63")
    server.start()
    # Keep the server running
    while True:
        time.sleep(1)
        if some_condition:
            break
    server.stop()

# Start server in a background thread
server_thread = threading.Thread(target=run_server)
server_thread.daemon = True
server_thread.start()

# Client on main thread
client = lpximage.LPXDebugClient("path/to/ScanTables63")
client.setWindowSize(800, 600)
client.initializeWindow()  # Must be on main thread
client.connect("localhost")

# Event loop on main thread
while client.isRunning():
    client.processEvents()  # Must be on main thread
    time.sleep(0.01)

client.disconnect()
```

## Performance Tips

1. Use the `scanImage` function for best performance, as it leverages the multithreaded C++ implementation.
2. Reuse LPXTables and LPXRenderer objects to avoid initialization overhead.
3. For real-time applications, consider using the WebcamLPXServer/LPXDebugClient approach to offload processing to a separate thread or machine.

## Troubleshooting

Common issues and solutions:

- **ImportError: No module named lpximage**: Make sure the module is installed in your Python path or set the PYTHONPATH environment variable.
- **Missing ScanTables**: Ensure the ScanTables63 directory is correctly specified.
- **UI Thread Issues**: Remember that all OpenCV window operations must be performed on the main thread.
