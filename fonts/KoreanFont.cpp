/*
 *  KoreanFont.cpp
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

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>

#include "nuvieDefs.h"
#include "Screen.h"
#include "KoreanFont.h"
#include "NuvieIOFile.h"

KoreanFont::KoreanFont()
{
    font_surface = NULL;
    char_widths = NULL;
    cell_width = 32;   // 32x32 font cells
    cell_height = 32;
    chars_per_row = 64; // 64 chars per row
    total_chars = 0;

    // Magenta transparent color: RGB(255, 0, 255)
    transparent_r = 255;
    transparent_g = 0;
    transparent_b = 255;

    num_chars = 0;
    offset = 0;

    // Anti-aliasing disabled by default
    enable_antialiasing = false;
}

KoreanFont::~KoreanFont()
{
    if (font_surface)
    {
        SDL_FreeSurface(font_surface);
        font_surface = NULL;
    }

    if (char_widths)
    {
        free(char_widths);
        char_widths = NULL;
    }
}

bool KoreanFont::init(const std::string &bmp_path, const std::string &charmap_path)
{
    // Load BMP font sprite sheet
    font_surface = SDL_LoadBMP(bmp_path.c_str());
    if (!font_surface)
    {
        DEBUG(0, LEVEL_ERROR, "KoreanFont: Failed to load BMP: %s\n", bmp_path.c_str());
        return false;
    }

    // Set transparent color key
    SDL_SetColorKey(font_surface, SDL_TRUE,
                    SDL_MapRGB(font_surface->format, transparent_r, transparent_g, transparent_b));

    // Calculate grid dimensions
    cell_width = font_surface->w / chars_per_row;
    cell_height = cell_width;  // Square cells

    // Recalculate with proper cell height
    uint16 num_rows = font_surface->h / cell_height;
    total_chars = chars_per_row * num_rows;

    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanFont: Loaded %dx%d sprite sheet, cell %dx%d, %d chars\n",
          font_surface->w, font_surface->h, cell_width, cell_height, total_chars);

    // Load character width data (.dat file)
    std::string dat_path = bmp_path;
    size_t dot_pos = dat_path.rfind('.');
    if (dot_pos != std::string::npos)
    {
        dat_path = dat_path.substr(0, dot_pos) + ".dat";
    }
    else
    {
        dat_path += ".dat";
    }

    NuvieIOFileRead dat_file;
    if (dat_file.open(dat_path))
    {
        uint32 bytes_read;
        char_widths = dat_file.readBuf(total_chars, &bytes_read);
        dat_file.close();
        DEBUG(0, LEVEL_INFORMATIONAL, "KoreanFont: Loaded %d width values\n", bytes_read);
    }
    else
    {
        // Use fixed width if no .dat file
        char_widths = (uint8 *)malloc(total_chars);
        memset(char_widths, cell_width, total_chars);
        DEBUG(0, LEVEL_WARNING, "KoreanFont: No .dat file, using fixed width %d\n", cell_width);
    }

    // Load character mapping
    if (!loadCharMap(charmap_path))
    {
        DEBUG(0, LEVEL_WARNING, "KoreanFont: Failed to load charmap, using ASCII fallback\n");
        // Create default ASCII mapping (indices 0-127 map to ASCII 0-127)
        for (uint32 i = 0; i < 128 && i < total_chars; i++)
        {
            char_map[i] = (uint16)i;
        }
    }

    num_chars = total_chars;
    return true;
}

bool KoreanFont::loadCharMap(const std::string &charmap_path)
{
    std::ifstream file(charmap_path.c_str());
    if (!file.is_open())
    {
        return false;
    }

    char_map.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
            continue;

        // Parse: index<tab>hex_codepoint<tab>character
        std::istringstream iss(line);
        uint16 index;
        std::string hex_str;

        if (iss >> index >> hex_str)
        {
            uint32 codepoint = 0;
            sscanf(hex_str.c_str(), "%X", &codepoint);

            if (codepoint > 0 && index < total_chars)
            {
                char_map[codepoint] = index;
                loaded++;
            }
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanFont: Loaded %d character mappings\n", loaded);
    return loaded > 0;
}

uint16 KoreanFont::getCharIndex(uint32 codepoint)
{
    // Special case: space character - BMP index 0 should be empty
    if (codepoint == 0x20)
    {
        return 0;
    }

    std::map<uint32, uint16>::iterator it = char_map.find(codepoint);
    if (it != char_map.end())
    {
        // BMP and charmap should be 1:1 aligned
        return it->second;
    }

    // Fallback: try ASCII range directly
    // charmap index 0 = space (0x20), index 1 = '!' (0x21), etc.
    if (codepoint >= 32 && codepoint < 127)
    {
        return (uint16)(codepoint - 32);
    }

    // Return space for unknown
    return 0;
}

uint32 KoreanFont::nextUTF8Char(const char *&str)
{
    if (!str || !*str)
        return 0;

    uint32 codepoint = 0;
    unsigned char c = (unsigned char)*str++;

    if (c < 0x80)
    {
        // ASCII (0xxxxxxx)
        codepoint = c;
    }
    else if ((c & 0xE0) == 0xC0)
    {
        // 2-byte UTF-8 (110xxxxx 10xxxxxx)
        codepoint = c & 0x1F;
        if (*str)
            codepoint = (codepoint << 6) | ((*str++) & 0x3F);
    }
    else if ((c & 0xF0) == 0xE0)
    {
        // 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx) - Korean is here
        codepoint = c & 0x0F;
        if (*str)
            codepoint = (codepoint << 6) | (((unsigned char)*str++) & 0x3F);
        if (*str)
            codepoint = (codepoint << 6) | (((unsigned char)*str++) & 0x3F);
    }
    else if ((c & 0xF8) == 0xF0)
    {
        // 4-byte UTF-8 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        codepoint = c & 0x07;
        if (*str)
            codepoint = (codepoint << 6) | ((*str++) & 0x3F);
        if (*str)
            codepoint = (codepoint << 6) | ((*str++) & 0x3F);
        if (*str)
            codepoint = (codepoint << 6) | ((*str++) & 0x3F);
    }
    else
    {
        // Invalid UTF-8, skip byte
        codepoint = '?';
    }

    return codepoint;
}

uint16 KoreanFont::drawStringUTF8(Screen *screen, const char *str, uint16 x, uint16 y,
                                   uint8 color, uint8 highlight_color, uint8 scale)
{
    if (!str || !font_surface)
        return 0;

    uint16 start_x = x;
    bool highlight = false;
    const char *p = str;

    while (*p)
    {
        // Check for highlight marker
        if (*p == '@')
        {
            highlight = true;
            p++;
            continue;
        }

        uint32 codepoint = nextUTF8Char(p);
        if (codepoint == 0)
            break;

        // Turn off highlight for non-alpha characters
        if (highlight && (codepoint < 'A' || (codepoint > 'Z' && codepoint < 'a') || codepoint > 'z'))
        {
            // Keep highlight for Korean characters (Hangul range)
            if (codepoint < 0xAC00 || codepoint > 0xD7A3)
            {
                highlight = false;
            }
        }

        x += drawCharUnicode(screen, codepoint, x, y, highlight ? highlight_color : color, scale);
    }

    return x - start_x;
}

uint16 KoreanFont::getStringWidthUTF8(const char *str, uint8 scale)
{
    if (!str)
        return 0;

    uint16 width = 0;
    const char *p = str;

    while (*p)
    {
        if (*p == '@')
        {
            p++;
            continue;
        }

        uint32 codepoint = nextUTF8Char(p);
        if (codepoint == 0)
            break;

        uint16 index = getCharIndex(codepoint);
        uint16 char_width;
        if (index < total_chars && char_widths)
        {
            // Use width from .dat file
            char_width = char_widths[index];
        }
        else
        {
            // Default spacing: 75% for Korean (24px for 32px cell), 50% for ASCII
            if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) {
                char_width = (cell_width * 3) / 4;  // Korean: 24px for 32px cell
            } else if (codepoint >= 0x20 && codepoint < 0x7F) {
                char_width = cell_width / 2;  // ASCII: 16px for 32px cell
            } else {
                char_width = (cell_width * 3) / 4;
            }
        }
        // Add extra letter spacing (4 pixels) to match drawCharUnicode
        width += (char_width + 4) * scale;
    }

    return width;
}

uint16 KoreanFont::getCharWidth(uint8 c)
{
    uint16 index = getCharIndex((uint32)c);
    if (index < total_chars && char_widths)
    {
        return char_widths[index];  // Native width
    }
    return cell_width;
}

uint16 KoreanFont::drawChar(Screen *screen, uint8 char_num, uint16 x, uint16 y, uint8 color)
{
    return drawCharUnicode(screen, (uint32)char_num, x, y, color);
}

uint16 KoreanFont::drawCharUnicode(Screen *screen, uint32 codepoint, uint16 x, uint16 y, uint8 color, uint8 scale)
{
    if (!font_surface)
        return cell_width * scale;

    uint16 index = getCharIndex(codepoint);
    if (index >= total_chars)
        index = 0; // Fallback to first char (space)

    // Calculate source position in sprite sheet
    uint16 src_x = (index % chars_per_row) * cell_width;
    uint16 src_y = (index / chars_per_row) * cell_height;

    // Create buffer for the character (0xff = transparent)
    // Support up to 64x64 cells (for 48x48 font and potential larger fonts)
    unsigned char buf[64 * 64];
    memset(buf, 0xff, cell_width * cell_height);

    // Lock font surface to read pixels
    if (SDL_MUSTLOCK(font_surface))
        SDL_LockSurface(font_surface);

    // Read pixels from BMP and convert to game palette
    // BMP has white text (255,255,255) on magenta background (255,0,255)
    uint8 *src_pixels = (uint8 *)font_surface->pixels;
    int bpp = font_surface->format->BytesPerPixel;
    int pitch = font_surface->pitch;

    for (int py = 0; py < cell_height; py++)
    {
        for (int px = 0; px < cell_width; px++)
        {
            uint8 *pixel = src_pixels + (src_y + py) * pitch + (src_x + px) * bpp;

            uint8 r, g, b;
            if (bpp == 1)
            {
                // 8-bit indexed
                SDL_Color *colors = font_surface->format->palette->colors;
                SDL_Color c = colors[*pixel];
                r = c.r; g = c.g; b = c.b;
            }
            else if (bpp == 3 || bpp == 4)
            {
                // 24/32-bit
                uint32 pixel_val;
                if (bpp == 3)
                    pixel_val = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
                else
                    pixel_val = *(uint32 *)pixel;
                SDL_GetRGB(pixel_val, font_surface->format, &r, &g, &b);
            }
            else
            {
                r = g = b = 0;
            }

            // Background is magenta (241, 15, 196) - transparent
            // Text is brown/tan color
            // Magenta: high R, low G, high B (around 241, 15, 196)
            bool is_magenta = (r > 180 && g < 60 && b > 150);
            if (!is_magenta && (r > 50 || g > 50 || b > 50))
            {
                buf[py * cell_width + px] = color;
            }
            // else: leave as 0xff (transparent)
        }
    }

    if (SDL_MUSTLOCK(font_surface))
        SDL_UnlockSurface(font_surface);

    // Blit using appropriate scale function
    if (scale >= 4) {
        screen->blit4x(x, y, buf, 8, cell_width, cell_height, cell_width, true, NULL);
    } else if (scale == 3) {
        screen->blit3x(x, y, buf, 8, cell_width, cell_height, cell_width, true, NULL);
    } else if (scale >= 2) {
        screen->blit2x(x, y, buf, 8, cell_width, cell_height, cell_width, true);
    } else {
        screen->blit(x, y, buf, 8, cell_width, cell_height, cell_width, true);
    }

    // Return character width (scaled)
    // For 32x32 font, use 75% of cell width for Korean, half for ASCII
    uint16 width;
    if (char_widths && index < total_chars)
    {
        // Use width from .dat file
        width = char_widths[index];
    }
    else
    {
        // Default spacing: 75% for Korean (24px for 32px cell), 50% for ASCII
        if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) {
            width = (cell_width * 3) / 4;  // Korean: 24px for 32px cell
        } else if (codepoint >= 0x20 && codepoint < 0x7F) {
            width = cell_width / 2;  // ASCII: 16px for 32px cell
        } else {
            width = (cell_width * 3) / 4;
        }
    }
    // Add extra letter spacing (4 pixels)
    width += 4;
    return width * scale;
}

bool KoreanFont::hasChar(uint32 codepoint)
{
    return char_map.find(codepoint) != char_map.end();
}
