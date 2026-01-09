#!/usr/bin/env python3
"""
Ultima 6 CONVERSE.A/CONVERSE.B 바이너리에서 NPC 대화 추출 v4
정확한 추출: 따옴표 안 = 대사, 따옴표 밖 = 행동 설명

출력 형식:
- DESC (따옴표 없음): NPC번호|영어 설명|<번역 필요>
- 대사 (따옴표 있음): NPC번호|"영어 대사"|"<번역 필요>"
- 행동 (따옴표 없음): NPC번호|영어 행동|<번역 필요>
"""

import struct
import re
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
        self.data = Path(filename).read_bytes()
        self.lib_size = lib_size
        self.lzw = U6Lzw()
        self.items = []
        self._parse()

    def _parse(self):
        num_items = self._calculate_num_items()
        pos = 0
        for i in range(num_items + 1):
            item = U6LibItem()
            if self.lib_size == 4:
                if pos + 4 > len(self.data):
                    break
                item.offset = struct.unpack_from('<I', self.data, pos)[0]
            else:
                if pos + 2 > len(self.data):
                    break
                item.offset = struct.unpack_from('<H', self.data, pos)[0]
            pos += self.lib_size
            self.items.append(item)
        self._calculate_item_sizes()

    def _calculate_num_items(self):
        """첫 번째 non-zero 오프셋으로 아이템 수 결정"""
        pos = 0
        while pos < len(self.data):
            if self.lib_size == 4:
                if pos + 4 > len(self.data):
                    break
                offset = struct.unpack_from('<I', self.data, pos)[0]
            else:
                if pos + 2 > len(self.data):
                    break
                offset = struct.unpack_from('<H', self.data, pos)[0]
            if offset != 0:
                return offset // self.lib_size
            pos += self.lib_size
        return 0

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
# U6 Opcodes
# =============================================================================

VALOPS = {
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
    0x9a, 0x9b, 0x9d, 0x9f, 0xa0, 0xa7,
    0xab, 0xb2, 0xb3, 0xb4, 0xb7, 0xbb,
    0xc6, 0xc7, 0xca, 0xcc, 0xd7,
    0xda, 0xdc, 0xdd, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4
}

DATASIZES = {0xd2, 0xd3, 0xd4}

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
    return b == 0x0a or (0x20 <= b <= 0x7a) or b == 0x7e or b == 0x7b


def is_ctrl(b):
    if b in VALOPS or b in DATASIZES:
        return False
    return b >= 0xa1 or b == 0x9c or b == 0x9e


# =============================================================================
# Text Parsing - 따옴표 안/밖 정확히 분리
# =============================================================================

def split_text_by_quotes(text):
    """
    텍스트를 따옴표 기준으로 분리
    반환: [(type, text), ...] where type is 'dialogue' or 'action'
    """
    results = []

    # 따옴표 쌍 찾기
    # 패턴: "..."  또는  "..." 형태
    pattern = r'"([^"]*)"'

    last_end = 0
    for match in re.finditer(pattern, text):
        # 따옴표 앞의 텍스트 (행동 설명)
        before = text[last_end:match.start()].strip()
        if before:
            results.append(('action', before))

        # 따옴표 안의 텍스트 (대사)
        dialogue = match.group(1).strip()
        if dialogue:
            results.append(('dialogue', dialogue))

        last_end = match.end()

    # 마지막 따옴표 뒤의 텍스트
    after = text[last_end:].strip()
    if after:
        results.append(('action', after))

    # 따옴표가 없으면 전체를 행동으로 처리 (단, 변수만 있는 경우는 제외)
    if not results and text.strip():
        results.append(('action', text.strip()))

    return results


# =============================================================================
# ConvScript Parser
# =============================================================================

