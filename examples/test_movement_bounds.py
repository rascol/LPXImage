#!/usr/bin/env python3
"""
Quick test to verify the improved movement bounds using scan table mapWidth
"""

import subprocess
import sys
import time
import os

def run_short_test():
    print("=== LPXImage Movement Bounds Test ===")
    print("Testing the improved bounds that use scan table mapWidth instead of output size")
    print("This will show the scan table mapWidth value and max offset bounds in the debug output")
    print()
    
    # Set up environment
    env = os.environ.copy()
    env['PYTHONPATH'] = '/Users/ray/Desktop/LPXImage/build/python'
    env['DYLD_LIBRARY_PATH'] = '/Users/ray/Desktop/LPXImage/build'
    
    try:
        # Start the file server
        print("Starting file server...")
        server_cmd = [
            '/opt/homebrew/bin/python3.13', 'lpx_file_server.py',
            '--file', '../2342260-hd_1920_1080_30fps.mp4',
            '--loop'
        ]
        
        server_process = subprocess.Popen(
            server_cmd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            universal_newlines=True
        )
        
        # Let server start
        print("Waiting for server to start...")
        time.sleep(3)
        
        # Start the renderer briefly to trigger movement commands
        print("Starting renderer to test movement bounds...")
        renderer_cmd = [
            '/opt/homebrew/bin/python3.13', 'lpx_renderer.py',
            '--host', '127.0.0.1',
            '--width', '800',
            '--height', '600'
        ]
        
        renderer_process = subprocess.Popen(
            renderer_cmd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            universal_newlines=True
        )
        
        print("Test running... Look for '[DEBUG] Scan table mapWidth' in the output below:")
        print("You should see mapWidth value (probably 6000) and max offset bounds (probably ±2400)")
        print("Press Ctrl+C after you see the bounds debugging output")
        print()
        
        # Monitor output for bounds information
        line_count = 0
        found_bounds = False
        
        while True:
            # Read from server process
            if server_process.poll() is None:
                try:
                    line = server_process.stdout.readline()
                    if line:
                        print(f"SERVER: {line.strip()}")
                        if "Scan table mapWidth" in line:
                            found_bounds = True
                            print("*** FOUND BOUNDS DEBUG INFO! ***")
                except:
                    pass
            
            # Read from renderer process  
            if renderer_process.poll() is None:
                try:
                    line = renderer_process.stdout.readline()
                    if line:
                        print(f"RENDERER: {line.strip()}")
                except:
                    pass
            
            line_count += 1
            if line_count > 100 and found_bounds:
                print("Found bounds info, stopping test...")
                break
                
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\nTest stopped by user")
    finally:
        # Clean up
        try:
            renderer_process.terminate()
            renderer_process.wait(timeout=2)
        except:
            try:
                renderer_process.kill()
            except:
                pass
        
        try:
            server_process.terminate()
            server_process.wait(timeout=2)
        except:
            try:
                server_process.kill()
            except:
                pass
        
        print("Test complete!")

if __name__ == "__main__":
    run_short_test()
