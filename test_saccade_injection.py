#!/usr/bin/env python3

import lpximage
import time
import random
import math

def test_saccade_injection_latency():
    """Test programmatic saccade injection with various durations and locations"""
    
    print("=== Saccade Injection Latency Test ===")
    print(f"Current throttle setting: {lpximage.getKeyThrottleMs()}ms")
    
    try:
        # Create client
        client = lpximage.LPXDebugClient("./ScanTables63")
        print("LPXDebugClient created successfully")
        
        # Test cases: various movement patterns and durations
        test_cases = [
            # Format: (description, deltaX, deltaY, stepSize, expected_behavior)
            ("Small rightward saccade", 10.0, 0.0, 5.0, "Quick 10px right movement"),
            ("Small leftward saccade", -10.0, 0.0, 5.0, "Quick 10px left movement"),
            ("Small upward saccade", 0.0, -10.0, 5.0, "Quick 10px up movement"),
            ("Small downward saccade", 0.0, 10.0, 5.0, "Quick 10px down movement"),
            
            ("Medium diagonal saccade", 20.0, -15.0, 8.0, "Diagonal up-right movement"),
            ("Medium diagonal saccade", -20.0, 15.0, 8.0, "Diagonal down-left movement"),
            
            ("Large horizontal saccade", 50.0, 0.0, 15.0, "Large right movement"),
            ("Large vertical saccade", 0.0, -50.0, 15.0, "Large up movement"),
            
            ("Precise micro-saccade", 2.0, 1.0, 1.0, "Very small movement"),
            ("Wide saccade", 100.0, 0.0, 25.0, "Wide horizontal movement"),
        ]
        
        print("\n=== Individual Saccade Timing Tests ===")
        print("Testing programmatic injection latency (no keyboard delay)...")
        
        successful_commands = 0
        total_commands = len(test_cases)
        timing_results = []
        
        for i, (description, deltaX, deltaY, stepSize, expected) in enumerate(test_cases):
            print(f"\nTest {i+1}/{total_commands}: {description}")
            print(f"  Parameters: deltaX={deltaX}, deltaY={deltaY}, stepSize={stepSize}")
            print(f"  Expected: {expected}")
            
            # Measure injection latency
            start_time = time.time()
            
            try:
                success = client.sendMovementCommand(deltaX, deltaY, stepSize)
                
                end_time = time.time()
                latency_ms = (end_time - start_time) * 1000
                
                if success:
                    successful_commands += 1
                    timing_results.append(latency_ms)
                    print(f"  ✓ SUCCESS: Command sent in {latency_ms:.3f}ms")
                else:
                    print(f"  ✗ FAILED: Command rejected (likely no server connection)")
                    
            except Exception as e:
                end_time = time.time()
                latency_ms = (end_time - start_time) * 1000
                print(f"  ✗ ERROR: {e} (took {latency_ms:.3f}ms)")
            
            # Small delay between tests to avoid overwhelming any potential server
            time.sleep(0.1)
        
        print(f"\n=== Rapid Saccade Sequence Test ===")
        print("Testing rapid sequence of saccades (simulating natural eye movement)...")
        
        # Rapid sequence test - simulating natural saccade patterns
        rapid_sequence = [
            (5.0, 0.0, 3.0),    # Small right
            (0.0, -3.0, 2.0),   # Small up
            (-8.0, 0.0, 4.0),   # Medium left
            (0.0, 5.0, 3.0),    # Small down
            (15.0, -10.0, 8.0), # Diagonal
            (-12.0, 8.0, 6.0),  # Return diagonal
        ]
        
        sequence_start = time.time()
        rapid_timings = []
        
        for j, (deltaX, deltaY, stepSize) in enumerate(rapid_sequence):
            cmd_start = time.time()
            
            try:
                success = client.sendMovementCommand(deltaX, deltaY, stepSize)
                cmd_end = time.time()
                cmd_latency = (cmd_end - cmd_start) * 1000
                rapid_timings.append(cmd_latency)
                
                status = "SUCCESS" if success else "FAILED"
                print(f"  Sequence {j+1}: ({deltaX:5.1f}, {deltaY:5.1f}) -> {status} in {cmd_latency:.3f}ms")
                
            except Exception as e:
                cmd_end = time.time()
                cmd_latency = (cmd_end - cmd_start) * 1000
                rapid_timings.append(cmd_latency)
                print(f"  Sequence {j+1}: ERROR {e} in {cmd_latency:.3f}ms")
            
            # Minimal delay to simulate natural saccade intervals (20-50ms between saccades)
            time.sleep(0.02)  # 20ms
        
        sequence_end = time.time()
        total_sequence_time = (sequence_end - sequence_start) * 1000
        
        print(f"\n=== Timing Analysis ===")
        
        if timing_results:
            avg_latency = sum(timing_results) / len(timing_results)
            min_latency = min(timing_results)
            max_latency = max(timing_results)
            
            print(f"Individual Command Latency:")
            print(f"  Average: {avg_latency:.3f}ms")
            print(f"  Minimum: {min_latency:.3f}ms") 
            print(f"  Maximum: {max_latency:.3f}ms")
            print(f"  Success Rate: {successful_commands}/{total_commands} ({100*successful_commands/total_commands:.1f}%)")
            
        if rapid_timings:
            rapid_avg = sum(rapid_timings) / len(rapid_timings)
            rapid_min = min(rapid_timings)
            rapid_max = max(rapid_timings)
            
            print(f"\nRapid Sequence Latency:")
            print(f"  Average per command: {rapid_avg:.3f}ms")
            print(f"  Minimum: {rapid_min:.3f}ms")
            print(f"  Maximum: {rapid_max:.3f}ms") 
            print(f"  Total sequence time: {total_sequence_time:.1f}ms")
            print(f"  Commands per second: {len(rapid_sequence) / (total_sequence_time/1000):.1f}")
        
        print(f"\n=== Performance Assessment ===")
        if timing_results and rapid_timings:
            # Assess if latency is suitable for saccade injection
            if avg_latency < 1.0:  # Less than 1ms average
                print("✓ EXCELLENT: Sub-millisecond latency suitable for real-time saccade injection")
            elif avg_latency < 5.0:  # Less than 5ms average
                print("✓ VERY GOOD: Low latency suitable for high-frequency saccade injection")
            elif avg_latency < 10.0:  # Less than 10ms average
                print("✓ GOOD: Acceptable latency for most saccade injection applications")
            elif avg_latency < 50.0:  # Less than 50ms average  
                print("⚠ ACCEPTABLE: Moderate latency, suitable for slower saccade patterns")
            else:
                print("✗ HIGH LATENCY: May not be suitable for real-time saccade injection")
                
            # Check if we can meet natural saccade rates (3-4 saccades per second)
            max_saccade_rate = 1000 / max(timing_results) if timing_results else 0
            print(f"\nMaximum sustainable saccade rate: {max_saccade_rate:.1f} saccades/second")
            
            if max_saccade_rate > 10:
                print("✓ Can easily support natural saccade rates (3-4/sec) and research applications")
            elif max_saccade_rate > 4:
                print("✓ Can support natural saccade rates (3-4/sec)")
            else:
                print("⚠ May struggle with natural saccade rates")
        
        print(f"\n=== Conclusion ===")
        print("Programmatic saccade injection bypasses OpenCV keyboard delays entirely.")
        print("The measured latencies represent the true system performance for automated")
        print("saccade control, which is the primary use case for research applications.")
        
        return True
        
    except Exception as e:
        print(f"Test failed with error: {e}")
        return False

if __name__ == "__main__":
    test_saccade_injection_latency()
