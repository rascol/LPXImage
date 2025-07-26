#!/bin/bash

# Script to apply timing diagnostics patches to LPXImage and build instrumented version
# This will help identify where the startup delays are occurring

set -e  # Exit on any error

echo "🔧 Applying timing diagnostics to LPXImage..."

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -d "src" ]]; then
    echo "❌ Error: Please run this script from the LPXImage root directory"
    echo "   Current directory: $(pwd)"
    echo "   Expected files: CMakeLists.txt, src/ directory"
    exit 1
fi

# Backup original files before patching
echo "📋 Creating backups of original files..."
if [[ -f "src/lpx_file_server.cpp" ]]; then
    cp "src/lpx_file_server.cpp" "src/lpx_file_server.cpp.backup"
    echo "   ✓ Backed up src/lpx_file_server.cpp"
fi

if [[ -f "src/multithreaded_scan.cpp" ]]; then
    cp "src/multithreaded_scan.cpp" "src/multithreaded_scan.cpp.backup"
    echo "   ✓ Backed up src/multithreaded_scan.cpp"
fi

# Function to apply patches manually since we can't rely on patch command
apply_timing_patches() {
    echo "🔨 Applying timing diagnostics patches..."
    
    # For now, we'll provide instructions since automatic patching is complex
    echo "   ⚠️  Manual patch application required:"
    echo "   1. Add timing helper functions to lpx_file_server.cpp"
    echo "   2. Add timing logs around constructor, video loading, and LPX processing"
    echo "   3. Add detailed timing to multithreaded_scan.cpp functions"
    echo "   4. Focus on scan table loading, video file operations, and pixel processing"
    echo ""
    echo "   📋 Key areas to instrument:"
    echo "      - FileLPXServer constructor (scan table loading)"
    echo "      - Video file opening and property reading"
    echo "      - LPX image processing in videoThread()"
    echo "      - multithreadedScanFromImage() and processImageRegion()"
    echo "      - Individual peripheral region processing"
    echo ""
    echo "   💡 Use high-resolution timestamps with microsecond precision"
    echo "   💡 Log operations that take >1ms for scan tables, >10ms for regions, >100ms for overall processing"
}

# Check for build directory
setup_build() {
    echo "📁 Setting up build directory..."
    if [[ ! -d "build" ]]; then
        mkdir build
        echo "   ✓ Created build directory"
    else
        echo "   ✓ Build directory exists"
    fi
}

# Build the instrumented version
build_instrumented() {
    echo "🔨 Building instrumented LPXImage server..."
    
    cd build
    
    # Clean previous build
    if [[ -f "Makefile" ]]; then
        echo "   🧹 Cleaning previous build..."
        make clean || true
    fi
    
    # Configure with debug symbols and optimizations disabled for accurate timing
    echo "   ⚙️  Configuring with debug mode..."
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O0 -g -DTIMING_DIAGNOSTICS" ..
    
    # Build
    echo "   🔨 Compiling..."
    make -j$(nproc) || make -j4 || make
    
    cd ..
    
    if [[ -f "build/lpx_file_server" ]]; then
        echo "   ✅ Build successful! Executable: build/lpx_file_server"
    else
        echo "   ❌ Build failed - executable not found"
        return 1
    fi
}

