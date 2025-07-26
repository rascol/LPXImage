#!/usr/bin/env python3
# lpx_file_server.py - Stream video from a file with LPXImage processing
import time
import signal
import sys
import os
import argparse

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

# Define signal handler for Ctrl+C
def signal_handler(sig, frame):
    global should_exit, server
    print("\nCtrl+C pressed, stopping server gracefully...")
    should_exit = True
    # Explicitly stop the server in the signal handler to avoid destructor issues
    if server:
        try:
            print("Signal handler: Stopping server immediately...")
            server.stop()
            print("Signal handler: Server stopped successfully")
        except Exception as e:
            print(f"Signal handler: Error stopping server: {e}")

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage File Server - Stream video files with log-polar processing')
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
    
    # Register signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Print startup info with version
    version, build, throttle = get_version_info()
    print("=" * 60)
    print(f"LPX File Server v{version} (Build {build})")
    print(f"Key Throttle: {throttle}ms")
    print("=" * 60)
    print(f"Video file: {args.file}")
    print(f"Output resolution: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print(f"Port: {args.port}")
    print(f"Loop video: {'Yes' if args.loop else 'No'}")
    print(f"Log-polar center: ({args.width/2 + args.x_offset}, {args.height/2 + args.y_offset})")
    print("Press Ctrl+C to exit")
    
    # Create and start the LPX file server
    global server
    try:
        # Initialize the server with scan tables
        print("DEBUG: About to create FileLPXServer...")
        create_start = time.time()
        server = lpximage.FileLPXServer(args.tables, args.port)
        create_time = time.time() - create_start
        print(f"DEBUG: FileLPXServer created successfully in {create_time*1000:.1f}ms")
        
        # Configure the server
        print("DEBUG: Configuring server...")
        config_start = time.time()
        if args.fps > 0:
            print(f"DEBUG: Setting FPS to {args.fps}")
            server.setFPS(args.fps)
        print(f"DEBUG: Setting looping to {args.loop}")
        server.setLooping(args.loop)
        print(f"DEBUG: Setting center offset to ({args.x_offset}, {args.y_offset})")
        server.setCenterOffset(args.x_offset, args.y_offset)
        config_time = time.time() - config_start
        print(f"DEBUG: Server configuration completed in {config_time*1000:.1f}ms")
        
        # Start the server with the video file
        print(f"DEBUG: About to start server with file: {args.file}")
        print(f"DEBUG: Using dimensions: {args.width}x{args.height}")
        start_begin = time.time()
        start_result = server.start(args.file, args.width, args.height)
        start_time = time.time() - start_begin
        print(f"DEBUG: Server start result: {start_result} (took {start_time*1000:.1f}ms)")
        if not start_result:
            print("Failed to start LPX file server. Check the video file path.")
            return
        
        print(f"Server started and streaming video on port {args.port}")
        print("Server started successfully")  # Ready signal for test script
        print("Waiting for clients to connect...")
        
        # Main server loop - report status periodically with shorter delays
        print("DEBUG: Entering main server loop...")
        loop_count = 0
        last_report_time = time.time()
        
        while not should_exit:
            loop_count += 1
            current_time = time.time()
            
            # Report client count every 10 seconds instead of every iteration
            if current_time - last_report_time >= 10.0:
                client_count = server.getClientCount()
                if client_count > 0:
                    print(f"Active clients: {client_count}")
                else:
                    print("Waiting for clients...")
                last_report_time = current_time
            
            # Much shorter sleep for better responsiveness
            time.sleep(0.1)  # 100ms instead of 5 seconds
            
    except Exception as e:
        print(f"Server error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # Clean up when exiting
        print("Cleaning up server...")
        if server:
            try:
                server.stop()
                print("Server stopped gracefully")
            except Exception as e:
                print(f"Error during server cleanup: {e}")

if __name__ == "__main__":
    main()
