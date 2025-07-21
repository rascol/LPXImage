#!/usr/bin/env python3
# test_wasd_commands.py - Test WASD movement commands with connected client

import lpximage
import time
import sys

def test_wasd_with_server():
    print("=== Testing WASD Movement Commands ===")
    print("Make sure lpx_file_server.py is running on port 5050")
    print()
    
    # Create and configure client
    client = lpximage.LPXDebugClient('/Users/ray/Desktop/LPXImage/ScanTables63')
    client.setWindowTitle("WASD Movement Test")
    client.setWindowSize(800, 600)
    client.initializeWindow()
    
    # Connect to server
    print("Connecting to server...")
    if not client.connect("127.0.0.1"):
        print("❌ Failed to connect to server. Is lpx_file_server.py running?")
        return False
    
    print("✅ Connected successfully!")
    print()
    print("Testing movement commands:")
    
    # Test each direction
    movements = [
        ("W", 0, -1, "UP"),
        ("S", 0, 1, "DOWN"), 
        ("A", -1, 0, "LEFT"),
        ("D", 1, 0, "RIGHT")
    ]
    
    for key, deltaX, deltaY, direction in movements:
        print(f"Testing {key} key - {direction} movement...")
        try:
            result = client.sendMovementCommand(deltaX, deltaY, 10.0)
            if result:
                print(f"  ✅ Movement command sent successfully: {direction}")
            else:
                print(f"  ❌ Movement command failed: {direction}")
        except Exception as e:
            print(f"  ❌ Error sending movement: {e}")
        
        # Process a few frames to see the effect
        for i in range(5):
            if not client.processEvents():
                break
            time.sleep(0.1)
        
        time.sleep(1)  # Pause between movements
    
    print()
    print("Movement test completed. Check if the image center moved.")
    
    # Keep running to see the effect
    print("Press any key in terminal to exit...")
    input()
    
    client.disconnect()
    return True

if __name__ == "__main__":
    test_wasd_with_server()
