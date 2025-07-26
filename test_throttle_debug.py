#!/usr/bin/env python3

import lpximage
import time

print("=== Throttle Debug Test ===")
print(f"Current throttle setting: {lpximage.getKeyThrottleMs()}ms")

# Create client
try:
    client = lpximage.LPXDebugClient("./ScanTables63")
    print("Client created successfully")
    
    # Test rapid command sending to see throttling behavior
    print("\nTesting rapid command sending...")
    for i in range(10):
        start_time = time.time()
        success = client.sendMovementCommand(1.0, 0.0, 10.0)
        end_time = time.time()
        
        elapsed_ms = (end_time - start_time) * 1000
        print(f"Command {i+1}: {'Success' if success else 'Failed'} (took {elapsed_ms:.1f}ms)")
        
        if not success:
            print("Command failed - checking if it's due to no server connection")
            break
        
        # Small delay between attempts to test throttling
        time.sleep(0.01)  # 10ms delay

except Exception as e:
    print(f"Error: {e}")
