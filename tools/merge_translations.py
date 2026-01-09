#!/usr/bin/env python3
"""
Merge translations from npc_dialogues_ko.txt to dialogues_ko.txt.
npc_dialogues_ko.txt has keyword-based translations (desc, greeting, bye, etc.)
dialogues_ko.txt has English text-based translations.

This script tries to match them and fill in translations.
"""

import os
import re

def load_npc_dialogues(filepath):
    """Load keyword-based NPC dialogues."""
    dialogues = {}  # npc_num -> {keyword: korean_text}

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            parts = line.split('|', 2)
            if len(parts) != 3:
                continue

            try:
                npc_num = int(parts[0])
            except:
                continue

            keyword = parts[1]
            korean = parts[2]

            if npc_num not in dialogues:
                dialogues[npc_num] = {}
            dialogues[npc_num][keyword] = korean

    return dialogues

def main():
    base_dir = "d:/proj/Ultima6/nuvie/data/translations/korean"
    npc_dialogues_file = os.path.join(base_dir, "npc_dialogues_ko.txt")
    dialogues_file = os.path.join(base_dir, "dialogues_ko.txt")
    output_file = os.path.join(base_dir, "dialogues_ko_merged.txt")

    # Load keyword-based translations
    npc_dialogues = load_npc_dialogues(npc_dialogues_file)
    print(f"Loaded NPC dialogues for {len(npc_dialogues)} NPCs")

    # Build a mapping of common English phrases to Korean
    # These are generic responses that appear in multiple NPCs
    common_phrases = {
        "A pleasure.": "즐거웠소.",
        "Certainly.": "물론이지.",
        "Yes?": "예?",
        "Yes, my friend?": "예, 친구?",
        "Ask Dupre about that.": "듀프레에게 물어봐.",
        "Ask Iolo about that.": "이올로에게 물어봐.",
        "Ask Shamino about that.": "샤미노에게 물어봐.",
        "Goodbye.": "안녕.",
        "Farewell.": "잘 가게.",
        "Good day.": "좋은 하루.",
        "Good day, $G.": "좋은 하루 되시오, $G.",
        "Good $T, $G.": "좋은 $T 되시오, $G.",
        "What dost thou need?": "무엇이 필요한가?",
        "What wouldst thou like?": "무엇을 원하시오?",
        "How may I help thee?": "어떻게 도와드릴까요?",
        "Greetings, $G.": "안녕하시오, $G.",
        "Hail, $G.": "안녕, $G.",
        "Hello.": "안녕.",
        "Hi!": "안녕!",
        "Welcome!": "환영합니다!",
        "Thank you.": "고맙소.",
        "Thanks.": "고마워.",
        "Very well.": "좋소.",
        "Of course.": "물론이지.",
        "Indeed.": "그렇소.",
        "I see.": "그렇군.",
        "Interesting.": "흥미롭군.",
        "Really?": "정말?",
        "I don't know.": "모르겠소.",
        "I cannot help thee.": "도와줄 수 없소.",
        "I'm sorry.": "미안하오.",
        "No.": "아니.",
        "Yes.": "예.",
        "Maybe.": "아마도.",
        "Perhaps.": "아마도.",
    }

    # Read and process dialogues_ko.txt
    lines = []
    updated = 0

    with open(dialogues_file, 'r', encoding='utf-8') as f:
        for line in f:
            original = line
            line = line.rstrip('\n')

            # Keep comments and empty lines as-is
            if not line or line.startswith('#'):
                lines.append(original)
                continue

            parts = line.split('|', 2)
            if len(parts) != 3:
                lines.append(original)
                continue

            npc_str = parts[0]
            english = parts[1]
            korean = parts[2]

            # Skip if already translated
            if korean and '<' not in korean:
                lines.append(original)
                continue

            try:
                npc_num = int(npc_str)
            except:
                lines.append(original)
                continue

            # Clean English text for matching
            clean_english = english.strip('"')

            # Try to find translation
            new_korean = None

            # Check common phrases first
            if clean_english in common_phrases:
                new_korean = common_phrases[clean_english]

            # Check NPC-specific dialogues
            elif npc_num in npc_dialogues:
                npc_data = npc_dialogues[npc_num]

                # Try to match by content similarity
                for keyword, ko_text in npc_data.items():
                    # For bye responses
                    if keyword == 'bye' and ('pleasure' in clean_english.lower() or
                                              'farewell' in clean_english.lower() or
                                              'goodbye' in clean_english.lower()):
                        new_korean = ko_text
                        break

                    # For greeting responses
                    if keyword == 'greeting' and ('yes' in clean_english.lower() and
                                                   ('friend' in clean_english.lower() or
                                                    '$p' in clean_english.lower())):
                        # Extract just the greeting part
                        if '\\n' in ko_text:
                            new_korean = ko_text.split('\\n')[0]
                        else:
                            new_korean = ko_text
                        break

            if new_korean:
                lines.append(f"{npc_str}|{english}|{new_korean}\n")
                updated += 1
            else:
                lines.append(original)

    # Write output
    with open(output_file, 'w', encoding='utf-8') as f:
        f.writelines(lines)

    print(f"Updated {updated} translations")
    print(f"Written to {output_file}")

if __name__ == "__main__":
    main()
