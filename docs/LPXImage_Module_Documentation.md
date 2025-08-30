# lpximage Python Module Documentation

## Overview

The `lpximage` Python module provides comprehensive interfaces for log-polar image transformation and related operations using the LPXImage C++ library. This module supports real-time video processing, webcam integration, file-based processing, and includes an LPXVision extension that can provide visual input to an LLM.

## Installation

The `lpximage` module is automatically built and installed when building the LPXImage project:

```bash
cd LPXImage
mkdir build && cd build
cmake ..
make -j$(nproc)
cd ..
pip install -e . --break-system-packages
```

## Basic Usage

```python
import lpximage
import numpy as np
from PIL import Image

# Initialize the LPX system
if not lpximage.initLPX("path/to/scan_tables.bin"):
    raise RuntimeError("Failed to initialize LPX system")

# Load and process an image
image_array = np.array(Image.open("image.jpg"))
lpx_image = lpximage.scanImage(image_array, centerX=320, centerY=240)

# Create vision processing object
vision = lpximage.LPXVision()
vision.makeVisionCells(lpx_image)

# Access vision cell data and properties
print(f"Number of cell types: {vision.numCellTypes}")
print(f"Vision length: {vision.getViewLength()}")

# Render back to standard image
renderer = lpximage.LPXRenderer()
rendered = renderer.renderToImage(lpx_image, 640, 480, 1.0)
```

## Functions

### `initLPX(scanTableFile: str, width: int = 640, height: int = 480) -> bool`
Initializes the LPX system with specified LPXImage scan tables.
- **Parameters**:
  - `scanTableFile`: Path to the scan tables file.
  - `width`: Width for initialization (default 640).
  - `height`: Height for initialization (default 480).
- **Returns**: `True` if initialization succeeded, otherwise `False`.

### `scanImage(image: np.ndarray, centerX: float, centerY: float) -> LPXImage`
Scans a standard image to create an LPXImage using multithreaded processing.
- **Parameters**:
  - `image`: Standard image to scan (numpy array).
  - `centerX`: center-relative X offset (in pixels on the standard image) of the scan location.
  - `centerY`: center-relative Y offset (in pixels on the standard image) of the scan location.
- **Returns**: An instance of `LPXImage`.

### `getVersionString() -> str`
Returns a string with the version and build timestamp.

### `getBuildDate() -> str`
Returns the build date.

### `getBuildTime() -> str`
Returns the build time.

### `getBuildTimestamp() -> str`
Returns the full build timestamp.

### `getBuildNumber() -> int`
Returns the build number.

### `getKeyThrottleMs() -> int`
Returns the key throttle time in milliseconds.

### `printBuildInfo() -> None`
Prints the build information to the console.

## Classes

### `LPXImage`
Represents a log-polar transformed image.
- **Methods**:
  - `getWidth() -> int`: Returns the image width in standard image pixels.
  - `getHeight() -> int`: Returns the image height in standard image pixels.
  - `getLength() -> int`: Returns the image length in LPXImage cells.
  - `getMaxCells() -> int`: Returns the maximum number of number of LPXImage cells.
  - `getSpiralPeriod() -> int`: Returns the spiral period in LPXImage cells.
  - `getXOffset() -> int`: Returns the center-relative X offset of the scan location on the standard image.
  - `getYOffset() -> int`: Returns the center-relative Y offset of the scan location on the standard image.
  - `setPosition(x: int, y: int) -> None`: Sets the center-relative scan location on the standard image.
  - `saveToFile(filePath: str) -> bool`: Saves to file.
  - `loadFromFile(filePath: str) -> bool`: Loads from file.

### `LPXRenderer`
Handles the rendering of LPXImages back to standard images.
- **Methods**:
  - `setScanTables(tables: LPXTables) -> None`: Sets the scan tables object by name.
  - `renderToImage(image: LPXImage, targetWidth: int, targetHeight: int, scale: float) -> np.ndarray`: Renders to a standard image.

