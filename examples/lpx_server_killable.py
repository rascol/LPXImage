#!/usr/bin/env python3
# lpx_server_killable.py - Wrapper that ensures Ctrl+C always works for server
import os
import sys
import signal
import subprocess
import time

def main():
    # Flag to track if we should exit
    should_exit = False
    
    def signal_handler(sig, frame):
        nonlocal should_exit
        print("\nCtrl+C detected - killing server process...")
        should_exit = True
    
    # Set up signal handler
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Start the actual server as a subprocess
    server_args = ["python3", "lpx_file_server_fixed.py"] + sys.argv[1:]
    print(f"Starting server: {' '.join(server_args)}")
    
    try:
        process = subprocess.Popen(server_args)
        
        # Monitor the process
        while process.poll() is None and not should_exit:
            time.sleep(0.1)
        
        if should_exit:
            print("Force-killing server process...")
            try:
                process.terminate()
                time.sleep(0.5)
                if process.poll() is None:
                    process.kill()
                    print("Server killed with SIGKILL")
            except:
                pass
        
        # Wait for process to finish
        process.wait()
        
    except Exception as e:
        print(f"Error running server: {e}")
        return 1
    
    return process.returncode if not should_exit else 0

if __name__ == "__main__":
    sys.exit(main())
