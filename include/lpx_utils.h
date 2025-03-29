/**
 * lpx_utils.h
 * 
 * Common utility functions for log-polar image processing
 */

#ifndef LPX_UTILS_H
#define LPX_UTILS_H

#include <cmath>

namespace lpx {

// Constants
const float TWO_PI = 2.0f * M_PI;
const float ONE_THIRD = 1.0f / 3.0f;
const float sv_A = M_PI * std::sqrt(3.0f);  // Spiral construction constant for hexagonal cells
const float r0 = 0.455f;  // Radius in pixels to the center of the LPXImage cell at absolute angle zero

/**
 * Calculate the index of the LPXImage cell that contains the point (x, y)
 * 
 * @param x Horizontal displacement from the center of the spiral
 * @param y Vertical displacement from the center of the spiral
 * @param spiralPer The integer number of cells per revolution of the spiral
 * @return The integer index of the cell containing the point (x,y)
 */
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

} // namespace lpx

#endif // LPX_UTILS_H
