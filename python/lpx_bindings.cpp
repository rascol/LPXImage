/**
 * lpx_bindings.cpp
 * 
 * Python bindings for the LPXImage C++ library
 * Provides a clean Python interface without any JavaScript remnants
 */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "../include/lpx_image.h"
#include "../include/lpx_renderer.h"
#include "../include/lpx_mt.h"
#include "../include/lpx_webcam_server.h"
#include "../include/lpx_file_server.h"  // Include file server header
#include "../include/lpx_version.h"       // Include version header
#include <opencv2/opencv.hpp>
#include <cstring>
#include <iostream>

namespace py = pybind11;

// Helper function to convert OpenCV Mat to numpy array
py::array_t<uint8_t> mat_to_numpy(const cv::Mat& image) {
    // Always clone the Mat to ensure independent memory management
    cv::Mat cloned = image.clone();
    
    // Allocate new memory for numpy array
    auto result = py::array_t<uint8_t>(
        {cloned.rows, cloned.cols, cloned.channels()},    // shape
        {cloned.step[0], cloned.step[1], sizeof(uint8_t)}   // strides
    );
    
    // Copy data to numpy array
    std::memcpy(result.mutable_data(), cloned.data, cloned.total() * cloned.elemSize());
    
    return result;
}

// Helper function to convert numpy array to OpenCV Mat
cv::Mat numpy_to_mat(py::array_t<uint8_t, py::array::c_style>& array) {
    py::buffer_info info = array.request();
    
    // Check dimensions
    if (info.ndim != 3) 
        throw std::runtime_error("Array must have 3 dimensions (height, width, channels)");
    
    // Create Mat from numpy array
    cv::Mat mat(
        info.shape[0], info.shape[1], CV_8UC(info.shape[2]),
        info.ptr, info.strides[0]
    );
    
    return mat.clone(); // Return a clone to ensure memory safety
}

