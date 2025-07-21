#!/usr/bin/env python3
# test_wasd_movement.py - Test WASD movement commands
import lpximage
import time
import sys

def test_movement_commands():
    print("Testing WASD movement commands...")
    
    # Create client
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowTitle("WASD Test")
    client.setWindowSize(400, 300)
    client.initializeWindow()
    
    # Connect to server
    if not client.connect("127.0.0.1"):
        print("Failed to connect to server on port 5050")
        return False
    
    print("Connected successfully!")
    
    # Test movement commands
    movements = [
        ("W", 0, -1, "up"),
        ("S", 0, 1, "down"), 
        ("A", -1, 0, "left"),
        ("D", 1, 0, "right")
    ]
    
    for key, deltaX, deltaY, direction in movements:
        print(f"Testing {key} key - moving {direction}")
        result = client.sendMovementCommand(deltaX, deltaY, 10.0)
        print(f"Movement command result: {result}")
        time.sleep(1)
    
    # Process a few frames to see if movement took effect
    for i in range(10):
        if not client.processEvents():
            break
        time.sleep(0.1)
    
    client.disconnect()
    print("Test completed!")
    return True

if __name__ == "__main__":
    test_movement_commands()
