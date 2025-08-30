/**
 * lpx_vision.cpp
 * 
 * LPXVision implementation for log-polar vision processing
 * Converted from LPXVision JavaScript module
 */

#include "lpx_vision.h"
#include "lpx_image.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// NOTE: LPXImage class needs to be defined or included here
// For now, using forward declaration and basic interface assumptions

namespace lpx_vision {

// Static member initialization
std::vector<std::vector<int>> LPXVision::distribArrays;
int LPXVision::cnt = 0;
bool LPXVision::distribArraysInitialized = false;

// Constructor
LPXVision::LPXVision(LPXImage* lpxImage)
    : spiralPer(0.0)
    , startIndex(0)
    , startPer(0.0)
    , tilt(0)
    , length(0)
    , viewlength(0)
    , viewIndex(0)
    , x_ofs(0.0)
    , y_ofs(0.0)
    , numCellTypes(NUM_IDENTIFIERS)
{
    initializeLPR(lpxImage);
}

// Destructor
LPXVision::~LPXVision() {
    // Cleanup if needed
}

// Public methods

std::string LPXVision::getCellIdentifierName(int i) const {
    if (i >= 0 && i < NUM_IDENTIFIERS) {
        return identifierName[i];
    }
    return "";
}

int LPXVision::getViewStartIndex() const {
    return static_cast<int>(std::floor(startPer * spiralPer + tilt));
}

int LPXVision::getViewLength(double spiralPer_param) const {
    if (spiralPer_param == 0.0) {
        return getViewLengthStatic(spiralPer);
    }
    return getViewLengthStatic(spiralPer_param);
}

void LPXVision::makeVisionCells(LPXImage* lpImage, LPXVision* lpD) {
    if (lpD != nullptr) {
        makeVisionCells(lpD, lpImage);
    } else {
        makeVisionCells(this, lpImage);
    }
}

// Private static helper functions

/**
 * Gets the total number of LPXVision cell locations in the view range.
 * 
 * @param spiralPer The number of LPXVision cell locations 
 * in one revolution of the log-polar spiral.
 * @returns The viewlength that has at least a number of spiral
 * revolutions equal to one-third of the spiral period but with the number 
 * of cells evenly divisible by four.
 */
int LPXVision::getViewLengthStatic(double spiralPer) {
    int sp = static_cast<int>(std::floor(spiralPer));
    spiralPer = sp + 0.5; // Exact spiralPer
    int vp = static_cast<int>(std::round(spiralPer / 3.0)); // Approximate desired number of spiral periods in the view
    int viewlength = static_cast<int>(std::round(vp * spiralPer));
    
    while ((viewlength % 4) != 0) {
        viewlength += 1;
    }
    
    return viewlength;
}

/**
 * if ang == ANG0, gets the color angle mapped to the range, 
 * 0 to 2PI, but corresponding to the actual range -3PI/4 to 
 * 5PI/4 with blue at -PI/2, Green at 0, yellow at PI/2 and 
 * red at PI.
 */
double LPXVision::getColorAngle(double myb, double mgr, double ang) {
    double angle;
    double mag = std::sqrt(myb * myb + mgr * mgr);
    
    if (mag < 50) { // Color is gray
        angle = 0.0;
    } else { // Color in the range: purple/blue through red/purple
        angle = std::atan2(myb, mgr); // Returns angle in the range -PI to PI.
        if (angle < -ang) { // So set angle range to -3PI/4 to 5PI/4
            angle = M_PI + (M_PI + angle);
        }
        
        angle += ang; // Rotate angle by 3PI/4
    }
    
    return angle; // Returns a value in range 0 to 2PI
}

/**
 * Returns color difference as a number in range -PI to PI.
 */
double LPXVision::getColorDifference(double color1, double color0) {
    double diff = color1 - color0;
    
    if (diff > M_PI) {
        diff = diff - 2 * M_PI;
    } else if (diff < -M_PI) {
        diff = diff + 2 * M_PI;
    }
    
    return diff;
}

/**
 * Sets LPXVision image cell identifier bit range to the value n
 * then shifts the bit range to the left by range_bits.
 */
void LPXVision::setCellBits(int n, std::vector<uint64_t>& retinaCells, int i, int range_bits) {
    uint64_t before = retinaCells[i];
    retinaCells[i] = (retinaCells[i] | n);
    retinaCells[i] = (retinaCells[i] << range_bits);
    
    // Debug output removed
}

/**
 * Rescales val to the range [movMin, movMax].
 */
int LPXVision::rescaleToMinMax(double val, double movMin, double movMax, int idx) {
    val = std::floor(val);
    
    if (val < movMin) {
        val = movMin;
    } else if (val > movMax) {
        val = movMax;
    }
    
    double range = movMax - movMin;
    if (range < 10) {
        range = 10;
    }
    
    val = std::round(255 * (val - movMin) / range); // Scaled to 8 bits
    return static_cast<int>(val);
}

/**
 * Gets the minimum value of mwh[i] over the preceding
 * viewlength values of mwh[i] (including the current 
 * index) from current index i == idx.
 */
MinMaxResult LPXVision::getMovingMin(const std::vector<double>& mwh, int idx, int viewlength) {
    double minVal = 1023;
    int minIdx = 0;
    
    for (int i = idx - viewlength + 1; i <= idx; i += 1) {
        double mwh_i = mwh[i];
        if (mwh_i < minVal) {
            minVal = mwh_i;
            minIdx = i;
        }
    }
    
    return {minVal, minIdx};
}

/**
 * If mwh[j] is less than movMin then movMin is 
 * set to mwh[j]. Otherwise, if j is exactly one
 * view length from the last movMin then sets 
 * movMin to the lowest value of mwh[j] that 
 * occurred over the preceding viewlength values
 * of mwh[j].
 */
MinMaxResult LPXVision::getMovingMinParams(const std::vector<double>& mwh, int j, 
                                         double movMin, int movMinIdx, int viewlength) {
    double mwh_j = mwh[j];
    if (mwh_j < movMin) {
        movMin = mwh_j;
        movMinIdx = j;
    } else if ((j - viewlength) == movMinIdx) {
        MinMaxResult min = getMovingMin(mwh, j, viewlength);
        movMin = min.value;
        movMinIdx = min.index;
    }
    
    return {movMin, movMinIdx};
}

/**
 * Gets the maximum value of mwh[i] over the preceding
 * viewlength values of mwh[i] (including the current 
 * index) from current index i == idx.
 */
MinMaxResult LPXVision::getMovingMax(const std::vector<double>& mwh, int idx, int viewlength) {
    double maxVal = 0;
    int maxIdx = 0;
    
    for (int i = idx - viewlength + 1; i <= idx; i += 1) {
        double mwh_i = mwh[i];
        if (mwh_i > maxVal) {
            maxVal = mwh_i;
            maxIdx = i;
        }
    }
    
    return {maxVal, maxIdx};
}

/**
 * If mwh[j] is greater than movMax then movMax is 
 * set to mwh[j]. Otherwise, if j is exactly one 
 * view length from the last movMax then sets 
 * movMax to the greatest value of mwh[j] that 
 * occurred over the preceding viewlengh values 
 * of mwh[j].
 */
MinMaxResult LPXVision::getMovingMaxParams(const std::vector<double>& mwh, int j, 
                                         double movMax, int movMaxIdx, int viewlength) {
    double mwh_j = mwh[j];
    if (mwh_j > movMax) {
        movMax = mwh_j;
        movMaxIdx = j;
    } else if ((j - viewlength) == movMaxIdx) {
        MinMaxResult max = getMovingMax(mwh, j, viewlength);
        movMax = max.value;
        movMaxIdx = max.index;
    }
    
    return {movMax, movMaxIdx};
}

/**
 * Expands each LPXImage cell from lpImage into twelve color 
 * LPXVision cells at each cell location and saves the cells to the 
 * cellBuffArray buffers. The expansion starts at the cell offset
 * given by lpImage.offset and extends over a cell span given
 * by lpR.viewlength.
 */
void LPXVision::fillVisionCells(LPXVision* lpR, LPXImage* lpImage) {
    // Converted to match JavaScript implementation exactly
    
    if (!lpImage) {
        return;
    }
    
    // Access private members via friend class - match JavaScript variable names and calculations exactly
    int spPer = static_cast<int>(std::floor(lpImage->spiralPer));
    std::vector<uint32_t>& cellArray = lpImage->cellArray;
    int foveaPeriods = static_cast<int>(std::floor(spPer * 0.1));  // Match JavaScript: Math.floor(spPer * 0.1)
    int foveaOfs = spPer * foveaPeriods;
    int viewlength = lpR->viewlength;
    int viewOfs = viewlength + 1;
    int mwhOfs = viewOfs + spPer;
    int length = lpImage->length;
    int comparelen = length - foveaOfs;
    
    // Debug output removed
    
    lpR->length = comparelen;
    
    // Initialize moving min/max variables
    double mwhMovMin, mwhMovMax;
    int mwhMovMinIdx, mwhMovMaxIdx;
    double mwh_xMovMin, mwh_xMovMax;
    int mwh_xMovMinIdx, mwh_xMovMaxIdx;
    double mwh_yMovMin, mwh_yMovMax;
    int mwh_yMovMinIdx, mwh_yMovMaxIdx;
    double mwh_zMovMin, mwh_zMovMax;
    int mwh_zMovMinIdx, mwh_zMovMaxIdx;
    
    // Initialize arrays - match JavaScript array sizes exactly
    std::vector<double> mwh(comparelen + mwhOfs);
    std::vector<double> mgr(comparelen + viewOfs);
    std::vector<double> myb(comparelen + viewOfs);
    std::vector<double> mwh_x(comparelen + mwhOfs);
    std::vector<double> mwh_y(comparelen + mwhOfs);
    std::vector<double> mwh_z(comparelen + mwhOfs);
    std::vector<double> hue(comparelen + viewOfs);
    
    int i, j, k, n;
    double diff, wht;
    
    // Clear existing retina cells
    lpR->retinaCells.clear();
    
    // LOOP 1: Begin construction of arrays back one view length - EXACT JavaScript match
    for (i = 0; i < mwhOfs; i++) {
        int cellIdx = i + foveaOfs - mwhOfs;
        if (cellIdx >= 0 && cellIdx < static_cast<int>(cellArray.size())) {
            mwh[i] = lpImage->extractCellLuminance(cellArray[cellIdx]);
            // Debug output removed
        } else {
            mwh[i] = 0;
            // Debug output removed
        }
        
        if (i < spPer) {
            mwh_x[i] = 0;
            mwh_y[i] = 0;
            mwh_z[i] = 0;
        } else {
            mwh_x[i] = 512 + (mwh[i] - mwh[i-1]) / 4;
            mwh_y[i] = 512 + (mwh[i] - mwh[i-spPer-1]) / 4;
            mwh_z[i] = 512 + (mwh[i] - mwh[i-spPer]) / 4;
            // Debug output removed
        }
    }
    
    // LOOP 2: Main processing loop for mwh and hue - EXACT JavaScript match
    for (i = 0; i < comparelen; i += 1) {
        lpR->retinaCells.push_back(0);
        
        j = i + mwhOfs;  // NOTE: This is the ONLY loop that uses mwhOfs for j
        k = i + foveaOfs;
        
        if (i == 0) {
            MinMaxResult min = getMovingMin(mwh, j, viewlength);
            mwhMovMin = min.value;
            mwhMovMinIdx = min.index;
            
            MinMaxResult max = getMovingMax(mwh, j, viewlength);
            mwhMovMax = max.value;
            mwhMovMaxIdx = max.index;
        }
        
        // The monochrome identifier in the range 0 to 1023
        mwh[j] = lpImage->extractCellLuminance(cellArray[k]);
        
        // Range check removed
        
        MinMaxResult min = getMovingMinParams(mwh, j, mwhMovMin, mwhMovMinIdx, viewlength);
        mwhMovMin = min.value;
        mwhMovMinIdx = min.index;
        
        MinMaxResult max = getMovingMaxParams(mwh, j, mwhMovMax, mwhMovMaxIdx, viewlength);
        mwhMovMax = max.value;
        mwhMovMaxIdx = max.index;
        
        wht = rescaleToMinMax(mwh[j], mwhMovMin, mwhMovMax, j);
        
        // Range checks removed
        
        n = static_cast<int>(std::floor(wht)); // Converts wht to an 8-bit value in the range 0 to 255
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
        
        mgr[j] = lpImage->extractCellGreenRed(cellArray[k]); // green-red identifier in the range -1024 to 1023
        myb[j] = lpImage->extractCellYellowBlue(cellArray[k]); // yellow-blue identifier in the range -1024 to 1023
        
        hue[j] = getColorAngle(myb[j], mgr[j], ANG0); // Generate color1 angle in range 0 to 2PI
        
        n = static_cast<int>(std::floor(EIGHT_BIT_RANGE * INV_2_PI * hue[j])); // Convert color to an 8-bit value in the range 0 to 255
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
    }
    
    // LOOP 3: Form cell forward differences along the spiral - EXACT JavaScript match
    for (i = 0; i < comparelen; i += 1) { // Form cell forward differences along the spiral
        j = i + viewOfs;  // JavaScript uses viewOfs, NOT mwhOfs
        
        if (i == 0) {
            MinMaxResult min = getMovingMin(mwh_x, j, viewlength);
            mwh_xMovMin = min.value;
            mwh_xMovMinIdx = min.index;
            
            MinMaxResult max = getMovingMax(mwh_x, j, viewlength);
            mwh_xMovMax = max.value;
            mwh_xMovMaxIdx = max.index;
        }
        
        mwh_x[j] = static_cast<int>(std::floor(512 + (mwh[j] - mwh[j-1]) / 4));
        
        MinMaxResult min = getMovingMinParams(mwh_x, j, mwh_xMovMin, mwh_xMovMinIdx, viewlength);
        mwh_xMovMin = min.value;
        mwh_xMovMinIdx = min.index;
        
        MinMaxResult max = getMovingMaxParams(mwh_x, j, mwh_xMovMax, mwh_xMovMaxIdx, viewlength);
        mwh_xMovMax = max.value;
        mwh_xMovMaxIdx = max.index;
        
        diff = rescaleToMinMax(mwh_x[j], mwh_xMovMin, mwh_xMovMax, j);
        
        n = static_cast<int>(std::floor(diff));
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
        
        double hue_x = getColorDifference(hue[j], hue[j-1]); // Color difference in range -PI to PI
        
        n = static_cast<int>(std::floor(EIGHT_BIT_RANGE * INV_2_PI * (hue_x + M_PI))); // Set 8-bit range
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
    }
    
    // LOOP 4: Form cell forward differences -60 degrees from spiral (mwh_y) - EXACT JavaScript match
    for (i = 0; i < comparelen; i += 1) { // Form cell forward differences -60 degrees from spiral
        j = i + viewOfs;  // JavaScript uses viewOfs, NOT mwhOfs
        
        if (i == 0) {
            MinMaxResult min = getMovingMin(mwh_y, j, viewlength);
            mwh_yMovMin = min.value;
            mwh_yMovMinIdx = min.index;
            
            MinMaxResult max = getMovingMax(mwh_y, j, viewlength);
            mwh_yMovMax = max.value;
            mwh_yMovMaxIdx = max.index;
        }
        
        mwh_y[j] = static_cast<int>(std::floor(512 + (mwh[j] - mwh[j-spPer-1]) / 4));
        
        MinMaxResult min = getMovingMinParams(mwh_y, j, mwh_yMovMin, mwh_yMovMinIdx, viewlength);
        mwh_yMovMin = min.value;
        mwh_yMovMinIdx = min.index;
        
        MinMaxResult max = getMovingMaxParams(mwh_y, j, mwh_yMovMax, mwh_yMovMaxIdx, viewlength);
        mwh_yMovMax = max.value;
        mwh_yMovMaxIdx = max.index;
        
        diff = rescaleToMinMax(mwh_y[j], mwh_yMovMin, mwh_yMovMax, j);
        
        n = static_cast<int>(std::floor(diff));
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
        
        double hue_y = getColorDifference(hue[j], hue[j-spPer-1]);
        n = static_cast<int>(std::floor(EIGHT_BIT_RANGE * INV_2_PI * (hue_y + M_PI))); // Set 8-bit range
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
    }
    
    // LOOP 5: Form cell forward differences -120 degrees from spiral (mwh_z) - EXACT JavaScript match
    for (i = 0; i < comparelen; i += 1) { // Form cell forward differences -120 degrees from spiral
        j = i + viewOfs;  // JavaScript uses viewOfs, NOT mwhOfs
        
        if (i == 0) {
            MinMaxResult min = getMovingMin(mwh_z, j, viewlength);
            mwh_zMovMin = min.value;
            mwh_zMovMinIdx = min.index;
            
            MinMaxResult max = getMovingMax(mwh_z, j, viewlength);
            mwh_zMovMax = max.value;
            mwh_zMovMaxIdx = max.index;
        }
        
        mwh_z[j] = static_cast<int>(std::floor(512 + (mwh[j] - mwh[j-spPer]) / 4));
        
        MinMaxResult min = getMovingMinParams(mwh_z, j, mwh_zMovMin, mwh_zMovMinIdx, viewlength);
        mwh_zMovMin = min.value;
        mwh_zMovMinIdx = min.index;
        
        MinMaxResult max = getMovingMaxParams(mwh_z, j, mwh_zMovMax, mwh_zMovMaxIdx, viewlength);
        mwh_zMovMax = max.value;
        mwh_zMovMaxIdx = max.index;
        
        diff = rescaleToMinMax(mwh_z[j], mwh_zMovMin, mwh_zMovMax, j);
        
        n = static_cast<int>(std::floor(diff));
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, NUM_IDENTIFIER_BITS);
        
        double hue_z = getColorDifference(hue[j], hue[j-spPer]);
        n = static_cast<int>(std::floor(EIGHT_BIT_RANGE * INV_2_PI * (hue_z + M_PI))); // Set 8-bit range
        n = n >> DIFFERENCE_BITS; // Remove the range comparison bits
        
        setCellBits(n, lpR->retinaCells, i, 0); // JavaScript uses 0 for final setCellBits call
    }
    
