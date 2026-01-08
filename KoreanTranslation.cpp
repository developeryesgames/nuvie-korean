/*
 *  KoreanTranslation.cpp
 *  Nuvie - Korean Localization
 *
 *  Manages loading and lookup of Korean translations for game text.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <fstream>
#include <sstream>
#include <algorithm>

#include "nuvieDefs.h"
#include "Configuration.h"
#include "GUI.h"
#include "U6misc.h"
#include "Console.h"
#include "KoreanTranslation.h"

KoreanTranslation::KoreanTranslation(Configuration *cfg)
{
    config = cfg;
    enabled = false;
}

KoreanTranslation::~KoreanTranslation()
{
}

bool KoreanTranslation::init()
{
    // Get data path
    std::string path;
    data_path = GUI::get_gui()->get_data_dir();

    ConsoleAddInfo("KoreanTranslation::init() called, data_path='%s'", data_path.c_str());

    build_path(data_path, "translations", path);
    build_path(path, "korean", data_path);

    ConsoleAddInfo("KoreanTranslation: Looking for files in: %s", data_path.c_str());

    // Try to load translation files
    std::string look_path;
    build_path(data_path, "look_items_ko.txt", look_path);

    ConsoleAddInfo("KoreanTranslation: Trying to load: %s", look_path.c_str());

    if (loadLookTranslations(look_path))
    {
        enabled = true;
        ConsoleAddInfo("KoreanTranslation: Loaded look translations! enabled=true");
    }
    else
    {
        ConsoleAddInfo("KoreanTranslation: Failed to load look translations");
    }

    // Load UI translations
    std::string ui_path;
    build_path(data_path, "ui_texts_ko.txt", ui_path);
    loadUITranslations(ui_path);

    if (enabled)
    {
        ConsoleAddInfo("KoreanTranslation: Korean translation system initialized - ENABLED");
    }
    else
    {
        ConsoleAddInfo("KoreanTranslation: No translation files found - DISABLED");
    }

    return enabled;
}

bool KoreanTranslation::loadLookTranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        DEBUG(0, LEVEL_WARNING, "KoreanTranslation: Cannot open %s\n", filename.c_str());
        return false;
    }

    look_translations.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
            continue;

        // Parse: INDEX|ENGLISH|KOREAN
        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos)
            continue;

        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;

        std::string index_str = line.substr(0, pos1);
        // Skip English text (we use index for lookup)
        std::string korean_text = line.substr(pos2 + 1);

        uint16 index = (uint16)atoi(index_str.c_str());

        if (!korean_text.empty())
        {
            look_translations[index] = korean_text;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d look translations\n", loaded);
    return loaded > 0;
}

bool KoreanTranslation::loadNPCTranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        return false;
    }

    npc_names.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // Format: ENGLISH_NAME|KOREAN_NAME
        size_t pos = line.find('|');
        if (pos == std::string::npos)
            continue;

        std::string english = line.substr(0, pos);
        std::string korean = line.substr(pos + 1);

        if (!english.empty() && !korean.empty())
        {
            npc_names[english] = korean;
            loaded++;
        }
    }

    file.close();
    return loaded > 0;
}

bool KoreanTranslation::loadUITranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        return false;
    }

    ui_texts.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // Format: KEY|KOREAN_TEXT
        size_t pos = line.find('|');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string korean = line.substr(pos + 1);

        if (!key.empty() && !korean.empty())
        {
            ui_texts[key] = korean;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d UI translations\n", loaded);
    return loaded > 0;
}

std::string KoreanTranslation::getLookText(uint16 item_index)
{
    if (!enabled)
        return "";

    std::map<uint16, std::string>::iterator it = look_translations.find(item_index);
    if (it != look_translations.end())
    {
        return it->second;
    }
    return "";
}

std::string KoreanTranslation::getNPCName(const std::string &english_name)
{
    if (!enabled)
        return english_name;

    std::map<std::string, std::string>::iterator it = npc_names.find(english_name);
    if (it != npc_names.end())
    {
        return it->second;
    }
    return english_name;
}

std::string KoreanTranslation::getNPCDialogue(uint16 npc_num, const std::string &keyword)
{
    if (!enabled)
        return "";

    std::map<uint16, std::map<std::string, std::string>>::iterator npc_it = npc_dialogues.find(npc_num);
    if (npc_it != npc_dialogues.end())
    {
        std::map<std::string, std::string>::iterator kw_it = npc_it->second.find(keyword);
        if (kw_it != npc_it->second.end())
        {
            return kw_it->second;
        }
    }
    return "";
}

std::string KoreanTranslation::getUIText(const std::string &key)
{
    if (!enabled)
        return "";

    std::map<std::string, std::string>::iterator it = ui_texts.find(key);
    if (it != ui_texts.end())
    {
        return it->second;
    }
    return "";
}

std::string KoreanTranslation::translate(const std::string &english_text)
{
    if (!enabled)
        return english_text;

    // First check UI texts
    std::map<std::string, std::string>::iterator it = ui_texts.find(english_text);
    if (it != ui_texts.end())
    {
        return it->second;
    }

    // Check NPC names
    it = npc_names.find(english_text);
    if (it != npc_names.end())
    {
        return it->second;
    }

    return english_text;
}

bool KoreanTranslation::hasLookTranslation(uint16 item_index)
{
    return look_translations.find(item_index) != look_translations.end();
}

bool KoreanTranslation::hasNPCName(const std::string &english_name)
{
    return npc_names.find(english_name) != npc_names.end();
}

// Check if the last character of a Korean string has a final consonant (받침/종성)
// Korean Hangul syllables are in Unicode range 0xAC00-0xD7A3
// Formula: (codepoint - 0xAC00) % 28 != 0 means it has jongseong
bool KoreanTranslation::hasJongseong(const std::string &korean_word)
{
    if (korean_word.empty())
        return false;

    // Get the last UTF-8 character
    const char *p = korean_word.c_str();
    const char *last_char = p;

    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x80) {
            last_char = p;
            p++;
        } else if ((c & 0xE0) == 0xC0) {
            last_char = p;
            p += 2;
        } else if ((c & 0xF0) == 0xE0) {
            last_char = p;
            p += 3;
        } else if ((c & 0xF8) == 0xF0) {
            last_char = p;
            p += 4;
        } else {
            p++;
        }
    }

    // Decode the last character
    unsigned char c = (unsigned char)*last_char;
    uint32 codepoint = 0;

    if (c < 0x80) {
        // ASCII - check if it's a number (numbers are treated as having jongseong based on pronunciation)
        // 0,1,3,6,7,8 have jongseong; 2,4,5,9 don't
        if (c >= '0' && c <= '9') {
            return (c == '0' || c == '1' || c == '3' || c == '6' || c == '7' || c == '8');
        }
        return false;  // Other ASCII has no jongseong
    } else if ((c & 0xE0) == 0xC0) {
        codepoint = c & 0x1F;
        codepoint = (codepoint << 6) | (((unsigned char)last_char[1]) & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        codepoint = c & 0x0F;
        codepoint = (codepoint << 6) | (((unsigned char)last_char[1]) & 0x3F);
        codepoint = (codepoint << 6) | (((unsigned char)last_char[2]) & 0x3F);
    }

    // Check if it's a Korean Hangul syllable
    if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) {
        // Has jongseong if (codepoint - 0xAC00) % 28 != 0
        return ((codepoint - 0xAC00) % 28) != 0;
    }

    return false;  // Non-Korean character
}

// 을/를 - object marker (목적격 조사)
std::string KoreanTranslation::getParticle_eulreul(const std::string &korean_word)
{
    return hasJongseong(korean_word) ? "을" : "를";
}

// 이/가 - subject marker (주격 조사)
std::string KoreanTranslation::getParticle_iga(const std::string &korean_word)
{
    return hasJongseong(korean_word) ? "이" : "가";
}

// 은/는 - topic marker (보조사)
std::string KoreanTranslation::getParticle_eunneun(const std::string &korean_word)
{
    return hasJongseong(korean_word) ? "은" : "는";
}

// 와/과 - and/with (접속 조사)
std::string KoreanTranslation::getParticle_wagwa(const std::string &korean_word)
{
    return hasJongseong(korean_word) ? "과" : "와";
}
