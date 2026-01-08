#!/usr/bin/env python3
"""
Ultima 6 CONVERSE.A/B Text Extractor
Extracts conversation text from Ultima 6 data files for translation.

Usage:
    python extract_converse.py <game_dir> <output_dir>

Example:
    python extract_converse.py "D:/proj/울티마6-4093-nahimjoa" ./extracted
"""

import sys
import os
import struct

class U6Lzw:
    """Simple LZW decompressor for Ultima 6 files"""

    def __init__(self):
        self.dict_size = 0
        self.code_size = 0
        self.dict = {}

    def decompress(self, data):
        """Decompress LZW data"""
        if len(data) < 4:
            return None

        # Check for uncompressed data (size == 0 in header)
        uncompressed_size = struct.unpack('<I', data[:4])[0]
        if uncompressed_size == 0:
            # Data is not compressed
            return data[4:]

        # Initialize dictionary
        self.dict = {i: bytes([i]) for i in range(256)}
        self.dict_size = 258  # 256 chars + CLEAR + END
        self.code_size = 9

        result = bytearray()
        bit_pos = 32  # Skip the uncompressed size header

        def read_code():
            nonlocal bit_pos
            byte_pos = bit_pos // 8
            bit_offset = bit_pos % 8

            if byte_pos + 2 >= len(data):
                return None

            # Read 3 bytes to get enough bits
            if byte_pos + 2 < len(data):
                value = data[byte_pos] | (data[byte_pos + 1] << 8) | (data[byte_pos + 2] << 16)
            else:
                value = data[byte_pos] | (data[byte_pos + 1] << 8)

            code = (value >> bit_offset) & ((1 << self.code_size) - 1)
            bit_pos += self.code_size
            return code

        prev_string = b''

        while True:
            code = read_code()
            if code is None:
                break

            # CLEAR code
            if code == 256:
                self.dict = {i: bytes([i]) for i in range(256)}
                self.dict_size = 258
                self.code_size = 9
                prev_string = b''
                continue

            # END code
            if code == 257:
                break

            if code < self.dict_size:
                string = self.dict.get(code, bytes([code]) if code < 256 else b'')
            elif code == self.dict_size and prev_string:
                string = prev_string + prev_string[0:1]
            else:
                # Invalid code
                break

            result.extend(string)

            if prev_string and self.dict_size < 4096:
                self.dict[self.dict_size] = prev_string + string[0:1]
                self.dict_size += 1

                # Increase code size when needed
                if self.dict_size >= (1 << self.code_size) and self.code_size < 12:
                    self.code_size += 1

            prev_string = string

            if len(result) >= uncompressed_size:
                break

        return bytes(result[:uncompressed_size])


