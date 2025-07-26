// lpx_version.cpp - Implementation of version functions
#include "../include/lpx_version.h"
#include <iostream>
#include <sstream>

namespace lpx {
    // Create version string with build timestamp
    const char* getVersionString() {
        static std::string versionStr = []() {
            std::ostringstream oss;
            oss << VERSION_MAJOR << "." << VERSION_MINOR << "." << __DATE__ << "-" << __TIME__;
            return oss.str();
        }();
        return versionStr.c_str();
    }
    
    const char* getBuildTimestamp() {
        static std::string timestampStr = []() {
            std::ostringstream oss;
            oss << __DATE__ << " " << __TIME__;
            return oss.str();
        }();
        return timestampStr.c_str();
    }
    
    const char* getBuildDate() {
        return __DATE__;
    }
    
    const char* getBuildTime() {
        return __TIME__;
    }
    
    int getBuildNumber() {
        // Generate a hash from the build timestamp for legacy compatibility
        static int buildNumber = []() {
            std::string timestamp = getBuildTimestamp();
            std::hash<std::string> hasher;
            return static_cast<int>(hasher(timestamp) % 100000); // Keep it reasonable
        }();
        return buildNumber;
    }
    
    int getKeyThrottleMs() {
        return KEY_THROTTLE_MS;
    }
    
    void printBuildInfo() {
        std::cout << "============================================================" << std::endl;
        std::cout << "LPXImage Library v" << getVersionString() << std::endl;
        std::cout << "Built: " << getBuildTimestamp() << std::endl;
        std::cout << "Key Throttle: " << getKeyThrottleMs() << "ms" << std::endl;
        std::cout << "============================================================" << std::endl;
    }
    
    // Static initialization to print build info when library is loaded
    namespace {
        struct BuildInfoPrinter {
            BuildInfoPrinter() {
                printBuildInfo();
            }
        };
        static BuildInfoPrinter buildInfoPrinter;
    }
}
