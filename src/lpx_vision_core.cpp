/**
 * lpx_vision_core.cpp
 * 
 * Implementation of core functionality for LPXVision library
 */

#include "lpx_vision.h"
#include "lpx_vision_core.h"
#include "lpx_vision_utils.h"
#include <iostream>

namespace lpx_vision {

VisionCore::VisionCore() 
    : width_(0), height_(0), initialized_(false) {
}

VisionCore::~VisionCore() {
}

bool VisionCore::initialize(int width, int height) {
    if (width <= 0 || height <= 0) {
        utils::logMessage("Invalid dimensions provided to VisionCore::initialize");
        return false;
    }
    
    width_ = width;
    height_ = height;
    initialized_ = true;
    
    utils::logMessage("VisionCore initialized with dimensions: " + 
                     std::to_string(width) + "x" + std::to_string(height));
    
    return true;
}

bool VisionCore::processImage(const cv::Mat& input, cv::Mat& output) {
    if (!initialized_) {
        utils::logMessage("VisionCore not initialized");
        return false;
    }
    
    if (input.empty()) {
        utils::logMessage("Empty input image provided");
        return false;
    }
    
    // Placeholder implementation - just copy the input to output
    // This is where you'll add JavaScript-converted algorithms
    input.copyTo(output);
    
    utils::logMessage("Image processed successfully");
    return true;
}

std::string VisionCore::getVersion() const {
    return "0.1.0";
}

} // namespace lpx_vision
