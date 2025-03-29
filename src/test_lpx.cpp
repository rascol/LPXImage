/**
 * test_lpx.cpp
 * 
 * Test program for log-polar image conversion and rendering
 */

#include "../include/lpx_image.h"
#include "../include/lpx_renderer.h"
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <chrono>

int main(int argc, char** argv) {
    // Check command line arguments
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <scan_tables_file> <input_image>" << std::endl;
        return 1;
    }

    std::string scanTablesFile = argv[1];
    std::string inputImageFile = argv[2];

    // Load scan tables
    std::cout << "Loading scan tables from: " << scanTablesFile << std::endl;
    
    // Check file size
    FILE* fp = fopen(scanTablesFile.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open scan tables file: " << scanTablesFile << std::endl;
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::cout << "Scan tables file size: " << fileSize << " bytes" << std::endl;
    
    // Read just enough to get the header information
    int header[7];
    size_t read = fread(header, sizeof(int), 7, fp);
    if (read != 7) {
        std::cerr << "Failed to read scan tables header" << std::endl;
        fclose(fp);
        return 1;
    }
    
    std::cout << "Reported total length: " << header[0] << " (32-bit integers)" << std::endl;
    std::cout << "Header values:" << std::endl;
    std::cout << "  mapWidth: " << header[1] << std::endl;
    std::cout << "  spiralPer: " << (header[2] + 0.5f) << " (from int: " << header[2] << ")" << std::endl;
    std::cout << "  length: " << header[3] << std::endl;
    std::cout << "  innerLength: " << header[4] << std::endl;
    std::cout << "  lastFoveaIndex: " << header[5] << std::endl;
    std::cout << "  lastCellIndex: " << header[6] << std::endl;
    
    fclose(fp);
    
    auto scanTables = std::make_shared<lpx::LPXTables>(scanTablesFile);
    if (!scanTables->isInitialized()) {
        std::cerr << "Failed to load scan tables: " << scanTablesFile << std::endl;
        return 1;
    }
    std::cout << "Successfully loaded scan tables in binary format" << std::endl;
      
    // Print first few values from the arrays
    if (scanTables->outerPixelIndex.size() > 0) {
        std::cout << "  First 5 outerPixelIndex values:" << std::endl;
        for (int i = 0; i < std::min(5, (int)scanTables->outerPixelIndex.size()); i++) {
            std::cout << "    " << i << ": " << scanTables->outerPixelIndex[i] << std::endl;
        }
    }
    
    if (scanTables->outerPixelCellIdx.size() > 0) {
        std::cout << "  First 5 outerPixelCellIdx values:" << std::endl;
        for (int i = 0; i < std::min(5, (int)scanTables->outerPixelCellIdx.size()); i++) {
            std::cout << "    " << i << ": " << scanTables->outerPixelCellIdx[i] << std::endl;
        }
    }

    // Initialize the renderer with scan tables
    lpx::LPXRenderer renderer;
    renderer.setScanTables(scanTables);

    // Load input image
    cv::Mat inputImage = cv::imread(inputImageFile);
    if (inputImage.empty()) {
        std::cerr << "Failed to load input image: " << inputImageFile << std::endl;
        return 1;
    }

    std::cout << "Loaded image: " << inputImageFile 
              << " (" << inputImage.cols << "x" << inputImage.rows << ")" << std::endl;

    // Create an LPXImage with scan tables
    std::cout << "Creating LPXImage with spiral period: " << scanTables->spiralPer
              << ", nMaxCells: " << (scanTables->lastCellIndex + 1) << std::endl;
    
    // Make sure the spiral period is valid
    if (scanTables->spiralPer < 0.1f || scanTables->spiralPer > 1000.0f) {
        std::cout << "WARNING: Invalid spiral period detected, fixing to 63.5" << std::endl;
        scanTables->spiralPer = 63.5f;
    }
    
    auto lpxImage = std::make_shared<lpx::LPXImage>(scanTables, inputImage.cols, inputImage.rows);
    
    // Verify that the spiral period was set correctly
    if (lpxImage->getSpiralPeriod() != scanTables->spiralPer) {
        std::cerr << "WARNING: Spiral period mismatch! LPXImage has " << lpxImage->getSpiralPeriod() 
                  << " but scan tables has " << scanTables->spiralPer << std::endl;
    }
    
    // Convert to log-polar format
    float centerX = inputImage.cols / 2.0f;
    float centerY = inputImage.rows / 2.0f;
    
    std::cout << "Scanning image with dimensions: " << inputImage.cols << "x" << inputImage.rows
              << ", center at: (" << centerX << ", " << centerY << ")" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (!lpxImage->scanFromImage(inputImage, centerX, centerY)) {
        std::cerr << "Failed to convert image to log-polar format" << std::endl;
        return 1;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    
    std::cout << "Scan complete, processed " << lpxImage->getLength() << " cells in " 
              << duration.count() << " ms" << std::endl;

    std::cout << "Converted to log-polar format: " 
              << lpxImage->getLength() << " cells, spiral period: " 
              << lpxImage->getSpiralPeriod() << std::endl;

    // Save the log-polar image
    std::string lpxOutputFile = "output.lpx";
    if (lpxImage->saveToFile(lpxOutputFile)) {
        std::cout << "Saved log-polar image to: " << lpxOutputFile << std::endl;
        
        // Debug: Try to load back the image to verify it's correctly saved
        auto loadedImage = std::make_shared<lpx::LPXImage>(nullptr, 0, 0);
        if (loadedImage->loadFromFile(lpxOutputFile)) {
            int nonZeroCells = 0;
            for (int i = 0; i < loadedImage->getLength(); i++) {
                if (loadedImage->getCellValue(i) != 0) {
                    nonZeroCells++;
                    
                    // Display info about some cells
                    if (nonZeroCells <= 5) {
                        uint32_t value = loadedImage->getCellValue(i);
                        int r = (value >> 16) & 0xFF;
                        int g = (value >> 8) & 0xFF;
                        int b = value & 0xFF;
                        
                    }
                }
            }
            std::cout << "DEBUG: Loaded image has " << nonZeroCells 
                      << " non-zero cells out of " << loadedImage->getLength() << std::endl;
        } else {
            std::cerr << "Failed to load back the saved log-polar image" << std::endl;
        }
    } else {
        std::cerr << "Failed to save log-polar image" << std::endl;
    }

    // Render the log-polar image back to standard format using our renderer
    std::cout << "Rendering from LPXImage with " << lpxImage->getLength() << " cells" << std::endl;
    
    // Get bounding box for rendering
    lpx::Rect box = renderer.getScanBoundingBox(lpxImage, inputImage.cols, inputImage.rows, 1.0f);
    std::cout << "Rendering bounding box: (" << box.xMin << "," << box.yMin 
              << ") to (" << box.xMax << "," << box.yMax << ")" << std::endl;
    
    startTime = std::chrono::high_resolution_clock::now();
    cv::Mat renderedImage = renderer.renderToImage(lpxImage, inputImage.cols, inputImage.rows);
    endTime = std::chrono::high_resolution_clock::now();
    duration = endTime - startTime;
    
    if (!renderedImage.empty()) {
        std::cout << "Rendering complete in " << duration.count() << " ms" << std::endl;
        
        std::string outputImageFile = "output_rendered.jpg";
        cv::imwrite(outputImageFile, renderedImage);
        std::cout << "Saved rendered log-polar image to: " << outputImageFile << std::endl;

        // Display the images
        cv::namedWindow("Original Image", cv::WINDOW_NORMAL);
        cv::namedWindow("Log-Polar Rendered Image", cv::WINDOW_NORMAL);
        
        cv::imshow("Original Image", inputImage);
        cv::imshow("Log-Polar Rendered Image", renderedImage);
        
        std::cout << "Displaying images. Press any key to exit." << std::endl;
        cv::waitKey(0);
        cv::destroyAllWindows();
    } else {
        std::cerr << "Failed to render log-polar image" << std::endl;
    }

    return 0;
}
