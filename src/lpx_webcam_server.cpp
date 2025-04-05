// lpx_webcam_server.cpp
#include "lpx_webcam_server.h"
#include <iostream>
#include <chrono>
#include <arpa/inet.h>
#include <fcntl.h>

namespace lpx {

// LPXStreamProtocol implementation
bool LPXStreamProtocol::sendLPXImage(int socket, const std::shared_ptr<LPXImage>& image) {
    if (!image) return false;
    
    // Extract image data
    int length = image->getLength();
    int nMaxCells = image->getMaxCells();
    float spiralPer = image->getSpiralPeriod();
    int width = image->getWidth();
    int height = image->getHeight();
    float x_ofs = image->getXOffset();
    float y_ofs = image->getYOffset();
    
    // Scale offsets for transmission (matching file format)
    int x_ofs_scaled = static_cast<int>(x_ofs * 100000);
    int y_ofs_scaled = static_cast<int>(y_ofs * 100000);
    
    // Prepare header (8 ints)
    int header[8];
    header[0] = length;
    header[1] = nMaxCells;
    header[2] = static_cast<int>(spiralPer); // Convert to int as in your saveToFile method
    header[3] = width;
    header[4] = height;
    header[5] = x_ofs_scaled;
    header[6] = y_ofs_scaled; 
    header[7] = 0; // Reserved
    
    // Calculate total size
    int headerSize = sizeof(header);
    int dataSize = length * sizeof(uint32_t);
    int totalSize = headerSize + dataSize;
    
    // Send total size first
    if (send(socket, &totalSize, sizeof(int), 0) != sizeof(int)) {
        return false;
    }
    
    // Send header
    if (send(socket, header, headerSize, 0) != headerSize) {
        return false;
    }
    
    // Send cell data
    const uint8_t* rawData = image->getRawData();
    int bytesSent = 0;
    while (bytesSent < dataSize) {
        int result = send(socket, rawData + bytesSent, dataSize - bytesSent, 0);
        if (result <= 0) {
            return false;
        }
        bytesSent += result;
    }
    
    return true;
}

std::shared_ptr<LPXImage> LPXStreamProtocol::receiveLPXImage(int socket, std::shared_ptr<LPXTables> scanTables) {
    // Read total size
    int totalSize;
    if (recv(socket, &totalSize, sizeof(int), 0) != sizeof(int)) {
        return nullptr;
    }
    
    // Sanity check
    if (totalSize <= 0 || totalSize > 10 * 1024 * 1024) { // Max 10MB
        std::cerr << "Invalid LPXImage size: " << totalSize << std::endl;
        return nullptr;
    }
    
    // Read header
    int header[8];
    if (recv(socket, header, sizeof(header), 0) != sizeof(header)) {
        return nullptr;
    }
    
    // Extract header fields
    int length = header[0];
    int nMaxCells = header[1];
    float spiralPer = static_cast<float>(header[2]) + 0.5f; // Add 0.5 as in loadFromFile
    int width = header[3];
    int height = header[4];
    float x_ofs = header[5] * 1e-5f; // Scale back from int
    float y_ofs = header[6] * 1e-5f; // Scale back from int
    
    // Create the LPXImage
    auto image = std::make_shared<LPXImage>(scanTables, width, height);
    image->setLength(length);
    image->setPosition(x_ofs, y_ofs);
    
    // Calculate data size
    int headerSize = sizeof(header);
    int dataSize = length * sizeof(uint32_t);
    
    // Get access to internal cell array
    std::vector<uint32_t>& cellArray = image->accessCellArray();
    
    // Read data in chunks
    int bytesRead = 0;
    while (bytesRead < dataSize) {
        int result = recv(socket, reinterpret_cast<uint8_t*>(cellArray.data()) + bytesRead, 
                         dataSize - bytesRead, 0);
        if (result <= 0) {
            return nullptr;
        }
        bytesRead += result;
    }
    
    return image;
}

// WebcamLPXServer implementation
WebcamLPXServer::WebcamLPXServer(const std::string& scanTableFile, int port) 
    : port(port), serverSocket(-1), running(false), currentSkipRate(3) {
    
    // Initialize scan tables
    scanTables = std::make_shared<LPXTables>(scanTableFile);
    if (!scanTables->isInitialized()) {
        throw std::runtime_error("Failed to initialize scan tables from: " + scanTableFile);
    }

    g_scanTables = scanTables;
}

WebcamLPXServer::~WebcamLPXServer() {
    stop();
}

bool WebcamLPXServer::start(int cameraId, int width, int height) {
    if (running) return false;
    
    captureWidth = width;
    captureHeight = height;
    
    // Initialize server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating server socket" << std::endl;
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(serverSocket);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket to port " << port << std::endl;
        close(serverSocket);
        return false;
    }
    
    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        return false;
    }
    
    running = true;
    
    // Start threads
    captureThreadHandle = std::thread(&WebcamLPXServer::captureThread, this, cameraId);
    processingThreadHandle = std::thread(&WebcamLPXServer::processingThread, this);
    networkThreadHandle = std::thread(&WebcamLPXServer::networkThread, this);
    acceptThreadHandle = std::thread(&WebcamLPXServer::acceptClients, this);
    
    std::cout << "WebcamLPXServer started on port " << port << std::endl;
    return true;
}

