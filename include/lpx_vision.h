/**
 * lpx_vision.h
 * 
 * LPXVision class for log-polar vision processing
 * Converted from LPXVision JavaScript module
 */

#ifndef LPX_VISION_H
#define LPX_VISION_H

#include <vector>
#include <string>
#include <cmath>
#include <iostream>

// Forward declaration - use real LPXImage from the LPXImage library
namespace lpx {
    class LPXImage;
}
using LPXImage = lpx::LPXImage;

namespace lpx_vision {

// Constants from JavaScript
const double INV_2_PI = 1.0 / (2.0 * M_PI);
const double ANG0 = 3.0 * M_PI / 4.0;

const int NUM_IDENTIFIERS = 8;
const int NUM_IDENTIFIER_BITS = 3;
const double EIGHT_BIT_RANGE = 255.9999;
const int DIFFERENCE_BITS = 5;

/**
 * The cell name strings for the virtual LPXVision cell types.
 */
const std::string identifierName[NUM_IDENTIFIERS] = {
    "mwh", "hue", "mwh_x", "hue_x", "mwh_y", "hue_y", "mwh_z", "hue_z"
};

/**
 * Structure to hold min/max values with their indices
 */
struct MinMaxResult {
    double value;
    int index;
};

/**
 * Constructor for LPXVision objects with hexagonal cell shapes.
 * 
 * An LPXVision object encapsulates buffers of vision cells that 
 * can make much more reliable image to image comparisons than 
 * can LPXImage cells. LPXVision objects can also accurately detect 
 * motion while filtering out luminance flicker and camera noise. 
 * There are twelve virtual cell types at each vision cell location. 
 * These detect color and luminance amplitude and also color and 
 * luminance gradients in three directions at each cell location. 
 * This is a consequence of hexagonal cell shapes that have three 
 * gradient directions from each hexagonal cell six adjoining
 * neighbors.
 */
class LPXVision {
public:
    /**
     * Constructor for LPXVision objects.
     * 
     * @param lpxImage An LPXImage object that provides 
     * the data to populate the new LPXVision object.
     */
    LPXVision(LPXImage* lpxImage = nullptr);

    /**
     * Destructor
     */
    ~LPXVision();

    // Public properties (equivalent to JS properties)
    
    /**
     * The spiral period of this LPXVision object.
     */
    double spiralPer;
    
    /**
     * The startIndex relative to 0 at which to start a view
     * comparison. The number is automatically rounded so that
     * views can be accessed sequentially by startPer just by 
     * adding actual startPer values to startIndex.
     */
    int startIndex;
    
    /**
     * The index of the spiral period at which the 
     * view range starts.
     */
    double startPer;
    
    /**
     * A cell buffer index adjustment to the start of the
     * view range that can be used to correct for visual 
     * rotation in the LPXImage that created this LPXVision object 
     * object.
     */
    int tilt;
    
    /**
     * The length of the vision cell buffer each
     * of which has this length.
     */
    int length;
    
    /**
     * The total number of vision cell locations in the
     * view range.
     */
    int viewlength;
    
    /**
     * A starting index for image comparisons that can be 
     * used instead of this.startPer and this.tilt.
     */
    int viewIndex;
    
    /**
     * The horizontal offset from the center of a scanned 
     * standard image at which the LPXImage scan was made.
     * Positive direction is to the right.
     */
    double x_ofs;
    
    /**
     * The vertical offset from the center of a scanned 
     * standard image at which the LPXImage scan was made.
     * Positive direction is down.
     */
    double y_ofs;
    
    /**
     * The number of LPXVision cell types.
     */
    int numCellTypes;
    
    /**
     * All retina cells in a single array. These span
     * only the viewable range of cells from above the fovea.
     * So valid index starts at zero and length is less than
     * LPXImage length.
     */
    std::vector<uint64_t> retinaCells;

    // Public methods (equivalent to JS methods)
    
    /**
     * Gets the string name of the LPXVision cell identifiers
     * stored in a retinaCell. Each identifier occupies 3-bits and 
     * the identifiers are listed from the lowest 3-bit location 
     * to the highest.
     * 
     * @param i The index of the cell identifier.
     * @returns The type name of the LPXVision identifier in the retinaCell.
     */
    std::string getCellIdentifierName(int i) const;
    
