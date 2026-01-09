#!/usr/bin/env python3
"""
Ultima 6 CONVERSE.A/CONVERSE.B 바이너리에서 NPC 대화 추출 v3
nuvie의 ConverseInterpret.cpp 로직을 완벽히 구현

출력 형식:
- DESC (따옴표 없음): NPC번호|영어 설명|<번역 필요>
- 대사 (따옴표 있음): NPC번호|"영어 대사"|"<번역 필요>"
"""

import struct
import sys
from pathlib import Path

# =============================================================================
# U6 LZW Decompression
# =============================================================================

class U6LzwDict:
    DICT_SIZE = 10000
    def __init__(self):
        self.dict = [(0, 0) for _ in range(self.DICT_SIZE)]
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
        return codeword & mask.get(codeword_size, 0)

    def get_string(self, codeword):
        self.stack = []
        current = codeword
        while current > 0xff:
            root = self.dict.get_root(current)
            current = self.dict.get_codeword(current)
            self.stack.append(root)
        self.stack.append(current)

    def decompress_buffer(self, source):
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
        source = source[4:]
        self.dict.reset()
        pW = 0
        end_reached = False
        while not end_reached:
            cW = self.get_next_codeword(source, bits_read, codeword_size)
            bits_read += codeword_size
            if cW == 0x100:
                codeword_size = 9
                next_free_codeword = 0x102
                dictionary_size = 0x200
                self.dict.reset()
                cW = self.get_next_codeword(source, bits_read, codeword_size)
                bits_read += codeword_size
                if bytes_written < dest_len:
                    destination[bytes_written] = cW & 0xff
                    bytes_written += 1
            elif cW == 0x101:
                end_reached = True
            else:
                if cW < next_free_codeword:
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
                    if next_free_codeword >= dictionary_size and codeword_size < max_codeword_length:
                        codeword_size += 1
                        dictionary_size *= 2
                else:
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
                    if next_free_codeword >= dictionary_size and codeword_size < max_codeword_length:
                        codeword_size += 1
                        dictionary_size *= 2
            pW = cW
        return bytes(destination)


# =============================================================================
# U6Lib_n Parser
# =============================================================================

class U6LibItem:
    def __init__(self):
        self.offset = 0
        self.flag = 0
        self.size = 0


class U6Lib_n:
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
        num_offsets = self._calculate_num_offsets()
        pos = 0
        for _ in range(num_offsets):
            item = U6LibItem()
            if self.lib_size == 4:
                offset_raw = struct.unpack_from('<I', self.data, pos)[0]
                item.flag = (offset_raw >> 24) & 0xff
                item.offset = offset_raw & 0xffffff
            else:
                item.offset = struct.unpack_from('<H', self.data, pos)[0]
            pos += self.lib_size
            self.items.append(item)
        sentinel = U6LibItem()
        sentinel.offset = self.filesize
        self.items.append(sentinel)
        self._calculate_item_sizes()

    def _calculate_num_offsets(self):
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
        num_items = len(self.items) - 1
        for i in range(num_items):
            self.items[i].size = 0
            next_offset = 0
            for o in range(i + 1, len(self.items)):
                if self.items[o].offset:
                    next_offset = self.items[o].offset
                    break
            if self.items[i].offset and next_offset > self.items[i].offset:
                self.items[i].size = next_offset - self.items[i].offset

    def get_num_items(self):
        return len(self.items) - 1

    def get_item(self, item_number):
        if item_number >= len(self.items) - 1:
            return None
        item = self.items[item_number]
        if item.size == 0 or item.offset == 0:
            return None
        raw_data = self.data[item.offset:item.offset + item.size]
        if len(raw_data) >= 4:
            first4 = struct.unpack_from('<I', raw_data, 0)[0]
            if first4 == 0:
                return raw_data[4:]
            else:
                decompressed = self.lzw.decompress_buffer(raw_data)
                if decompressed:
                    return decompressed
        return raw_data


# =============================================================================
# U6 Opcodes (from ConverseInterpret.h)
# =============================================================================

