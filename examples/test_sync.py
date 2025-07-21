#!/usr/bin/env python3
# test_sync.py - Test video synchronization on client connection
import lpximage
import time

def test_sync_behavior():
    """Test that the video restarts from frame 0 when first client connects"""
    print("=== Testing Video Sync Behavior ===")
    
    # Start server
    server = lpximage.FileLPXServer("../ScanTables63", 5055)
    video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
    
    print("Starting server and letting it run for a moment...")
    server.start(video_path, 800, 600)
    
    # Let server run for a few seconds to advance past frame 0
    print("Waiting 3 seconds to let video advance...")
    time.sleep(3)
    
    print("Now connecting first client - should trigger video restart to frame 0")
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowSize(400, 300)
    client.initializeWindow()
    
    # Connect - this should trigger video restart
    connected = client.connect("127.0.0.1:5055")
    
    if connected:
        print("✓ Client connected successfully")
        print("Looking for sync restart message in server output...")
        
        # Give it a moment to process
        time.sleep(2)
        
        # Check that client receives data
        if client.isRunning():
            print("✓ Client is receiving data")
            client.processEvents()
        
        client.disconnect()
        print("✓ Client disconnected")
    else:
        print("✗ Failed to connect client")
    
    server.stop()
    print("✓ Server stopped")
    print("\nLook for the message: 'First client connected - restarting video for sync'")

if __name__ == "__main__":
    print("LPXImage Sync Test")
    print("=" * 40)
    test_sync_behavior()
    print("\nSync test completed!")
