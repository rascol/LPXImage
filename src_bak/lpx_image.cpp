/**
 * lpx_image.cpp
 * 
 * Implementation of Log-Polar Image functions
 * Based on original LPXUSBVidScanner but redesigned for cross-platform
 * use with OpenCV
 */

#include "../include/lpx_image.h"
#include "../include/lpx_common.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <thread>
#include <future>

namespace lpx {

// Global shared instance of scan tables
static std::shared_ptr<LPXTables> g_scanTables;

// LPXTables implementation
LPXTables::LPXTables(const std::string& filename) : initialized(false), spiralPer(63.5f) {
    if (!filename.empty()) {
        load(filename);
    }
}

LPXTables::~LPXTables() {
    // Clean up any resources
}

bool LPXTables::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open scan tables file: " << filename << std::endl;
        return false;
    }

    // Read header values
    int totalLength;
    file.read(reinterpret_cast<char*>(&totalLength), sizeof(int));
    file.read(reinterpret_cast<char*>(&mapWidth), sizeof(int));
    
    // Read spiral period as integer and add 0.5 as per JavaScript implementation
    int spiralPerInt;
    file.read(reinterpret_cast<char*>(&spiralPerInt), sizeof(int));
    
    // Add 0.5 to match JavaScript's approach (sct.spiralPer = buf.readUInt32LE(4 * 2) + 0.5)
    float loadedSpiralPer = static_cast<float>(spiralPerInt) + 0.5f;
     
    // Force a valid value in case of bad read
    if (loadedSpiralPer < 0.1f || loadedSpiralPer > 1000.0f) {
        std::cerr << "ERROR: Invalid spiralPer read from file: " << loadedSpiralPer << ", using default 63.5" << std::endl;
        spiralPer = 63.5f;
    } else {
        spiralPer = loadedSpiralPer;
    }
    
    file.read(reinterpret_cast<char*>(&length), sizeof(int));
    file.read(reinterpret_cast<char*>(&innerLength), sizeof(int));
    file.read(reinterpret_cast<char*>(&lastFoveaIndex), sizeof(int));
    file.read(reinterpret_cast<char*>(&lastCellIndex), sizeof(int));

    // Resize and read arrays
    outerPixelIndex.resize(length);
    outerPixelCellIdx.resize(length);
    innerCells.resize(innerLength);

    file.read(reinterpret_cast<char*>(outerPixelIndex.data()), length * sizeof(int));
    file.read(reinterpret_cast<char*>(outerPixelCellIdx.data()), length * sizeof(int));
    file.read(reinterpret_cast<char*>(innerCells.data()), innerLength * sizeof(PositionPair));

    initialized = true;
    return true;
}

// LPXImage implementation
LPXImage::LPXImage(std::shared_ptr<LPXTables> tables, int imageWidth, int imageHeight)
    : length(0), nMaxCells(0), spiralPer(0), width(imageWidth), height(imageHeight),
      x_ofs(0), y_ofs(0), sct(tables) {
    std::cout << "DEBUG: LPXImage constructor - tables initialized: " << (tables && tables->isInitialized()) << std::endl;
    std::cout << "DEBUG: Input tables->spiralPer = " << (tables ? tables->spiralPer : -1.0f) << std::endl;
    
    if (tables && tables->lastCellIndex > 0) {
        nMaxCells = tables->lastCellIndex + 1;
        spiralPer = tables->spiralPer;
        
        // Force the value to verify it's correctly set
        std::cout << "DEBUG: Setting spiralPer = " << spiralPer << std::endl;
        
        // Add additional safeguard - if it's still 0, use the value from tables directly
        if (spiralPer < 0.1f) {  // If it's close to zero or garbage value
            spiralPer = tables->spiralPer;
            std::cout << "DEBUG: Re-setting spiralPer = " << spiralPer << std::endl;
        }
        
        // Initialize arrays
        cellArray.resize(nMaxCells, 0);
        accR.resize(nMaxCells, 0);
        accG.resize(nMaxCells, 0);
        accB.resize(nMaxCells, 0);
        count.resize(nMaxCells, 0);
        
    } else {
        std::cout << "DEBUG: Tables check failed: tables null? " << (tables == nullptr) << " lastCellIndex = " << (tables ? tables->lastCellIndex : -1) << std::endl;
    }
}

