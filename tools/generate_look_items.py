#!/usr/bin/env python3
"""Generate look_items_ko.txt with correct tile_num indices from tileflag_data.txt"""

import re
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
base_dir = os.path.dirname(script_dir)

# Read tileflag_data.txt to get tile_num -> name mapping
tile_to_name = {}
tileflag_path = os.path.join(base_dir, 'docs/ultima6/tileflag_data.txt')
with open(tileflag_path, 'r') as f:
    for line in f:
        match = re.match(r'^(\d{4})\s*:\s*\d+\s+\d+\s+\d+\s+(.+)$', line.strip())
        if match:
            tile_num = int(match.group(1))
            name = match.group(2).strip()
            tile_to_name[tile_num] = name

# Read existing Korean translations (name -> korean)
name_to_korean = {}
look_items_path = os.path.join(base_dir, 'data/translations/korean/look_items_ko.txt')
with open(look_items_path, 'r', encoding='utf-8') as f:
    for line in f:
        if line.startswith('#') or '|' not in line:
            continue
        parts = line.strip().split('|')
        if len(parts) >= 3:
            english = parts[1].strip().lower()
            korean = parts[2].strip()
            if english and korean:
                name_to_korean[english] = korean

# Generate new file with correct tile_num indices
output_lines = ['# Item descriptions - Korean translation (indexed by tile_num)',
                '# Format: TILE_NUM|ENGLISH|KOREAN',
                '# Reference: docs/ultima6/tileflag_data.txt', '']

for tile_num in sorted(tile_to_name.keys()):
    name = tile_to_name[tile_num]
    name_lower = name.lower()
    if name_lower in name_to_korean:
        korean = name_to_korean[name_lower]
        output_lines.append(f'{tile_num}|{name}|{korean}')

output_path = os.path.join(base_dir, 'data/translations/korean/look_items_ko_new.txt')
with open(output_path, 'w', encoding='utf-8') as f:
    f.write('\n'.join(output_lines))

print(f"Generated {len(output_lines)-4} entries to look_items_ko_new.txt")
print("\nSample entries around clock/desk/ribs:")
for line in output_lines:
    if 'clock' in line.lower() or 'desk' in line.lower() or 'ribs' in line.lower():
        print(line)
