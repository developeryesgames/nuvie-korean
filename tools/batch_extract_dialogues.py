#!/usr/bin/env python3
"""
Batch extract all NPC dialogues and generate translation file.
Extracts ALL dialogue text from extracted NPC files.
Format: NPC_NUM|ENGLISH_TEXT|KOREAN_TEXT

NOTE: English text includes quotes as they appear in game script.
"""

import os
import re
import glob

def clean_text(text):
    """Clean dialogue text - remove control characters but keep variables."""
    # Remove control characters and binary junk
    text = re.sub(r'[Ë¿Óë§¤¡¢£¦²¨¶°î\x00-\x1f\x7f-\x9f]', '', text)
    # Remove section markers that got captured
    text = re.sub(r'\[KEYWORDS\].*$', '', text)
    text = re.sub(r'\[TEXT\].*$', '', text)
    text = re.sub(r'\[DESC\].*$', '', text)
    text = re.sub(r'[UXT\d]*\[', '', text)  # Remove leftover markers
    text = re.sub(r'\*+', ' ', text)  # Replace asterisks with space
    text = re.sub(r'\s+', ' ', text)  # Normalize whitespace
    return text.strip()

def extract_dialogues_from_file(npc_file):
    """Extract all dialogue texts from an NPC file, keeping quotes."""
    dialogues = []

    try:
        with open(npc_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except:
        return dialogues

    # Extract NPC number from filename
    basename = os.path.basename(npc_file)
    match = re.match(r'npc_(\d+)_', basename)
    if not match:
        return dialogues
    npc_num = int(match.group(1))

    # Find all quoted text including the quotes (as they appear in game)
    # Pattern: "text" - capture the whole thing with quotes
    quotes = re.findall(r'"([^"]+)"', content)

    seen = set()
    for quote in quotes:
        text = clean_text(quote)
        # Skip very short texts, already seen, or pure control sequences
        if len(text) < 3:
            continue
        if text in seen:
            continue
        # Skip texts that contain section markers (not actual dialogue)
        if '[KEYWORDS]' in text or '[TEXT]' in text or '[DESC]' in text:
            continue
        # Skip texts that are mostly non-ASCII garbage
        ascii_ratio = sum(1 for c in text if c.isascii() or c in '$@') / len(text)
        if ascii_ratio < 0.5:
            continue
        seen.add(text)
        # Store with quotes - this is how the game script stores it
        dialogues.append((npc_num, f'"{text}"'))

    return dialogues

def load_existing_translations(filepath):
    """Load existing translations to preserve them."""
    existing = {}  # (npc_num, english) -> korean
    if not os.path.exists(filepath):
        return existing
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split('|', 2)
                if len(parts) == 3:
                    try:
                        npc_num = int(parts[0])
                        english = parts[1]
                        korean = parts[2]
                        if korean and korean != '<번역 필요>':
                            # Store both with and without quotes
                            existing[(npc_num, english)] = korean
                            if english.startswith('"') and english.endswith('"'):
                                existing[(npc_num, english[1:-1])] = korean
                            else:
                                existing[(npc_num, f'"{english}"')] = korean
                    except:
                        pass
    except:
        pass
    return existing

def main():
    extracted_dir = "d:/proj/Ultima6/nuvie/tools/extracted"
    output_file = "d:/proj/Ultima6/nuvie/data/translations/korean/dialogues_ko.txt"

    # Load existing translations first
    existing = load_existing_translations(output_file)
    print(f"Loaded {len(existing)} existing translation entries")

    # Get all NPC files
    npc_files = sorted(glob.glob(os.path.join(extracted_dir, "npc_*.txt")))

    all_dialogues = []
    npc_names = {}

    for npc_file in npc_files:
        # Extract NPC name from filename
        basename = os.path.basename(npc_file)
        match = re.match(r'npc_(\d+)_(.+)\.txt', basename)
        if match:
            npc_num = int(match.group(1))
            npc_name = match.group(2).replace('_', ' ')
            npc_names[npc_num] = npc_name

        dialogues = extract_dialogues_from_file(npc_file)
        all_dialogues.extend(dialogues)

    print(f"Found {len(all_dialogues)} dialogue texts from {len(npc_files)} NPCs")

    # Group by NPC
    by_npc = {}
    for npc_num, text in all_dialogues:
        if npc_num not in by_npc:
            by_npc[npc_num] = []
        by_npc[npc_num].append(text)

    # Write output file
    preserved = 0
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# Dialogue translations\n")
        f.write("# Format: NPC_NUM|ENGLISH_TEXT|KOREAN_TEXT\n")
        f.write("# English text includes quotes as stored in game script\n")
        f.write("# Use \\n for line breaks in Korean text\n")
        f.write("# $P, $G, $T, $Z etc. will be replaced after translation\n")
        f.write("\n")

        for npc_num in sorted(by_npc.keys()):
            npc_name = npc_names.get(npc_num, f"Unknown_{npc_num}")
            f.write(f"# ===========================================\n")
            f.write(f"# NPC {npc_num}: {npc_name}\n")
            f.write(f"# ===========================================\n\n")

            for text in by_npc[npc_num]:
                # Escape pipe characters
                eng = text.replace('|', '\\|')

                # Check if we have existing translation
                korean = existing.get((npc_num, text))
                if not korean:
                    # Try without quotes
                    text_noquotes = text.strip('"')
                    korean = existing.get((npc_num, text_noquotes))

                if korean:
                    f.write(f"{npc_num}|{eng}|{korean}\n")
                    preserved += 1
                else:
                    f.write(f"{npc_num}|{eng}|<번역 필요>\n")

            f.write("\n")

    print(f"Wrote {output_file}")
    print(f"Total NPCs: {len(by_npc)}")
    print(f"Total dialogues: {len(all_dialogues)}")
    print(f"Preserved translations: {preserved}")

if __name__ == "__main__":
    main()
