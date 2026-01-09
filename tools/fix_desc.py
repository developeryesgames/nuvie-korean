#!/usr/bin/env python3
"""Fix DESC to use English text-based translation instead of keyword-based."""

import os

def main():
    filepath = "d:/proj/Ultima6/nuvie/ConverseInterpret.cpp"

    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    old_code = """            if (korean && korean->isEnabled())
            {
                // Check if NPC is in party for conditional description
                std::string korean_desc;
                Actor *npc_actor = converse->actors->get_actor(converse->script_num);
                bool is_in_party = npc_actor && game->get_party()->contains_actor(npc_actor);

                if (is_in_party)
                    korean_desc = korean->getNPCDialogue(converse->script_num, "desc_party");

                if (korean_desc.empty())
                    korean_desc = korean->getNPCDialogue(converse->script_num, "desc");

                if (!korean_desc.empty())"""

    new_code = """            if (korean && korean->isEnabled())
            {
                // Use English text-based translation for DESC
                std::string korean_desc = korean->getDialogueTranslation(converse->script_num, converse->desc);

                if (!korean_desc.empty())"""

    if old_code in content:
        content = content.replace(old_code, new_code)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print("Fixed!")
    else:
        print("Pattern not found - maybe already fixed?")
        # Check if new code exists
        if "// Use English text-based translation for DESC" in content:
            print("Already using new code!")
        else:
            print("Unknown state")

if __name__ == "__main__":
    main()
