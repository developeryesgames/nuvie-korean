#!/usr/bin/env python3
"""
FM Towns vs DOS CONVERSE 비교
DOS 번역 파일(dialogues_ko.txt)에 없는 FM Towns 대사 추출

출력: NPC별 빠진 대사 수와 샘플
"""

import struct
import re
import sys
from pathlib import Path
from collections import defaultdict

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
# 텍스트 추출 및 비교
# =============================================================================

def is_print(b):
    return b == 0x0a or (0x20 <= b <= 0x7e)


def extract_dialogues(data):
    """바이너리 데이터에서 대사 텍스트 추출"""
    if not data:
        return []

    results = []
    text_chars = []
    pos = 0

    while pos < len(data):
        b = data[pos]
        if is_print(b):
            text_chars.append(chr(b))
            pos += 1
        else:
            if text_chars:
                text = ''.join(text_chars)
                if text.strip() and len(text.strip()) > 3:
                    results.append(text.strip())
                text_chars = []
            pos += 1

    if text_chars:
        text = ''.join(text_chars)
        if text.strip() and len(text.strip()) > 3:
            results.append(text.strip())

    return results


def normalize_text(text):
    """비교를 위한 텍스트 정규화"""
    # ~P, ~L 태그 제거
    text = re.sub(r'~[PL]\d+', '', text)
    # +숫자word+ 패턴 제거 (FM Towns 특수 태그)
    text = re.sub(r'\+\d+[^+]*\+', '', text)
    # @ 기호 제거 (키워드 마커)
    text = text.replace('@', '')
    # 줄바꿈 통일
    text = text.replace('\\n', ' ').replace('\n', ' ').replace('\r', ' ')
    # 따옴표 제거
    text = text.replace('"', '').replace("'", '')
    # 공백 정규화
    text = ' '.join(text.split())
    # 소문자로
    return text.strip().lower()


def load_dos_translations(filepath):
    """dialogues_ko.txt에서 영어 원문 로드 (NPC별)"""
    translations = defaultdict(set)  # npc_num -> set of normalized english texts

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            parts = line.split('|')
            if len(parts) < 2:
                continue

            try:
                npc_num = int(parts[0])
                english = parts[1]
                normalized = normalize_text(english)
                if normalized:
                    translations[npc_num].add(normalized)
            except ValueError:
                continue

    return translations


def extract_fmtowns_dialogues(lib_path, start_npc):
    """FM Towns CONVERSE에서 대사 추출"""
    lib = U6Lib_n(lib_path)
    num_items = lib.get_num_items()
    results = defaultdict(list)  # npc_num -> list of (original_text, normalized_text)

    for i in range(num_items):
        npc_num = start_npc + i
        script_data = lib.get_item(i)
        if not script_data:
            continue

        dialogues = extract_dialogues(script_data)
        for text in dialogues:
            normalized = normalize_text(text)
            if normalized and len(normalized) > 5:  # 너무 짧은 텍스트 제외
                results[npc_num].append((text, normalized))

    return results


def main():
    fmtowns_dir = Path("d:/proj/Ultima6/fmtowns")
    korean_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    dialogues_file = korean_dir / "dialogues_ko.txt"

    # FM Towns CONVERSE 파일
    converse_files = [
        (fmtowns_dir / "CONVERSE.A", 0),
        (fmtowns_dir / "CONVERSE.B", 99),
    ]

    # 파일 존재 확인
    if not dialogues_file.exists():
        print(f"Error: {dialogues_file} not found")
        return 1

    for path, _ in converse_files:
        if not path.exists():
            print(f"Error: {path} not found")
            return 1

    print("=" * 70)
    print("FM Towns vs DOS CONVERSE 비교")
    print("=" * 70)

    # DOS 번역 로드
    print("\nLoading DOS translations...")
    dos_translations = load_dos_translations(dialogues_file)
    total_dos = sum(len(v) for v in dos_translations.values())
    print(f"  Loaded {total_dos} unique texts from {len(dos_translations)} NPCs")

    # FM Towns 대사 추출
    print("\nExtracting FM Towns dialogues...")
    fmtowns_dialogues = defaultdict(list)
    for path, start_npc in converse_files:
        print(f"  Processing {path.name}...")
        dialogues = extract_fmtowns_dialogues(str(path), start_npc)
        for npc, texts in dialogues.items():
            fmtowns_dialogues[npc].extend(texts)

    total_fmtowns = sum(len(v) for v in fmtowns_dialogues.values())
    print(f"  Extracted {total_fmtowns} texts from {len(fmtowns_dialogues)} NPCs")

    # 비교
    print("\n" + "=" * 70)
    print("비교 결과")
    print("=" * 70)

    missing_by_npc = {}  # npc_num -> list of missing texts
    matched_count = 0
    missing_count = 0

    for npc_num in sorted(fmtowns_dialogues.keys()):
        dos_texts = dos_translations.get(npc_num, set())
        fmtowns_texts = fmtowns_dialogues[npc_num]

        missing = []
        for original, normalized in fmtowns_texts:
            # 정확히 일치하는지 확인
            if normalized in dos_texts:
                matched_count += 1
                continue

            # 부분 일치 확인 (FM Towns 텍스트가 DOS 텍스트에 포함되거나 그 반대)
            found = False
            for dos_text in dos_texts:
                if normalized in dos_text or dos_text in normalized:
                    found = True
                    matched_count += 1
                    break

            if not found:
                missing.append(original)
                missing_count += 1

        if missing:
            missing_by_npc[npc_num] = missing

    # 결과 출력
    print(f"\n총 FM Towns 대사: {total_fmtowns}")
    print(f"매칭된 대사: {matched_count}")
    print(f"빠진 대사: {missing_count}")
    print(f"매칭률: {matched_count / total_fmtowns * 100:.1f}%")

    print(f"\n빠진 대사가 있는 NPC 수: {len(missing_by_npc)}")

    # NPC별 상세
    print("\n" + "-" * 70)
    print("NPC별 빠진 대사 수 (상위 20개)")
    print("-" * 70)

    sorted_npcs = sorted(missing_by_npc.items(), key=lambda x: len(x[1]), reverse=True)

    for npc_num, missing in sorted_npcs[:20]:
        print(f"\nNPC {npc_num}: {len(missing)}개 빠짐")
        # 샘플 3개만 표시
        for text in missing[:3]:
            # 줄바꿈을 \\n으로 표시
            display = text.replace('\n', '\\n')
            if len(display) > 80:
                display = display[:77] + "..."
            print(f"  - {display}")
        if len(missing) > 3:
            print(f"  ... ({len(missing) - 3}개 더)")

    # 빠진 대사 파일로 저장
    output_file = korean_dir / "fmtowns_missing.txt"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# FM Towns 대사 중 DOS 번역에 없는 것들\n")
        f.write("# Format: NPC번호|FM Towns 영어 원문|<번역 필요>\n")
        f.write(f"# 총 {missing_count}개\n")
        f.write("#" + "=" * 67 + "\n\n")

        for npc_num in sorted(missing_by_npc.keys()):
            for text in missing_by_npc[npc_num]:
                escaped = text.replace('\n', '\\n')
                f.write(f"{npc_num}|{escaped}|<번역 필요>\n")

    print(f"\n빠진 대사 저장됨: {output_file}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
