#!/usr/bin/env python3
# test_movement_commands.py - Test WASD movement command sending

import lpximage
import time
import sys

def test_movement_command_sending():
    """Test sending movement commands to verify they work beyond 8 keypresses"""
    print("Testing WASD movement command sending...")
    
    # Create client 
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowTitle("Movement Command Test")
    client.setWindowSize(400, 300)
    client.initializeWindow()
    
    print("Client created and initialized")
    
    # Test 12 movement commands (more than the supposed 8-command limit)
    movements = [
        ("W", 0, -1, "up"),
        ("S", 0, 1, "down"), 
        ("A", -1, 0, "left"),
        ("D", 1, 0, "right"),
        ("W", 0, -1, "up"),
        ("S", 0, 1, "down"), 
        ("A", -1, 0, "left"),
        ("D", 1, 0, "right"),
        ("W", 0, -1, "up"),      # Command 9 - should work if bug is fixed
        ("S", 0, 1, "down"),     # Command 10
        ("A", -1, 0, "left"),    # Command 11  
        ("D", 1, 0, "right")     # Command 12
    ]
    
    print(f"Testing {len(movements)} movement commands...")
    success_count = 0
    failure_count = 0
    
    # Test each command
    for i, (key, deltaX, deltaY, direction) in enumerate(movements, 1):
        print(f"Command {i:2d}: {key} key - moving {direction}")
        
        start_time = time.time()
        try:
            # Note: sendMovementCommand may fail due to no server connection,
            # but the important part is that it doesn't crash or return false due to internal limits
            result = client.sendMovementCommand(deltaX, deltaY, 10.0)
            end_time = time.time()
            
            if result:
                success_count += 1
                status = "SUCCESS"
            else:
                failure_count += 1
                status = "FAILED"
            
            print(f"    Result: {status}, Time: {(end_time - start_time)*1000:.2f}ms")
            
        except Exception as e:
            failure_count += 1
            print(f"    Result: EXCEPTION - {e}")
        
        # Small delay between commands
        time.sleep(0.05)
    
    print(f"\\nTest Results:")
    print(f"  Total commands: {len(movements)}")
    print(f"  Successful: {success_count}")
    print(f"  Failed: {failure_count}")
    
    if failure_count == 0:
        print("\\n✅ SUCCESS: All movement commands executed without internal failures!")
        print("The 8-keypress limit bug appears to be fixed.")
    elif success_count > 8:
        print("\\n✅ PARTIAL SUCCESS: Commands beyond 8 worked!")
        print("The 8-keypress limit bug appears to be fixed.")
        print("(Some failures may be due to no server connection, which is expected)")
    else:
        print("\\n❌ FAILURE: Commands failed, possibly due to the 8-keypress bug.")
    
    return success_count > 8

def main():
    print("=== WASD Movement Command Test ===")
    print("This test verifies that movement commands work beyond the 8-command limit.")
    print("Note: Commands may fail due to no server connection, but should not fail due to internal limits.")
    print()
    
    success = test_movement_command_sending()
    
    if success:
        print("\\n🎉 The 8-keypress WASD movement bug has been fixed!")
    else:
        print("\\n⚠️  The issue may still exist or there are other problems.")

if __name__ == "__main__":
    main()