# Value opcodes (used in expressions)
VALOPS = {
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86,  # GT, GE, LT, LE, NE, EQ
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95,  # ADD, SUB, MUL, DIV, LOR, LAND
    0x9a, 0x9b, 0x9d, 0x9f, 0xa0, 0xa7,  # CANCARRY, WEIGHT, HORSED, HASOBJ, RAND, EVAL
    0xab, 0xb2, 0xb3, 0xb4, 0xb7, 0xbb,  # FLAG, VAR, SVAR, DATA, INDEXOF, OBJCOUNT
    0xc6, 0xc7, 0xca, 0xcc, 0xd7,        # INPARTY, OBJINPARTY, JOIN, LEAVE, NPCNEARBY
    0xda, 0xdc, 0xdd, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4  # WOUNDED, POISONED, NPC, EXP, LVL, STR, INT, DEX
}

# Data size prefixes
DATASIZES = {0xd2, 0xd3, 0xd4}

# Control opcodes
U6OP_HORSE = 0x9c
U6OP_SLEEP = 0x9e
U6OP_IF = 0xa1
U6OP_ENDIF = 0xa2
U6OP_ELSE = 0xa3
U6OP_SETF = 0xa4
U6OP_CLEARF = 0xa5
U6OP_DECL = 0xa6
U6OP_EVAL = 0xa7
U6OP_ASSIGN = 0xa8
U6OP_JUMP = 0xb0
U6OP_DPRINT = 0xb5
U6OP_BYE = 0xb6
U6OP_NEW = 0xb9
U6OP_DELETE = 0xba
U6OP_INVENTORY = 0xbe
U6OP_PORTRAIT = 0xbf
U6OP_ADDKARMA = 0xc4
U6OP_SUBKARMA = 0xc5
U6OP_GIVE = 0xc9
U6OP_WAIT = 0xcb
U6OP_WORKTYPE = 0xcd
U6OP_RESURRECT = 0xd6
U6OP_SETNAME = 0xd8
U6OP_HEAL = 0xd9
U6OP_CURE = 0xdb
U6OP_ENDANSWER = 0xee
U6OP_KEYWORDS = 0xef
U6OP_SLOOK = 0xf1
U6OP_SCONVERSE = 0xf2
U6OP_SPREFIX = 0xf3
U6OP_ANSWER = 0xf6
U6OP_ASK = 0xf7
U6OP_ASKC = 0xf8
U6OP_INPUTSTR = 0xf9
U6OP_INPUT = 0xfb
U6OP_INPUTNUM = 0xfc
U6OP_SIDENT = 0xff


def is_print(b):
    """Check if byte is printable text (from nuvie is_print)"""
    return b == 0x0a or (0x20 <= b <= 0x7a) or b == 0x7e or b == 0x7b


def is_ctrl(b):
    """Check if byte is control code (from nuvie is_ctrl)"""
    if b in VALOPS or b in DATASIZES:
        return False
    return b >= 0xa1 or b == 0x9c or b == 0x9e


# =============================================================================
# ConvScript Parser - nuvie 로직 완벽 구현
# =============================================================================