class ConvScriptParser:
    def __init__(self, data, npc_num):
        self.data = data
        self.npc_num = npc_num
        self.pos = 0
        self.name = ""
        self.desc = ""
        self.raw_texts = []  # 원본 텍스트 블록들

    def peek(self, offset=0):
        p = self.pos + offset
        if p < len(self.data):
            return self.data[p]
        return 0

    def read(self):
        if self.pos < len(self.data):
            b = self.data[self.pos]
            self.pos += 1
            return b
        return 0

    def read2(self):
        if self.pos + 2 <= len(self.data):
            val = self.data[self.pos] + (self.data[self.pos + 1] << 8)
            self.pos += 2
            return val
        return 0

    def read4(self):
        if self.pos + 4 <= len(self.data):
            val = struct.unpack_from('<I', self.data, self.pos)[0]
            self.pos += 4
            return val
        return 0

    def overflow(self, offset=0):
        return self.pos + offset >= len(self.data)

    def collect_text(self):
        text_chars = []
        while not self.overflow() and is_print(self.peek()):
            text_chars.append(chr(self.read()))
        return ''.join(text_chars)

    def skip_value(self):
        b = self.read()
        if b == 0xd3:
            self.read()
        elif b == 0xd2:
            self.read4()
        elif b == 0xd4:
            self.read2()

    def skip_eval(self):
        while not self.overflow():
            b = self.peek()
            if is_ctrl(b) and b != U6OP_EVAL:
                break
            if is_print(b):
                break
            if b == U6OP_EVAL:
                self.read()
                continue
            self.skip_value()

    def parse(self):
        if not self.data or len(self.data) < 3:
            return False

        self.pos = 2

        # Read name
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
                text = self.collect_text()
                if text.strip():
                    self.raw_texts.append(text)

            elif is_ctrl(b):
                self._handle_control()

            else:
                self.read()

        return True

    def _handle_control(self):
        b = self.read()

        if b == U6OP_JUMP:
            self.read4()

        elif b == U6OP_SIDENT:
            self.read()
            text = self.collect_text()

        elif b in (U6OP_KEYWORDS, U6OP_ASKC, U6OP_SLOOK):
            text = self.collect_text()
            if b == U6OP_SLOOK:
                self.desc = text.strip()

        elif b == U6OP_IF:
            self.skip_eval()

        elif b in (U6OP_SETF, U6OP_CLEARF):
            self.skip_eval()
            self.skip_eval()

        elif b == U6OP_DECL:
            self.skip_eval()
            self.skip_eval()

        elif b == U6OP_ASSIGN:
            self.skip_eval()

        elif b == U6OP_SETNAME:
            self.skip_eval()

        elif b == U6OP_DPRINT:
            self.skip_eval()
            self.skip_eval()

        elif b == U6OP_NEW:
            for _ in range(4):
                self.skip_eval()

        elif b == U6OP_DELETE:
            for _ in range(4):
                self.skip_eval()

        elif b == U6OP_GIVE:
            for _ in range(4):
                self.skip_eval()

        elif b in (U6OP_ADDKARMA, U6OP_SUBKARMA):
            self.skip_eval()

        elif b == U6OP_WORKTYPE:
            self.skip_eval()
            self.skip_eval()

        elif b in (U6OP_PORTRAIT, U6OP_RESURRECT, U6OP_HEAL, U6OP_CURE, U6OP_HORSE):
            self.skip_eval()

        elif b in (U6OP_INPUTSTR, U6OP_INPUT, U6OP_INPUTNUM):
            self.skip_eval()
            self.skip_eval()


def extract_from_lib(lib_path, start_npc):
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

        # DESC 추가 (U6OP_SLOOK에서 추출된 NPC 설명)
        if parser.desc:
            desc = parser.desc.strip()
            # 줄바꿈만 통일
            desc = desc.replace('\r\n', '\n').replace('\r', '\n')
            if desc:
                results.append((npc_num, desc))

        # 원본 텍스트 그대로 저장 (따옴표 포함)
        for raw_text in parser.raw_texts:
            text = raw_text.strip()
            if not text:
                continue

            # 줄바꿈만 통일
            text = text.replace('\r\n', '\n').replace('\r', '\n')

            if text:
                results.append((npc_num, text))

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
    print("Ultima 6 NPC Dialogue Extractor v4")
    print("정확한 추출: 따옴표 안 = 대사, 밖 = 행동")
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

    # Sort by NPC, then text
    all_results.sort(key=lambda x: (x[0], x[1]))

    # Deduplicate
    seen = set()
    unique_results = []
    for item in all_results:
        key = (item[0], item[1])
        if key not in seen:
            seen.add(key)
            unique_results.append(item)

    # Write output as TXT with | delimiter
    output_file = output_dir / "dialogues_ko_original.txt"
    output_file.parent.mkdir(parents=True, exist_ok=True)

    # Get unique NPCs
    npcs = set(r[0] for r in unique_results)

    with open(output_file, 'w', encoding='utf-8') as f:
        for npc_num, text in unique_results:
            # 줄바꿈을 \n 문자열로 이스케이프
            escaped_text = text.replace('\n', '\\n')
            f.write(f'{npc_num}|{escaped_text}|\n')

    print(f"\n결과:")
    print(f"  총 NPC 수: {len(npcs)}")
    print(f"  총 엔트리: {len(unique_results)}")
    print(f"\n저장됨: {output_file}")

    # Also create working copy
    working_file = output_dir / "dialogues_ko.txt"
    import shutil
    shutil.copy(output_file, working_file)
    print(f"작업용 복사: {working_file}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
