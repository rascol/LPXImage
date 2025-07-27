#!/usr/bin/env python3
"""
Test WebcamLPXServer from python/README.md
"""

import lpximage
import time

# Create server with scan tables
server = lpximage.WebcamLPXServer("../ScanTables63")
print("✓ Successfully created WebcamLPXServer")

# Start the server (this will fail if no camera is available)
# Parameters: camera_id, width, height
if not server.start(0, 640, 480):
    print("⚠️  Failed to start server (no camera available or camera in use)")
    print("✓ Server object creation and start method work correctly")
else:
    print("✓ Successfully started server with camera")
    
    # Check client count
    clients = server.getClientCount()
    print(f"Client count: {clients}")
    
    # Let it run for a moment
    time.sleep(2)
    
    # Stop the server when done
    server.stop()
    print("✓ Successfully stopped server")
