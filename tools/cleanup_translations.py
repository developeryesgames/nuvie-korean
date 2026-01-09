#!/usr/bin/env python3
"""
Cleanup mismatched translations in dialogues_ko.txt.
Removes translations where Korean text doesn't match the English context.
"""

import os
import re

def main():
    base_dir = "d:/proj/Ultima6/nuvie/data/translations/korean"
    dialogues_file = os.path.join(base_dir, "dialogues_ko.txt")

    # Known bad mappings to clean up (English patterns that got wrong Korean)
    # Format: (npc_num, english_pattern, expected_korean_start)
    # If the korean doesn't start with expected, it's wrong
    cleanup_rules = [
        # Iolo's lines that got wrong translations
        (4, "Or I could @split it up evenly", None),  # Should be about splitting gold
        (4, "I'm sure she'll carry on the tradition", None),  # Should be about apprentice
        (4, "But after this @quest I hope to join her", None),  # Should be about Gwenno
    ]

    # Read dialogues_ko.txt
    with open(dialogues_file, 'r', encoding='utf-8') as f:
        content = f.read()

    lines = content.split('\n')
    new_lines = []
    cleaned = 0

    for line in lines:
        if not line or line.startswith('#'):
            new_lines.append(line)
            continue

        parts = line.split('|', 2)
        if len(parts) != 3:
            new_lines.append(line)
            continue

        npc_str = parts[0]
        english = parts[1]
        korean = parts[2]

        try:
            npc_num = int(npc_str)
        except:
            new_lines.append(line)
            continue

        # Check if this is a known bad mapping
        should_clean = False
        for rule_npc, rule_pattern, expected in cleanup_rules:
            if npc_num == rule_npc and rule_pattern in english:
                if expected is None or not korean.startswith(expected):
                    should_clean = True
                    break

        if should_clean and korean != '<번역 필요>':
            new_lines.append(f"{npc_str}|{english}|<번역 필요>")
            cleaned += 1
        else:
            new_lines.append(line)

    # Write back
    with open(dialogues_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(new_lines))

    print(f"Cleaned {cleaned} mismatched translations")
    print(f"Written to {dialogues_file}")

if __name__ == "__main__":
    main()
