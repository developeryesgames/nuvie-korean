#!/usr/bin/env python3
"""
Expand short greetings by appending relevant content from other keywords.
"""

import os

def load_translations(filepath):
    """Load translations preserving order."""
    data = []
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n\r')
            data.append(line)
    return data

def parse_npc_data(lines):
    """Parse NPC data into structured format."""
    npcs = {}
    for i, line in enumerate(lines):
        if not line or line.startswith('#'):
            continue
        parts = line.split('|', 2)
        if len(parts) == 3:
            try:
                npc_num = int(parts[0])
                keyword = parts[1]
                text = parts[2]
                if npc_num not in npcs:
                    npcs[npc_num] = {}
                npcs[npc_num][keyword] = text
            except:
                pass
    return npcs

def expand_greeting(npcs, npc_num):
    """Expand greeting by appending job/name info if greeting is short."""
    if npc_num not in npcs:
        return None

    npc = npcs[npc_num]
    if 'greeting' not in npc:
        return None

    greeting = npc['greeting']
    # Count actual Korean characters (rough estimate)
    greeting_len = len(greeting.replace('\\n', ''))

    # If greeting is already long enough (>100 chars), skip
    if greeting_len > 100:
        return None

    additions = []

    # Priority order for expansion: job, name, then other story-related keywords
    expand_keywords = ['job', 'name']

    for kw in expand_keywords:
        if kw in npc and kw != 'greeting' and kw != 'desc':
            text = npc[kw]
            # Don't add if it's too short or just a redirect
            if len(text) > 20 and '@' not in text[:20]:
                additions.append(text)
                if greeting_len + sum(len(a) for a in additions) > 150:
                    break

    if additions:
        new_greeting = greeting + '\\n' + '\\n'.join(additions)
        return new_greeting

    return None

def process_file(filepath):
    """Process and update the translation file."""
    lines = load_translations(filepath)
    npcs = parse_npc_data(lines)

    # Track expansions
    expansions = {}

    for npc_num in npcs:
        expanded = expand_greeting(npcs, npc_num)
        if expanded:
            expansions[npc_num] = expanded
            print(f"NPC {npc_num}: Expanding greeting")

    if not expansions:
        print("No greetings need expansion.")
        return

    # Update lines
    new_lines = []
    for line in lines:
        if not line or line.startswith('#'):
            new_lines.append(line)
            continue

        parts = line.split('|', 2)
        if len(parts) == 3:
            try:
                npc_num = int(parts[0])
                keyword = parts[1]
                if keyword == 'greeting' and npc_num in expansions:
                    new_lines.append(f"{npc_num}|greeting|{expansions[npc_num]}")
                    continue
            except:
                pass
        new_lines.append(line)

    # Write back
    with open(filepath, 'w', encoding='utf-8') as f:
        for line in new_lines:
            f.write(line + '\n')

    print(f"\nExpanded {len(expansions)} greetings.")

if __name__ == "__main__":
    trans_file = "d:/proj/Ultima6/nuvie/data/translations/korean/npc_dialogues_ko.txt"
    process_file(trans_file)
