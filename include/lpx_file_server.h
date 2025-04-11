// lpx_file_server.h
#pragma once

#include "../include/lpx_mt.h"
#include "../include/lpx_renderer.h"
#include "../include/lpx_webcam_server.h"  // Reuse protocol from webcam server
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <set>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace lpx {

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
    void setLooping(bool loop) { loopVideo = loop; }
    bool isLooping() const { return loopVideo; }
    
    // Set log-polar center offset
    void setCenterOffset(float x_offset, float y_offset) {
        centerXOffset = x_offset;
        centerYOffset = y_offset;
    }
    
    // Check client count
    int getClientCount();
    
private:
    // Thread functions
    void videoThread();
    void networkThread();
    void acceptClients();
    
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
    
    // Network
    int serverSocket;
    int port;
    
    // Video control
    std::atomic<float> targetFPS;
    std::atomic<bool> loopVideo;
    
    // Output size
    int outputWidth = 1920;
    int outputHeight = 1080;
};

} // namespace lpx
