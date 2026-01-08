#!/usr/bin/env python3
"""
TTF to BMP Font Converter for Nuvie Korean Localization
Converts TTF font to BMP sprite sheet format that Nuvie can use.

Usage:
    python ttf_to_bmp_font.py <ttf_file> <output_prefix> [--size 16] [--chars hangul]
"""

import sys
import os
from PIL import Image, ImageDraw, ImageFont
import struct

# Korean Hangul syllables range: 0xAC00 ~ 0xD7A3 (11,172 chars)
# But we'll use a subset for common usage (2,350 KS X 1001 chars)
# Plus ASCII printable: 0x20 ~ 0x7E (95 chars)

def get_hangul_chars():
    """Get all Korean Hangul syllables (11,172 chars) + ASCII + symbols"""
    chars = []

    # ASCII printable (32-126) = 95 chars
    for i in range(32, 127):
        chars.append(chr(i))

    # Korean Hangul Jamo (자모) - ㄱ-ㅎ, ㅏ-ㅣ (0x3131-0x3163) = 51 chars
    for i in range(0x3131, 0x3164):
        chars.append(chr(i))

    # Full Korean Hangul syllables (완성형 11,172자)
    # Range: 0xAC00 to 0xD7A3
    for i in range(0xAC00, 0xD7A4):
        chars.append(chr(i))

    # Common symbols and punctuation
    extra_chars = "—–''""…·×÷±≠≤≥∞♠♣♥♦★☆○●□■△▲▽▼◇◆→←↑↓↔"
    for c in extra_chars:
        if c not in chars:
            chars.append(c)

    return chars

def get_basic_chars():
    """Get only ASCII + basic Korean for testing"""
    chars = []

    # ASCII printable (32-126)
    for i in range(32, 127):
        chars.append(chr(i))

    # Common Korean consonants and vowels (자음 모음)
    # ㄱ-ㅎ (0x3131-0x314E), ㅏ-ㅣ (0x314F-0x3163)
    for i in range(0x3131, 0x3164):
        chars.append(chr(i))

    # 500 most common Hangul syllables (for smaller test file)
    common_syllables = "가각간갈감갑강개객거건걸검겁게격견결겸경계고곡곤골공과곽관광괘괴교구국군굴궁권궐귀규균귤극근금급긍기긴길김깅나낙난날남납낭내녀녁년념녕노녹논농뇌뇨누눈눌뉴늑능니닉닌닐님다단달담답당대덕도독돈돌동두둔둘득등디락란랄람랑래랭략량려력련렬렴렵령례로록론롤롱뢰료룡루룩룬류륙률륭륵름릉리린림립마막만말맘망매맥맨멀멍메면멸명모목몰몽묘무묵문물미민밀박반발방배백번벌범법벽변별병보복본볼봉부북분불붕비빈빙사삭산살삼상새색생서석선설섬섭성세소속손송수숙순술숭쉬슬습승시식신실심십쌍씨아악안알암압앙애액야약양어억언얼엄업엔여역연열염엽영예오옥온올옹와완왕외요욕용우욱운울웅원월위유육윤율융은을음응의이익인일임입잉자작잔잘잠장재쟁저적전절점접정제조족존졸종좌죄주죽준줄중즉증지직진질짐집징차착찬찰참창채책처척천철첨첩청체초촉촌총최추축춘출충취측층치칙친칠침칭쾌타탁탄탈탐탑탕태택터토톤통퇴투특틴파판팔패팽퍼편평폐포폭표푸풍프피픽필하학한할함합항해핵행향허헌험혁현혈협형혜호혹혼홀홍화확환활황회획횡효후훈훌훙휘휴흉흑흔흘흥희"
    for c in common_syllables:
        if c not in chars:
            chars.append(c)

    return chars

