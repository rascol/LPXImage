// lpx_file_server.cpp
#include "../include/lpx_file_server.h"
#include <iostream>
#include <chrono>
#include <arpa/inet.h>
#include <fcntl.h>

namespace lpx {

// FileLPXServer implementation
FileLPXServer::FileLPXServer(const std::string& scanTableFile, int port) 
    : port(port), serverSocket(-1), running(false), targetFPS(30.0f), loopVideo(false),
      currentFrame(0), centerXOffset(0.0f), centerYOffset(0.0f) {
    
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
    std::cout << "Looping: " << (loopVideo ? "Yes" : "No") << std::endl;
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
    
    running = false;
    
    // Wake up waiting threads
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
    if (videoThreadHandle.joinable()) videoThreadHandle.join();
    if (networkThreadHandle.joinable()) networkThreadHandle.join();
    if (acceptThreadHandle.joinable()) acceptThreadHandle.join();
    
    // Close video
    videoCapture.release();
    
    std::cout << "FileLPXServer stopped" << std::endl;
}

int FileLPXServer::getClientCount() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clientSockets.size();
}

void FileLPXServer::videoThread() {
    float frameDelayMs = 1000.0f / targetFPS;
    std::cout << "Frame delay: " << frameDelayMs << "ms" << std::endl;
    
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        // Skip processing if no clients connected
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (clientSockets.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        
        // Read a frame from the video
        cv::Mat frame;
        if (!videoCapture.read(frame)) {
            // End of video
            if (loopVideo) {
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

} // namespace lpx