def read_lib32_file(filepath):
    """Read a lib_32 format file (CONVERSE.A/B)"""
    with open(filepath, 'rb') as f:
        data = f.read()

    if len(data) < 4:
        return []

    # Read number of items (first 4 bytes / 4)
    # Actually, lib_32 stores offsets, not count
    # Find number of items by reading offsets until we hit data

    offsets = []
    pos = 0

    # Find first non-zero offset to determine item count
    first_offset = struct.unpack('<I', data[0:4])[0]

    # If first offset is 0, scan for the first non-zero offset
    if first_offset == 0:
        for i in range(len(data) // 4):
            offset = struct.unpack('<I', data[i*4:(i+1)*4])[0]
            if offset != 0:
                first_offset = offset
                break

    if first_offset == 0 or first_offset > len(data):
        return []

    num_items = first_offset // 4

    # Read all offsets
    for i in range(num_items):
        offset = struct.unpack('<I', data[i*4:(i+1)*4])[0]
        offsets.append(offset)

    # Extract items
    items = []
    for i in range(num_items):
        start = offsets[i]
        if start == 0:
            items.append(None)
            continue

        # End is next non-zero offset or end of file
        end = len(data)
        for j in range(i + 1, num_items):
            if offsets[j] != 0:
                end = offsets[j]
                break

        items.append(data[start:end])

    return items


def extract_text_from_conv(data):
    """Extract readable text from a conversation script"""
    if not data:
        return None, None, []

    # First two bytes: unknown, NPC number
    if len(data) < 3:
        return None, None, []

    npc_num = data[1]

    # Find NPC name (ends with 0xF1 or 0xF3)
    name_end = 2
    while name_end < len(data) and data[name_end] not in (0xF1, 0xF3):
        name_end += 1

    npc_name = data[2:name_end].decode('latin-1', errors='replace')

    # Extract all text strings
    texts = []
    i = name_end

    while i < len(data):
        byte = data[i]

        # Text markers and control codes
        if byte == 0xF1:  # Description
            i += 1
            text_start = i
            while i < len(data) and data[i] not in (0xF1, 0xF3, 0xEF, 0xF6, 0xF7, 0xF8, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF):
                i += 1
            if i > text_start:
                text = data[text_start:i].decode('latin-1', errors='replace')
                texts.append(('DESC', text))

        elif byte == 0xF6:  # Answer text
            i += 1
            text_start = i
            while i < len(data) and data[i] not in (0xF1, 0xF3, 0xEF, 0xF6, 0xF7, 0xF8, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF):
                i += 1
            if i > text_start:
                text = data[text_start:i].decode('latin-1', errors='replace')
                texts.append(('TEXT', text))

        elif byte == 0xEF:  # Keywords
            i += 1
            text_start = i
            while i < len(data) and data[i] not in (0xF1, 0xF3, 0xEF, 0xF6, 0xF7, 0xF8, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF):
                i += 1
            if i > text_start:
                text = data[text_start:i].decode('latin-1', errors='replace')
                texts.append(('KEYWORDS', text))

        else:
            i += 1

    return npc_num, npc_name, texts


def main():
    if len(sys.argv) < 3:
        print("Usage: python extract_converse.py <game_dir> <output_dir>")
        print("")
        print("Example:")
        print('  python extract_converse.py "D:/proj/울티마6-4093-nahimjoa" ./extracted')
        sys.exit(1)

    game_dir = sys.argv[1]
    output_dir = sys.argv[2]

    os.makedirs(output_dir, exist_ok=True)

    lzw = U6Lzw()

    for conv_file in ['CONVERSE.A', 'CONVERSE.B']:
        filepath = os.path.join(game_dir, conv_file)
        if not os.path.exists(filepath):
            print(f"Warning: {filepath} not found")
            continue

        print(f"\nProcessing {conv_file}...")

        items = read_lib32_file(filepath)
        print(f"  Found {len(items)} items")

        # Output file for all texts
        all_texts_file = os.path.join(output_dir, f"{conv_file.lower()}_texts.txt")
        with open(all_texts_file, 'w', encoding='utf-8') as out:
            out.write(f"# Extracted from {conv_file}\n")
            out.write(f"# Format: NPC_NUM|TYPE|TEXT\n\n")

            for idx, item in enumerate(items):
                if item is None:
                    continue

                # Decompress if needed
                decompressed = lzw.decompress(item)
                if decompressed is None:
                    print(f"  Item {idx}: Failed to decompress")
                    continue

                npc_num, npc_name, texts = extract_text_from_conv(decompressed)

                if npc_name:
                    out.write(f"\n# === NPC {npc_num}: {npc_name} ===\n")

                    # Also save individual NPC file
                    npc_file = os.path.join(output_dir, f"npc_{npc_num:03d}_{npc_name.replace(' ', '_')}.txt")
                    with open(npc_file, 'w', encoding='utf-8') as npc_out:
                        npc_out.write(f"# NPC {npc_num}: {npc_name}\n")
                        npc_out.write(f"# Source: {conv_file}\n\n")

                        for text_type, text in texts:
                            # Clean up text
                            text = text.replace('\r', ' ').replace('\n', ' ')
                            text = ' '.join(text.split())  # Normalize whitespace

                            if text.strip():
                                out.write(f"{npc_num}|{text_type}|{text}\n")
                                npc_out.write(f"[{text_type}]\n{text}\n\n")

                    print(f"  NPC {npc_num}: {npc_name} - {len(texts)} text entries")

        print(f"  Saved to {all_texts_file}")

    # Also extract BOOK.DAT
    book_file = os.path.join(game_dir, 'BOOK.DAT')
    if os.path.exists(book_file):
        print(f"\nProcessing BOOK.DAT...")
        extract_books(book_file, output_dir, lzw)

    # Extract LOOK.LZD (item descriptions)
    look_file = os.path.join(game_dir, 'LOOK.LZD')
    if os.path.exists(look_file):
        print(f"\nProcessing LOOK.LZD...")
        extract_look(look_file, output_dir, lzw)

    print("\nDone!")
    print(f"All extracted texts are in: {output_dir}")


def extract_books(filepath, output_dir, lzw):
    """Extract book texts from BOOK.DAT"""
    with open(filepath, 'rb') as f:
        data = f.read()

    # BOOK.DAT may have different format - try lib_32 first
    try:
        items = read_lib32_file(filepath)
    except:
        # If lib_32 fails, try as single compressed file
        items = [data]

    books_file = os.path.join(output_dir, "books.txt")
    book_count = 0
    with open(books_file, 'w', encoding='utf-8') as out:
        out.write("# Books from BOOK.DAT\n\n")

        for idx, item in enumerate(items):
            if item is None:
                continue

            # Try to decompress
            decompressed = lzw.decompress(item)
            if decompressed is None:
                decompressed = item

            # Book text is usually plain text
            try:
                text = decompressed.decode('latin-1', errors='replace')
                text = text.replace('\r', '\n')
                if text.strip():
                    out.write(f"=== Book {idx} ===\n")
                    out.write(text)
                    out.write("\n\n")
                    book_count += 1
            except:
                pass

    print(f"  Extracted {book_count} books to {books_file}")


def extract_look(filepath, output_dir, lzw):
    """Extract item descriptions from LOOK.LZD"""
    with open(filepath, 'rb') as f:
        data = f.read()

    # Decompress
    decompressed = lzw.decompress(data)
    if decompressed is None:
        print("  Failed to decompress LOOK.LZD")
        return

    look_file = os.path.join(output_dir, "look_items.txt")
    with open(look_file, 'w', encoding='utf-8') as out:
        out.write("# Item descriptions from LOOK.LZD\n")
        out.write("# Format: INDEX|NAME\n\n")

        # Parse null-terminated strings
        idx = 0
        item_num = 0
        while idx < len(decompressed):
            # Find end of string
            end = idx
            while end < len(decompressed) and decompressed[end] != 0:
                end += 1

            if end > idx:
                text = decompressed[idx:end].decode('latin-1', errors='replace')
                out.write(f"{item_num}|{text}\n")
                item_num += 1

            idx = end + 1

    print(f"  Saved to {look_file}")


if __name__ == '__main__':
    main()
