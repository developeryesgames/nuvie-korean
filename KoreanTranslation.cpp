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
#include <set>

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

    // Load NPC dialogue translations (keyword-based: desc, greeting, name, job, etc.)
    std::string npc_dialogues_path;
    build_path(data_path, "npc_dialogues_ko.txt", npc_dialogues_path);
    if (loadNPCDialogues(npc_dialogues_path))
    {
        ConsoleAddInfo("KoreanTranslation: Loaded NPC dialogues");
    }

    // Load dialogue translations (English -> Korean)
    std::string dialogues_path;
    build_path(data_path, "dialogues_ko.txt", dialogues_path);
    if (loadDialogueTranslations(dialogues_path))
    {
        ConsoleAddInfo("KoreanTranslation: Loaded dialogue translations");
    }

    // Load book/sign/scroll translations
    std::string books_path;
    build_path(data_path, "books_ko.txt", books_path);
    if (loadBookTranslations(books_path))
    {
        ConsoleAddInfo("KoreanTranslation: Loaded book translations");
    }

    // Load spell name translations
    std::string spells_path;
    build_path(data_path, "spells_ko.txt", spells_path);
    if (loadSpellTranslations(spells_path))
    {
        ConsoleAddInfo("KoreanTranslation: Loaded spell translations");
    }

    // Load keyword translations (Korean -> English for conversation input)
    std::string keywords_path;
    build_path(data_path, "keywords_ko.txt", keywords_path);
    if (loadKeywordTranslations(keywords_path))
    {
        ConsoleAddInfo("KoreanTranslation: Loaded keyword translations");
    }

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

        // Convert \n string to actual newlines
        size_t npos;
        while ((npos = key.find("\\n")) != std::string::npos)
            key.replace(npos, 2, "\n");
        while ((npos = korean.find("\\n")) != std::string::npos)
            korean.replace(npos, 2, "\n");

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

bool KoreanTranslation::loadNPCDialogues(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        DEBUG(0, LEVEL_WARNING, "KoreanTranslation: Cannot open %s\n", filename.c_str());
        return false;
    }

    npc_dialogues.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos)
            continue;

        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;

        std::string npc_str = line.substr(0, pos1);
        std::string keyword = line.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string korean_text = line.substr(pos2 + 1);

        uint16 npc_num = (uint16)atoi(npc_str.c_str());

        // Handle \n escape sequence
        size_t npos;
        while ((npos = korean_text.find("\\n")) != std::string::npos)
            korean_text.replace(npos, 2, "\n");

        if (!keyword.empty() && !korean_text.empty())
        {
            npc_dialogues[npc_num][keyword] = korean_text;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d NPC dialogue entries\n", loaded);
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

bool KoreanTranslation::hasJongseongRieul(const std::string &korean_word)
{
    if (korean_word.empty())
        return false;

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

    unsigned char c = (unsigned char)*last_char;
    uint32 codepoint = 0;

    if (c < 0x80) {
        return false;
    } else if ((c & 0xE0) == 0xC0) {
        codepoint = c & 0x1F;
        codepoint = (codepoint << 6) | (((unsigned char)last_char[1]) & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        codepoint = c & 0x0F;
        codepoint = (codepoint << 6) | (((unsigned char)last_char[1]) & 0x3F);
        codepoint = (codepoint << 6) | (((unsigned char)last_char[2]) & 0x3F);
    }

    if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) {
        return ((codepoint - 0xAC00) % 28) == 8;
    }

    return false;
}

std::string KoreanTranslation::getParticle_euroro(const std::string &korean_word)
{
    if (!hasJongseong(korean_word) || hasJongseongRieul(korean_word))
        return "\xEB\xA1\x9C";  // ro
    return "\xEC\x9C\xBC\xEB\xA1\x9C";  // euro
}

// Normalize text for comparison: collapse whitespace/newlines around * separators
static std::string normalizeDialogueText(const std::string &text)
{
    std::string result;
    result.reserve(text.length());

    for (size_t i = 0; i < text.length(); i++)
    {
        char c = text[i];
        if (c == '\r')
            continue; // Skip carriage returns
        if (c == '\n' || c == '*')
        {
            // Skip whitespace and * between quoted parts
            // But preserve the separator concept by collapsing to single *
            while (i + 1 < text.length() &&
                   (text[i+1] == '\n' || text[i+1] == '\r' || text[i+1] == '*' || text[i+1] == ' '))
                i++;
            // Only add separator if we're between content
            if (!result.empty() && result.back() != '*' && i + 1 < text.length())
                result += '*';
        }
        else
        {
            result += c;
        }
    }

    // Trim leading/trailing whitespace and *
    while (!result.empty() && (result[0] == ' ' || result[0] == '*'))
        result.erase(0, 1);
    while (!result.empty() && (result.back() == ' ' || result.back() == '*'))
        result.pop_back();

    return result;
}

bool KoreanTranslation::loadDialogueTranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        DEBUG(0, LEVEL_WARNING, "KoreanTranslation: Cannot open %s\n", filename.c_str());
        return false;
    }

    dialogue_translations.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // Format: npc|english|korean
        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos)
            continue;

        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;

        std::string npc_str = line.substr(0, pos1);
        std::string english_text = line.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string korean_text = line.substr(pos2 + 1);

        // Skip untranslated entries
        if (korean_text.empty())
            continue;

        // Convert \n string to actual newlines
        size_t npos;
        while ((npos = english_text.find("\\n")) != std::string::npos)
            english_text.replace(npos, 2, "\n");
        while ((npos = korean_text.find("\\n")) != std::string::npos)
            korean_text.replace(npos, 2, "\n");

        // Trim trailing whitespace, newlines, and * from english_text (same as lookupSingleDialogue)
        while (!english_text.empty() && (english_text.back() == ' ' || english_text.back() == '\n' || english_text.back() == '\r' || english_text.back() == '*'))
            english_text.pop_back();

        uint16 npc_num = (uint16)atoi(npc_str.c_str());

        if (!english_text.empty())
        {
            dialogue_translations[npc_num][english_text] = korean_text;
            // Also store normalized version for multiline matching
            std::string normalized = normalizeDialogueText(english_text);
            if (normalized != english_text && !normalized.empty())
                dialogue_translations[npc_num][normalized] = korean_text;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d dialogue translations\n", loaded);

    // Debug: list which NPCs have translations
    for (std::map<uint16, std::map<std::string, std::string> >::iterator it = dialogue_translations.begin();
         it != dialogue_translations.end(); ++it)
    {
        DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: NPC %d has %d dialogue entries\n", it->first, (int)it->second.size());
    }

    return loaded > 0;
}

