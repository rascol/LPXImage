#!/usr/bin/env python3
"""
Simple timing test to measure actual key press response time
"""
import time
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

try:
    import lpximage
except ImportError:
    print("ERROR: lpximage module not found!")
    sys.exit(1)

def test_throttling():
    print("=== KEY THROTTLING TEST ===")
    print("Testing lpximage module location:", lpximage.__file__)
    
    # Try to create a debug client to test key throttling
    try:
        client = lpximage.LPXDebugClient("ScanTables63")
        print("LPXDebugClient created successfully")
        
        # Test key press timing simulation
        print("\nSimulating rapid key presses...")
        start_time = time.time()
        
        for i in range(10):
            press_time = time.time()
            print(f"Key press {i+1} at {press_time:.4f}s")
            
            # Try to send a movement command
            result = client.sendMovementCommand(0, -1, 10)
            
            end_time = time.time()
            duration_ms = (end_time - press_time) * 1000
            print(f"  Command sent in {duration_ms:.2f}ms, result: {result}")
            
            # Small delay between tests
            time.sleep(0.1)
        
        total_time = time.time() - start_time
        print(f"\nTotal test time: {total_time:.4f}s")
        
    except Exception as e:
        print(f"Error creating debug client: {e}")
        print("This is expected if no server is running")

if __name__ == "__main__":
    test_throttling()