LPXImage::~LPXImage() {
    // Clean up any resources
}

uint32_t LPXImage::packColor(int r, int g, int b) const {
    uint32_t color = 0;
    
    // We'll pack in BGR format (like OpenCV) to maintain compatibility
    color |= (b & 0xFF);
    color |= ((g & 0xFF) << 8);
    color |= ((r & 0xFF) << 16);
    
    return color;
}

void LPXImage::unpackColor(uint32_t packed, int& r, int& g, int& b) const {
    b = packed & 0xFF;
    g = (packed >> 8) & 0xFF;
    r = (packed >> 16) & 0xFF;
}

// Process a portion of the image for multi-threaded scan
static void processImageRegion(const cv::Mat& image, int yStart, int yEnd, 
                      float centerX, float centerY, 
                      std::shared_ptr<LPXTables> sct,
                      std::vector<int>& accR, std::vector<int>& accG, 
                      std::vector<int>& accB, std::vector<int>& count) {
    
    // Debug counters
    int pixelsProcessed = 0;
    int cellsUpdated = 0;
    int foveaCellsUpdated = 0;
    
    // Process the inner (fovea) region first
    int innerCellsProcessed = 0;
    for (int i = 0; i < sct->innerLength; i++) {
        int x = static_cast<int>(centerX + sct->innerCells[i].x);
        int y = static_cast<int>(centerY + sct->innerCells[i].y);
        
        // Skip if outside our assigned region
        if (y < yStart || y >= yEnd) {
            continue;
        }
        
        innerCellsProcessed++;
        
        // Ensure coordinates are within image bounds
        bool inBounds = (x >= 0 && x < image.cols && y >= 0 && y < image.rows);
        // Debug every 500th cell bounds check
        if (i % 500 == 0) {
            std::string boundsStatus = inBounds ? "yes" : "NO - image size: " + 
                                      std::to_string(image.cols) + "x" + std::to_string(image.rows);
            std::cout << "DEBUG: Fovea cell " << i << " coords (" << x << "," << y 
                      << ") in bounds: " << boundsStatus << std::endl;
        }
        if (inBounds) {
            // Debug the first few valid coordinates
            if (foveaCellsUpdated < 5) {
                std::cout << "DEBUG: Valid fovea cell coordinate at (" << x << "," << y 
                          << ") for inner cell " << i << std::endl;
            }
            // Get the pixel color at the coordinates
            cv::Vec3b color;
            if (image.channels() == 3) {
                color = image.at<cv::Vec3b>(y, x);
            } else if (image.channels() == 1) {
                // Grayscale image - replicate intensity for all channels
                uchar intensity = image.at<uchar>(y, x);
                color = cv::Vec3b(intensity, intensity, intensity);
            } else {
                continue; // Unsupported format
            }
            
            // Accumulate color values for the cell
            int cellIndex = sct->outerPixelCellIdx[i];
            if (cellIndex >= 0 && cellIndex < static_cast<int>(accR.size())) {
                accR[cellIndex] += color[2]; // OpenCV uses BGR format
                accG[cellIndex] += color[1];
                accB[cellIndex] += color[0];
                count[cellIndex]++;
                cellsUpdated++;
            }
        }
    }
    
    // Use binary search to find pixel indices efficiently
    auto findCellIndex = [&sct](int pixelIdx) -> int {
        // Binary search in the outerPixelIndex array
        int low = 0, high = sct->length - 1;
        
        // Debug first 5 calls to findCellIndex
        static int findCellDebugCount = 0;
        bool debugThis = findCellDebugCount < 5;
        
        if (debugThis) {
            std::cout << "DEBUG: [process] findCellIndex searching for pixelIdx=" << pixelIdx 
                      << " in range [0," << high << "]" << std::endl;
        }
        
        // Simple sequential scan for debugging
        if (debugThis) {
            // Check first few indices sequentially
            for (int i = 0; i < std::min(10, sct->length); i++) {
                std::cout << "DEBUG: [process] Index " << i << ": pixelIdx=" << sct->outerPixelIndex[i] 
                          << ", cellIdx=" << sct->outerPixelCellIdx[i] << std::endl;
            }
        }
        
        while (low <= high) {
            int mid = (low + high) / 2;
            if (sct->outerPixelIndex[mid] < pixelIdx) {
                low = mid + 1;
            } else if (sct->outerPixelIndex[mid] > pixelIdx) {
                high = mid - 1;
            } else {
                if (debugThis) {
                    std::cout << "DEBUG: [process] findCellIndex found exact match at index " << mid 
                              << ", returning cell index " << sct->outerPixelCellIdx[mid] << std::endl;
                    findCellDebugCount++;
                }
                return sct->outerPixelCellIdx[mid];
            }
        }
        
        // Use the highest index that's less than or equal to the target
        int result = (high >= 0) ? sct->outerPixelCellIdx[high] : sct->lastFoveaIndex;
        
        if (debugThis) {
            std::cout << "DEBUG: [process] findCellIndex found approximate match, high=" << high 
                      << ", returning cell index " << result << std::endl;
            findCellDebugCount++;
        }
        
        return result;
    };
    
    // Process each row in our assigned region
    for (int y = yStart; y < yEnd; y++) {
        for (int x = 0; x < image.cols; x++) {
            // Convert to coordinates relative to center
            float relX = x - centerX;
            float relY = y - centerY;
            
            // Get cell index directly for pixels outside fovea
            int cellIndex;
            
            // Use scan tables instead of computing the cell index for every pixel
            int pixelIndex = y * image.cols + x;
            cellIndex = findCellIndex(pixelIndex);
            
            // If we're in the fovea region, calculate exact cell index
            if (cellIndex == sct->lastFoveaIndex) {
                // Debug the first few fovea calculations
                static int foveaDebugCount = 0;
                bool debugThis = foveaDebugCount < 5;
                
                if (debugThis) {
                    std::cout << "DEBUG: Calculating fovea cell index for coords (" 
                              << relX << "," << relY << ") with spiralPer=" 
                              << sct->spiralPer << std::endl;
                }
                
                cellIndex = getXCellIndex(relX, relY, sct->spiralPer);
                
                foveaCellsUpdated++;
                
                if (debugThis) {
                    std::cout << "DEBUG: Calculated fovea cell index: " << cellIndex << std::endl;
                    foveaDebugCount++;
                }
            }
            
            // Skip if outside valid range
            if (cellIndex < 0 || cellIndex >= static_cast<int>(accR.size())) {
                continue;
            }
            
            // Get the pixel color
            cv::Vec3b color;
            if (image.channels() == 3) {
                color = image.at<cv::Vec3b>(y, x);
            } else if (image.channels() == 1) {
                uchar intensity = image.at<uchar>(y, x);
                color = cv::Vec3b(intensity, intensity, intensity);
            } else {
                continue; // Unsupported format
            }
            
            // Accumulate color values
            accR[cellIndex] += color[2];
            accG[cellIndex] += color[1];
            accB[cellIndex] += color[0];
            count[cellIndex]++;
            cellsUpdated++;
            pixelsProcessed++;
        }
    }

bool LPXImage::scanFromImage(const cv::Mat& image, float x_center, float y_center) {
    if (!sct || !sct->isInitialized() || image.empty()) {
        return false;
    }
    
    // Verify that the scan tables have valid data
    if (sct->outerPixelIndex.empty() || sct->outerPixelCellIdx.empty() || sct->innerCells.empty()) {
        std::cerr << "ERROR: Scan tables are empty! Cannot scan image." << std::endl;
        return false;
    }
    
    // Check if the last fovea index is set correctly
    if (sct->lastFoveaIndex <= 0 || sct->lastFoveaIndex >= sct->lastCellIndex) {
        std::cerr << "ERROR: Invalid lastFoveaIndex: " << sct->lastFoveaIndex 
                  << ", lastCellIndex: " << sct->lastCellIndex << std::endl;
        return false;
    }

    // Store offset coordinates
    x_ofs = x_center;
    y_ofs = y_center;
    
    // Reset accumulators
    std::fill(accR.begin(), accR.end(), 0);
    std::fill(accG.begin(), accG.end(), 0);
    std::fill(accB.begin(), accB.end(), 0);
    std::fill(count.begin(), count.end(), 0);
    
    // Step 1: Process the fovea region directly
    int foveaCellsUpdated = 0;
    
    // Calculate scan map offsets (needed for coordinate conversion)
    int w_s = image.cols;
    int h_s = image.rows;
    int w_m = sct->mapWidth;
    int h_m = w_m;
    
    // Assume the scan map center is at mapWidth/2, mapHeight/2
    int scanMapCenterX = w_m / 2; // Likely 3000 for 6000x6000 map
    int scanMapCenterY = h_m / 2; // Likely 3000 for 6000x6000 map
    
    std::cout << "DEBUG: Scan map size: " << w_m << "x" << h_m << ", center at (" << scanMapCenterX << "," << scanMapCenterY << ")" << std::endl;
    
    // Check the first few inner cells for debugging
    for (int i = 0; i < std::min(5, sct->innerLength); i++) {
        std::cout << "DEBUG: Inner cell " << i << " original coords: (" << sct->innerCells[i].x << "," << sct->innerCells[i].y << ")" << std::endl;
        // Expected true coordinates if centered at (0,0)
        int expectedX = sct->innerCells[i].x - 3000;
        int expectedY = sct->innerCells[i].y - 3000;
        std::cout << "DEBUG: Inner cell " << i << " center-adjusted coords: (" << expectedX << "," << expectedY << ")" << std::endl;
    }
    
    for (int i = 0; i < sct->innerLength; i++) {
        // Adjust coordinates to be center-based
        // Assuming the scan table coordinates are centered at scan map center
        int adjustedX = sct->innerCells[i].x - scanMapCenterX;
        int adjustedY = sct->innerCells[i].y - scanMapCenterY;
        
        // Now add to image center to get actual image coordinates
        int x = static_cast<int>(x_center + adjustedX);
        int y = static_cast<int>(y_center + adjustedY);
        
        // Debug every 500th cell
        if (i % 500 == 0) {
            std::cout << "DEBUG: Processing inner cell " << i << ", adjusted coords (" << adjustedX << "," << adjustedY 
                      << "), image coords (" << x << "," << y << ")" << std::endl;
        }
        
        // Ensure coordinates are within image bounds
        bool inBounds = (x >= 0 && x < image.cols && y >= 0 && y < image.rows);
        // Debug every 500th cell bounds check
        if (i % 500 == 0) {
            std::string boundsStatus = inBounds ? "yes" : "NO - image size: " + 
                                      std::to_string(image.cols) + "x" + std::to_string(image.rows);
            std::cout << "DEBUG: Inner cell " << i << " coords (" << x << "," << y 
                      << ") in bounds: " << boundsStatus << std::endl;
        }
        if (inBounds) {
            // Get the pixel color at the coordinates
            cv::Vec3b color;
            if (image.channels() == 3) {
                color = image.at<cv::Vec3b>(y, x);
            } else if (image.channels() == 1) {
                // Grayscale image - replicate intensity for all channels
                uchar intensity = image.at<uchar>(y, x);
                color = cv::Vec3b(intensity, intensity, intensity);
            } else {
                continue; // Unsupported format
            }
            
            // For fovea region, the cell index might be the inner index
            // But to be safe, let's use the outerPixelCellIdx mapping if available
            int cellIndex;
            if (i <= sct->lastFoveaIndex && i < static_cast<int>(cellArray.size())) {
                // For the first lastFoveaIndex+1 cells, use the index directly
                cellIndex = i;
            } else {
                // For others, use the mapping from the scan tables
                cellIndex = sct->outerPixelCellIdx[i];
            }
               
            // Set the cell color directly (no averaging for fovea cells)
            if (cellIndex >= 0 && cellIndex < static_cast<int>(cellArray.size())) {
                         cellArray[cellIndex] = packColor(color[2], color[1], color[0]); // RGB from BGR
                foveaCellsUpdated++;
            } else {
                // This shouldn't happen, but debug just in case
                std::cout << "ERROR: Cell index " << cellIndex << " (from inner cell " << i << ") is outside cellArray size " << cellArray.size() << std::endl;
            }
        }
    }
     
    // Step 2: Process the rest of the image using scan tables
    int outerCellsWithPixels = 0;
    
    // Calculate dimensions and offsets as in the JavaScript version
    // We don't need to recalculate these since we already did above
    // int w_s = image.cols;
    // int h_s = image.rows;
    // int w_m = sct->mapWidth;
    // int h_m = w_m;
    
    int j_ofs = static_cast<int>(x_center);
    int k_ofs = static_cast<int>(y_center);
    
    // Use the adjusted offset calculation for scan map to image conversion
    int ws_wm_jofs = scanMapCenterX - j_ofs; // Column offset into scan map
    int hs_hm_kofs = scanMapCenterY - k_ofs; // Row offset into scan map
     
    int i_m_ofs_0 = ws_wm_jofs + w_m * hs_hm_kofs; // Index offset into the scan map
    
    // Get bounding box for processing
    lpx::Rect box = getScannedBox(x_center, y_center, w_s, h_s, nMaxCells, sct->spiralPer);
     
    // Process each row in the bounding box
    for (int k_s = box.yMin; k_s < box.yMax; k_s++) {
        int i_m_ofs = i_m_ofs_0 + w_m * k_s; // Scan map offset for this row
        
        // Process each column in the row
        for (int j_s = box.xMin; j_s < box.xMax; j_s++) {
            int i_m = i_m_ofs + j_s; // Current pixel index in scan map
            
            // Find the cell index for this pixel using binary search in the scan tables
            int iAr = 0;
            int iCell = 0;
            
            // Search for i_m in the outerPixelIndex array
            int low = 0, high = sct->length - 1;
            while (low <= high) {
                int mid = (low + high) / 2;
                if (sct->outerPixelIndex[mid] < i_m) {
                    low = mid + 1;
                } else if (sct->outerPixelIndex[mid] > i_m) {
                    high = mid - 1;
                } else {
                    // Exact match found
                    iCell = sct->outerPixelCellIdx[mid];
                    break;
                }
            }
            
            // If no exact match, use the highest index less than the target
            if (low > high) {
                iCell = (high >= 0) ? sct->outerPixelCellIdx[high] : sct->lastFoveaIndex;
            }
            
            // Skip if this is a fovea cell (already processed) or outside valid range
            if (iCell <= sct->lastFoveaIndex || iCell >= static_cast<int>(accR.size())) {
                continue;
            }
            
            // Get the pixel color
            cv::Vec3b color;
            if (j_s >= 0 && j_s < image.cols && k_s >= 0 && k_s < image.rows) {
                if (image.channels() == 3) {
                    color = image.at<cv::Vec3b>(k_s, j_s);
                } else if (image.channels() == 1) {
                    uchar intensity = image.at<uchar>(k_s, j_s);
                    color = cv::Vec3b(intensity, intensity, intensity);
                } else {
                    continue; // Unsupported format
                }
                
                // Accumulate color values for this cell
                accR[iCell] += color[2]; // OpenCV uses BGR format
                accG[iCell] += color[1];
                accB[iCell] += color[0];
                count[iCell]++;
            }
        }
    }

    // Calculate average colors for each cell
    int nonZeroCells = 0;
    int nonZeroCount = 0;
    for (int i = 0; i < nMaxCells; i++) {
        // Use accumulated color values for cells that were scanned
        if (count[i] > 0) {
            int r = accR[i] / count[i];
            int g = accG[i] / count[i];
            int b = accB[i] / count[i];
            cellArray[i] = packColor(r, g, b);
            nonZeroCells++;
            
            // Debug the first few cells with color
            if (nonZeroCount < 5) {
                std::cout << "DEBUG: Cell " << i << " populated with color: R=" << r
                          << ", G=" << g << ", B=" << b << ", count=" << count[i] << std::endl;
                nonZeroCount++;
            }
        } else if (i <= sct->lastFoveaIndex) {
            // Don't clear fovea cells that were directly populated!
            if (cellArray[i] == 0) {
                // Only debug if we need to keep some cell values
                if (nonZeroCount < 10) {
                    std::cout << "DEBUG: Keeping fovea cell " << i << " value intact" << std::endl;
                }
            }
        } else {
            cellArray[i] = 0; // Black for cells without pixels
        }
    }
    
    length = nMaxCells;
    return true;
}

bool LPXImage::saveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    // Calculate total length (header + data)
    int totalLength = 8 + length;
    
    // Write totalLength
    file.write(reinterpret_cast<const char*>(&totalLength), sizeof(int));
    
    // Write header
    file.write(reinterpret_cast<const char*>(&length), sizeof(int));
    file.write(reinterpret_cast<const char*>(&nMaxCells), sizeof(int));

    // Convert spiralPer to int for compatibility with original format but preserve fractional part
    int spiralPerInt = static_cast<int>(spiralPer);
    float spiralPerFrac = spiralPer - static_cast<float>(spiralPerInt);
    std::cout << "DEBUG: Saving spiralPer = " << spiralPer 
            << ", as int = " << spiralPerInt
            << ", frac = " << spiralPerFrac << std::endl;
    file.write(reinterpret_cast<const char*>(&spiralPerInt), sizeof(int));
 
    file.write(reinterpret_cast<const char*>(&width), sizeof(int));
    file.write(reinterpret_cast<const char*>(&height), sizeof(int));
    
    // Scale offsets by 100000 as in original format
    int x_ofs_scaled = static_cast<int>(x_ofs * 100000);
    int y_ofs_scaled = static_cast<int>(y_ofs * 100000);
    file.write(reinterpret_cast<const char*>(&x_ofs_scaled), sizeof(int));
    file.write(reinterpret_cast<const char*>(&y_ofs_scaled), sizeof(int));
    
    // Write cell array
    file.write(reinterpret_cast<const char*>(cellArray.data()), cellArray.size() * sizeof(uint32_t));
    
    return true;
}

