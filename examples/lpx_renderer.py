#!/usr/bin/env python3
# lpx_renderer.py - Receives LPXImage frames from server, renders them, and displays
import numpy as np
import cv2

try:
    import lpximage
except ModuleNotFoundError:
    print("ERROR: lpximage module not found!")
    print("Please ensure LPXImage is properly installed on this machine.")
    print("Refer to INSTALL_PYTHON.md in the LPXImage directory for installation instructions.")
    print("Typically you would need to:")
    print("  1. Build the C++ library and Python bindings")
    print("  2. Install the Python module with pip or add it to your PYTHONPATH")
    import sys
    sys.exit(1)
import time
import signal
import sys
import os
import argparse

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage Renderer - Receive and display LPXImage video')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--host', default='127.0.0.1', help='Server hostname or IP address')
    parser.add_argument('--width', type=int, default=800, help='Window width')
    parser.add_argument('--height', type=int, default=600, help='Window height')
    parser.add_argument('--scale', type=float, default=1.0, help='Rendering scale factor')
    args = parser.parse_args()
    
    # Print startup info
    print(f"LPXImage Renderer - Receiving and displaying LPX video")
    print(f"Connecting to: {args.host}")
    print(f"Window size: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print("Press Ctrl+C in terminal to exit")
    
    # Create the LPX debug client
    client = lpximage.LPXDebugClient(args.tables)
    
    # Configure the display window
    client.setWindowTitle("LPX Remote Renderer")
    client.setWindowSize(args.width, args.height)
    client.setScale(args.scale)
    
    # Initialize the window (must be on main thread)
    client.initializeWindow()
    
    # Define a clean exit function
    def clean_exit():
        # Just call disconnect - the isConnected() method doesn't exist
        try:
            client.disconnect()
            print("Disconnected from server")
        except Exception as e:
            print(f"Error disconnecting: {e}")
        cv2.destroyAllWindows()
        print("Renderer exiting...")
        sys.exit(0)
    
    # Set up signal handler for Ctrl+C
    def signal_handler(sig, frame):
        print("\nCtrl+C pressed, exiting...")
        print("Forcing immediate exit...")
        # Skip any cleanup - just terminate immediately
        os._exit(0)  # Immediately terminates the process without any cleanup
    
    # Register signal handlers for various signals
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Connect to the server
    print(f"Connecting to LPX server at {args.host}...")
    try:
        if not client.connect(args.host):
            print("Failed to connect to server. Check if server is running.")
            return
        
        print("Connected to LPX server, receiving video stream...")
        
        # Display frames in a loop
        frame_count = 0
        start_time = time.time()
        
        while client.isRunning():
            # Process events and update display
            if not client.processEvents():
                break
            
            # Calculate and display FPS every second
            frame_count += 1
            elapsed = time.time() - start_time
            if elapsed >= 1.0:
                fps = frame_count / elapsed
                print(f"FPS: {fps:.2f}")
                frame_count = 0
                start_time = time.time()
            
    except KeyboardInterrupt:
        print("\nKeyboard interrupt detected")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Skip all cleanup and just exit forcefully
        print("Exiting program forcefully...")
        os._exit(0)  # Force immediate exit without any cleanup

if __name__ == "__main__":
    main()
