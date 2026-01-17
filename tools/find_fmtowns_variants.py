#!/usr/bin/env python3
"""
FM Towns vs DOS 대사 차이점 찾기
travelers vs traveler, you vs thee 같은 실제 대사 변형 추출
"""

import struct
import re
import sys
from pathlib import Path
from collections import defaultdict
from difflib import SequenceMatcher

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


def is_print(b):
    return b == 0x0a or (0x20 <= b <= 0x7e)


def extract_dialogues(data):
    """바이너리 데이터에서 대사 텍스트 추출 (따옴표로 시작하는 것만)"""
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
                text = ''.join(text_chars).strip()
                # 따옴표로 시작하는 대사만 (키워드 리스트 제외)
                if text and len(text) > 10 and '"' in text:
                    results.append(text)
                text_chars = []
            pos += 1

    if text_chars:
        text = ''.join(text_chars).strip()
        if text and len(text) > 10 and '"' in text:
            results.append(text)

    return results


def clean_text(text):
    """태그 제거하고 정규화"""
    # ~P, ~L 태그 제거
    text = re.sub(r'~[PL]\d+', '', text)
    # +숫자word+ 패턴 제거
    text = re.sub(r'\+\d+[^+]*\+', '', text)
    # @ 제거
    text = text.replace('@', '')
    # 줄바꿈 정리
    text = text.replace('\\n', ' ').replace('\n', ' ')
    # 공백 정규화
    text = ' '.join(text.split())
    return text.strip()


def normalize_for_compare(text):
    """비교용 정규화 (더 관대하게)"""
    text = clean_text(text)
    text = text.replace('"', '').replace("'", '')
    return text.lower().strip()


def load_dos_translations(filepath):
    """DOS 번역 파일에서 영어 원문 로드"""
    translations = defaultdict(list)  # npc_num -> [(original, normalized)]

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
                # 따옴표가 있는 대사만
                if '"' in english and len(english) > 10:
                    normalized = normalize_for_compare(english)
                    translations[npc_num].append((english, normalized))
            except ValueError:
                continue

    return translations


def extract_fmtowns_dialogues(lib_path, start_npc):
    """FM Towns CONVERSE에서 대사 추출"""
    lib = U6Lib_n(lib_path)
    num_items = lib.get_num_items()
    results = defaultdict(list)

    for i in range(num_items):
        npc_num = start_npc + i
        script_data = lib.get_item(i)
        if not script_data:
            continue

        dialogues = extract_dialogues(script_data)
        for text in dialogues:
            cleaned = clean_text(text)
            normalized = normalize_for_compare(text)
            if normalized and len(normalized) > 10:
                results[npc_num].append((text, cleaned, normalized))

    return results


def find_similar(fmtowns_norm, dos_texts, threshold=0.85):
    """유사한 DOS 대사 찾기"""
    best_match = None
    best_ratio = 0

    for dos_orig, dos_norm in dos_texts:
        ratio = SequenceMatcher(None, fmtowns_norm, dos_norm).ratio()
        if ratio > best_ratio and ratio >= threshold:
            best_ratio = ratio
            best_match = (dos_orig, dos_norm, ratio)

    return best_match


