#!/usr/bin/env python3
# lpx_file_server_fixed.py - Fixed version that avoids problematic getClientCount() calls
import time
import signal
import sys
import os
import argparse
import threading
from datetime import datetime

try:
    import lpximage
except ModuleNotFoundError:
    print("ERROR: lpximage module not found!")
    print("Please ensure LPXImage is properly installed on this machine.")
    print("Refer to INSTALL_PYTHON.md in the LPXImage directory for installation instructions.")
    sys.exit(1)

# Helper function to get version info with fallback
def get_version_info():
    try:
        version = lpximage.getVersionString()
        build = lpximage.getBuildNumber()
        throttle = lpximage.getKeyThrottleMs()
        return version, build, throttle
    except AttributeError:
        return "Unknown", "Unknown", "Unknown"

# Global variables
server = None
should_exit = False
startup_time = None

def log_with_timestamp(message):
    """Log message with precise timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] {message}")

# Define signal handler for Ctrl+C
def signal_handler(sig, frame):
    global should_exit, server
    log_with_timestamp("Ctrl+C pressed, stopping server gracefully...")
    should_exit = True
    # Explicitly stop the server in the signal handler to avoid destructor issues
    if server:
        try:
            log_with_timestamp("Signal handler: Stopping server immediately...")
            server.stop()
            log_with_timestamp("Signal handler: Server stopped successfully")
        except Exception as e:
            log_with_timestamp(f"Signal handler: Error stopping server: {e}")

def main():
    global server, startup_time
    
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage File Server (FIXED) - No problematic getClientCount() calls')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--file', required=True, help='Path to video file')
    parser.add_argument('--width', type=int, default=1920, help='Output video width')
    parser.add_argument('--height', type=int, default=1080, help='Output video height')
    parser.add_argument('--port', type=int, default=5050, help='Server port (LPXDebugClient connects to 5050)')
    parser.add_argument('--loop', action='store_true', help='Loop the video when it ends')
    parser.add_argument('--fps', type=float, default=0, help='Override FPS (0 = use video\'s FPS)')
    parser.add_argument('--x_offset', type=int, default=0, help='X offset from center (positive = right)')
    parser.add_argument('--y_offset', type=int, default=0, help='Y offset from center (positive = down)')
    args = parser.parse_args()
    
    startup_time = time.time()
    
    # Register signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Print startup info with version
    version, build, throttle = get_version_info()
    print("=" * 60)
    print(f"LPX File Server (FIXED) v{version} (Build {build})")
    print(f"Key Throttle: {throttle}ms")
    print("=" * 60)
    print(f"Video file: {args.file}")
    print(f"Output resolution: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print(f"Port: {args.port}")
    print(f"Loop video: {'Yes' if args.loop else 'No'}")
    print(f"Log-polar center: ({args.width/2 + args.x_offset}, {args.height/2 + args.y_offset})")
    print("Press Ctrl+C to exit")
    print("")
    print("IMPORTANT: This version avoids the problematic getClientCount() calls")
    print("that were causing 1-15 second delays in the original server!")
    
    # Verify video file exists
    if not os.path.exists(args.file):
        log_with_timestamp(f"ERROR: Video file not found: {args.file}")
        return
    
    # Create and start the LPX file server
    try:
        # Initialize the server with scan tables
        log_with_timestamp("Creating FileLPXServer...")
        create_start = time.time()
        server = lpximage.FileLPXServer(args.tables, args.port)
        create_end = time.time()
        log_with_timestamp(f"FileLPXServer created in {(create_end - create_start)*1000:.1f}ms")
        
        # Configure the server
        log_with_timestamp("Configuring server...")
        if args.fps > 0:
            log_with_timestamp(f"Setting FPS to {args.fps}")
            server.setFPS(args.fps)
        log_with_timestamp(f"Setting looping to {args.loop}")
        server.setLooping(args.loop)
        log_with_timestamp(f"Setting center offset to ({args.x_offset}, {args.y_offset})")
        server.setCenterOffset(args.x_offset, args.y_offset)
        
        # Start the server with the video file
        log_with_timestamp(f"Starting server with file: {args.file}")
        log_with_timestamp(f"Using dimensions: {args.width}x{args.height}")
        
        start_begin = time.time()
        start_result = server.start(args.file, args.width, args.height)
        start_end = time.time()
        
        log_with_timestamp(f"Server start completed in {(start_end - start_begin)*1000:.1f}ms")
        log_with_timestamp(f"Server start result: {start_result}")
        
        if not start_result:
            log_with_timestamp("FAILED to start LPX file server. Check the video file path.")
            return
        
        log_with_timestamp(f"✓ Server started successfully on port {args.port}")
        log_with_timestamp("✓ Server is ready for client connections")
        
        # Calculate total startup time
        startup_complete_time = time.time()
        total_startup = (startup_complete_time - startup_time) * 1000
        log_with_timestamp(f"✓ Total startup time: {total_startup:.1f}ms")
        
        log_with_timestamp("Server is running - clients should connect immediately")
        log_with_timestamp("No more delays from problematic getClientCount() calls!")
        
        # FIXED MAIN LOOP - NO getClientCount() calls!
        log_with_timestamp("Entering optimized main server loop...")
        loop_count = 0
        last_status_time = time.time()
        
        while not should_exit:
            loop_count += 1
            current_time = time.time()
            
            # Minimal status reporting without getClientCount()
            if current_time - last_status_time >= 60.0:  # Every minute instead of every 10 seconds
                uptime = current_time - startup_time
                log_with_timestamp(f"Server status: Running normally, uptime: {uptime:.1f}s")
                log_with_timestamp("  (Client count not checked to avoid delays)")
                last_status_time = current_time
            
            # Very responsive sleep - server handles clients asynchronously
            time.sleep(0.1)  # 100ms for responsiveness
            
    except KeyboardInterrupt:
        log_with_timestamp("Keyboard interrupt received")
    except Exception as e:
        log_with_timestamp(f"Server error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # Clean up when exiting
        log_with_timestamp("Cleaning up server...")
        if server:
            try:
                stop_start = time.time()
                server.stop()
                stop_end = time.time()
                log_with_timestamp(f"Server stopped gracefully in {(stop_end - stop_start)*1000:.1f}ms")
            except Exception as e:
                log_with_timestamp(f"Error during server cleanup: {e}")

if __name__ == "__main__":
    main()
