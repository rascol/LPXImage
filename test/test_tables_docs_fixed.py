#!/usr/bin/env python3
"""
Test the corrected LPXTables example from python/README.md
"""

import lpximage

# Initialize scan tables (this is the corrected documentation example)
tables = lpximage.LPXTables("../ScanTables63")

# Check if initialization was successful
if not tables.isInitialized():
    print("Failed to initialize scan tables")
    exit(1)
    
print("✓ Successfully initialized scan tables")

# Access properties (corrected attribute names)
print(f"Spiral period: {tables.spiralPer}")
print(f"Length: {tables.length}")
print("✓ Documentation example now uses correct attribute names!")
