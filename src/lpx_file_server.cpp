// lpx_file_server.cpp
#include "../include/lpx_file_server.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>  // For TCP_NODELAY
#include <cstring>        // For strerror and memcpy

namespace lpx {

// Timing helper function
void logTiming(const std::string& operation, std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "[TIMING] " << operation << " took: " << duration << "μs (" 
              << std::fixed << std::setprecision(2) << (duration / 1000.0) << "ms)" << std::endl;
}

#define TIME_OPERATION(name, code) { auto start = std::chrono::high_resolution_clock::now(); code; logTiming(name, start); }

// FileLPXServer implementation
FileLPXServer::FileLPXServer(const std::string& scanTableFile, int port) 
    : port(port), serverSocket(-1), commandSocket(-1), running(false), targetFPS(30.0f), loopVideo{false},
      currentFrame(0), centerXOffset(0.0f), centerYOffset(0.0f), restartVideoFlag{false} {
    
    // Initialize scan tables
    scanTables = std::make_shared<LPXTables>(scanTableFile);
    if (!scanTables->isInitialized()) {
        throw std::runtime_error("Failed to initialize scan tables from: " + scanTableFile);
    }

    g_scanTables = scanTables;
}

FileLPXServer::~FileLPXServer() {
    stop();
}

bool FileLPXServer::start(const std::string& videoFile, int width, int height) {
    if (running) return false;
    this->videoFile = videoFile;
    outputWidth = width;
    outputHeight = height;
    
    // Open the video file
TIME_OPERATION("Video file opening", {
        videoCapture.open(videoFile);
    });
    if (!videoCapture.isOpened()) {
        std::cerr << "Failed to open video file: " << videoFile << std::endl;
        return false;
    }
    
    // Get video properties
TIME_OPERATION("Video property reading", {
        videoWidth = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH));
        videoHeight = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
        videoFPS = videoCapture.get(cv::CAP_PROP_FPS);
        totalFrames = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_COUNT));
    });
    currentFrame = 0;
    
    // If not specified, use video's native FPS
    if (targetFPS <= 0.0f) {
        targetFPS.store(videoFPS);
    }
    
    std::cout << "Opened video file: " << videoFile << std::endl;
    std::cout << "Video properties: " << videoWidth << "x" << videoHeight 
              << ", " << videoFPS << " FPS, " << totalFrames << " frames" << std::endl;
    std::cout << "Output size: " << outputWidth << "x" << outputHeight << std::endl;
    std::cout << "Target FPS: " << targetFPS << std::endl;
    bool currentLoopStatus = loopVideo.load();
    std::cout << "[DEBUG] loopVideo.load() returns: " << currentLoopStatus << std::endl;
    std::cout << "Looping: " << (currentLoopStatus ? "Yes" : "No") << std::endl;
    std::cout << "Center offset: (" << centerXOffset << ", " << centerYOffset << ")" << std::endl;
    
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
    
    // Start threads (matching WebcamLPXServer architecture)
    captureThreadHandle = std::thread(&FileLPXServer::captureThread, this);
    processingThreadHandle = std::thread(&FileLPXServer::processingThread, this);
    networkThreadHandle = std::thread(&FileLPXServer::networkThread, this);
    acceptThreadHandle = std::thread(&FileLPXServer::acceptClients, this);
    
    std::cout << "FileLPXServer started on port " << port << std::endl;
    return true;
}

void FileLPXServer::stop() {
    if (!running) return;
    
    std::cout << "Stopping server..." << std::endl;
    running = false;
    
    // Wake up waiting threads
    frameCondition.notify_all();
    lpxImageCondition.notify_all();
    
    // Close server socket first to stop accepting new connections
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
    
    // Join network and accept threads FIRST (before closing client sockets)
    // This ensures they stop trying to use the sockets
    if (networkThreadHandle.joinable()) {
        std::cout << "Waiting for network thread to stop..." << std::endl;
        networkThreadHandle.join();
        std::cout << "Network thread stopped" << std::endl;
    }
    if (acceptThreadHandle.joinable()) {
        std::cout << "Waiting for accept thread to stop..." << std::endl;
        acceptThreadHandle.join();
        std::cout << "Accept thread stopped" << std::endl;
    }
    
    // NOW it's safe to close client sockets since network thread has stopped
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::cout << "Closing " << clientSockets.size() << " client connections..." << std::endl;
        for (int clientSocket : clientSockets) {
            shutdown(clientSocket, SHUT_RDWR); // Disallow further sends and receives
            close(clientSocket);
        }
        clientSockets.clear();
    }
    
    // Join capture and processing threads
    if (captureThreadHandle.joinable()) {
        std::cout << "Waiting for capture thread to stop..." << std::endl;
        captureThreadHandle.join();
        std::cout << "Capture thread stopped" << std::endl;
    }
    if (processingThreadHandle.joinable()) {
        std::cout << "Waiting for processing thread to stop..." << std::endl;
        processingThreadHandle.join();
        std::cout << "Processing thread stopped" << std::endl;
    }
    
    // Close video
    videoCapture.release();
    
    std::cout << "FileLPXServer stopped" << std::endl;
}

