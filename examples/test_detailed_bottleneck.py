#!/usr/bin/env python3
"""
test_detailed_bottleneck.py - Detailed analysis of LPX processing bottleneck

This will test each component in isolation:
1. Scan table loading time
2. Video file opening time  
3. First LPX image generation time
4. Subsequent LPX image generation times
"""

import subprocess
import time
import os
import sys
from datetime import datetime

def log_message(message):
    """Log with timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    full_message = f"[{timestamp}] {message}"
    print(full_message)

def test_component_timing():
    """Test individual components to find the bottleneck"""
    
    log_message("=" * 60)
    log_message("LPXImage Detailed Bottleneck Analysis")
    log_message("=" * 60)
    
    # Test multiple runs to see timing variability
    num_runs = 3
    
    for run in range(1, num_runs + 1):
        log_message(f"=== RUN {run}/{num_runs} ===")
        
        # Start server and capture all output to analyze timing
        server_process = subprocess.Popen([
            "python3", "lpx_file_server.py",
            "--file", "../2342260-hd_1920_1080_30fps.mp4",
            "--width", "640", "--height", "480"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        
        server_start_time = time.time()
        log_message(f"Server process started (PID: {server_process.pid})")
        
        # Monitor server startup to capture timing information
        server_ready = False
        startup_timeout = 15  # 15 seconds max for server startup
        
        timing_events = []
        
        start_monitor = time.time()
        while (time.time() - start_monitor) < startup_timeout:
            # Check if process is still running
            if server_process.poll() is not None:
                log_message("ERROR: Server process exited unexpectedly!")
                break
            
            # Try to read server output
            try:
                line = server_process.stdout.readline()
                if line:
                    line_clean = line.strip()
                    log_message(f"SERVER: {line_clean}")
                    
                    # Look for timing information
                    if "[TIMING]" in line_clean:
                        timing_events.append(line_clean)
                        log_message(f"🔍 TIMING: {line_clean}")
                    
                    # Look for specific events
                    if "Scan table loading took:" in line_clean:
                        log_message(f"📊 SCAN TABLE LOAD: {line_clean}")
                    
                    if "Video file opening took:" in line_clean:
                        log_message(f"🎥 VIDEO OPEN: {line_clean}")
                    
                    if "LPX processing took:" in line_clean:
                        log_message(f"⚡ LPX PROCESS: {line_clean}")
                    
                    if "FileLPXServer started on port" in line_clean:
                        server_ready = True
                        server_ready_time = time.time() - server_start_time
                        log_message(f"✅ SERVER READY after {server_ready_time*1000:.1f}ms")
                        break
            except:
                pass
            
            time.sleep(0.01)
        
        if not server_ready:
            log_message("⚠️  Server ready signal not detected")
        
        # Wait a moment for server stabilization
        time.sleep(1)
        
        # Start client and measure connection + first frame
        client_start_time = time.time()
        
        client_process = subprocess.Popen([
            "python3", "lpx_renderer_simple.py"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        
        log_message(f"Client process started (PID: {client_process.pid})")
        
        # Monitor client output for timing
        connection_time = None
        first_frame_time = None
        client_timeout = 20  # 20 seconds max
        
        start_monitor = time.time()
        while (time.time() - start_monitor) < client_timeout:
            if client_process.poll() is not None:
                break
            
            try:
                line = client_process.stdout.readline()
                if line:
                    line_clean = line.strip()
                    
                    if "Connected to LPX server" in line_clean:
                        connection_time = time.time() - client_start_time
                        log_message(f"🔗 CONNECTION: {connection_time*1000:.1f}ms")
                    
                    elif "Successfully rendered image" in line_clean or "Image ready for display" in line_clean:
                        first_frame_time = time.time() - client_start_time
                        log_message(f"🖼️  FIRST FRAME: {first_frame_time*1000:.1f}ms")
                        break
            except:
                pass
            
            time.sleep(0.01)
        
        # Cleanup
        try:
            client_process.terminate()
            client_process.wait(timeout=2)
        except:
            try:
                client_process.kill()
            except:
                pass
        
        try:
            server_process.terminate()
            server_process.wait(timeout=2)
        except:
            try:
                server_process.kill()
            except:
                pass
        
        log_message(f"=== END RUN {run} ===")
        log_message("")
        
        # Wait between runs
        if run < num_runs:
            time.sleep(2)
    
    log_message("=" * 60)
    log_message("ANALYSIS COMPLETE")
    log_message("Look for patterns in the timing data above.")
    log_message("Key things to check:")
    log_message("1. How long does 'Scan table loading' take?")
    log_message("2. How long does 'Video file opening' take?") 
    log_message("3. How long until 'SERVER READY'?")
    log_message("4. How long for first 'LPX processing'?")
    log_message("5. How long for 'CONNECTION' and 'FIRST FRAME'?")
    log_message("")
    log_message("If scan table loading is >1000ms, that's the bottleneck.")
    log_message("If video opening is >1000ms, that's the bottleneck.")
    log_message("If LPX processing is >500ms, that's the bottleneck.")
    log_message("=" * 60)

if __name__ == "__main__":
    test_component_timing()
