#!/usr/bin/env python3
"""
Generate 32x32 font BMP sprite sheet from TTF font
For Nuvie Korean localization
"""

import os
from PIL import Image, ImageDraw, ImageFont

# Configuration
CELL_SIZE = 32
CHARS_PER_ROW = 64  # 64 chars per row to match KoreanFont.cpp
FONT_SIZE = 28  # Slightly smaller than cell to have padding
FONT_PATH = "../data/fonts/korean/IBMPlexSansKR-Bold.ttf"
CHARMAP_PATH = "../data/fonts/korean/korean_font_charmap.txt"
OUTPUT_BMP = "../data/fonts/korean/korean_font_32x32.bmp"
OUTPUT_DAT = "../data/fonts/korean/korean_font_32x32.dat"
OUTPUT_CHARMAP = "../data/fonts/korean/korean_font_32x32_charmap.txt"

# Colors
BG_COLOR = (255, 0, 255)  # Magenta (transparent)
FG_COLOR = (255, 255, 255)  # White (text)

def load_charmap(path):
    """Load character mapping from file"""
    chars = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) >= 3:
                index = int(parts[0])
                hex_code = parts[1]
                codepoint = int(hex_code, 16)
                char = chr(codepoint) if codepoint > 0 else ' '
                chars.append((index, codepoint, char))
    return chars

def generate_font_bmp(chars, font_path, cell_size, chars_per_row, font_size):
    """Generate BMP sprite sheet and width data"""

    # Load font
    try:
        font = ImageFont.truetype(font_path, font_size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return None, None

    # Calculate image dimensions
    num_chars = len(chars)
    num_rows = (num_chars + chars_per_row - 1) // chars_per_row
    img_width = chars_per_row * cell_size
    img_height = num_rows * cell_size

    print(f"Generating {img_width}x{img_height} image for {num_chars} characters")

    # Create image
    img = Image.new('RGB', (img_width, img_height), BG_COLOR)
    draw = ImageDraw.Draw(img)

    # Character widths
    widths = []

    # Draw each character
    for index, codepoint, char in chars:
        row = index // chars_per_row
        col = index % chars_per_row

        x = col * cell_size
        y = row * cell_size

        # Get character bounding box
        try:
            bbox = font.getbbox(char)
            char_width = bbox[2] - bbox[0] if bbox else cell_size
            char_height = bbox[3] - bbox[1] if bbox else cell_size

            # Center character in cell
            offset_x = (cell_size - char_width) // 2 - (bbox[0] if bbox else 0)
            offset_y = (cell_size - char_height) // 2 - (bbox[1] if bbox else 0)

            # Draw character
            draw.text((x + offset_x, y + offset_y), char, font=font, fill=FG_COLOR)

            # Store width (with some padding)
            widths.append(min(char_width + 2, cell_size))
        except Exception as e:
            print(f"Error drawing char {index} (U+{codepoint:04X}): {e}")
            widths.append(cell_size // 2)

    return img, widths

def main():
    # Change to script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    print("Loading character map...")
    chars = load_charmap(CHARMAP_PATH)
    print(f"Loaded {len(chars)} characters")

    print(f"Using font: {FONT_PATH}")
    print(f"Cell size: {CELL_SIZE}x{CELL_SIZE}")

    print("Generating font sprite sheet...")
    img, widths = generate_font_bmp(chars, FONT_PATH, CELL_SIZE, CHARS_PER_ROW, FONT_SIZE)

    if img is None:
        print("Failed to generate font")
        return

    # Save BMP
    print(f"Saving BMP: {OUTPUT_BMP}")
    os.makedirs(os.path.dirname(OUTPUT_BMP), exist_ok=True)
    img.save(OUTPUT_BMP, 'BMP')

    # Save width data
    print(f"Saving DAT: {OUTPUT_DAT}")
    with open(OUTPUT_DAT, 'wb') as f:
        f.write(bytes(widths))

    # Save charmap (copy original)
    print(f"Copying charmap: {OUTPUT_CHARMAP}")
    with open(CHARMAP_PATH, 'r', encoding='utf-8') as src:
        with open(OUTPUT_CHARMAP, 'w', encoding='utf-8') as dst:
            dst.write(src.read())

    print("Done!")
    print(f"Generated {len(chars)} characters in {img.width}x{img.height} image")

if __name__ == "__main__":
    main()
