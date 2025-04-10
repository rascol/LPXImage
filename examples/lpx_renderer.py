#!/usr/bin/env python3
# lpx_renderer.py - Receives LPXImage frames from server, renders them, and displays
import numpy as np
import cv2
import lpximage
import time
import signal
import sys
import os

def main():
    # Parse command-line arguments (in a real application)
    scan_tables_path = "../ScanTables63"  # Default path
    server_host = "127.0.0.1"  # Default to localhost - change to server IP
    window_width = 800  # Default display width
    window_height = 600  # Default display height
    scale = 1.0  # Default scaling factor
    
    # Print startup info
    print(f"LPXImage Renderer - Receiving and displaying LPX video")
    print(f"Connecting to: {server_host}")
    print(f"Window size: {window_width}x{window_height}")
    print(f"Scan Tables: {scan_tables_path}")
    print("Press 'q' in the window or Ctrl+C in terminal to exit")
    
    # Create the LPX debug client
    client = lpximage.LPXDebugClient(scan_tables_path)
    
    # Configure the display window
    client.setWindowTitle("LPX Remote Renderer")
    client.setWindowSize(window_width, window_height)
    client.setScale(scale)
    
    # Initialize the window (must be on main thread)
    client.initializeWindow()
    
    # Define a clean exit function
    def clean_exit():
        if client.isConnected():
            client.disconnect()
            print("Disconnected from server")
        cv2.destroyAllWindows()
        print("Renderer exiting...")
        sys.exit(0)
    
    # Set up signal handler for Ctrl+C
    def signal_handler(sig, frame):
        print("\nCtrl+C pressed, exiting...")
        clean_exit()
    
    signal.signal(signal.SIGINT, signal_handler)
    
    # Connect to the server
    print(f"Connecting to LPX server at {server_host}...")
    try:
        if not client.connect(server_host):
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
            
            # Check for 'q' key press to exit
            key = cv2.waitKey(10) & 0xFF
            if key == ord('q'):
                print("'q' key pressed, exiting...")
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
        # Clean up resources
        clean_exit()

if __name__ == "__main__":
    main()
