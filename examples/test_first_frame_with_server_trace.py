#!/usr/bin/env python3
"""
Test to capture server trace output during first frame delay measurement.
This will show us exactly where the 2.5-3 second delay is occurring.
"""

import subprocess
import time
import socket
import threading
import sys
import os
import signal
from queue import Queue

def capture_server_output(process, output_queue):
    """Capture server output in a separate thread."""
    while True:
        line = process.stdout.readline()
        if not line:
            break
        output_queue.put(('SERVER', line.strip()))

def measure_first_frame_with_trace():
    """Measure first frame delay while capturing server trace output."""
    
    print("=" * 80)
    print("MEASURING FIRST FRAME DELAY WITH SERVER TRACE OUTPUT")
    print("=" * 80)
    
    # Kill any existing servers
    try:
        subprocess.run(['pkill', '-f', 'lpx_server.py'], check=False)
        time.sleep(1)
    except Exception as e:
        print(f"Note: Error killing existing processes: {e}")
    
    print("\n--- Starting Fresh Server with Trace Capture ---")
    
    # Start server with output capture
    server_process = subprocess.Popen(
        [sys.executable, 'lpx_server.py'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1
    )
    
    # Start output capture thread
    output_queue = Queue()
    capture_thread = threading.Thread(
        target=capture_server_output, 
        args=(server_process, output_queue),
        daemon=True
    )
    capture_thread.start()
    
    # Wait for server to be ready
    print("Waiting for server to start...")
    server_ready = False
    start_wait = time.time()
    
    while time.time() - start_wait < 15:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex(('127.0.0.1', 5050))
            sock.close()
            if result == 0:
                server_ready = True
                break
        except:
            pass
        time.sleep(0.1)
    
    if not server_ready:
        print("❌ Server failed to start within 15 seconds")
        server_process.terminate()
        return
    
    print("✅ Server started, connecting client to trigger first frame...")
    
    # Record start time for first frame measurement
    client_start_time = time.time()
    
    # Connect client
    try:
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.settimeout(10)
        
        connect_start = time.time()
        client_sock.connect(('127.0.0.1', 5050))
        connect_time = time.time() - connect_start
        
        print(f"CLIENT: Connected in {connect_time*1000:.1f}ms")
        
        # Wait for first frame
        first_frame_start = time.time()
        data = client_sock.recv(8192)
        first_frame_time = time.time() - first_frame_start
        total_time = time.time() - connect_start
        
        print(f"CLIENT: First frame received in {first_frame_time*1000:.1f}ms")
        print(f"CLIENT: Total time: {total_time*1000:.1f}ms")
        
        client_sock.close()
        
    except Exception as e:
        print(f"❌ Client error: {e}")
    
    # Give server a moment to finish processing
    time.sleep(0.5)
    
    # Terminate server
    server_process.terminate()
    
    # Collect and display server output
    print("\n" + "=" * 80)
    print("SERVER TRACE OUTPUT:")
    print("=" * 80)
    
    # Collect remaining output
    timeout_start = time.time()
    while time.time() - timeout_start < 2:  # 2 second timeout
        try:
            source, line = output_queue.get_nowait()
            print(f"{source}: {line}")
        except:
            time.sleep(0.1)
    
    print("=" * 80)
    print("TRACE CAPTURE COMPLETE")
    print("=" * 80)

if __name__ == "__main__":
    # Change to examples directory
    if not os.path.exists('lpx_server.py'):
        print("Error: lpx_server.py not found. Run from examples directory.")
        sys.exit(1)
        
    measure_first_frame_with_trace()
