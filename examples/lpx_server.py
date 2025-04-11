#!/usr/bin/env python3
# lpx_server.py - Captures video, converts it to LPXImage format, and streams to clients
import numpy as np
import cv2
import time
import threading
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
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage Server - Stream video in LPXImage format')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--camera', type=int, default=0, help='Camera ID')
    parser.add_argument('--width', type=int, default=1920, help='Video width')
    parser.add_argument('--height', type=int, default=1080, help='Video height')
    parser.add_argument('--port', type=int, default=5050, help='Server port')
    parser.add_argument('--x_offset', type=int, default=0, help='X offset from center (positive = right)')
    parser.add_argument('--y_offset', type=int, default=0, help='Y offset from center (positive = down)')
    args = parser.parse_args()
    
    # Register signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    # Print startup info
    print(f"LPXImage Server - Converting and streaming video")
    print(f"Camera ID: {args.camera}")
    print(f"Resolution: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print(f"Port: {args.port}")
    print("Press Ctrl+C to exit")
    
    # Create and start the LPX server
    global server
    try:
        # Initialize the server with scan tables
        server = lpximage.WebcamLPXServer(args.tables)
        
        # Start the server with the specified camera and resolution
        if not server.start(args.camera, args.width, args.height):
            print("Failed to start LPX server. Check camera connection.")
            return
        
        print(f"Server started and listening on port {args.port}")
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
