#!/usr/bin/env python3
"""
Sync translations from npc_dialogues_ko.txt to dialogues_ko.txt.
Uses exact text matching where possible.
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

def build_exact_mappings():
    """Build exact English -> Korean mappings for common phrases."""
    return {
        # Greetings
        "Yes, my friend?": "왜, 친구?",
        "Yes, $P?": "그래, $P?",
        "Yes?": "예?",
        "Hello.": "안녕.",
        "Hi!": "안녕!",
        "Welcome!": "환영합니다!",
        "Greetings, $G.": "안녕하시오, $G.",
        "Hail, $G.": "안녕, $G.",
        "Good $T, $G.": "좋은 $T 되시오, $G.",
        "Good day.": "좋은 하루.",
        "Good day, $G.": "좋은 하루 되시오, $G.",

        # Farewells
        "A pleasure.": "즐거웠소.",
        "Goodbye.": "안녕.",
        "Farewell.": "잘 가게.",
        "Good day.": "좋은 하루.",
        "Certainly.": "물론이지.",

        # Common responses
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
        "I cannot help thee with that.": "도와줄 수 없소.",
        "I can't help you with that.": "도와드릴 수 없네요.",
        "I'm sorry.": "미안하오.",
        "No.": "아니.",
        "Yes.": "예.",
        "Maybe.": "아마도.",
        "Perhaps.": "아마도.",
        "What dost thou need?": "무엇이 필요한가?",
        "What wouldst thou like?": "무엇을 원하시오?",
        "How may I help thee?": "어떻게 도와드릴까요?",

        # Referrals
        "Ask Dupre about that.": "그건 듀프레에게 물어보시오.",
        "Ask Iolo about that.": "그건 이올로에게 물어봐.",
        "Ask Shamino about that.": "그건 샤미노에게 물어봐.",
    }

def main():
    base_dir = "d:/proj/Ultima6/nuvie/data/translations/korean"
    npc_dialogues_file = os.path.join(base_dir, "npc_dialogues_ko.txt")
    dialogues_file = os.path.join(base_dir, "dialogues_ko.txt")

    # Load keyword-based translations
    npc_dialogues = load_npc_dialogues(npc_dialogues_file)
    print(f"Loaded NPC dialogues for {len(npc_dialogues)} NPCs")

    # Get exact mappings
    exact_mappings = build_exact_mappings()

    # Read dialogues_ko.txt
    with open(dialogues_file, 'r', encoding='utf-8') as f:
        content = f.read()

    lines = content.split('\n')
    new_lines = []
    updated = 0

    for line in lines:
        # Keep comments and empty lines
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

        # Skip if already translated (not placeholder)
        if korean and korean != '<번역 필요>':
            new_lines.append(line)
            continue

        try:
            npc_num = int(npc_str)
        except:
            new_lines.append(line)
            continue

        # Clean English text for matching (remove quotes)
        clean_english = english.strip('"')

        # Try exact mapping first
        new_korean = exact_mappings.get(clean_english)

        # If no exact match, try NPC-specific for very specific patterns
        if not new_korean and npc_num in npc_dialogues:
            npc_data = npc_dialogues[npc_num]

            # Only match very specific patterns that we're confident about

            # "bye" keyword matches
            if 'bye' in npc_data:
                bye_patterns = [
                    "enough chit chat",
                    "pleasure talking",
                    "farewell",
                    "goodbye",
                ]
                if any(p in clean_english.lower() for p in bye_patterns):
                    ko_text = npc_data['bye']
                    # Take first line only
                    new_korean = ko_text.split('\\n')[0] if '\\n' in ko_text else ko_text

        if new_korean:
            new_lines.append(f"{npc_str}|{english}|{new_korean}")
            updated += 1
        else:
            new_lines.append(line)

    # Write back
    with open(dialogues_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(new_lines))

    print(f"\nUpdated {updated} translations")
    print(f"Written to {dialogues_file}")

if __name__ == "__main__":
    main()
