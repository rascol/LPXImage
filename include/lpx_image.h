/**
 * lpx_image.h
 * 
 * Header file for Log-Polar Image structures and functions
 * Based on original LPXUSBVidScanner but redesigned for cross-platform
 * use with OpenCV
 */

#ifndef LPX_IMAGE_H
#define LPX_IMAGE_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include "lpx_common.h"  // Include common definitions

// Forward declarations
namespace lpx_vision {
    class LPXVision;
}

namespace lpx {

// Position in standard 2D image
struct PositionPair {
    int x;  // Pixel horizontal location
    int y;  // Pixel vertical location
};

// Rectangle in standard 2D image
struct Rect {
    int xMin;  // Low x boundary
    int xMax;  // High x boundary
    int yMin;  // Low y boundary
    int yMax;  // High y boundary
};

// Scan Tables for mapping between standard and log-polar images
class LPXTables {
public:
    LPXTables(const std::string& filename);
    ~LPXTables();

    // Load scan tables from file
    bool load(const std::string& filename);
    
    // Check if tables are properly initialized
    bool isInitialized() const { return initialized; }

    // Debug function to print scan table info
    void printInfo() const;

    // Properties
    int mapWidth;            // X and Y dimension of the scan map in pixels
    float spiralPer;         // Spiral period of the scanned LPXImage
    int length;              // Lengths of the outerPixel arrays
    int innerLength;         // Length of the innerCells array
    int lastFoveaIndex;      // Index of the last fovea cell
    int lastCellIndex;       // Index of the last LPXImage cell

    // Mapping arrays
    std::vector<int> outerPixelIndex;       // Pixel indexes at which the LPXImage cell index changed value
    std::vector<int> outerPixelCellIdx;     // Cell indexes at the outerPixelIndex values
    std::vector<PositionPair> innerCells;   // X,Y locations for pixels in the fovea region

private:
    bool initialized;
    bool loadBinaryFormat(const std::string& filename);
    bool loadJsonFormat(const std::string& filename);
};

// Log-Polar Image class
class LPXImage {
public:
    // Create a new log-polar image with specified parameters
    LPXImage(std::shared_ptr<LPXTables> tables, int imageWidth, int imageHeight);
    ~LPXImage();
    
    // Convert from standard image to log-polar image
    bool scanFromImage(const cv::Mat& image, float x_center, float y_center);
    
    // Save log-polar image to file
    bool saveToFile(const std::string& filename) const;
    
    // Load log-polar image from file
    bool loadFromFile(const std::string& filename);
    
    // Get raw binary data
    const uint8_t* getRawData() const;
    size_t getRawDataSize() const;

    // Properties
    int getLength() const { return length; }
    int getMaxCells() const { return nMaxCells; }
    float getSpiralPeriod() const { return spiralPer; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getXOffset() const { return x_ofs; }
    float getYOffset() const { return y_ofs; }
    
    std::shared_ptr<LPXTables> getScanTables() const { return sct; }
    
    void setLength(int len) { length = std::min(len, nMaxCells); }
    void setPosition(float x, float y) { x_ofs = x; y_ofs = y; }
    
    // Pack/unpack RGB values for cell encoding/decoding (already declared as private below)
    // Helper function to calculate scan bounding box (already declared as private below)
    
    // Get the value of a specific cell
    uint32_t getCellValue(int index) const {
        static int debugCount = 0;
        if (index >= 0 && index < static_cast<int>(cellArray.size())) {
            // Add limited debug output for the first few fovea cells accessed
            // if (index < 20 && debugCount < 10) {
            //     std::cout << "DEBUG: getCellValue - Cell " << index << " = 0x" 
            //               << std::hex << cellArray[index] << std::dec << std::endl;
            //     debugCount++;
            // }
            return cellArray[index];
        }
        return 0;
    }
    
    // Direct access to internal data - for multithreaded implementation only
    std::vector<uint32_t>& accessCellArray() { return cellArray; }
    std::vector<int>& accessAccR() { return accR; }
    std::vector<int>& accessAccG() { return accG; }
    std::vector<int>& accessAccB() { return accB; }
    std::vector<int>& accessCount() { return count; }
    
    // Color extraction methods for LPXVision
    int extractCellLuminance(uint32_t cellValue) const;
    int extractCellGreenRed(uint32_t cellValue) const;
    int extractCellYellowBlue(uint32_t cellValue) const;
    
    // Friend class for LPXVision access to private members
    friend class lpx_vision::LPXVision;

private:
    int length;                 // Number of cells in the cellArray
    int nMaxCells;              // Maximum number of cells allowed
    float spiralPer;            // Spiral period in cells per revolution
    int width;                  // Width of source image in pixels
    int height;                 // Height of source image in rows
    float x_ofs;                // X-offset in source image for log-polar center
    float y_ofs;                // Y-offset in source image for log-polar center
    std::vector<uint32_t> cellArray;  // Array of cells for the LPXImage
    std::shared_ptr<LPXTables> sct;   // Scan tables

    // Accumulators for color processing
    std::vector<int> accR;      // Red accumulator for each cell
    std::vector<int> accG;      // Green accumulator for each cell
    std::vector<int> accB;      // Blue accumulator for each cell
    std::vector<int> count;     // Count of pixels in each cell
    
    // Helper function to calculate scan bounding box
    lpx::Rect getScannedBox(float x_center, float y_center, int width, int height, int length, float spiralPer);
    
    // Pack RGB values into a single 32-bit value
    uint32_t packColor(int r, int g, int b) const;
    
    // Unpack a 32-bit value into RGB components
    void unpackColor(uint32_t packed, int& r, int& g, int& b) const;
};

// Initialize the LPX system
bool initLPX(const std::string& scanTableFile, int imageWidth, int imageHeight);

// Shut down the LPX system
void shutdownLPX();

// Global scan function to create LPXImage from standard image
std::shared_ptr<LPXImage> scanImage(const cv::Mat& image, float x_center, float y_center);

// Global shared instance of scan tables
extern std::shared_ptr<LPXTables> g_scanTables;

// Multithreaded scan functions
bool multithreadedScanFromImage(LPXImage* lpxImage, const cv::Mat& image, float x_center, float y_center);
std::shared_ptr<LPXImage> multithreadedScanImage(const cv::Mat& image, float x_center, float y_center);

} // namespace lpx

#endif // LPX_IMAGE_H
