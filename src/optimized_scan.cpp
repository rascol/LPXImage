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
        std::cout << "[OPTIMIZATION] Initialized scan cache with " << mapSize << " entries" << std::endl;
    }
    
    inline int getCellIndex(int pixelIdx) const {
        return (pixelIdx >= 0 && pixelIdx < mapSize) ? pixelToCellLUT[pixelIdx] : 0;
    }
};

// Global cache instance (initialized once per scan tables)
static ScanCache g_scanCache;

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
            
            // Atomic accumulation (lock-free)
            atomicAccR[iCell].fetch_add(color[2], std::memory_order_relaxed);
            atomicAccG[iCell].fetch_add(color[1], std::memory_order_relaxed);
            atomicAccB[iCell].fetch_add(color[0], std::memory_order_relaxed);
            atomicCount[iCell].fetch_add(1, std::memory_order_relaxed);
        }
    }
}

// High-performance multithreaded scan with optimizations
bool optimizedMultithreadedScan(LPXImage* lpxImage, const cv::Mat& image, float x_center, float y_center) {
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
                // Pack color: BGR format
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
    
    // Convert atomic results back to regular arrays and compute averages
    for (int i = 0; i < nMaxCells; i++) {
        const int pixelCount = atomicCount[i].load(std::memory_order_relaxed);
        if (pixelCount > 0) {
            const int r = atomicAccR[i].load(std::memory_order_relaxed) / pixelCount;
            const int g = atomicAccG[i].load(std::memory_order_relaxed) / pixelCount;
            const int b = atomicAccB[i].load(std::memory_order_relaxed) / pixelCount;
            cellArray[i] = b | (g << 8) | (r << 16);  // BGR format
        } else if (i > sct->lastFoveaIndex) {
            cellArray[i] = 0;  // Black for empty peripheral cells
        }
    }
    
    lpxImage->setLength(nMaxCells);
    
    auto totalEnd = std::chrono::high_resolution_clock::now();
    
    // Timing output
    auto resetDuration = std::chrono::duration_cast<std::chrono::microseconds>(foveaTime - resetTime).count();
    auto foveaDuration = std::chrono::duration_cast<std::chrono::microseconds>(foveaTime - resetTime).count();
    auto peripheralDuration = std::chrono::duration_cast<std::chrono::microseconds>(peripheralEnd - peripheralStart).count();
    auto colorDuration = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - colorStart).count();
    auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart).count();
    
    std::cout << "[OPTIMIZED-TIMING] Reset: " << resetDuration << "μs (" 
              << std::fixed << std::setprecision(2) << resetDuration/1000.0 << "ms)" << std::endl;
    std::cout << "[OPTIMIZED-TIMING] Fovea: " << foveaDuration << "μs (" 
              << foveaDuration/1000.0 << "ms)" << std::endl;
    std::cout << "[OPTIMIZED-TIMING] Peripheral: " << peripheralDuration << "μs (" 
              << peripheralDuration/1000.0 << "ms)" << std::endl;
    std::cout << "[OPTIMIZED-TIMING] Color: " << colorDuration << "μs (" 
              << colorDuration/1000.0 << "ms)" << std::endl;
    std::cout << "[OPTIMIZED-TIMING] TOTAL: " << totalDuration << "μs (" 
              << totalDuration/1000.0 << "ms)" << std::endl;
    
    return true;
}

} // namespace optimized
} // namespace lpx
