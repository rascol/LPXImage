#!/usr/bin/env python3
# debug_keyboard.py - Debug keyboard input issues

import lpximage
import time
import sys

def debug_keyboard_input():
    print("=== Keyboard Input Debug Test ===")
    
    # Get version info
    try:
        version = lpximage.getVersionString()
        build = lpximage.getBuildNumber()
        throttle = lpximage.getKeyThrottleMs()
        print(f"LPXImage v{version} (Build {build}), Throttle: {throttle}ms")
    except AttributeError:
        print("Version info not available")
    
    # Create client
    print("Creating LPXDebugClient...")
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowTitle("Keyboard Debug Test")
    client.setWindowSize(600, 400)
    client.setScale(1.0)
    
    # Initialize window
    print("Initializing window...")
    client.initializeWindow()
    
    # Connect to server
    print("Connecting to server...")
    if not client.connect("127.0.0.1:5050"):
        print("ERROR: Failed to connect to server")
        return False
    
    print("Connected successfully!")
    print("INSTRUCTIONS:")
    print("1. Click on the display window to focus it")
    print("2. Try pressing WASD keys") 
    print("3. Watch for key press logs in this terminal")
    print("4. Press Q or ESC to quit")
    print("5. If keys stop working, check for error messages")
    
    # Main loop - monitor for issues
    loop_count = 0
    last_running_check = time.time()
    
    while client.isRunning():
        loop_count += 1
        
        # Process events
        try:
            result = client.processEvents()
            if not result:
                print("processEvents() returned False - client stopping")
                break
        except Exception as e:
            print(f"ERROR in processEvents(): {e}")
            break
        
        # Check if still running every 5 seconds
        now = time.time()
        if now - last_running_check >= 5.0:
            print(f"Debug: Still running - loop {loop_count}, isRunning: {client.isRunning()}")
            last_running_check = now
        
        # Very brief sleep
        time.sleep(0.001)  # 1ms like the C++ code
    
    print("Main loop ended")
    
    # Disconnect
    try:
        client.disconnect()
        print("Disconnected successfully")
    except Exception as e:
        print(f"Error during disconnect: {e}")
    
    return True

if __name__ == "__main__":
    debug_keyboard_input()
