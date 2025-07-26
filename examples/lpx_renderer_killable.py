#!/usr/bin/env python3
# lpx_renderer_killable.py - Wrapper that ensures Ctrl+C always works
import os
import sys
import signal
import subprocess
import time

def main():
    # Get the current process ID
    parent_pid = os.getpid()
    
    # Flag to track if we should exit
    should_exit = False
    
    def signal_handler(sig, frame):
        nonlocal should_exit
        print("\nCtrl+C detected - killing renderer process...")
        should_exit = True
    
    # Set up signal handler
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Start the actual renderer as a subprocess
    renderer_args = ["python3", "lpx_renderer.py"] + sys.argv[1:]
    print(f"Starting renderer: {' '.join(renderer_args)}")
    
    try:
        process = subprocess.Popen(renderer_args)
        
        # Monitor the process
        while process.poll() is None and not should_exit:
            time.sleep(0.1)
        
        if should_exit:
            print("Force-killing renderer process...")
            try:
                process.terminate()
                time.sleep(0.5)
                if process.poll() is None:
                    process.kill()
                    print("Renderer killed with SIGKILL")
            except:
                pass
        
        # Wait for process to finish
        process.wait()
        
    except Exception as e:
        print(f"Error running renderer: {e}")
        return 1
    
    return process.returncode if not should_exit else 0

if __name__ == "__main__":
    sys.exit(main())
