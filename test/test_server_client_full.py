#!/usr/bin/env python3
"""
Full server/client example from README Usage from Python section
"""

import lpximage
import threading
import time
import cv2
import numpy as np

# Function to run the webcam server in a background thread
def run_server():
    # Create the webcam server
    server = lpximage.WebcamLPXServer("../ScanTables63")
    
    # Start the server (camera ID, width, height)
    if not server.start(0, 640, 480):
        print("Failed to start server")
        return
    
    print("Server started. Processing video stream...")
    
    # Keep the server running for 60 seconds
    time.sleep(60)
    
    # Stop the server
    server.stop()
    print("Server stopped")

# Function to run the client that displays the log-polar processed stream
def run_client():
    # Create the client
    client = lpximage.LPXDebugClient("../ScanTables63")
    
    # Configure the client
    client.setWindowTitle("Log-Polar Video Stream")
    client.setWindowSize(800, 600)
    client.setScale(1.0)
    
    # Initialize the display window (must be on main thread for macOS)
    client.initializeWindow()
    
    # Connect to the server
    if not client.connect("127.0.0.1"):
        print("Failed to connect to server")
        return
    
    print("Connected to server, receiving video stream...")
    
    # Process events on main thread
    while client.isRunning():
        # Process UI events and update display
        if not client.processEvents():
            break
        time.sleep(0.01)
    
    # Disconnect when done
    client.disconnect()

if __name__ == "__main__":
    # Start the server in a background thread
    server_thread = threading.Thread(target=run_server)
    server_thread.daemon = True
    server_thread.start()
    
    # Give the server a moment to start
    time.sleep(1)
    
    # Run the client on the main thread
    run_client()