# Create test script for instrumented version
create_instrumented_test() {
    cat > examples/test_instrumented_delays.py << 'EOF'
#!/usr/bin/env python3
"""
Test script for instrumented LPXImage server to identify startup delays.
This version captures detailed C++ timing logs.
"""

import subprocess
import time
import threading
import os
import sys
from datetime import datetime

class InstrumentedDelayTester:
    def __init__(self):
        self.server_process = None
        self.server_output = []
        self.client_output = []
        self.log_file = "instrumented_timing_log.txt"
        
    def log_message(self, source, message):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        log_entry = f"[{timestamp}] [{source}] {message}"
        print(log_entry)
        
        # Write to log file
        with open(self.log_file, "a") as f:
            f.write(log_entry + "\n")
            
    def capture_server_output(self, process):
        """Capture server output line by line with timestamps"""
        while process.poll() is None:
            try:
                line = process.stdout.readline()
                if line:
                    line = line.decode('utf-8').strip()
                    if line:
                        self.server_output.append(line)
                        self.log_message("SERVER", line)
                        
                        # Check for key timing indicators
                        if "[TIMING]" in line or "[SCAN-TIMING]" in line:
                            self.log_message("ANALYSIS", f"Performance indicator: {line}")
                            
                time.sleep(0.001)  # Small delay to prevent busy waiting
            except Exception as e:
                self.log_message("ERROR", f"Server output capture error: {e}")
                break
                
    def start_instrumented_server(self):
        """Start the instrumented LPXImage server"""
        server_path = "../build/lpx_file_server"
        scan_table = "scan_table.bin"
        video_file = "sample_video.mp4"
        
        if not os.path.exists(server_path):
            self.log_message("ERROR", f"Instrumented server not found at {server_path}")
            self.log_message("INFO", "Run apply_timing_diagnostics.sh to build instrumented version")
            return False
            
        self.log_message("TEST", "Starting instrumented LPXImage server...")
        start_time = time.time()
        
        try:
            self.server_process = subprocess.Popen(
                [server_path, scan_table, video_file, "512", "512"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                cwd=os.path.dirname(os.path.abspath(__file__))
            )
            
            # Start output capture thread
            output_thread = threading.Thread(
                target=self.capture_server_output, 
                args=(self.server_process,)
            )
            output_thread.daemon = True
            output_thread.start()
            
            self.log_message("TEST", f"Server process started (PID: {self.server_process.pid})")
            return True
            
        except Exception as e:
            self.log_message("ERROR", f"Failed to start server: {e}")
            return False
            
    def wait_for_server_ready(self, timeout=60):
        """Wait for server to be ready for connections"""
        self.log_message("TEST", "Waiting for server to be ready...")
        start_time = time.time()
        
        # Look for server ready indicators in output
        while time.time() - start_time < timeout:
            if self.server_process and self.server_process.poll() is not None:
                self.log_message("ERROR", "Server process terminated unexpectedly")
                return False
                
            # Check for ready indicators in recent output
            recent_output = self.server_output[-10:] if len(self.server_output) >= 10 else self.server_output
            for line in recent_output:
                if "FileLPXServer started on port" in line or "Server listening" in line:
                    ready_time = time.time() - start_time
                    self.log_message("TEST", f"Server ready after {ready_time:.2f} seconds")
                    return True
                    
            time.sleep(0.1)
            
        self.log_message("ERROR", f"Server not ready after {timeout} seconds")
        return False
        
    def test_client_connection(self):
        """Test client connection and measure first frame timing"""
        self.log_message("TEST", "Starting simple renderer client...")
        client_start = time.time()
        
        try:
            client_process = subprocess.Popen(
                ["python3", "simple_renderer.py"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                cwd=os.path.dirname(os.path.abspath(__file__))
            )
            
            # Monitor client output for first frame
            first_frame_received = False
            while client_process.poll() is None and time.time() - client_start < 30:
                line = client_process.stdout.readline()
                if line:
                    line = line.decode('utf-8').strip()
                    if line:
                        self.client_output.append(line)
                        self.log_message("CLIENT", line)
                        
                        # Check for first frame indicator
                        if not first_frame_received and ("Received LPX image" in line or "Frame" in line):
                            first_frame_time = time.time() - client_start
                            self.log_message("ANALYSIS", f"First frame received after {first_frame_time:.2f} seconds")
                            first_frame_received = True
                            break
                            
            client_process.terminate()
            return first_frame_received
            
        except Exception as e:
            self.log_message("ERROR", f"Client test failed: {e}")
            return False
            
    def analyze_timing_results(self):
        """Analyze the collected timing data"""
        self.log_message("ANALYSIS", "=== TIMING ANALYSIS ===")
        
        # Extract timing data from server output
        constructor_time = None
        scan_table_time = None
        video_open_time = None
        total_startup_time = None
        lpx_processing_times = []
        
        for line in self.server_output:
            if "[TIMING] FileLPXServer constructor total" in line:
                try:
                    constructor_time = float(line.split("took: ")[1].split("ms)")[0])
                except:
                    pass
            elif "[TIMING] Scan table loading" in line:
                try:
                    scan_table_time = float(line.split("took: ")[1].split("ms)")[0])
                except:
                    pass
            elif "[TIMING] Video file opening" in line:
                try:
                    video_open_time = float(line.split("took: ")[1].split("ms)")[0])
                except:
                    pass
            elif "[TIMING] LPX processing took:" in line:
                try:
                    lpx_time = float(line.split("took: ")[1].split("ms")[0])
                    lpx_processing_times.append(lpx_time)
                except:
                    pass
                    
        # Report findings
        if constructor_time:
            self.log_message("ANALYSIS", f"Constructor time: {constructor_time:.2f}ms")
        if scan_table_time:
            self.log_message("ANALYSIS", f"Scan table loading: {scan_table_time:.2f}ms")
        if video_open_time:
            self.log_message("ANALYSIS", f"Video file opening: {video_open_time:.2f}ms")
            
        if lpx_processing_times:
            avg_lpx_time = sum(lpx_processing_times) / len(lpx_processing_times)
            max_lpx_time = max(lpx_processing_times)
            self.log_message("ANALYSIS", f"LPX processing - Avg: {avg_lpx_time:.2f}ms, Max: {max_lpx_time:.2f}ms")
            
    def cleanup(self):
        """Clean up processes"""
        if self.server_process:
            self.log_message("TEST", "Stopping server...")
            self.server_process.terminate()
            try:
                self.server_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.server_process.kill()
                
    def run_test(self):
        """Run the complete instrumented timing test"""
        # Clear log file
        open(self.log_file, 'w').close()
        
        self.log_message("TEST", "=== INSTRUMENTED TIMING TEST START ===")
        test_start = time.time()
        
        try:
            # Start instrumented server
            if not self.start_instrumented_server():
                return False
                
            # Wait for server ready
            if not self.wait_for_server_ready():
                return False
                
            # Test client connection
            self.test_client_connection()
            
            # Allow some processing time
            time.sleep(5)
            
            # Analyze results
            self.analyze_timing_results()
            
            test_duration = time.time() - test_start
            self.log_message("TEST", f"=== TEST COMPLETED in {test_duration:.2f}s ===")
            self.log_message("INFO", f"Detailed log saved to: {self.log_file}")
            
            return True
            
        finally:
            self.cleanup()

if __name__ == "__main__":
    tester = InstrumentedDelayTester()
    success = tester.run_test()
    sys.exit(0 if success else 1)
EOF

    chmod +x examples/test_instrumented_delays.py
    echo "   ✅ Created instrumented test script: examples/test_instrumented_delays.py"
}

# Main execution
main() {
    echo "🚀 LPXImage Timing Diagnostics Setup"
    echo "===================================="
    
    apply_timing_patches
    setup_build
    
    echo ""
    echo "📋 Next steps:"
    echo "1. Manually apply the timing patches to the C++ source files"
    echo "2. Run: $0 --build    (to build the instrumented version)"
    echo "3. Run: cd examples && python3 test_instrumented_delays.py"
    echo ""
    echo "🔍 The patches will add detailed timing logs to identify:"
    echo "   • Scan table loading time"
    echo "   • Video file opening delays"
    echo "   • LPX image processing bottlenecks"
    echo "   • Per-region processing times"
    echo ""
}

# Handle command line arguments
if [[ "$1" == "--build" ]]; then
    build_instrumented
    create_instrumented_test
elif [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]]; then
    echo "Usage: $0 [--build|--help]"
    echo "  --build    Build the instrumented version after patches are applied"
    echo "  --help     Show this help message"
else
    main
fi
