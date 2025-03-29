#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include "../include/lpx_common.h"
#include "../include/lpx_image.h"
#include "../include/lpx_mt.h"  // Include the multithreaded header
#include "../include/lpx_renderer.h"  // Include the renderer header

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <scan_tables_file> <image_file>" << std::endl;
        return -1;
    }

    // Load scan tables
    std::cout << "Loading scan tables from: " << argv[1] << std::endl;
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
    std::cout << "Loaded image: " << argv[2] << " (" << image.cols << "x" << image.rows << ")" << std::endl;

    // Test with original serial implementation
    std::shared_ptr<lpx::LPXImage> serialLpxImage;
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        serialLpxImage = lpx::scanImage(image, image.cols / 2.0f, image.rows / 2.0f);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        std::cout << "Serial scan complete, processed " << serialLpxImage->getLength() << " cells in " 
                  << duration << " ms" << std::endl;
        
        // Save the LPX image
        if (serialLpxImage->saveToFile("output_serial.lpx")) {
            std::cout << "Saved log-polar image to: output_serial.lpx" << std::endl;
        }
    }

    // Test with multithreaded implementation
    std::shared_ptr<lpx::LPXImage> mtLpxImage;
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        mtLpxImage = lpx::multithreadedScanImage(image, image.cols / 2.0f, image.rows / 2.0f);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        if (mtLpxImage) {
            std::cout << "Multithreaded scan complete, processed " << mtLpxImage->getLength() << " cells in " 
                      << duration << " ms" << std::endl;
            
            // Save the LPX image
            if (mtLpxImage->saveToFile("output_mt.lpx")) {
                std::cout << "Saved log-polar image to: output_mt.lpx" << std::endl;
            }
        } else {
            std::cerr << "Multithreaded scan failed!" << std::endl;
            return -1;
        }
    }

    // Now render the LPXImages back to standard images for verification
    std::cout << "Rendering images back to standard format for verification..." << std::endl;
    
    // Initialize renderer and load scan tables
    lpx::LPXRenderer renderer;
    renderer.setScanTables(serialLpxImage->getScanTables());
    
    // Render the images
    cv::Mat renderedSerialImage = renderer.renderToImage(serialLpxImage, image.cols, image.rows);
    cv::Mat renderedMtImage = renderer.renderToImage(mtLpxImage, image.cols, image.rows);
    
    if (renderedSerialImage.empty() || renderedMtImage.empty()) {
        std::cerr << "Failed to render one or both LPX images" << std::endl;
        return -1;
    }
    
    // Save rendered images
    cv::imwrite("output_serial_rendered.jpg", renderedSerialImage);
    cv::imwrite("output_mt_rendered.jpg", renderedMtImage);
    
    std::cout << "Saved rendered images to output_serial_rendered.jpg and output_mt_rendered.jpg" << std::endl;
    
    // Display the images
    cv::namedWindow("Original Image", cv::WINDOW_NORMAL);
    cv::namedWindow("Serial Rendered Image", cv::WINDOW_NORMAL);
    cv::namedWindow("Multithreaded Rendered Image", cv::WINDOW_NORMAL);
    
    cv::imshow("Original Image", image);
    cv::imshow("Serial Rendered Image", renderedSerialImage);
    cv::imshow("Multithreaded Rendered Image", renderedMtImage);
    
    std::cout << "Displaying images. Press any key to exit." << std::endl;
    cv::waitKey(0);
    cv::destroyAllWindows();
    
    // Compare the serial and multi-threaded images
    double similarity = 0.0;
    int diffPixelCount = 0;
    int totalPixels = renderedSerialImage.rows * renderedSerialImage.cols;
    
    // Simple pixel-by-pixel comparison
    for (int y = 0; y < renderedSerialImage.rows; y++) {
        for (int x = 0; x < renderedSerialImage.cols; x++) {
            cv::Vec3b serialPixel = renderedSerialImage.at<cv::Vec3b>(y, x);
            cv::Vec3b mtPixel = renderedMtImage.at<cv::Vec3b>(y, x);
            
            // Count exact matches
            if (serialPixel == mtPixel) {
                similarity += 1.0;
            } else {
                diffPixelCount++;
            }
        }
    }
    
    // Calculate percentage of matching pixels
    similarity = (similarity / totalPixels) * 100.0;
    
    std::cout << "Image comparison results:" << std::endl;
    std::cout << "  - Total pixels: " << totalPixels << std::endl;
    std::cout << "  - Different pixels: " << diffPixelCount << std::endl;
    std::cout << "  - Similarity: " << similarity << "%" << std::endl;
    
    // Create a difference image for visualization
    cv::Mat diffImage(renderedSerialImage.size(), CV_8UC3, cv::Scalar(0, 0, 0));
    for (int y = 0; y < renderedSerialImage.rows; y++) {
        for (int x = 0; x < renderedSerialImage.cols; x++) {
            cv::Vec3b serialPixel = renderedSerialImage.at<cv::Vec3b>(y, x);
            cv::Vec3b mtPixel = renderedMtImage.at<cv::Vec3b>(y, x);
            
            if (serialPixel != mtPixel) {
                // Highlight differences in red
                diffImage.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
            } else if (serialPixel[0] > 0 || serialPixel[1] > 0 || serialPixel[2] > 0) {
                // Show matching non-black pixels in green
                diffImage.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 255, 0);
            }
        }
    }
    
    // Save and display the difference image
    cv::imwrite("output_difference.jpg", diffImage);
    cv::namedWindow("Difference Image", cv::WINDOW_NORMAL);
    cv::imshow("Difference Image", diffImage);
    cv::waitKey(0);
    cv::destroyAllWindows();
    
    std::cout << "Testing completed. Rendered images saved for verification." << std::endl;

    return 0;
}
