#!/usr/bin/env python3
"""Extract book texts from Ultima 6 book.dat file for translation"""

import struct
import os

def extract_books(book_dat_path, output_path):
    """Extract all book texts from book.dat"""

    with open(book_dat_path, 'rb') as f:
        data = f.read()

    # book.dat format:
    # First 2 bytes: seems to be version/count info
    # Then pairs of 2-byte offsets for each item
    # Data follows after offset table

    if len(data) < 2:
        print("File too small")
        return

    # The file starts with offsets - each 2 bytes is a relative offset
    # We need to find where the offset table ends and data begins

    # Looking at the hex dump:
    # 0x0100-0x011e: "The perpetual motion machine."
    # The first text starts around offset 0x0100 (256)

    # Let's try reading 2-byte offsets until we hit text data
    offsets = []
    pos = 0

    # Read offsets until we find one that points before current position
    # or until we reach the first offset value
    first_offset = struct.unpack('<H', data[0:2])[0]

    # The offset table is at the beginning, read until first_offset
    while pos < first_offset and pos + 2 <= len(data):
        offset = struct.unpack('<H', data[pos:pos+2])[0]
        offsets.append(offset)
        pos += 2

    print(f"Found {len(offsets)} offsets")
    print(f"First offset: {offsets[0] if offsets else 'N/A'}")
    print(f"Last offset: {offsets[-1] if offsets else 'N/A'}")

    # Extract each book text
    books = []
    for i in range(len(offsets)):
        start = offsets[i]
        end = offsets[i + 1] if i + 1 < len(offsets) else len(data)

        if start >= len(data):
            continue

        text_data = data[start:end]

        # Decode as latin-1 (U6 encoding), strip null terminator
        try:
            text = text_data.decode('latin-1').rstrip('\x00')
        except:
            text = f"[Binary data {len(text_data)} bytes]"

        if text:  # Only add non-empty entries
            books.append((i, text))

    print(f"Extracted {len(books)} non-empty books")

    # Write output file
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("# Book texts extracted from book.dat\n")
        f.write("# Format for translation: BOOK_NUM|KOREAN_TEXT\n")
        f.write("# Use \\n for newlines in translation\n\n")

        for book_num, text in books:
            # Show first 80 chars as preview
            preview = text[:80].replace('\n', ' ').replace('\r', '')
            if len(text) > 80:
                preview += "..."

            f.write(f"# Book {book_num}: {preview}\n")
            f.write(f"# Length: {len(text)} chars\n")

            # Write escaped text for reference
            escaped_text = text.replace('\n', '\\n').replace('\r', '')
            f.write(f"# EN: {escaped_text}\n")
            f.write(f"# {book_num}|<KOREAN>\n\n")

    print(f"Wrote to {output_path}")

    # Also write a simpler format for easy translation
    simple_path = output_path.replace('.txt', '_simple.txt')
    with open(simple_path, 'w', encoding='utf-8') as f:
        f.write("# Simple book text list for translation\n")
        f.write("# One book per section\n\n")

        for book_num, text in books:
            f.write(f"=== BOOK {book_num} ===\n")
            f.write(text)
            f.write(f"\n\n")

    print(f"Also wrote simple format to {simple_path}")

    # Write a ready-to-translate file
    translate_path = output_path.replace('_extracted.txt', '_ko.txt')
    with open(translate_path, 'w', encoding='utf-8') as f:
        f.write("# Korean book translations for Ultima 6\n")
        f.write("# Format: BOOK_NUM|KOREAN_TEXT\n")
        f.write("# Use \\n for line breaks\n\n")

        for book_num, text in books:
            escaped_text = text.replace('\n', '\\n').replace('\r', '')
            f.write(f"# EN: {escaped_text[:100]}{'...' if len(escaped_text) > 100 else ''}\n")
            f.write(f"{book_num}|<번역 필요>\n\n")

    print(f"Also wrote translation template to {translate_path}")

if __name__ == "__main__":
    # Possible paths
    paths = [
        r"D:\proj\Ultima6\game_data\BOOK.DAT",
        r"D:\proj\Ultima6\game_data\book.dat",
        r"C:\games\Ultima6\BOOK.DAT",
        r"C:\games\Ultima6\book.dat",
    ]

    book_file = None
    for p in paths:
        if os.path.exists(p):
            book_file = p
            break

    if book_file:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        output = os.path.join(script_dir, "extracted", "books_extracted.txt")
        os.makedirs(os.path.dirname(output), exist_ok=True)

        print(f"Reading from: {book_file}")
        extract_books(book_file, output)
    else:
        print("Could not find book.dat")
        print("Tried:", paths)
