#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
원본에만 있고 번역파일에 없는 키를 찾아서 추가하는 스크립트
(유사한 번역이 있으면 그 번역을 사용)
"""

import difflib
from collections import defaultdict

def parse_dialogues_file(filepath, is_original=False):
    entries = {}
    with open(filepath, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('|')
            if len(parts) < 2:
                continue
            npc_id = parts[0]
            original_text = parts[1]
            if is_original:
                entries[original_text] = (npc_id, line_num)
            else:
                if len(parts) >= 3 and parts[2]:
                    entries[original_text] = (npc_id, line_num, parts[2])
    return entries

def normalize_text(text):
    return ' '.join(text.lower().replace('"', '').split())

def quick_similarity(s1, s2):
    len1, len2 = len(s1), len(s2)
    if len1 == 0 or len2 == 0:
        return 0
    if abs(len1 - len2) / max(len1, len2) > 0.25:
        return 0
    return 1

def main():
    # Parse files
    original_entries = parse_dialogues_file('dialogues_ko_original.txt', is_original=True)
    translation_entries = parse_dialogues_file('dialogues_ko.txt', is_original=False)

    print(f"원본 항목 수: {len(original_entries)}")
    print(f"번역 항목 수: {len(translation_entries)}")

    # Group by NPC
    translation_by_npc = defaultdict(dict)
    for key, info in translation_entries.items():
        translation_by_npc[info[0]][key] = info

    # Find entries in original that need to be added
    additions = []

    for orig_key, orig_info in original_entries.items():
        if orig_key in translation_entries:
            continue  # Already exists

        npc_id = orig_info[0]

        # Find best match in translation for same NPC
        npc_translations = translation_by_npc.get(npc_id, {})

        best_match = None
        best_ratio = 0
        best_trans = None

        for trans_key, trans_info in npc_translations.items():
            if quick_similarity(orig_key, trans_key) == 0:
                continue
            ratio = difflib.SequenceMatcher(None, orig_key, trans_key).ratio()
            if ratio > best_ratio and ratio >= 0.85:
                best_ratio = ratio
                best_match = trans_key
                best_trans = trans_info[2] if len(trans_info) > 2 else None

        if best_match and best_trans:
            additions.append({
                'npc_id': npc_id,
                'original_key': orig_key,
                'translation': best_trans,
                'matched_from': best_match,
                'ratio': best_ratio
            })

    print(f"\n추가할 항목 수: {len(additions)}")

    if not additions:
        print("추가할 항목이 없습니다.")
        return

    # Show preview
    print("\n=== 추가할 항목 미리보기 (처음 10개) ===\n")
    for i, item in enumerate(additions[:10], 1):
        print(f"[{i}] NPC {item['npc_id']} (유사도: {item['ratio']:.1%})")
        print(f"    원본: {item['original_key'][:70]}...")
        print(f"    매칭: {item['matched_from'][:70]}...")
        print(f"    번역: {item['translation'][:70]}...")
        print()

    # Ask for confirmation
    response = input(f"\n{len(additions)}개 항목을 dialogues_ko.txt에 추가하시겠습니까? (y/n): ")
    if response.lower() != 'y':
        print("취소되었습니다.")
        return

    # Read existing file and append
    with open('dialogues_ko.txt', 'r', encoding='utf-8') as f:
        content = f.read()

    # Add new entries at the end
    new_lines = ["\n# === Auto-added entries (matching original keys) ==="]
    for item in additions:
        new_line = f"{item['npc_id']}|{item['original_key']}|{item['translation']}"
        new_lines.append(new_line)

    with open('dialogues_ko.txt', 'a', encoding='utf-8') as f:
        f.write('\n'.join(new_lines))
        f.write('\n')

    print(f"\n{len(additions)}개 항목이 추가되었습니다.")

if __name__ == '__main__':
    main()
