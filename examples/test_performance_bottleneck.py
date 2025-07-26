#!/usr/bin/env python3
"""
test_performance_bottleneck.py - Identify the exact performance bottleneck

This test will:
1. Create a controlled environment to measure LPX processing time
2. Test different image sizes to see how performance scales
3. Identify if the bottleneck is in the binary search or elsewhere
4. Provide clear data on where the delays are occurring
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

def test_lpx_processing_performance():
    """Test LPX processing performance with different scenarios"""
    
    log_message("=" * 60)
    log_message("LPXImage Performance Bottleneck Analysis")
    log_message("=" * 60)
    
    # Test different image sizes to see how performance scales
    test_sizes = [
        (160, 120),   # Very small
        (320, 240),   # Small
        (640, 480),   # Medium (default)
        (1280, 720),  # Large
    ]
    
    server_process = None
    
    try:
        for width, height in test_sizes:
            log_message(f"Testing image size: {width}x{height}")
            
            # Start server with this size
            server_process = subprocess.Popen([
                "python3", "lpx_file_server.py",
                "--file", "../2342260-hd_1920_1080_30fps.mp4",
                "--width", str(width), "--height", str(height)
            ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            
            log_message(f"Server started (PID: {server_process.pid})")
            
            # Let server initialize
            time.sleep(2)
            
            # Start client and measure time to first frame
            start_time = time.time()
            
            client_process = subprocess.Popen([
                "python3", "lpx_renderer_simple.py"
            ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            
            # Monitor client for first frame
            first_frame_time = None
            timeout = 30  # 30 seconds max
            
            start_monitor = time.time()
            while (time.time() - start_monitor) < timeout:
                if client_process.poll() is not None:
                    break
                
                try:
                    line = client_process.stdout.readline()
                    if line:
                        line_clean = line.strip()
                        if "Successfully rendered image" in line_clean or "Image ready for display" in line_clean:
                            first_frame_time = time.time() - start_time
                            log_message(f"  First frame for {width}x{height}: {first_frame_time*1000:.1f}ms")
                            break
                except:
                    pass
                
                time.sleep(0.01)
            
            if first_frame_time is None:
                log_message(f"  First frame for {width}x{height}: TIMEOUT or FAILED")
            
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
            
            server_process = None
            
            # Wait between tests
            time.sleep(1)
        
        log_message("=" * 60)
        log_message("ANALYSIS:")
        log_message("If performance degrades significantly with larger images,")
        log_message("the bottleneck is likely in the per-pixel processing")
        log_message("(binary search in scan tables).")
        log_message("")
        log_message("Expected results:")
        log_message("- 160x120 (19,200 pixels): ~50-100ms")
        log_message("- 320x240 (76,800 pixels): ~200-400ms") 
        log_message("- 640x480 (307,200 pixels): ~800-1600ms")
        log_message("- 1280x720 (921,600 pixels): ~2400-4800ms")
        log_message("")
        log_message("If the times scale linearly with pixel count,")
        log_message("the issue is definitely the per-pixel binary search!")
        log_message("=" * 60)
        
    except Exception as e:
        log_message(f"ERROR in test: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        # Cleanup any remaining process
        if server_process:
            try:
                server_process.terminate()
                server_process.wait(timeout=5)
            except:
                try:
                    server_process.kill()
                except:
                    pass

if __name__ == "__main__":
    test_lpx_processing_performance()
