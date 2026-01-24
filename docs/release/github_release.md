# Ultima VI Korean Localization Patch v1.5.2

## Nuvie Korean Translation - AI-Assisted Localization

A complete Korean localization for Ultima VI: The False Prophet, powered by Nuvie engine.

### v1.5.2 Changes

**Bug Fixes:**
- Fixed party followers not following through swamps and dangerous terrain (implemented original U6 danger tile handling with eager scoring)

**Translation:**
- Fixed Unlock Magic spell name: "마법 해제" → "잠금 해제" (was same as Dispel Magic, causing confusion)

### v1.5.1 Changes

**Bug Fixes:**
- Fixed NPC scheduler bug where NPCs keep sleeping while being watched
- Fixed game freeze during camping (duplicate party movement prevention)
- Fixed multi-tile corpse (cyclops, etc.) Z-order rendering bug
- Fixed crash when entering dungeons (TimedPartyMove memory management)
- Fixed party followers not following well in narrow dungeon corridors (implemented original U6 eager-based movement algorithm)

**Translation:**
- Improved some system messages

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

### Screenshots

[Add screenshots here]

### Credits

- **Translation**: AI-assisted (Claude) + human review
- **Engine**: Nuvie (GPL v2)
- **Original Game**: Origin Systems / EA

### Technical Notes

This project demonstrates AI-assisted game localization workflow:
- Dialogue extraction and analysis
- Context-aware translation with Korean particle system (을/를, 이/가, 은/는)
- Custom font rendering for Korean characters
- Keyword mapping system for Korean input

### License

- Nuvie engine modifications: GPL v2
- Translation files: Free to use for non-commercial purposes
- Original Ultima VI game files NOT included - you must own the original game

### Links

- [Nuvie Project](http://nuvie.sourceforge.net/)
- [Ultima VI on GOG](https://www.gog.com/game/ultima_6)

---

*This localization was created in approximately 40 hours using AI-assisted translation tools.*
