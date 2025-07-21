#!/usr/bin/env python3
# test_movement_binding.py - Test to isolate the sendMovementCommand binding issue

import lpximage
import sys

def test_movement_command():
    print("=== Testing sendMovementCommand Binding ===")
    
    # Create client
    try:
        client = lpximage.LPXDebugClient('/Users/ray/Desktop/LPXImage/ScanTables63')
        print("✓ LPXDebugClient created successfully")
    except Exception as e:
        print(f"✗ Failed to create client: {e}")
        return False
    
    # List all available methods
    print("\nAvailable methods:")
    methods = [method for method in dir(client) if not method.startswith('_')]
    for i, method in enumerate(methods, 1):
        print(f"  {i}. {method}")
    
    # Check for sendMovementCommand specifically
    print(f"\nTotal methods: {len(methods)}")
    print(f"sendMovementCommand present: {'sendMovementCommand' in methods}")
    
    # Try to get the method directly
    try:
        method = getattr(client, 'sendMovementCommand', None)
        print(f"Direct attribute access: {method}")
    except Exception as e:
        print(f"Error accessing attribute: {e}")
    
    # Check the Python module info
    print(f"\nModule file: {lpximage.__file__}")
    print(f"Module methods: {[m for m in dir(lpximage) if not m.startswith('_')]}")
    
    return True

if __name__ == "__main__":
    test_movement_command()