// Helper function to look up a single dialogue line
std::string KoreanTranslation::lookupSingleDialogue(uint16 npc_num, const std::string &text)
{
    std::string trimmed = text;
    // Trim whitespace and trailing * (used as continuation marker in scripts)
    while (!trimmed.empty() && (trimmed[0] == ' ' || trimmed[0] == '\n' || trimmed[0] == '\r'))
        trimmed.erase(0, 1);
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == '*' || trimmed.back() == '\''))
        trimmed.pop_back();
    // Strip leading single quote if present
    if (!trimmed.empty() && trimmed[0] == '\'')
        trimmed.erase(0, 1);

    if (trimmed.empty())
        return "";

    std::string no_quotes = trimmed;
    if (!no_quotes.empty() && no_quotes[0] == '"')
        no_quotes.erase(0, 1);
    if (!no_quotes.empty() && no_quotes.back() == '"')
        no_quotes.pop_back();

    std::map<uint16, std::map<std::string, std::string> >::iterator npc_it = dialogue_translations.find(npc_num);
    if (npc_it != dialogue_translations.end())
    {
        std::map<std::string, std::string>::iterator it = npc_it->second.find(trimmed);
        if (it != npc_it->second.end())
            return it->second;

        it = npc_it->second.find(no_quotes);
        if (it != npc_it->second.end())
            return it->second;

        std::string with_quotes = "\"" + trimmed + "\"";
        it = npc_it->second.find(with_quotes);
        if (it != npc_it->second.end())
            return it->second;

        with_quotes = "\"" + no_quotes + "\"";
        it = npc_it->second.find(with_quotes);
        if (it != npc_it->second.end())
            return it->second;

        // Try normalized version for multiline texts
        std::string normalized = normalizeDialogueText(trimmed);
        it = npc_it->second.find(normalized);
        if (it != npc_it->second.end())
            return it->second;
    }

    return "";
}

