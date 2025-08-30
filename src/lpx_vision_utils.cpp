/**
 * lpx_vision_utils.cpp
 * 
 * Implementation of utility functions for LPXVision library
 */

#include "lpx_vision_utils.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace lpx_vision {
namespace utils {

bool convertImageFormat(const cv::Mat& input, cv::Mat& output, int format) {
    if (input.empty()) {
        logMessage("Empty input image provided to convertImageFormat");
        return false;
    }
    
    try {
        cv::cvtColor(input, output, format);
        return true;
    } catch (const cv::Exception& e) {
        logMessage("OpenCV error in convertImageFormat: " + std::string(e.what()));
        return false;
    }
}

bool resizeImageKeepAspect(const cv::Mat& input, cv::Mat& output, int maxWidth, int maxHeight) {
    if (input.empty()) {
        logMessage("Empty input image provided to resizeImageKeepAspect");
        return false;
    }
    
    if (maxWidth <= 0 || maxHeight <= 0) {
        logMessage("Invalid dimensions provided to resizeImageKeepAspect");
        return false;
    }
    
    double scaleX = static_cast<double>(maxWidth) / input.cols;
    double scaleY = static_cast<double>(maxHeight) / input.rows;
    double scale = std::min(scaleX, scaleY);
    
    int newWidth = static_cast<int>(input.cols * scale);
    int newHeight = static_cast<int>(input.rows * scale);
    
    try {
        cv::resize(input, output, cv::Size(newWidth, newHeight));
        return true;
    } catch (const cv::Exception& e) {
        logMessage("OpenCV error in resizeImageKeepAspect: " + std::string(e.what()));
        return false;
    }
}

std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

void logMessage(const std::string& message) {
    std::cout << "[" << getTimestamp() << "] " << message << std::endl;
}

} // namespace utils
} // namespace lpx_vision
