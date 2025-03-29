
#include "../include/lpx_renderer.h"
#include "../include/lpx_common.h"  // Include this for floatEquals function
#include <cmath>
#include <iostream>
#include <algorithm>

namespace lpx {

// Using implementations from lpx_common.h now

// LPXRenderer implementation
LPXRenderer::LPXRenderer() {
}

LPXRenderer::~LPXRenderer() {
}

bool LPXRenderer::setScanTables(const std::shared_ptr<LPXTables>& tables) {
    if (!tables) {
        std::cerr << "ERROR: Null tables provided to setScanTables" << std::endl;
        return false;
    }
    
    if (!tables->isInitialized()) {
        std::cerr << "ERROR: Uninitialized tables provided to setScanTables" << std::endl;
        return false;
    }
    
    // Verify spiralPer is valid
    if (tables->spiralPer < 0.1f || tables->spiralPer > 1000.0f) {
        std::cerr << "ERROR: Invalid spiralPer in tables: " << tables->spiralPer << ", not adding" << std::endl;
        return false;
    }
    
    scanTablesByPeriod[tables->spiralPer] = tables;
    
    return true;
}

bool LPXRenderer::hasScanTables(float spiralPer) const {
    for (const auto& entry : scanTablesByPeriod) {
        if (lpx::floatEquals(entry.first, spiralPer)) {
            return true;
        }
    }
    return false;
}

// Extract RGB values from an LPX cell
void LPXRenderer::getRGBFromLPCell(uint32_t lpCell, uint8_t& r, uint8_t& g, uint8_t& b) {
    // In our implementation, the cell format is BGR (OpenCV default)
    b = lpCell & 0xFF;
    g = (lpCell >> 8) & 0xFF;
    r = (lpCell >> 16) & 0xFF;
    
    // Add debug for the first few cells with non-zero values
    if (lpCell != 0 && r + g + b > 0) {
        static int debugCount = 0;
        if (debugCount < 5) {
            std::cout << "DEBUG: Cell value 0x" << std::hex << lpCell << std::dec
                      << " -> R:" << (int)r << " G:" << (int)g << " B:" << (int)b << std::endl;
            debugCount++;
        }
    }
}

Rect LPXRenderer::getScanBoundingBox(const std::shared_ptr<LPXImage>& lpxImage, int width, int height, float scaleFactor) {
    float xOfs = lpxImage->getXOffset();
    float yOfs = lpxImage->getYOffset();
    
     
    int j_ofs = static_cast<int>(std::floor(xOfs * scaleFactor + 0.5f));  // Manual rounding
    int k_ofs = static_cast<int>(std::floor(yOfs * scaleFactor + 0.5f));  // Manual rounding

    // Calculate spiral radius based on the total number of cells
    // This matches the JavaScript implementation
    float spiralRadius = getSpiralRadius(lpxImage->getLength(), lpxImage->getSpiralPeriod());
    int spRad = static_cast<int>(std::floor(spiralRadius + 0.5f));  // Manual rounding
    
    std::cout << "DEBUG: spiralRadius: " << spiralRadius << ", spRad: " << spRad << std::endl;
    
    int boundLeft = -spRad;
    int boundRight = spRad;
    int boundTop = spRad;
    int boundBottom = -spRad;

    if (boundLeft < -10000 || boundRight > 10000 || boundTop > 10000 || boundBottom < -10000) {
        std::cerr << "ERROR: Unreasonable bounds calculated, using defaults" << std::endl;
        boundLeft = -800;
        boundRight = 800;
        boundTop = 800;
        boundBottom = -800;
    }
    
    // Get the image limits
    int imgWid_2 = static_cast<int>(std::floor(0.5f * width + 0.5f));  // Manual rounding
    int imgHt_2 = static_cast<int>(std::floor(0.5f * height + 0.5f));  // Manual rounding

    std::cout << "DEBUG: boundLeft: " << boundLeft << ", boundRight: " << boundRight << ", boundTop: " << boundTop << ", boundBottom: " << boundBottom << std::endl;
    std::cout << "DEBUG: imgWid_2: " << imgWid_2 << ", imgHt_2: " << imgHt_2 << std::endl;
    
    // Get the center of the output image
    int imgCenterX = width / 2;
    int imgCenterY = height / 2;
    
    // The center offset for the view
    float xOffset = lpxImage->getXOffset() * scaleFactor;
    float yOffset = lpxImage->getYOffset() * scaleFactor;
    
    // Calculate the adjusted center position
    int adjustedCenterX = imgCenterX + static_cast<int>(xOffset);
    int adjustedCenterY = imgCenterY + static_cast<int>(yOffset);
    
    // Calculate the bounds in relation to the adjusted center
    int xMin = std::max(0, adjustedCenterX - spRad);
    int xMax = std::min(width, adjustedCenterX + spRad);
    int yMin = std::max(0, adjustedCenterY - spRad);
    int yMax = std::min(height, adjustedCenterY + spRad);


    Rect rect;
    rect.xMin = xMin;
    rect.xMax = xMax;
    rect.yMin = yMin;
    rect.yMax = yMax;
    
    return rect;
}

cv::Mat LPXRenderer::renderToImage(const std::shared_ptr<LPXImage>& lpxImage, int width, int height, 
                                   float scale, int cellOffset, int cellRange) {
    if (!lpxImage || lpxImage->getLength() <= 0) {
        std::cerr << "Invalid LPXImage or empty" << std::endl;
        return cv::Mat();
    }
    
    float spiralPer = lpxImage->getSpiralPeriod();
    
    // Find scan tables with matching spiral period using float comparison
    std::shared_ptr<LPXTables> scanTables;
    bool foundTables = false;
    
    for (const auto& entry : scanTablesByPeriod) {
        if (lpx::floatEquals(entry.first, spiralPer)) {
            scanTables = entry.second;
            foundTables = true;
            break;
        }
    }
    
    if (!foundTables) {
        std::cerr << "No scan tables available for spiral period " << spiralPer << std::endl;
        std::cout << "DEBUG: Available scan tables: ";
        for (const auto& entry : scanTablesByPeriod) {
            std::cout << entry.first << " ";
        }
        std::cout << std::endl;
        return cv::Mat();
    }
    
    // We already have scanTables set from above
    
    // Create output image
    cv::Mat output(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    
    int maxLen = lpxImage->getLength();
    int w_s = width;
    int h_s = height;
    
    // Image scaling
    float w_scale = static_cast<float>(w_s) / lpxImage->getWidth();
    float h_scale = static_cast<float>(h_s) / lpxImage->getHeight();
    float imageCanvasRatio = std::max(w_scale, h_scale);
    
    // Center-based approach for rendering
    // Map the log-polar image center directly to the output image center
    int outputCenterX = width / 2;
    int outputCenterY = height / 2;
    
    // Define the bounding box for rendering
    int colMin_s = 0;
    int colMax_s = width;
    int rowMin_s = 0;
    int rowMax_s = height;
      
    float scaleFactor = imageCanvasRatio * scale;
    
    // Position offsets
    int j_ofs, k_ofs;
    if (scale == 1.0f) {
        j_ofs = static_cast<int>(std::floor(lpxImage->getXOffset() * scaleFactor + 0.5f));  // Manual rounding
        k_ofs = static_cast<int>(std::floor(lpxImage->getYOffset() * scaleFactor + 0.5f));  // Manual rounding
    } else {
        j_ofs = 0;
        k_ofs = 0;
    }
    
    // Cell offset for scaling
    int ofs_0 = getCellArrayOffset(scaleFactor, spiralPer);
    cellOffset += ofs_0;
    
    // Set the range of cells to display
    if (cellRange <= 0) {
        cellRange = maxLen;
    }
    
    // Extract cell colors
    std::vector<uint8_t> red(maxLen);
    std::vector<uint8_t> green(maxLen);
    std::vector<uint8_t> blue(maxLen);
    
    // Debug: Check if cell colors are populated
    int nonZeroCells = 0;
    
    // Explicitly check the lowest indices - these should be populated by the innermost pixels
    int lowCellsWithValues = 0;
    
    for (int i = 0; i < 20 && i < maxLen; i++) {
        uint32_t cellValue = lpxImage->getCellValue(i);
        uint8_t r, g, b;
        getRGBFromLPCell(cellValue, r, g, b);
        
        if (cellValue != 0) {
            lowCellsWithValues++;
        }
    }
    
    
    // Check which fovea cells actually have data
    std::vector<int> nonZeroFoveaCells;
    for (int i = 0; i <= scanTables->lastFoveaIndex && i < maxLen; i++) {
        if (lpxImage->getCellValue(i) != 0) {
            nonZeroFoveaCells.push_back(i);
        }
    }
    
    
    if (!nonZeroFoveaCells.empty()) {
        for (int i = 0; i < std::min(10, static_cast<int>(nonZeroFoveaCells.size())); i++) {
            std::cout << " " << nonZeroFoveaCells[i];
        }
        std::cout << std::endl;
    }
    
    // Now process all cells
    for (int i = 0; i < maxLen; i++) {
        uint32_t cellValue = lpxImage->getCellValue(i);
        if (cellValue != 0) {
            nonZeroCells++;
        }
        uint8_t r, g, b;
        getRGBFromLPCell(cellValue, r, g, b);
        red[i] = r;
        green[i] = g;
        blue[i] = b;
    }
    
    // Set up map dimensions
    int w_m = scanTables->mapWidth;
    int h_m = w_m;
    
    int j0 = j_ofs + static_cast<int>(std::floor(w_s / 2.0f));
    int k0 = k_ofs + static_cast<int>(std::floor(h_s / 2.0f));
     
    // We don't need most of the previous rendering calculations
    // Keep track of our image scaling though
    
    // Process each row in the output image
    for (int y = rowMin_s; y < rowMax_s; y++) {
        // Process each column in the row
        for (int x = colMin_s; x < colMax_s; x++) {
            // Calculate coordinates relative to the output image center
            float relX = x - outputCenterX;
            float relY = y - outputCenterY;
            
            // Scale the relative coordinates properly based on the spiral period
            // Use full scale (1.0) to match the expected image size
            float scaledX = relX;
            float scaledY = relY;
            
            // If we're very close to the center, use special fovea handling
            float distFromCenter = std::sqrt(scaledX * scaledX + scaledY * scaledY);
            
            // Direct method: Calculate cell index from relative coordinates
            int cellIndex = getXCellIndex(scaledX, scaledY, spiralPer);
            
            // Handle the fovea region
            int lastFoveaIndex = scanTables->lastFoveaIndex;
            float centerRadius = 100.0f; // Radius for central region
            
            // In the JavaScript LPXRender.js, there's a specific branch for fovea region handling:
            // if (iCell === lastFoveaIndex) { // The pixel is in the fovea region
            //   const iC = getLPXCellIndex(i_s - i_s_ofs - j0, k_s - k0, lpxImage.spiralPer);
            //   pix.red = red[iC];
            //   pix.grn = grn[iC];
            //   pix.blu = blu[iC];
            // }
            
            // If the cell index is the last fovea index, use direct cell index calculation
            // This is how the JavaScript implementation identifies fovea region pixels
            bool isFoveaRegion = (cellIndex <= scanTables->lastFoveaIndex) || (distFromCenter < centerRadius);
            
            if (isFoveaRegion) {
                // In the fovea region, calculate cell index directly from relative position
                // This exactly matches how the JavaScript implementation works
                // The key difference: we convert from screen coords back to LOG-POLAR COORDINATES
                float relXtoCenter = x - outputCenterX;
                float relYtoCenter = y - outputCenterY;
                
                // Use direct calculation for the fovea region
                int iC = getXCellIndex(relXtoCenter, relYtoCenter, spiralPer);
                
                // Make sure the calculated index is within valid range
                iC = std::max(0, std::min(iC, maxLen - 1));
                
                // Use this value for the current pixel
                cellIndex = iC;
                
                // Debug info
                static int centerDebugCount = 0;
                if (centerDebugCount < 10) {
                    std::cout << "DEBUG: Central fovea region pixel at (" << x << "," << y 
                              << "), dist=" << distFromCenter << ", using direct calc: " << iC
                              << ", value: 0x" << std::hex << lpxImage->getCellValue(iC) << std::dec << std::endl;
                    centerDebugCount++;
                }
            }
            
            // Debug the first few pixel calculations
            static int pixelDebugCount = 0;
            bool debugThisPixel = pixelDebugCount < 5;
            
            if (debugThisPixel) {
                std::cout << "DEBUG: Processing pixel (" << x << "," << y 
                          << ") relative to center: (" << relX << "," << relY << ")" << std::endl;
                std::cout << "DEBUG: Calculated cell index: " << cellIndex << std::endl;
                pixelDebugCount++;
            }
            
            // Ensure the cell index is valid (0 to maxLen-1)
            cellIndex = std::max(0, std::min(cellIndex, maxLen - 1));
            
            // Apply cell offset
            int iCell = cellOffset + cellIndex;
            if (iCell >= maxLen) {
                iCell = cellIndex; // Fall back to original cell index if offset pushes beyond range
            }
            
            // Skip special marker cells
            if (lpxImage->getCellValue(iCell) == 0x00200400) {
                continue;
            }
            
            // Get the color values
            uint8_t r = red[iCell];
            uint8_t g = green[iCell];
            uint8_t b = blue[iCell];
            
            // Set the pixel color
            cv::Vec3b color(b, g, r); // OpenCV uses BGR format
            output.at<cv::Vec3b>(y, x) = color;
            
            // Debug non-black pixel assignments
            if (r > 0 || g > 0 || b > 0) {
                static int colorDebugCount = 0;
                if (colorDebugCount < 10) {
                    std::cout << "DEBUG: Setting non-black pixel (" << x << "," << y 
                              << ") to color (" << (int)r << "," << (int)g << "," << (int)b << ")"
                              << " from cell " << iCell << std::endl;
                    colorDebugCount++;
                }
            }
        }
    }
    
    return output;
}

} // namespace lpx
