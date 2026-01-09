#!/usr/bin/env python3
"""Extract item names from Ultima 6 look.lzd file"""

import struct
import sys
import os

def decompress_lzw(data):
    """Simple LZW decompression for U6 look.lzd"""
    # U6 uses a variant of LZW compression
    # For simplicity, let's try to read the raw file first
    # The look.lzd is actually already decompressed in some builds
    return data

def extract_look_items(look_file_path):
    """Extract item names from look.lzd file"""
    items = {}

    with open(look_file_path, 'rb') as f:
        data = f.read()

    # Parse the look table format:
    # Each entry is: 2-byte tile number + null-terminated string
    pos = 0
    while pos < len(data) - 2:
        # Read 2-byte tile number (little-endian)
        tile_num = struct.unpack('<H', data[pos:pos+2])[0]

        if tile_num >= 2048:
            break

        pos += 2

        # Read null-terminated string
        string_start = pos
        while pos < len(data) and data[pos] != 0:
            pos += 1

        if pos > string_start:
            item_name = data[string_start:pos].decode('latin-1', errors='replace')
            items[tile_num] = item_name

        pos += 1  # Skip null terminator

    return items

def main():
    # Try different possible locations
    possible_paths = [
        r"C:\games\Ultima6\LOOK.LZD",
        r"C:\games\Ultima6\look.lzd",
        r"D:\proj\Ultima6\nuvie\msvc\Release\data\u6\LOOK.LZD",
        r"D:\proj\Ultima6\nuvie\msvc\Release\data\u6\look.lzd",
    ]

    look_file = None
    for path in possible_paths:
        if os.path.exists(path):
            look_file = path
            break

    if not look_file:
        print("Could not find look.lzd file")
        print("Tried:", possible_paths)
        return

    print(f"Reading from: {look_file}")
    items = extract_look_items(look_file)

    # Sort by tile number and print
    for tile_num in sorted(items.keys()):
        print(f"{tile_num}|{items[tile_num]}")

if __name__ == "__main__":
    main()
