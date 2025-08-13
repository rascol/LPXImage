/**
 * lpx_optimized.h
 * 
 * Header file for optimized Log-Polar Image operations
 * High-performance implementations targeting sub-20ms scan times
 */

#ifndef LPX_OPTIMIZED_H
#define LPX_OPTIMIZED_H

#include "lpx_image.h"
#include <atomic>

namespace lpx {
namespace optimized {

// Cache structure for fast pixel-to-cell lookup
struct ScanCache {
    std::vector<int> pixelToCellLUT;  // Direct lookup table: pixel index -> cell index
    int mapSize;
    bool initialized = false;
    
    void initialize(const std::shared_ptr<LPXTables>& sct);
    inline int getCellIndex(int pixelIdx) const;
};

// High-performance optimized scanning function
bool optimizedMultithreadedScan(LPXImage* lpxImage, const cv::Mat& image, float x_center, float y_center);

// Optimized region processing with minimal overhead
void optimizedProcessImageRegion(const cv::Mat& image, int yStart, int yEnd,
                               float centerX, float centerY,
                               const ScanCache& cache,
                               int scanMapCenterX, int scanMapCenterY,
                               int w_m, int lastFoveaIndex,
                               std::vector<std::atomic<int>>& atomicAccR,
                               std::vector<std::atomic<int>>& atomicAccG,
                               std::vector<std::atomic<int>>& atomicAccB,
                               std::vector<std::atomic<int>>& atomicCount);

} // namespace optimized
} // namespace lpx

#endif // LPX_OPTIMIZED_H
