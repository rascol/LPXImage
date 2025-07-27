// lpx_webcam_server.h
#pragma once

#include "../include/lpx_mt.h"
#include "../include/lpx_renderer.h"
#include "../include/lpx_version.h"
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

// Movement command structure
struct MovementCommand {
    float deltaX;
    float deltaY;
    float stepSize;
};

// Simple network protocol for LPXImage streaming
class LPXStreamProtocol {
public:
    enum CommandType : uint32_t {
        CMD_LPX_IMAGE = 0x01,
        CMD_MOVEMENT = 0x02
    };
    
    // Send a frame index over a socket
    static bool sendFrameIndex(int socket, int frameIndex);
    // Send an LPXImage over a socket
    static bool sendLPXImage(int socket, const std::shared_ptr<LPXImage>& image);
    
    // Receive a frame index from a socket
    static int receiveFrameIndex(int socket);
    // Receive an LPXImage from a socket
    static std::shared_ptr<LPXImage> receiveLPXImage(int socket, std::shared_ptr<LPXTables> scanTables);
    
    // Receive a command (returns command type)
    static uint32_t receiveCommand(int socket, void* data, size_t maxSize);
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
    
    // Movement command handling
    void handleMovementCommand(const MovementCommand& cmd);
    
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
    
    // Center offset for log-polar transform
    float centerXOffset = 0.0f;
    float centerYOffset = 0.0f;
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
    // Use centralized throttling constant
    static constexpr int KEY_THROTTLE_MS = lpx::KEY_THROTTLE_MS;
    
    // Frame synchronization - only one command per frame
    std::mutex pendingCommandMutex;
    bool hasPendingCommand = false;
    float pendingDeltaX = 0.0f;
    float pendingDeltaY = 0.0f;
    float pendingStepSize = 10.0f;
    std::atomic<bool> canSendCommand{true};  // Only true after receiving a frame
};

} // namespace lpx