#!/bin/bash
# A simpler performance test script that redirects output to /dev/null

# Make script executable
chmod +x "$0"

# Create build directory
mkdir -p build_perf
cd build_perf

# Create a simple performance test program
cat > perf_test.cpp << EOF
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <fstream>
#include "../include/lpx_common.h"
#include "../include/lpx_image.h"
#include "../include/lpx_mt.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <scan_tables_file> <image_file>" << std::endl;
        return -1;
    }

    // Redirect stdout to null for performance testing
    std::ofstream nullStream("/dev/null");
    std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
    std::cout.rdbuf(nullStream.rdbuf());

    // Keep stderr for our performance measurements
    
    // Load scan tables
    if (!lpx::initLPX(argv[1], 0, 0)) {
        std::cerr << "Failed to load scan tables" << std::endl;
        return -1;
    }

    // Load image
    cv::Mat image = cv::imread(argv[2]);
    if (image.empty()) {
        std::cerr << "Failed to load image: " << argv[2] << std::endl;
        return -1;
    }
    
    std::cerr << "Image loaded: " << image.cols << "x" << image.rows << std::endl;

    // Run serial version 5 times and take average
    double total_serial_ms = 0;
    const int num_runs = 5;
    
    std::cerr << "Running " << num_runs << " iterations of each implementation..." << std::endl;
    
    for (int i = 0; i < num_runs; i++) {
        auto startTime = std::chrono::high_resolution_clock::now();
        auto lpxImage = lpx::scanImage(image, image.cols / 2.0f, image.rows / 2.0f);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        total_serial_ms += duration;
        
        std::cerr << "  Serial run " << (i+1) << ": " << duration << " ms" << std::endl;
    }
    
    double avg_serial_ms = total_serial_ms / num_runs;
    std::cerr << "Average serial scan time: " << avg_serial_ms << " ms" << std::endl;
    
    // Run multi-threaded version 5 times and take average
    double total_mt_ms = 0;
    
    for (int i = 0; i < num_runs; i++) {
        auto startTime = std::chrono::high_resolution_clock::now();
        auto lpxImage = lpx::multithreadedScanImage(image, image.cols / 2.0f, image.rows / 2.0f);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        total_mt_ms += duration;
        
        std::cerr << "  Multi-threaded run " << (i+1) << ": " << duration << " ms" << std::endl;
    }
    
    double avg_mt_ms = total_mt_ms / num_runs;
    std::cerr << "Average multi-threaded scan time: " << avg_mt_ms << " ms" << std::endl;
    
    // Calculate speedup
    double speedup = avg_serial_ms / avg_mt_ms;
    std::cerr << "\n=== PERFORMANCE RESULTS ===" << std::endl;
    std::cerr << "Serial:          " << avg_serial_ms << " ms" << std::endl;
    std::cerr << "Multi-threaded:  " << avg_mt_ms << " ms" << std::endl;
    std::cerr << "Speedup:         " << speedup << "x" << std::endl;

    // Restore stdout
    std::cout.rdbuf(oldCoutStreamBuf);
    return 0;
}
EOF

# Create a CMakeLists.txt for our tests
cat > CMakeLists.txt << EOF
cmake_minimum_required(VERSION 3.10)
project(lpx_perf_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(\${OpenCV_INCLUDE_DIRS})

# Include parent project
include_directories(../include)

# Add executablegi
add_executable(perf_test perf_test.cpp)

# Link to the main library
target_link_libraries(perf_test \${OpenCV_LIBS})

# Add library from parent project
add_library(lpx_image SHARED IMPORTED)
set_target_properties(lpx_image PROPERTIES IMPORTED_LOCATION ../build/liblpx_image.dylib)
target_link_libraries(perf_test lpx_image)
EOF

# Build and run
echo "Building performance test..."
cmake .
make

echo "Running performance test with stdout redirected to /dev/null..."
./perf_test ../ScanTables63 ../lion.jpg

echo "Performance test complete."
