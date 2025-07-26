#include "include/lpx_webcam_server.h"
#include <iostream>

int main() {
    std::cout << "Checking actual KEY_THROTTLE_MS value..." << std::endl;
    
    // Create a debug client just to access the constant
    try {
        lpx::LPXDebugClient client("ScanTables63");
        std::cout << "LPXDebugClient created successfully" << std::endl;
        
        // The KEY_THROTTLE_MS should be accessible as a static constexpr
        // Let's check what value is actually compiled in
        std::cout << "KEY_THROTTLE_MS value: " << lpx::LPXDebugClient::KEY_THROTTLE_MS << " ms" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