    /**
     * Gets the index into the vision cell buffers of the start
     * of the view range. Internal values for 
     * 
     *   this.spiralPer - the length in cells of one revolution 
     *   of the spiral,
     *   
     *   this.startPer - the index of the spiral period counted 
     *   as 0, 1, 2, ..., from the start of this.retinaCells[].
     *      
     *   this.tilt - a cell offset value to account for tilting
     *   in the image
     *   
     * all need to have been assigned to this LPXVision object
     * before this function can be called. The this.spiralPer 
     * value will have been taken from the LPXImage object that 
     * created this LPXVision object but the others will typically 
     * need to have been assigned explicitly since their default 
     * values are all zero.
     * 
     * @returns The integer index value that starts the 
     * view range.
     */
    int getViewStartIndex() const;
    
    /**
     * Gets the total number of LPXVision cell locations in the view range.
     * 
     * @param spiralPer The number of LPXVision cell locations 
     * in one revolution of the log-polar spiral.
     * @returns The number of cell locations.
     */
    int getViewLength(double spiralPer = 0.0) const;
    
    /**
     * Constructs LPXVision cells from an LPXImage object. 
     * If lpImage.range is defined and non-zero then that value 
     * defines the length of the vision buffers. Otherwise, 
     * lpImage.length defines vision buffer length.
     * 
     * If lpD is defined, LPXVision cells are constructed for 
     * that LPXVision object instead.
     * 
     * This function is used externally.
     * 
     * @param lpImage The LPXImage object.
     * @param lpD Alternative LPXVision object 
     * that will receive the cells.
     */
    void makeVisionCells(LPXImage* lpImage, LPXVision* lpD = nullptr);

private:
    // Private helper functions (converted from JS helper functions)
    
    /**
     * Gets the total number of LPXVision cell locations in the view range.
     * 
     * @param spiralPer The number of LPXVision cell locations 
     * in one revolution of the log-polar spiral.
     * @returns The viewlength that has at least a number of spiral
     * revolutions equal to one-third of the spiral period but with the number 
     * of cells evenly divisible by four.
     */
    static int getViewLengthStatic(double spiralPer);
    
    /**
     * if ang == ANG0, gets the color angle mapped to the range, 
     * 0 to 2PI, but corresponding to the actual range -3PI/4 to 
     * 5PI/4 with blue at -PI/2, Green at 0, yellow at PI/2 and 
     * red at PI. The returned color ranges look like this:
     * 
     *   0 to PI/2    purple-blue to blue to blue-green
     *   PI/2 to PI   blue-green to green to green-yellow
     *   PI to 3PI/2  green-yellow to yellow to orange
     *   3PI/2 to 2PI orange to red to red-purple
     * 
     * If these ranges are normalized to PI/2 the result is
     * 
     *   0 to 1     blue range
     *   1 to 2     green range
     *   2 to 3     yellow range
     *   3 to 4     red range
     *   
     * Consequently, Math.floor(PI/2 normalized range) maps 
     * 
     *   0  blue range
     *   1  green range
     *   2  yellow range
     *   3  red range
     *   
     * If vector magnitude of the mgr and myb x and y identifiers 
     * is too small to reliably calculate color then the color is 
     * assumed to be white (gray) and the color is set to -3PI/4 
     * which is the purple-blue midway between red and blue and 
     * has output value 0.
     * 
     * @param myb Yellow - blue in the range -1023 to 1023.
     * @param mgr Green - red in the range -1023 to 1023.
     * @param ang
     * @returns Color angle in range 0 to 2PI
     */
    static double getColorAngle(double myb, double mgr, double ang);
    
    /**
     * Returns color difference as a number in range -PI to PI.
     *
     * @param color1 Color in range 0 to 2PI
     * @param color0 Color in range 0 to 2PI
     * @returns color difference
     */
    static double getColorDifference(double color1, double color0);
    
    /**
     * Sets LPXVision image cell identifier bit range to the value n
     * then shifts the bit range to the left by range_bits.
     * 
     * @param n
     * @param retinaCells
     * @param i
     * @param range_bits
     */
    static void setCellBits(int n, std::vector<uint64_t>& retinaCells, int i, int range_bits);
    