int FileLPXServer::getClientCount() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clientSockets.size();
}

void FileLPXServer::setLooping(bool loop) {
    loopVideo = loop;
}

void FileLPXServer::setCenterOffset(float x, float y) {
    centerXOffset = x;
    centerYOffset = y;
    std::cout << "[DEBUG] FileLPXServer: Center offset set to (" << x << ", " << y << ")" << std::endl;
}

void FileLPXServer::handleMovementCommand(const MovementCommand& cmd) {
    auto cmdStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "[TIMER] Server received movement command at " << std::chrono::duration_cast<std::chrono::microseconds>(cmdStartTime.time_since_epoch()).count() << "μs" << std::endl;
    std::cout << "[DEBUG] Handling movement command: (" << cmd.deltaX << ", " << cmd.deltaY 
              << ") step=" << cmd.stepSize << std::endl;
    
    // Apply movement with step size
    centerXOffset += cmd.deltaX * cmd.stepSize;
    centerYOffset += cmd.deltaY * cmd.stepSize;
    
    // Apply bounds checking to prevent crashes
    // Keep center within reasonable bounds relative to scan map size (not output size)
    // The scan table mapWidth represents the full scanning region
    if (scanTables && scanTables->mapWidth > 0) {
        // Reduce bounds to be more conservative - 20% instead of 40%
        float maxOffsetX = scanTables->mapWidth * 0.2f;  // Allow center to move up to 20% of scan map width
        float maxOffsetY = scanTables->mapWidth * 0.2f;  // Assume square scan map for height
        
        std::cout << "[DEBUG] Scan table mapWidth: " << scanTables->mapWidth 
                  << ", max offset bounds: ±" << maxOffsetX << std::endl;
        
        centerXOffset = std::max(-maxOffsetX, std::min(maxOffsetX, centerXOffset));
        centerYOffset = std::max(-maxOffsetY, std::min(maxOffsetY, centerYOffset));
    } else {
        // Fallback to output dimensions if scan tables not available
        float maxOffsetX = outputWidth * 0.4f;
        float maxOffsetY = outputHeight * 0.4f;
        
        centerXOffset = std::max(-maxOffsetX, std::min(maxOffsetX, centerXOffset));
        centerYOffset = std::max(-maxOffsetY, std::min(maxOffsetY, centerYOffset));
    }
    
    auto cmdEndTime = std::chrono::high_resolution_clock::now();
    auto cmdDuration = std::chrono::duration_cast<std::chrono::microseconds>(cmdEndTime - cmdStartTime).count();
    std::cout << "[TIMER] Server processed movement command in " << cmdDuration << "μs" << std::endl;
    std::cout << "[DEBUG] New center offset (bounded): (" << centerXOffset << ", " << centerYOffset << ")" << std::endl;
}

void FileLPXServer::captureThread() {
    std::cout << "Video file capture thread started" << std::endl;
    
    // Calculate frame interval based on target FPS
    float currentTargetFPS = targetFPS.load();
    auto frameInterval = std::chrono::microseconds(static_cast<int64_t>(1000000.0f / currentTargetFPS));
    std::cout << "[DEBUG] Target FPS: " << currentTargetFPS << ", frame interval: " 
              << frameInterval.count() << "μs (" << (frameInterval.count() / 1000.0f) << "ms)" << std::endl;
    
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        cv::Mat frame;
        if (!videoCapture.read(frame)) {
            // End of video
            if (loopVideo.load()) {
                std::cout << "End of video, looping back to start" << std::endl;
                videoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
                currentFrame = 0;
                continue;
            } else {
                std::cout << "End of video reached" << std::endl;
                break;
            }
        }
        
        // Convert from RGB to BGR since video files provide RGB data 
        // but OpenCV and our scanning pipeline expect BGR format
        cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
        
        currentFrame++;
        
        // Resize if necessary
        if (frame.cols != outputWidth || frame.rows != outputHeight) {
            cv::resize(frame, frame, cv::Size(outputWidth, outputHeight));
        }
        
        // Simple approach: Add every frame to processing queue (like early webcam servers)
        {
            std::unique_lock<std::mutex> lock(frameMutex);
            
            // Prevent queue from growing too large
            while (frameQueue.size() >= 3) {
                frameQueue.pop();
            }
            
            frameQueue.push(frame.clone());
            lock.unlock();
            
            frameCondition.notify_one();
        }
        
        // Sleep to maintain target FPS
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - lastFrameTime;
        
        if (elapsed < frameInterval) {
            auto sleepTime = frameInterval - elapsed;
            std::this_thread::sleep_for(sleepTime);
        }
        
        // Update target FPS if it changed
        float newTargetFPS = targetFPS.load();
        if (newTargetFPS != currentTargetFPS) {
            currentTargetFPS = newTargetFPS;
            frameInterval = std::chrono::microseconds(static_cast<int64_t>(1000000.0f / currentTargetFPS));
            std::cout << "[DEBUG] Updated target FPS to: " << currentTargetFPS 
                      << ", new frame interval: " << frameInterval.count() << "μs" << std::endl;
        }
        
        lastFrameTime = std::chrono::high_resolution_clock::now();
        
        // Report progress periodically
        if (currentFrame % 100 == 0 || currentFrame == totalFrames) {
            std::cout << "Captured frame " << currentFrame << "/" << totalFrames 
                      << " (" << (100.0f * currentFrame / totalFrames) << "%)" << std::endl;
        }
    }
    
    std::cout << "Capture thread stopped" << std::endl;
}

