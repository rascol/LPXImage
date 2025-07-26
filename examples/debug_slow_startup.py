#!/usr/bin/env python3

import subprocess
import time
import socket
import threading
import sys
import os
import signal
import queue

def capture_server_output(process, output_queue):
    """Capture server output in a separate thread"""
    try:
        for line in iter(process.stdout.readline, ''):
            if line:
                output_queue.put(('stdout', time.time(), line.strip()))
            else:
                break
    except Exception as e:
        output_queue.put(('error', time.time(), f"Output capture error: {e}"))

def test_startup_with_logging():
    """Test server startup with detailed logging"""
    
    print("Starting server with detailed logging...")
    
    # Start server process
    server_process = subprocess.Popen(
        ['python3', 'lpx_file_server.py', '--file', '../2342260-hd_1920_1080_30fps.mp4', '--port', '8080'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        preexec_fn=os.setsid
    )
    
    # Queue to collect server output
    output_queue = queue.Queue()
    
    # Start output capture thread
    output_thread = threading.Thread(target=capture_server_output, args=(server_process, output_queue))
    output_thread.daemon = True
    output_thread.start()
    
    server_start_time = time.time()
    print(f"[{server_start_time:.3f}] Server process started")
    
    # Monitor server output and attempt connection
    client_socket = None
    connect_time = None
    first_data_time = None
    
    try:
        # Wait for server to be ready (up to 30 seconds)
        max_wait_time = 30
        connection_attempts = 0
        
        while time.time() - server_start_time < max_wait_time:
            # Process any server output
            try:
                while True:
                    msg_type, timestamp, message = output_queue.get_nowait()
                    elapsed = (timestamp - server_start_time) * 1000
                    print(f"[{elapsed:7.1f}ms] SERVER: {message}")
            except queue.Empty:
                pass
            
            # Check if server process is still running
            if server_process.poll() is not None:
                print("Server process terminated unexpectedly")
                break
            
            # Try to connect every 500ms after initial 2 second wait
            if time.time() - server_start_time > 2.0 and connection_attempts == 0:
                try:
                    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    client_socket.settimeout(1.0)
                    client_socket.connect(('localhost', 8080))
                    connect_time = time.time()
                    connection_delay = (connect_time - server_start_time) * 1000
                    print(f"[{connection_delay:7.1f}ms] CLIENT: Connected successfully")
                    break
                except (ConnectionRefusedError, socket.timeout):
                    connection_attempts += 1
                    if connection_attempts % 10 == 0:  # Report every 5 seconds
                        elapsed = (time.time() - server_start_time) * 1000
                        print(f"[{elapsed:7.1f}ms] CLIENT: Still waiting for connection...")
                    try:
                        client_socket.close()
                    except:
                        pass
                    client_socket = None
            
            time.sleep(0.1)  # Short sleep
        
        if client_socket is None:
            print("Failed to connect to server within timeout")
            return None
        
        # Now try to receive first frame data
        print(f"[{(connect_time - server_start_time) * 1000:7.1f}ms] CLIENT: Waiting for first frame data...")
        
        client_socket.settimeout(20.0)  # 20 second timeout for first frame
        total_received = 0
        
        while total_received < 4:  # At least get the header
            try:
                # Continue processing server output while waiting
                try:
                    while True:
                        msg_type, timestamp, message = output_queue.get_nowait()
                        elapsed = (timestamp - server_start_time) * 1000
                        print(f"[{elapsed:7.1f}ms] SERVER: {message}")
                except queue.Empty:
                    pass
                
                # Try to receive data
                data = client_socket.recv(1024)
                if data:
                    if first_data_time is None:
                        first_data_time = time.time()
                        first_frame_delay = (first_data_time - server_start_time) * 1000
                        print(f"[{first_frame_delay:7.1f}ms] CLIENT: First frame data received!")
                        print(f"Total startup delay: {first_frame_delay:.1f}ms")
                        return first_frame_delay
                    total_received += len(data)
            except socket.timeout:
                elapsed = (time.time() - server_start_time) * 1000
                print(f"[{elapsed:7.1f}ms] CLIENT: Timeout waiting for frame data")
                break
        
        return None
        
    except Exception as e:
        elapsed = (time.time() - server_start_time) * 1000
        print(f"[{elapsed:7.1f}ms] CLIENT ERROR: {e}")
        return None
        
    finally:
        # Clean up
        if client_socket:
            try:
                client_socket.close()
            except:
                pass
        
        # Collect any remaining server output
        time.sleep(0.5)
        try:
            while True:
                msg_type, timestamp, message = output_queue.get_nowait()
                elapsed = (timestamp - server_start_time) * 1000
                print(f"[{elapsed:7.1f}ms] SERVER: {message}")
        except queue.Empty:
            pass
        
        # Kill server
        try:
            os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
            server_process.wait(timeout=3)
        except:
            try:
                os.killpg(os.getpgid(server_process.pid), signal.SIGKILL)
            except:
                pass

def main():
    print("Testing server startup with detailed logging to identify slow startup causes...")
    print("This will run tests until we catch a slow startup (>5 seconds)")
    print("Press Ctrl+C to stop")
    
    test_count = 0
    delays = []
    
    try:
        while True:
            test_count += 1
            print(f"\n{'='*60}")
            print(f"TEST {test_count}")
            print(f"{'='*60}")
            
            delay = test_startup_with_logging()
            
            if delay is not None:
                delays.append(delay)
                print(f"\nTest {test_count} result: {delay:.1f}ms")
                
                if delay > 5000:
                    print(f"\n*** SLOW STARTUP DETECTED! {delay:.1f}ms ***")
                    print("Check the detailed logs above to identify the bottleneck!")
                    
                    if len(delays) > 1:
                        print(f"\nComparison with previous tests:")
                        for i, d in enumerate(delays[-5:], start=max(1, test_count-4)):
                            status = " <-- SLOW" if d > 5000 else ""
                            print(f"  Test {i}: {d:.1f}ms{status}")
                    
                    response = input("\nContinue testing for more slow startups? (y/n): ")
                    if response.lower() != 'y':
                        break
            else:
                print(f"\nTest {test_count} failed to measure delay")
            
            print(f"Waiting 2 seconds before next test...")
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\nStopping tests...")
    
    if delays:
        print(f"\n{'='*60}")
        print(f"SUMMARY AFTER {len(delays)} SUCCESSFUL TESTS")
        print(f"{'='*60}")
        print(f"Average delay: {sum(delays)/len(delays):.1f}ms")
        print(f"Max delay: {max(delays):.1f}ms")
        print(f"Min delay: {min(delays):.1f}ms")
        
        slow_startups = [d for d in delays if d > 5000]
        if slow_startups:
            print(f"Slow startups (>5s): {len(slow_startups)} out of {len(delays)}")
            print(f"Slow startup delays: {[f'{d:.1f}ms' for d in slow_startups]}")
        else:
            print("No slow startups detected - keep running to catch one!")

if __name__ == "__main__":
    main()
