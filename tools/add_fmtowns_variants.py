#!/usr/bin/env python3
"""
FM Towns 변형 대사를 dialogues_ko.txt에 추가
DOS 번역을 참고하여 한국어 번역 생성
"""

import re
from pathlib import Path
from collections import defaultdict


def load_dos_translations(filepath):
    """DOS 번역 파일 로드 (NPC별)"""
    translations = defaultdict(dict)  # npc_num -> {english: korean}

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            parts = line.split('|')
            if len(parts) < 3:
                continue

            try:
                npc_num = int(parts[0])
                english = parts[1]
                korean = parts[2]
                translations[npc_num][english] = korean
            except ValueError:
                continue

    return translations


def normalize_for_lookup(text):
    """lookup용 정규화"""
    # @ 제거
    text = text.replace('@', '')
    # 줄바꿈 정리
    text = text.replace('\\n', '\n')
    # 공백 정규화
    text = ' '.join(text.split())
    return text.strip()


def find_dos_translation(npc_num, dos_text, dos_translations):
    """DOS 번역 찾기"""
    npc_trans = dos_translations.get(npc_num, {})

    # 정확히 일치
    if dos_text in npc_trans:
        return npc_trans[dos_text]

    # 정규화해서 찾기
    dos_norm = normalize_for_lookup(dos_text)
    for eng, kor in npc_trans.items():
        if normalize_for_lookup(eng) == dos_norm:
            return kor

    # 부분 일치 (DOS 텍스트가 일부만 있는 경우)
    for eng, kor in npc_trans.items():
        eng_norm = normalize_for_lookup(eng)
        if dos_norm in eng_norm or eng_norm in dos_norm:
            return kor

    return None


def adapt_korean_for_fmtowns(fmtowns_text, dos_text, korean_text):
    """FM Towns 대사에 맞게 한국어 번역 수정"""
    if not korean_text:
        return None

    result = korean_text

    # FM Towns에 * 가 있으면 추가
    if fmtowns_text.endswith('*') and not result.endswith('*'):
        result = result.rstrip() + ' *'

    # $Y -> 실제 이름으로 변환 (FM Towns에서 하드코딩된 경우)
    if 'Dupre' in fmtowns_text and '$Y' in dos_text:
        result = result.replace('$Y', '듀프레(Dupre)')
    if 'Iolo' in fmtowns_text and '$Y' in dos_text:
        result = result.replace('$Y', '아이올로(Iolo)')
    if 'Shamino' in fmtowns_text and '$Y' in dos_text:
        result = result.replace('$Y', '샤미노(Shamino)')

    # $N -> 실제 이름
    if 'Hendle' in fmtowns_text and '$N' in dos_text:
        result = result.replace('$N', '헨들(Hendle)')

    # FM Towns에서 sir/madam 추가된 경우
    if 'sir?' in fmtowns_text.lower() and 'sir' not in dos_text.lower():
        # 한국어에서 적절한 위치에 추가
        if result.endswith('?"'):
            result = result[:-2] + ', 선생님?"'
        elif result.endswith('"'):
            result = result[:-1] + ', 선생님"'
    if 'madam?' in fmtowns_text.lower() and 'madam' not in dos_text.lower():
        if result.endswith('?"'):
            result = result[:-2] + ', 부인?"'
        elif result.endswith('"'):
            result = result[:-1] + ', 부인"'

    # FM Towns에서 boy/girl 구분된 경우
    if 'naughty boy!' in fmtowns_text.lower():
        result = result.replace('장난꾸러기', '장난꾸러기 소년')
    if 'naughty girl!' in fmtowns_text.lower():
        result = result.replace('장난꾸러기', '장난꾸러기 소녀')

    return result