std::string KoreanTranslation::getDialogueTranslation(uint16 npc_num, const std::string &english_text)
{
    if (!enabled)
        return "";

    // Check if this contains multiple dialogues separated by * or newlines
    // Pattern: "text1" * "text2" or "text1"\n"text2"
    std::string trimmed = english_text;
    while (!trimmed.empty() && (trimmed[0] == ' ' || trimmed[0] == '\n' || trimmed[0] == '\r'))
        trimmed.erase(0, 1);
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r'))
        trimmed.pop_back();

    // Check for compound dialogue pattern: contains "* or *" pattern (separator between quoted texts)
    bool is_compound = false;
    size_t star_pos = trimmed.find("\"*");
    if (star_pos == std::string::npos)
        star_pos = trimmed.find("\" *");
    if (star_pos == std::string::npos)
        star_pos = trimmed.find("\"\n*");
    if (star_pos == std::string::npos)
        star_pos = trimmed.find("\"\n\"");
    if (star_pos != std::string::npos && star_pos > 0)
        is_compound = true;

    if (is_compound)
    {
        // First try to match the entire compound text as-is
        std::string full_result = lookupSingleDialogue(npc_num, trimmed);
        if (!full_result.empty())
            return full_result;

        // Split and translate each part
        std::string result;
        std::string current;
        bool in_quote = false;

        for (size_t i = 0; i < trimmed.length(); i++)
        {
            char c = trimmed[i];
            if (c == '"')
            {
                current += c;
                if (in_quote)
                {
                    // End of quoted text - try to translate
                    std::string trans = lookupSingleDialogue(npc_num, current);
                    if (!trans.empty())
                    {
                        if (!result.empty()) result += "\n";
                        result += trans;
                    }
                    else
                    {
                        if (!result.empty()) result += "\n";
                        result += current;
                    }
                    current.clear();
                }
                in_quote = !in_quote;
            }
            else if (in_quote)
            {
                current += c;
            }
            // Skip * and whitespace between quotes
        }

        if (!result.empty())
            return result;
    }

    // Single dialogue - try direct lookup
    std::string result = lookupSingleDialogue(npc_num, trimmed);
    if (!result.empty())
        return result;

    // Log untranslated text
    static std::set<std::string> logged_texts;
    std::string log_key = std::to_string(npc_num) + "|" + trimmed;
    if (logged_texts.find(log_key) == logged_texts.end() && !trimmed.empty() && trimmed.length() > 3)
    {
        logged_texts.insert(log_key);
        std::string log_path;
        build_path(data_path, "untranslated_log.txt", log_path);
        std::ofstream log(log_path.c_str(), std::ios::app);
        if (log.is_open())
        {
            std::string escaped = trimmed;
            size_t pos;
            while ((pos = escaped.find('\n')) != std::string::npos)
                escaped.replace(pos, 1, "\\n");
            log << npc_num << "|" << escaped << "|<need translation>\n";
            log.close();
        }
    }

    return "";
}

bool KoreanTranslation::loadBookTranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        return false;

    book_translations.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        size_t pos = line.find('|');
        if (pos == std::string::npos)
            continue;

        std::string num_str = line.substr(0, pos);
        std::string korean_text = line.substr(pos + 1);

        uint16 book_num = (uint16)atoi(num_str.c_str());

        if (!korean_text.empty())
        {
            // Convert \n string to actual newlines
            size_t npos;
            while ((npos = korean_text.find("\\n")) != std::string::npos)
                korean_text.replace(npos, 2, "\n");

            book_translations[book_num] = korean_text;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d book translations\n", loaded);
    return loaded > 0;
}

std::string KoreanTranslation::getBookText(uint16 book_num)
{
    if (!enabled)
        return "";

    std::map<uint16, std::string>::iterator it = book_translations.find(book_num);
    if (it != book_translations.end())
        return it->second;
    return "";
}

bool KoreanTranslation::loadSpellTranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        return false;

    spell_translations.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // Format: SPELL_NUM|ENGLISH|KOREAN
        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos)
            continue;

        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;

        std::string num_str = line.substr(0, pos1);
        // English name is between pos1 and pos2, we skip it
        std::string korean_text = line.substr(pos2 + 1);

        uint16 spell_num = (uint16)atoi(num_str.c_str());

        if (!korean_text.empty())
        {
            spell_translations[spell_num] = korean_text;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d spell translations\n", loaded);
    return loaded > 0;
}

std::string KoreanTranslation::getSpellName(uint16 spell_num)
{
    if (!enabled)
        return "";

    std::map<uint16, std::string>::iterator it = spell_translations.find(spell_num);
    if (it != spell_translations.end())
        return it->second;
    return "";
}

bool KoreanTranslation::loadKeywordTranslations(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        DEBUG(0, LEVEL_WARNING, "KoreanTranslation: Cannot open %s\n", filename.c_str());
        return false;
    }

    keywords.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // Format: KOREAN|ENGLISH (first 4 chars)
        size_t pos = line.find('|');
        if (pos == std::string::npos)
            continue;

        std::string korean = line.substr(0, pos);
        std::string english = line.substr(pos + 1);

        // Trim whitespace
        while (!korean.empty() && (korean.back() == ' ' || korean.back() == '\r'))
            korean.pop_back();
        while (!english.empty() && (english.back() == ' ' || english.back() == '\r'))
            english.pop_back();

        if (!korean.empty() && !english.empty())
        {
            keywords[korean] = english;
            loaded++;
        }
    }

    file.close();
    DEBUG(0, LEVEL_INFORMATIONAL, "KoreanTranslation: Loaded %d keyword translations\n", loaded);
    return loaded > 0;
}

std::string KoreanTranslation::getEnglishKeyword(const std::string &korean_keyword)
{
    if (!enabled)
        return "";

    std::map<std::string, std::string>::iterator it = keywords.find(korean_keyword);
    if (it != keywords.end())
        return it->second;
    return "";
}
