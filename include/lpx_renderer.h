/**
 * lpx_renderer.h
 * 
 * Header file for Log-Polar Image rendering
 * Based on serverLPXRenderer.js from the original project
 */

#ifndef LPX_RENDERER_H
#define LPX_RENDERER_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include "lpx_common.h"  // Include common definitions first
#include "lpx_image.h"

namespace lpx {

// Constants and functions are now in lpx_common.h
const float FOUR_PI = 2.0f * TWO_PI;  // Only keeping this one as it's not in common.h

// Class for rendering log-polar images back to standard format
class LPXRenderer {
public:
    LPXRenderer();
    ~LPXRenderer();

    // Set scan tables for a specific spiral period
    bool setScanTables(const std::shared_ptr<LPXTables>& tables);
    
    // Check if we have scan tables for a specific spiral period
    bool hasScanTables(float spiralPer) const;
    
    // Render a log-polar image to a standard image
    cv::Mat renderToImage(const std::shared_ptr<LPXImage>& lpxImage, int width, int height, 
                         float scale = 1.0f, int cellOffset = 0, int cellRange = 0);
    
    // Get the bounding box for scanning
    Rect getScanBoundingBox(const std::shared_ptr<LPXImage>& lpxImage, int width, int height, float scaleFactor);
    
    // Extract RGB values from a log-polar cell
    void getRGBFromLPCell(uint32_t lpCell, uint8_t& r, uint8_t& g, uint8_t& b);

private:
    // Map of scan tables by spiral period
    std::unordered_map<float, std::shared_ptr<LPXTables>> scanTablesByPeriod;
};

} // namespace lpx

#endif // LPX_RENDERER_H
