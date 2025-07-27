// lpx_webcam_server.cpp
#include "lpx_webcam_server.h"
#include <iostream>
#include <chrono>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>  // For TCP_NODELAY

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
    : clientSocket(-1), running(false), lastKeyTime(std::chrono::steady_clock::now() - std::chrono::milliseconds(KEY_THROTTLE_MS + 1)) {
    
    // Log version information
    std::cout << "[VERSION] LPXDebugClient v" << getVersionString() << std::endl;
    std::cout << "[VERSION] Built: " << getBuildTimestamp() << std::endl;
    std::cout << "[VERSION] Key throttle: " << getKeyThrottleMs() << "ms" << std::endl;
    
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
    // Stop threads but don't call disconnect() from destructor
    // as it may involve OpenCV window operations that need main thread
    try {
        if (running) {
            running = false;
            
            if (clientSocket >= 0) {
                close(clientSocket);
                clientSocket = -1;
            }
            
            if (receiverThreadHandle.joinable()) {
                receiverThreadHandle.join();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in LPXDebugClient destructor: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in LPXDebugClient destructor" << std::endl;
    }
}

bool LPXDebugClient::connect(const std::string& serverAddress, int port) {
    if (running) disconnect();
    
    // Parse server address - handle "host:port" format
    std::string host = serverAddress;
    int actualPort = port;
    
    size_t colonPos = serverAddress.find(':');
    if (colonPos != std::string::npos) {
        host = serverAddress.substr(0, colonPos);
        actualPort = std::stoi(serverAddress.substr(colonPos + 1));
        std::cout << "[DEBUG] Parsed address: host=" << host << ", port=" << actualPort << std::endl;
    }
    
    this->serverAddress = host;
    this->port = actualPort;
    
    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating client socket" << std::endl;
        return false;
    }
    
    // Connect to server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(actualPort);
    
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported: " << host << std::endl;
        close(clientSocket);
        return false;
    }
    
    if (::connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "LPXDebugClient: Connection failed to " << host << ":" << actualPort 
                  << " - " << strerror(errno) << std::endl;
        close(clientSocket);
        return false;
    }
    
    // Configure socket for low-latency command transmission
    int flag = 1;
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
        std::cerr << "Warning: Failed to set TCP_NODELAY on client socket" << std::endl;
    }
    
    // Set receive buffer size to handle bursts of data
    int recvBuf = 256 * 1024; // 256KB receive buffer
    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF, &recvBuf, sizeof(recvBuf)) < 0) {
        std::cerr << "Warning: Failed to set receive buffer size" << std::endl;
    }
    
    running = true;
    
    // Start receiver thread
    receiverThreadHandle = std::thread(&LPXDebugClient::receiverThread, this);
    
    std::cout << "Connected to LPX server at " << host << ":" << actualPort << std::endl;
    return true;
}

void LPXDebugClient::disconnect() {
    if (!running) return;
    
    running = false;
    
    try {
        if (clientSocket >= 0) {
            close(clientSocket);
            clientSocket = -1;
        }
        
        if (receiverThreadHandle.joinable()) {
            receiverThreadHandle.join();
        }
        
        // Cleanup OpenCV windows from main thread
        cv::destroyWindow(windowTitle);
        
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error during disconnect: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during disconnect: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception during disconnect" << std::endl;
    }
    
    std::cout << "Disconnected from LPX server" << std::endl;
}

