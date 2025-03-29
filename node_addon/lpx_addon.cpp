/**
 * lpx_addon.cpp
 * 
 * Node.js addon for Log-Polar Image transformation
 */

#include <napi.h>
#include <opencv2/opencv.hpp>
#include "../include/lpx_image.h"

// Global webcam capture
cv::VideoCapture g_capture;

// Initialize the log-polar system
Napi::Boolean InitLPX(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected for scan tables file path").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    
    std::string scanTablesFile = info[0].As<Napi::String>().Utf8Value();
    int width = 0;
    int height = 0;
    
    if (info.Length() >= 3) {
        if (info[1].IsNumber()) width = info[1].As<Napi::Number>().Int32Value();
        if (info[2].IsNumber()) height = info[2].As<Napi::Number>().Int32Value();
    }
    
    bool result = lpx::initLPX(scanTablesFile, width, height);
    return Napi::Boolean::New(env, result);
}

// Shutdown the log-polar system
void ShutdownLPX(const Napi::CallbackInfo& info) {
    lpx::shutdownLPX();
}

// Start the webcam capture
Napi::Boolean StartCamera(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    int deviceId = 0;
    if (info.Length() >= 1 && info[0].IsNumber()) {
        deviceId = info[0].As<Napi::Number>().Int32Value();
    }
    
    // Close previous capture if open
    if (g_capture.isOpened()) {
        g_capture.release();
    }
    
    // Open the webcam
    g_capture.open(deviceId);
    if (!g_capture.isOpened()) {
        return Napi::Boolean::New(env, false);
    }
    
    return Napi::Boolean::New(env, true);
}

// Stop the webcam capture
void StopCamera(const Napi::CallbackInfo& info) {
    if (g_capture.isOpened()) {
        g_capture.release();
    }
}

// Capture a frame from the webcam and convert to log-polar
Napi::Buffer<uint8_t> CaptureFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (!g_capture.isOpened()) {
        Napi::Error::New(env, "Camera not initialized").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    float centerX = 0.5f;
    float centerY = 0.5f;
    
    if (info.Length() >= 2) {
        if (info[0].IsNumber()) centerX = info[0].As<Napi::Number>().FloatValue();
        if (info[1].IsNumber()) centerY = info[1].As<Napi::Number>().FloatValue();
    }
    
    // Capture frame from webcam
    cv::Mat frame;
    g_capture >> frame;
    
    if (frame.empty()) {
        Napi::Error::New(env, "Failed to capture frame").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    // Convert relative coordinates to absolute coordinates
    centerX *= frame.cols;
    centerY *= frame.rows;
    
    // Convert to log-polar
    auto lpxImage = lpx::scanImage(frame, centerX, centerY);
    if (!lpxImage) {
        Napi::Error::New(env, "Failed to convert to log-polar").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    // Get raw data
    size_t dataSize = lpxImage->getRawDataSize();
    const uint8_t* data = lpxImage->getRawData();
    
    // Create a copy of the data for the return value
    uint8_t* dataCopy = new uint8_t[dataSize];
    std::memcpy(dataCopy, data, dataSize);
    
    // Return as Node.js buffer (with freeing callback)
    return Napi::Buffer<uint8_t>::New(
        env, 
        dataCopy, 
        dataSize, 
        [](Napi::Env env, uint8_t* data) {
            delete[] data;
        }
    );
}

// Convert a standard image file to log-polar format
Napi::Buffer<uint8_t> ConvertImageFile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected for image file path").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    std::string imageFile = info[0].As<Napi::String>().Utf8Value();
    
    float centerX = 0.5f;
    float centerY = 0.5f;
    
    if (info.Length() >= 3) {
        if (info[1].IsNumber()) centerX = info[1].As<Napi::Number>().FloatValue();
        if (info[2].IsNumber()) centerY = info[2].As<Napi::Number>().FloatValue();
    }
    
    // Load the image
    cv::Mat image = cv::imread(imageFile);
    if (image.empty()) {
        Napi::Error::New(env, "Failed to load image").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    // Convert relative coordinates to absolute coordinates
    centerX *= image.cols;
    centerY *= image.rows;
    
    // Convert to log-polar
    auto lpxImage = lpx::scanImage(image, centerX, centerY);
    if (!lpxImage) {
        Napi::Error::New(env, "Failed to convert to log-polar").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    // Get raw data
    size_t dataSize = lpxImage->getRawDataSize();
    const uint8_t* data = lpxImage->getRawData();
    
    // Create a copy of the data for the return value
    uint8_t* dataCopy = new uint8_t[dataSize];
    std::memcpy(dataCopy, data, dataSize);
    
    // Return as Node.js buffer (with freeing callback)
    return Napi::Buffer<uint8_t>::New(
        env, 
        dataCopy, 
        dataSize, 
        [](Napi::Env env, uint8_t* data) {
            delete[] data;
        }
    );
}

// Render a log-polar image to a standard image
Napi::Buffer<uint8_t> RenderLogPolarImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer expected for log-polar data").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
    uint8_t* data = buffer.Data();
    size_t dataSize = buffer.Length();
    
    int width = 640;
    int height = 480;
    
    if (info.Length() >= 3) {
        if (info[1].IsNumber()) width = info[1].As<Napi::Number>().Int32Value();
        if (info[2].IsNumber()) height = info[2].As<Napi::Number>().Int32Value();
    }
    
    // Create LPXImage from data
    auto lpxImage = std::make_shared<lpx::LPXImage>(nullptr, width, height);
    
    // TODO: Implement a method to load the LPXImage from raw data
    // For now, we'll just render a placeholder
    
    // Render to standard image
    cv::Mat outputImage = lpxImage->renderToImage(width, height);
    if (outputImage.empty()) {
        Napi::Error::New(env, "Failed to render log-polar image").ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
    }
    
    // Encode the image to JPEG
    std::vector<uint8_t> buffer_jpeg;
    cv::imencode(".jpg", outputImage, buffer_jpeg);
    
    // Create a copy of the data for the return value
    uint8_t* dataCopy = new uint8_t[buffer_jpeg.size()];
    std::memcpy(dataCopy, buffer_jpeg.data(), buffer_jpeg.size());
    
    // Return as Node.js buffer (with freeing callback)
    return Napi::Buffer<uint8_t>::New(
        env, 
        dataCopy, 
        buffer_jpeg.size(), 
        [](Napi::Env env, uint8_t* data) {
            delete[] data;
        }
    );
}

// Initialize the addon
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("initLPX", Napi::Function::New(env, InitLPX));
    exports.Set("shutdownLPX", Napi::Function::New(env, ShutdownLPX));
    exports.Set("startCamera", Napi::Function::New(env, StartCamera));
    exports.Set("stopCamera", Napi::Function::New(env, StopCamera));
    exports.Set("captureFrame", Napi::Function::New(env, CaptureFrame));
    exports.Set("convertImageFile", Napi::Function::New(env, ConvertImageFile));
    exports.Set("renderLogPolarImage", Napi::Function::New(env, RenderLogPolarImage));
    return exports;
}

NODE_API_MODULE(lpx_addon, Init)
