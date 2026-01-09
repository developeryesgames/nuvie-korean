#!/usr/bin/env python3
"""
Ultima 6 CONVERSE.A/CONVERSE.B 바이너리에서 NPC 대화 추출
nuvie의 U6Lzw, U6Lib_n, ConvScript 로직을 Python으로 구현

출력 형식:
- DESC (따옴표 없음): NPC번호|영어 설명|<번역 필요>
- 대사 (따옴표 있음): NPC번호|"영어 대사"|"<번역 필요>"
"""

import struct
import os
import sys
from pathlib import Path

# =============================================================================
# U6 LZW Decompression (from nuvie U6Lzw.cpp)
# =============================================================================

class U6LzwDict:
    """LZW Dictionary"""
    DICT_SIZE = 10000

    def __init__(self):
        self.dict = [(0, 0) for _ in range(self.DICT_SIZE)]  # (root, codeword)
        self.contains = 0x102

    def reset(self):
        self.contains = 0x102

    def add(self, root, codeword):
        self.dict[self.contains] = (root, codeword)
        self.contains += 1

    def get_root(self, codeword):
        return self.dict[codeword][0]

    def get_codeword(self, codeword):
        return self.dict[codeword][1]


class U6Lzw:
    """Ultima 6 LZW Decompressor"""

    def __init__(self):
        self.dict = U6LzwDict()
        self.stack = []

    def is_valid_lzw_buffer(self, buf):
        if len(buf) < 6:
            return False
        if buf[3] != 0:
            return False
        if buf[4] != 0 or (buf[5] & 1) != 1:
            return False
        return True

    def get_uncompressed_size(self, buf):
        if not self.is_valid_lzw_buffer(buf):
            return -1
        return buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24)

    def get_next_codeword(self, source, bits_read, codeword_size):
        byte_pos = bits_read // 8
        b0 = source[byte_pos] if byte_pos < len(source) else 0
        b1 = source[byte_pos + 1] if byte_pos + 1 < len(source) else 0
        b2 = 0
        if codeword_size + (bits_read % 8) > 16:
            b2 = source[byte_pos + 2] if byte_pos + 2 < len(source) else 0

        codeword = (b2 << 16) + (b1 << 8) + b0
        codeword = codeword >> (bits_read % 8)

        mask = {9: 0x1ff, 10: 0x3ff, 11: 0x7ff, 12: 0xfff}
        codeword = codeword & mask.get(codeword_size, 0)

        return codeword

    def get_string(self, codeword):
        """Build string on stack from dictionary"""
        self.stack = []
        current = codeword
        while current > 0xff:
            root = self.dict.get_root(current)
            current = self.dict.get_codeword(current)
            self.stack.append(root)
        self.stack.append(current)

    def decompress_buffer(self, source):
        """Decompress LZW buffer"""
        if not self.is_valid_lzw_buffer(source):
            return None

        dest_len = self.get_uncompressed_size(source)
        if dest_len <= 0:
            return None

        destination = bytearray(dest_len)
        bytes_written = 0

        max_codeword_length = 12
        codeword_size = 9
        bits_read = 0
        next_free_codeword = 0x102
        dictionary_size = 0x200

        source = source[4:]  # Skip 4-byte size header
        self.dict.reset()

        pW = 0
        end_reached = False

        while not end_reached:
            cW = self.get_next_codeword(source, bits_read, codeword_size)
            bits_read += codeword_size

            if cW == 0x100:  # Reset dictionary
                codeword_size = 9
                next_free_codeword = 0x102
                dictionary_size = 0x200
                self.dict.reset()
                cW = self.get_next_codeword(source, bits_read, codeword_size)
                bits_read += codeword_size
                if bytes_written < dest_len:
                    destination[bytes_written] = cW & 0xff
                    bytes_written += 1
            elif cW == 0x101:  # End marker
                end_reached = True
            else:
                if cW < next_free_codeword:
                    # Codeword in dictionary
                    self.get_string(cW)
                    C = self.stack[-1] if self.stack else 0
                    while self.stack:
                        if bytes_written < dest_len:
                            destination[bytes_written] = self.stack.pop()
                            bytes_written += 1
                        else:
                            self.stack.pop()
                    self.dict.add(C, pW)
                    next_free_codeword += 1
                    if next_free_codeword >= dictionary_size:
                        if codeword_size < max_codeword_length:
                            codeword_size += 1
                            dictionary_size *= 2
                else:
                    # Codeword not yet defined
                    self.get_string(pW)
                    C = self.stack[-1] if self.stack else 0
                    while self.stack:
                        if bytes_written < dest_len:
                            destination[bytes_written] = self.stack.pop()
                            bytes_written += 1
                        else:
                            self.stack.pop()
                    if bytes_written < dest_len:
                        destination[bytes_written] = C
                        bytes_written += 1
                    self.dict.add(C, pW)
                    next_free_codeword += 1
                    if next_free_codeword >= dictionary_size:
                        if codeword_size < max_codeword_length:
                            codeword_size += 1
                            dictionary_size *= 2

            pW = cW

        return bytes(destination)


