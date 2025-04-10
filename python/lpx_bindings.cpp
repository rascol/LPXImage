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
#include <opencv2/opencv.hpp>

namespace py = pybind11;

// Helper function to convert OpenCV Mat to numpy array
py::array_t<uint8_t> mat_to_numpy(const cv::Mat& image) {
    // Create a Python object that will free the allocated memory when destroyed
    py::capsule free_when_done(image.data, [](void* f) {
        // This lambda function does nothing since OpenCV will free the memory
    });

    // Return numpy array with the data from the Mat
    return py::array_t<uint8_t>(
        {image.rows, image.cols, image.channels()},    // shape
        {image.step[0], image.step[1], sizeof(uint8_t)},   // strides
        image.data,   // pointer to data
        free_when_done   // capsule with deleter
    );
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
            cv::Mat rendered = self.renderToImage(image, targetWidth, targetHeight, scale);
            return mat_to_numpy(rendered);
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

    // Bind debug client functionality
    py::class_<lpx::LPXDebugClient>(m, "LPXDebugClient")
        .def(py::init<const std::string&>())
        .def("connect", [](lpx::LPXDebugClient& self, const std::string& serverAddress, int port) {
            try {
                bool success = self.connect(serverAddress, port);
                if (!success) {
                    std::cerr << "LPXDebugClient: Connection failed" << std::endl;
                }
                return success;
            } catch (const std::exception& e) {
                std::cerr << "LPXDebugClient: Exception during connect: " << e.what() << std::endl;
                throw;
            }
        }, py::arg("serverAddress"), py::arg("port") = 5050)
        .def("disconnect", &lpx::LPXDebugClient::disconnect)
        .def("setWindowTitle", &lpx::LPXDebugClient::setWindowTitle)
        .def("setWindowSize", &lpx::LPXDebugClient::setWindowSize)
        .def("setScale", &lpx::LPXDebugClient::setScale)
        .def("initializeWindow", &lpx::LPXDebugClient::initializeWindow)
        .def("processEvents", &lpx::LPXDebugClient::processEvents)
        .def("isRunning", &lpx::LPXDebugClient::isRunning);
}