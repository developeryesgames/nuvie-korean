#!/usr/bin/env python3
"""
Expand short greeting translations to match original English dialogue length.
Analyzes extracted NPC files and updates npc_dialogues_ko.txt accordingly.
"""

import os
import re
import glob

def get_english_greeting_length(npc_num):
    """Get the approximate length of original English greeting from extracted file."""
    patterns = [
        f"d:/proj/Ultima6/nuvie/tools/extracted/npc_{npc_num:03d}_*.txt",
    ]

    for pattern in patterns:
        files = glob.glob(pattern)
        if files:
            try:
                with open(files[0], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    # Find DESC section which contains greeting
                    if '[DESC]' in content:
                        desc_start = content.find('[DESC]')
                        keywords_start = content.find('[KEYWORDS]', desc_start)
                        if keywords_start > desc_start:
                            desc_section = content[desc_start:keywords_start]
                            # Count quoted sentences (dialogue)
                            quotes = re.findall(r'"[^"]+?"', desc_section)
                            return len(quotes), desc_section
            except:
                pass
    return 0, ""

def load_translations(filepath):
    """Load current translations."""
    translations = {}
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('|', 2)
            if len(parts) == 3:
                npc_num = int(parts[0])
                keyword = parts[1]
                text = parts[2]
                if npc_num not in translations:
                    translations[npc_num] = {}
                translations[npc_num][keyword] = text
    return translations

def main():
    trans_file = "d:/proj/Ultima6/nuvie/data/translations/korean/npc_dialogues_ko.txt"
    translations = load_translations(trans_file)

    short_greetings = []

    for npc_num in sorted(translations.keys()):
        if 'greeting' in translations[npc_num]:
            greeting = translations[npc_num]['greeting']
            # Count Korean characters (excluding \n markers)
            korean_len = len(greeting.replace('\\n', ''))

            # Get English greeting info
            quote_count, desc_section = get_english_greeting_length(npc_num)

            # If English has many quotes but Korean is short
            if quote_count >= 3 and korean_len < 100:
                short_greetings.append({
                    'npc_num': npc_num,
                    'korean_greeting': greeting,
                    'korean_len': korean_len,
                    'english_quotes': quote_count,
                    'desc_section': desc_section[:500]
                })

    print(f"Found {len(short_greetings)} NPCs with potentially short greetings:")
    print("=" * 80)

    for item in short_greetings[:30]:  # Show first 30
        print(f"\nNPC {item['npc_num']}: Korean={item['korean_len']} chars, English={item['english_quotes']} quotes")
        print(f"  Korean: {item['korean_greeting'][:80]}...")
        # Show other available keywords for this NPC
        if item['npc_num'] in translations:
            other_keys = [k for k in translations[item['npc_num']].keys() if k != 'greeting' and k != 'desc']
            if other_keys:
                print(f"  Other keywords: {', '.join(other_keys)}")

if __name__ == "__main__":
    main()
