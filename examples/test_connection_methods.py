#!/usr/bin/env python3
# test_connection_methods.py - Test different ways to connect
import lpximage
import time

def test_connection_methods():
    print("Testing different connection methods...")
    
    # Start a server on port 5052
    server = lpximage.FileLPXServer("../ScanTables63", 5052)
    video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
    
    if not server.start(video_path, 800, 600):
        print("Failed to start server")
        return
    
    print("Server started on port 5052")
    time.sleep(1)
    
    # Create client
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowSize(400, 300)
    client.initializeWindow()
    
    # Test 1: Just hostname (will probably use default port 5050)
    print("\nTest 1: Connecting to just '127.0.0.1' (default port)")
    result = client.connect("127.0.0.1")
    print(f"Result: {result}")
    if result:
        client.disconnect()
    
    # Test 2: Try with localhost
    print("\nTest 2: Connecting to 'localhost'")
    result = client.connect("localhost")
    print(f"Result: {result}")
    if result:
        client.disconnect()
    
    # Test 3: Check if there are other methods on the client
    print("\nTest 3: Available methods on LPXDebugClient:")
    methods = [method for method in dir(client) if not method.startswith('_')]
    for method in methods:
        print(f"  - {method}")
    
    server.stop()
    print("\nServer stopped")

if __name__ == "__main__":
    test_connection_methods()
