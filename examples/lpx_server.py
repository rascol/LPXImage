#!/usr/bin/env python3
# lpx_server.py - Captures video, converts it to LPXImage format, and streams to clients
import numpy as np
import cv2
import lpximage
import time
import threading
import signal
import sys
import os

# Global variable for server
server = None

# Define signal handler
def signal_handler(sig, frame):
    print("\nCtrl+C pressed, stopping server and exiting...")
    global server
    if server is not None:
        try:
            server.stop()
            print("Server stopped")
        except Exception as e:
            print(f"Error stopping server: {e}")
    
    # Clean up OpenCV resources
    cv2.destroyAllWindows()
    
    # Exit the program
    print("Server exiting...")
    sys.exit(0)

def main():
    # Register signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    # Parse command-line arguments (in a real application)
    scan_tables_path = "../ScanTables63"  # Default path
    camera_id = 0  # Default camera
    width = 640  # Default width
    height = 480  # Default height
    port = 5050  # Default port
    
    # Print startup info
    print(f"LPXImage Server - Converting and streaming video")
    print(f"Camera ID: {camera_id}")
    print(f"Resolution: {width}x{height}")
    print(f"Scan Tables: {scan_tables_path}")
    print("Press Ctrl+C to exit")
    
    # Create and start the LPX server
    global server
    try:
        # Initialize the server with scan tables
        server = lpximage.WebcamLPXServer(scan_tables_path)
        
        # Start the server with the specified camera and resolution
        if not server.start(camera_id, width, height):
            print("Failed to start LPX server. Check camera connection.")
            return
        
        print(f"Server started and listening on port {port}")
        print("Waiting for clients to connect...")
        
        # Main server loop
        while True:
            # Report status periodically
            client_count = server.getClientCount()
            if client_count > 0:
                print(f"Active clients: {client_count}")
            
            # Add a short delay to allow signal handling
            time.sleep(1)
            
            # You could add more server management code here
            
    except Exception as e:
        print(f"Server error: {e}")
    finally:
        # Clean up if we exit the loop
        if server is not None:
            server.stop()
            print("Server stopped")

if __name__ == "__main__":
    main()
