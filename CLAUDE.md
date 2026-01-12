# Nuvie Project - Claude Code Guidelines

## Build Configuration

### IMPORTANT: Always use Release build
**절대로 Debug 빌드하지 말 것. 무조건 Release만 사용.**

### Build Command
```bash
cd "d:/proj/Ultima6/nuvie/msvc" && "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" nuvie.vcxproj -p:Configuration=Release -p:Platform=Win32 -nologo -verbosity:minimal 2>&1 | tail -10
```

### Build Output Location
- Release: `D:\proj\Ultima6\nuvie\msvc\Release\nuvie.exe`
- Data files: `D:\proj\Ultima6\nuvie\msvc\Release\data\`

## Project Structure

- Solution file: `d:\proj\Ultima6\nuvie\msvc\nuvie-msvc15.sln`
- Project file: `d:\proj\Ultima6\nuvie\msvc\nuvie.vcxproj`
- Main source: `d:\proj\Ultima6\nuvie\`
- Game data: `d:\proj\Ultima6\game_data\`
- Save files: `d:\proj\Ultima6\saves\`

## Key Source Files

- `MapWindow.cpp/h` - Map rendering and smooth scrolling
- `Actor.cpp/h` - Actor/NPC handling
- `VideoDialog.cpp/h` - Video settings menu
- `TileManager.cpp/h` - Tile management and lookups
- `KoreanTranslation.cpp/h` - Korean localization

## Current Features in Development

### Smooth Map Scrolling
- Toggle option in Video settings (`smooth_movement`)
- Variables in MapWindow.h: `scroll_offset_x/y`, `smooth_scrolling`, `smooth_move_duration`
- Implementation in `centerMapOnActor()` and `update_smooth_movement()`
- Edge tile rendering for seamless scrolling

## Notes

- Config file: `nuvie.cfg` (XML format)
- Korean translations: `data/translations/korean/`
- UI scale: 4x (1280x800 from 320x200)
- Map tile scale: configurable (1, 2, 3, or 4)
