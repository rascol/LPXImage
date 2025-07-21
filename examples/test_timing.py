#!/usr/bin/env python3
# test_timing.py - Test timing improvements for client recognition
import lpximage
import time
import threading

def test_connection_timing():
    """Test how quickly a client can connect after server starts"""
    print("=== Testing Connection Timing ===")
    
    # Start server
    server = lpximage.FileLPXServer("../ScanTables63", 5053)
    video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
    
    print("Starting server...")
    start_time = time.time()
    server.start(video_path, 800, 600)
    server_start_time = time.time()
    
    # Give server a moment to fully initialize
    time.sleep(0.1)  # 100ms should be enough with our improvements
    
    print("Attempting immediate connection...")
    print("Creating LPXDebugClient...")
    client = lpximage.LPXDebugClient("../ScanTables63")
    print("Setting window size...")
    client.setWindowSize(400, 300)
    print("Initializing window...")
    client.initializeWindow()
    print("Window initialized!")
    
    connection_start = time.time()
    connected = client.connect("127.0.0.1:5053")
    connection_end = time.time()
    
    if connected:
        print(f"✓ Connection successful!")
        print(f"Server startup time: {(server_start_time - start_time)*1000:.1f}ms")
        print(f"Connection time: {(connection_end - connection_start)*1000:.1f}ms")
        
        # Test if we can get data quickly
        data_start = time.time()
        running_check = client.isRunning()
        if running_check:
            # Try to process events to see if data flows
            events_result = client.processEvents()
            data_end = time.time()
            print(f"Data flow time: {(data_end - data_start)*1000:.1f}ms")
            print(f"Events processed: {events_result}")
        
        client.disconnect()
    else:
        print("✗ Connection failed")
    
    server.stop()
    print("Test completed")

def test_multiple_rapid_connections():
    """Test multiple rapid connections to see if server can handle them"""
    print("\n=== Testing Multiple Rapid Connections ===")
    
    # Start server
    server = lpximage.FileLPXServer("../ScanTables63", 5054)
    video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
    
    print("Starting server...")
    server.start(video_path, 800, 600)
    time.sleep(0.1)  # Brief startup time
    
    # Try connecting multiple clients rapidly
    clients = []
    connection_times = []
    
    for i in range(3):
        print(f"Connecting client {i+1}...")
        client = lpximage.LPXDebugClient("../ScanTables63")
        client.setWindowTitle(f"LPX Client {i+1}")  # Unique window title
        client.setWindowSize(400, 300)
        client.initializeWindow()
        
        start = time.time()
        connected = client.connect(f"127.0.0.1:5054")
        end = time.time()
        
        if connected:
            connection_times.append((end - start) * 1000)
            clients.append(client)
            print(f"  ✓ Client {i+1} connected in {connection_times[-1]:.1f}ms")
        else:
            print(f"  ✗ Client {i+1} failed to connect")
        
        # Small delay between connections
        time.sleep(0.05)
    
    print(f"Connected {len(clients)} clients")
    if connection_times:
        avg_time = sum(connection_times) / len(connection_times)
        print(f"Average connection time: {avg_time:.1f}ms")
        print(f"Max connection time: {max(connection_times):.1f}ms")
        print(f"Min connection time: {min(connection_times):.1f}ms")
    
    # Disconnect all clients
    for i, client in enumerate(clients):
        client.disconnect()
        print(f"  Disconnected client {i+1}")
    
    server.stop()
    print("Multiple connection test completed")

if __name__ == "__main__":
    print("LPXImage Timing Test")
    print("=" * 40)
    
    # test_connection_timing()  # Skip this for now
    test_multiple_rapid_connections()
    
    print("\nAll timing tests completed!")
