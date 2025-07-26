#!/usr/bin/env python3

import subprocess
import time
import socket
import threading
import sys
import os
import signal

def test_startup_delay():
    """Test one server startup and measure first frame delay"""
    
    # Start server
    print("Starting server...")
    server_process = subprocess.Popen(
        ['python3', 'lpx_file_server.py', '--file', '../2342260-hd_1920_1080_30fps.mp4', '--port', '8080'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        preexec_fn=os.setsid  # Create new process group for clean termination
    )
    
    # Give server time to initialize
    time.sleep(2.0)  # Longer wait for server startup
    
    # Check if server started successfully
    if server_process.poll() is not None:
        # Server process has terminated
        output, _ = server_process.communicate()
        print(f"Server failed to start. Output:\n{output}")
        return None
    
    try:
        # Connect and measure first frame delay
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.settimeout(10.0)  # 10 second timeout
        
        start_time = time.time()
        print(f"[{start_time:.3f}] Attempting to connect...")
        
        client_socket.connect(('localhost', 8080))
        connect_time = time.time()
        print(f"[{connect_time:.3f}] Connected after {(connect_time - start_time)*1000:.1f}ms")
        
        # Try to receive first frame
        first_byte_time = None
        total_received = 0
        
        while total_received < 4:  # At least get the header
            data = client_socket.recv(1024)
            if data:
                if first_byte_time is None:
                    first_byte_time = time.time()
                    delay = (first_byte_time - start_time) * 1000
                    print(f"[{first_byte_time:.3f}] First data received after {delay:.1f}ms")
                    return delay
                total_received += len(data)
        
        return None  # No data received
        
    except Exception as e:
        print(f"Client error: {e}")
        return None
    finally:
        try:
            client_socket.close()
        except:
            pass
        
        # Kill server process group
        try:
            os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
            server_process.wait(timeout=2)
        except:
            try:
                os.killpg(os.getpgid(server_process.pid), signal.SIGKILL)
            except:
                pass

def main():
    print("Testing server startup repeatedly to catch slow startup...")
    print("Press Ctrl+C to stop")
    
    test_count = 0
    delays = []
    
    try:
        while True:
            test_count += 1
            print(f"\n=== Test {test_count} ===")
            
            delay = test_startup_delay()
            
            if delay is not None:
                delays.append(delay)
                print(f"First frame delay: {delay:.1f}ms")
                
                # Check if this is a slow startup (more than 5 seconds)
                if delay > 5000:
                    print(f"*** SLOW STARTUP DETECTED! {delay:.1f}ms ***")
                    print("This is the problematic startup we're looking for!")
                    
                    # Collect additional information
                    print("\nRecent delays:")
                    for i, d in enumerate(delays[-10:]):
                        print(f"  Test {test_count-len(delays)+i+1}: {d:.1f}ms")
                    
                    print(f"\nAverage delay so far: {sum(delays)/len(delays):.1f}ms")
                    print(f"Max delay: {max(delays):.1f}ms")
                    print(f"Min delay: {min(delays):.1f}ms")
                    
                    # Ask if we should continue looking for more slow startups
                    try:
                        response = input("\nContinue testing for more slow startups? (y/n): ")
                        if response.lower() != 'y':
                            break
                    except KeyboardInterrupt:
                        break
                        
            else:
                print("Failed to measure delay")
            
            # Brief pause between tests
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\nStopping tests...")
    
    if delays:
        print(f"\n=== Summary after {len(delays)} successful tests ===")
        print(f"Average delay: {sum(delays)/len(delays):.1f}ms")
        print(f"Max delay: {max(delays):.1f}ms")
        print(f"Min delay: {min(delays):.1f}ms")
        
        slow_startups = [d for d in delays if d > 5000]
        if slow_startups:
            print(f"Slow startups (>5s): {len(slow_startups)} out of {len(delays)}")
            print(f"Slow startup delays: {[f'{d:.1f}ms' for d in slow_startups]}")
        else:
            print("No slow startups detected yet - keep running to catch one!")

if __name__ == "__main__":
    main()
