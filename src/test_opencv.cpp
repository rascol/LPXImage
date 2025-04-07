#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Create a simple colored image
    cv::Mat image(480, 640, CV_8UC3, cv::Scalar(0, 0, 255));  // Red background
    
    // Draw some shapes to make it distinctive
    cv::circle(image, cv::Point(320, 240), 100, cv::Scalar(0, 255, 0), -1);
    cv::rectangle(image, cv::Point(100, 100), cv::Point(200, 200), cv::Scalar(255, 0, 0), -1);
    
    // Add text
    cv::putText(image, "OpenCV Test", cv::Point(50, 50), 
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    
    // Create window and show image
    std::string windowName = "OpenCV Test Window";
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::imshow(windowName, image);
    
    std::cout << "Window created with test image. Press ESC to exit." << std::endl;
    
    // Main loop to process events
    while (true) {
        int key = cv::waitKey(10);  // Process events every 10ms
        if (key == 27) break;  // ESC key
        
        // Just to show the program is running
        static int counter = 0;
        if (++counter % 100 == 0) {
            std::cout << "Still running..." << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    cv::destroyAllWindows();
    return 0;
}