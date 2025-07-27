#!/usr/bin/env python3
"""
Test LPXTables class from python/README.md
"""

import lpximage

# Initialize scan tables
tables = lpximage.LPXTables("../ScanTables63")

# Check if initialization was successful
if not tables.isInitialized():
    print("❌ Failed to initialize scan tables")
    exit(1)
else:
    print("✓ Successfully initialized scan tables")
    
# Access properties
print(f"Spiral period: {tables.spiralPer}")
print(f"Length: {tables.length}")