### `LPXTables`
Represents the LPXImage scan tables object used in transformations.
- **Attributes**:
  - `spiralPer`: Spiral period in number of LPXImage cells.
  - `length`: Length of tables.
- **Methods**:
  - `isInitialized() -> bool`: Checks if tables are initialized.

### `FileLPXServer`
File-based LPX server.
- **Methods**:
  - `start(videoFile: str, width: int = 1920, height: int = 1080) -> bool`: Starts the server.
  - `stop() -> None`: Stops the server.
  - `setFPS(fps: int) -> None`: Sets the video frames per second.
  - `getFPS() -> int`: Gets the video frames per second.
  - `setLooping(loop: bool) -> None`: Sets video looping.
  - `isLooping() -> bool`: Checks if looping is active.
  - `setCenterOffset(x: float, y: float) -> None`: Sets center-relative x, y offsets of the LPXImage scan on the standard image while images are being streamed.
  - `getClientCount() -> int`: Gets count of active clients.

### `WebcamLPXServer`
Webcam-based LPX server.
- **Methods**:
  - `start(cameraId: int = 0, width: int = 640, height: int = 480) -> bool`: Starts the server.
  - `stop() -> None`: Stops the server.
  - `setSkipRate(skip: int, minRate: int, maxRate: float) -> None`: Sets video frame skip rate.
  - `setCenterOffset(x: float, y: float) -> None`: Sets center-relative x, y offsets of the LPXImage scan on the standard image while images are being streamed.
  - `getClientCount() -> int`: Gets client count.

### `LPXDebugClient`
This class renders and displays an LPXImage video stream as a standard video stream.
- **Methods**:
  - `connect(serverAddress: str) -> bool`: Connects to server.
  - `disconnect() -> None`: Disconnects from server.
  - `initializeWindow() -> None`: Initializes display window.
  - `isRunning() -> bool`: Checks if running.
  - `processEvents() -> bool`: Processes events.
  - `sendMovementCommand(deltaX: float, deltaY: float, stepSize: float = 10.0) -> bool`: Sends movement command.
  - `setWindowTitle(title: str) -> None`: Sets display window title.
  - `setWindowSize(width: int, height: int) -> None`: Sets display window size in pixels.
  - `setScale(scale: float) -> None`: Sets scale.

### `LPXVision`
Provides vision processing capabilities on top of LPXImage data.
- **Constructor**:
  - `__init__(lpxImage: LPXImage = None)`: Creates an LPXVision object, optionally linked to an LPXImage.
- **Attributes**:
  - `spiralPer`: Spiral period value.
  - `startIndex`: Start index of the vision cells.
  - `startPer`: Start percentage of the spiral.
  - `tilt`: Tilt value for cell orientation.
  - `length`: Total length of all cell types.
  - `viewlength`: Length of the current view.
  - `viewIndex`: Current view index.
  - `x_ofs`: X offset for positioning.
  - `y_ofs`: Y offset for positioning.
  - `numCellTypes`: Number of different cell types.
  - `retinaCells`: Array of retina cell values.
- **Methods**:
  - `getCellIdentifierName(i: int) -> str`: Get the string name of the LPXVision cell identifiers.
  - `getViewStartIndex() -> int`: Get the index into the vision cell buffers of the start of the view range.
  - `getViewLength(spiralPer: float = 0.0) -> int`: Get the total number of LPXVision cell locations in the view range.
  - `makeVisionCells(lpImage: LPXImage, lpD: LPXVision = None) -> None`: Construct LPXVision cells from an LPXImage object.

### `vision_utils`
A submodule of utility functions for vision processing.
- **Functions**:
  - `convertImageFormat(input: np.ndarray, output: np.ndarray, format: int) -> None`: Convert image format using OpenCV.
  - `resizeImageKeepAspect(input: np.ndarray, output: np.ndarray, maxWidth: int, maxHeight: int) -> None`: Resize image while maintaining aspect ratio.
  - `getTimestamp() -> str`: Get current timestamp as string.
  - `logMessage(message: str) -> None`: Log a message with timestamp.
