#!/usr/bin/env python3
"""
Test script to verify the BGR color channel consistency fix.
This will run the file server, process a colorful test image, 
and check if the colors are now correctly represented.
"""

import subprocess
import time
import signal
import sys
import os
import cv2
import numpy as np
import socket
import struct

def create_test_color_image(filename="test_color_stripes.jpg"):
    """Create a test image with clear color stripes for easy verification"""
    width, height = 640, 480
    
    # Create image with BGR color stripes
    img = np.zeros((height, width, 3), dtype=np.uint8)
    
    stripe_width = width // 6
    
    # Red stripe (BGR: 0, 0, 255)
    img[:, 0:stripe_width] = [0, 0, 255]
    
    # Green stripe (BGR: 0, 255, 0)  
    img[:, stripe_width:2*stripe_width] = [0, 255, 0]
    
    # Blue stripe (BGR: 255, 0, 0)
    img[:, 2*stripe_width:3*stripe_width] = [255, 0, 0]
    
    # Yellow stripe (BGR: 0, 255, 255)
    img[:, 3*stripe_width:4*stripe_width] = [0, 255, 255]
    
    # Magenta stripe (BGR: 255, 0, 255)
    img[:, 4*stripe_width:5*stripe_width] = [255, 0, 255]
    
    # Cyan stripe (BGR: 255, 255, 0)
    img[:, 5*stripe_width:6*stripe_width] = [255, 255, 0]
    
    cv2.imwrite(filename, img)
    print(f"Created test color image: {filename}")
    return filename

def receive_lpx_data(sock):
    """Receive LPX data from the file server"""
    try:
        # Read total size first
        size_data = sock.recv(4)
        if len(size_data) != 4:
            return None
            
        total_size = struct.unpack('I', size_data)[0]
        print(f"[CLIENT] Expecting {total_size} bytes of LPX data")
        
        # Read header (8 ints = 32 bytes)
        header_size = 32
        header_data = sock.recv(header_size)
        if len(header_data) != header_size:
            return None
            
        header = struct.unpack('8I', header_data)
        length, nMaxCells, spiralPer, width, height, x_ofs, y_ofs, reserved = header
        
        print(f"[CLIENT] Header: length={length}, nMaxCells={nMaxCells}, "
              f"spiralPer={spiralPer}, size={width}x{height}")
        
        # Read cell data
        data_size = total_size - 4 - header_size  # Subtract size field and header
        expected_data_size = length * 4  # 4 bytes per uint32_t cell
        
        print(f"[CLIENT] Reading {data_size} bytes of cell data (expected: {expected_data_size})")
        
        cell_data = b''
        while len(cell_data) < data_size:
            chunk = sock.recv(min(4096, data_size - len(cell_data)))
            if not chunk:
                break
            cell_data += chunk
            
        if len(cell_data) == data_size:
            print(f"[CLIENT] Successfully received LPX data: {len(cell_data)} bytes")
            # Unpack first few cells to check colors
            cells = struct.unpack(f'{length}I', cell_data[:length*4])
            
            # Check first 20 cells for color values
            print("[CLIENT] First 20 cell colors (BGR packed):")
            for i in range(min(20, len(cells))):
                cell = cells[i]
                b = cell & 0xFF
                g = (cell >> 8) & 0xFF
                r = (cell >> 16) & 0xFF
                print(f"  Cell {i:2d}: 0x{cell:08X} -> R={r:3d}, G={g:3d}, B={b:3d}")
            
            return cell_data
        else:
            print(f"[CLIENT] Failed to receive complete data: {len(cell_data)}/{data_size}")
            return None
            
    except Exception as e:
        print(f"[CLIENT] Error receiving LPX data: {e}")
        return None

def test_file_server():
    """Test the file server with color stripe image"""
    
    # Create test image
    test_image = create_test_color_image()
    
    # Start the file server
    server_process = None
    try:
        print("[TEST] Starting file server...")
        server_process = subprocess.Popen([
            "/Users/ray/Desktop/LPXImage/build/main_file_server",
            "/Users/ray/Desktop/LPXImage/ScanTables63",
            "/Users/ray/Desktop/LPXImage/data/video",
            "5050"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        
        # Give the server time to start
        time.sleep(2)
        
        # Copy test image to video directory for processing
        video_dir = "/Users/ray/Desktop/LPXImage/data/video"
        os.makedirs(video_dir, exist_ok=True)
        
        # Copy the test image with a frame number
        test_frame = os.path.join(video_dir, "frame_0001.jpg")
        subprocess.run(["cp", test_image, test_frame], check=True)
        print(f"[TEST] Copied test image to {test_frame}")
        
        # Connect to server and receive data
        print("[TEST] Connecting to file server...")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect(('localhost', 5050))
            print("[TEST] Connected to file server")
            
            # Receive LPX data
            lpx_data = receive_lpx_data(sock)
            
            if lpx_data:
                print("[TEST] ✅ Successfully received LPX data from file server")
                print("[TEST] Color channel test completed - check the cell color values above")
                print("[TEST] If BGR format is consistent, you should see:")
                print("[TEST] - Red regions: R=high, G=0, B=0")
                print("[TEST] - Green regions: R=0, G=high, B=0") 
                print("[TEST] - Blue regions: R=0, G=0, B=high")
            else:
                print("[TEST] ❌ Failed to receive LPX data")
                
    except KeyboardInterrupt:
        print("\n[TEST] Interrupted by user")
    except Exception as e:
        print(f"[TEST] Error: {e}")
    finally:
        # Clean up
        if server_process:
            print("[TEST] Stopping file server...")
            server_process.terminate()
            try:
                server_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                server_process.kill()
                server_process.wait()
        
        # Clean up test files
        if os.path.exists(test_image):
            os.remove(test_image)
        
        test_frame = "/Users/ray/Desktop/LPXImage/data/video/frame_0001.jpg"
        if os.path.exists(test_frame):
            os.remove(test_frame)

if __name__ == "__main__":
    test_file_server()
