# LPXImage Python API Documentation

This document provides detailed information about the Python API for the LPXImage library, which allows you to perform log-polar image transformations and stream processing in Python.

## Installation

See the main [INSTALL_PYTHON.md](../INSTALL_PYTHON.md) file for installation instructions.

## Core Components

### LPXTables

The `LPXTables` class handles the scan tables that define the mapping between standard image coordinates and log-polar coordinates.

```python
import lpximage

# Initialize scan tables
tables = lpximage.LPXTables("path/to/ScanTables63")

# Check if initialization was successful
if not tables.isInitialized():
    print("Failed to initialize scan tables")
    
# Access properties
print(f"Spiral period: {tables.spiralPer}")
print(f"Length: {tables.length}")
```

### LPXImage

The `LPXImage` class represents a log-polar transformed image.

```python
import lpximage

# Get an LPXImage from scanning a standard image
lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)

# Get properties
cell_count = lpx_image.getLength()
```

### LPXRenderer

The `LPXRenderer` class renders LPXImage objects back to standard images.

```python
import lpximage

# Initialize scan tables first
tables = lpximage.LPXTables("path/to/ScanTables63")

# Initialize renderer
renderer = lpximage.LPXRenderer()
renderer.setScanTables(tables)

# Render a log-polar image to a standard RGB image
# Parameters: lpx_image, width, height, scale
standard_img = renderer.renderToImage(lpx_image, 800, 600, 1.0)
```

## Streaming Components

### WebcamLPXServer

The `WebcamLPXServer` class captures video from a webcam, converts it to log-polar format, and streams it to clients.

```python
import lpximage

# Create server with scan tables
server = lpximage.WebcamLPXServer("path/to/ScanTables63")

# Start the server
# Parameters: camera_id, width, height
if not server.start(0, 640, 480):
    print("Failed to start server")
    
# Check client count
clients = server.getClientCount()

# Stop the server when done
server.stop()
```

### LPXDebugClient

The `LPXDebugClient` class receives log-polar image streams and displays them.

```python
import lpximage

# Create client with scan tables
client = lpximage.LPXDebugClient("path/to/ScanTables63")

# Configure display window
client.setWindowTitle("LPX Stream")
client.setWindowSize(800, 600)
client.setScale(1.0)

# Initialize window (must be done on main thread)
client.initializeWindow()

# Connect to a server
if not client.connect("127.0.0.1"):
    print("Failed to connect to server")
    
# Process events in a loop
while client.isRunning():
    if not client.processEvents():
        break
    
# Disconnect when done
client.disconnect()
```

## Utility Functions

### initLPX

Initializes the LPX system with scan tables and dimensions.

```python
import lpximage

# Initialize LPX system
if not lpximage.initLPX("path/to/ScanTables63", width, height):
    print("Failed to initialize LPX system")
```

### scanImage

Converts a standard image to log-polar format.

```python
import lpximage

# Convert BGR to RGB
img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

# Center of the image
center_x = img.shape[1] / 2
center_y = img.shape[0] / 2

# Scan the image to create an LPXImage
lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)
```

## Examples

### Basic Image Transformation

```python
import cv2
import numpy as np
import lpximage

# Load an image
img = cv2.imread("input.jpg")
if img is None:
    print("Failed to load image")
    exit(1)

# Convert BGR to RGB for processing
img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

# Initialize the LPX system
if not lpximage.initLPX("ScanTables63", img.shape[1], img.shape[0]):
    print("Failed to initialize LPX system")
    exit(1)

# Define center point (usually the center of the image)
center_x = img.shape[1] / 2
center_y = img.shape[0] / 2

# Convert to log-polar
lpx_image = lpximage.scanImage(img_rgb, center_x, center_y)
print(f"Created LPX image with {lpx_image.getLength()} cells")

# Initialize renderer
renderer = lpximage.LPXRenderer()
renderer.setScanTables(lpximage.LPXTables("ScanTables63"))

# Render back to standard format
rendered = renderer.renderToImage(lpx_image, 800, 600, 1.0)

# Convert back to BGR for display with OpenCV
result_bgr = cv2.cvtColor(rendered, cv2.COLOR_RGB2BGR)

# Display the result
cv2.imshow("Original", img)
cv2.imshow("Log-Polar Rendered", result_bgr)
cv2.waitKey(0)
cv2.destroyAllWindows()
```

### Streaming Video between Computers

See the example scripts in the examples directory:
- `lpx_server.py`: Runs on the server computer to capture and stream video
- `lpx_renderer.py`: Runs on the client computer to display the streamed video

These scripts demonstrate how to set up cross-computer streaming of log-polar processed video.

## Handling Errors

Most methods in the API return boolean values to indicate success or failure. Always check these return values:

```python
import lpximage

# Example of proper error handling
if not client.connect("127.0.0.1"):
    print("Failed to connect to server")
    # Handle the error appropriately
    return

# When starting the server
if not server.start(camera_id, width, height):
    print("Failed to start server")
    # Handle the error appropriately
    return
```

## Thread Safety

- The `WebcamLPXServer` class is designed to run in a background thread
- The `LPXDebugClient.processEvents()` method must be called from the main thread (especially on macOS)
- Always use proper thread synchronization when accessing shared resources

## Best Practices

1. Always check return values for functions that can fail
2. Initialize windows on the main thread when using `LPXDebugClient`
3. Use thread-safe patterns when working with the server in a multi-threaded context
4. Close resources properly when you're done with them
5. Add appropriate error handling for network operations

## Troubleshooting

### Module Import Issues

If you encounter `ModuleNotFoundError: No module named 'lpximage'`:

1. Verify the module was installed correctly
2. Check your Python paths
3. Try reinstalling the module
4. See the [INSTALL_PYTHON.md](../INSTALL_PYTHON.md) file for more details

### Connection Issues

If the client cannot connect to the server:

1. Verify the server is running
2. Check network connectivity between machines
3. Ensure firewalls are not blocking the connection
4. Try using the IP address instead of hostname

### Video Capture Issues

If the server fails to start:

1. Verify the camera is connected and working
2. Try a different camera ID
3. Check if the camera supports the requested resolution
4. Ensure no other application is using the camera

## Version Compatibility

This documentation is for lpximage version 0.1.0 and higher.
