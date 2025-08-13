#!/usr/bin/env python3
"""
Test optimized file server performance with client connection
"""

import os
import sys
import time
import threading
import socket
import struct

# Set up the Python path to use the local module
sys.path.insert(0, os.getcwd())
os.environ['PYTHONPATH'] = os.getcwd()

import lpximage

class SimpleClient:
    def __init__(self):
        self.sock = None
        self.connected = False
        self.frames_received = 0
        self.first_frame_time = None
        self.last_frame_time = None
    
    def connect(self, host='localhost', port=5055):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((host, port))
            self.connected = True
            print(f"Connected to server at {host}:{port}")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False
    
    def receive_frame(self):
        if not self.connected:
            return None
        
        try:
            # Read frame header (8 bytes: length + timestamp)
            header_data = self.sock.recv(8)
            if len(header_data) != 8:
                return None
            
            length, timestamp = struct.unpack('<II', header_data)
            
            # Read frame data
            frame_data = b''
            while len(frame_data) < length:
                chunk = self.sock.recv(length - len(frame_data))
                if not chunk:
                    return None
                frame_data += chunk
            
            current_time = time.time()
            
            if self.first_frame_time is None:
                self.first_frame_time = current_time
                print(f"✓ First frame received! Size: {length} bytes")
            
            self.frames_received += 1
            self.last_frame_time = current_time
            
            # Show progress every 10 frames
            if self.frames_received % 10 == 0:
                elapsed = current_time - self.first_frame_time if self.first_frame_time else 0
                fps = self.frames_received / elapsed if elapsed > 0 else 0
                print(f"Frame {self.frames_received}: {length} bytes, {fps:.1f} FPS avg")
            
            return frame_data
            
        except Exception as e:
            print(f"Error receiving frame: {e}")
            return None
    
    def disconnect(self):
        if self.sock:
            self.sock.close()
            self.connected = False

def test_optimized_performance():
    print("=== Testing Optimized File Server Performance ===")
    
    # Start file server
    print("Starting optimized file server...")
    server = lpximage.FileLPXServer('./ScanTables63', 5055)
    
    # Start server with test video
    if not server.start('./test_video.mp4', 1920, 1080):
        print("Failed to start server")
        return
    
    print("Server started successfully")
    
    # Give server time to initialize and preload frames
    print("Waiting for server initialization...")
    time.sleep(2.0)
    
    # Connect client and measure frame delivery
    client = SimpleClient()
    start_time = time.time()
    
    if not client.connect():
        print("Failed to connect client")
        server.stop()
        return
    
    connect_time = time.time()
    print(f"Client connected in {(connect_time - start_time)*1000:.1f}ms")
    
    # Receive frames for 10 seconds
    print("Receiving frames...")
    test_duration = 10.0
    end_time = time.time() + test_duration
    
    try:
        while time.time() < end_time:
            frame_data = client.receive_frame()
            if frame_data is None:
                print("Connection lost or end of video")
                break
                
            # Small delay to prevent overwhelming
            time.sleep(0.01)
    
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    
    # Calculate results
    total_time = time.time() - connect_time
    print(f"\n=== Performance Results ===")
    print(f"Test duration: {total_time:.2f}s")
    print(f"Frames received: {client.frames_received}")
    
    if client.first_frame_time:
        first_frame_delay = (client.first_frame_time - connect_time) * 1000
        print(f"First frame delay: {first_frame_delay:.1f}ms")
    
    if total_time > 0:
        avg_fps = client.frames_received / total_time
        print(f"Average FPS: {avg_fps:.1f}")
    
    # Cleanup
    client.disconnect()
    server.stop()
    print("Test completed")

if __name__ == "__main__":
    test_optimized_performance()