void WebcamLPXServer::stop() {
    if (!running) return;
    
    running = false;
    
    // Wake up waiting threads
    frameCondition.notify_all();
    lpxImageCondition.notify_all();
    
    // Close server socket
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
    
    // Close client sockets
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (int clientSocket : clientSockets) {
            close(clientSocket);
        }
        clientSockets.clear();
    }
    
    // Join threads
    if (captureThreadHandle.joinable()) captureThreadHandle.join();
    if (processingThreadHandle.joinable()) processingThreadHandle.join();
    if (networkThreadHandle.joinable()) networkThreadHandle.join();
    if (acceptThreadHandle.joinable()) acceptThreadHandle.join();
    
    std::cout << "WebcamLPXServer stopped" << std::endl;
}

void WebcamLPXServer::setSkipRate(int min, int max, float motionThreshold) {
    minSkipRate = min;
    maxSkipRate = max;
    this->motionThreshold = motionThreshold;
    
    // Ensure current skip rate is within bounds
    int current = currentSkipRate.load();
    if (current < min) currentSkipRate.store(min);
    if (current > max) currentSkipRate.store(max);
}

int WebcamLPXServer::getClientCount() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clientSockets.size();
}

void WebcamLPXServer::captureThread(int cameraId) {
    cv::VideoCapture cap(cameraId);
    
    if (!cap.isOpened()) {
        std::cerr << "Failed to open webcam" << std::endl;
        running = false;
        return;
    }
    
    cap.set(cv::CAP_PROP_FRAME_WIDTH, captureWidth);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, captureHeight);
    
    // Get actual width and height
    captureWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    captureHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    
    std::cout << "Webcam initialized at " << captureWidth << "x" << captureHeight << std::endl;
    
    while (running) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            std::cerr << "Failed to read frame from webcam" << std::endl;
            break;
        }
        
        // Adaptive frame skipping
        frameCount++;
        if (frameCount % currentSkipRate.load() == 0) {
            // Motion detection
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            
            bool hasMotion = false;
            if (!previousGrayFrame.empty()) {
                float motionScore = detectMotion(gray, previousGrayFrame);
                hasMotion = motionScore > motionThreshold;
            }
            gray.copyTo(previousGrayFrame);
            
            // Add to processing queue if needed
            if (hasMotion || frameQueue.empty()) {
                std::unique_lock<std::mutex> lock(frameMutex);
                
                // Prevent queue from growing too large
                while (frameQueue.size() >= 3) {
                    frameQueue.pop();
                }
                
                frameQueue.push(frame.clone());
                lock.unlock();
                
                frameCondition.notify_one();
            }
        }
        
        // Brief sleep to avoid CPU burnout
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    cap.release();
    std::cout << "Capture thread stopped" << std::endl;
}

