#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
번역 파일 원문 키 검증 스크립트

dialogues_ko_original.txt (원문)과 dialogues_ko.txt (번역본)의
원문 키를 비교하여 불일치하는 항목을 찾습니다.

사용법: python check_translation_keys.py
"""

import os
import difflib
from collections import defaultdict

def parse_dialogues_file(filepath, is_original=False):
    """대화 파일을 파싱하여 {원문: (npc_id, line_num, [번역])} 딕셔너리 반환"""
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
    """텍스트 정규화 (비교용)"""
    # 따옴표 제거, 소문자화, 공백 정규화
    return ' '.join(text.lower().replace('"', '').split())

def quick_similarity(s1, s2):
    """빠른 유사도 계산 (길이 기반 사전 필터링)"""
    len1, len2 = len(s1), len(s2)
    if len1 == 0 or len2 == 0:
        return 0
    # 길이 차이가 20% 이상이면 낮은 유사도
    if abs(len1 - len2) / max(len1, len2) > 0.2:
        return 0
    return 1  # 상세 비교 필요

def highlight_diff(text1, text2):
    """두 텍스트의 차이점 하이라이트"""
    words1 = text1.split()
    words2 = text2.split()

    diff = []
    matcher = difflib.SequenceMatcher(None, words1, words2)

    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op == 'equal':
            diff.append(' '.join(words1[i1:i2]))
        elif op == 'replace':
            diff.append(f'[{" ".join(words1[i1:i2])} -> {" ".join(words2[j1:j2])}]')
        elif op == 'delete':
            diff.append(f'[-{" ".join(words1[i1:i2])}]')
        elif op == 'insert':
            diff.append(f'[+{" ".join(words2[j1:j2])}]')

    return ' '.join(diff)

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))

    original_file = os.path.join(script_dir, 'dialogues_ko_original.txt')
    translation_file = os.path.join(script_dir, 'dialogues_ko.txt')

    print("=" * 80)
    print("번역 파일 원문 키 검증")
    print("=" * 80)

    # 파일 파싱
    print("파일 파싱 중...")
    original_entries = parse_dialogues_file(original_file, is_original=True)
    translation_entries = parse_dialogues_file(translation_file, is_original=False)

    print(f"원문 항목 수: {len(original_entries)}")
    print(f"번역 항목 수: {len(translation_entries)}")

    # 정규화된 텍스트로 인덱스 생성 (빠른 검색용)
    original_normalized = {normalize_text(k): k for k in original_entries.keys()}

    # NPC ID별로 원문 그룹화 (같은 NPC 내에서만 비교)
    original_by_npc = defaultdict(dict)
    for key, info in original_entries.items():
        original_by_npc[info[0]][key] = info

    translation_by_npc = defaultdict(dict)
    for key, info in translation_entries.items():
        translation_by_npc[info[0]][key] = info

    # 번역 파일의 키 중 원문에 없는 것 찾기
    mismatched = []

    print("불일치 검색 중...")

    for trans_key, trans_info in translation_entries.items():
        if trans_key in original_entries:
            continue  # 정확히 일치하면 스킵

        npc_id = trans_info[0]
        trans_normalized = normalize_text(trans_key)

        # 정규화된 버전이 일치하는지 확인
        if trans_normalized in original_normalized:
            continue  # 공백/대소문자만 다르면 스킵

        # 같은 NPC의 원문들과만 비교 (성능 최적화)
        npc_originals = original_by_npc.get(npc_id, {})

        best_match = None
        best_ratio = 0

        for orig_key in npc_originals.keys():
            # 길이 사전 필터링
            if quick_similarity(trans_key, orig_key) == 0:
                continue

            ratio = difflib.SequenceMatcher(None, trans_key, orig_key).ratio()
            if ratio > best_ratio and ratio >= 0.85:
                best_ratio = ratio
                best_match = orig_key

        if best_match:
            mismatched.append({
                'trans_key': trans_key,
                'trans_info': trans_info,
                'original_key': best_match,
                'original_info': original_entries[best_match],
                'ratio': best_ratio
            })

    # 결과 출력
    print()
    if mismatched:
        print("=" * 80)
        print(f"불일치 항목 발견: {len(mismatched)}개")
        print("=" * 80)
        print()

        # 유사도가 높은 순으로 정렬
        mismatched.sort(key=lambda x: x['ratio'], reverse=True)

        for i, item in enumerate(mismatched, 1):
            npc_id = item['trans_info'][0]
            trans_line = item['trans_info'][1]
            orig_line = item['original_info'][1]

            print(f"[{i}] NPC {npc_id} (유사도: {item['ratio']:.1%})")
            print(f"    원문파일 {orig_line}행: {item['original_key'][:100]}")
            print(f"    번역파일 {trans_line}행: {item['trans_key'][:100]}")
            print(f"    차이: {highlight_diff(item['original_key'], item['trans_key'])[:120]}")
            if len(item['trans_info']) > 2:
                print(f"    번역: {item['trans_info'][2][:60]}...")
            print()

        # 수정이 필요한 라인 출력
        print("=" * 80)
        print("수정 필요 목록 (번역파일 라인번호)")
        print("=" * 80)
        for item in mismatched:
            print(f"  {item['trans_info'][1]}행: NPC {item['trans_info'][0]}")
    else:
        print("모든 번역 키가 원문과 일치합니다!")

    print()
    print("검증 완료!")

if __name__ == '__main__':
    main()