def create_font_bmp(ttf_path, output_prefix, char_size=16, char_set='basic'):
    """
    Create BMP font sprite sheet and width data file.

    Nuvie BMPFont expects:
    - 16 characters per row
    - BMP file with transparent color (0, 0x70, 0xFC)
    - DAT file with character widths
    """

    # Load TTF font
    try:
        font = ImageFont.truetype(ttf_path, char_size - 2)  # Leave padding
    except Exception as e:
        print(f"Error loading font: {e}")
        return False

    # Get character set
    if char_set == 'hangul':
        chars = get_hangul_chars()
    else:
        chars = get_basic_chars()

    num_chars = len(chars)
    chars_per_row = 16
    num_rows = (num_chars + chars_per_row - 1) // chars_per_row

    print(f"Creating font with {num_chars} characters")
    print(f"Grid: {chars_per_row} x {num_rows} = {chars_per_row * num_rows} cells")
    print(f"Cell size: {char_size} x {char_size}")

    # Create image
    img_width = chars_per_row * char_size
    img_height = num_rows * char_size

    # Nuvie transparent color: RGB(0, 0x70, 0xFC)
    transparent_color = (0, 0x70, 0xFC)
    text_color = (255, 255, 255)  # White text

    img = Image.new('RGB', (img_width, img_height), transparent_color)
    draw = ImageDraw.Draw(img)

    # Width data for each character
    widths = []

    for idx, char in enumerate(chars):
        row = idx // chars_per_row
        col = idx % chars_per_row

        x = col * char_size
        y = row * char_size

        # Get character bounding box
        try:
            bbox = font.getbbox(char)
            char_width = bbox[2] - bbox[0] if bbox else char_size // 2
            char_height = bbox[3] - bbox[1] if bbox else char_size
        except:
            char_width = char_size // 2
            char_height = char_size

        # Center character in cell
        offset_x = (char_size - char_width) // 2
        offset_y = (char_size - char_height) // 2

        # Draw character
        draw.text((x + offset_x, y + offset_y), char, font=font, fill=text_color)

        # Store width (capped at cell size)
        widths.append(min(char_width + 2, char_size))

    # Pad widths to fill grid
    while len(widths) < chars_per_row * num_rows:
        widths.append(char_size // 2)

    # Save BMP
    bmp_path = f"{output_prefix}.bmp"
    img.save(bmp_path, 'BMP')
    print(f"Saved: {bmp_path}")

    # Save width data
    dat_path = f"{output_prefix}.dat"
    with open(dat_path, 'wb') as f:
        f.write(bytes(widths))
    print(f"Saved: {dat_path}")

    # Save character mapping (for text conversion later)
    map_path = f"{output_prefix}_charmap.txt"
    with open(map_path, 'w', encoding='utf-8') as f:
        f.write(f"# Character mapping for {output_prefix}\n")
        f.write(f"# Index -> Unicode Character\n")
        for idx, char in enumerate(chars):
            f.write(f"{idx}\t{ord(char):04X}\t{char}\n")
    print(f"Saved: {map_path}")

    return True

def main():
    if len(sys.argv) < 3:
        print("Usage: python ttf_to_bmp_font.py <ttf_file> <output_prefix> [--size 16] [--chars basic|hangul]")
        print("")
        print("Example:")
        print("  python ttf_to_bmp_font.py NotoSansKR-Medium.ttf korean_font --size 16 --chars basic")
        sys.exit(1)

    ttf_path = sys.argv[1]
    output_prefix = sys.argv[2]

    char_size = 16
    char_set = 'basic'

    # Parse optional arguments
    i = 3
    while i < len(sys.argv):
        if sys.argv[i] == '--size' and i + 1 < len(sys.argv):
            char_size = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == '--chars' and i + 1 < len(sys.argv):
            char_set = sys.argv[i + 1]
            i += 2
        else:
            i += 1

    if not os.path.exists(ttf_path):
        print(f"Error: TTF file not found: {ttf_path}")
        sys.exit(1)

    success = create_font_bmp(ttf_path, output_prefix, char_size, char_set)

    if success:
        print("\nDone! Next steps:")
        print("1. Copy .bmp and .dat files to Nuvie data folder")
        print("2. Modify FontManager to load Korean font")
        print("3. Use charmap to convert game text to indexed format")
    else:
        print("Failed to create font files")
        sys.exit(1)

if __name__ == '__main__':
    main()
