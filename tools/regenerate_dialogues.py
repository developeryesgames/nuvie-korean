#!/usr/bin/env python3
"""
원본 추출 파일에서 dialogues_ko.txt 새로 생성
형식:
- DESC: NPC_NUM|영어텍스트|<번역 필요>  (따옴표 없음)
- 대사: NPC_NUM|"영어텍스트"|<번역 필요>  (따옴표 있음)
"""

import os
import re
import glob

def extract_npc_num(filename):
    """npc_003_Shamino.txt -> 3"""
    match = re.search(r'npc_(\d+)_', filename)
    if match:
        return int(match.group(1))
    return None

def clean_text(text):
    """제어문자 및 쓰레기 제거"""
    # 제어문자 제거 (° ò ¤ Ó ë § ¶ î ¡ ¢ 등)
    # @는 유지 (키워드 하이라이트용)
    cleaned = re.sub(r'[°òÓë§¶î¡¢¤¿¹º¦²¨ÝÙÛÚÜKÇÔMN\x00-\x1f]', '', text)
    # 연속 공백 정리
    cleaned = re.sub(r'\s+', ' ', cleaned)
    return cleaned.strip()

def parse_npc_file(filepath):
    """NPC 파일에서 DESC와 TEXT 추출"""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    lines = []
    npc_num = extract_npc_num(filepath)
    if npc_num is None:
        return []

    # DESC 추출
    desc_match = re.search(r'\[DESC\]\s*\n(.+?)(?:\n\[|\Z)', content, re.DOTALL)
    if desc_match:
        desc_text = desc_match.group(1).strip()
        # * 이후 제거, 제어문자 제거
        if '*' in desc_text:
            desc_text = desc_text.split('*')[0].strip()
        desc_text = clean_text(desc_text)
        if desc_text and not desc_text.startswith('"'):
            lines.append(f'{npc_num}|{desc_text}|<번역 필요>')

    # TEXT 추출 (대사)
    text_matches = re.findall(r'\[TEXT\]\s*\n(.+?)(?:\n\[|\Z)', content, re.DOTALL)
    for text_block in text_matches:
        # * 로 분리된 대사들 처리
        parts = text_block.split('*')
        for part in parts:
            part = clean_text(part)
            if not part:
                continue
            # 따옴표로 시작하는 대사만 추출
            quotes = re.findall(r'"([^"]+)"', part)
            for quote in quotes:
                quote = quote.strip()
                if quote and len(quote) > 1:
                    lines.append(f'{npc_num}|"{quote}"|<번역 필요>')

    return lines

def main():
    extracted_dir = "d:/proj/Ultima6/nuvie/tools/extracted"
    output_file = "d:/proj/Ultima6/nuvie/data/translations/korean/dialogues_ko.txt"

    all_lines = []
    all_lines.append("# Ultima 6 Korean Dialogue Translations")
    all_lines.append("# Format: NPC_NUM|ENGLISH|KOREAN")
    all_lines.append("# DESC (no quotes): NPC_NUM|english desc|한국어 설명")
    all_lines.append("# Dialogue (with quotes): NPC_NUM|\"english\"|\"한국어\"")
    all_lines.append("")

    npc_files = sorted(glob.glob(os.path.join(extracted_dir, "npc_*.txt")))
    print(f"Processing {len(npc_files)} NPC files...")

    seen = set()  # 중복 제거
    for filepath in npc_files:
        lines = parse_npc_file(filepath)
        for line in lines:
            if line not in seen:
                seen.add(line)
                all_lines.append(line)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(all_lines))

    print(f"Generated {len(seen)} unique entries to {output_file}")

if __name__ == "__main__":
    main()
