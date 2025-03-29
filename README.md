# Log-Polar Vision
This is strictly experimental and under development. The conversions work only for single images at present. No streaming yet. It is a cross-platform implementation of log-polar image transformation for computer vision applications. This system provides a way to convert standard video streams to log-polar format and back, mimicking aspects of vertebrate vision.

## Features

- Convert standard images/video to log-polar format
- Convert log-polar images back to standard format for visualization
- Stream processed images via WebSocket
- Cross-platform support (macOS, Linux, Windows)
- Node.js native addon for integration with web applications

## Requirements

- CMake 3.10+
- C++14 compatible compiler
- OpenCV 4.x
- Node.js 14+ (for Node.js integration)

## Building the C++ Library

```bash
mkdir build
cd build
cmake ..
make
```

## Installing the Node.js Module

```bash
npm install
```

## Running the Web Server

```bash
node server.js
```

Then open your browser to http://localhost:3000 to see the log-polar stream.

## Usage from C++

```cpp
#include "lpx_image.h"
#include <opencv2/opencv.hpp>

int main() {
    // Initialize
    lpx::initLPX("ScanTables63", 640, 480);
    
    // Load an image
    cv::Mat image = cv::imread("input.jpg");
    
    // Convert to log-polar
    auto lpxImage = lpx::scanImage(image, image.cols/2, image.rows/2);
    
    // Save
    lpxImage->saveToFile("output.lpx");
    
    // Render for visualization
    cv::Mat rendered = lpxImage->renderToImage(640, 480);
    cv::imwrite("rendered.jpg", rendered);
    
    // Cleanup
    lpx::shutdownLPX();
    return 0;
}
```

## Usage from Node.js

```javascript
const lpx = require('log-polar-vision');

// Initialize
lpx.initialize('ScanTables63');

// Start camera
lpx.startCamera(0);

// Capture and convert frame
const lpxData = lpx.captureFrame(0.5, 0.5);

// Save to file
lpx.saveLogPolarData(lpxData, 'output.lpx');

// Render for visualization
const jpegData = lpx.renderLogPolarImage(lpxData, 640, 480);

// Clean up
lpx.stopCamera();
lpx.shutdown();
```

## Scan Tables

The system requires scan tables that define the mapping between standard image coordinates and log-polar coordinates. A default scan table file `ScanTables63` is provided, which works for most cases.

## License

Log-Polar Vision Software
Copyright (c) 2025 Raymond S. Connell, Jr.
 
This software is dual-licensed:
 - Free for non-commercial and open-source use
 - Commercial use requires a paid license
 
The log-polar algorithms are patent pending.
 
See LICENSE.md for full terms.
