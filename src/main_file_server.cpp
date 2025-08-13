#include "../include/lpx_file_server.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <cstring>

using namespace lpx;

FileLPXServer* server = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nReceived SIGINT, shutting down server..." << std::endl;
        if (server) {
            server->stop();
        }
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <scan_table_file> <video_file> [port] [width] [height]" << std::endl;
        std::cerr << "Example: " << argv[0] << " data/scan-6000-63.sct data/test_video.mp4 8080 640 480" << std::endl;
        return -1;
    }
    
    std::string scanTableFile = argv[1];
    std::string videoFile = argv[2];
    int port = (argc > 3) ? std::atoi(argv[3]) : 8080;
    int width = (argc > 4) ? std::atoi(argv[4]) : 640;
    int height = (argc > 5) ? std::atoi(argv[5]) : 480;
    
    std::cout << "Starting file server with:" << std::endl;
    std::cout << "  Scan table: " << scanTableFile << std::endl;
    std::cout << "  Video file: " << videoFile << std::endl;
    std::cout << "  Port: " << port << std::endl;
    std::cout << "  Output size: " << width << "x" << height << std::endl;
    
    // Install signal handler
    signal(SIGINT, signalHandler);
    
    try {
        // Create server
        server = new FileLPXServer(scanTableFile, port);
        
        // Set loop enabled for continuous playback
        server->setLooping(true);
        
        // Start the server
        if (!server->start(videoFile, width, height)) {
            std::cerr << "Failed to start file server" << std::endl;
            return -1;
        }
        
        std::cout << "File server started successfully!" << std::endl;
        std::cout << "Streaming video on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server..." << std::endl;
        
        // Run the server
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Optionally print status
            int clientCount = server->getClientCount();
            if (clientCount > 0) {
                static int lastClientCount = -1;
                if (clientCount != lastClientCount) {
                    std::cout << "Active clients: " << clientCount << std::endl;
                    lastClientCount = clientCount;
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    // Clean up
    if (server) {
        server->stop();
        delete server;
    }
    
    return 0;
}