void LPXDebugClient::setWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void LPXDebugClient::initializeWindow() {
    // Create window (must be called from main thread on macOS)
    cv::namedWindow(windowTitle, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::resizeWindow(windowTitle, windowWidth, windowHeight);
    
    // Create a black image to show initially
    cv::Mat initialImage(windowHeight, windowWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::putText(initialImage, "Waiting for LPX data...", cv::Point(windowWidth/4, windowHeight/2), 
               cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    cv::imshow(windowTitle, initialImage);
    cv::waitKey(1); // Force window update
}

bool LPXDebugClient::processEvents() {
    // Process UI events (must be called from main thread on macOS)
    // Process multiple keys in a single frame to drain keyboard buffer
    
    // Display new image if available (from main thread only)
    {
        std::lock_guard<std::mutex> lock(displayMutex);
        if (newImageAvailable && !latestImage.empty()) {
            cv::imshow(windowTitle, latestImage);
            newImageAvailable = false;
            // Don't call waitKey here - it will be called below
        }
    }
    
    // Process only ONE key per frame to prevent blocking
    static int totalKeysProcessed = 0;
    static int totalCommandsSent = 0;
    static int throttledCommands = 0;
    
    // Call waitKey only once per frame
    int key = cv::waitKey(1);  // This handles both display refresh and keyboard input
        
        // Handle WASD keyboard input for movement
        if (key != -1 && key != 255) {  // A key was pressed
            totalKeysProcessed++;
            float deltaX = 0, deltaY = 0;
            float stepSize = 10.0f;
            bool shouldSendCommand = false;
            std::string keyName = "";
            
            switch (key) {
                case 'w':
                case 'W':
                    deltaY = -1;
                    shouldSendCommand = true;
                    keyName = "W";
                    break;
                case 's':
                case 'S':
                    deltaY = 1;
                    shouldSendCommand = true;
                    keyName = "S";
                    break;
                case 'a':
                case 'A':
                    deltaX = -1;
                    shouldSendCommand = true;
                    keyName = "A";
                    break;
                case 'd':
                case 'D':
                    deltaX = 1;
                    shouldSendCommand = true;
                    keyName = "D";
                    break;
                case 27: // ESC key
                case 'q':
                case 'Q':
                    running = false;
                    return false;
            }
            
            std::cout << "[DEBUG CLIENT] Key pressed: '" << keyName << "' (code=" << key 
                      << "), Total keys: " << totalKeysProcessed << std::endl;
            
            // Queue movement command if any movement was detected and throttling allows
            if (shouldSendCommand && (deltaX != 0 || deltaY != 0)) {
                auto now = std::chrono::steady_clock::now();
                auto timeSinceLastKey = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastKeyTime).count();
                
                if (timeSinceLastKey >= KEY_THROTTLE_MS) {
                    // Store the command as pending instead of sending immediately
                    {
                        std::lock_guard<std::mutex> lock(pendingCommandMutex);
                        hasPendingCommand = true;
                        pendingDeltaX = deltaX;
                        pendingDeltaY = deltaY;
                        pendingStepSize = stepSize;
                        std::cout << "[DEBUG CLIENT] Queued movement command: (" 
                                  << deltaX << ", " << deltaY << ")" << std::endl;
                    }
                    
                    // Try to send if we can (i.e., after receiving a frame)
                    if (canSendCommand.load()) {
                        std::lock_guard<std::mutex> lock(pendingCommandMutex);
                        if (hasPendingCommand) {
                            std::cout << "[DEBUG CLIENT] Sending queued command immediately" << std::endl;
                            bool sent = sendMovementCommand(pendingDeltaX, pendingDeltaY, pendingStepSize);
                            if (sent) {
                                totalCommandsSent++;
                                hasPendingCommand = false;
                                canSendCommand = false;  // Wait for next frame
                                std::cout << "[DEBUG CLIENT] Command sent successfully. Total sent: " 
                                          << totalCommandsSent << std::endl;
                            } else {
                                std::cout << "[DEBUG CLIENT] Failed to send command!" << std::endl;
                            }
                        }
                    } else {
                        std::cout << "[DEBUG CLIENT] Command queued, waiting for frame sync" << std::endl;
                    }
                    
                    lastKeyTime = now;
                } else {
                    throttledCommands++;
                    std::cout << "[DEBUG CLIENT] Command throttled (time since last: " 
                              << timeSinceLastKey << "ms < " << KEY_THROTTLE_MS 
                              << "ms). Total throttled: " << throttledCommands << std::endl;
                }
            }
        }
    
    // Log periodically that we're processing events
    static int counter = 0;
    if (++counter % 100 == 0) {
        std::cout << "[DEBUG CLIENT] Main thread alive - Events processed: " << counter 
                  << ", Keys: " << totalKeysProcessed 
                  << ", Commands sent: " << totalCommandsSent 
                  << ", Throttled: " << throttledCommands << std::endl;
    }
    
    return true;
}

bool LPXDebugClient::sendMovementCommand(float deltaX, float deltaY, float stepSize) {
    auto sendStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "[TIMER CLIENT] sendMovementCommand called at " 
              << std::chrono::duration_cast<std::chrono::microseconds>(sendStartTime.time_since_epoch()).count() 
              << "μs" << std::endl;
    std::cout << "[DEBUG CLIENT] Sending movement command (" << deltaX << ", " << deltaY 
              << ") step=" << stepSize << std::endl;
    
    if (clientSocket < 0 || !running) {
        std::cerr << "[ERROR CLIENT] Not connected to server" << std::endl;
        return false;
    }
    
    // Frame synchronization check - only send if we've received a frame
    if (!canSendCommand.load()) {
        std::cout << "[DEBUG CLIENT] Frame sync: Command blocked - waiting for frame" << std::endl;
        
        // Store as pending command
        {
            std::lock_guard<std::mutex> lock(pendingCommandMutex);
            hasPendingCommand = true;
            pendingDeltaX = deltaX;
            pendingDeltaY = deltaY;
            pendingStepSize = stepSize;
        }
        
        return false;  // Command queued but not sent
    }
    
    // Check socket is still valid before sending
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0) {
        std::cerr << "[ERROR CLIENT] Socket error detected: " << strerror(error) << std::endl;
        running = false;
        return false;
    }
    
    // Send command type with error checking
    uint32_t cmdType = 0x02; // CMD_MOVEMENT
    std::cout << "[DEBUG CLIENT] About to send command type 0x" << std::hex << cmdType << std::dec 
              << " (" << sizeof(cmdType) << " bytes) on socket " << clientSocket << std::endl;
    
    // Check send buffer space before sending
    int sendSpace = 0;
    socklen_t optlen = sizeof(sendSpace);
    if (getsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, &sendSpace, &optlen) == 0) {
        std::cout << "[DEBUG CLIENT] Socket send buffer size: " << sendSpace << " bytes" << std::endl;
    }
    
    ssize_t sent = send(clientSocket, &cmdType, sizeof(cmdType), MSG_NOSIGNAL);
    if (sent != sizeof(cmdType)) {
        std::cerr << "[ERROR CLIENT] Failed to send movement command type: sent " << sent 
                  << " bytes, expected " << sizeof(cmdType) << ", errno: " << errno 
                  << " (" << strerror(errno) << ")" << std::endl;
        if (errno == EPIPE || errno == ECONNRESET) {
            running = false;
        }
        return false;
    }
    std::cout << "[DEBUG CLIENT] Command type sent successfully" << std::endl;
    
    // Send movement data with error checking
    struct {
        float deltaX;
        float deltaY;
        float stepSize;
    } cmd = {deltaX, deltaY, stepSize};
    
    std::cout << "[DEBUG CLIENT] About to send movement data: deltaX=" << cmd.deltaX 
              << ", deltaY=" << cmd.deltaY << ", stepSize=" << cmd.stepSize 
              << " (" << sizeof(cmd) << " bytes)" << std::endl;
    
    sent = send(clientSocket, &cmd, sizeof(cmd), MSG_NOSIGNAL);
    if (sent != sizeof(cmd)) {
        std::cerr << "[ERROR CLIENT] Failed to send movement command data: sent " << sent 
                  << " bytes, expected " << sizeof(cmd) << ", errno: " << errno 
                  << " (" << strerror(errno) << ")" << std::endl;
        if (errno == EPIPE || errno == ECONNRESET) {
            running = false;
        }
        return false;
    }
    
    std::cout << "[DEBUG CLIENT] Movement command sent successfully" << std::endl;
    
    // Clear the flag - we need to wait for next frame before sending another command
    canSendCommand = false;
    
    auto sendEndTime = std::chrono::high_resolution_clock::now();
    auto sendDuration = std::chrono::duration_cast<std::chrono::microseconds>(sendEndTime - sendStartTime).count();
    std::cout << "[TIMER CLIENT] Total send operation took " << sendDuration << "µs" << std::endl;
    
    return true;
}

