#!/usr/bin/env python3
# test_file_server.py - Test server using new FileLPXServer with movement support
import lpximage
import time
import signal
import sys
import os

def main():
    print("Starting FileLPXServer with WASD movement support...")
    
    # Set up signal handler
    def signal_handler(sig, frame):
        print("\nShutting down server...")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    try:
        # Create file server
        server = lpximage.FileLPXServer("../ScanTables63", 5050)
        
        # Start server with test video
        video_file = "../2342260-hd_1920_1080_30fps.mp4"
        if not os.path.exists(video_file):
            print(f"Test video not found: {video_file}")
            return
            
        if not server.start(video_file, 800, 600):
            print("Failed to start file server")
            return
            
        print("FileLPXServer started on port 5050")
        print("Server now supports WASD movement commands!")
        print("Press Ctrl+C to stop")
        
        # Keep server running
        try:
            while True:
                time.sleep(1)
                # Report client count every 10 seconds
                count = server.getClientCount()
                if count > 0:
                    print(f"Active clients: {count}")
        except KeyboardInterrupt:
            pass
        finally:
            server.stop()
            print("Server stopped")
            
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
