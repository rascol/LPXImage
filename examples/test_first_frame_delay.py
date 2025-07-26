#!/usr/bin/env python3
"""
Test to measure the actual first frame processing delay.
This test starts fresh server instances to avoid any caching effects.
"""

import subprocess
import time
import socket
import threading
import sys
import os
import signal

def wait_for_server(host='127.0.0.1', port=5050, timeout=30):
    """Wait for server to be available."""
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((host, port))
            sock.close()
            if result == 0:
                return True
        except:
            pass
        time.sleep(0.1)
    return False

def measure_first_frame_delay():
    """Measure the delay for the first frame processing."""
    
    print("=" * 60)
    print("MEASURING FIRST FRAME PROCESSING DELAY")
    print("=" * 60)
    
    # Kill any existing servers
    try:
        subprocess.run(['pkill', '-f', 'lpx_server.py'], check=False)
        subprocess.run(['pkill', '-f', 'lpx_file_server'], check=False)
        time.sleep(1)
    except Exception as e:
        print(f"Note: Error killing existing processes: {e}")
    
    for test_num in range(3):
        print(f"\n--- TEST {test_num + 1}/3: Fresh Server Start ---")
        
        # Start server in background
        server_env = os.environ.copy()
        server_process = subprocess.Popen(
            [sys.executable, 'lpx_server.py'],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1,
            env=server_env
        )
        
        # Wait for server to be ready
        if not wait_for_server(timeout=10):
            print("❌ Server failed to start")
            server_process.terminate()
            continue
            
        print("✅ Server started successfully")
        
        # Connect client and measure first frame time
        client_start_time = time.time()
        
        try:
            # Connect and request first frame
            client_process = subprocess.Popen(
                [sys.executable, '-c', '''
import socket
import time
import sys

try:
    # Connect to server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(30)  # Long timeout for first frame
    
    connect_start = time.time()
    sock.connect(("127.0.0.1", 5050))
    connect_time = time.time() - connect_start
    print(f"CLIENT: Connected in {connect_time*1000:.1f}ms")
    
    # Wait for first frame
    first_frame_start = time.time()
    data = sock.recv(8192)  # Receive first data
    first_frame_time = time.time() - first_frame_start
    
    print(f"CLIENT: First frame received in {first_frame_time*1000:.1f}ms")
    print(f"CLIENT: Total time from connect to first frame: {(time.time() - connect_start)*1000:.1f}ms")
    
    sock.close()
    sys.exit(0)
    
except Exception as e:
    print(f"CLIENT ERROR: {e}")
    sys.exit(1)
                '''],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True
            )
            
            # Capture client output with timeout
            try:
                client_output, _ = client_process.communicate(timeout=30)
                print("Client results:")
                for line in client_output.strip().split('\n'):
                    if line.strip():
                        print(f"  {line}")
                        
            except subprocess.TimeoutExpired:
                print("❌ Client timed out waiting for first frame")
                client_process.kill()
                
        except Exception as e:
            print(f"❌ Client connection failed: {e}")
            
        finally:
            # Clean up server
            try:
                server_process.terminate()
                server_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                server_process.kill()
            
            # Brief pause between tests
            time.sleep(2)
    
    print(f"\n{'=' * 60}")
    print("FIRST FRAME DELAY MEASUREMENT COMPLETE")
    print(f"{'=' * 60}")

if __name__ == "__main__":
    # Change to examples directory
    if not os.path.exists('lpx_server.py'):
        print("Error: lpx_server.py not found. Run from examples directory.")
        sys.exit(1)
        
    measure_first_frame_delay()
