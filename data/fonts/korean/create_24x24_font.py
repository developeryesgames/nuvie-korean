#!/usr/bin/env python3
"""
Create 24x24 Korean font from 32x32 font by scaling 0.75x
For compact_ui mode (3x scale): 24px font matches 8px * 3 = 24px target
"""

from PIL import Image
import os

def create_24x24_font():
    input_bmp = "korean_font_32x32.bmp"
    output_bmp = "korean_font_24x24.bmp"
    input_dat = "korean_font_32x32.dat"
    output_dat = "korean_font_24x24.dat"

    # Load original 32x32 font
    print(f"Loading {input_bmp}...")
    img = Image.open(input_bmp)
    orig_width, orig_height = img.size
    print(f"Original size: {orig_width}x{orig_height}")

    # Calculate new dimensions (0.75x scale)
    # 32x32 cells -> 24x24 cells
    # chars_per_row = 64, so width = 64 * 32 = 2048 -> 64 * 24 = 1536
    new_width = int(orig_width * 0.75)
    new_height = int(orig_height * 0.75)
    print(f"New size: {new_width}x{new_height}")

    # Resize using LANCZOS for better downscaling quality
    print("Resizing with LANCZOS interpolation...")
    new_img = img.resize((new_width, new_height), Image.LANCZOS)

    # Save as BMP
    print(f"Saving {output_bmp}...")
    new_img.save(output_bmp, "BMP")

    # Also create scaled width data
    if os.path.exists(input_dat):
        print(f"Scaling width data from {input_dat}...")
        with open(input_dat, 'rb') as f:
            widths = f.read()

        # Scale each width by 0.75
        new_widths = bytes([max(1, int(w * 0.75)) for w in widths])

        with open(output_dat, 'wb') as f:
            f.write(new_widths)
        print(f"Saved {output_dat}")

    # Copy charmap (same mapping, just reference the new file)
    print("Charmap remains the same (korean_font_32x32_charmap.txt can be used)")

    print("Done!")
    print(f"\nCreated 24x24 font:")
    print(f"  {output_bmp}")
    print(f"  {output_dat}")

if __name__ == "__main__":
    create_24x24_font()
