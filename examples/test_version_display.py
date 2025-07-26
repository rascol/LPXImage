#!/usr/bin/env python3
# test_version_display.py - Demonstrate the prominent version display

import sys
print(f"Using Python: {sys.executable}")

try:
    import lpximage
    print("LPXImage module found!")
    
    # Helper function to get version info with fallback
    def get_version_info():
        try:
            version = lpximage.getVersionString()
            build = lpximage.getBuildNumber()
            throttle = lpximage.getKeyThrottleMs()
            return version, build, throttle
        except AttributeError:
            return "Unknown", "Unknown", "Unknown"
    
    # Show the prominent version display
    version, build, throttle = get_version_info()
    print("\n" + "=" * 60)
    print(f"LPXImage Renderer v{version} (Build {build})")
    print(f"Key Throttle: {throttle}ms")
    print("=" * 60)
    
    if version == "Unknown":
        print("⚠️  Version functions not available in this Python environment")
        print("💡 Try running with: python3.12 test_version_display.py")
    else:
        print("✅ Version functions working perfectly!")
        print("🎉 This is what you'll see at startup!")

except ImportError as e:
    print(f"❌ Error importing lpximage: {e}")
