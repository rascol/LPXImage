#!/usr/bin/env python3
# lpx_renderer_improved.py - Enhanced renderer with connection timing diagnostics
import numpy as np
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
    print("Typically you would need to:")
    print("  1. Build the C++ library and Python bindings")
    print("  2. Install the Python module with pip or add it to your PYTHONPATH")
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

def test_connection_with_retries(client, host, max_retries=5, retry_delay=1.0):
    """Attempt connection with retries and detailed timing"""
    log_with_timestamp(f"Attempting connection to {host} (max {max_retries} retries)")
    
    for attempt in range(max_retries):
        log_with_timestamp(f"Connection attempt {attempt + 1}/{max_retries}")
        
        connect_start = time.time()
        connected = client.connect(host)
        connect_end = time.time()
        connect_time = (connect_end - connect_start) * 1000
        
        if connected:
            log_with_timestamp(f"✓ Connection successful on attempt {attempt + 1}")
            log_with_timestamp(f"Connection time: {connect_time:.1f}ms")
            return True
        else:
            log_with_timestamp(f"✗ Connection attempt {attempt + 1} failed ({connect_time:.1f}ms)")
            
            if attempt < max_retries - 1:
                log_with_timestamp(f"Waiting {retry_delay}s before retry...")
                time.sleep(retry_delay)
                retry_delay *= 1.5  # Exponential backoff
    
    return False

def monitor_data_flow(client):
    """Monitor data flow and timing in a separate thread"""
    frame_count = 0
    last_frame_time = time.time()
    start_time = time.time()
    
    while client.isRunning():
        try:
            current_time = time.time()
            
            # Try to process events
            if client.processEvents():
                frame_count += 1
                frame_time = current_time - last_frame_time
                
                # Log first frame and then every 100 frames
                if frame_count == 1:
                    time_to_first_frame = (current_time - start_time) * 1000
                    log_with_timestamp(f"✓ First frame received after {time_to_first_frame:.1f}ms")
                elif frame_count % 100 == 0:
                    elapsed = current_time - start_time
                    fps = frame_count / elapsed if elapsed > 0 else 0
                    log_with_timestamp(f"Frame {frame_count}: Average FPS: {fps:.2f}")
                
                last_frame_time = current_time
            
            time.sleep(0.01)  # 10ms polling interval
            
        except Exception as e:
            log_with_timestamp(f"Data flow monitoring error: {e}")
            break

def main():
    startup_time = time.time()
    
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage Renderer (Enhanced) - Receive and display LPXImage video with diagnostics')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--host', default='127.0.0.1', help='Server hostname or IP address')
    parser.add_argument('--port', type=int, default=5050, help='Server port')
    parser.add_argument('--width', type=int, default=800, help='Window width')
    parser.add_argument('--height', type=int, default=600, help='Window height')
    parser.add_argument('--scale', type=float, default=1.0, help='Rendering scale factor')
    parser.add_argument('--retries', type=int, default=5, help='Maximum connection retries')
    parser.add_argument('--retry-delay', type=float, default=1.0, help='Initial retry delay in seconds')
    parser.add_argument('--connect-timeout', type=int, default=30, help='Connection timeout in seconds')
    args = parser.parse_args()
    
    # Print startup info with version
    version, build, throttle = get_version_info()
    print("=" * 60)
    print(f"LPXImage Renderer (Enhanced) v{version} (Build {build})")
    print(f"Key Throttle: {throttle}ms")
    print("=" * 60)
    print(f"Connecting to: {args.host}:{args.port}")
    print(f"Window size: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print(f"Max retries: {args.retries}")
    print(f"Connect timeout: {args.connect_timeout}s")
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
    client.setWindowTitle("LPX Remote Renderer (Enhanced)")
    client.setWindowSize(args.width, args.height)
    client.setScale(args.scale)
    config_end = time.time()
    config_time = (config_end - config_start) * 1000
    log_with_timestamp(f"Window configured in {config_time:.1f}ms - {args.width}x{args.height}, scale={args.scale}")
    
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
    
    # Connect to the server with retries and timing
    server_address = f"{args.host}:{args.port}"
    log_with_timestamp(f"Connecting to LPX server at {server_address}")
    
    try:
        # Attempt connection with retries
        connection_start = time.time()
        connected = test_connection_with_retries(client, server_address, args.retries, args.retry_delay)
        connection_end = time.time()
        
        if not connected:
            total_attempt_time = (connection_end - connection_start) * 1000
            log_with_timestamp(f"✗ Failed to connect after {args.retries} attempts ({total_attempt_time:.1f}ms total)")
            log_with_timestamp("Check if server is running and accessible")
            return
        
        total_connection_time = (connection_end - connection_start) * 1000
        log_with_timestamp(f"✓ Connected to LPX server")
        log_with_timestamp(f"Total connection process time: {total_connection_time:.1f}ms")
        
        # Calculate time from startup to first connection
        startup_to_connection = (connection_end - startup_time) * 1000
        log_with_timestamp(f"Time from startup to connection: {startup_to_connection:.1f}ms")
        # Data flow monitoring removed to prevent concurrent processEvents() calls
        # This prevents the "Unknown error in processEvents" issue
        
        log_with_timestamp("Receiving video stream...")
        
        # Display frames in a loop
        frame_count = 0
        start_time = time.time()
        last_fps_time = start_time
        fps_frame_count = 0
        
        log_with_timestamp("Entering main display loop...")
        
        loop_count = 0
        while client.isRunning():
            loop_count += 1
            
            # Process events and update display with error handling
            try:
                process_result = client.processEvents()
                
                if not process_result:
                    log_with_timestamp("processEvents() returned False, breaking loop")
                    break
            except Exception as e:
                log_with_timestamp(f"Error in processEvents(): {e}")
                # Continue rather than break to see if it recovers
                time.sleep(0.01)  # Brief pause before retry
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