def main():
    korean_dir = Path("d:/proj/Ultima6/nuvie/data/translations/korean")

    variants_file = korean_dir / "fmtowns_variants.txt"
    dialogues_file = korean_dir / "dialogues_ko.txt"

    print("=" * 70)
    print("FM Towns 변형 대사 추가")
    print("=" * 70)

    # DOS 번역 로드
    print("\nLoading DOS translations...")
    dos_translations = load_dos_translations(dialogues_file)

    # 변형 파일 파싱
    print("Parsing variants...")
    variants = []

    with open(variants_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            parts = line.split('|')
            if len(parts) < 4:
                continue

            try:
                npc_num = int(parts[0])
                fmtowns_text = parts[1]
                dos_text = parts[2]
                similarity = parts[3]
                variants.append((npc_num, fmtowns_text, dos_text, similarity))
            except (ValueError, IndexError):
                continue

    print(f"  Found {len(variants)} variants")

    # 번역 생성
    print("\nGenerating translations...")
    new_translations = []
    failed = []

    for npc_num, fmtowns_text, dos_text, similarity in variants:
        # DOS 번역 찾기
        korean = find_dos_translation(npc_num, dos_text, dos_translations)

        if korean:
            # FM Towns에 맞게 수정
            adapted = adapt_korean_for_fmtowns(fmtowns_text, dos_text, korean)
            if adapted:
                new_translations.append((npc_num, fmtowns_text, adapted))
            else:
                failed.append((npc_num, fmtowns_text, dos_text, "adaptation failed"))
        else:
            failed.append((npc_num, fmtowns_text, dos_text, "no DOS translation"))

    print(f"  Generated: {len(new_translations)}")
    print(f"  Failed: {len(failed)}")

    # 실패한 것들 출력
    if failed:
        print("\n실패한 항목:")
        for npc_num, fmtowns, dos, reason in failed[:10]:
            print(f"  NPC {npc_num}: {reason}")
            print(f"    FM: {fmtowns[:60]}...")
            print(f"    DOS: {dos[:60]}...")

    # dialogues_ko.txt에 추가
    print("\n" + "-" * 70)
    print("dialogues_ko.txt에 추가할 내용:")
    print("-" * 70)

    # NPC별로 그룹화
    by_npc = defaultdict(list)
    for npc_num, fmtowns, korean in new_translations:
        by_npc[npc_num].append((fmtowns, korean))

    # 추가할 라인들 생성
    lines_to_add = []
    lines_to_add.append("\n# FM Towns 변형 대사 (자동 생성)")
    lines_to_add.append("# DOS와 약간 다른 FM Towns 대사들")

    for npc_num in sorted(by_npc.keys()):
        for fmtowns, korean in by_npc[npc_num]:
            # 이스케이프
            fmtowns_escaped = fmtowns.replace('\n', '\\n')
            korean_escaped = korean.replace('\n', '\\n')
            lines_to_add.append(f"{npc_num}|{fmtowns_escaped}|{korean_escaped}")

    # 미리보기
    for line in lines_to_add[:20]:
        print(line)
    if len(lines_to_add) > 20:
        print(f"... ({len(lines_to_add) - 20} more lines)")

    # 파일에 추가
    print(f"\n총 {len(new_translations)}개 번역 추가 중...")

    with open(dialogues_file, 'a', encoding='utf-8') as f:
        f.write('\n')
        for line in lines_to_add:
            f.write(line + '\n')

    print(f"완료: {dialogues_file}")

    # 실패한 것들 별도 파일로 저장
    if failed:
        failed_file = korean_dir / "fmtowns_failed.txt"
        with open(failed_file, 'w', encoding='utf-8') as f:
            f.write("# FM Towns 변형 - 수동 번역 필요\n")
            f.write(f"# 총 {len(failed)}개\n\n")
            for npc_num, fmtowns, dos, reason in failed:
                f.write(f"# {reason}\n")
                f.write(f"{npc_num}|{fmtowns}|<번역 필요>\n")
                f.write(f"# DOS 참고: {dos}\n\n")
        print(f"실패 항목 저장: {failed_file}")

    return 0


if __name__ == "__main__":
    main()