    // Debug output removed
}

/**
 * Constructs color LPXVision cells from an LPXImage object. 
 * If lpImage.range is defined and non-zero then that value 
 * defines the length of the vision buffers. Otherwise, 
 * lpImage.length defines vision buffer length.
 */
void LPXVision::makeVisionCells(LPXVision* lpR, LPXImage* lpImage) {
    // Simplified implementation that works with public interface
    if (!lpImage) {
        return;
    }
    
    // Use public interface only
    lpR->length = lpImage->getLength();
    
    fillVisionCells(lpR, lpImage);
}

/**
 * The LPXVision constructor initializer.
 */
void LPXVision::initializeLPR(LPXImage* lpxImage) {
    // Initialize distribution arrays if not already done
    if (!distribArraysInitialized) {
        cnt = 0;
        
        distribArrays.resize(NUM_IDENTIFIERS);
        
        int nVals = static_cast<int>(std::pow(2, NUM_IDENTIFIER_BITS));
        
        for (int i = 0; i < NUM_IDENTIFIERS; i += 1) {
            std::vector<int> distrib(nVals, 0);
            distribArrays[i] = distrib;
        }
        
        distribArraysInitialized = true;
    }
    
    if (lpxImage != nullptr) {
        // Simplified implementation using public interface only
        spiralPer = lpxImage->getSpiralPeriod();
        // Debug output removed
        
        int calculatedViewLength = getViewLength(spiralPer);
        // Debug output removed
        
        viewlength = static_cast<int>(std::floor(calculatedViewLength));
        // Debug output removed
        
        x_ofs = static_cast<double>(lpxImage->getXOffset());
        y_ofs = static_cast<double>(lpxImage->getYOffset());
        
        makeVisionCells(this, lpxImage);
    }
}

} // namespace lpx_vision


// ORIGINAL JAVASCRIPT CODE FOR REFERENCE:
// The original JavaScript code that was converted to C++ can be found in:
// src/lpx_vision.js