bool LPXImage::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    // Read totalLength
    int totalLength;
    file.read(reinterpret_cast<char*>(&totalLength), sizeof(int));
    
    // Read header
    file.read(reinterpret_cast<char*>(&length), sizeof(int));
    file.read(reinterpret_cast<char*>(&nMaxCells), sizeof(int));
    
    int spiralPerInt;
    file.read(reinterpret_cast<char*>(&spiralPerInt), sizeof(int));
    spiralPer = static_cast<float>(spiralPerInt) + 0.5f; // Add the fractional part
    
    // Sanity check after loading
    if (spiralPer < 0.1f) {
        std::cerr << "ERROR: Invalid spiralPer loaded: " << spiralPer << ", using default of 63.5" << std::endl;
        spiralPer = 63.5f;
    }
    
    file.read(reinterpret_cast<char*>(&width), sizeof(int));
    file.read(reinterpret_cast<char*>(&height), sizeof(int));
    
    // Read scaled offsets
    int x_ofs_scaled, y_ofs_scaled;
    file.read(reinterpret_cast<char*>(&x_ofs_scaled), sizeof(int));
    file.read(reinterpret_cast<char*>(&y_ofs_scaled), sizeof(int));
    
    // Convert scaled offsets back to float
    x_ofs = x_ofs_scaled * 1e-5f;
    y_ofs = y_ofs_scaled * 1e-5f;
    
    // Resize and read cell array
    cellArray.resize(length);
    file.read(reinterpret_cast<char*>(cellArray.data()), length * sizeof(uint32_t));
    
    // Resize accumulators
    accR.resize(nMaxCells, 0);
    accG.resize(nMaxCells, 0);
    accB.resize(nMaxCells, 0);
    count.resize(nMaxCells, 0);
    
    // Debug: Check the first few loaded cells to verify fovea cell values
    int nonZeroCells = 0;
    if (sct) {
        int lastFoveaIndex = sct->lastFoveaIndex;
        int nonZeroFoveaCells = 0;
        for (int i = 0; i <= lastFoveaIndex && i < length; i++) {
            if (cellArray[i] != 0) {
                nonZeroFoveaCells++;
                if (nonZeroFoveaCells <= 5) {
                    int r, g, b;
                    unpackColor(cellArray[i], r, g, b);
                }
            }
        }
    }
    
    // Count all non-zero cells
    for (int i = 0; i < length; i++) {
        if (cellArray[i] != 0) {
            nonZeroCells++;
            if (nonZeroCells <= 5) {
                int r, g, b;
                unpackColor(cellArray[i], r, g, b);
            }
        }
    }
    
    return true;
}

