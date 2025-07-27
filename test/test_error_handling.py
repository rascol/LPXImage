#!/usr/bin/env python3
"""
Test error handling example from python/README.md
"""

import lpximage

# Example of proper error handling
client = lpximage.LPXDebugClient("../ScanTables63")

# Test connection error handling (no server running)
if not client.connect("127.0.0.1"):
    print("✓ Properly handled connection failure when no server is running")
else:
    print("⚠️  Unexpectedly connected when no server is running")
    client.disconnect()

# Test server error handling  
server = lpximage.WebcamLPXServer("../ScanTables63")

# When starting the server (likely to fail without camera)
if not server.start(99, 640, 480):  # Use unlikely camera ID
    print("✓ Properly handled server start failure")
else:
    print("⚠️  Unexpectedly started server with camera ID 99")
    server.stop()

print("✓ Error handling tests completed")
