/**
 * lpx_globals.cpp
 * 
 * Global variables used across multiple files
 */

#include "../include/lpx_image.h"

namespace lpx {

// Global shared instance of scan tables
std::shared_ptr<LPXTables> g_scanTables;

} // namespace lpx
