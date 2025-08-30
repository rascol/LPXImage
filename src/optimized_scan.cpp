/**
 * optimized_scan.cpp
 * 
 * High-performance optimized implementation of log-polar scanning
 * Targets sub-20ms processing times through several optimization techniques
 */

#include "../include/lpx_mt.h"
#include "../include/lpx_common.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <future>
#include <atomic>
#include <cstdlib>  // For getenv
#include <cmath>    // For sin, cos

namespace lpx {
namespace optimized {

// Cache-friendly lookup table for binary search results
struct ScanCache {
    std::vector<int> pixelToCellLUT;  // Direct lookup table: pixel index -> cell index
    int mapSize;
    bool initialized = false;
    
    void initialize(const std::shared_ptr<LPXTables>& sct) {
        if (initialized) return;
        
        mapSize = sct->mapWidth * sct->mapWidth;
        pixelToCellLUT.resize(mapSize, -1);
        
        // Pre-compute all pixel-to-cell mappings
        for (int i = 0; i < sct->length; i++) {
            int pixelIdx = sct->outerPixelIndex[i];
            int cellIdx = sct->outerPixelCellIdx[i];
            
            if (pixelIdx >= 0 && pixelIdx < mapSize) {
                pixelToCellLUT[pixelIdx] = cellIdx;
            }
        }
        
        // Fill gaps with nearest valid cell index
        int lastValidCell = sct->lastFoveaIndex;
        for (int i = 0; i < mapSize; i++) {
            if (pixelToCellLUT[i] == -1) {
                pixelToCellLUT[i] = lastValidCell;
            } else {
                lastValidCell = pixelToCellLUT[i];
            }
        }
        
        initialized = true;
        // Initialization complete
    }
    
    inline int getCellIndex(int pixelIdx) const {
        return (pixelIdx >= 0 && pixelIdx < mapSize) ? pixelToCellLUT[pixelIdx] : 0;
    }
};

// Global cache instance (initialized once per scan tables)
static ScanCache g_scanCache;

// Generate rainbow colors based on log-polar coordinates for smooth visual transitions
uint32_t generateRainbowColor(int cellIndex, float spiralPer) {
    // Convert cell index to approximate log-polar coordinates
    // This creates a smoother visual pattern that follows the spiral sampling
    
    if (cellIndex <= 0) {
        // Center is red
        return 0x0000FF; // Red in BGR format
    }
    
    // Calculate approximate radius and angle from cell index
    // This is a simplified approximation of the log-polar mapping
    const float radius = std::log(static_cast<float>(cellIndex) / spiralPer + 1.0f);
    const float angle = static_cast<float>(cellIndex) / spiralPer * 2.0f * M_PI;
    
    // Create a smooth color transition based on radius and angle
    // Use radius for primary hue progression and angle for variation
    float hue = fmod(radius * 2.0f + angle * 0.1f, 6.28f); // 2*PI
    if (hue < 0) hue += 6.28f;
    
    // Normalize hue to [0, 1] and map to color spectrum
    hue = hue / 6.28f;
    
    // Generate HSV color with varying hue
    const float saturation = 1.0f;
    const float value = 1.0f;
    
    float r, g, b;
    
    // Convert HSV to RGB
    const float hue_scaled = hue * 6.0f;
    const int h_i = static_cast<int>(hue_scaled) % 6;
    const float f = hue_scaled - static_cast<int>(hue_scaled);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - saturation * f);
    const float t_val = value * (1.0f - saturation * (1.0f - f));
    
    switch (h_i) {
        case 0: r = value; g = t_val; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t_val; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t_val; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
    }
    
    // Convert to 8-bit integers and pack as BGR
    const int r_int = static_cast<int>(r * 255.0f);
    const int g_int = static_cast<int>(g * 255.0f);
    const int b_int = static_cast<int>(b * 255.0f);
    
