#!/usr/bin/env python3
"""
Ultima 6 CONVERSE.A/CONVERSE.B 바이너리에서 NPC 대화 추출 v2
바이트코드 파싱 대신 원시 텍스트 추출 방식

출력 형식:
- DESC (따옴표 없음): NPC번호|영어 설명|<번역 필요>
- 대사 (따옴표 있음): NPC번호|"영어 대사"|"<번역 필요>"
"""

import struct
import re
import sys
from pathlib import Path

# =============================================================================
# U6 LZW Decompression (from nuvie U6Lzw.cpp)
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
        self.uncomp_size = 0


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
            self.items[i].uncomp_size = self._calc_uncomp_size(self.items[i])

    def _calc_uncomp_size(self, item):
        if item.flag in (0x01, 0x20):
            if item.offset + 4 <= len(self.data):
                return struct.unpack_from('<I', self.data, item.offset)[0]
        return item.size

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
# Text Extractor - 원시 바이트에서 텍스트 직접 추출
# =============================================================================

U6OP_LOOK = 0xf1
U6OP_CONVERSE = 0xf2
U6OP_NAME = 0xf3


def extract_texts_from_script(data, npc_num):
    """스크립트 데이터에서 모든 텍스트 추출"""
    if not data or len(data) < 3:
        return None, None, []

    # 첫 2바이트는 스크립트 크기
    pos = 2

    # 이름 추출 (0xf1, 0xf2, 0xf3 전까지)
    name_chars = []
    while pos < len(data):
        b = data[pos]
        if b in (U6OP_LOOK, U6OP_NAME, U6OP_CONVERSE):
            break
        if b == ord('_'):
            name_chars.append('.')
        elif 0x20 <= b < 0x7f:
            name_chars.append(chr(b))
        pos += 1
    name = ''.join(name_chars).strip()

    if not name:
        return None, None, []

    # DESC 추출 (0xf1 다음, * 전까지)
    desc = ""
    while pos < len(data):
        if data[pos] == U6OP_LOOK:
            pos += 1
            desc_chars = []
            while pos < len(data):
                b = data[pos]
                if b >= 0xa0 or b == 0:
                    break
                if b == ord('*'):
                    pos += 1
                    break
                if 0x20 <= b < 0x7f:
                    desc_chars.append(chr(b))
                pos += 1
            desc = ''.join(desc_chars).strip()
            break
        elif data[pos] in (U6OP_NAME, U6OP_CONVERSE):
            break
        pos += 1

    # 대사 추출 - 원시 바이트에서 따옴표로 둘러싸인 텍스트 찾기
    dialogues = []

    # 방법: 전체 데이터를 스캔하여 "..."로 둘러싸인 텍스트 추출
    i = 0
    while i < len(data):
        if data[i] == ord('"'):
            # 따옴표 시작 찾음
            start = i
            i += 1
            text_chars = ['"']

            while i < len(data):
                b = data[i]

                # 제어 코드 (0xa0-0xff)를 만나면 중단
                # 단, 줄바꿈(0x0a)은 허용
                if b >= 0xa0:
                    break

                if b == ord('"'):
                    text_chars.append('"')
                    i += 1
                    # 텍스트 완성
                    text = ''.join(text_chars)
                    # 정리
                    text = text.replace('\n', ' ').replace('\r', ' ')
                    text = ' '.join(text.split())  # 연속 공백 제거
                    if len(text) > 2:  # 최소 길이 체크 (빈 따옴표 제외)
                        dialogues.append(text)
                    break
                elif b == ord('*'):
                    # pause marker -> space
                    text_chars.append(' ')
                elif b == 0x0a or b == 0x0d:
                    # 줄바꿈 -> space
                    text_chars.append(' ')
                elif 0x20 <= b < 0x7f:
                    text_chars.append(chr(b))
                # 다른 제어 문자는 무시

                i += 1
        else:
            i += 1

    return name, desc, dialogues


def extract_from_lib(lib_path, start_npc):
    """라이브러리 파일에서 모든 NPC 대화 추출"""
    lib = U6Lib_n(lib_path)
    num_items = lib.get_num_items()
    results = []

    for i in range(num_items):
        npc_num = start_npc + i
        script_data = lib.get_item(i)
        if not script_data:
            continue

        name, desc, dialogues = extract_texts_from_script(script_data, npc_num)
        if not name:
            continue

        # DESC 추가
        if desc:
            results.append((npc_num, 'DESC', desc))

        # 대사 추가
        for dialogue in dialogues:
            results.append((npc_num, 'DIALOGUE', dialogue))

    return results


def main():
    game_data = Path("d:/proj/Ultima6/game_data")
    output_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    converse_a = game_data / "CONVERSE.A"
    converse_b = game_data / "CONVERSE.B"

    if not converse_a.exists() or not converse_b.exists():
        print("Error: CONVERSE.A or CONVERSE.B not found")
        return 1

    print("=" * 60)
    print("Ultima 6 NPC Dialogue Extractor v2")
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

    # 정렬
    all_results.sort(key=lambda x: (x[0], x[1] == 'DIALOGUE'))

    # 중복 제거
    seen = set()
    unique_results = []
    for item in all_results:
        key = (item[0], item[2])
        if key not in seen:
            seen.add(key)
            unique_results.append(item)

    # 출력
    output_file = output_dir / "dialogues_ko_v2.txt"
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# Ultima 6 Korean Translation (v2 - Raw Text Extraction)\n")
        f.write("# Format:\n")
        f.write("#   DESC (no quotes): NPC_NUM|english desc|<번역 필요>\n")
        f.write("#   Dialogue (quotes): NPC_NUM|\"english\"|\"<번역 필요>\"\n")
        f.write("#\n")
        f.write("# Variables: $P=player, $G=gender title, $T=time, $N=NPC name, $Z=input\n")
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
                # 대사는 이미 따옴표 포함
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