# =============================================================================
# U6Lib_n Parser (from nuvie U6Lib_n.cpp)
# =============================================================================

class U6LibItem:
    def __init__(self):
        self.offset = 0
        self.flag = 0
        self.size = 0
        self.uncomp_size = 0


class U6Lib_n:
    """Ultima 6 Library File Parser"""

    def __init__(self, filename, lib_size=4):
        self.filename = filename
        self.lib_size = lib_size
        self.items = []
        self.data = None
        self.filesize = 0
        self.lzw = U6Lzw()

        self._load()

    def _load(self):
        with open(self.filename, 'rb') as f:
            self.data = f.read()
        self.filesize = len(self.data)
        self._parse_lib()

    def _parse_lib(self):
        """Parse library offset table"""
        num_offsets = self._calculate_num_offsets()

        # Read offsets
        pos = 0
        for i in range(num_offsets):
            item = U6LibItem()
            if self.lib_size == 4:
                offset_raw = struct.unpack_from('<I', self.data, pos)[0]
                item.flag = (offset_raw >> 24) & 0xff
                item.offset = offset_raw & 0xffffff
            else:
                item.offset = struct.unpack_from('<H', self.data, pos)[0]
            pos += self.lib_size
            self.items.append(item)

        # Add sentinel for size calculation
        sentinel = U6LibItem()
        sentinel.offset = self.filesize
        self.items.append(sentinel)

        # Calculate sizes
        self._calculate_item_sizes()

    def _calculate_num_offsets(self):
        """Calculate number of items in library"""
        max_count = 0xffffffff
        pos = 0
        i = 0

        while pos < len(self.data):
            if i == max_count:
                return i

            if self.lib_size == 4:
                if pos + 4 > len(self.data):
                    break
                offset_raw = struct.unpack_from('<I', self.data, pos)[0]
                offset = offset_raw & 0xffffff
            else:
                if pos + 2 > len(self.data):
                    break
                offset = struct.unpack_from('<H', self.data, pos)[0]

            pos += self.lib_size

            if offset != 0:
                if offset // self.lib_size < max_count:
                    max_count = offset // self.lib_size

            i += 1

        return max_count if max_count != 0xffffffff else 0

    def _calculate_item_sizes(self):
        """Calculate item sizes from offsets"""
        num_items = len(self.items) - 1  # Exclude sentinel

        for i in range(num_items):
            self.items[i].size = 0

            # Find next non-zero offset
            next_offset = 0
            for o in range(i + 1, len(self.items)):
                if self.items[o].offset:
                    next_offset = self.items[o].offset
                    break

            if self.items[i].offset and next_offset > self.items[i].offset:
                self.items[i].size = next_offset - self.items[i].offset

            # Calculate uncompressed size
            self.items[i].uncomp_size = self._calc_uncomp_size(self.items[i])

    def _calc_uncomp_size(self, item):
        """Calculate uncompressed size based on flag"""
        if item.flag in (0x01, 0x20):  # Compressed
            if item.offset + 4 <= len(self.data):
                return struct.unpack_from('<I', self.data, item.offset)[0]
        return item.size

    def is_compressed(self, item_number):
        """Check if item is LZW compressed"""
        if item_number >= len(self.items) - 1:
            return False
        flag = self.items[item_number].flag
        if flag in (0x01, 0x20):
            return True
        if flag == 0xff:
            # Look for next item with valid flag
            for i in range(item_number, len(self.items) - 1):
                if self.items[i].flag != 0xff:
                    return self.is_compressed(i)
        return False

    def get_num_items(self):
        return len(self.items) - 1

    def get_item(self, item_number):
        """Get decompressed item data"""
        if item_number >= len(self.items) - 1:
            return None

        item = self.items[item_number]
        if item.size == 0 or item.offset == 0:
            return None

        raw_data = self.data[item.offset:item.offset + item.size]

        # Check for U6 converse compression
        # First 4 bytes are 0 means uncompressed
        if len(raw_data) >= 4:
            first4 = struct.unpack_from('<I', raw_data, 0)[0]
            if first4 == 0:
                # Uncompressed - skip 4 byte header
                return raw_data[4:]
            else:
                # Try LZW decompression
                decompressed = self.lzw.decompress_buffer(raw_data)
                if decompressed:
                    return decompressed

        return raw_data


