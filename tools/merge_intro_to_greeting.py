#!/usr/bin/env python3
"""
Merge 'intro' keyword content into 'greeting' for all NPCs.
This ensures the first dialogue shows the full introduction.
"""

import os
import re

def process_translations(filepath):
    """Process translation file and merge intro into greeting."""

    # Read all lines preserving structure
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # Parse into NPCs
    npcs = {}  # npc_num -> {keyword -> (line_num, text)}
    comments = {}  # line_num -> comment

    for i, line in enumerate(lines):
        stripped = line.rstrip('\n\r')
        if not stripped or stripped.startswith('#'):
            comments[i] = stripped
            continue

        parts = stripped.split('|', 2)
        if len(parts) == 3:
            try:
                npc_num = int(parts[0])
                keyword = parts[1]
                text = parts[2]
                if npc_num not in npcs:
                    npcs[npc_num] = {}
                npcs[npc_num][keyword] = (i, text)
            except:
                comments[i] = stripped

    # Find NPCs with both greeting and intro
    merged_count = 0
    for npc_num in npcs:
        if 'greeting' in npcs[npc_num] and 'intro' in npcs[npc_num]:
            greeting_line, greeting_text = npcs[npc_num]['greeting']
            intro_line, intro_text = npcs[npc_num]['intro']

            # Check if intro is not already in greeting
            if intro_text not in greeting_text:
                # Merge: greeting + \n + intro
                new_greeting = greeting_text + "\\n" + intro_text
                npcs[npc_num]['greeting'] = (greeting_line, new_greeting)
                # Mark intro for removal
                npcs[npc_num]['intro'] = (intro_line, None)  # None means remove
                merged_count += 1
                print(f"NPC {npc_num}: Merged intro into greeting")

    # Rebuild file
    output_lines = []
    processed_lines = set()

    for i, line in enumerate(lines):
        if i in comments:
            output_lines.append(comments[i])
            continue

        stripped = line.rstrip('\n\r')
        parts = stripped.split('|', 2)
        if len(parts) == 3:
            try:
                npc_num = int(parts[0])
                keyword = parts[1]
                if npc_num in npcs and keyword in npcs[npc_num]:
                    line_num, text = npcs[npc_num][keyword]
                    if text is None:
                        # Skip removed lines (merged intros)
                        continue
                    output_lines.append(f"{npc_num}|{keyword}|{text}")
                    continue
            except:
                pass
        output_lines.append(stripped)

    # Write output
    with open(filepath, 'w', encoding='utf-8') as f:
        for line in output_lines:
            f.write(line + '\n')

    print(f"\nTotal merged: {merged_count} NPCs")

if __name__ == "__main__":
    trans_file = "d:/proj/Ultima6/nuvie/data/translations/korean/npc_dialogues_ko.txt"
    process_translations(trans_file)
