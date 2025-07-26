#!/usr/bin/env python3
# lpx_renderer_simple.py - Simple enhanced renderer with timing info but no threading
import numpy as np
import time
import signal
import sys
import os
import argparse
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

def log_with_timestamp(message):
    """Log message with precise timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] {message}")

def main():
    startup_time = time.time()
    
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage Renderer (Simple Enhanced) - Clean version with timing')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--host', default='127.0.0.1', help='Server hostname or IP address')
    parser.add_argument('--port', type=int, default=5050, help='Server port')
    parser.add_argument('--width', type=int, default=800, help='Window width')
    parser.add_argument('--height', type=int, default=600, help='Window height')
    parser.add_argument('--scale', type=float, default=1.0, help='Rendering scale factor')
    args = parser.parse_args()
    
    # Print startup info with version
    version, build, throttle = get_version_info()
    print("=" * 60)
    print(f"LPXImage Renderer (Simple Enhanced) v{version} (Build {build})")
    print(f"Key Throttle: {throttle}ms")
    print("=" * 60)
    print(f"Connecting to: {args.host}:{args.port}")
    print(f"Window size: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print("IMPORTANT: Click on the display window and use WASD keys to move")
    print("Press Ctrl+C in terminal to exit")
    
    # Create the LPX debug client
    log_with_timestamp("Creating LPXDebugClient...")
    client_create_start = time.time()
    client = lpximage.LPXDebugClient(args.tables)
    client_create_end = time.time()
    create_time = (client_create_end - client_create_start) * 1000
    log_with_timestamp(f"LPXDebugClient created in {create_time:.1f}ms")
    
    # Configure the display window
    log_with_timestamp("Configuring display window...")
    config_start = time.time()
    client.setWindowTitle("LPX Remote Renderer (Simple)")
    client.setWindowSize(args.width, args.height)
    client.setScale(args.scale)
    config_end = time.time()
    config_time = (config_end - config_start) * 1000
    log_with_timestamp(f"Window configured in {config_time:.1f}ms")
    
    # Initialize the window (must be on main thread)
    log_with_timestamp("Initializing window...")
    init_start = time.time()
    client.initializeWindow()
    init_end = time.time()
    init_time = (init_end - init_start) * 1000
    log_with_timestamp(f"Window initialized in {init_time:.1f}ms")
    
    # Calculate total client setup time
    setup_time = (init_end - client_create_start) * 1000
    log_with_timestamp(f"Total client setup time: {setup_time:.1f}ms")
    
    # Keyboard input is now handled directly by the LPXDebugClient window
    print("\n=== KEYBOARD CONTROLS ===")
    print("Click on the main LPX display window and use WASD keys to move")
    print("W/S: Move up/down | A/D: Move left/right | Q/ESC: Quit")
    print("=========================")
    
    # Define a clean exit function
    def clean_exit():
        try:
            log_with_timestamp("Cleaning up...")
            client.disconnect()
            log_with_timestamp("Disconnected from server")
        except Exception as e:
            log_with_timestamp(f"Error disconnecting: {e}")
        log_with_timestamp("Renderer exiting...")
        sys.exit(0)
    
    # Set up signal handler for Ctrl+C
    def signal_handler(sig, frame):
        log_with_timestamp("Ctrl+C pressed, cleaning up...")
        clean_exit()
    
    # Register signal handlers for various signals
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Connect to the server
    server_address = f"{args.host}:{args.port}"
    log_with_timestamp(f"Connecting to LPX server at {server_address}")
    
    try:
        # Attempt connection
        connection_start = time.time()
        connected = client.connect(server_address)
        connection_end = time.time()
        connection_time = (connection_end - connection_start) * 1000
        
        if not connected:
            log_with_timestamp(f"✗ Failed to connect after {connection_time:.1f}ms")
            log_with_timestamp("Check if server is running and accessible")
            return
        
        log_with_timestamp(f"✓ Connected to LPX server in {connection_time:.1f}ms")
        
        # Calculate time from startup to connection
        startup_to_connection = (connection_end - startup_time) * 1000
        log_with_timestamp(f"Time from startup to connection: {startup_to_connection:.1f}ms")
        
        log_with_timestamp("Receiving video stream...")
        log_with_timestamp("Entering main display loop...")
        
        # Display frames in a loop - SINGLE THREAD ONLY
        frame_count = 0
        start_time = time.time()
        last_fps_time = start_time
        fps_frame_count = 0
        first_frame_received = False
        
        while client.isRunning():
            # Process events and update display - NO THREADING
            try:
                process_result = client.processEvents()
                
                if not process_result:
                    log_with_timestamp("processEvents() returned False, breaking loop")
                    break
                
                # Track first frame
                if not first_frame_received:
                    first_frame_time = time.time()
                    time_to_first_frame = (first_frame_time - connection_end) * 1000
                    log_with_timestamp(f"✓ First frame received after {time_to_first_frame:.1f}ms")
                    first_frame_received = True
                
            except Exception as e:
                log_with_timestamp(f"Error in processEvents(): {e}")
                # Brief pause and continue
                time.sleep(0.01)
                continue
            
            # Calculate and display FPS every second
            frame_count += 1
            fps_frame_count += 1
            current_time = time.time()
            
            if current_time - last_fps_time >= 1.0:
                fps = fps_frame_count / (current_time - last_fps_time)
                log_with_timestamp(f"Display FPS: {fps:.2f} (frame {frame_count})")
                fps_frame_count = 0
                last_fps_time = current_time
            
    except KeyboardInterrupt:
        log_with_timestamp("Keyboard interrupt detected")
        clean_exit()
    except Exception as e:
        log_with_timestamp(f"Error: {e}")
        import traceback
        traceback.print_exc()
        clean_exit()
    
    # Normal exit path
    clean_exit()

if __name__ == "__main__":
    main()
