#!/usr/bin/env python3
# lpx_stream_demo.py - Demonstrates streaming video with LPXImage
import numpy as np
import cv2
import lpximage
import time
import threading
import signal
import sys
import os  # Add os module for os._exit

# Global server variable
server = None

# Define signal handler function
def signal_handler(sig, frame):
    print("\nCtrl+C pressed, stopping streams and exiting...")
    global server
    if server is not None:
        try:
            server.stop()
            print("Server stopped")
        except Exception as e:
            print(f"Error stopping server: {e}")
    
    # Force destroy all OpenCV windows
    cv2.destroyAllWindows()
    
    # Force exit the program
    print("Exiting...")
    # Use a more forceful approach for exiting
    os._exit(0)  # Force immediate exit without cleanup

def main():
    # Initialize variables
    global server
    server = None
    
    # Register the signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    print("Starting LPX streaming demo...")
    print("Press Ctrl+C to exit")
    
    # Function to run the webcam server in a background thread
    def run_webcam_server():
        global server
        try:
            # Create server with scan tables
            server = lpximage.WebcamLPXServer("../ScanTables63")
            
            # Start server with webcam (use default camera, 640x480 resolution)
            # Adjust camera ID and resolution as needed
            if not server.start(0, 640, 480):
                print("Failed to start webcam server")
                return
            
            print("Webcam server started")
            
            # Keep server running until program exits
            while True:
                # Report client count periodically
                print(f"Active clients: {server.getClientCount()}")
                time.sleep(5)
        except Exception as e:
            print(f"Server error: {e}")
            if server is not None:
                try:
                    server.stop()
                except:
                    pass
    
    # Function to run the debug client that displays log-polar processed stream
    def run_debug_client():
        # Create client with scan tables
        client = lpximage.LPXDebugClient("../ScanTables63")
        
        # Configure display window
        client.setWindowTitle("LPX Stream (Python)")
        client.setWindowSize(800, 600)
        client.setScale(1.0)
        
        # Initialize window (must be done on main thread)
        client.initializeWindow()
        
        # Connect to server
        print("Connecting to LPX server at 127.0.0.1:5050...")
        try:
            if not client.connect("127.0.0.1"):
                print("Failed to connect to server")
                return
        except Exception as e:
            print(f"Error connecting to server: {e}")
            return
        
        print("Connected to LPX server, receiving video stream...")
        print("Press 'q' in the video window or Ctrl+C in the terminal to exit")
        
        # Process events on main thread
        try:
            while client.isRunning():
                # Process UI events and update display
                if not client.processEvents():
                    break
                
                # Check for 'q' key press in the window with a short timeout
                # This timeout is crucial for allowing the signal handler to work
                # Use a short timeout (10ms) to balance responsiveness with signal handling
                key = cv2.waitKey(10) & 0xFF  # 10ms timeout
                if key == ord('q'):
                    print("'q' key pressed, exiting...")
                    break
                
                # No need for additional sleep as waitKey already provides it
        except KeyboardInterrupt:
            print("\nKeyboard interrupt detected in client loop")
        except Exception as e:
            print(f"Error in client: {e}")
        finally:
            # Clean up
            client.disconnect()
            print("Disconnected from server")
    
    # Start server in a background thread
    server_thread = threading.Thread(target=run_webcam_server)
    server_thread.daemon = True  # Thread will exit when main program exits
    server_thread.start()
    
    # Give server time to start
    time.sleep(2)
    
    # Run client on main thread
    run_debug_client()
    
    # If we get here, client has exited, so stop the server
    if server is not None:
        server.stop()
        print("Server stopped")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nMain thread caught KeyboardInterrupt, cleaning up...")
        if 'server' in globals() and server is not None:
            server.stop()
            print("Server stopped")
        cv2.destroyAllWindows()
        sys.exit(0)
