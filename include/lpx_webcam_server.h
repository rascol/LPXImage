// lpx_webcam_server.h
#pragma once

#include "../include/lpx_mt.h"
#include "../include/lpx_renderer.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <set>
#include <string>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace lpx {

// Simple network protocol for LPXImage streaming
class LPXStreamProtocol {
public:
    // Send a frame index over a socket
    static bool sendFrameIndex(int socket, int frameIndex);
    // Send an LPXImage over a socket
    static bool sendLPXImage(int socket, const std::shared_ptr<LPXImage>& image);
    
    // Receive a frame index from a socket
    static int receiveFrameIndex(int socket);
    // Receive an LPXImage from a socket
    static std::shared_ptr<LPXImage> receiveLPXImage(int socket, std::shared_ptr<LPXTables> scanTables);
};

class WebcamLPXServer {
public:
    WebcamLPXServer(const std::string& scanTableFile, int port = 5050);
    ~WebcamLPXServer();
    
    bool start(int cameraId = 0, int width = 640, int height = 480);
    void stop();
    
    // Adaptive frame skip settings
    void setSkipRate(int min, int max, float motionThreshold = 5.0f);
    int getClientCount();
    
private:
    // Thread functions
    void captureThread(int cameraId);
    void processingThread();
    void networkThread();
    void acceptClients();
    
    // Adaptive processing
    void adjustSkipRate(float processingTime, bool hasMotion);
    float detectMotion(const cv::Mat& current, const cv::Mat& previous);
    
    // Components 
    std::shared_ptr<LPXTables> scanTables;
    
    // Frame and image queues
    std::mutex frameMutex;
    std::condition_variable frameCondition;
    std::queue<cv::Mat> frameQueue;
    cv::Mat previousGrayFrame;
    
    std::mutex lpxImageMutex;
    std::condition_variable lpxImageCondition;
    std::queue<std::shared_ptr<LPXImage>> lpxImageQueue;
    
    // Client management
    std::mutex clientsMutex;
    std::set<int> clientSockets;
    
    // Threading
    std::atomic<bool> running;
    std::thread captureThreadHandle;
    std::thread processingThreadHandle;
    std::thread networkThreadHandle;
    std::thread acceptThreadHandle;
    
    // Network
    int serverSocket;
    int port;
    
    // Adaptive frame skipping
    std::atomic<int> currentSkipRate;
    int minSkipRate = 2;
    int maxSkipRate = 6;
    float motionThreshold = 5.0f;
    std::vector<float> processingTimes;
    std::mutex timingMutex;
    
    // Webcam parameters
    int captureWidth = 640;
    int captureHeight = 480;
    int frameCount = 0;
};

class LPXDebugClient {
public:
    LPXDebugClient(const std::string& scanTableFile);
    ~LPXDebugClient();
    
    bool connect(const std::string& serverAddress, int port = 5050);
    void disconnect();
    
    // Display settings
    void setWindowTitle(const std::string& title) { windowTitle = title; }
    void setWindowSize(int width, int height);
    void setScale(float scale) { this->scale = scale; }
    
    // Initialize UI (must be called from main thread on macOS)
    void initializeWindow();
    
    // Process UI events (must be called from main thread on macOS)
    bool processEvents();
    
    // Check if client is still running
    bool isRunning() const { return running; }
    
    // Send movement command to server
    bool sendMovementCommand(float deltaX, float deltaY, float stepSize = 10.0f);
    
private:
    void receiverThread();
    
    // Components
    std::shared_ptr<LPXTables> scanTables;
    std::shared_ptr<LPXRenderer> renderer;
    
    // Network
    int clientSocket;
    std::string serverAddress;
    int port;
    
    // Threading
    std::atomic<bool> running;
    std::thread receiverThreadHandle;
    
    // Display
    std::string windowTitle = "LPX Debug View";
    int windowWidth = 800;
    int windowHeight = 600;
    float scale = 1.0f;
    
    // Shared image buffer for thread-safe display
    std::mutex displayMutex;
    cv::Mat latestImage;
    bool newImageAvailable = false;
    
    // Key throttling to prevent socket overflow
    std::chrono::steady_clock::time_point lastKeyTime;
    static constexpr int KEY_THROTTLE_MS = 500; // Minimum 500ms between movement commands
};

} // namespace lpx