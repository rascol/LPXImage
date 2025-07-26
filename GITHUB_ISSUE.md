# FileLPXServer Startup Timing Bug - Inconsistent Long Delays

## Summary
The FileLPXServer experiences inconsistent startup delays of 5-10+ seconds when clients connect, significantly impacting user experience. While one timing bug has been partially fixed, the underlying synchronization issue remains unresolved.

## Environment
- **Platform**: macOS
- **Compiler**: C++17 with CMake
- **Dependencies**: OpenCV, pthread
- **Affected Component**: `src/lpx_file_server.cpp`

## Problem Description

### Symptoms
- **Expected behavior**: First frame should be available within 1-3 seconds of client connection
- **Actual behavior**: Inconsistent delays ranging from 2.8 seconds to 10+ seconds
- **Frequency**: Approximately 30-60% of server startups experience the long delay
- **Impact**: Poor user experience, unpredictable performance

### Detailed Analysis

The issue manifests as follows:
1. Client connects to FileLPXServer successfully (< 10ms)
2. Server detects client and initiates video processing 
3. **BUG**: Long delay (5-10 seconds) occurs before first frame data is sent
4. Subsequent frames process normally (~150ms LPX processing time)

### Investigation Results

Through extensive logging and testing, we identified:

**FIXED**: FPS Control Bug
- The FPS timing loop was calculating elapsed time from thread start instead of frame processing start
- Caused incorrect sleep delays on first frame
- **Status**: Fixed in commit 9f084a3

**REMAINING**: Synchronization Bug
- After fixing the FPS issue, inconsistent 5-10 second delays still occur
- The delay appears to be in the synchronization between:
  - Video thread waiting for client connections
  - Client connection being established
  - First frame processing initiation

### Evidence

Sample timing logs showing the problem:
```
[ 2078.3ms] CLIENT: Connected successfully
[ 6406.4ms] CLIENT: First frame data received!  # <-- 4+ second gap
```

The gap between client connection and first frame varies dramatically between runs.

## Technical Details

### Code Location
- **File**: `src/lpx_file_server.cpp`
- **Method**: `FileLPXServer::videoThread()`
- **Lines**: Approximately 307-450

### Suspected Root Causes
1. **Race condition** between video thread client detection and actual frame processing
2. **Thread synchronization issue** with the restart video flag mechanism
3. **Mutex contention** between video thread and accept thread
4. **OpenCV VideoCapture timing** inconsistencies on first frame read

### Current Workarounds
None available - the delay is unpredictable and cannot be mitigated at the application level.

## Reproduction Steps

1. Start FileLPXServer with any video file
2. Connect a client immediately after server starts
3. Measure time from client connection to first frame received
4. Repeat multiple times - inconsistent delays will be observed

Test script available: `examples/debug_slow_startup.py`

## Attempted Fixes

1. ✅ **Fixed FPS timing bug** - Prevented incorrect sleep calculations
2. ❌ **Threading analysis** - Examined mutex usage, no obvious deadlocks found
3. ❌ **Video capture optimization** - Attempted OpenCV buffer size adjustments
4. ❌ **Synchronization improvements** - Modified restart flag mechanism

## Impact Assessment

- **Severity**: High - Affects user experience significantly
- **Frequency**: Intermittent but common (30-60% of startups)
- **Workaround**: None available
- **Business Impact**: Poor first impression for end users

## Requested Action

This bug requires investigation by a developer with deep expertise in:
- Multi-threaded C++ applications
- OpenCV VideoCapture internals
- Network socket programming
- Real-time video processing pipelines

The issue appears to be a complex race condition or synchronization problem that requires careful analysis of the thread interactions and timing dependencies.

## Additional Information

- All diagnostic scripts and test results are available in the `examples/` directory
- The partial fix for FPS timing is in commit 9f084a3
- Complete timing logs and analysis available upon request

---
**Priority**: High  
**Labels**: bug, performance, threading, video-processing  
**Assignee**: TBD  
**Milestone**: Next Release