# =============================================================================
# ConvScript Parser (from nuvie Converse.cpp, ConverseInterpret.cpp)
# =============================================================================

# Script opcodes
U6OP_LOOK = 0xf1       # Look description start
U6OP_CONVERSE = 0xf2   # Converse/dialogue section
U6OP_NAME = 0xf3       # Name section
U6OP_JMP = 0xb0        # Jump
U6OP_ENDIF = 0xb1      # End if
U6OP_SETF = 0xb2       # Set flag
U6OP_CLEARF = 0xb3     # Clear flag
U6OP_DECL = 0xb4       # Declare variable
U6OP_ASSIGN = 0xb6     # Assignment
U6OP_IF = 0xb7         # If statement
U6OP_EVAL = 0xb9       # Evaluate
U6OP_NEW = 0xa7        # New NPC
U6OP_DELETE = 0xa8     # Delete NPC
U6OP_ASK = 0xee        # Ask for input
U6OP_KEYWORDS = 0xef   # Keywords section
U6OP_ENDANSWER = 0xf6  # End answer
U6OP_ENDASK = 0xf7     # End ask
U6OP_KEYWORD = 0xf8    # Keyword
U6OP_SIDENT = 0xf9     # String identifier
U6OP_SLOOK = 0xfa      # String look
U6OP_SCONVERSE = 0xfb  # String converse
U6OP_SPREFIX = 0xfc    # String prefix
U6OP_ANSWER = 0xfd     # Answer
U6OP_SANSWER = 0xfe    # String answer
U6OP_BYE = 0xff        # Bye/end


