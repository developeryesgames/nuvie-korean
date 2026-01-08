#ifndef __KoreanFont_h__
#define __KoreanFont_h__

/*
 *  KoreanFont.h
 *  Nuvie - Korean Localization
 *
 *  Created for Ultima 6 Korean Translation Project
 *  Copyright (c) 2024. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "Font.h"
#include <string>
#include <map>

class Configuration;
class Screen;

// Korean font class that supports UTF-8 encoded Korean text
// Uses BMP sprite sheet with character mapping

class KoreanFont : public Font
{
private:
    SDL_Surface *font_surface;      // Font sprite sheet
    uint8 *char_widths;             // Width data for each character

    uint16 cell_width;              // Width of each cell in sprite sheet
    uint16 cell_height;             // Height of each cell
    uint16 chars_per_row;           // Characters per row in sprite sheet
    uint16 total_chars;             // Total characters in font

    // Character mapping: Unicode codepoint -> sprite index
    std::map<uint32, uint16> char_map;

    // Transparent color key
    uint8 transparent_r, transparent_g, transparent_b;

public:
    KoreanFont();
    ~KoreanFont();

    // Initialize with BMP font file and character map
    bool init(const std::string &bmp_path, const std::string &charmap_path);

    // Load character mapping from file
    bool loadCharMap(const std::string &charmap_path);

    // Get sprite index for a Unicode codepoint
    uint16 getCharIndex(uint32 codepoint);

    // UTF-8 string handling
    // scale: 1 = native 16px, 2 = 32px, 4 = 64px
    uint16 drawStringUTF8(Screen *screen, const char *str, uint16 x, uint16 y,
                          uint8 color, uint8 highlight_color, uint8 scale = 1);

    // Get next UTF-8 character from string and advance pointer
    static uint32 nextUTF8Char(const char *&str);

    // Calculate UTF-8 string width in pixels
    // scale: 1 = native 16px, 2 = 32px, 4 = 64px
    uint16 getStringWidthUTF8(const char *str, uint8 scale = 1);

    // Override base class methods
    uint16 getCharWidth(uint8 c) override;
    uint16 getCharHeight() override { return cell_height; } // Native 16px
    uint16 getCharHeightScaled(uint8 scale) { return cell_height * scale; }
    uint16 drawChar(Screen *screen, uint8 char_num, uint16 x, uint16 y, uint8 color) override;

    // Draw character by Unicode codepoint
    // scale: 1 = native, 2 = 2x, 4 = 4x
    uint16 drawCharUnicode(Screen *screen, uint32 codepoint, uint16 x, uint16 y, uint8 color, uint8 scale = 1);

    // Check if this font supports a character
    bool hasChar(uint32 codepoint);
};

#endif /* __KoreanFont_h__ */
