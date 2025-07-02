#!/usr/bin/env python3
# quick_movement_test.py - Quick test for movement commands
import lpximage
import time

def main():
    print("Quick Movement Command Test")
    print("===========================")
    
    try:
        # Create client
        client = lpximage.LPXDebugClient("../ScanTables63")
        
        # Try to connect to server
        print("Connecting to server on 127.0.0.1:5050...")
        if not client.connect("127.0.0.1", 5050):
            print("Failed to connect to server")
            print("Make sure the file server is running:")
            print("python lpx_file_server_py.py --file ../2342260-hd_1920_1080_30fps.mp4 --port 5050")
            return
            
        print("Connected successfully!")
        
        # Test a few movement commands
        movements = [
            ("W", 0, -1, 15.0),  # Move up (W key)
            ("S", 0, 1, 15.0),   # Move down (S key) 
            ("A", -1, 0, 15.0),  # Move left (A key)
            ("D", 1, 0, 15.0),   # Move right (D key)
        ]
        
        print("\nTesting movement commands...")
        for key, deltaX, deltaY, stepSize in movements:
            print(f"Simulating '{key}' key press: deltaX={deltaX}, deltaY={deltaY}, stepSize={stepSize}")
            if client.sendMovementCommand(deltaX, deltaY, stepSize):
                print("✓ Movement command sent successfully")
            else:
                print("✗ Failed to send movement command")
            time.sleep(0.5)
        
        print("\nMovement test completed!")
        
        # Disconnect
        client.disconnect()
        print("Disconnected from server")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