PYBIND11_MODULE(lpximage, m) {
    m.doc() = "Python bindings for LPX Image Processing Library";

    // Bind LPXTables class
    py::class_<lpx::LPXTables, std::shared_ptr<lpx::LPXTables>>(m, "LPXTables")
        .def(py::init<const std::string&>())
        .def("isInitialized", &lpx::LPXTables::isInitialized)
        .def_readonly("spiralPer", &lpx::LPXTables::spiralPer)
        .def_readonly("length", &lpx::LPXTables::length);

    // Bind LPXImage class
    py::class_<lpx::LPXImage, std::shared_ptr<lpx::LPXImage>>(m, "LPXImage")
        .def(py::init<std::shared_ptr<lpx::LPXTables>, int, int>())
        .def("getWidth", &lpx::LPXImage::getWidth)
        .def("getHeight", &lpx::LPXImage::getHeight)
        .def("getLength", &lpx::LPXImage::getLength)
        .def("getMaxCells", &lpx::LPXImage::getMaxCells)
        .def("getSpiralPeriod", &lpx::LPXImage::getSpiralPeriod)
        .def("getXOffset", &lpx::LPXImage::getXOffset)
        .def("getYOffset", &lpx::LPXImage::getYOffset)
        .def("setPosition", &lpx::LPXImage::setPosition)
        .def("saveToFile", &lpx::LPXImage::saveToFile)
        .def("loadFromFile", &lpx::LPXImage::loadFromFile);

    // Bind LPXRenderer class
    py::class_<lpx::LPXRenderer, std::shared_ptr<lpx::LPXRenderer>>(m, "LPXRenderer")
        .def(py::init<>())
        .def("setScanTables", &lpx::LPXRenderer::setScanTables)
        .def("renderToImage", [](lpx::LPXRenderer& self, 
                              std::shared_ptr<lpx::LPXImage> image,
                              int targetWidth, int targetHeight, float scale) {
            try {
                cv::Mat rendered = self.renderToImage(image, targetWidth, targetHeight, scale);
                if (rendered.empty()) {
                    throw std::runtime_error("Failed to render image - returned empty Mat");
                }
                return mat_to_numpy(rendered);
            } catch (const cv::Exception& e) {
                throw std::runtime_error("OpenCV error in renderToImage: " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw std::runtime_error("Error in renderToImage: " + std::string(e.what()));
            }
        });

    // Bind multithreaded scanning function
    m.def("scanImage", [](py::array_t<uint8_t, py::array::c_style>& input, float centerX, float centerY) {
        // Ensure global scan tables are set before calling the scan function
        if (!lpx::g_scanTables || !lpx::g_scanTables->isInitialized()) {
            throw std::runtime_error("Global scan tables not initialized. Call initLPX() first.");
        }

        cv::Mat inputMat = numpy_to_mat(input);
        auto result = lpx::multithreadedScanImage(inputMat, centerX, centerY);
        return result;
    }, py::arg("image"), py::arg("centerX"), py::arg("centerY"),
    "Scan an image and create an LPXImage using multithreaded processing");

    // Bind initialization function
    m.def("initLPX", [](const std::string& scanTableFile, int width, int height) {
        bool success = lpx::initLPX(scanTableFile, width, height);
        return success;
    }, py::arg("scanTableFile"), py::arg("width") = 640, py::arg("height") = 480,
    "Initialize the LPX system with scan tables");

    // Bind webcam server functionality
    py::class_<lpx::WebcamLPXServer>(m, "WebcamLPXServer")
        .def(py::init<const std::string&, int>(), 
             py::arg("scanTableFile"), py::arg("port") = 5050)
        .def("start", &lpx::WebcamLPXServer::start,
             py::arg("cameraId") = 0, py::arg("width") = 640, py::arg("height") = 480)
        .def("stop", &lpx::WebcamLPXServer::stop)
        .def("setSkipRate", &lpx::WebcamLPXServer::setSkipRate)
        .def("getClientCount", &lpx::WebcamLPXServer::getClientCount);

    // Bind file server functionality
    py::class_<lpx::FileLPXServer>(m, "FileLPXServer")
        .def(py::init<const std::string&, int>(), 
             py::arg("scanTableFile"), py::arg("port") = 5050)
        .def("start", [](lpx::FileLPXServer& self, const std::string& videoFile, int width, int height) {
            return self.start(videoFile, width, height);
        }, py::arg("videoFile"), py::arg("width") = 1920, py::arg("height") = 1080)
        .def("stop", &lpx::FileLPXServer::stop)
        .def("setFPS", &lpx::FileLPXServer::setFPS)
        .def("getFPS", &lpx::FileLPXServer::getFPS)
        .def("setLooping", &lpx::FileLPXServer::setLooping)
        .def("isLooping", &lpx::FileLPXServer::isLooping)
        .def("setCenterOffset", &lpx::FileLPXServer::setCenterOffset)
        .def("getClientCount", &lpx::FileLPXServer::getClientCount);

    // Bind debug client functionality
    py::class_<lpx::LPXDebugClient>(m, "LPXDebugClient")
        .def(py::init<const std::string&>())
        .def("connect", [](lpx::LPXDebugClient& self, const std::string& serverAddress) {
            try {
                return self.connect(serverAddress, 5050);
            } catch (const std::exception& e) {
                throw std::runtime_error("Connection failed: " + std::string(e.what()));
            }
        }, py::arg("serverAddress"))
        .def("disconnect", [](lpx::LPXDebugClient& self) {
            try {
                self.disconnect();
            } catch (const std::exception& e) {
                // Log but don't throw on disconnect - allow cleanup to continue
                std::cerr << "Warning during disconnect: " << e.what() << std::endl;
            }
        })
        .def("setWindowTitle", &lpx::LPXDebugClient::setWindowTitle)
        .def("setWindowSize", &lpx::LPXDebugClient::setWindowSize)
        .def("setScale", &lpx::LPXDebugClient::setScale)
        .def("initializeWindow", [](lpx::LPXDebugClient& self) {
            try {
                self.initializeWindow();
            } catch (const cv::Exception& e) {
                throw std::runtime_error("OpenCV error initializing window: " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw std::runtime_error("Error initializing window: " + std::string(e.what()));
            }
        })
        .def("processEvents", [](lpx::LPXDebugClient& self) {
            try {
                bool result = self.processEvents();
                return result;
            } catch (const cv::Exception& e) {
                std::cerr << "OpenCV error in processEvents: " << e.what() << std::endl;
                // Don't throw - return false to indicate failure
                return false;
            } catch (const std::runtime_error& e) {
                std::cerr << "Runtime error in processEvents: " << e.what() << std::endl;
                return false;
            } catch (const std::exception& e) {
                std::cerr << "Error in processEvents: " << e.what() << std::endl;
                return false;
            } catch (...) {
                std::cerr << "Unknown error in processEvents" << std::endl;
                return false;
            }
        })
        .def("isRunning", &lpx::LPXDebugClient::isRunning)
        .def("sendMovementCommand", [](lpx::LPXDebugClient& self, float deltaX, float deltaY, float stepSize) {
            try {
                return self.sendMovementCommand(deltaX, deltaY, stepSize);
            } catch (const std::exception& e) {
                std::cerr << "Error sending movement command: " << e.what() << std::endl;
                return false;
            }
        }, py::arg("deltaX"), py::arg("deltaY"), py::arg("stepSize") = 10.0f);
    
    // Version information functions - timestamp-based versioning
    m.def("getVersionString", &lpx::getVersionString, "Get version string with build timestamp");
    m.def("getBuildTimestamp", &lpx::getBuildTimestamp, "Get full build timestamp (date and time)");
    m.def("getBuildDate", &lpx::getBuildDate, "Get build date only");
    m.def("getBuildTime", &lpx::getBuildTime, "Get build time only");
    m.def("getBuildNumber", &lpx::getBuildNumber, "Get build number (hash of timestamp for legacy compatibility)");
    m.def("getKeyThrottleMs", &lpx::getKeyThrottleMs, "Get key throttle milliseconds");
    m.def("printBuildInfo", &lpx::printBuildInfo, "Print build information");
}
