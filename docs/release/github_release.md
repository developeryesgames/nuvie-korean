# Ultima VI Korean Localization Patch v1.5.6

## Nuvie Korean Translation - AI-Assisted Localization

A complete Korean localization for Ultima VI: The False Prophet, powered by Nuvie engine.

### v1.5.6 Changes

**Bug Fixes:**
- Fixed party view visible during ending sequence in compact UI mode

**Translation:**
- Fixed gargoyle statue dialogue spacing and awkward expressions
- Fixed gargoyle shrine names (Control/Passion/Diligence)

### v1.5.5 Changes

**Translation:**
- Fixed duplicate spell names:
  - Clone → 분신 (Replicate remains 복제)
  - Peer → 원시 (X-ray remains 투시)
  - Conjure → 강령 (Summon remains 소환)
- Fixed NPC 173 gargoyle healer translation

### v1.5.4 Changes

**Bug Fixes:**
- Fixed fountain not rendering properly

### v1.5.3 Changes

**Bug Fixes:**
- Fixed multi-tile objects (white pillars, etc.) Z-order rendering bug

**Translation:**
- Fixed missing "Found it" message when searching
- Fixed 3 ship merchant NPC dialogues
- Added missing translations due to whitespace differences

### v1.5.2 Changes

**Bug Fixes:**
- Fixed party followers not following through swamps and dangerous terrain

**Translation:**
- Fixed Unlock Magic spell name: "마법 해제" → "잠금 해제"

### v1.5.1 Changes

**Bug Fixes:**
- Fixed NPC scheduler bug where NPCs keep sleeping while being watched
- Fixed game freeze during camping
- Fixed multi-tile corpse Z-order rendering bug
- Fixed crash when entering dungeons
- Fixed party followers not following well in narrow dungeon corridors

### Features

- **Full Korean Translation**: 200+ NPCs, 7,700+ dialogue lines
- **Korean Font Support**: Custom 16x16 and 32x32 bitmap fonts
- **Korean Keyword Input**: Type Korean keywords in conversations (770+ mappings)
- **Translated Content**:
  - All NPC dialogues
  - Item/object descriptions (600+)
  - Magic spell names (121 spells)
  - Books, signs, scrolls (127 entries)
  - UI texts, combat messages, system messages
  - Intro and ending sequences

### Requirements

- **Ultima VI original game files** (legally obtained)
- Windows 10/11

### Installation

1. Download and extract the patch
2. Copy your original Ultima VI game files to the `ULTIMA6` folder
3. Run `nuvie.exe`
4. Korean mode is enabled by default

### Configuration

Edit `nuvie.cfg` to toggle Korean mode:
```xml
<config>
  <video>
    <korean_mode>true</korean_mode>
  </video>
</config>
```

### Credits

- **Translation**: AI-assisted (Claude) + human review
- **Engine**: Nuvie (GPL v2)
- **Original Game**: Origin Systems / EA

### License

- Nuvie engine modifications: GPL v2
- Translation files: Free to use for non-commercial purposes
- Original Ultima VI game files NOT included - you must own the original game

### Links

- [Nuvie Project](http://nuvie.sourceforge.net/)
- [Ultima VI on GOG](https://www.gog.com/game/ultima_6)

---

*This localization was created using AI-assisted translation tools.*
