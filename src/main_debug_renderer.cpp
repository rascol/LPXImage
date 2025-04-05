// main_debug_renderer.cpp
#include "lpx_webcam_server.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // Default parameters
    std::string scanTableFile = "../data/scan_tables.bin";
    std::string serverAddress = "localhost";
    int port = 5050;
    
    // Parse command line arguments
    if (argc > 1) {
        serverAddress = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    if (argc > 3) {
        scanTableFile = argv[3];
    }
    
    try {
        // Create the debug client
        lpx::LPXDebugClient client(scanTableFile);
        
        // Configure display
        client.setWindowTitle("LPX Debug View");
        client.setWindowSize(800, 600);
        client.setScale(1.0f);
        
        // Connect to server
        std::cout << "Connecting to " << serverAddress << ":" << port << std::endl;
        if (!client.connect(serverAddress, port)) {
            std::cerr << "Failed to connect to server" << std::endl;
            return 1;
        }
        
        std::cout << "Connected to LPX webcam server" << std::endl;
        std::cout << "Receiving and displaying LPXImages..." << std::endl;
        std::cout << "Press ESC in the video window to exit" << std::endl;
        
        // The client thread is already running and showing the window
        // Just wait for user input to ensure the program doesn't exit immediately
        std::cin.get();
        
        // Disconnect and clean up
        client.disconnect();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}