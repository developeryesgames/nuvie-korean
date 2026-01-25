#!/usr/bin/env python3
# -*- coding: utf-8 -*-

def parse_file(filepath):
    entries = set()
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('|')
            if len(parts) >= 2:
                entries.add(parts[1])
    return entries

original = parse_file('dialogues_ko_original.txt')
translation = parse_file('dialogues_ko.txt')

missing = original - translation
print(f'원본에만 있고 번역에 없는 항목: {len(missing)}개')

if missing:
    print('\n=== 누락된 항목 ===')
    for i, item in enumerate(sorted(list(missing))[:20], 1):
        print(f'{i}. {item[:100]}')