class ConvScriptParser:
    """Parse Ultima 6 conversation script and extract text"""

    def __init__(self, script_data, npc_num):
        self.data = script_data
        self.npc_num = npc_num
        self.pos = 0
        self.name = ""
        self.desc = ""
        self.dialogues = []  # List of dialogue strings

    def parse(self):
        """Parse the script and extract all text"""
        if not self.data or len(self.data) < 3:
            return False

        # First 2 bytes are script size
        script_size = struct.unpack_from('<H', self.data, 0)[0]
        self.pos = 2

        # Read name (until 0xf1 LOOK or 0xf3 NAME marker)
        name_chars = []
        while self.pos < len(self.data):
            b = self.data[self.pos]
            if b in (U6OP_LOOK, U6OP_NAME, U6OP_CONVERSE):
                break
            # Convert underscore to period for names like "Dr._Cat"
            if b == ord('_'):
                name_chars.append('.')
            elif 0x20 <= b < 0x7f:
                name_chars.append(chr(b))
            self.pos += 1

        self.name = ''.join(name_chars).strip()

        # Parse rest of script
        while self.pos < len(self.data):
            b = self.data[self.pos]
            self.pos += 1

            if b == U6OP_LOOK:  # 0xf1 - Look description
                self._parse_look()
            elif b == U6OP_CONVERSE:  # 0xf2 - Start of dialogue section
                self._parse_converse()
            elif b == U6OP_NAME:  # 0xf3 - Name section (alternative)
                pass  # Name already parsed

        return True

    def _parse_look(self):
        """Parse LOOK section (DESC)"""
        # Read until we hit an opcode, * (pause marker), or end
        desc_chars = []
        while self.pos < len(self.data):
            b = self.data[self.pos]
            # Stop at control codes/opcodes
            if b >= 0xa0 or b == 0:
                break
            # Stop at * (pause marker) - this separates DESC from subsequent text
            if b == ord('*'):
                self.pos += 1
                break
            if 0x20 <= b < 0x7f or b in (0x0a, 0x0d):  # Printable or newline
                desc_chars.append(chr(b))
            self.pos += 1

        self.desc = ''.join(desc_chars).strip()

    def _parse_converse(self):
        """Parse CONVERSE section (dialogues)"""
        while self.pos < len(self.data):
            b = self.data[self.pos]

            if b == U6OP_BYE:  # 0xff - End
                break

            if b == 0x00:  # Null byte
                self.pos += 1
                continue

            # Text string (starts with printable character or ")
            if 0x20 <= b < 0x7f:
                text = self._read_text_string()
                if text:
                    self.dialogues.append(text)
            elif b == U6OP_KEYWORDS:  # 0xef - Keywords
                self.pos += 1
                self._skip_keywords()
            elif b == U6OP_ASK:  # 0xee - Ask
                self.pos += 1
            elif b == U6OP_ANSWER:  # 0xfd - Answer
                self.pos += 1
            elif b == U6OP_SANSWER:  # 0xfe - String Answer
                self.pos += 1
            elif b == U6OP_ENDANSWER:  # 0xf6
                self.pos += 1
            elif b == U6OP_ENDASK:  # 0xf7
                self.pos += 1
            elif b == U6OP_KEYWORD:  # 0xf8
                self.pos += 1
                self._skip_keywords()
            elif b == U6OP_SIDENT:  # 0xf9 - String identifier
                self.pos += 1
            elif b == U6OP_SLOOK:  # 0xfa
                self.pos += 1
            elif b == U6OP_SCONVERSE:  # 0xfb
                self.pos += 1
            elif b == U6OP_SPREFIX:  # 0xfc - String prefix (variables etc)
                self.pos += 1
                self._handle_sprefix()
            elif b in (U6OP_JMP, U6OP_IF):  # Jump/If with 2-byte offset
                self.pos += 3  # opcode + 2 bytes
            elif b in (U6OP_EVAL,):  # Eval
                self.pos += 1
                self._skip_eval()
            elif b in (U6OP_SETF, U6OP_CLEARF):  # Set/Clear flag
                self.pos += 2  # opcode + 1 byte
            elif b == U6OP_DECL:  # Declare
                self.pos += 3  # opcode + 2 bytes
            elif b == U6OP_ASSIGN:  # Assignment
                self.pos += 1
                self._skip_assignment()
            elif b == U6OP_ENDIF:  # End if
                self.pos += 1
            elif b in (U6OP_NEW, U6OP_DELETE):  # New/Delete NPC
                self.pos += 2
            else:
                self.pos += 1

    def _read_text_string(self):
        """Read a text string until opcode or special marker"""
        text_chars = []
        in_quotes = False

        while self.pos < len(self.data):
            b = self.data[self.pos]

            # Check for opcodes/control bytes
            if b >= 0xa0 and b not in (0xa0,):  # Allow some characters
                break

            if b == 0:  # Null terminator
                self.pos += 1
                break

            # Handle special formatting
            if b == ord('"'):
                in_quotes = not in_quotes
                text_chars.append('"')
            elif b == ord('*'):  # Pause marker in U6
                text_chars.append(' ')
            elif b == ord('@'):  # Keyword highlight
                text_chars.append('@')
            elif b == ord('$'):  # Variable
                text_chars.append('$')
            elif 0x20 <= b < 0x7f:  # Printable ASCII
                text_chars.append(chr(b))
            elif b == 0x0a:  # Newline
                text_chars.append(' ')

            self.pos += 1

        text = ''.join(text_chars).strip()

        # Clean up the text
        text = ' '.join(text.split())  # Normalize whitespace

        return text if text and len(text) > 1 else ""

    def _skip_keywords(self):
        """Skip keyword section"""
        while self.pos < len(self.data):
            b = self.data[self.pos]
            if b == 0 or b >= 0xa0:
                if b == 0:
                    self.pos += 1
                break
            self.pos += 1

    def _skip_eval(self):
        """Skip evaluation expression"""
        depth = 1
        while self.pos < len(self.data) and depth > 0:
            b = self.data[self.pos]
            self.pos += 1
            if b == 0xa1:  # End eval marker
                depth -= 1
            elif b == 0xb9:  # Nested eval
                depth += 1

    def _skip_assignment(self):
        """Skip assignment statement"""
        # Skip variable reference
        while self.pos < len(self.data):
            b = self.data[self.pos]
            if b == 0xb6 or b >= 0xa0:
                break
            self.pos += 1
        # Skip value
        self._skip_eval()

    def _handle_sprefix(self):
        """Handle string prefix (variables like $P, $G, etc.)"""
        if self.pos >= len(self.data):
            return
        # Just advance past it - the variable reference
        b = self.data[self.pos]
        if 0x20 <= b < 0x7f:
            self.pos += 1


