/**
 * lpx_vision_utils.h
 * 
 * Utility functions for LPXVision library
 */

#ifndef LPX_VISION_UTILS_H
#define LPX_VISION_UTILS_H

#include <opencv2/opencv.hpp>
#include <string>

namespace lpx_vision {
namespace utils {

/**
 * Convert image format
 * @param input Input image
 * @param output Output image
 * @param format Target format
 * @return true if conversion successful
 */
bool convertImageFormat(const cv::Mat& input, cv::Mat& output, int format);

/**
 * Resize image with aspect ratio preservation
 * @param input Input image
 * @param output Output image
 * @param maxWidth Maximum width
 * @param maxHeight Maximum height
 * @return true if resize successful
 */
bool resizeImageKeepAspect(const cv::Mat& input, cv::Mat& output, int maxWidth, int maxHeight);

/**
 * Get timestamp string
 * @return Current timestamp as string
 */
std::string getTimestamp();

/**
 * Log message with timestamp
 * @param message Message to log
 */
void logMessage(const std::string& message);

} // namespace utils
} // namespace lpx_vision

#endif // LPX_VISION_UTILS_H
