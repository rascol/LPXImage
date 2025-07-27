#!/usr/bin/env python3
"""
Simple test of server/client functionality from README.md
"""

import lpximage
import threading
import time

def test_server_client():
    print("Testing server/client functionality...")
    
    # Create server
    server = lpximage.WebcamLPXServer("../ScanTables63")
    
    # Start server in background
    def run_server():
        if not server.start(0, 640, 480):
            print("❌ Failed to start server")
            return
        print("✓ Server started successfully")
        time.sleep(5)  # Run for 5 seconds
        server.stop()
        print("✓ Server stopped")
    
    server_thread = threading.Thread(target=run_server)
    server_thread.daemon = True
    server_thread.start()
    
    # Give server time to start
    time.sleep(2)
    
    # Test client connection
    client = lpximage.LPXDebugClient("../ScanTables63")
    client.setWindowTitle("Test Connection")
    client.setWindowSize(400, 300)
    client.setScale(1.0)
    
    # Initialize window (required for macOS)
    client.initializeWindow()
    
    # Try to connect
    if client.connect("127.0.0.1"):
        print("✓ Client connected successfully")
        
        # Process a few events to verify streaming
        for i in range(10):
            if not client.processEvents():
                break
            time.sleep(0.1)
        
        client.disconnect()
        print("✓ Client disconnected")
    else:
        print("❌ Client failed to connect")
    
    # Wait for server to finish
    server_thread.join(timeout=10)
    
    print("✓ Server/client test completed")

if __name__ == "__main__":
    test_server_client()
