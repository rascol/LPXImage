#!/usr/bin/env python3

import lpximage
import time

print("=== Keypress Timing Test ===")
print("This test will show exactly where the delay occurs")
print("Press WASD keys and watch the timing output")
print("Press 'q' to quit")

try:
    # Create client but don't connect - we just want to test the UI responsiveness
    client = lpximage.LPXDebugClient("./ScanTables63")
    
    # Initialize the window
    client.initializeWindow()
    
    print("\nWindow created. Press keys and watch timing...")
    
    # Process events in a tight loop to see where the delay occurs
    start_time = time.time()
    last_print_time = start_time
    
    while True:
        current_time = time.time()
        
        # Print a status message every 2 seconds to show we're running
        if current_time - last_print_time > 2.0:
            print(f"Waiting for keypresses... (elapsed: {current_time - start_time:.1f}s)")
            last_print_time = current_time
        
        # Process events - this should show the timing
        if not client.processEvents():
            break
            
        # Very small sleep to prevent CPU spinning
        time.sleep(0.001)  # 1ms
    
    print("Test completed")
    
except Exception as e:
    print(f"Error: {e}")
