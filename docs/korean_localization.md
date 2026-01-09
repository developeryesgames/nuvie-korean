# Korean Localization System for Nuvie

## Overview

This document describes the Korean localization system for Nuvie (Ultima 6 engine).

## Key Files

### Translation Data Files
- `data/translations/korean/look_items_ko.txt` - Item/object name translations
- `data/translations/korean/ui_texts_ko.txt` - UI text translations
- `data/translations/korean/npc_dialogues_ko.txt` - NPC dialogue translations

### Source Files
- `KoreanTranslation.cpp/h` - Translation loading and lookup
- `fonts/KoreanFont.cpp/h` - Korean font rendering (16x16 BMP sprite sheet)
- `fonts/FontManager.cpp/h` - Font management with Korean mode support

## Critical: look_items_ko.txt Index System

### The Problem
The original `look_items_ko.txt` used sequential indices from the look.lzd file (e.g., index 127 = halberd). However, the game's `Look::get_description()` function uses **tile_num** for lookups, not sequential indices.

### The Solution
The `look_items_ko.txt` file must use **tile_num** as the index, not sequential file order.

### Index Mapping Reference

| Item | obj_n | tile_num | Old Index | Correct Index |
|------|-------|----------|-----------|---------------|
| halberd | 47 | 558 | 127 | 558 |
| sword | - | 554 | - | 554 |
| carpet | - | 1104-1121 | - | 1104-1121 |

### How to Find tile_num
1. **From tileflag_data.txt**: `docs/ultima6/tileflag_data.txt` contains all tile_num to name mappings
   ```
   0558 : 00000000 00000000 01001000 halberd
   ```

2. **From object_numbers.txt**: `docs/ultima6/object_numbers.txt` contains obj_n mappings
   ```
   0047: halberd
   ```

3. **Relationship**: `obj_n` -> `tile_num` via `obj_to_tile[]` array in TileManager

### File Format
```
# Comment lines start with #
TILE_NUM|ENGLISH_NAME|KOREAN_TRANSLATION

558|halberd|미늘창
554|sword|검
1104|carpet|양탄자
```

### Regenerating look_items_ko.txt

Use this Python script to regenerate the file with correct indices:

```python
import re

# Read tileflag_data.txt to get tile_num -> name mapping
tile_to_name = {}
with open('docs/ultima6/tileflag_data.txt', 'r') as f:
    for line in f:
        match = re.match(r'^(\d{4})\s*:\s*\d+\s+\d+\s+\d+\s+(.+)$', line.strip())
        if match:
            tile_num = int(match.group(1))
            name = match.group(2).strip()
            tile_to_name[tile_num] = name

# Read existing Korean translations (name -> korean)
name_to_korean = {}
with open('data/translations/korean/look_items_ko.txt', 'r', encoding='utf-8') as f:
    for line in f:
        if line.startswith('#') or '|' not in line:
            continue
        parts = line.strip().split('|')
        if len(parts) >= 3:
            english = parts[1].strip().lower()
            korean = parts[2].strip()
            if english and korean:
                name_to_korean[english] = korean

# Generate new file with correct tile_num indices
output_lines = ['# Item descriptions - Korean translation (indexed by tile_num)',
                '# Format: TILE_NUM|ENGLISH|KOREAN', '']

for tile_num in sorted(tile_to_name.keys()):
    name = tile_to_name[tile_num]
    name_lower = name.lower()
    if name_lower in name_to_korean:
        korean = name_to_korean[name_lower]
        output_lines.append(f'{tile_num}|{name}|{korean}')

with open('data/translations/korean/look_items_ko_new.txt', 'w', encoding='utf-8') as f:
    f.write('\n'.join(output_lines))
```

## Korean Particle System

Korean uses postpositional particles that change based on whether the preceding word ends with a consonant (받침/jongseong).

### Implemented Particles

| Particle | With Jongseong | Without Jongseong | Usage |
|----------|----------------|-------------------|-------|
| 을/를 | 을 | 를 | Object marker |
| 이/가 | 이 | 가 | Subject marker |
| 은/는 | 은 | 는 | Topic marker |
| 와/과 | 과 | 와 | And/with |
| 으로/로 | 으로 | 로 | Instrumental (special: ㄹ ending uses 로) |

### Usage Example
```cpp
#include "KoreanTranslation.h"

std::string item_name = "미늘창";  // halberd
std::string particle = KoreanTranslation::getParticle_euroro(item_name);
// Result: "으로" (because 창 ends with ㅇ jongseong)

scroll->display_string(item_name + particle + " 공격-");
// Output: "미늘창으로 공격-"
```

## Korean Font System

### Font Specifications
- Size: 16x16 pixels per character
- Format: BMP sprite sheet
- Encoding: UTF-8
- UI Scaling: 4x in original+ mode (rendered as 32x32)

### Unicode Range
- Hangul Syllables: U+AC00 - U+D7A3 (11,172 characters)
- Jongseong detection: `(codepoint - 0xAC00) % 28 != 0`

## IME (Input Method Editor) Support

### SDL2 Text Input Events
- `SDL_TEXTINPUT`: Final committed text
- `SDL_TEXTEDITING`: Composition preview (e.g., typing "안녕" shows partial characters)

### Implementation Notes
- `composing_text` member in MsgScroll stores IME preview
- Backspace clears composing_text before removing from input_buf
- Enter key commits composing_text to input_buf before finalizing

## Deployment

**Important**: After modifying translation files in `data/translations/korean/`, copy them to the Release folder:

```bash
cp data/translations/korean/*.txt msvc/Release/data/translations/korean/
```

The game reads data files from the executable's directory, not the source tree.