class ConvScriptParser:
    """Parse Ultima 6 conversation script exactly like nuvie"""

    def __init__(self, data, npc_num):
        self.data = data
        self.npc_num = npc_num
        self.pos = 0
        self.name = ""
        self.desc = ""
        self.dialogues = []

    def peek(self, offset=0):
        """Look at byte without advancing"""
        p = self.pos + offset
        if p < len(self.data):
            return self.data[p]
        return 0

    def read(self):
        """Read one byte and advance"""
        if self.pos < len(self.data):
            b = self.data[self.pos]
            self.pos += 1
            return b
        return 0

    def read2(self):
        """Read 2-byte little-endian value"""
        if self.pos + 2 <= len(self.data):
            val = self.data[self.pos] + (self.data[self.pos + 1] << 8)
            self.pos += 2
            return val
        return 0

    def read4(self):
        """Read 4-byte little-endian value"""
        if self.pos + 4 <= len(self.data):
            val = struct.unpack_from('<I', self.data, self.pos)[0]
            self.pos += 4
            return val
        return 0

    def overflow(self, offset=0):
        return self.pos + offset >= len(self.data)

    def collect_text(self):
        """Collect text until non-printable byte"""
        text_chars = []
        while not self.overflow() and is_print(self.peek()):
            text_chars.append(chr(self.read()))
        return ''.join(text_chars)

    def skip_value(self):
        """Skip a value (respecting datasize prefixes)"""
        b = self.read()
        if b == 0xd3:  # 1-byte value follows
            self.read()
        elif b == 0xd2:  # 4-byte value follows
            self.read4()
        elif b == 0xd4:  # 2-byte value follows
            self.read2()

    def skip_eval(self):
        """Skip an evaluation expression until we hit a control code"""
        while not self.overflow():
            b = self.peek()
            if is_ctrl(b) and b != U6OP_EVAL:
                break
            if is_print(b):
                break
            if b == U6OP_EVAL:  # 0xa7
                self.read()
                continue
            self.skip_value()

    def parse(self):
        """Parse the script and extract all text"""
        if not self.data or len(self.data) < 3:
            return False

        # First 2 bytes are script size
        self.pos = 2

        # Read name (until SLOOK/SIDENT/SCONVERSE marker)
        name_chars = []
        while not self.overflow():
            b = self.peek()
            if b in (U6OP_SLOOK, U6OP_SIDENT, U6OP_SCONVERSE):
                break
            if b == ord('_'):
                name_chars.append('.')
                self.read()
            elif is_print(b):
                name_chars.append(chr(self.read()))
            else:
                break
        self.name = ''.join(name_chars).strip()

        if not self.name:
            return False

        # Process rest of script
        while not self.overflow():
            b = self.peek()

            if is_print(b):
                # Collect and save text
                text = self.collect_text()
                if text.strip():
                    self.dialogues.append(text)

            elif is_ctrl(b):
                self._handle_control()

            else:
                self.read()  # Skip unknown byte

        return True

    def _handle_control(self):
        """Handle control code"""
        b = self.read()

        if b == U6OP_JUMP:  # 0xb0 - Jump (4-byte address)
            self.read4()

        elif b == U6OP_SIDENT:  # 0xff - NPC identity
            self.read()  # NPC number
            text = self.collect_text()  # Name

        elif b in (U6OP_KEYWORDS, U6OP_ASKC, U6OP_SLOOK):  # 0xef, 0xf8, 0xf1
            text = self.collect_text()
            if b == U6OP_SLOOK:  # DESC
                self.desc = text.strip()

        elif b == U6OP_IF:  # 0xa1
            self.skip_eval()

        elif b in (U6OP_SETF, U6OP_CLEARF):  # 0xa4, 0xa5
            self.skip_eval()  # npc
            self.skip_eval()  # flag

        elif b == U6OP_DECL:  # 0xa6
            self.skip_eval()  # variable
            self.skip_eval()  # type

        elif b == U6OP_ASSIGN:  # 0xa8
            self.skip_eval()

        elif b == U6OP_SETNAME:  # 0xd8
            self.skip_eval()

        elif b == U6OP_DPRINT:  # 0xb5
            self.skip_eval()  # db location
            self.skip_eval()  # index

        elif b == U6OP_NEW:  # 0xb9
            self.skip_eval()  # npc
            self.skip_eval()  # obj
            self.skip_eval()  # qual
            self.skip_eval()  # quant

        elif b == U6OP_DELETE:  # 0xba
            self.skip_eval()  # npc
            self.skip_eval()  # obj
            self.skip_eval()  # qual
            self.skip_eval()  # quant

        elif b == U6OP_GIVE:  # 0xc9
            self.skip_eval()  # obj
            self.skip_eval()  # qual
            self.skip_eval()  # from
            self.skip_eval()  # to

        elif b in (U6OP_ADDKARMA, U6OP_SUBKARMA):  # 0xc4, 0xc5
            self.skip_eval()

        elif b == U6OP_WORKTYPE:  # 0xcd
            self.skip_eval()  # npc
            self.skip_eval()  # worktype

        elif b in (U6OP_PORTRAIT, U6OP_RESURRECT, U6OP_HEAL, U6OP_CURE, U6OP_HORSE):
            self.skip_eval()  # npc

        elif b in (U6OP_INPUTSTR, U6OP_INPUT, U6OP_INPUTNUM):  # 0xf9, 0xfb, 0xfc
            self.skip_eval()  # var
            self.skip_eval()  # type

        # Simple opcodes with no arguments
        elif b in (U6OP_ENDIF, U6OP_ELSE, U6OP_BYE, U6OP_WAIT, U6OP_INVENTORY,
                   U6OP_ENDANSWER, U6OP_SCONVERSE, U6OP_SPREFIX, U6OP_ANSWER,
                   U6OP_ASK, U6OP_SLEEP):
            pass


