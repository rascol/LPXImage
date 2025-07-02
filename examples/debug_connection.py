#!/usr/bin/env python3
# debug_connection.py - Simple test to debug connection issues
import lpximage
import time
import threading
import signal
import sys

def test_basic_connection():
    print("=== Testing basic LPXDebugClient connection ===")
    
    # Test 1: Can we create a client?
    try:
        client = lpximage.LPXDebugClient("../ScanTables63")
        print("✓ LPXDebugClient created successfully")
    except Exception as e:
        print(f"✗ Failed to create LPXDebugClient: {e}")
        return False
    
    # Test 2: Can we configure the client?
    try:
        client.setWindowTitle("Debug Test")
        client.setWindowSize(400, 300)
        client.setScale(1.0)
        print("✓ Client configuration successful")
    except Exception as e:
        print(f"✗ Failed to configure client: {e}")
        return False
    
    # Test 3: Can we initialize the window?
    try:
        client.initializeWindow()
        print("✓ Window initialization successful")
    except Exception as e:
        print(f"✗ Failed to initialize window: {e}")
        return False
    
    # Test 4: Try to connect to a non-existent server (should fail gracefully)
    try:
        result = client.connect("127.0.0.1:9999")  # Non-existent port
        if result:
            print("? Unexpected: Connected to non-existent server")
        else:
            print("✓ Correctly failed to connect to non-existent server")
    except Exception as e:
        print(f"✓ Connection to non-existent server failed as expected: {e}")
    
    return True

def test_file_server():
    print("\n=== Testing FileLPXServer creation ===")
    
    try:
        server = lpximage.FileLPXServer("../ScanTables63", 5052)
        print("✓ FileLPXServer created successfully")
        
        # Try to start with the video file
        video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
        result = server.start(video_path, 800, 600)
        if result:
            print("✓ FileLPXServer started successfully")
            
            # Let it run for a moment
            time.sleep(1)
            
            # Test connection
            client = lpximage.LPXDebugClient("../ScanTables63")
            client.setWindowSize(400, 300)
            client.initializeWindow()
            
            print("Attempting connection to server...")
            if client.connect("127.0.0.1:5052"):
                print("✓ Client connected to server")
                
                # Try to process one frame
                if client.isRunning():
                    print("✓ Client is running")
                    result = client.processEvents()
                    print(f"Process events result: {result}")
                else:
                    print("✗ Client not running after connection")
                
                client.disconnect()
                print("✓ Client disconnected")
            else:
                print("✗ Failed to connect client to server")
            
            server.stop()
            print("✓ Server stopped")
        else:
            print("✗ Failed to start FileLPXServer")
            
    except Exception as e:
        print(f"✗ Error with FileLPXServer: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("LPXImage Connection Debug Test")
    print("=" * 40)
    
    if test_basic_connection():
        test_file_server()
    
    print("\nTest completed.")