const uint8_t* LPXImage::getRawData() const {
    return reinterpret_cast<const uint8_t*>(cellArray.data());
}

size_t LPXImage::getRawDataSize() const {
    return cellArray.size() * sizeof(uint32_t);
}

// Global functions
bool initLPX(const std::string& scanTableFile, int imageWidth, int imageHeight) {
    g_scanTables = std::make_shared<LPXTables>(scanTableFile);
    return g_scanTables->isInitialized();
}

void shutdownLPX() {
    g_scanTables.reset();
}

std::shared_ptr<LPXImage> scanImage(const cv::Mat& image, float x_center, float y_center) {
    if (!g_scanTables || !g_scanTables->isInitialized()) {
        return nullptr;
    }
    
    auto lpxImage = std::make_shared<LPXImage>(g_scanTables, image.cols, image.rows);
    if (lpxImage->scanFromImage(image, x_center, y_center)) {
        return lpxImage;
    }
    
    return nullptr;
}

// Helper function to calculate scan bounding box
lpx::Rect LPXImage::getScannedBox(float x_center, float y_center, int width, int height, int length, float spiralPer) {
    lpx::Rect box;
    
    // Calculate spiral radius based on the total number of cells
    float spiralRadius = getSpiralRadius(length, spiralPer);
    int spRad = static_cast<int>(std::floor(spiralRadius + 0.5f));
    
    // Calculate bounding box limits
    int j_ofs = static_cast<int>(std::floor(x_center + 0.5f));
    int k_ofs = static_cast<int>(std::floor(y_center + 0.5f));
    
    // Get the image limits
    int imgWid_2 = static_cast<int>(std::floor(0.5f * width + 0.5f));
    int imgHt_2 = static_cast<int>(std::floor(0.5f * height + 0.5f));
    
    // Calculate initial boundaries
    box.xMin = imgWid_2 - spRad - j_ofs;
    if (box.xMin < 0) box.xMin = 0;
    
    box.xMax = width - (imgWid_2 - spRad) - j_ofs;
    if (box.xMax > width) box.xMax = width;
    
    box.yMin = imgHt_2 - spRad - k_ofs;
    if (box.yMin < 0) box.yMin = 0;
    
    box.yMax = height - (imgHt_2 - spRad) - k_ofs;
    if (box.yMax > height) box.yMax = height;
      
    return box;
}

} // namespace lpx
