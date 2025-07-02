#!/usr/bin/env python3
# test_wasd.py - Test WASD movement functionality with file server
import lpximage
import threading
import time
import signal
import sys
import os

def test_file_server():
    """Test the file server with WASD movement controls"""
    print("Testing WASD movement with FileLPXServer...")
    
    # Create and start file server
    try:
        server = lpximage.FileLPXServer("../ScanTables63", 5051)
        
        # Start server with a test video
        video_file = "../2342260-hd_1920_1080_30fps.mp4"
        if not os.path.exists(video_file):
            print(f"Test video not found: {video_file}")
            print("Please ensure the test video file exists or modify the path")
            return False
            
        if not server.start(video_file, 800, 600):
            print("Failed to start file server")
            return False
            
        print("File server started on port 5051")
        print("Server should now be accepting movement commands from clients")
        
        # Let server run for a bit
        time.sleep(5)
        print(f"Active clients: {server.getClientCount()}")
        
        # Stop server
        server.stop()
        print("File server stopped")
        return True
        
    except Exception as e:
        print(f"Error testing file server: {e}")
        return False

def test_debug_client():
    """Test the debug client with WASD movement controls"""
    print("Testing WASD movement with LPXDebugClient...")
    
    try:
        # Create client
        client = lpximage.LPXDebugClient("../ScanTables63")
        
        # Configure client
        client.setWindowTitle("WASD Movement Test")
        client.setWindowSize(800, 600)
        client.setScale(1.0)
        
        # Initialize window
        client.initializeWindow()
        
        # Try to connect to server (assuming one is running on default port)
        print("Attempting to connect to server on localhost:5050...")
        if not client.connect("localhost", 5050):
            print("Failed to connect to server")
            print("Make sure a LPX server is running on port 5050")
            return False
            
        print("Connected to server!")
        print("Testing movement commands...")
        
        # Test movement commands
        movements = [
            (1, 0, 10.0),   # Move right
            (0, 1, 10.0),   # Move down  
            (-1, 0, 10.0),  # Move left
            (0, -1, 10.0),  # Move up
        ]
        
        for deltaX, deltaY, stepSize in movements:
            print(f"Sending movement: deltaX={deltaX}, deltaY={deltaY}, stepSize={stepSize}")
            if client.sendMovementCommand(deltaX, deltaY, stepSize):
                print("Movement command sent successfully")
            else:
                print("Failed to send movement command")
            time.sleep(1)
        
        # Clean up
        client.disconnect()
        print("Debug client test completed")
        return True
        
    except Exception as e:
        print(f"Error testing debug client: {e}")
        return False

def main():
    print("WASD Movement Test")
    print("==================")
    
    # Set up signal handler
    def signal_handler(sig, frame):
        print("\nTest interrupted")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    # Test file server
    print("\n1. Testing File Server...")
    server_success = test_file_server()
    
    # Test debug client (requires server to be running)
    print("\n2. Testing Debug Client...")
    print("Note: This test requires a server to be running on port 5050")
    print("You can start one with: python lpx_file_server_py.py --file ../2342260-hd_1920_1080_30fps.mp4")
    
    # Give user option to test client
    response = input("Do you want to test the debug client? (y/N): ").strip().lower()
    client_success = False
    if response == 'y':
        client_success = test_debug_client()
    else:
        print("Skipping debug client test")
        client_success = True
    
    print("\n" + "="*50)
    print("Test Results:")
    print(f"File Server: {'PASS' if server_success else 'FAIL'}")
    print(f"Debug Client: {'PASS' if client_success else 'SKIP/FAIL'}")
    
    if server_success and client_success:
        print("\nAll tests passed! The WASD movement functionality should work.")
        print("\nTo test interactively:")
        print("1. Run: python lpx_file_server_py.py --file ../2342260-hd_1920_1080_30fps.mp4")
        print("2. In another terminal: python lpx_renderer.py")
        print("3. Use WASD keys in the renderer window to control movement")
    else:
        print("\nSome tests failed. Please check the error messages above.")

if __name__ == "__main__":
    main()
