#!/usr/bin/env python3
"""NPC 정보 추출"""
import os
import re
import glob

npc_files = sorted(glob.glob("d:/proj/Ultima6/nuvie/tools/extracted/npc_*.txt"))

print("# Ultima 6 NPC 정보")
print("# 번호|이름|성별|DESC 설명")
print("")

for f in npc_files:
    match = re.search(r'npc_(\d+)_(.+)\.txt', f)
    if not match:
        continue
    num = int(match.group(1))
    name = match.group(2).replace('_', ' ')

    with open(f, 'r', encoding='utf-8', errors='replace') as fp:
        content = fp.read()

    # DESC 추출
    desc_match = re.search(r'\[DESC\]\s*\n(.+?)(?:\n\[|\*|$)', content)
    desc = desc_match.group(1).strip()[:100] if desc_match else ""

    # 성별 추측
    gender = "?"
    desc_lower = desc.lower()
    if "man" in desc_lower or "he " in desc_lower or "his " in desc_lower:
        gender = "M"
    elif "woman" in desc_lower or "she " in desc_lower or "her " in desc_lower or "lady" in desc_lower or "girl" in desc_lower or "maid" in desc_lower or "ess " in desc_lower:
        gender = "F"

    print(f"{num}|{name}|{gender}|{desc}")
