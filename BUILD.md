# Nuvie Korean Build Guide

## Requirements
- Visual Studio 2022 Community
- Windows 10/11
- vcpkg (x86-windows triplet)

## Build Steps

### Method 1: Visual Studio IDE (Recommended)
1. Open `D:\proj\Ultima6\nuvie\msvc\nuvie-msvc15.sln` in Visual Studio 2022
2. Select **Release** | **Win32** configuration
3. Build > Rebuild Solution (Ctrl+Shift+B)
4. Output: `D:\proj\Ultima6\nuvie\msvc\Release\nuvie.exe`

### Method 2: MSBuild Command Line
```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "D:\proj\Ultima6\nuvie\msvc\nuvie.vcxproj" /p:Configuration=Release /p:Platform=Win32 /t:Rebuild /v:minimal
```

## Important Notes
- Solution file: `nuvie-msvc15.sln` (not nuvie.sln)
- Platform: **Win32** (not x64)
- Configuration: **Release**

## Output Location
- Release exe: `D:\proj\Ultima6\nuvie\msvc\Release\nuvie.exe`

## Korean Localization Files
Required files in game data folder:
- `data/fonts/korean/korean_font_24x24.bmp`
- `data/fonts/korean/korean_font_24x24.dat`
- `data/fonts/korean/korean_font_32x32.bmp`
- `data/fonts/korean/korean_font_32x32.dat`
- `data/korean_translation.txt`

## nuvie.cfg Settings for Korean
```xml
<config>
  <video>
    <screen_width>1360</screen_width>
    <screen_height>768</screen_height>
    <game_width>1360</game_width>
    <game_height>768</game_height>
    <game_style>original+</game_style>
    <compact_ui>yes</compact_ui>
  </video>
  <general>
    <korean>yes</korean>
  </general>
</config>
```
