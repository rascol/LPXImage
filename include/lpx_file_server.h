// lpx_file_server.h
#pragma once

#include "../include/lpx_mt.h"
#include "../include/lpx_renderer.h"
#include "../include/lpx_webcam_server.h"  // Reuse protocol from webcam server
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <set>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace lpx {

// Movement command for file server
struct MovementCommand {
    float deltaX;
    float deltaY;
    float stepSize;
};

// Extended protocol for file server with movement support
class FileLPXProtocol {
public:
    enum CommandType : uint32_t {
        CMD_LPX_IMAGE = 0x01,
        CMD_MOVEMENT = 0x02
    };
    
    // Send an LPXImage
    static bool sendLPXImage(int socket, const std::shared_ptr<LPXImage>& image);
    
    // Send a movement command to server
    static bool sendMovementCommand(int socket, float deltaX, float deltaY, float stepSize = 10.0f);
    
    // Receive a command (returns command type)
    static uint32_t receiveCommand(int socket, void* data, size_t maxSize);
};

class FileLPXServer {
public:
    FileLPXServer(const std::string& scanTableFile, int port = 5050);
    ~FileLPXServer();
    
    // Start streaming from a video file
    bool start(const std::string& videoFile, int width = 1920, int height = 1080);
    void stop();
    
    // FPS control
    void setFPS(float fps) { targetFPS = fps; }
    float getFPS() const { return targetFPS; }
    
    // Looping control
    void setLooping(bool loop);
    bool isLooping() const { return loopVideo.load(); }
    
    // Set log-polar center offset
    void setCenterOffset(float x_offset, float y_offset) {
        centerXOffset = x_offset;
        centerYOffset = y_offset;
    }
    
    // Check client count
    int getClientCount();
    
    // Handle movement command
    void handleMovementCommand(const MovementCommand& cmd);
    
private:
    // Thread functions
    void videoThread();
    void networkThread();
    void acceptClients();
    void handleClient(int clientSocket);
    
    // Components 
    std::shared_ptr<LPXTables> scanTables;
    cv::VideoCapture videoCapture;
    
    // Video file info
    std::string videoFile;
    int videoWidth = 0;
    int videoHeight = 0;
    float videoFPS = 0.0f;
    int totalFrames = 0;
    std::atomic<int> currentFrame;
    
    // Center offset for log-polar transform
    float centerXOffset = 0.0f;
    float centerYOffset = 0.0f;
    
    // LPXImage queue
    std::mutex lpxImageMutex;
    std::condition_variable lpxImageCondition;
    std::queue<std::shared_ptr<LPXImage>> lpxImageQueue;
    
    // Client management
    std::mutex clientsMutex;
    std::set<int> clientSockets;
    
    // Threading
    std::atomic<bool> running;
    std::thread videoThreadHandle;
    std::thread networkThreadHandle;
    std::thread acceptThreadHandle;
    std::thread commandThreadHandle;  // New thread for UDP commands
    
    // Network
    int serverSocket;  // TCP socket for streaming
    int commandSocket; // UDP socket for commands
    int port;
    
    // Video control
    std::atomic<float> targetFPS;
    std::atomic<bool> loopVideo;
    std::atomic<bool> restartVideoFlag{false};
    
    // Output size
    int outputWidth = 1920;
    int outputHeight = 1080;
};

} // namespace lpx
