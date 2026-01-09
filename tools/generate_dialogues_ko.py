#!/usr/bin/env python3
"""
Generate dialogues_ko.txt from extracted NPC files.
Format: ENGLISH_TEXT|KOREAN_TEXT

This creates a translation table where English dialogue text is the key.
"""

import os
import re
import glob

def extract_english_dialogues(npc_file):
    """Extract English dialogue texts from NPC file."""
    dialogues = []

    try:
        with open(npc_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except:
        return dialogues

    # Find all quoted text (dialogue)
    # Pattern: "text" followed by various control characters
    quotes = re.findall(r'"([^"]+)"', content)

    for quote in quotes:
        # Clean up the text
        text = quote.strip()
        if text and len(text) > 3:  # Skip very short texts
            dialogues.append(text)

    return dialogues

def main():
    extracted_dir = "d:/proj/Ultima6/nuvie/tools/extracted"
    output_file = "d:/proj/Ultima6/nuvie/data/translations/korean/dialogues_ko.txt"

    # Get all NPC files
    npc_files = glob.glob(os.path.join(extracted_dir, "npc_*.txt"))

    all_dialogues = set()

    for npc_file in npc_files:
        dialogues = extract_english_dialogues(npc_file)
        for d in dialogues:
            all_dialogues.add(d)

    print(f"Found {len(all_dialogues)} unique dialogue texts")

    # Write template file
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# Dialogue translations: English -> Korean\n")
        f.write("# Format: ENGLISH_TEXT|KOREAN_TEXT\n")
        f.write("# Use \\n for line breaks in Korean text\n\n")

        for dialogue in sorted(all_dialogues, key=len):
            # Escape any | in the English text
            eng = dialogue.replace('|', '\\|')
            f.write(f"# EN: {eng[:80]}{'...' if len(eng) > 80 else ''}\n")
            f.write(f"{eng}|<번역 필요>\n\n")

    print(f"Wrote template to {output_file}")
    print("Now you need to translate each line!")

if __name__ == "__main__":
    main()
