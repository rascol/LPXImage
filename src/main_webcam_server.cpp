// main_webcam_server.cpp
#include "lpx_webcam_server.h"
#include <iostream>
#include <string>
#include <csignal>

lpx::WebcamLPXServer* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "Interrupt signal received, stopping server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(signum);
}

int main(int argc, char** argv) {
    // Path to scan tables file
    std::string scanTableFile = "../data/scan_tables.bin";
    int port = 5050;
    
    // Parse command line arguments
    if (argc > 1) {
        scanTableFile = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    
    // Register signal handler
    signal(SIGINT, signalHandler);
    
    try {
        // Create the server
        lpx::WebcamLPXServer server(scanTableFile, port);
        g_server = &server;
        
        // Configure adaptive frame skipping
        server.setSkipRate(2, 6, 5.0f);
        
        // Start the server with webcam
        if (!server.start(0, 1920, 1080)) {
            std::cerr << "Failed to start webcam server" << std::endl;
            return 1;
        }
        
        std::cout << "LPX webcam server started on port " << port << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;
        
        // Wait for termination
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // Periodically report status
            std::cout << "Connected clients: " << server.getClientCount() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}