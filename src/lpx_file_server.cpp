// lpx_file_server.cpp
#include "../include/lpx_file_server.h"
#include <iostream>
#include <chrono>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>  // For TCP_NODELAY
#include <cstring>        // For strerror

namespace lpx {

// FileLPXProtocol implementation
bool FileLPXProtocol::sendLPXImage(int socket, const std::shared_ptr<LPXImage>& image) {
    std::cout << "[DEBUG] FileLPXProtocol: Sending LPX image" << std::endl;
    
    // Use the existing LPXStreamProtocol directly (no command type prefix for image broadcasts)
    // Command types are only used for client-to-server communication
    return LPXStreamProtocol::sendLPXImage(socket, image);
}

bool FileLPXProtocol::sendMovementCommand(int socket, float deltaX, float deltaY, float stepSize) {
    std::cout << "[DEBUG] FileLPXProtocol: Sending movement command (" << deltaX << ", " << deltaY << ")" << std::endl;
    
    // Send command type
    uint32_t cmdType = CMD_MOVEMENT;
    if (send(socket, &cmdType, sizeof(cmdType), 0) != sizeof(cmdType)) {
        std::cerr << "[ERROR] Failed to send movement command type" << std::endl;
        return false;
    }
    
    // Send movement data
    MovementCommand cmd = {deltaX, deltaY, stepSize};
    if (send(socket, &cmd, sizeof(cmd), 0) != sizeof(cmd)) {
        std::cerr << "[ERROR] Failed to send movement command data" << std::endl;
        return false;
    }
    
    return true;
}

uint32_t FileLPXProtocol::receiveCommand(int socket, void* data, size_t maxSize) {
    // Receive command type (non-blocking)
    uint32_t cmdType;
    ssize_t result = recv(socket, &cmdType, sizeof(cmdType), 0);
    
    if (result != sizeof(cmdType)) {
        // For non-blocking sockets, EAGAIN/EWOULDBLOCK means no data available
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // This is normal for non-blocking sockets - no data available
            return 0;
        }
        // For streaming clients that don't send commands, result=0 is normal (client closed gracefully)
        if (result == 0) {
            // Client disconnected gracefully
            return 0;
        }
        // Only log error for actual connection issues (negative result)
        if (result < 0) {
            std::cerr << "[ERROR] Failed to receive command type: result=" << result 
                      << ", errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
        }
        return 0;
    }
    
    std::cout << "[DEBUG] FileLPXProtocol: Received command type " << cmdType << std::endl;
    
    if (cmdType == CMD_MOVEMENT) {
        if (maxSize >= sizeof(MovementCommand)) {
            if (recv(socket, data, sizeof(MovementCommand), 0) != sizeof(MovementCommand)) {
                std::cerr << "[ERROR] Failed to receive movement command data: " << strerror(errno) << std::endl;
                return 0;
            }
        } else {
            std::cerr << "[ERROR] Buffer too small for movement command" << std::endl;
            return 0;
        }
    }
    
    return cmdType;
}

// FileLPXServer implementation
FileLPXServer::FileLPXServer(const std::string& scanTableFile, int port) 
    : port(port), serverSocket(-1), running(false), targetFPS(30.0f), loopVideo{false},
      currentFrame(0), centerXOffset(0.0f), centerYOffset(0.0f), restartVideoFlag{false} {
    
    // Initialize scan tables
    scanTables = std::make_shared<LPXTables>(scanTableFile);
    if (!scanTables->isInitialized()) {
        throw std::runtime_error("Failed to initialize scan tables from: " + scanTableFile);
    }

    g_scanTables = scanTables;
    std::cout << "[DEBUG] FileLPXServer constructor: loopVideo initialized to " << loopVideo.load() << std::endl;
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
    videoCapture.open(videoFile);
    if (!videoCapture.isOpened()) {
        std::cerr << "Failed to open video file: " << videoFile << std::endl;
        return false;
    }
    
    // Get video properties
    videoWidth = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH));
    videoHeight = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
    videoFPS = videoCapture.get(cv::CAP_PROP_FPS);
    totalFrames = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_COUNT));
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
    
    // Start threads
    videoThreadHandle = std::thread(&FileLPXServer::videoThread, this);
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
    
    // Join video thread last
    if (videoThreadHandle.joinable()) {
        std::cout << "Waiting for video thread to stop..." << std::endl;
        videoThreadHandle.join();
        std::cout << "Video thread stopped" << std::endl;
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
    std::cout << "[DEBUG] FileLPXServer::setLooping IMPLEMENTATION called with: " << (loop ? "true" : "false") << std::endl;
    std::cout << "[DEBUG] Current loopVideo address: " << &loopVideo << std::endl;
    std::cout << "[DEBUG] About to assign value " << loop << " to loopVideo atomic" << std::endl;
    loopVideo = loop;  // Try direct assignment instead of .store()
    std::cout << "[DEBUG] FileLPXServer::setLooping - atomic value is now: " << loopVideo.load() << std::endl;
    std::cout << "[DEBUG] FileLPXServer::setLooping IMPLEMENTATION completed" << std::endl;
}

