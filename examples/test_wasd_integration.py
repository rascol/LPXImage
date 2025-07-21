#!/usr/bin/env python3
"""
Test script to verify that WASD keyboard integration works correctly.
This will start a file server and renderer to test the integrated keyboard controls.
"""
import subprocess
import time
import sys
import os

def main():
    print("=== LPXImage WASD Integration Test ===")
    print("This test will:")
    print("1. Start the lpx_file_server with a test video")
    print("2. Start the lpx_renderer")
    print("3. You should see a single window with video")
    print("4. Click on the window and use WASD keys to move")
    print("5. Press Q or ESC to quit")
    print("\nStarting in 3 seconds...")
    time.sleep(3)
    
    # Start file server using Python 3.13
    server_cmd = [
        '/opt/homebrew/bin/python3.13', 'lpx_file_server.py',
        '--file', '../2342260-hd_1920_1080_30fps.mp4',
        '--width', '640',
        '--height', '480',
        '--loop'
    ]
    
    print("Starting file server...")
    server_process = subprocess.Popen(server_cmd)
    
    # Give server time to start - reduced for faster sync
    time.sleep(0.5)
    
    try:
        # Start renderer using Python 3.13
        renderer_cmd = [
            '/opt/homebrew/bin/python3.13', 'lpx_renderer.py',
            '--host', '127.0.0.1',
            '--width', '800',
            '--height', '600'
        ]
        
        print("Starting renderer...")
        print("You should now see a single window.")
        print("Click on it and use WASD keys to move the view!")
        renderer_process = subprocess.run(renderer_cmd)
        
    except KeyboardInterrupt:
        print("Test interrupted by user")
    
    finally:
        print("Stopping server...")
        server_process.terminate()
        server_process.wait()
        print("Test complete!")

if __name__ == "__main__":
    main()