void WebcamLPXServer::processingThread() {
    while (running) {
        cv::Mat frameToProcess;
        
        // Wait for a frame to process
        {
            std::unique_lock<std::mutex> lock(frameMutex);
            while (frameQueue.empty() && running) {
                frameCondition.wait(lock);
            }
            
            if (!running) break;
            
            frameToProcess = frameQueue.front();
            frameQueue.pop();
        }
        
        // Process the frame
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Center the LPX scan at the center of the image
        float centerX = frameToProcess.cols / 2.0f;
        float centerY = frameToProcess.rows / 2.0f;
        
        // Use the existing multithreaded scanning function
        auto lpxImage = multithreadedScanImage(frameToProcess, centerX, centerY);
        
        if (lpxImage) {
            // Add to broadcast queue
            std::unique_lock<std::mutex> lock(lpxImageMutex);
            
            // Keep queue manageable
            while (lpxImageQueue.size() >= 3) {
                lpxImageQueue.pop();
            }
            
            lpxImageQueue.push(lpxImage);
            lock.unlock();
            
            lpxImageCondition.notify_one();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        float processingTime = std::chrono::duration_cast<std::chrono::duration<float>>(
            endTime - startTime).count();
        
        // Adjust skip rate based on processing time
        adjustSkipRate(processingTime, false);
    }
    
    std::cout << "Processing thread stopped" << std::endl;
}

void WebcamLPXServer::networkThread() {
    while (running) {
        std::shared_ptr<LPXImage> imageToSend;
        
        // Wait for an image to send
        {
            std::unique_lock<std::mutex> lock(lpxImageMutex);
            while (lpxImageQueue.empty() && running) {
                lpxImageCondition.wait(lock);
            }
            
            if (!running) break;
            
            imageToSend = lpxImageQueue.front();
            lpxImageQueue.pop();
        }
        
        // Send to all clients
        if (imageToSend) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::vector<int> disconnectedClients;
            
            for (int clientSocket : clientSockets) {
                if (!LPXStreamProtocol::sendLPXImage(clientSocket, imageToSend)) {
                    disconnectedClients.push_back(clientSocket);
                }
            }
            
            // Remove disconnected clients
            for (int socket : disconnectedClients) {
                close(socket);
                clientSockets.erase(socket);
                std::cout << "Client disconnected" << std::endl;
            }
        }
    }
    
    std::cout << "Network thread stopped" << std::endl;
}

void WebcamLPXServer::acceptClients() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    // Set socket to non-blocking mode
    fcntl(serverSocket, F_SETFL, O_NONBLOCK);
    
    while (running) {
        // Accept new connection
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket >= 0) {
            std::cout << "New client connected from " 
                     << inet_ntoa(clientAddr.sin_addr) << ":" 
                     << ntohs(clientAddr.sin_port) << std::endl;
            
            // Add to clients set
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientSockets.insert(clientSocket);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // An actual error occurred
            if (running) {
                std::cerr << "Error accepting client connection: " << strerror(errno) << std::endl;
            }
        }
        
        // Sleep briefly to avoid CPU burnout
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Accept thread stopped" << std::endl;
}

float WebcamLPXServer::detectMotion(const cv::Mat& current, const cv::Mat& previous) {
    cv::Mat diff;
    cv::absdiff(current, previous, diff);
    cv::Scalar meanDiff = cv::mean(diff);
    return static_cast<float>(meanDiff[0]);
}

