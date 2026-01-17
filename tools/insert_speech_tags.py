#!/usr/bin/env python3
"""
FM Towns ~P 태그를 한국어 번역 파일에 삽입
speech_tags_fmtowns.txt에서 추출한 태그를 dialogues_ko.txt에 삽입
"""

import re
import sys
from pathlib import Path
from collections import defaultdict


def normalize_for_match(text):
    """매칭을 위한 텍스트 정규화"""
    # ~P, ~L 태그 제거
    text = re.sub(r'~[PL]\d+', '', text)
    # 줄바꿈 통일
    text = text.replace('\\n', '\n').replace('\r\n', '\n').replace('\r', '\n')
    # 공백 정규화
    text = ' '.join(text.split())
    return text.strip()


def extract_dialogue_only(text):
    """따옴표 안의 대사만 추출"""
    # 따옴표 안의 내용만 추출
    matches = re.findall(r'"([^"]*)"', text)
    if matches:
        return ' '.join(matches)
    return text


def load_speech_tags(filepath):
    """speech_tags_fmtowns.txt에서 ~P 태그 로드"""
    tags = defaultdict(list)  # npc_num -> [(tag, normalized_text, full_text)]

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            parts = line.split('|', 2)
            if len(parts) < 3:
                continue

            try:
                npc_num = int(parts[0])
                tag = parts[1]  # ~P1, ~P2, etc.
                full_text = parts[2].replace('\\n', '\n')

                # 정규화된 대사 텍스트
                dialogue_text = extract_dialogue_only(full_text)
                normalized = normalize_for_match(dialogue_text)

                tags[npc_num].append({
                    'tag': tag,
                    'tag_num': int(tag[2:]),  # ~P숫자에서 숫자만
                    'full_text': full_text,
                    'dialogue': dialogue_text,
                    'normalized': normalized
                })
            except (ValueError, IndexError):
                continue

    return tags


def load_translations(filepath):
    """dialogues_ko.txt 로드"""
    translations = []  # [(npc_num, english, korean, line_num)]

    with open(filepath, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.rstrip('\n')

            # 헤더나 빈 줄 스킵
            if not line or line.startswith('#'):
                translations.append((None, None, None, line_num, line))
                continue

            parts = line.split('|')
            if len(parts) < 3:
                translations.append((None, None, None, line_num, line))
                continue

            try:
                npc_num = int(parts[0])
                english = parts[1]
                korean = parts[2]
                translations.append((npc_num, english, korean, line_num, line))
            except ValueError:
                translations.append((None, None, None, line_num, line))

    return translations


def find_best_tag_match(english_text, speech_tags):
    """영어 텍스트에 가장 잘 맞는 ~P 태그 찾기"""
    # 영어 텍스트 정규화
    english_dialogue = extract_dialogue_only(english_text)
    english_norm = normalize_for_match(english_dialogue)

    if not english_norm:
        return None

    best_match = None
    best_score = 0

    for tag_info in speech_tags:
        tag_norm = tag_info['normalized']

        # 정확히 일치하면 즉시 반환
        if english_norm == tag_norm:
            return tag_info

        # 부분 일치 체크 (영어 텍스트가 태그 텍스트에 포함되거나 그 반대)
        if english_norm in tag_norm or tag_norm in english_norm:
            # 더 긴 매칭이 더 좋음
            score = len(tag_norm) if english_norm in tag_norm else len(english_norm)
            if score > best_score:
                best_score = score
                best_match = tag_info

    # 일정 수준 이상의 매칭만 반환
    if best_match and best_score > 20:
        return best_match

    return None


def insert_tag_into_korean(korean_text, tag):
    """한국어 텍스트에 ~P 태그 삽입 (따옴표 뒤에)"""
    # 이미 태그가 있으면 스킵
    if '~P' in korean_text:
        return korean_text, False

    # 따옴표가 있는 경우: 마지막 따옴표 뒤에 삽입
    if '"' in korean_text:
        # 마지막 닫는 따옴표 찾기
        last_quote = korean_text.rfind('"')
        if last_quote >= 0:
            # 따옴표 바로 뒤에 삽입
            return korean_text[:last_quote+1] + tag + korean_text[last_quote+1:], True

    # 따옴표가 없는 경우: 끝에 삽입
    return korean_text + tag, True


def main():
    korean_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    speech_tags_file = korean_dir / "speech_tags_fmtowns.txt"
    dialogues_file = korean_dir / "dialogues_ko.txt"
    output_file = korean_dir / "dialogues_ko_with_speech.txt"

    if not speech_tags_file.exists():
        print(f"Error: {speech_tags_file} not found")
        print("Run extract_speech_tags.py first")
        return 1

    if not dialogues_file.exists():
        print(f"Error: {dialogues_file} not found")
        return 1

    print("=" * 70)
    print("FM Towns Speech Tag Inserter")
    print("한국어 번역에 ~P 태그 삽입")
    print("=" * 70)

    # 데이터 로드
    print("\nLoading speech tags...")
    speech_tags = load_speech_tags(speech_tags_file)
    total_tags = sum(len(v) for v in speech_tags.values())
    print(f"  Loaded {total_tags} tags from {len(speech_tags)} NPCs")

    print("\nLoading translations...")
    translations = load_translations(dialogues_file)
    print(f"  Loaded {len(translations)} lines")

    # 태그 삽입
    print("\nInserting tags...")
    inserted_count = 0
    skipped_existing = 0
    no_match = 0

    output_lines = []

    for item in translations:
        npc_num, english, korean, line_num, original_line = item

        # 헤더나 빈 줄은 그대로
        if npc_num is None:
            output_lines.append(original_line)
            continue

        # 이 NPC의 태그가 없으면 그대로
        if npc_num not in speech_tags:
            output_lines.append(original_line)
            no_match += 1
            continue

        # 최적의 태그 찾기
        tag_info = find_best_tag_match(english, speech_tags[npc_num])

        if tag_info:
            new_korean, was_inserted = insert_tag_into_korean(korean, tag_info['tag'])
            if was_inserted:
                output_lines.append(f"{npc_num}|{english}|{new_korean}")
                inserted_count += 1
            else:
                output_lines.append(original_line)
                skipped_existing += 1
        else:
            output_lines.append(original_line)
            no_match += 1

    # 결과 저장
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(output_lines))

    print(f"\n결과:")
    print(f"  삽입된 태그: {inserted_count}")
    print(f"  이미 존재: {skipped_existing}")
    print(f"  매칭 실패: {no_match}")
    print(f"\n저장됨: {output_file}")

    # 사용자에게 확인 요청
    print("\n확인 후 다음 명령으로 적용:")
    print(f"  cp '{output_file}' '{dialogues_file}'")

    return 0


if __name__ == "__main__":
    sys.exit(main())
