/**
 * lpx_vision_core.h
 * 
 * Core functionality for LPXVision library
 */

#ifndef LPX_VISION_CORE_H
#define LPX_VISION_CORE_H

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>

namespace lpx_vision {

/**
 * Core vision processing class
 */
class VisionCore {
public:
    VisionCore();
    ~VisionCore();

    /**
     * Initialize the vision system
     * @param width Image width
     * @param height Image height
     * @return true if initialization successful
     */
    bool initialize(int width, int height);

    /**
     * Process an image
     * @param input Input image
     * @param output Output image
     * @return true if processing successful
     */
    bool processImage(const cv::Mat& input, cv::Mat& output);

    /**
     * Get version information
     */
    std::string getVersion() const;

private:
    int width_;
    int height_;
    bool initialized_;
};

} // namespace lpx_vision

#endif // LPX_VISION_CORE_H