void FileLPXServer::processingThread() {
    std::cout << "[DEBUG] File server processing thread started" << std::endl;
    while (running) {
        cv::Mat frameToProcess;
        
        // Wait for a frame to process
        {
            std::unique_lock<std::mutex> lock(frameMutex);
            while (frameQueue.empty() && running) {
                frameCondition.wait(lock);
            }
            
            if (!running) break;
            
            std::cout << "[DEBUG] Processing frame in file server thread" << std::endl;
            frameToProcess = frameQueue.front();
            frameQueue.pop();
        }
        
        // Process the frame
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Center the LPX scan at the center of the image with offsets
        float centerX = frameToProcess.cols / 2.0f + centerXOffset;
        float centerY = frameToProcess.rows / 2.0f + centerYOffset;
        
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
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        // Log slow processing
        if (duration > 100) {
            std::cout << "[TIMING] LPX processing took: " << duration << "ms (slow!)" << std::endl;
        }
    }
    
    std::cout << "Processing thread stopped" << std::endl;
}

void FileLPXServer::networkThread() {
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
        
        // Send to all clients and check for movement commands
        if (imageToSend) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::vector<int> disconnectedClients;
            
            for (int clientSocket : clientSockets) {
                // Check for incoming movement commands first
                MovementCommand cmd;
                uint32_t cmdType = LPXStreamProtocol::receiveCommand(clientSocket, &cmd, sizeof(cmd));
                if (cmdType == LPXStreamProtocol::CMD_MOVEMENT) {
                    std::cout << "[DEBUG] Received movement command from client " << clientSocket << std::endl;
                    handleMovementCommand(cmd);
                }
                
                // Send image
                if (!LPXStreamProtocol::sendLPXImage(clientSocket, imageToSend)) {
                    disconnectedClients.push_back(clientSocket);
                    continue;
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

void FileLPXServer::acceptClients() {
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
            
            // Optimize socket for low-latency streaming
            int flag = 1;
            if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
                std::cerr << "Warning: Failed to set TCP_NODELAY" << std::endl;
            }
            
            // Set smaller send buffer for faster flushing
            int sendbuf = 64 * 1024; // 64KB instead of default
            if (setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf)) < 0) {
                std::cerr << "Warning: Failed to set send buffer size" << std::endl;
            }
            
            // Add to clients set (matches WebcamLPXServer behavior)
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clientSockets.insert(clientSocket);
                
                // Set client socket to non-blocking for command polling
                int flags = fcntl(clientSocket, F_GETFL, 0);
                fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
                
                std::cout << "[DEBUG] Client " << clientSocket << " added to active set. Total clients: " << clientSockets.size() << std::endl;
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // An actual error occurred
            if (running) {
                std::cerr << "Error accepting client connection: " << strerror(errno) << std::endl;
            }
        }
        
        // Sleep briefly to avoid CPU burnout - matches WebcamLPXServer timing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Accept thread stopped" << std::endl;
}

void FileLPXServer::handleClient(int clientSocket) {
    std::cout << "[DEBUG] Client handler started for socket " << clientSocket << std::endl;
    
    // This client handler is now dedicated solely to streaming.
    // No command polling is performed here to avoid socket desynchronization.
    // Commands should be handled via a separate communication channel.
    
    // Just keep the socket alive - streaming is handled by the main video thread
    while (running) {
        // Check if socket is still connected by testing if it's readable
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int result = select(clientSocket + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (result < 0) {
            std::cout << "[DEBUG] Client " << clientSocket << " select error" << std::endl;
            break;
        } else if (result > 0 && FD_ISSET(clientSocket, &readfds)) {
            // Check if client disconnected by trying to peek at data
            char buffer[1];
            ssize_t peekResult = recv(clientSocket, buffer, 1, MSG_PEEK | MSG_DONTWAIT);
            if (peekResult == 0) {
                std::cout << "[DEBUG] Client " << clientSocket << " disconnected gracefully" << std::endl;
                break;
            } else if (peekResult < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << "[DEBUG] Client " << clientSocket << " disconnected with error" << std::endl;
                break;
            }
        }
        
        // Sleep briefly to avoid CPU burnout
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Clean up this client
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientSockets.erase(clientSocket);
        close(clientSocket);
    }
    
    std::cout << "[DEBUG] Client handler stopped for socket " << clientSocket << std::endl;
}

} // namespace lpx
