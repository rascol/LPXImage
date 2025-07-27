#!/usr/bin/env python3
"""
Test basic import and server creation from README Quick Start section
"""

import lpximage

# Test basic import and server creation
server = lpximage.WebcamLPXServer("../ScanTables63")
print("✓ Successfully imported lpximage and created WebcamLPXServer")
print(f"Server object: {server}")