void WebcamLPXServer::adjustSkipRate(float processingTime, bool hasMotion) {
    std::lock_guard<std::mutex> lock(timingMutex);
    
    // Keep track of processing times (last 10)
    processingTimes.push_back(processingTime);
    if (processingTimes.size() > 10) {
        processingTimes.erase(processingTimes.begin());
    }
    
    // Calculate average processing time
    float avgTime = 0.0f;
    for (float time : processingTimes) {
        avgTime += time;
    }
    avgTime /= processingTimes.size();
    
    // Adjust skip rate based on processing time and motion
    int newSkipRate = currentSkipRate.load();
    
    if (hasMotion && avgTime < 0.09f) {
        // We have headroom and need detail
        newSkipRate = std::max(minSkipRate, newSkipRate - 1);
    } 
    else if (avgTime > 0.11f || (!hasMotion && newSkipRate < 4)) {
        // Either we're falling behind or no motion detected
        newSkipRate = std::min(maxSkipRate, newSkipRate + 1);
    }
    
    if (newSkipRate != currentSkipRate.load()) {
        std::cout << "Adjusting skip rate to " << newSkipRate 
                 << ", Avg time: " << avgTime << "s" << std::endl;
        currentSkipRate.store(newSkipRate);
    }
}

// LPXDebugClient implementation
LPXDebugClient::LPXDebugClient(const std::string& scanTableFile)
    : clientSocket(-1), running(false) {
    
    // Initialize scan tables
    scanTables = std::make_shared<LPXTables>(scanTableFile);
    if (!scanTables->isInitialized()) {
        throw std::runtime_error("Failed to initialize scan tables from: " + scanTableFile);
    }
    
    // Initialize renderer
    renderer = std::make_shared<LPXRenderer>();
    renderer->setScanTables(scanTables);
}

LPXDebugClient::~LPXDebugClient() {
    disconnect();
}

bool LPXDebugClient::connect(const std::string& serverAddress, int port) {
    if (running) disconnect();
    
    this->serverAddress = serverAddress;
    this->port = port;
    
    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating client socket" << std::endl;
        return false;
    }
    
    // Connect to server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        close(clientSocket);
        return false;
    }
    
    if (::connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(clientSocket);
        return false;
    }
    
    running = true;
    
    // Start receiver thread
    receiverThreadHandle = std::thread(&LPXDebugClient::receiverThread, this);
    
    std::cout << "Connected to LPX server at " << serverAddress << ":" << port << std::endl;
    return true;
}

void LPXDebugClient::disconnect() {
    if (!running) return;
    
    running = false;
    
    if (clientSocket >= 0) {
        close(clientSocket);
        clientSocket = -1;
    }
    
    if (receiverThreadHandle.joinable()) {
        receiverThreadHandle.join();
    }
    
    std::cout << "Disconnected from LPX server" << std::endl;
}

void LPXDebugClient::setWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void LPXDebugClient::receiverThread() {
    cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowTitle, windowWidth, windowHeight);
    
    while (running) {
        // Receive an LPXImage
        auto image = LPXStreamProtocol::receiveLPXImage(clientSocket, scanTables);
        
        if (!image) {
            std::cout << "Connection lost" << std::endl;
            running = false;
            break;
        }
        
        // Render the image using the existing multithreaded renderer
        auto startTime = std::chrono::high_resolution_clock::now();
        
        cv::Mat rendered = renderer->renderToImage(image, windowWidth, windowHeight, scale);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        
        if (!rendered.empty()) {
            // Display stats on the image
            std::string stats = "Render: " + std::to_string(duration) + "ms, Cells: " 
                               + std::to_string(image->getLength());
            cv::putText(rendered, stats, cv::Point(10, 20), 
                      cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
            
            cv::imshow(windowTitle, rendered);
            
            // Check for key press
            int key = cv::waitKey(1);
            if (key == 27) { // ESC key
                running = false;
                break;
            }
        }
    }
    
    cv::destroyAllWindows();
    std::cout << "Receiver thread stopped" << std::endl;
}

} // namespace lpx