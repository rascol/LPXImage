#!/usr/bin/env python3
# lpx_file_server.py - Stream video from a file using FileLPXServer with instant WASD response
import time
import threading
import signal
import sys
import os
import argparse
import select
import termios
import tty
import socket

try:
    import lpximage
except ModuleNotFoundError:
    print("ERROR: lpximage module not found!")
    print("Please ensure LPXImage is properly installed on this machine.")
    print("Refer to INSTALL_PYTHON.md in the LPXImage directory for installation instructions.")
    sys.exit(1)

# Global variables
server = None
current_x_offset = 0.0
current_y_offset = 0.0

# Define signal handler
def signal_handler(sig, frame):
    print("\nCtrl+C pressed, stopping server and exiting...")
    global server
    if server is not None:
        try:
            server.stop()
            print("Server stopped")
        except Exception as e:
            print(f"Error stopping server: {e}")
    
    # Exit the program
    print("Server exiting...")
    sys.exit(0)

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='LPX File Server - Stream video files with log-polar processing')
    parser.add_argument('--tables', default='../ScanTables63', help='Path to scan tables')
    parser.add_argument('--file', required=True, help='Path to video file')
    parser.add_argument('--width', type=int, default=1920, help='Output video width')
    parser.add_argument('--height', type=int, default=1080, help='Output video height')
    parser.add_argument('--port', type=int, default=5050, help='Server port')
    parser.add_argument('--saccade_port', type=int, default=5051, help='Port for saccade commands')
    parser.add_argument('--loop', action='store_true', help='Loop the video when it ends')
    parser.add_argument('--x_offset', type=int, default=0, help='X offset from center (positive = right)')
    parser.add_argument('--y_offset', type=int, default=0, help='Y offset from center (positive = down)')
    args = parser.parse_args()
    
    # Register signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    # Print startup info
    print(f"LPX File Server - Streaming video from file")
    print(f"Video file: {args.file}")
    print(f"Resolution: {args.width}x{args.height}")
    print(f"Scan Tables: {args.tables}")
    print(f"Port: {args.port}")
    print(f"Loop video: {'Yes' if args.loop else 'No'}")
    print("Press Ctrl+C to exit")
    
    # Create and start the LPX server
    global server, current_x_offset, current_y_offset
    current_x_offset = args.x_offset
    current_y_offset = args.y_offset
    
    try:
        # Initialize LPX processing
        if not lpximage.initLPX(args.tables, args.width, args.height):
            print("Failed to initialize LPX processing")
            return
        
        # Create FileLPXServer instance (correct constructor pattern)
        server = lpximage.FileLPXServer(args.tables, args.port)
        if args.loop:
            server.setLooping(True)
        
        # Set high FPS for responsive WASD input
        server.setFPS(60)  # Try 60 FPS for minimal delay
        print(f"Set server FPS to 60 for responsive input")
        
        server.setCenterOffset(current_x_offset, current_y_offset)
        
        # Start the server with video file and dimensions
        if not server.start(args.file, args.width, args.height):
            print("Failed to start server")
            return
        print(f"FileLPXServer started and listening on port {args.port}")
        print("Waiting for clients to connect...")
        
        print("\n=== WASD Movement Controls ===")
        print("Use WASD keys to control the log-polar transform center:")
        print("  W - Move up")
        print("  A - Move left")
        print("  S - Move down")
        print("  D - Move right")
        print("  R - Reset to center")
        print("  Q - Quit movement control")
        print("===========================\n")
        
        # Start WASD movement thread (same pattern as WebcamLPXServer)
        movement_thread = threading.Thread(target=handle_wasd_movement, args=(args.saccade_port,), daemon=True)
        movement_thread.start()
        
        # Main server loop (same pattern as WebcamLPXServer)
        while True:
            # Report status periodically
            client_count = server.getClientCount()
            if client_count > 0:
                print(f"Active clients: {client_count}")
            
            # Add a short delay to allow signal handling
            time.sleep(1)
            
    except Exception as e:
        print(f"Server error: {e}")
    finally:
        # Clean up if we exit the loop
        if server is not None:
            server.stop()
            print("Server stopped")

def handle_wasd_movement(saccade_port=None):
    """Handle WASD movement commands and saccade network commands (same as lpx_server.py)."""
    global current_x_offset, current_y_offset, server
    
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setcbreak(fd)

    # Setup saccade socket (non-blocking)
    saccade_sock = None
    if saccade_port:
        try:
            saccade_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            saccade_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            saccade_sock.bind(('127.0.0.1', saccade_port))
            saccade_sock.listen(1)
            saccade_sock.setblocking(False)
            print(f"Saccade commands available on port {saccade_port}")
        except:
            saccade_sock = None
            print(f"Warning: Could not bind saccade port {saccade_port}")

    move_map = {
        'w': (0, -10), 'a': (-10, 0), 's': (0, 10), 'd': (10, 0),
    }

    try:
        while True:
            # WASD handling (exactly like lpx_server.py)
            if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                key = sys.stdin.read(1)
                if key in move_map:
                    dx, dy = move_map[key]
                    current_x_offset += dx
                    current_y_offset += dy
                    # INSTANT: Call server.setCenterOffset() immediately!
                    server.setCenterOffset(current_x_offset, current_y_offset)
                    print(f"WASD Center: ({current_x_offset:.1f}, {current_y_offset:.1f})")
                elif key == 'q':
                    break
                elif key == 'r':
                    current_x_offset = current_y_offset = 0.0
                    server.setCenterOffset(current_x_offset, current_y_offset)
                    print("Reset to center (0, 0)")

            # Saccade commands (exactly like lpx_server.py)
            if saccade_sock:
                try:
                    conn, _ = saccade_sock.accept()
                    conn.setblocking(False)
                    try:
                        data = conn.recv(1024).decode().strip()
                        if data:
                            x_rel, y_rel = map(float, data.split(','))
                            current_x_offset += x_rel
                            current_y_offset += y_rel
                            # INSTANT: Call server.setCenterOffset() immediately!
                            server.setCenterOffset(current_x_offset, current_y_offset)
                            print(f"Saccade: ({x_rel:+.1f}, {y_rel:+.1f}) -> ({current_x_offset:.1f}, {current_y_offset:.1f})")
                    except:
                        pass
                    conn.close()
                except:
                    pass

            time.sleep(0.05)

    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        if saccade_sock:
            saccade_sock.close()

if __name__ == "__main__":
    main()
