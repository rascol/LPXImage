#!/usr/bin/env python3
"""
test_startup_delays.py - Integrated test to identify startup delays

This script will:
1. Start the fixed server
2. Wait a moment for it to initialize
3. Start a client and measure connection timing
4. Report all timing information clearly
5. Clean up everything

All output goes to both console and a single clear log file.
"""

import subprocess
import time
import os
import signal
import sys
from datetime import datetime

# Global process tracking
server_process = None
client_process = None

def log_message(message):
    """Log with timestamp to both console and file"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    full_message = f"[{timestamp}] {message}"
    print(full_message)
    
    # Also write to a clear log file
    with open("current_test_log.txt", "a") as f:
        f.write(full_message + "\n")

def cleanup():
    """Clean up any running processes"""
    global server_process, client_process
    
    log_message("Cleaning up processes...")
    
    if client_process:
        try:
            client_process.terminate()
            client_process.wait(timeout=5)
            log_message("Client process terminated")
        except:
            try:
                client_process.kill()
                log_message("Client process killed")
            except:
                pass
    
    if server_process:
        try:
            server_process.terminate()
            server_process.wait(timeout=5)
            log_message("Server process terminated")
        except:
            try:
                server_process.kill()
                log_message("Server process killed")
            except:
                pass

def signal_handler(sig, frame):
    log_message("Interrupt received, cleaning up...")
    cleanup()
    sys.exit(0)

def test_startup_timing():
    global server_process, client_process
    
    # Clear previous log
    if os.path.exists("current_test_log.txt"):
        os.remove("current_test_log.txt")
    
    log_message("=" * 60)
    log_message("LPXImage Startup Delay Investigation")
    log_message("=" * 60)
    
    # Register signal handler
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        # Step 1: Start the fixed server
        log_message("Step 1: Starting the fixed server...")
        server_start_time = time.time()
        
        server_process = subprocess.Popen([
            "python3", "lpx_file_server.py",
            "--file", "../2342260-hd_1920_1080_30fps.mp4",
            "--width", "640", "--height", "480"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env={**os.environ, "TIMING_DIAGNOSTICS": "1"})
        
        log_message(f"Server process started (PID: {server_process.pid})")
        
        # Monitor server startup output for a few seconds
        log_message("Monitoring server startup output...")
        server_ready = False
        startup_timeout = 10  # 10 seconds max for server startup
        
        start_monitor = time.time()
        while (time.time() - start_monitor) < startup_timeout:
            # Check if process is still running
            if server_process.poll() is not None:
                log_message("ERROR: Server process exited unexpectedly!")
                stdout, stderr = server_process.communicate()
                log_message(f"Server output: {stdout}")
                return
            
            # Try to read some output
            try:
                server_process.stdout.settimeout(0.1)
                line = server_process.stdout.readline()
                if line:
                    line_clean = line.strip()
                    log_message(f"SERVER: {line_clean}")
                    
                    # Look for timing diagnostics from instrumented C++ code
                    if "TIMING:" in line_clean:
                        log_message(f"🔍 TIMING DIAGNOSTIC: {line_clean}")
                    
                    # Look for slow operations (over 10ms)
                    if "SLOW" in line_clean and "ms" in line_clean:
                        log_message(f"⚠️  SLOW OPERATION DETECTED: {line_clean}")
                    
                    if "Server started successfully" in line_clean or "ready for client connections" in line_clean:
                        server_ready = True
                        break
            except:
                pass
            
            time.sleep(0.1)
        
        server_startup_time = time.time() - server_start_time
        log_message(f"Server startup took: {server_startup_time*1000:.1f}ms")
        
        if not server_ready:
            log_message("WARNING: Server ready signal not detected, proceeding anyway")
        else:
            log_message("✓ Server appears to be ready")
        
        # Step 2: Wait a moment for server to fully initialize
        log_message("Step 2: Waiting 2 seconds for server stabilization...")
        time.sleep(2)
        
        # Step 3: Start client and measure connection timing
        log_message("Step 3: Starting client and measuring connection timing...")
        
        client_start_time = time.time()
        
        # Use the simple renderer for clean timing measurement
        client_process = subprocess.Popen([
            "python3", "lpx_renderer_simple.py"
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        
        log_message(f"Client process started (PID: {client_process.pid})")
        
        # Monitor client output for connection timing
        connection_established = False
        first_frame_received = False
        client_timeout = 30  # 30 seconds max for client connection
        
        start_monitor = time.time()
        while (time.time() - start_monitor) < client_timeout:
            # Check if process is still running
            if client_process.poll() is not None:
                log_message("Client process exited")
                break
            
            # Try to read output
            try:
                line = client_process.stdout.readline()
                if line:
                    line_clean = line.strip()
                    log_message(f"CLIENT: {line_clean}")
                    
                    # Look for key timing events
                    if "Connected to LPX server" in line_clean and not connection_established:
                        connection_time = time.time() - client_start_time
                        log_message(f"✓ CONNECTION ESTABLISHED after {connection_time*1000:.1f}ms")
                        connection_established = True
                    
                    elif "First frame received" in line_clean and not first_frame_received:
                        first_frame_time = time.time() - client_start_time
                        log_message(f"✓ FIRST FRAME RECEIVED after {first_frame_time*1000:.1f}ms")
                        first_frame_received = True
                        
                        # We have what we need, can stop monitoring
                        break
                    
                    elif "Failed to connect" in line_clean:
                        log_message("✗ CLIENT CONNECTION FAILED")
                        break
            except:
                pass
            
            time.sleep(0.01)
        
        # Step 4: Summary
        total_time = time.time() - server_start_time
        log_message("=" * 60)
        log_message("TIMING SUMMARY:")
        log_message(f"  Server startup: {server_startup_time*1000:.1f}ms")
        if connection_established:
            log_message(f"  Client connection: {connection_time*1000:.1f}ms")
        else:
            log_message("  Client connection: FAILED or TIMEOUT")
        
        if first_frame_received:
            log_message(f"  First frame delivery: {first_frame_time*1000:.1f}ms")
        else:
            log_message("  First frame delivery: NOT RECEIVED")
        
        log_message(f"  Total end-to-end time: {total_time*1000:.1f}ms")
        log_message("=" * 60)
        
        if connection_established and first_frame_received:
            if connection_time < 0.1 and first_frame_time < 0.5:
                log_message("✅ RESULT: Startup timing appears to be FIXED!")
            else:
                log_message("⚠️  RESULT: Some delays still present")
        else:
            log_message("❌ RESULT: Connection or frame delivery FAILED")
        
        # Let it run for a few more seconds to see FPS
        log_message("Letting system run for 5 more seconds to check FPS...")
        time.sleep(5)
        
        # Step 5: Analyze timing diagnostics from the log
        log_message("=" * 60)
        log_message("ANALYZING TIMING DIAGNOSTICS:")
        log_message("=" * 60)
        
        timing_entries = []
        slow_operations = []
        
        try:
            with open("current_test_log.txt", "r") as f:
                for line in f:
                    if "TIMING DIAGNOSTIC:" in line:
                        timing_entries.append(line.strip())
                    elif "SLOW OPERATION DETECTED:" in line:
                        slow_operations.append(line.strip())
            
            if timing_entries:
                log_message(f"Found {len(timing_entries)} timing diagnostic entries:")
                for entry in timing_entries[:10]:  # Show first 10
                    log_message(f"  {entry}")
                if len(timing_entries) > 10:
                    log_message(f"  ... and {len(timing_entries) - 10} more entries")
            else:
                log_message("⚠️  No timing diagnostics found - check if instrumented build is being used")
            
            if slow_operations:
                log_message(f"\n🐌 SLOW OPERATIONS DETECTED ({len(slow_operations)} total):")
                for op in slow_operations:
                    log_message(f"  {op}")
            else:
                log_message("\n✅ No slow operations detected")
                
        except Exception as e:
            log_message(f"Error analyzing timing data: {e}")
        
    except Exception as e:
        log_message(f"ERROR in test: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        cleanup()
        log_message("Test completed. Check 'current_test_log.txt' for full details.")

if __name__ == "__main__":
    test_startup_timing()
