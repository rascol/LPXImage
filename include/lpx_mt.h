/**
 * lpx_mt.h
 * 
 * Header file for multithreaded Log-Polar Image operations
 */

#ifndef LPX_MT_H
#define LPX_MT_H

#include "lpx_image.h"
#include <mutex>

namespace lpx {

// Function to scan an image in a multithreaded fashion
bool multithreadedScanFromImage(LPXImage* lpxImage, const cv::Mat& image, float x_center, float y_center);

// Helper function to create and scan an image in one go
std::shared_ptr<LPXImage> multithreadedScanImage(const cv::Mat& image, float x_center, float y_center);

// Internal helper functions
namespace internal {
    // Process a portion of the image for multi-threaded scan
    void processImageRegion(const cv::Mat& image, int yStart, int yEnd, 
                        float centerX, float centerY, 
                        std::shared_ptr<LPXTables> sct,
                        std::vector<int>& accR, std::vector<int>& accG, 
                        std::vector<int>& accB, std::vector<int>& count,
                        std::mutex& accMutex);
}

} // namespace lpx

#endif // LPX_MT_H