def main():
    fmtowns_dir = Path("d:/proj/Ultima6/fmtowns")
    korean_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")
    dos_dir = Path("d:/proj/Ultima6/game_data")

    dialogues_file = korean_dir / "dialogues_ko.txt"

    # FM Towns CONVERSE 파일
    fmtowns_converse = [
        (fmtowns_dir / "CONVERSE.A", 0),
        (fmtowns_dir / "CONVERSE.B", 99),
    ]

    print("=" * 70)
    print("FM Towns vs DOS 대사 변형 찾기")
    print("=" * 70)

    # DOS 번역 로드
    print("\nLoading DOS translations...")
    dos_translations = load_dos_translations(dialogues_file)
    total_dos = sum(len(v) for v in dos_translations.values())
    print(f"  Loaded {total_dos} dialogues from {len(dos_translations)} NPCs")

    # FM Towns 대사 추출
    print("\nExtracting FM Towns dialogues...")
    fmtowns_dialogues = defaultdict(list)
    for path, start_npc in fmtowns_converse:
        if not path.exists():
            print(f"  Warning: {path} not found")
            continue
        print(f"  Processing {path.name}...")
        dialogues = extract_fmtowns_dialogues(str(path), start_npc)
        for npc, texts in dialogues.items():
            fmtowns_dialogues[npc].extend(texts)

    total_fmtowns = sum(len(v) for v in fmtowns_dialogues.values())
    print(f"  Extracted {total_fmtowns} dialogues from {len(fmtowns_dialogues)} NPCs")

    # 변형 찾기
    print("\n" + "=" * 70)
    print("변형 대사 찾기 (유사하지만 다른 대사)")
    print("=" * 70)

    variants = []  # (npc, fmtowns_text, dos_text, similarity)
    exact_matches = 0
    no_match = 0

    for npc_num in sorted(fmtowns_dialogues.keys()):
        dos_texts = dos_translations.get(npc_num, [])
        if not dos_texts:
            continue

        dos_norm_set = set(d[1] for d in dos_texts)

        for fmtowns_orig, fmtowns_cleaned, fmtowns_norm in fmtowns_dialogues[npc_num]:
            # 정확히 일치
            if fmtowns_norm in dos_norm_set:
                exact_matches += 1
                continue

            # 유사한 것 찾기
            match = find_similar(fmtowns_norm, dos_texts, threshold=0.7)
            if match:
                dos_orig, dos_norm, ratio = match
                # 90% 미만이면 변형으로 간주
                if ratio < 0.95:
                    variants.append((npc_num, fmtowns_cleaned, dos_orig, ratio))
            else:
                no_match += 1

    print(f"\n정확히 일치: {exact_matches}")
    print(f"매칭 없음: {no_match}")
    print(f"변형 대사 (70-95% 유사): {len(variants)}")

    # 변형 대사 상세 출력
    if variants:
        print("\n" + "-" * 70)
        print("변형 대사 목록 (번역 파일에 추가 필요)")
        print("-" * 70)

        # 유사도 순으로 정렬
        variants.sort(key=lambda x: x[3], reverse=True)

        for npc_num, fmtowns_text, dos_text, ratio in variants[:50]:  # 상위 50개만
            print(f"\nNPC {npc_num} (유사도: {ratio*100:.1f}%)")
            ft_display = fmtowns_text[:80] + "..." if len(fmtowns_text) > 80 else fmtowns_text
            dos_display = dos_text[:80] + "..." if len(dos_text) > 80 else dos_text
            print(f"  FM Towns: {ft_display}")
            print(f"  DOS:      {dos_display}")

        if len(variants) > 50:
            print(f"\n... ({len(variants) - 50}개 더)")

    # 파일로 저장
    output_file = korean_dir / "fmtowns_variants.txt"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# FM Towns 대사 변형 (DOS와 다른 대사)\n")
        f.write("# Format: NPC번호|FM Towns 대사|DOS 대사|유사도\n")
        f.write(f"# 총 {len(variants)}개\n")
        f.write("#" + "=" * 67 + "\n\n")

        for npc_num, fmtowns_text, dos_text, ratio in variants:
            ft_escaped = fmtowns_text.replace('\n', '\\n')
            dos_escaped = dos_text.replace('\n', '\\n')
            f.write(f"{npc_num}|{ft_escaped}|{dos_escaped}|{ratio*100:.1f}%\n")

    print(f"\n변형 대사 저장됨: {output_file}")

    # 번역 파일에 추가할 형식으로도 저장
    add_file = korean_dir / "fmtowns_to_add.txt"
    with open(add_file, 'w', encoding='utf-8') as f:
        f.write("# FM Towns 대사 - dialogues_ko.txt에 추가할 내용\n")
        f.write("# DOS 번역을 참고하여 한국어 번역 추가 필요\n")
        f.write(f"# 총 {len(variants)}개\n")
        f.write("#" + "=" * 67 + "\n\n")

        for npc_num, fmtowns_text, dos_text, ratio in variants:
            ft_escaped = fmtowns_text.replace('\n', '\\n')
            f.write(f"{npc_num}|{ft_escaped}|<DOS 참고: {dos_text[:50]}...>\n")

    print(f"추가용 파일 저장됨: {add_file}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
