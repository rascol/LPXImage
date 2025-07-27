# lpximage Python Module Documentation

## Overview

The `lpximage` Python module provides comprehensive interfaces for log-polar image transformation and related operations using the LPXImage C++ library. This module supports real-time video processing, webcam integration, file-based processing, and includes advanced features like frame synchronization for stable client-server communication.

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

# Render back to standard image
renderer = lpximage.LPXRenderer()
rendered = renderer.renderToImage(lpx_image, 640, 480, 1.0)
```

## Functions

### `initLPX(scanTableFile: str, width: int = 640, height: int = 480) -> bool`
Initializes the LPX system with specified scan tables.
- **Parameters**:
  - `scanTableFile`: Path to the scan tables file.
  - `width`: Width for initialization (default 640).
  - `height`: Height for initialization (default 480).
- **Returns**: `True` if initialization succeeded, otherwise `False`.

### `scanImage(image: np.ndarray, centerX: float, centerY: float) -> LPXImage`
Scans an image to create an LPXImage using multithreaded processing.
- **Parameters**:
  - `image`: Image to scan (numpy array).
  - `centerX`: X-coordinate for the center of processing.
  - `centerY`: Y-coordinate for the center of processing.
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
  - `getWidth() -> int`: Returns the image width.
  - `getHeight() -> int`: Returns the image height.
  - `getLength() -> int`: Returns the image length.
  - `getMaxCells() -> int`: Returns the maximum number of cells.
  - `getSpiralPeriod() -> int`: Returns the spiral period.
  - `getXOffset() -> int`: Returns the X offset.
  - `getYOffset() -> int`: Returns the Y offset.
  - `setPosition(x: int, y: int) -> None`: Sets the image position.
  - `saveToFile(filePath: str) -> bool`: Saves to file.
  - `loadFromFile(filePath: str) -> bool`: Loads from file.

### `LPXRenderer`
Handles the rendering of LPXImages back to standard images.
- **Methods**:
  - `setScanTables(tables: LPXTables) -> None`: Sets the scan tables.
  - `renderToImage(image: LPXImage, targetWidth: int, targetHeight: int, scale: float) -> np.ndarray`: Renders to an image.

### `LPXTables`
Represents the scan tables used in transformations.
- **Attributes**:
  - `spiralPer`: Spiral period.
  - `length`: Length of tables.
- **Methods**:
  - `isInitialized() -> bool`: Checks if tables are initialized.

### `FileLPXServer`
File-based LPX server.
- **Methods**:
  - `start(videoFile: str, width: int = 1920, height: int = 1080) -> bool`: Starts the server.
  - `stop() -> None`: Stops the server.
  - `setFPS(fps: int) -> None`: Sets the FPS.
  - `getFPS() -> int`: Gets the FPS.
  - `setLooping(loop: bool) -> None`: Sets looping.
  - `isLooping() -> bool`: Checks if looping.
  - `setCenterOffset(x: float, y: float) -> None`: Sets center offset.
  - `getClientCount() -> int`: Gets client count.

### `WebcamLPXServer`
Webcam-based LPX server.
- **Methods**:
  - `start(cameraId: int = 0, width: int = 640, height: int = 480) -> bool`: Starts the server.
  - `stop() -> None`: Stops the server.
  - `setSkipRate(skip: int, minRate: int, maxRate: float) -> None`: Sets skip rate.
  - `getClientCount() -> int`: Gets client count.

### `LPXDebugClient`
Debug client for LPX servers.
- **Methods**:
  - `connect(serverAddress: str) -> bool`: Connects to server.
  - `disconnect() -> None`: Disconnects from server.
  - `initializeWindow() -> None`: Initializes window.
  - `isRunning() -> bool`: Checks if running.
  - `processEvents() -> bool`: Processes events.
  - `sendMovementCommand(deltaX: float, deltaY: float, stepSize: float = 10.0) -> bool`: Sends movement command.
  - `setWindowTitle(title: str) -> None`: Sets window title.
  - `setWindowSize(width: int, height: int) -> None`: Sets window size.
  - `setScale(scale: float) -> None`: Sets scale.
