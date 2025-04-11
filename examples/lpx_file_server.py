#!/usr/bin/env python3
# lpx_file_server.py - Streams video from a file with LPXImage processing
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
running = True

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
    print("Press Ctrl+C to exit")
    
    # Create and start the server thread
    server_thread = threading.Thread(target=run_server, args=(args,))
    server_thread.daemon = True
    server_thread.start()
    
    # Keep the main thread alive
    try:
        while running:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nKeyboard interrupt detected")
    finally:
        print("Exiting program forcefully...")
        os._exit(0)  # Force immediate exit

def run_server(args):
    global server, running
    
    # Initialize the LPX server
    server = lpximage.LPXServer(args.tables)
    if not server:
        print("Failed to create LPX server")
        running = False
        return
    
    # Open the video file
    cap = cv2.VideoCapture(args.file)
    if not cap.isOpened():
        print(f"Error: Could not open video file {args.file}")
        running = False
        return
    
    # Get video file properties
    video_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    video_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    video_fps = cap.get(cv2.CAP_PROP_FPS)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    
    # Use provided FPS or the video's native FPS
    fps = args.fps if args.fps > 0 else video_fps
    frame_delay = 1.0 / fps
    
    print(f"Video properties: {video_width}x{video_height}, {video_fps} FPS, {total_frames} frames")
    print(f"Streaming at: {fps} FPS (frame delay: {frame_delay:.3f}s)")
    print(f"Log-polar center: ({args.width/2 + args.x_offset}, {args.height/2 + args.y_offset})")
    
    # Start the server
    if not server.start(args.port):
        print("Failed to start LPX server")
        running = False
        return
    
    print(f"Server started and listening on port {args.port}")
    print("Waiting for clients to connect...")
    
    # Process the video file in a loop
    try:
        frame_count = 0
        last_time = time.time()
        
        while running:
            # Read a frame from the video
            ret, frame = cap.read()
            
            # If we reached the end of the video
            if not ret:
                if args.loop:
                    print("Reached end of video, looping back to start")
                    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                    continue
                else:
                    print("Reached end of video, stopping server")
                    break
            
            # Convert the frame to RGB (LPXImage expects RGB format)
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            
            # Resize if necessary
            if video_width != args.width or video_height != args.height:
                frame_rgb = cv2.resize(frame_rgb, (args.width, args.height))
            
            # Process and send the frame
            # Calculate center with offsets
            center_x = args.width / 2 + args.x_offset
            center_y = args.height / 2 + args.y_offset
            
            # Scan the image to create an LPXImage
            lpx_image = lpximage.scanImage(frame_rgb, center_x, center_y)
            
            # Send the LPX image to connected clients
            server.sendLPXImage(lpx_image)
            
            # Calculate and control FPS
            frame_count += 1
            elapsed = time.time() - last_time
            
            # Report status every 100 frames
            if frame_count % 100 == 0:
                clients = server.getClientCount()
                current_fps = 100 / elapsed if elapsed > 0 else 0
                print(f"Frame: {frame_count}/{total_frames}, FPS: {current_fps:.2f}, Clients: {clients}")
                last_time = time.time()
            
            # Control the frame rate
            wait_time_ms = max(1, int(frame_delay * 1000))
            cv2.waitKey(wait_time_ms)  # Use OpenCV's wait instead of sleep for better system compatibility
            
    except Exception as e:
        print(f"Error processing video: {e}")
    finally:
        # Clean up resources
        cap.release()
        if server:
            server.stop()
            print("Server stopped")
        running = False

if __name__ == "__main__":
    main()
