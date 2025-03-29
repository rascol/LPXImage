/**
 * lpx_constants.h
 * 
 * Common constants for log-polar image processing
 */

#ifndef LPX_CONSTANTS_H
#define LPX_CONSTANTS_H

#include <cmath>

namespace lpx {

// Geometric constants for log-polar calculations
const float TWO_PI = 2.0f * M_PI;
const float FOUR_PI = 2.0f * TWO_PI;
const float ONE_THIRD = 1.0f / 3.0f;
const float sv_A = M_PI * std::sqrt(3.0f);  // Spiral construction constant for hexagonal cells
const float r0 = 0.455f;  // Radius in pixels to the center of the LPXImage cell at absolute angle zero

} // namespace lpx

#endif // LPX_CONSTANTS_H
