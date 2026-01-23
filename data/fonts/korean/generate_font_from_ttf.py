#!/usr/bin/env python3
"""
Generate Korean bitmap font from TTF font file.
Creates sprite sheet BMP and character width data file.

Usage:
    python generate_font_from_ttf.py [cell_size]

    cell_size: 24 or 32 (default: 24)
"""

from PIL import Image, ImageDraw, ImageFont
import os
import sys

def generate_font(cell_size=24):
    """Generate Korean font bitmap from TTF."""

    ttf_file = "IBMPlexSansKR-Bold.ttf"
    charmap_file = "korean_font_32x32_charmap.txt"

    output_bmp = f"korean_font_{cell_size}x{cell_size}.bmp"
    output_dat = f"korean_font_{cell_size}x{cell_size}.dat"

    # Font size: close to cell size for tight fit
    # For 32x32 cell, use ~28pt font; for 24x24, use ~22pt
    font_size = int(cell_size * 0.92)

    print(f"Generating {cell_size}x{cell_size} font from {ttf_file}")
    print(f"Font size: {font_size}pt")

    # Load TTF font
    try:
        ttf_font = ImageFont.truetype(ttf_file, font_size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return False

    # Parse charmap to get all characters
    characters = []
    with open(charmap_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) >= 3:
                index = int(parts[0])
                hex_code = parts[1]
                char = parts[2] if len(parts[2]) > 0 else ' '
                codepoint = int(hex_code, 16)
                characters.append((index, codepoint, char))

    print(f"Loaded {len(characters)} characters from charmap")

    # Calculate sprite sheet dimensions
    chars_per_row = 64
    total_chars = len(characters)
    num_rows = (total_chars + chars_per_row - 1) // chars_per_row

    sheet_width = chars_per_row * cell_size
    sheet_height = num_rows * cell_size

    print(f"Sprite sheet: {sheet_width}x{sheet_height} ({num_rows} rows)")

    # Create sprite sheet with magenta background (transparent color)
    # RGB(255, 0, 255) = magenta
    magenta = (255, 0, 255)
    text_color = (139, 90, 43)  # Brown color matching original font

    img = Image.new('RGB', (sheet_width, sheet_height), magenta)
    draw = ImageDraw.Draw(img)

    # Track character widths
    char_widths = []

    # Render each character
    for index, codepoint, char in characters:
        # Calculate position in sprite sheet
        row = index // chars_per_row
        col = index % chars_per_row
        x = col * cell_size
        y = row * cell_size

        # Get character bounding box for width calculation
        if char and char != ' ':
            bbox = draw.textbbox((0, 0), char, font=ttf_font)
            char_width = bbox[2] - bbox[0]
            char_height = bbox[3] - bbox[1]

            # Center character in cell
            x_offset = (cell_size - char_width) // 2
            y_offset = (cell_size - char_height) // 2 - bbox[1]

            # Draw character
            draw.text((x + x_offset, y + y_offset), char, font=ttf_font, fill=text_color)

            # Store actual width (minimal padding for tighter spacing)
            actual_width = min(char_width, cell_size)
        else:
            # Space character
            actual_width = cell_size // 3

        char_widths.append(actual_width)

    # Pad char_widths to match total_chars if needed
    while len(char_widths) < total_chars:
        char_widths.append(cell_size // 2)

    # Save BMP
    print(f"Saving {output_bmp}...")
    img.save(output_bmp, "BMP")

    # Save width data
    print(f"Saving {output_dat}...")
    with open(output_dat, 'wb') as f:
        f.write(bytes(char_widths))

    print("Done!")
    print(f"  {output_bmp}: {os.path.getsize(output_bmp)} bytes")
    print(f"  {output_dat}: {os.path.getsize(output_dat)} bytes")

    return True

if __name__ == "__main__":
    cell_size = 24
    if len(sys.argv) > 1:
        try:
            cell_size = int(sys.argv[1])
        except ValueError:
            print(f"Invalid cell size: {sys.argv[1]}")
            sys.exit(1)

    if cell_size not in [24, 32, 48]:
        print(f"Warning: unusual cell size {cell_size}")

    if not generate_font(cell_size):
        sys.exit(1)
