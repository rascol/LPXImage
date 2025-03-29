/**
 * mt_lpx_image.cpp
 * 
 * Multithreaded implementation of Log-Polar Image functions
 */

#include "../include/lpx_mt.h"
#include "../include/lpx_common.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <thread>
#include <future>
#include <mutex>
#include <algorithm>
#include <chrono>

namespace lpx {

// No need to externally reference g_scanTables anymore

// The implementation of the internal region processing function
namespace internal {

void processImageRegion(const cv::Mat& image, int yStart, int yEnd, 
                  float centerX, float centerY, 
                  std::shared_ptr<LPXTables> sct,
                  std::vector<int>& accR, std::vector<int>& accG, 
                  std::vector<int>& accB, std::vector<int>& count,
                  std::mutex& accMutex) {
    
    // Local accumulators for this thread
    std::vector<int> localAccR(accR.size(), 0);
    std::vector<int> localAccG(accG.size(), 0);
    std::vector<int> localAccB(accB.size(), 0);
    std::vector<int> localCount(count.size(), 0);
    
    // Debug counters
    int pixelsProcessed = 0;
    int cellsUpdated = 0;
    
    // Use binary search to find pixel indices efficiently
    auto findCellIndex = [&sct](int pixelIdx) -> int {
        // Binary search in the outerPixelIndex array
        int low = 0, high = sct->length - 1;
        
        while (low <= high) {
            int mid = (low + high) / 2;
            if (sct->outerPixelIndex[mid] < pixelIdx) {
                low = mid + 1;
            } else if (sct->outerPixelIndex[mid] > pixelIdx) {
                high = mid - 1;
            } else {
                return sct->outerPixelCellIdx[mid];
            }
        }
        
        // Use the highest index that's less than or equal to the target
        int result = (high >= 0) ? sct->outerPixelCellIdx[high] : sct->lastFoveaIndex;
        return result;
    };
    
    // Calculate scan map offsets (needed for coordinate conversion)
    int w_s = image.cols;
    int h_s = image.rows;
    int w_m = sct->mapWidth;
    int h_m = w_m;
    
    // Assume the scan map center is at mapWidth/2, mapHeight/2
    int scanMapCenterX = w_m / 2; // Likely 3000 for 6000x6000 map
    int scanMapCenterY = h_m / 2; // Likely 3000 for 6000x6000 map
    
    // Use the adjusted offset calculation for scan map to image conversion
    int j_ofs = static_cast<int>(centerX);
    int k_ofs = static_cast<int>(centerY);
    int ws_wm_jofs = scanMapCenterX - j_ofs; // Column offset into scan map
    int hs_hm_kofs = scanMapCenterY - k_ofs; // Row offset into scan map
    int i_m_ofs_0 = ws_wm_jofs + w_m * hs_hm_kofs; // Index offset into the scan map

    // Process each row in our assigned region
    for (int k_s = yStart; k_s < yEnd; k_s++) {
        int i_m_ofs = i_m_ofs_0 + w_m * k_s; // Scan map offset for this row
        
        // Process each column in the row
        for (int j_s = 0; j_s < image.cols; j_s++) {
            int i_m = i_m_ofs + j_s; // Current pixel index in scan map
            
            // Find the cell index for this pixel using binary search in the scan tables
            int iCell = findCellIndex(i_m);
            
            // Skip if this is a fovea cell (already processed) or outside valid range
            if (iCell <= sct->lastFoveaIndex || iCell >= static_cast<int>(localAccR.size())) {
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
                
                // Accumulate color values for this cell in our local accumulators
                localAccR[iCell] += color[2]; // OpenCV uses BGR format
                localAccG[iCell] += color[1];
                localAccB[iCell] += color[0];
                localCount[iCell]++;
                cellsUpdated++;
                pixelsProcessed++;
            }
        }
    }
    
    // Merge local accumulators into global ones with a lock
    {
        std::lock_guard<std::mutex> lock(accMutex);
        for (size_t i = 0; i < accR.size(); i++) {
            if (localCount[i] > 0) {
                accR[i] += localAccR[i];
                accG[i] += localAccG[i];
                accB[i] += localAccB[i];
                count[i] += localCount[i];
            }
        }
    }
    
    // We processed this many pixels/cells - no need to print debug info
}

} // namespace internal

// We no longer need this function since we're accessing the internal arrays directly
// through the accessCellArray() methods in the LPXImage class

// This is a pack/unpack helper since we don't have access to the original
static uint32_t packColor(int r, int g, int b) {
    uint32_t color = 0;
    color |= (b & 0xFF);
    color |= ((g & 0xFF) << 8);
    color |= ((r & 0xFF) << 16);
    return color;
}

// Helper function to calculate scan bounding box
static lpx::Rect getScannedBox(float x_center, float y_center, int width, int height, int length, float spiralPer) {
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
    
    // Calculate bounding box based on parameters
    
    return box;
}

// Multithreaded implementation of scanFromImage
bool multithreadedScanFromImage(LPXImage* lpxImage, const cv::Mat& image, float x_center, float y_center) {
    auto sct = lpxImage->getScanTables();
    if (!sct || !sct->isInitialized() || image.empty()) {
        std::cerr << "ERROR: Invalid scan tables or image" << std::endl;
        return false;
    }
    
    // Set position in the LPXImage
    lpxImage->setPosition(x_center, y_center);
    
    // Get direct access to the LPXImage's internal arrays
    std::vector<uint32_t>& cellArray = lpxImage->accessCellArray();
    std::vector<int>& accR = lpxImage->accessAccR();
    std::vector<int>& accG = lpxImage->accessAccG();
    std::vector<int>& accB = lpxImage->accessAccB();
    std::vector<int>& count = lpxImage->accessCount();
    
    // Get max cells
    int nMaxCells = lpxImage->getMaxCells();
    
    // Reset accumulators
    std::fill(accR.begin(), accR.end(), 0);
    std::fill(accG.begin(), accG.end(), 0);
    std::fill(accB.begin(), accB.end(), 0);
    std::fill(count.begin(), count.end(), 0);
    
    // Start the multithreaded scanning approach
    std::cout << "Using multithreaded scanning approach" << std::endl;
    
    // Get the scan map dimensions
    int w_m = sct->mapWidth;
    int h_m = w_m;
    int scanMapCenterX = w_m / 2;
    int scanMapCenterY = h_m / 2;
    
    // STEP 1: Process the fovea region first in the main thread
    std::cout << "Processing fovea region from 0 to " << sct->lastFoveaIndex << std::endl;
    int foveaCellsUpdated = 0;
    
    // Process each inner cell directly
    for (int i = 0; i < sct->innerLength; i++) {
        // Adjust coordinates to be center-based
        // Assuming the scan table coordinates are centered at scan map center
        int adjustedX = sct->innerCells[i].x - scanMapCenterX;
        int adjustedY = sct->innerCells[i].y - scanMapCenterY;
        
        // Now add to image center to get actual image coordinates
        int x = static_cast<int>(x_center + adjustedX);
        int y = static_cast<int>(y_center + adjustedY);
        
        // Ensure coordinates are within image bounds
        bool inBounds = (x >= 0 && x < image.cols && y >= 0 && y < image.rows);
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
                // Error message for invalid cell index
                std::cerr << "ERROR: Cell index " << cellIndex << " outside valid range (size: " << cellArray.size() << ")" << std::endl;
            }
        }
    }
    
    // STEP 2: Process the rest of the image using multiple threads
    // Determine the number of threads to use (leave 1 core free for system)
    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    if (numThreads == 0) numThreads = 1; // Safeguard
    std::cout << "Using " << numThreads << " threads for outer region processing" << std::endl;
    
    // Calculate dimensions and offsets for the outer region
    int j_ofs = static_cast<int>(x_center);
    int k_ofs = static_cast<int>(y_center);
    
    // Get bounding box for processing
    lpx::Rect box = getScannedBox(x_center, y_center, image.cols, image.rows, nMaxCells, sct->spiralPer);
    
    // Create a mutex for thread-safe accumulator updates
    std::mutex accMutex;
    
    // Divide the image into horizontal strips and process each strip in a separate thread
    std::vector<std::thread> threads;
    int rowsPerThread = (box.yMax - box.yMin) / numThreads;
    if (rowsPerThread < 1) rowsPerThread = 1; // Safeguard
    
    // Launch threads
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (unsigned int t = 0; t < numThreads; t++) {
        int startRow = box.yMin + t * rowsPerThread;
        int endRow = (t == numThreads - 1) ? box.yMax : startRow + rowsPerThread;
        
        threads.push_back(std::thread(internal::processImageRegion, std::ref(image), startRow, endRow, 
                                    x_center, y_center, sct, 
                                    std::ref(accR), std::ref(accG), std::ref(accB), std::ref(count),
                                    std::ref(accMutex)));
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Multithreaded processing completed in " << duration << " ms using " << numThreads << " threads" << std::endl;

    // Calculate average colors for each cell
    int nonZeroCells = 0;
    for (int i = 0; i < nMaxCells; i++) {
        // Use accumulated color values for cells that were scanned
        if (count[i] > 0) {
            int r = accR[i] / count[i];
            int g = accG[i] / count[i];
            int b = accB[i] / count[i];
            cellArray[i] = packColor(r, g, b);
            nonZeroCells++;
        } else if (i <= sct->lastFoveaIndex) {
            // Don't clear fovea cells that were directly populated!
            // (No action needed)
        } else {
            cellArray[i] = 0; // Black for cells without pixels
        }
    }

    // Set the length
    lpxImage->setLength(nMaxCells);
    return true;
}

// Helper function that creates a new LPXImage and scans it using multithreading
std::shared_ptr<LPXImage> multithreadedScanImage(const cv::Mat& image, float x_center, float y_center) {
    // Check if scan tables are initialized
    if (!g_scanTables || !g_scanTables->isInitialized()) {
        std::cerr << "ERROR: No scan tables available for multithreaded scan" << std::endl;
        return nullptr;
    }
    
    // Create a new LPXImage with the scan tables
    auto lpxImage = std::make_shared<LPXImage>(g_scanTables, image.cols, image.rows);
    
    // Use multithreaded scanning directly - this now works directly with the lpxImage's internal buffers
    if (multithreadedScanFromImage(lpxImage.get(), image, x_center, y_center)) {
        return lpxImage;
    }
    
    std::cerr << "ERROR: Multithreaded scan failed" << std::endl;
    return nullptr;
}

} // namespace lpx
