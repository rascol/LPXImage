#!/usr/bin/env python3
# test_8_keypresses.py - Test exactly 8 WASD keypresses to reproduce the bug

import lpximage
import time
import threading
import sys
import os

def test_8_keypresses():
    """Test sending exactly 8 movement commands to see if movement stops"""
    print("Testing 8 consecutive WASD movement commands...")
    
    # Check if video file exists
    video_file = "../2342260-hd_1920_1080_30fps.mp4"
    if not os.path.exists(video_file):
        print(f"Test video file not found: {video_file}")
        print("Please ensure the test video file exists")
        return False
    
    # Create and start file server for testing
    print("Creating file server for testing...")
    server = lpximage.FileLPXServer("../ScanTables63", 5051)  # Use port 5051 to avoid conflicts
    if not server.start(video_file, 800, 600):
        print("Failed to start file server")
        return False
    
    print("File server started on port 5051")
    
    # Give server time to initialize
    time.sleep(2)
    
    # Create client
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowTitle("8 Keypress Test")
    client.setWindowSize(400, 300)
    client.initializeWindow()
    
    # Connect to our test server
    print("Connecting to test server on localhost:5051...")
    if not client.connect("localhost:5051"):
        print("Failed to connect to test server")
        server.stop()
        return False
    
    print("Connected successfully!")
    
    # Define 8 movement commands
    movements = [
        ("W", 0, -1, "up"),
        ("S", 0, 1, "down"), 
        ("A", -1, 0, "left"),
        ("D", 1, 0, "right"),
        ("W", 0, -1, "up"),
        ("S", 0, 1, "down"), 
        ("A", -1, 0, "left"),
        ("D", 1, 0, "right")
    ]
    
    print(f"\nSending {len(movements)} movement commands...")
    
    # Send each command with timing info
    for i, (key, deltaX, deltaY, direction) in enumerate(movements, 1):
        print(f"Command {i}/8: {key} key - moving {direction}")
        
        start_time = time.time()
        result = client.sendMovementCommand(deltaX, deltaY, 10.0)
        end_time = time.time()
        
        print(f"  Result: {result}, Time: {(end_time - start_time)*1000:.2f}ms")
        
        if not result:
            print(f"  ERROR: Movement command {i} failed!")
            break
        
        # Wait between commands to respect throttling
        time.sleep(0.1)  # 100ms between commands
    
    print("\nAll 8 commands sent. Now testing if movement still works...")
    
    # Test additional commands to see if they still work
    additional_movements = [
        ("W", 0, -1, "up"),
        ("S", 0, 1, "down")
    ]
    
    for i, (key, deltaX, deltaY, direction) in enumerate(additional_movements, 9):
        print(f"Additional command {i}: {key} key - moving {direction}")
        
        start_time = time.time()
        result = client.sendMovementCommand(deltaX, deltaY, 10.0)
        end_time = time.time()
        
        print(f"  Result: {result}, Time: {(end_time - start_time)*1000:.2f}ms")
        
        if not result:
            print(f"  ERROR: Additional movement command {i} failed!")
            print("  This confirms the bug - movement stopped after 8 keypresses!")
            break
        
        time.sleep(0.1)
    
    # Process a few frames to see results
    print("\nProcessing frames to see movement effects...")
    for i in range(20):
        if not client.processEvents():
            break
        time.sleep(0.05)
    
    client.disconnect()
    server.stop()
    print("Test completed!")
    return True

def main():
    print("=== 8-Keypress WASD Movement Bug Test ===")
    print("This test reproduces the issue where WASD movement stops after exactly 8 keypresses.")
    print("The test will create its own server instance for testing.")
    print()
    
    success = test_8_keypresses()
    
    if success:
        print("\n=== Test Analysis ===")
        print("If movement commands 9+ failed, this confirms the 8-keypress bug.")
        print("The issue might be:")
        print("1. A buffer or queue that fills up after 8 entries")
        print("2. A counter that resets or stops at 8")
        print("3. Socket or connection issues after 8 commands")
        print("4. Server-side command processing limits")
    else:
        print("\n=== Test Failed ===")
        print("Could not complete the test. Check the error messages above.")

if __name__ == "__main__":
    main()