    return b_int | (g_int << 8) | (r_int << 16);
}

// Check if rainbow mode is enabled
bool isRainbowModeEnabled() {
    const char* env_var = std::getenv("LPX_RAINBOW_MODE");
    return (env_var != nullptr && (std::string(env_var) == "1" || std::string(env_var) == "true"));
}

// Optimized region processing with minimal overhead
void optimizedProcessImageRegion(const cv::Mat& image, int yStart, int yEnd,
                               float centerX, float centerY,
                               const ScanCache& cache,
                               int scanMapCenterX, int scanMapCenterY,
                               int w_m, int lastFoveaIndex,
                               std::vector<std::atomic<int>>& atomicAccR,
                               std::vector<std::atomic<int>>& atomicAccG,
                               std::vector<std::atomic<int>>& atomicAccB,
                               std::vector<std::atomic<int>>& atomicCount) {
    
    // Pre-calculate offsets to avoid repeated computation
    const int j_ofs = static_cast<int>(centerX);
    const int k_ofs = static_cast<int>(centerY);
    const int ws_wm_jofs = scanMapCenterX - j_ofs;
    const int hs_hm_kofs = scanMapCenterY - k_ofs;
    
    // Process in chunks to improve cache locality
    const int cols = image.cols;
    const bool is3Channel = (image.channels() == 3);
    
    for (int k_s = yStart; k_s < yEnd; k_s++) {
        const int i_m_base = ws_wm_jofs + w_m * (hs_hm_kofs + k_s);
        
        // Get pointer to current row for faster access
        const cv::Vec3b* imageRow = nullptr;
        const uchar* grayRow = nullptr;
        
        if (is3Channel) {
            imageRow = image.ptr<cv::Vec3b>(k_s);
        } else {
            grayRow = image.ptr<uchar>(k_s);
        }
        
        // Process pixels in chunks for better cache performance
        for (int j_s = 0; j_s < cols; j_s++) {
            const int i_m = i_m_base + j_s;
            
            // Bounds check with early exit
            if (i_m < 0 || i_m >= cache.mapSize) continue;
            
            // Fast lookup instead of binary search
            const int iCell = cache.getCellIndex(i_m);
            
            // Skip fovea cells (already processed)
            if (iCell <= lastFoveaIndex) continue;
            
            // Fast pixel access
            cv::Vec3b color;
            if (is3Channel) {
                color = imageRow[j_s];
            } else {
                const uchar intensity = grayRow[j_s];
                color = cv::Vec3b(intensity, intensity, intensity);
            }
            
            // Atomic accumulation in BGR order (OpenCV's native format)
            atomicAccR[iCell].fetch_add(color[2], std::memory_order_relaxed);  // R channel
            atomicAccG[iCell].fetch_add(color[1], std::memory_order_relaxed);  // G channel
            atomicAccB[iCell].fetch_add(color[0], std::memory_order_relaxed);  // B channel
            atomicCount[iCell].fetch_add(1, std::memory_order_relaxed);
        }
    }
}

// High-performance multithreaded scan with optimizations
bool optimizedMultithreadedScan(LPXImage* lpxImage, const cv::Mat& image, float x_center, float y_center) {
    // Starting optimized multithreaded scan
    auto totalStart = std::chrono::high_resolution_clock::now();
    
    auto sct = lpxImage->getScanTables();
    if (!sct || !sct->isInitialized() || image.empty()) {
        return false;
    }
    
    // Initialize cache if needed
    g_scanCache.initialize(sct);
    
    // Get direct access to arrays
    auto& cellArray = lpxImage->accessCellArray();
    auto& accR = lpxImage->accessAccR();
    auto& accG = lpxImage->accessAccG();
    auto& accB = lpxImage->accessAccB();
    auto& count = lpxImage->accessCount();
    
    const int nMaxCells = lpxImage->getMaxCells();
    
    // Set position
    lpxImage->setPosition(x_center, y_center);
    
    // Fast reset using memset
    std::fill(accR.begin(), accR.end(), 0);
    std::fill(accG.begin(), accG.end(), 0);
    std::fill(accB.begin(), accB.end(), 0);
    std::fill(count.begin(), count.end(), 0);
    
    auto resetTime = std::chrono::high_resolution_clock::now();
    
    // STEP 1: Fast fovea processing (minimal overhead)
    const int w_m = sct->mapWidth;
    const int scanMapCenterX = w_m / 2;
    const int scanMapCenterY = w_m / 2;
    
    // Process fovea with minimal function call overhead
    for (int i = 0; i < sct->innerLength; i++) {
        const int x = static_cast<int>(x_center + sct->innerCells[i].x - scanMapCenterX);
        const int y = static_cast<int>(y_center + sct->innerCells[i].y - scanMapCenterY);
        
        if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
            cv::Vec3b color;
            if (image.channels() == 3) {
                color = image.at<cv::Vec3b>(y, x);
            } else {
                const uchar intensity = image.at<uchar>(y, x);
                color = cv::Vec3b(intensity, intensity, intensity);
            }
            
            const int cellIndex = (i <= sct->lastFoveaIndex && i < static_cast<int>(cellArray.size())) ? i : sct->outerPixelCellIdx[i];
            if (cellIndex >= 0 && cellIndex < static_cast<int>(cellArray.size())) {
                // Pack color in BGR format (OpenCV's native format)
                cellArray[cellIndex] = (color[0]) | (color[1] << 8) | (color[2] << 16);
            }
        }
    }
    