def extract_from_lib(lib_path, start_npc):
    """Extract all dialogues from library file"""
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
        if not parser.name:
            continue

        # Add DESC
        if parser.desc:
            # Clean DESC - remove trailing * and text after it
            desc = parser.desc
            star_pos = desc.find('*')
            if star_pos != -1:
                desc = desc[:star_pos].strip()
            if desc:
                results.append((npc_num, 'DESC', desc))

        # Add dialogues
        for dialogue in parser.dialogues:
            text = dialogue.strip()
            if not text:
                continue

            # Clean up text
            text = text.replace('\n', ' ').replace('\r', ' ')
            text = ' '.join(text.split())

            # Only include if it has quotes (actual dialogue)
            if '"' in text and len(text) > 2:
                results.append((npc_num, 'DIALOGUE', text))

    return results


def main():
    game_data = Path("d:/proj/Ultima6/game_data")
    output_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    converse_a = game_data / "CONVERSE.A"
    converse_b = game_data / "CONVERSE.B"

    if not converse_a.exists() or not converse_b.exists():
        print("Error: CONVERSE files not found")
        return 1

    print("=" * 60)
    print("Ultima 6 NPC Dialogue Extractor v3 (nuvie-accurate)")
    print("=" * 60)

    all_results = []

    print(f"\nProcessing {converse_a}...")
    results_a = extract_from_lib(str(converse_a), 0)
    all_results.extend(results_a)
    print(f"  Extracted {len(results_a)} entries")

    print(f"\nProcessing {converse_b}...")
    results_b = extract_from_lib(str(converse_b), 99)
    all_results.extend(results_b)
    print(f"  Extracted {len(results_b)} entries")

    # Sort and deduplicate
    all_results.sort(key=lambda x: (x[0], x[1] == 'DIALOGUE', x[2]))

    seen = set()
    unique_results = []
    for item in all_results:
        key = (item[0], item[2])
        if key not in seen:
            seen.add(key)
            unique_results.append(item)

    # Write output
    output_file = output_dir / "dialogues_ko_v3.txt"
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# Ultima 6 Korean Translation (v3 - nuvie-accurate extraction)\n")
        f.write("# Format:\n")
        f.write("#   DESC (no quotes): NPC_NUM|english desc|<번역 필요>\n")
        f.write("#   Dialogue (quotes): NPC_NUM|\"english\"|\"<번역 필요>\"\n")
        f.write("#\n")
        f.write("# Variables: $P=player, $G=gender, $T=time, $N=NPC, $Z=input\n")
        f.write("# @keyword = highlighted keyword (preserve)\n")
        f.write("#\n\n")

        current_npc = -1
        desc_count = 0
        dialogue_count = 0

        for npc_num, entry_type, text in unique_results:
            if npc_num != current_npc:
                if current_npc >= 0:
                    f.write("\n")
                current_npc = npc_num

            if entry_type == 'DESC':
                f.write(f"{npc_num}|{text}|<번역 필요>\n")
                desc_count += 1
            else:
                # Ensure dialogue has proper quotes
                if not text.startswith('"'):
                    text = '"' + text
                if not text.endswith('"'):
                    text = text + '"'
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
