/**
 * lpx_common.h
 * 
 * Common utility functions and constants for log-polar vision
 */

#ifndef LPX_COMMON_H
#define LPX_COMMON_H

#include <cmath>
#include <iostream>
#include <string>

namespace lpx {

// Logging system
enum LogLevel {
    LOG_ERROR = 0,    // Only critical errors
    LOG_WARNING = 1,  // Important issues that don't prevent execution
    LOG_INFO = 2,     // Key processing stages and timing information
    LOG_DEBUG = 3     // Detailed information (many messages)
};

// Global log level - default to INFO
extern LogLevel g_logLevel;

// Function to set the global log level
inline void setLogLevel(LogLevel level) {
    g_logLevel = level;
}

// Logging function with level check
inline void log(LogLevel level, const std::string& message) {
    if (level <= g_logLevel) {
        switch (level) {
            case LOG_ERROR:
                std::cerr << "ERROR: " << message << std::endl;
                break;
            case LOG_WARNING:
                std::cerr << "WARNING: " << message << std::endl;
                break;
            case LOG_INFO:
                std::cout << message << std::endl;
                break;
            case LOG_DEBUG:
                std::cout << "DEBUG: " << message << std::endl;
                break;
        }
    }
}

// Convenience macros for different log levels
#define LOG_ERROR(msg) lpx::log(lpx::LOG_ERROR, msg)
#define LOG_WARNING(msg) lpx::log(lpx::LOG_WARNING, msg)
#define LOG_INFO(msg) lpx::log(lpx::LOG_INFO, msg)
#define LOG_DEBUG(msg) lpx::log(lpx::LOG_DEBUG, msg)

// Constants
const float TWO_PI = 2.0f * M_PI;
const float ONE_THIRD = 1.0f / 3.0f;
const float sv_A = M_PI * std::sqrt(3.0f);  // Spiral construction constant for hexagonal cells
const float r0 = 0.455f;  // Radius in pixels to the center of the LPXImage cell at absolute angle zero
const float FLOAT_EPSILON = 0.001f;  // Epsilon for float comparisons

// Float comparison function for near equality
inline bool floatEquals(float a, float b, float epsilon = FLOAT_EPSILON) {
    return std::abs(a - b) < epsilon;
}

// Calculate the index of the LPXImage cell that contains the point (x, y)
inline int getXCellIndex(float x, float y, float spiralPer) {
    if (x == 0 && y == 0) {
        return 0;
    }

    spiralPer = std::floor(spiralPer) + 0.5f;

    float radius = std::sqrt(x * x + y * y);
    float angle = std::atan2(y, x);

    float pitch = 1.0f / spiralPer;
    float pitchAng = 0.99999999f * TWO_PI * pitch;  // Fixup for round off error
    float invPitchAng = 1.0f / pitchAng;

    float ang = (angle < 0.0f) ? angle + TWO_PI : angle;  // Map angles to range 0 to TWO_PI

    float arg = ang * invPitchAng;
    float j = 2 * arg - 0.0000001f;  // Offset the angle enough that the low boundary is included in the cell
    float sv_A_pitch_1 = sv_A * pitch + 1;

    int iPer = static_cast<int>(((4.0f * M_PI * std::log(radius / r0) / std::log(sv_A_pitch_1) * invPitchAng) - j) * pitch * 0.5f);

    int iPer_2_spiralPer = static_cast<int>(iPer * 2 * spiralPer);

    int iCell_2 = iPer_2_spiralPer + static_cast<int>(j);  // Half-period index

    float absAng = 0.5f * (iPer_2_spiralPer + j) * pitchAng;

    float ang1 = 0.5f * iCell_2 * pitchAng;  // Absolute ang1 on half-cell boundaries

    float r1 = r0 * std::pow(sv_A_pitch_1, (absAng / TWO_PI));  // Radius through center of cell at ang

    float r2 = r1 * sv_A_pitch_1;  // Radius through center of cells at next spiral period
    float s_2 = (r2 - r1) * ONE_THIRD;

    int iCell = static_cast<int>(iCell_2 / 2);  // Index of bounding cell

    float dr = radius - r1;  // The part of radius within r1 to r2
    float da = absAng - ang1;  // The part of ang in the half-cell with lower bound ang1

    if (dr < s_2) {  // Region 1
        return iCell;
    }
    
    if (dr >= s_2 && dr < 2.0f * s_2) {
        float width = M_PI * pitch;
        float bound = width * (dr - s_2) / s_2;

        if (iCell_2 % 2 > 0) {  // If in the upper half-cell
            if (da >= width - bound) {  // If Region 4
                iCell = iCell + static_cast<int>(spiralPer) + 1;
                return iCell;
            } else {  // Else if Region 3
                return iCell;
            }
        } else {  // Else in the lower half-cell
            if (da < bound) {  // If Region 5
                iCell = iCell + static_cast<int>(spiralPer);
            }
            return iCell;  // Else if Region 2
        }
    } else {  // if (dr >= 2.0 * s_2)
        if (iCell_2 % 2 > 0) {  // If Region 4
            iCell = iCell + static_cast<int>(spiralPer) + 1;
        } else {  // Else if Region 5
            iCell = iCell + static_cast<int>(spiralPer);
        }
        return iCell;
    }
}

inline float getSpiralRadius(int length, float spiralPer) {
    // Validate spiralPer
    if (spiralPer < 0.1f) {
        LOG_ERROR("Invalid spiral period in getSpiralRadius: " + std::to_string(spiralPer));
        return 600.0f; // Default fallback
    }
    
    // Match JavaScript: return r0 * Math.pow((sv_A / spiralPer) + 1, revs);
    float sv_A_pitch_1 = (sv_A / spiralPer) + 1.0f;
    
    // In JavaScript, length parameter is the total cell count,
    // and revs = length / spiralPer
    float revs = static_cast<float>(length) / spiralPer;
    float radius = r0 * std::pow(sv_A_pitch_1, revs);
    
    LOG_DEBUG("getSpiralRadius - length=" + std::to_string(length) + 
              ", spiralPer=" + std::to_string(spiralPer) + 
              ", revs=" + std::to_string(revs) + 
              ", radius=" + std::to_string(radius));
    
    return radius;
}

// Get cell array offset for scaling
inline int getCellArrayOffset(float scaleFactor, float spiralPer) {
    int sp = static_cast<int>(std::floor(spiralPer));

    float ofs = -spiralPer * std::log(scaleFactor) / std::log((sv_A / spiralPer) + 1.0f);

    ofs = std::floor(spiralPer * std::round(ofs / spiralPer));

    if (sp % 2 == 0) {
        ofs -= sp;
    }
    return static_cast<int>(ofs);
}

} // namespace lpx

#endif // LPX_COMMON_H
