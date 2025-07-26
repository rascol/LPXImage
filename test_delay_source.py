#!/usr/bin/env python3
"""
test_delay_source.py - Identify the exact source of startup delays
"""

import lpximage
import time
from datetime import datetime

def log_with_timestamp(message):
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] {message}")

def test_server_operations():
    """Test each server operation individually to identify delay source"""
    log_with_timestamp("=== Individual Server Operation Test ===")
    
    # Test server creation
    log_with_timestamp("Creating server...")
    create_start = time.time()
    server = lpximage.FileLPXServer("./ScanTables63", 5070)
    create_end = time.time()
    log_with_timestamp(f"Server created in {(create_end - create_start)*1000:.1f}ms")
    
    # Test server start
    log_with_timestamp("Starting server...")
    start_begin = time.time()
    start_result = server.start("./2342260-hd_1920_1080_30fps.mp4", 640, 480)
    start_end = time.time()
    log_with_timestamp(f"Server start completed in {(start_end - start_begin)*1000:.1f}ms, result: {start_result}")
    
    if not start_result:
        log_with_timestamp("Server failed to start - aborting test")
        return
    
    # Test getClientCount operation timing
    log_with_timestamp("Testing getClientCount() timing...")
    for i in range(5):
        count_start = time.time()
        client_count = server.getClientCount()
        count_end = time.time()
        count_time = (count_end - count_start) * 1000
        log_with_timestamp(f"getClientCount() call {i+1}: {count_time:.1f}ms, count: {client_count}")
        
        # Small delay between calls
        time.sleep(0.01)
    
    # Test multiple rapid getClientCount calls
    log_with_timestamp("Testing rapid getClientCount() calls...")
    rapid_start = time.time()
    for i in range(100):
        client_count = server.getClientCount()
    rapid_end = time.time()
    rapid_total = (rapid_end - rapid_start) * 1000
    log_with_timestamp(f"100 rapid getClientCount() calls: {rapid_total:.1f}ms total, {rapid_total/100:.2f}ms avg")
    
    # Test server stop
    log_with_timestamp("Stopping server...")
    stop_start = time.time()
    server.stop()
    stop_end = time.time()
    log_with_timestamp(f"Server stopped in {(stop_end - stop_start)*1000:.1f}ms")

def test_client_creation_timing():
    """Test client creation timing variations"""
    log_with_timestamp("=== Client Creation Timing Test ===")
    
    creation_times = []
    
    for i in range(5):
        log_with_timestamp(f"Creating client {i+1}/5...")
        
        create_start = time.time()
        client = lpximage.LPXDebugClient("./ScanTables63")
        create_end = time.time()
        create_time = (create_end - create_start) * 1000
        creation_times.append(create_time)
        
        log_with_timestamp(f"Client {i+1} created in {create_time:.1f}ms")
        
        # Configure client
        config_start = time.time()
        client.setWindowSize(400, 300)
        client.initializeWindow()
        config_end = time.time()
        config_time = (config_end - config_start) * 1000
        
        log_with_timestamp(f"Client {i+1} configured in {config_time:.1f}ms")
        
        # Small delay
        time.sleep(0.1)
    
    if creation_times:
        avg_time = sum(creation_times) / len(creation_times)
        min_time = min(creation_times)
        max_time = max(creation_times)
        variance = max_time - min_time
        
        log_with_timestamp(f"Client creation stats:")
        log_with_timestamp(f"  Average: {avg_time:.1f}ms")
        log_with_timestamp(f"  Min: {min_time:.1f}ms")
        log_with_timestamp(f"  Max: {max_time:.1f}ms")
        log_with_timestamp(f"  Variance: {variance:.1f}ms")

def test_server_client_interaction():
    """Test server-client interaction timing"""
    log_with_timestamp("=== Server-Client Interaction Test ===")
    
    # Start server
    log_with_timestamp("Starting server for interaction test...")
    server = lpximage.FileLPXServer("./ScanTables63", 5071)
    if not server.start("./2342260-hd_1920_1080_30fps.mp4", 640, 480):
        log_with_timestamp("Failed to start server for interaction test")
        return
    
    log_with_timestamp("Server started, waiting 100ms for stabilization...")
    time.sleep(0.1)
    
    # Create client
    log_with_timestamp("Creating client for interaction test...")
    client = lpximage.LPXDebugClient("./ScanTables63")
    client.setWindowSize(400, 300)
    client.initializeWindow()
    
    # Test connection timing
    log_with_timestamp("Attempting connection...")
    connect_start = time.time()
    connected = client.connect("127.0.0.1:5071")
    connect_end = time.time()
    connect_time = (connect_end - connect_start) * 1000
    
    if connected:
        log_with_timestamp(f"Connection successful in {connect_time:.1f}ms")
        
        # Test immediate data availability
        log_with_timestamp("Testing immediate data availability...")
        data_start = time.time()
        data_available = False
        
        for attempt in range(10):  # Try 10 times with 10ms intervals
            if client.isRunning():
                if client.processEvents():
                    data_end = time.time()
                    data_time = (data_end - data_start) * 1000
                    log_with_timestamp(f"Data available after {data_time:.1f}ms (attempt {attempt+1})")
                    data_available = True
                    break
            time.sleep(0.01)  # 10ms delay
        
        if not data_available:
            log_with_timestamp("No data received after 100ms")
        
        # Check server client count
        server_client_count = server.getClientCount()
        log_with_timestamp(f"Server reports {server_client_count} clients")
        
        client.disconnect()
        log_with_timestamp("Client disconnected")
    else:
        log_with_timestamp(f"Connection failed after {connect_time:.1f}ms")
    
    server.stop()
    log_with_timestamp("Server stopped")

def main():
    log_with_timestamp("Starting delay source identification test")
    log_with_timestamp("=" * 50)
    
    # Test individual server operations
    test_server_operations()
    
    log_with_timestamp("")
    
    # Test client creation timing
    test_client_creation_timing()
    
    log_with_timestamp("")
    
    # Test server-client interaction
    test_server_client_interaction()
    
    log_with_timestamp("=" * 50)
    log_with_timestamp("Delay source test completed")

if __name__ == "__main__":
    main()
