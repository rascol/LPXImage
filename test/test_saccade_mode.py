#!/usr/bin/env python3
"""
Test script for visual saccade mode functionality with the file server.
This script tests both WASD keyboard controls and network saccade commands.
"""
import time
import threading
import socket
import subprocess
import signal
import sys
import os

def send_saccade_command(x_rel, y_rel, port=5053):
    """Send a saccade command to the server."""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', port))
        command = f"{x_rel},{y_rel}"
        sock.send(command.encode())
        sock.close()
        print(f"Sent saccade: ({x_rel:+.1f}, {y_rel:+.1f})")
        return True
    except Exception as e:
        print(f"Failed to send saccade command: {e}")
        return False

def test_saccade_sequence():
    """Test a sequence of saccade movements."""
    print("Testing saccade sequence...")
    
    # Wait for server to start
    time.sleep(2)
    
    # Test sequence: move in a square pattern
    saccade_moves = [
        (50, 0),   # Right
        (0, 50),   # Down  
        (-50, 0),  # Left
        (0, -50),  # Up (back to start)
        (0, 0),    # No movement (test)
    ]
    
    for i, (x_rel, y_rel) in enumerate(saccade_moves):
        print(f"Step {i+1}: Moving by ({x_rel:+.1f}, {y_rel:+.1f})")
        if send_saccade_command(x_rel, y_rel):
            time.sleep(2)  # Wait to see the movement
        else:
            print("Failed to send command, server may not be ready")
            return False
    
    # Test some rapid saccades
    print("\nTesting rapid saccades...")
    for i in range(5):
        x_rel = 20 if i % 2 == 0 else -20
        y_rel = 15 if i % 2 == 0 else -15
        send_saccade_command(x_rel, y_rel)
        time.sleep(0.5)  # Quick movements
    
    # Reset to center
    print("\nResetting to center...")
    # Calculate approximate return to center (accumulated: 0, 0 from the square)
    send_saccade_command(0, 0)
    
    return True

def main():
    print("=== LPX File Server Visual Saccade Mode Test ===")
    print("This script will:")
    print("1. Start the file server with saccade mode")
    print("2. Test programmatic saccade commands")
    print("3. You can also test WASD keys manually")
    print("")
    
    # Check if test video exists
    video_file = "test_video.mp4"
    if not os.path.exists(video_file):
        print(f"Test video '{video_file}' not found.")
        print("Creating a test video first...")
        
        # Try to run the create_test_video script
        try:
            subprocess.run([sys.executable, "create_test_video.py"], check=True)
        except subprocess.CalledProcessError:
            print("Failed to create test video. Please ensure test_video.mp4 exists.")
            return 1
    
    # Start the file server in a subprocess
    print(f"Starting file server with saccade mode...")
    server_cmd = [
        sys.executable, 
        "examples/lpx_file_server_clean.py",
        "--file", video_file,
        "--tables", "ScanTables63",
        "--width", "800",
        "--height", "600", 
        "--port", "5052",
        "--saccade_port", "5053",
        "--loop"
    ]
    
    print(f"Server command: {' '.join(server_cmd)}")
    
    try:
        # Set environment to include current directory in PYTHONPATH
        env = os.environ.copy()
        env['PYTHONPATH'] = '.'
        
        # Start server process
        server_process = subprocess.Popen(server_cmd, 
                                        stdout=subprocess.PIPE, 
                                        stderr=subprocess.STDOUT, 
                                        universal_newlines=True,
                                        env=env)
        
        # Start saccade test in a separate thread
        test_thread = threading.Thread(target=test_saccade_sequence, daemon=True)
        test_thread.start()
        
        print("\n=== Server Output ===")
        print("You can also test WASD keys in the server terminal:")
        print("W/A/S/D = move up/left/down/right")
        print("R = reset to center")
        print("Q = quit movement control")
        print("Ctrl+C = stop server")
        print("========================\n")
        
        # Monitor server output
        try:
            while True:
                output = server_process.stdout.readline()
                if output == '' and server_process.poll() is not None:
                    break
                if output:
                    print(f"[SERVER] {output.strip()}")
                time.sleep(0.1)
        except KeyboardInterrupt:
            print("\nStopping server...")
            server_process.terminate()
            server_process.wait()
            print("Server stopped.")
            
    except FileNotFoundError:
        print("Could not find lpx_file_server_clean.py")
        print("Please ensure you're running this from the LPXImage root directory")
        return 1
    except Exception as e:
        print(f"Error running server: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
