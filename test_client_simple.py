#!/usr/bin/env python3
"""
Simple client to connect to the file server and verify color data
"""
import socket
import struct
import time
import subprocess
import threading

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
            # Unpack cells - use the actual data size, not expected
            num_cells = len(cell_data) // 4  # 4 bytes per uint32_t
            cells = struct.unpack(f'{num_cells}I', cell_data[:num_cells*4])
            
            # Check first 20 cells for color values - sampling different regions
            print("[CLIENT] Color samples from LPX cells (BGR packed):")
            sample_indices = [0, 1, 2, 10, 50, 100, 500, 1000, 5000, 10000] if length > 10000 else range(min(10, length))
            
            for i in sample_indices:
                if i >= len(cells):
                    continue
                cell = cells[i]
                b = cell & 0xFF
                g = (cell >> 8) & 0xFF
                r = (cell >> 16) & 0xFF
                print(f"  Cell {i:5d}: 0x{cell:08X} -> R={r:3d}, G={g:3d}, B={b:3d}")
            
            return cell_data
        else:
            print(f"[CLIENT] Failed to receive complete data: {len(cell_data)}/{data_size}")
            return None
            
    except Exception as e:
        print(f"[CLIENT] Error receiving LPX data: {e}")
        return None

def main():
    """Test the file server with a simple client"""
    
    print("[TEST] Testing file server with simple client...")
    
    # Start the file server in background
    server_process = subprocess.Popen([
        "/Users/ray/Desktop/LPXImage/build/main_file_server",
        "/Users/ray/Desktop/LPXImage/ScanTables63",
        "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4",
        "5050"
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    try:
        # Wait for server to start
        time.sleep(3)
        
        print("[CLIENT] Connecting to file server...")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect(('localhost', 5050))
            print("[CLIENT] Connected successfully!")
            
            # Receive a few frames to check color consistency
            for frame_num in range(3):
                print(f"\n[CLIENT] === Receiving frame {frame_num + 1} ===")
                lpx_data = receive_lpx_data(sock)
                
                if lpx_data:
                    print(f"[CLIENT] ✅ Frame {frame_num + 1} received successfully")
                else:
                    print(f"[CLIENT] ❌ Failed to receive frame {frame_num + 1}")
                    break
                    
                time.sleep(0.5)  # Brief pause between frames
            
            print("\n[TEST] ✅ Color channel fix test completed!")
            print("[TEST] Check the color values above - they should now show consistent BGR format")
                
    except Exception as e:
        print(f"[TEST] Error: {e}")
    finally:
        # Clean up server
        server_process.terminate()
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_process.kill()
            server_process.wait()

if __name__ == "__main__":
    main()