def extract_dialogues_from_lib(lib_path, start_npc, output_file):
    """Extract all dialogues from a CONVERSE library file"""
    lib = U6Lib_n(lib_path)
    num_items = lib.get_num_items()

    results = []

    for i in range(num_items):
        npc_num = start_npc + i

        script_data = lib.get_item(i)
        if not script_data:
            continue

        parser = ConvScriptParser(script_data, npc_num)
        if not parser.parse():
            continue

        # Skip if no name
        if not parser.name:
            continue

        # Add DESC (no quotes)
        if parser.desc:
            # Clean the description
            desc = parser.desc.strip()
            if desc:
                results.append((npc_num, 'DESC', desc))

        # Add dialogues (with quotes)
        for dialogue in parser.dialogues:
            # Only add if it looks like actual dialogue (has quotes or substantial text)
            text = dialogue.strip()
            if text and len(text) > 1:
                # Check if it starts with quote
                if text.startswith('"') and text.endswith('"'):
                    results.append((npc_num, 'DIALOGUE', text))
                elif '"' in text:
                    # Has quotes somewhere
                    results.append((npc_num, 'DIALOGUE', text))

    return results


def main():
    # Paths
    game_data = Path("d:/proj/Ultima6/game_data")
    output_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    converse_a = game_data / "CONVERSE.A"
    converse_b = game_data / "CONVERSE.B"

    if not converse_a.exists():
        print(f"Error: {converse_a} not found")
        return 1

    if not converse_b.exists():
        print(f"Error: {converse_b} not found")
        return 1

    print("=" * 60)
    print("Ultima 6 NPC Dialogue Extractor (Clean)")
    print("=" * 60)

    all_results = []

    # Extract from CONVERSE.A (NPCs 0-98)
    print(f"\nProcessing {converse_a}...")
    results_a = extract_dialogues_from_lib(str(converse_a), 0, None)
    all_results.extend(results_a)
    print(f"  Extracted {len(results_a)} entries")

    # Extract from CONVERSE.B (NPCs 99+)
    print(f"\nProcessing {converse_b}...")
    results_b = extract_dialogues_from_lib(str(converse_b), 99, None)
    all_results.extend(results_b)
    print(f"  Extracted {len(results_b)} entries")

    # Sort by NPC number
    all_results.sort(key=lambda x: (x[0], x[1] == 'DIALOGUE'))

    # Write output
    output_file = output_dir / "dialogues_ko_clean.txt"
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# Ultima 6 Korean Translation\n")
        f.write("# Format:\n")
        f.write("#   DESC (no quotes): NPC_NUM|english desc|<번역 필요>\n")
        f.write("#   Dialogue (quotes): NPC_NUM|\"english\"|\"<번역 필요>\"\n")
        f.write("#\n")
        f.write("# Variables: $P=player name, $G=gender title, $T=time, $N=NPC name, $Z=input\n")
        f.write("# @keyword = highlighted keyword (preserve as-is)\n")
        f.write("#\n\n")

        current_npc = -1
        desc_count = 0
        dialogue_count = 0

        for npc_num, entry_type, text in all_results:
            if npc_num != current_npc:
                if current_npc >= 0:
                    f.write("\n")
                current_npc = npc_num

            if entry_type == 'DESC':
                # DESC without quotes
                f.write(f"{npc_num}|{text}|<번역 필요>\n")
                desc_count += 1
            else:
                # Dialogue with quotes
                # Ensure proper quoting
                if not text.startswith('"'):
                    text = f'"{text}'
                if not text.endswith('"'):
                    text = f'{text}"'
                f.write(f'{npc_num}|{text}|"<번역 필요>"\n')
                dialogue_count += 1

    print(f"\n" + "=" * 60)
    print(f"Output: {output_file}")
    print(f"DESC entries: {desc_count}")
    print(f"Dialogue entries: {dialogue_count}")
    print(f"Total: {desc_count + dialogue_count}")
    print("=" * 60)

    return 0


if __name__ == "__main__":
    sys.exit(main())
