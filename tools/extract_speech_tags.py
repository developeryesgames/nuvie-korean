#!/usr/bin/env python3
"""
FM Towns Ultima 6 CONVERSE 파일에서 ~P 음성 태그 추출
원본 대사와 ~P 태그를 매핑하여 한국어 번역에 삽입할 수 있게 함

~P 태그 형식: ~P숫자 (예: ~P1, ~P23)
이 태그가 있으면 FM Towns 음성이 재생됨
"""

import struct
import re
import sys
from pathlib import Path

# =============================================================================
# U6 LZW Decompression (extract_converse_v4.py에서 복사)
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
# ~P 태그 추출
# =============================================================================

def is_print(b):
    return b == 0x0a or (0x20 <= b <= 0x7e)


def extract_text_with_tags(data):
    """바이너리 데이터에서 텍스트와 ~P 태그 추출"""
    if not data:
        return []

    results = []
    text_chars = []
    pos = 0

    while pos < len(data):
        b = data[pos]

        # 출력 가능 문자
        if is_print(b):
            text_chars.append(chr(b))
            pos += 1
        else:
            # 텍스트 블록 저장
            if text_chars:
                text = ''.join(text_chars)
                if text.strip():
                    results.append(text)
                text_chars = []
            pos += 1

    # 마지막 텍스트 블록
    if text_chars:
        text = ''.join(text_chars)
        if text.strip():
            results.append(text)

    return results


def extract_speech_tags_from_lib(lib_path, start_npc):
    """CONVERSE 라이브러리에서 ~P 태그가 포함된 텍스트 추출"""
    lib = U6Lib_n(lib_path)
    num_items = lib.get_num_items()
    results = []  # (npc_num, text_with_tag, tag_number, text_without_tag)

    for i in range(num_items):
        npc_num = start_npc + i
        script_data = lib.get_item(i)
        if not script_data:
            continue

        # 텍스트 추출
        texts = extract_text_with_tags(script_data)

        for text in texts:
            # ~P 태그 찾기
            matches = list(re.finditer(r'~P(\d+)', text))
            if matches:
                for match in matches:
                    tag_num = int(match.group(1))
                    # 태그 제거한 텍스트 (비교용)
                    text_no_tag = re.sub(r'~P\d+', '', text).strip()
                    results.append((npc_num, text, tag_num, text_no_tag))

    return results


def normalize_text(text):
    """비교를 위한 텍스트 정규화"""
    # ~P 태그 제거
    text = re.sub(r'~P\d+', '', text)
    # 줄바꿈 통일
    text = text.replace('\\n', '\n').replace('\r\n', '\n').replace('\r', '\n')
    # 공백 정규화
    text = ' '.join(text.split())
    # 따옴표 제거 (비교 목적)
    text = text.replace('"', '').replace("'", '')
    return text.strip().lower()


def main():
    fmtowns_dir = Path("d:/proj/Ultima6/fmtowns")
    korean_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    # FM Towns CONVERSE 파일 목록
    converse_files = [
        (fmtowns_dir / "CONVERSE.A", 0),
        (fmtowns_dir / "CONVERSE.B", 99),
    ]

    # 파일 존재 확인
    for path, _ in converse_files:
        if not path.exists():
            print(f"Error: {path} not found")
            return 1

    print("=" * 70)
    print("FM Towns Ultima 6 Speech Tag Extractor")
    print("~P 태그 추출하여 한국어 번역에 삽입")
    print("=" * 70)

    # 모든 ~P 태그 추출
    all_tags = []
    for path, start_npc in converse_files:
        print(f"\nProcessing {path.name}...")
        tags = extract_speech_tags_from_lib(str(path), start_npc)
        all_tags.extend(tags)
        print(f"  Found {len(tags)} speech tags")

    print(f"\n총 {len(all_tags)} 개의 ~P 태그 발견")

    # NPC별로 정리
    tags_by_npc = {}
    for npc_num, text_with_tag, tag_num, text_no_tag in all_tags:
        if npc_num not in tags_by_npc:
            tags_by_npc[npc_num] = []
        tags_by_npc[npc_num].append({
            'tag': f'~P{tag_num}',
            'tag_num': tag_num,
            'full_text': text_with_tag,
            'text_no_tag': text_no_tag,
            'normalized': normalize_text(text_no_tag)
        })

    # 결과 파일 저장
    output_file = korean_dir / "speech_tags_fmtowns.txt"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# FM Towns Ultima 6 Speech Tags\n")
        f.write("# Format: NPC번호|~P태그|원문텍스트\n")
        f.write("# 이 파일의 ~P 태그를 한국어 번역(dialogues_ko.txt)에 삽입하세요\n")
        f.write("#" + "=" * 67 + "\n\n")

        for npc_num in sorted(tags_by_npc.keys()):
            f.write(f"# NPC {npc_num}\n")
            for tag_info in tags_by_npc[npc_num]:
                escaped = tag_info['full_text'].replace('\n', '\\n')
                f.write(f"{npc_num}|{tag_info['tag']}|{escaped}\n")
            f.write("\n")

    print(f"\n태그 저장됨: {output_file}")

    # 통계 출력
    print(f"\nNPC별 태그 수:")
    for npc_num in sorted(tags_by_npc.keys())[:20]:  # 처음 20개만
        print(f"  NPC {npc_num}: {len(tags_by_npc[npc_num])} tags")
    if len(tags_by_npc) > 20:
        print(f"  ... ({len(tags_by_npc) - 20} more NPCs)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
