#!/usr/bin/env python3
# lpx_file_server.py - Stream video from a file with LPXImage processing
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

# Global variables
server = None

# Define signal handler for Ctrl+C
def signal_handler(sig, frame):
    print("\nCtrl+C pressed, stopping server and exiting...")
    print("Forcing immediate exit...")
    os._exit(0)  # Force immediate exit without cleanup

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPXImage File Server - Stream video files with log-polar processing')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--file', required=True, help='Path to video file')
    parser.add_argument('--width', type=int, default=1920, help='Output video width')
    parser.add_argument('--height', type=int, default=1080, help='Output video height')
    parser.add_argument('--port', type=int, default=5050, help='Server port')
    parser.add_argument('--loop', action='store_true', help='Loop the video when it ends')
    parser.add_argument('--fps', type=float, default=0, help='Override FPS (0 = use video\'s FPS)')
    parser.add_argument('--x_offset', type=int, default=0, help='X offset from center (positive = right)')
    parser.add_argument('--y_offset', type=int, default=0, help='Y offset from center (positive = down)')
    args = parser.parse_args()
    
    # Register signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Print startup info
    print(f"LPXImage File Server - Streaming video with log-polar processing")
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
        server = lpximage.FileLPXServer(args.tables, args.port)
        
        # Configure the server
        if args.fps > 0:
            server.setFPS(args.fps)
        server.setLooping(args.loop)
        server.setCenterOffset(args.x_offset, args.y_offset)
        
        # Start the server with the video file
        if not server.start(args.file, args.width, args.height):
            print("Failed to start LPX file server. Check the video file path.")
            return
        
        print(f"Server started and streaming video on port {args.port}")
        print("Waiting for clients to connect...")
        
        # Main server loop - just report status periodically
        while True:
            # Report client count
            client_count = server.getClientCount()
            if client_count > 0:
                print(f"Active clients: {client_count}")
            
            # Add a delay to allow signal handling
            time.sleep(5)
            
    except Exception as e:
        print(f"Server error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # Clean up if we exit the loop
        if server:
            server.stop()
            print("Server stopped")

if __name__ == "__main__":
    main()