    auto foveaTime = std::chrono::high_resolution_clock::now();
    
    // STEP 2: Optimized peripheral processing with lock-free atomics
    auto peripheralStart = std::chrono::high_resolution_clock::now();
    
    // Use atomic accumulators to eliminate mutex overhead
    std::vector<std::atomic<int>> atomicAccR(nMaxCells);
    std::vector<std::atomic<int>> atomicAccG(nMaxCells);
    std::vector<std::atomic<int>> atomicAccB(nMaxCells);
    std::vector<std::atomic<int>> atomicCount(nMaxCells);
    
    // Initialize atomics
    for (int i = 0; i < nMaxCells; i++) {
        atomicAccR[i].store(0, std::memory_order_relaxed);
        atomicAccG[i].store(0, std::memory_order_relaxed);
        atomicAccB[i].store(0, std::memory_order_relaxed);
        atomicCount[i].store(0, std::memory_order_relaxed);
    }
    
    // Calculate processing bounds
    const float spiralRadius = getSpiralRadius(nMaxCells, sct->spiralPer);
    const int spRad = static_cast<int>(spiralRadius + 0.5f);
    
    int yMin = std::max(0, static_cast<int>(y_center - spRad));
    int yMax = std::min(image.rows, static_cast<int>(y_center + spRad));
    
    // Use optimal number of threads (avoid oversubscription)
    const unsigned int numThreads = std::min(4u, std::thread::hardware_concurrency());
    const int rowsPerThread = (yMax - yMin) / numThreads;
    
    if (rowsPerThread > 10) {  // Only use multithreading for significant work
        std::vector<std::future<void>> futures;
        
        for (unsigned int t = 0; t < numThreads; t++) {
            const int startRow = yMin + t * rowsPerThread;
            const int endRow = (t == numThreads - 1) ? yMax : startRow + rowsPerThread;
            
            futures.push_back(std::async(std::launch::async,
                optimizedProcessImageRegion,
                std::ref(image), startRow, endRow,
                x_center, y_center,
                std::ref(g_scanCache),
                scanMapCenterX, scanMapCenterY,
                w_m, sct->lastFoveaIndex,
                std::ref(atomicAccR), std::ref(atomicAccG),
                std::ref(atomicAccB), std::ref(atomicCount)
            ));
        }
        
        // Wait for completion
        for (auto& future : futures) {
            future.wait();
        }
    } else {
        // Single-threaded for small workloads
        std::mutex dummyMutex;  // Not used in optimized version
        optimizedProcessImageRegion(image, yMin, yMax,
                                  x_center, y_center,
                                  g_scanCache,
                                  scanMapCenterX, scanMapCenterY,
                                  w_m, sct->lastFoveaIndex,
                                  atomicAccR, atomicAccG,
                                  atomicAccB, atomicCount);
    }
    
    auto peripheralEnd = std::chrono::high_resolution_clock::now();
    
    // STEP 3: Fast color computation
    auto colorStart = std::chrono::high_resolution_clock::now();
    
    // Check if rainbow mode is enabled
    const bool rainbowMode = isRainbowModeEnabled();
    // Rainbow mode check completed
    
    // Convert atomic results back to regular arrays and compute averages
    for (int i = 0; i < nMaxCells; i++) {
        if (rainbowMode) {
            // Generate rainbow pattern that repeats every viewlength
            cellArray[i] = generateRainbowColor(i, 1323);
        } else {
            // Normal color computation
            const int pixelCount = atomicCount[i].load(std::memory_order_relaxed);
            if (pixelCount > 0) {
                const int r = atomicAccR[i].load(std::memory_order_relaxed) / pixelCount;
                const int g = atomicAccG[i].load(std::memory_order_relaxed) / pixelCount;
                const int b = atomicAccB[i].load(std::memory_order_relaxed) / pixelCount;
                // Pack in BGR format (OpenCV's native format)
                cellArray[i] = b | (g << 8) | (r << 16);  // BGR format
            } else if (i > sct->lastFoveaIndex) {
                cellArray[i] = 0;  // Black for empty peripheral cells
            }
        }
    }
    
    lpxImage->setLength(nMaxCells);
    
    auto totalEnd = std::chrono::high_resolution_clock::now();
    
    // Timing calculations removed
    
    return true;
}

} // namespace optimized
} // namespace lpx