    /**
     * Rescales val to the range [movMin, movMax].
     *
     * @param val
     * @param movMin
     * @param movMax
     * @param idx
     * @returns The rescaled value.
     */
    static int rescaleToMinMax(double val, double movMin, double movMax, int idx);
    
    /**
     * Gets the minimum value of mwh[i] over the preceding
     * viewlength values of mwh[i] (including the current 
     * index) from current index i == idx.
     * 
     * @param mwh The monochrome magnitude array.
     * @param idx The current index of mwh.
     * @param viewlength The length of a view.
     * @returns {minVal, index_of_minVal}
     */
    static MinMaxResult getMovingMin(const std::vector<double>& mwh, int idx, int viewlength);
    
    /**
     * If mwh[j] is less than movMin then movMin is 
     * set to mwh[j]. Otherwise, if j is exactly one
     * view length from the last movMin then sets 
     * movMin to the lowest value of mwh[j] that 
     * occurred over the preceding viewlength values
     * of mwh[j].
     * 
     * @param mwh The monochrome magnitude array.
     * @param j The current index of mwh.
     * @param movMin The last minimum value of mwh[j].
     * @param movMinIdx The index of mwh at which movMin was captured.
     * @param viewlength The length of a view.
     * @returns {minVal, index_of_minVal}
     */
    static MinMaxResult getMovingMinParams(const std::vector<double>& mwh, int j, 
                                         double movMin, int movMinIdx, int viewlength);
    
    /**
     * Gets the maximum value of mwh[i] over the preceding
     * viewlength values of mwh[i] (including the current 
     * index) from current index i == idx.
     * 
     * @param mwh The monochrome magnitude array.
     * @param idx The current index of mwh.
     * @param viewlength The length of a view.
     * @returns {maxVal, index_of_maxVal}
     */
    static MinMaxResult getMovingMax(const std::vector<double>& mwh, int idx, int viewlength);
    
    /**
     * If mwh[j] is greater than movMax then movMax is 
     * set to mwh[j]. Otherwise, if j is exactly one 
     * view length from the last movMax then sets 
     * movMax to the greatest value of mwh[j] that 
     * occurred over the preceding viewlengh values 
     * of mwh[j].
     * 
     * @param mwh The monochrome magnitude array.
     * @param j The current index of mwh.
     * @param movMax The last maximum value of mwh[j].
     * @param movMaxIdx The index of mwh at which movMax was captured.
     * @param viewlength The length of a view.
     * @returns {maxVal, index_of_maxVal}
     */
    static MinMaxResult getMovingMaxParams(const std::vector<double>& mwh, int j, 
                                         double movMax, int movMaxIdx, int viewlength);
    
    /**
     * Expands each LPXImage cell from lpImage into twelve color 
     * LPXVision cells at each cell location and saves the cells to the 
     * cellBuffArray buffers. The expansion starts at the cell offset
     * given by lpImage.offset and extends over a cell span given
     * by lpR.viewlength.
     * 
     * @param lpR The LPXVision object containing, in
     * particular, a cellBuffArray array of NUM_IDENTIFIERS buffers 
     * with each buffer of length lpR.viewlength.
     * @param lpImage The LPXImage object that will fill
     * lpR.cellBufArray[NUM_IDENTIFIERS].
     */
    static void fillVisionCells(LPXVision* lpR, LPXImage* lpImage);
    
    /**
     * Constructs color LPXVision cells from an LPXImage object. 
     * If lpImage.range is defined and non-zero then that value 
     * defines the length of the vision buffers. Otherwise, 
     * lpImage.length defines vision buffer length.
     * 
     * This function is used externally by LPCortexImage.
     * 
     * @param lpR This LPXVision object.
     * @param lpImage The LPXImage object.
     */
    static void makeVisionCells(LPXVision* lpR, LPXImage* lpImage);
    
    /**
     * The LPXVision constructor initializer.
     * 
     * @param lpR This LPXVision object.
     * @param lpxImage The LPXImage from 
     * which the LPXVision object is constructed.
     */
    void initializeLPR(LPXImage* lpxImage);

    // Static distribution arrays (equivalent to JS module-level variables)
    static std::vector<std::vector<int>> distribArrays;
    static int cnt;
    static bool distribArraysInitialized;
};

} // namespace lpx_vision

#endif // LPX_VISION_H
