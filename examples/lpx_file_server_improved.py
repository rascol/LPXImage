#!/usr/bin/env python3
# lpx_file_server_improved.py - Enhanced file server with better timing and diagnostics
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
last_client_activity = None

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

def monitor_server_health(server):
    """Monitor server health and client connections in a separate thread"""
    global should_exit, last_client_activity
    
    last_client_count = 0
    no_client_warnings = 0
    
    while not should_exit:
        try:
            current_client_count = server.getClientCount()
            
            # Detect client count changes
            if current_client_count != last_client_count:
                if current_client_count > last_client_count:
                    log_with_timestamp(f"Client connected: {current_client_count} total clients")
                    last_client_activity = time.time()
                elif current_client_count < last_client_count:
                    log_with_timestamp(f"Client disconnected: {current_client_count} total clients")
                    last_client_activity = time.time()
                
                last_client_count = current_client_count
                no_client_warnings = 0  # Reset warning counter
            
            # Check for extended periods with no clients
            elif current_client_count == 0:
                no_client_warnings += 1
                if no_client_warnings == 600:  # After 60 seconds (600 * 0.1s)
                    log_with_timestamp("WARNING: No clients connected for 60 seconds")
                elif no_client_warnings == 1800:  # After 3 minutes
                    log_with_timestamp("WARNING: No clients connected for 3 minutes")
                elif no_client_warnings % 3000 == 0:  # Every 5 minutes after that
                    minutes = (no_client_warnings * 0.1) / 60
                    log_with_timestamp(f"WARNING: No clients connected for {minutes:.1f} minutes")
            
            time.sleep(0.1)  # Check every 100ms
            
        except Exception as e:
            log_with_timestamp(f"Health monitor error: {e}")
            break

def test_server_responsiveness(server):
    """Test if server is responsive"""
    try:
        client_count = server.getClientCount()
        log_with_timestamp(f"Server responsiveness test: {client_count} clients")
        return True
    except Exception as e:
        log_with_timestamp(f"Server responsiveness test failed: {e}")
        return False

def main():
    global server, startup_time, last_client_activity
    
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage File Server - Enhanced with better timing diagnostics')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--file', required=True, help='Path to video file')
    parser.add_argument('--width', type=int, default=1920, help='Output video width')
    parser.add_argument('--height', type=int, default=1080, help='Output video height')
    parser.add_argument('--port', type=int, default=5050, help='Server port (LPXDebugClient connects to 5050)')
    parser.add_argument('--loop', action='store_true', help='Loop the video when it ends')
    parser.add_argument('--fps', type=float, default=0, help='Override FPS (0 = use video\'s FPS)')
    parser.add_argument('--x_offset', type=int, default=0, help='X offset from center (positive = right)')
    parser.add_argument('--y_offset', type=int, default=0, help='Y offset from center (positive = down)')
    parser.add_argument('--timeout', type=int, default=300, help='Exit after N seconds if no clients connect')
    args = parser.parse_args()
    
    startup_time = time.time()
    
    # Register signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Print startup info with version
    version, build, throttle = get_version_info()
    print("=" * 60)
    print(f"LPX File Server (Enhanced) v{version} (Build {build})")
    print(f"Key Throttle: {throttle}ms")
    print("=" * 60)
    print(f"Video file: {args.file}")
    print(f"Output resolution: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print(f"Port: {args.port}")
    print(f"Loop video: {'Yes' if args.loop else 'No'}")
    print(f"Log-polar center: ({args.width/2 + args.x_offset}, {args.height/2 + args.y_offset})")
    print(f"Timeout: {args.timeout} seconds")
    print("Press Ctrl+C to exit")
    
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
        
        # Test immediate responsiveness
        if not test_server_responsiveness(server):
            log_with_timestamp("WARNING: Server not immediately responsive")
        
        log_with_timestamp(f"Server started successfully on port {args.port}")
        log_with_timestamp("Waiting for clients to connect...")
        
        # Start health monitoring thread
        health_thread = threading.Thread(target=monitor_server_health, args=(server,))
        health_thread.daemon = True
        health_thread.start()
        
        # Main server loop with improved timing and diagnostics
        log_with_timestamp("Entering main server loop...")
        loop_count = 0
        last_status_time = time.time()
        startup_complete_time = time.time()
        
        # Calculate total startup time
        total_startup = (startup_complete_time - startup_time) * 1000
        log_with_timestamp(f"Total startup time: {total_startup:.1f}ms")
        
        while not should_exit:
            loop_count += 1
            current_time = time.time()
            
            # Periodic status reporting (every 30 seconds)
            if current_time - last_status_time >= 30.0:
                client_count = server.getClientCount()
                uptime = current_time - startup_time
                
                if client_count > 0:
                    log_with_timestamp(f"Status: {client_count} active clients, uptime: {uptime:.1f}s")
                else:
                    log_with_timestamp(f"Status: Waiting for clients, uptime: {uptime:.1f}s")
                
                last_status_time = current_time
            
            # Check timeout for no client connections
            if args.timeout > 0 and last_client_activity is None:
                if (current_time - startup_time) > args.timeout:
                    log_with_timestamp(f"Timeout: No clients connected after {args.timeout} seconds, exiting...")
                    break
            
            # Responsive sleep
            time.sleep(0.1)  # 100ms for good responsiveness
            
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
