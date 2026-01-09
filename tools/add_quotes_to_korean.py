#!/usr/bin/env python3
"""
Add quotes to Korean translations in dialogues_ko.txt.
Format: NPC|"English"|"Korean"
"""

import os

def main():
    filepath = "d:/proj/Ultima6/nuvie/data/translations/korean/dialogues_ko.txt"

    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    modified = 0

    for line in lines:
        # Keep comments and empty lines
        if not line.strip() or line.strip().startswith('#'):
            new_lines.append(line)
            continue

        parts = line.rstrip('\n').split('|', 2)
        if len(parts) != 3:
            new_lines.append(line)
            continue

        npc_str = parts[0]
        english = parts[1]
        korean = parts[2]

        # Skip if placeholder or already has quotes
        if '<' in korean or korean.startswith('"'):
            new_lines.append(line)
            continue

        # Add quotes to Korean if not empty
        if korean:
            korean = '"' + korean + '"'
            modified += 1

        new_lines.append(f"{npc_str}|{english}|{korean}\n")

    with open(filepath, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)

    print(f"Added quotes to {modified} Korean translations")
    print(f"Written to {filepath}")

if __name__ == "__main__":
    main()
