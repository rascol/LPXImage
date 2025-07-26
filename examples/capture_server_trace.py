#!/usr/bin/env python3
"""
Simple test to capture the detailed server-side tracing during first frame processing
"""

import subprocess
import time
import signal
import sys

def run_test():
    server_process = None
    client_process = None
    
    try:
        print("Starting server with detailed tracing...")
        server_process = subprocess.Popen([
            "python3", "lpx_file_server.py",
            "--file", "../2342260-hd_1920_1080_30fps.mp4",
            "--width", "640", "--height", "480"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        
        # Wait for server to start
        time.sleep(2)
        
        print("Starting client...")
        client_process = subprocess.Popen([
            "python3", "lpx_renderer_simple.py"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        
        # Monitor server output for tracing information
        print("Monitoring server output for 15 seconds...")
        start_time = time.time()
        while (time.time() - start_time) < 15:
            line = server_process.stdout.readline()
            if line:
                line_clean = line.strip()
                print(f"SERVER: {line_clean}")
                
                # Stop when we see the first frame completion
                if "[TRACE] First frame processing complete" in line_clean:
                    print("✅ First frame processing completed!")
                    break
            
            time.sleep(0.01)
        
        # Check client output briefly
        print("\nClient output:")
        for _ in range(10):
            try:
                line = client_process.stdout.readline()
                if line:
                    print(f"CLIENT: {line.strip()}")
            except:
                break
            time.sleep(0.1)
        
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    finally:
        # Cleanup
        if client_process:
            try:
                client_process.terminate()
                client_process.wait(timeout=2)
            except:
                try:
                    client_process.kill()
                except:
                    pass
        
        if server_process:
            try:
                server_process.terminate()  
                server_process.wait(timeout=2)
            except:
                try:
                    server_process.kill()
                except:
                    pass

if __name__ == "__main__":
    run_test()