void FileLPXServer::handleMovementCommand(const MovementCommand& cmd) {
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
    
    std::cout << "[DEBUG] New center offset (bounded): (" << centerXOffset << ", " << centerYOffset << ")" << std::endl;
}

void FileLPXServer::videoThread() {
    float frameDelayMs = 1000.0f / targetFPS;
    std::cout << "Frame delay: " << frameDelayMs << "ms" << std::endl;
    
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        // Check if video restart is requested
        if (restartVideoFlag.load()) {
            std::cout << "Video thread: Restarting video for sync" << std::endl;
            videoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
            currentFrame = 0;
            restartVideoFlag = false;
        }
        
        // Skip processing if no clients connected
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (clientSockets.empty()) {
                // Reduced sleep for faster client recognition
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
        }
        
        // Read a frame from the video
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
        
        currentFrame++;
        
        // Convert to RGB (LPXImage expects RGB format)
        cv::Mat frameRGB;
        cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);
        
        // Resize if necessary
        if (frameRGB.cols != outputWidth || frameRGB.rows != outputHeight) {
            cv::resize(frameRGB, frameRGB, cv::Size(outputWidth, outputHeight));
        }
        
        // Process the frame with LPX transform
        float centerX = outputWidth / 2.0f + centerXOffset;
        float centerY = outputHeight / 2.0f + centerYOffset;
        
        auto lpxImage = multithreadedScanImage(frameRGB, centerX, centerY);
        
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
            
            // Report progress periodically
            if (currentFrame % 100 == 0 || currentFrame == totalFrames) {
                std::cout << "Processed frame " << currentFrame << "/" << totalFrames 
                          << " (" << (100.0f * currentFrame / totalFrames) << "%)" << std::endl;
            }
        }
        
        // Control FPS
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastFrameTime).count();
        
        if (elapsedMs < frameDelayMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(frameDelayMs - elapsedMs)));
        }
        
        lastFrameTime = std::chrono::high_resolution_clock::now();
    }
    
    std::cout << "Video thread stopped" << std::endl;
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
                // Send image
                if (!FileLPXProtocol::sendLPXImage(clientSocket, imageToSend)) {
                    disconnectedClients.push_back(clientSocket);
                    continue;
                }
                
                // Check for movement commands (non-blocking)
                MovementCommand cmd;
                uint32_t cmdType = FileLPXProtocol::receiveCommand(clientSocket, &cmd, sizeof(cmd));
                
                if (cmdType == FileLPXProtocol::CMD_MOVEMENT) {
                    std::cout << "[DEBUG] Received movement command from client " << clientSocket << std::endl;
                    handleMovementCommand(cmd);
                } else if (cmdType == 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    // Client disconnected
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
            
            // Add to clients set (no per-client threads)
            bool isFirstClient = false;
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                isFirstClient = clientSockets.empty();
                std::cout << "[DEBUG] Client count before adding: " << clientSockets.size() 
                          << ", isFirstClient: " << isFirstClient << std::endl;
                clientSockets.insert(clientSocket);
                
                // Set client socket to non-blocking for command polling
                int flags = fcntl(clientSocket, F_GETFL, 0);
                fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
                
                std::cout << "[DEBUG] Client " << clientSocket << " added to active set" << std::endl;
            }
            
            // If this is the first client, signal video thread to restart for sync
            if (isFirstClient) {
                std::cout << "First client connected - signaling video restart for sync" << std::endl;
                restartVideoFlag = true; // Signal video thread to restart
                
                // Clear any old frames from queue
                std::lock_guard<std::mutex> queueLock(lpxImageMutex);
                while (!lpxImageQueue.empty()) {
                    lpxImageQueue.pop();
                }
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // An actual error occurred
            if (running) {
                std::cerr << "Error accepting client connection: " << strerror(errno) << std::endl;
            }
        }
        
        // Sleep briefly to avoid CPU burnout - reduced for faster client acceptance
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "Accept thread stopped" << std::endl;
}

void FileLPXServer::handleClient(int clientSocket) {
    std::cout << "[DEBUG] Client handler started for socket " << clientSocket << std::endl;
    
    // Set socket to non-blocking for movement command reception
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
    
    while (running) {
        // Check for incoming movement commands
        MovementCommand cmd;
        uint32_t cmdType = FileLPXProtocol::receiveCommand(clientSocket, &cmd, sizeof(cmd));
        
        if (cmdType == FileLPXProtocol::CMD_MOVEMENT) {
            std::cout << "[DEBUG] Received movement command from client " << clientSocket << std::endl;
            handleMovementCommand(cmd);
        } else if (cmdType == 0) {
            // No command received or error - check if socket is still valid
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << "[DEBUG] Client " << clientSocket << " disconnected or error occurred" << std::endl;
                break;
            }
        }
        
        // Sleep briefly to avoid CPU burnout - keep responsive for commands
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
