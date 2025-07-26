#!/usr/bin/env python3
# test_bug_fixes.py - Test script to validate keyboard delay, Ctrl+C, and hanging fixes

import subprocess
import time
import signal
import sys
import os
from datetime import datetime

def log_with_timestamp(message):
    """Log message with precise timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] {message}")

def test_keyboard_throttle():
    """Test if keyboard throttle has been reduced"""
    log_with_timestamp("=== Testing Keyboard Throttle ===")
    
    try:
        import lpximage
        throttle = lpximage.getKeyThrottleMs()
        log_with_timestamp(f"Current key throttle: {throttle}ms")
        
        if throttle <= 20:
            log_with_timestamp("✓ PASS: Key throttle is appropriately low for responsive input")
            return True
        else:
            log_with_timestamp(f"✗ FAIL: Key throttle is too high ({throttle}ms), should be ≤20ms")
            return False
            
    except Exception as e:
        log_with_timestamp(f"✗ ERROR: Failed to check throttle: {e}")
        return False

def test_server_startup_and_stop():
    """Test server startup and graceful shutdown"""
    log_with_timestamp("=== Testing Server Startup/Stop ===")
    
    server_cmd = [
        "python3", "lpx_file_server.py",
        "--file", "../2342260-hd_1920_1080_30fps.mp4",
        "--loop"
    ]
    
    try:
        log_with_timestamp("Starting server...")
        server_proc = subprocess.Popen(
            server_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            preexec_fn=os.setsid  # Create process group for clean termination
        )
        
        # Wait for server to start (look for ready signal)
        start_time = time.time()
        server_ready = False
        
        while time.time() - start_time < 15:  # 15 second timeout
            if server_proc.poll() is not None:
                log_with_timestamp("✗ FAIL: Server exited unexpectedly")
                return False
                
            # Check if we can connect (simple socket test)
            import socket
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(1)
                result = sock.connect_ex(('127.0.0.1', 5050))
                sock.close()
                if result == 0:
                    server_ready = True
                    break
            except:
                pass
            
            time.sleep(0.5)
        
        if not server_ready:
            log_with_timestamp("✗ FAIL: Server did not become ready within 15 seconds")
            server_proc.terminate()
            return False
            
        log_with_timestamp("✓ Server started and accepting connections")
        
        # Test graceful shutdown with SIGINT
        log_with_timestamp("Sending SIGINT to server...")
        os.killpg(os.getpgid(server_proc.pid), signal.SIGINT)
        
        # Wait for graceful shutdown
        try:
            server_proc.wait(timeout=5)
            log_with_timestamp("✓ PASS: Server shut down gracefully")
            return True
        except subprocess.TimeoutExpired:
            log_with_timestamp("✗ FAIL: Server did not shut down within 5 seconds, force killing")
            server_proc.kill()
            return False
            
    except Exception as e:
        log_with_timestamp(f"✗ ERROR: Server test failed: {e}")
        try:
            server_proc.kill()
        except:
            pass
        return False

def test_renderer_ctrlc():
    """Test renderer Ctrl+C handling"""
    log_with_timestamp("=== Testing Renderer Ctrl+C Handling ===")
    
    # First start a server
    server_cmd = [
        "python3", "lpx_file_server.py",
        "--file", "../2342260-hd_1920_1080_30fps.mp4",
        "--loop"
    ]
    
    try:
        log_with_timestamp("Starting server for renderer test...")
        server_proc = subprocess.Popen(
            server_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            preexec_fn=os.setsid
        )
        
        # Wait for server to be ready
        time.sleep(3)
        
        # Start renderer
        renderer_cmd = ["python3", "lpx_renderer_simple.py"]
        
        log_with_timestamp("Starting renderer...")
        renderer_proc = subprocess.Popen(
            renderer_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            preexec_fn=os.setsid
        )
        
        # Let renderer connect and run briefly
        time.sleep(3)
        
        # Send SIGINT to renderer
        log_with_timestamp("Sending SIGINT to renderer...")
        os.killpg(os.getpgid(renderer_proc.pid), signal.SIGINT)
        
        # Check if renderer exits gracefully
        try:
            renderer_proc.wait(timeout=3)
            log_with_timestamp("✓ PASS: Renderer handled Ctrl+C and exited gracefully")
            success = True
        except subprocess.TimeoutExpired:
            log_with_timestamp("✗ FAIL: Renderer did not exit within 3 seconds after Ctrl+C")
            renderer_proc.kill()
            success = False
        
        # Clean up server
        os.killpg(os.getpgid(server_proc.pid), signal.SIGINT)
        try:
            server_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_proc.kill()
        
        return success
        
    except Exception as e:
        log_with_timestamp(f"✗ ERROR: Renderer Ctrl+C test failed: {e}")
        try:
            renderer_proc.kill()
            server_proc.kill()
        except:
            pass
        return False

def main():
    """Run all bug fix tests"""
    log_with_timestamp("Starting LPXImage Bug Fix Validation Tests")
    log_with_timestamp("=" * 50)
    
    tests = [
        ("Keyboard Throttle", test_keyboard_throttle),
        ("Server Startup/Stop", test_server_startup_and_stop),
        ("Renderer Ctrl+C", test_renderer_ctrlc),
    ]
    
    results = []
    
    for test_name, test_func in tests:
        log_with_timestamp(f"Running test: {test_name}")
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            log_with_timestamp(f"✗ ERROR: Test {test_name} crashed: {e}")
            results.append((test_name, False))
        
        log_with_timestamp("")  # Blank line between tests
    
    # Summary
    log_with_timestamp("=" * 50)
    log_with_timestamp("TEST SUMMARY:")
    
    passed = 0
    for test_name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        log_with_timestamp(f"  {status}: {test_name}")
        if result:
            passed += 1
    
    log_with_timestamp(f"\nOverall: {passed}/{len(results)} tests passed")
    
    if passed == len(results):
        log_with_timestamp("🎉 All bug fixes validated successfully!")
        return 0
    else:
        log_with_timestamp("❌ Some tests failed - bug fixes need attention")
        return 1

if __name__ == "__main__":
    sys.exit(main())