void LPXDebugClient::receiverThread() {
    // Window should already be initialized from main thread
    std::cout << "[DEBUG] LPXDebugClient::receiverThread started" << std::endl;
    
    while (running) {
        std::cout << "[DEBUG] LPXDebugClient: Attempting to receive LPXImage..." << std::endl;
        
        // Receive an LPXImage with timeout handling
        auto image = LPXStreamProtocol::receiveLPXImage(clientSocket, scanTables);
        
        if (!image) {
            std::cout << "[DEBUG] LPXDebugClient: Connection lost or failed to receive image" << std::endl;
            running = false;
            break;
        } else {
            std::cout << "[DEBUG] LPXDebugClient: Received LPXImage with " << image->getLength() << " cells" << std::endl;
        }
        
        // Render the image using the existing multithreaded renderer
        auto startTime = std::chrono::high_resolution_clock::now();
        
        cv::Mat rendered = renderer->renderToImage(image, windowWidth, windowHeight, scale);
        
        if (rendered.empty()) {
            std::cout << "Failed to render image" << std::endl;
        } else {
            std::cout << "Successfully rendered image: " << rendered.cols << "x" << rendered.rows << std::endl;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        
        if (!rendered.empty()) {
            try {
                // Verify image properties before text operations
                if (rendered.channels() >= 3 && rendered.rows > 25 && rendered.cols > 200) {
                    // Display stats on the image
                    std::string stats = "Render: " + std::to_string(duration) + "ms, Cells: " 
                                       + std::to_string(image->getLength());
                    // Add stats to the rendered image with error handling
                    cv::putText(rendered, stats, cv::Point(10, 20), 
                              cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
                }
            } catch (const cv::Exception& e) {
                std::cerr << "OpenCV error in text rendering: " << e.what() << std::endl;
                // Continue without text - don't crash
            } catch (const std::exception& e) {
                std::cerr << "Error in text rendering: " << e.what() << std::endl;
                // Continue without text - don't crash
            }
            
            // Update shared buffer (threadsafe) for main thread to display
            try {
                std::lock_guard<std::mutex> lock(displayMutex);
                // Create a safe clone instead of copyTo to avoid memory issues
                latestImage = rendered.clone();
                newImageAvailable = true;
            } catch (const cv::Exception& e) {
                std::cerr << "OpenCV error in image cloning: " << e.what() << std::endl;
                running = false;  // Stop if we can't clone images safely
            } catch (const std::exception& e) {
                std::cerr << "Error in image cloning: " << e.what() << std::endl;
                running = false;  // Stop if we can't clone images safely
            }
            
            std::cout << "Image ready for display" << std::endl;
            
            // Frame received - now we can send the next movement command
            canSendCommand = true;
            
            // Check if we have a pending command to send
            {
                std::lock_guard<std::mutex> lock(pendingCommandMutex);
                if (hasPendingCommand) {
                    std::cout << "[DEBUG CLIENT] Frame received, sending pending command" << std::endl;
                    bool sent = sendMovementCommand(pendingDeltaX, pendingDeltaY, pendingStepSize);
                    if (sent) {
                        hasPendingCommand = false;
                        canSendCommand = false;  // Wait for next frame
                        std::cout << "[DEBUG CLIENT] Pending command sent after frame" << std::endl;
                    } else {
                        std::cout << "[DEBUG CLIENT] Failed to send pending command" << std::endl;
                    }
                }
            }
        }
    }
    
    // Don't destroy windows from receiver thread - let main thread handle cleanup
    std::cout << "Receiver thread stopped" << std::endl;
}

} // namespace lpx