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
    // COMMAND RECEPTION RE-ENABLED WITH PROPER ERROR HANDLING
    // We'll receive commands but with better error checking to prevent
    // socket desynchronization issues
    
    uint32_t cmdType;
    ssize_t received = recv(socket, &cmdType, sizeof(cmdType), MSG_DONTWAIT);
    
    if (received == sizeof(cmdType)) {
        // Successfully received command type
        if (cmdType == CMD_MOVEMENT) {
            // Receive movement command data
            MovementCommand cmd;
            received = recv(socket, &cmd, sizeof(cmd), MSG_DONTWAIT);
            if (received == sizeof(cmd)) {
                // Copy command data to output buffer if provided
                if (data && maxSize >= sizeof(cmd)) {
                    memcpy(data, &cmd, sizeof(cmd));
                }
                return cmdType;
            } else {
                std::cerr << "[ERROR] Failed to receive complete movement command data" << std::endl;
                return 0;
            }
        } else {
            std::cerr << "[ERROR] Unknown command type: 0x" << std::hex << cmdType << std::dec << std::endl;
            return 0;
        }
    } else if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // No data available (non-blocking), this is normal
        return 0;
    } else {
        // Error or partial read
        if (received == 0) {
            std::cout << "[DEBUG] Client disconnected during command reception" << std::endl;
        } else {
            std::cerr << "[ERROR] Failed to receive command type: " << strerror(errno) << std::endl;
        }
        return 0;
    }
}

// FileLPXServer implementation
FileLPXServer::FileLPXServer(const std::string& scanTableFile, int port) 
    : port(port), serverSocket(-1), commandSocket(-1), running(false), targetFPS(30.0f), loopVideo{false},
      currentFrame(0), centerXOffset(0.0f), centerYOffset(0.0f), restartVideoFlag{false} {
    
    // Initialize scan tables
TIME_OPERATION("Scan table loading", {
        scanTables = std::make_shared<LPXTables>(scanTableFile);
    });
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

void FileLPXServer::videoThread() {
    float frameDelayMs = 1000.0f / targetFPS;
    std::cout << "Frame delay: " << frameDelayMs << "ms" << std::endl;
    
    bool isFirstFrame = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        // Check if video restart is requested
        if (restartVideoFlag.load()) {
            videoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
            currentFrame = 0;
            restartVideoFlag = false;
        }
        
        // Skip processing if no clients connected
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (clientSockets.empty()) {
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
        auto lpxProcessStart = std::chrono::high_resolution_clock::now();
        float centerX = outputWidth / 2.0f + centerXOffset;
        float centerY = outputHeight / 2.0f + centerYOffset;
        
        auto lpxImage = multithreadedScanImage(frameRGB, centerX, centerY);
        auto lpxProcessEnd = std::chrono::high_resolution_clock::now();
        auto lpxDuration = std::chrono::duration_cast<std::chrono::milliseconds>(lpxProcessEnd - lpxProcessStart).count();
        
        // Log slow LPX processing
        if (lpxDuration > 100) {  // Log if processing takes more than 100ms
            std::cout << "[TIMING] LPX processing took: " << lpxDuration << "ms (slow!)" << std::endl;
        }
        
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
        
        if (isFirstFrame) {
            isFirstFrame = false;
        }
        
        // Control FPS - but only after first frame is processed
        if (!isFirstFrame) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastFrameTime).count();
            
            if (elapsedMs < frameDelayMs) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    static_cast<int>(frameDelayMs - elapsedMs)));
            }
        }
        
        // Update lastFrameTime AFTER first frame processing is complete
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
                // Check for incoming movement commands first
                MovementCommand cmd;
                uint32_t cmdType = FileLPXProtocol::receiveCommand(clientSocket, &cmd, sizeof(cmd));
                if (cmdType == FileLPXProtocol::CMD_MOVEMENT) {
                    std::cout << "[DEBUG] Received movement command from client " << clientSocket << std::endl;
                    handleMovementCommand(cmd);
                }
                
                // Send image
                if (!FileLPXProtocol::sendLPXImage(clientSocket, imageToSend)) {
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
