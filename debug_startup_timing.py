#!/usr/bin/env python3
"""
debug_startup_timing.py - Diagnostic tool for LPXImage startup timing issues

This tool analyzes the variable delay in startup image synchronization by:
1. Testing server startup timing consistency
2. Measuring client connection time variations
3. Monitoring network/socket behavior
4. Analyzing video loading/processing delays
5. Testing thread synchronization issues

The goal is to identify why sometimes images appear immediately while other
times it takes several minutes.
"""

import lpximage
import time
import threading
import subprocess
import socket
import sys
import os
from datetime import datetime

class StartupTimingDiagnostic:
    def __init__(self):
        self.results = []
        self.video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
        self.scan_tables = "./ScanTables63"
        
    def log(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        full_message = f"[{timestamp}] {message}"
        print(full_message)
        self.results.append(full_message)
    
    def check_system_resources(self):
        """Check system resources that might affect timing"""
        self.log("=== System Resource Check ===")
        
        # Check available memory
        try:
            result = subprocess.run(['vm_stat'], capture_output=True, text=True)
            if result.returncode == 0:
                lines = result.stdout.split('\n')
                for line in lines[:5]:  # First few lines have key info
                    if line.strip():
                        self.log(f"Memory: {line.strip()}")
        except:
            self.log("Could not check memory stats")
        
        # Check CPU load
        try:
            result = subprocess.run(['uptime'], capture_output=True, text=True)
            if result.returncode == 0:
                self.log(f"Load: {result.stdout.strip()}")
        except:
            self.log("Could not check system load")
    
    def test_port_availability(self, port):
        """Test if port is available and responsive"""
        self.log(f"Testing port {port} availability...")
        
        # Check if port is in use
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1.0)
        try:
            result = sock.connect_ex(('127.0.0.1', port))
            if result == 0:
                self.log(f"Port {port} is already in use")
                return False
            else:
                self.log(f"Port {port} is available")
                return True
        except Exception as e:
            self.log(f"Port test error: {e}")
            return False
        finally:
            sock.close()
    
    def test_video_file_access(self):
        """Test video file accessibility and properties"""
        self.log("=== Video File Access Test ===")
        
        if not os.path.exists(self.video_path):
            self.log(f"ERROR: Video file not found: {self.video_path}")
            return False
        
        # Check file size and access time
        try:
            stat = os.stat(self.video_path)
            self.log(f"Video file size: {stat.st_size / (1024*1024):.1f} MB")
            
            # Test read access timing
            start_time = time.time()
            with open(self.video_path, 'rb') as f:
                # Read first 1MB to test disk access speed
                data = f.read(1024 * 1024)
            end_time = time.time()
            
            self.log(f"Disk read test (1MB): {(end_time - start_time)*1000:.1f}ms")
            return True
            
        except Exception as e:
            self.log(f"Video file access error: {e}")
            return False
    
    def test_server_startup_timing(self, port, iterations=5):
        """Test server startup timing consistency"""
        self.log(f"=== Server Startup Timing Test ({iterations} iterations) ===")
        
        startup_times = []
        
        for i in range(iterations):
            self.log(f"--- Iteration {i+1}/{iterations} ---")
            
            # Ensure port is free
            if not self.test_port_availability(port):
                self.log(f"Port {port} not available for iteration {i+1}")
                port += 1
                continue
            
            try:
                # Measure server creation time
                create_start = time.time()
                server = lpximage.FileLPXServer(self.scan_tables, port)
                create_end = time.time()
                create_time = (create_end - create_start) * 1000
                
                # Measure server start time
                start_time = time.time()
                start_result = server.start(self.video_path, 640, 480)
                end_time = time.time()
                startup_time = (end_time - start_time) * 1000
                
                if start_result:
                    self.log(f"Server create time: {create_time:.1f}ms")
                    self.log(f"Server start time: {startup_time:.1f}ms")
                    startup_times.append(startup_time)
                    
                    # Test immediate responsiveness
                    time.sleep(0.1)  # Brief settling time
                    client_count = server.getClientCount()
                    self.log(f"Initial client count: {client_count}")
                    
                    # Stop the server
                    stop_start = time.time()
                    server.stop()
                    stop_end = time.time()
                    stop_time = (stop_end - stop_start) * 1000
                    self.log(f"Server stop time: {stop_time:.1f}ms")
                    
                else:
                    self.log(f"Server failed to start on iteration {i+1}")
                
            except Exception as e:
                self.log(f"Server test error on iteration {i+1}: {e}")
            
            # Wait between iterations
            if i < iterations - 1:
                time.sleep(1.0)
            port += 1
        
        if startup_times:
            avg_time = sum(startup_times) / len(startup_times)
            min_time = min(startup_times)
            max_time = max(startup_times)
            variance = max_time - min_time
            
            self.log(f"Startup time statistics:")
            self.log(f"  Average: {avg_time:.1f}ms")
            self.log(f"  Min: {min_time:.1f}ms")
            self.log(f"  Max: {max_time:.1f}ms")
            self.log(f"  Variance: {variance:.1f}ms")
            
            if variance > 1000:  # More than 1 second variance
                self.log("WARNING: High variance in startup times detected!")
        
        return startup_times
    
    def test_client_connection_timing(self, port, iterations=5):
        """Test client connection timing consistency"""
        self.log(f"=== Client Connection Timing Test ({iterations} iterations) ===")
        
        connection_times = []
        first_data_times = []
        
        # Start a server for testing
        server = lpximage.FileLPXServer(self.scan_tables, port)
        if not server.start(self.video_path, 640, 480):
            self.log("Failed to start server for client tests")
            return
        
        time.sleep(0.5)  # Let server stabilize
        
        for i in range(iterations):
            self.log(f"--- Client Test {i+1}/{iterations} ---")
            
            try:
                # Create client
                client_create_start = time.time()
                client = lpximage.LPXDebugClient(self.scan_tables)
                client.setWindowSize(400, 300)
                client.initializeWindow()
                client_create_end = time.time()
                create_time = (client_create_end - client_create_start) * 1000
                
                # Connect to server
                connect_start = time.time()
                connected = client.connect(f"127.0.0.1:{port}")
                connect_end = time.time()
                connect_time = (connect_end - connect_start) * 1000
                
                if connected:
                    self.log(f"Client create time: {create_time:.1f}ms")
                    self.log(f"Connection time: {connect_time:.1f}ms")
                    connection_times.append(connect_time)
                    
                    # Test time to first data
                    data_start = time.time()
                    data_received = False
                    
                    # Try for up to 5 seconds to receive data
                    timeout = 5.0
                    while (time.time() - data_start) < timeout:
                        if client.isRunning():
                            if client.processEvents():
                                data_end = time.time()
                                first_data_time = (data_end - data_start) * 1000
                                self.log(f"First data time: {first_data_time:.1f}ms")
                                first_data_times.append(first_data_time)
                                data_received = True
                                break
                        time.sleep(0.01)  # 10ms polling
                    
                    if not data_received:
                        self.log(f"No data received within {timeout}s timeout")
                    
                    client.disconnect()
                else:
                    self.log(f"Failed to connect on iteration {i+1}")
                
            except Exception as e:
                self.log(f"Client test error on iteration {i+1}: {e}")
            
            # Wait between iterations
            if i < iterations - 1:
                time.sleep(0.5)
        
        server.stop()
        
        # Analyze results
        if connection_times:
            avg_connect = sum(connection_times) / len(connection_times)
            self.log(f"Connection time stats: avg={avg_connect:.1f}ms, "
                    f"min={min(connection_times):.1f}ms, max={max(connection_times):.1f}ms")
        
        if first_data_times:
            avg_data = sum(first_data_times) / len(first_data_times)
            self.log(f"First data time stats: avg={avg_data:.1f}ms, "
                    f"min={min(first_data_times):.1f}ms, max={max(first_data_times):.1f}ms")
    
    def test_threading_issues(self):
        """Test for potential threading/synchronization issues"""
        self.log("=== Threading Issue Test ===")
        
        # Test with multiple rapid server/client cycles
        port = 5060
        
        def server_thread(port_num):
            try:
                server = lpximage.FileLPXServer(self.scan_tables, port_num)
                success = server.start(self.video_path, 640, 480)
                self.log(f"Server on port {port_num}: {'Started' if success else 'Failed'}")
                if success:
                    time.sleep(2)
                    server.stop()
                    self.log(f"Server on port {port_num}: Stopped")
            except Exception as e:
                self.log(f"Server thread error on port {port_num}: {e}")
        
        # Start multiple servers simultaneously
        threads = []
        for i in range(3):
            thread = threading.Thread(target=server_thread, args=(port + i,))
            threads.append(thread)
            thread.start()
        
        # Wait for all threads
        for thread in threads:
            thread.join()
        
        self.log("Threading test completed")
    
    def run_full_diagnostic(self):
        """Run all diagnostic tests"""
        self.log("Starting LPXImage Startup Timing Diagnostic")
        self.log("=" * 50)
        
        # System checks
        self.check_system_resources()
        
        # File access test
        if not self.test_video_file_access():
            self.log("CRITICAL: Video file access failed - aborting tests")
            return
        
        # Server timing tests
        self.test_server_startup_timing(5050, 3)
        
        # Client timing tests  
        self.test_client_connection_timing(5056, 3)
        
        # Threading tests
        self.test_threading_issues()
        
        self.log("=" * 50)
        self.log("Diagnostic completed")
        
        # Save results to file
        try:
            with open('startup_timing_diagnostic.log', 'w') as f:
                for result in self.results:
                    f.write(result + '\n')
            self.log("Results saved to startup_timing_diagnostic.log")
        except Exception as e:
            self.log(f"Could not save results: {e}")

if __name__ == "__main__":
    # Check if video file exists
    video_path = "/Users/ray/Desktop/LPXImage/2342260-hd_1920_1080_30fps.mp4"
    if not os.path.exists(video_path):
        print(f"ERROR: Video file not found: {video_path}")
        print("Please ensure the video file exists or update the path in the script")
        sys.exit(1)
    
    diagnostic = StartupTimingDiagnostic()
    diagnostic.run_full_diagnostic()
