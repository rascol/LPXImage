// lpx_version.h - Build version tracking for LPXImage
#pragma once

namespace lpx {
    // Version constants - using proper C++ constants instead of macros
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    
    // Throttling configuration - clearly documented
    constexpr int KEY_THROTTLE_MS = 16;  // 16ms (~60fps) for responsive WASD
    
    // Functions to get version and build info at runtime
    // These are populated automatically at compile time with actual build timestamps
    const char* getVersionString();
    const char* getBuildTimestamp();  // When the library was compiled
    const char* getBuildDate();       // Date of compilation
    const char* getBuildTime();       // Time of compilation
    int getBuildNumber();             // Legacy compatibility - returns timestamp hash
    int getKeyThrottleMs();
    
    // Print build information when library is loaded
    void printBuildInfo();
}
