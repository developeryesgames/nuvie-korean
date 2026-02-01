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
    # Use dict to store by index, then convert to sorted list
    char_dict = {}
    with open(charmap_file, 'r', encoding='utf-8') as f:
        for line in f:
            # Don't strip trailing space - it might be the space character!
            line = line.rstrip('\n\r')
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) >= 2:
                index = int(parts[0])
                hex_code = parts[1]
                # parts[2] might be empty for space character (U+0020)
                char = parts[2] if len(parts) >= 3 and len(parts[2]) > 0 else ' '
                codepoint = int(hex_code, 16)
                char_dict[index] = (codepoint, char)

    # Convert to sorted list by index
    max_index = max(char_dict.keys())
    characters = []
    for i in range(max_index + 1):
        if i in char_dict:
            codepoint, char = char_dict[i]
            characters.append((i, codepoint, char))
        else:
            # Missing index - use space as placeholder
            characters.append((i, 0x20, ' '))

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

            # 한글(U+AC00~U+D7A3)인지 확인
            is_korean = 0xAC00 <= codepoint <= 0xD7A3

            if is_korean:
                # 한글: 가운데 정렬 (고정폭)
                x_offset = (cell_size - char_width) // 2
            else:
                # ASCII/문장부호: 왼쪽 정렬 (가변폭)
                # bbox[0]은 문자의 왼쪽 bearing (좌측 여백)
                x_offset = max(0, -bbox[0])  # 음수 방지, 셀 왼쪽에서 시작

            # Y: baseline 기준 정렬 (모든 문자가 같은 baseline에 맞춰짐)
            # TTF 폰트의 ascent/descent 메트릭 사용
            ascent, descent = ttf_font.getmetrics()
            # baseline 위치: 셀 중앙에서 ascent 기준으로 배치
            baseline_y = (cell_size + ascent - descent) // 2
            # 문자를 baseline 기준으로 그리기 (bbox[1]은 baseline에서 top까지의 거리)
            y_offset = baseline_y - ascent

            # Draw character
            draw.text((x + x_offset, y + y_offset), char, font=ttf_font, fill=text_color)

            # Get advance width from TTF (프로포셔널 자간)
            # ttf_font.getlength()는 실제 advance width 반환
            try:
                advance_width = int(ttf_font.getlength(char))
            except:
                advance_width = char_width

            # 자간에 약간의 여백 추가 (1~2픽셀)
            actual_width = min(advance_width + 1, cell_size)
        else:
            # Space character
            actual_width = cell_size // 3

        char_widths.append(actual_width)

    # 숫자 9의 width 수동 보정 (필요시)
    # charmap에서 '9'의 인덱스를 찾아서 보정
    for i, (index, codepoint, char) in enumerate(characters):
        if char == '9':
            # 다른 숫자들과 동일한 width로 맞춤 (숫자 0 기준)
            for j, (idx2, cp2, ch2) in enumerate(characters):
                if ch2 == '0':
                    char_widths[i] = char_widths[j]
                    print(f"Fixed '9' width: {char_widths[i]} (same as '0')")
                    break
            break

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
