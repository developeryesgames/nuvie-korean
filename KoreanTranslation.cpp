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
#include <cctype>
#include <vector>

#include "nuvieDefs.h"

#include "Configuration.h"
#include "GUI.h"
#include "U6misc.h"
#include "Console.h"
#include "Game.h"
#include "Party.h"

#include "KoreanTranslation.h"

// Helper function to convert string to lowercase
static std::string toLowercase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

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
    // Check if Korean is enabled in config
    std::string korean_enabled_str;
    config->value("config/language/korean_enabled", korean_enabled_str, "yes");
    if(korean_enabled_str != "yes")
    {
        ConsoleAddInfo("KoreanTranslation: Disabled in config (korean_enabled=%s)", korean_enabled_str.c_str());
        enabled = false;
        return false;
    }

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

    // Load compound dialogue translations (multi-part dialogues)
    std::string compound_path;
    build_path(data_path, "compound_dialogues.txt", compound_path);
    if (loadDialogueTranslations(compound_path))
    {
        ConsoleAddInfo("KoreanTranslation: Loaded compound dialogue translations");
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
    item_names.clear();
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
        std::string english_text = line.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string korean_text = line.substr(pos2 + 1);

        uint16 index = (uint16)atoi(index_str.c_str());

        if (!korean_text.empty())
        {
            look_translations[index] = korean_text;
            // Also add to item_names for name-based lookup
            if (!english_text.empty())
            {
                item_names[english_text] = korean_text;
            }
            loaded++;
        }
    }

    file.close();
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

    // Skip empty or whitespace-only text
    if (english_text.empty())
        return english_text;

    bool only_whitespace = true;
    for (size_t i = 0; i < english_text.length(); i++)
    {
        char c = english_text[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
        {
            only_whitespace = false;
            break;
        }
    }
    if (only_whitespace)
        return english_text;

    // Skip text that's already Korean (UTF-8 Korean starts with 0xEA-0xED)
    // This prevents unnecessary lookups and "NOT FOUND" logs for already-translated text
    unsigned char first_byte = (unsigned char)english_text[0];
    if (first_byte >= 0xEA && first_byte <= 0xED)
    {
        return english_text;  // Already Korean
    }

    // Skip translation for avatar name (user input like "구명식량")
    // Avatar name is stored in party member 0 and should not be translated
    Game *game = Game::get_game();
    if (game)
    {
        Party *party = game->get_party();
        if (party && party->get_party_size() > 0)
        {
            const char *avatar_name = party->get_actor_name(0);
            if (avatar_name)
            {
                // Check if text starts with avatar name (handles "구명식량:\n>" case)
                std::string avatar_str(avatar_name);
                if (english_text == avatar_str ||
                    (english_text.length() > avatar_str.length() &&
                     english_text.substr(0, avatar_str.length()) == avatar_str))
                {
                    return english_text;
                }
            }
        }
    }

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

    // Check item names (from look_items_ko.txt)
    it = item_names.find(english_text);
    if (it != item_names.end())
    {
        return it->second;
    }

    // Debug: log when searching for item names
    if (english_text == "thread" || english_text == "cloth") {
    }

    // Also try case-insensitive match for item names
    std::string lower_text = toLowercase(english_text);
    for (std::map<std::string, std::string>::iterator iter = item_names.begin();
         iter != item_names.end(); ++iter)
    {
        if (toLowercase(iter->first) == lower_text)
        {
            return iter->second;
        }
    }

    // Try dialogue translations (for shop items stored as NPC dialogues)
    for (std::map<uint16, std::map<std::string, std::string> >::iterator npc_it = dialogue_translations.begin();
         npc_it != dialogue_translations.end(); ++npc_it)
    {
        std::map<std::string, std::string>::iterator it = npc_it->second.find(lower_text);
        if (it != npc_it->second.end())
            return it->second;
    }

    // Not found - return original text without logging (too noisy)
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

// 이야/야 - informal copula (비격식 서술격 조사)
std::string KoreanTranslation::getParticle_iya(const std::string &korean_word)
{
    return hasJongseong(korean_word) ? "이야" : "야";
}

// 이오/오 - formal archaic copula (격식 고어체 서술격 조사)
std::string KoreanTranslation::getParticle_io(const std::string &korean_word)
{
    return hasJongseong(korean_word) ? "이오" : "요";
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

    // Don't clear - allow multiple files to be loaded (dialogues_ko.txt + compound_dialogues.txt)
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
            // Store with lowercase key for case-insensitive matching
            std::string lower_key = toLowercase(english_text);
            dialogue_translations[npc_num][lower_key] = korean_text;

            // Also store normalized version for multiline matching
            std::string normalized = toLowercase(normalizeDialogueText(english_text));
            if (normalized != lower_key && !normalized.empty())
                dialogue_translations[npc_num][normalized] = korean_text;

            // Also store version with all quotes removed (for fuzzy matching)
            std::string no_quotes_key;
            for (size_t i = 0; i < english_text.size(); i++)
            {
                if (english_text[i] != '"' && english_text[i] != '\'')
                    no_quotes_key += english_text[i];
            }
            std::string lower_no_quotes = toLowercase(no_quotes_key);
            if (lower_no_quotes != lower_key && !lower_no_quotes.empty())
                dialogue_translations[npc_num][lower_no_quotes] = korean_text;

            // Store FM Towns compatible key: @ removed AND spaces before punctuation removed (combined)
            std::string fmtowns_key = english_text;
            // Remove @ symbols
            size_t at_pos;
            while ((at_pos = fmtowns_key.find('@')) != std::string::npos)
                fmtowns_key.erase(at_pos, 1);
            // Remove spaces before punctuation
            size_t sp_pos;
            while ((sp_pos = fmtowns_key.find(" ?")) != std::string::npos)
                fmtowns_key.erase(sp_pos, 1);
            while ((sp_pos = fmtowns_key.find(" !")) != std::string::npos)
                fmtowns_key.erase(sp_pos, 1);
            while ((sp_pos = fmtowns_key.find(" .")) != std::string::npos)
                fmtowns_key.erase(sp_pos, 1);
            std::string lower_fmtowns = toLowercase(fmtowns_key);
            if (lower_fmtowns != lower_key && !lower_fmtowns.empty())
                dialogue_translations[npc_num][lower_fmtowns] = korean_text;

            // Store key with all spaces AND quotes removed (handles spacing differences)
            std::string no_spaces_key;
            for (size_t i = 0; i < lower_no_quotes.size(); i++)
            {
                if (lower_no_quotes[i] != ' ')
                    no_spaces_key += lower_no_quotes[i];
            }
            if (!no_spaces_key.empty() && no_spaces_key != lower_no_quotes)
                dialogue_translations[npc_num][no_spaces_key] = korean_text;

            // If this contains compound text with \n*\n or *, also register individual parts
            // This allows both combined and split forms to work
            if (english_text.find("\n*\n") != std::string::npos ||
                english_text.find("\"*\"") != std::string::npos ||
                english_text.find("\n\n*") != std::string::npos)
            {
                // Split by various * patterns and register each part
                std::string eng_remaining = english_text;
                std::string kor_remaining = korean_text;

                // Try to split both english and korean the same way
                std::vector<std::string> eng_parts, kor_parts;

                // Split english by \n*\n or "*" patterns
                size_t split_pos;
                while ((split_pos = eng_remaining.find("\"\n*\n\"")) != std::string::npos ||
                       (split_pos = eng_remaining.find("\"*\"")) != std::string::npos ||
                       (split_pos = eng_remaining.find("\"\n\n*\"")) != std::string::npos)
                {
                    std::string part = eng_remaining.substr(0, split_pos + 1); // include closing quote
                    eng_parts.push_back(part);
                    // Find next quote
                    size_t next_quote = eng_remaining.find('"', split_pos + 1);
                    if (next_quote != std::string::npos)
                        eng_remaining = eng_remaining.substr(next_quote);
                    else
                        break;
                }
                if (!eng_remaining.empty())
                    eng_parts.push_back(eng_remaining);

                // Split korean the same way
                while ((split_pos = kor_remaining.find("\"\n*\n\"")) != std::string::npos ||
                       (split_pos = kor_remaining.find("\"*\"")) != std::string::npos ||
                       (split_pos = kor_remaining.find("\"\n\n*\"")) != std::string::npos)
                {
                    std::string part = kor_remaining.substr(0, split_pos + 1);
                    kor_parts.push_back(part);
                    size_t next_quote = kor_remaining.find('"', split_pos + 1);
                    if (next_quote != std::string::npos)
                        kor_remaining = kor_remaining.substr(next_quote);
                    else
                        break;
                }
                if (!kor_remaining.empty())
                    kor_parts.push_back(kor_remaining);

                // Register individual parts if counts match
                if (eng_parts.size() == kor_parts.size() && eng_parts.size() > 1)
                {
                    for (size_t i = 0; i < eng_parts.size(); i++)
                    {
                        std::string eng_part = eng_parts[i];
                        std::string kor_part = kor_parts[i];
                        // Trim
                        while (!eng_part.empty() && (eng_part[0] == ' ' || eng_part[0] == '\n'))
                            eng_part.erase(0, 1);
                        while (!eng_part.empty() && (eng_part.back() == ' ' || eng_part.back() == '\n' || eng_part.back() == '*'))
                            eng_part.pop_back();
                        if (!eng_part.empty())
                        {
                            std::string part_key = toLowercase(eng_part);
                            dialogue_translations[npc_num][part_key] = kor_part;
                        }
                    }
                }
            }

            loaded++;
        }
    }

    file.close();

    // Debug: list which NPCs have translations
    for (std::map<uint16, std::map<std::string, std::string> >::iterator it = dialogue_translations.begin();
         it != dialogue_translations.end(); ++it)
    {
    }

    return loaded > 0;
}

// Helper function to look up a single dialogue line
std::string KoreanTranslation::lookupSingleDialogue(uint16 npc_num, const std::string &text)
{
    std::string trimmed = text;

    // Remove FM Towns speech tags (~P and ~L followed by numbers)
    // These are in the format ~P123 or ~L123
    size_t tag_pos;
    while ((tag_pos = trimmed.find("~P")) != std::string::npos)
    {
        size_t end_pos = tag_pos + 2;
        while (end_pos < trimmed.length() && trimmed[end_pos] >= '0' && trimmed[end_pos] <= '9')
            end_pos++;
        trimmed.erase(tag_pos, end_pos - tag_pos);
    }
    while ((tag_pos = trimmed.find("~L")) != std::string::npos)
    {
        size_t end_pos = tag_pos + 2;
        while (end_pos < trimmed.length() && trimmed[end_pos] >= '0' && trimmed[end_pos] <= '9')
            end_pos++;
        trimmed.erase(tag_pos, end_pos - tag_pos);
    }


    // Remove script item tags like +33Book+ (format: +number+text+)
    // These are item references added by the game script
    while ((tag_pos = trimmed.find('+')) != std::string::npos)
    {
        // Check if it starts with a number (like +33Book+)
        if (tag_pos + 1 < trimmed.length() && trimmed[tag_pos + 1] >= '0' && trimmed[tag_pos + 1] <= '9')
        {
            // Find the number part
            size_t num_end = tag_pos + 1;
            while (num_end < trimmed.length() && trimmed[num_end] >= '0' && trimmed[num_end] <= '9')
                num_end++;
            // Find the text part and closing +
            size_t close_pos = trimmed.find('+', num_end);
            if (close_pos != std::string::npos)
            {
                trimmed.erase(tag_pos, close_pos - tag_pos + 1);
                continue;
            }
        }
        break; // Not a valid tag pattern, stop
    }

    DEBUG(0, LEVEL_DEBUGGING, "KoreanTranslation::lookupSingleDialogue NPC %d after tag removal: [%s]\n", npc_num, trimmed.c_str());

    // Trim whitespace, { and trailing * (FM Towns inserts {\n before narrative text)
    while (!trimmed.empty() && (trimmed[0] == ' ' || trimmed[0] == '\n' || trimmed[0] == '\r' || trimmed[0] == '{'))
        trimmed.erase(0, 1);
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == '*' || trimmed.back() == '{'))
        trimmed.pop_back();
    // Note: Don't strip leading single quote - it may be part of the text (e.g., 'Abyss')

    if (trimmed.empty())
        return "";

    // Remove outer double quotes
    std::string no_quotes = trimmed;
    if (!no_quotes.empty() && no_quotes[0] == '"')
        no_quotes.erase(0, 1);
    if (!no_quotes.empty() && no_quotes.back() == '"')
        no_quotes.pop_back();

    // Create version with ALL quotes removed (for fuzzy matching)
    std::string no_all_quotes;
    for (size_t i = 0; i < trimmed.size(); i++)
    {
        if (trimmed[i] != '"' && trimmed[i] != '\'')
            no_all_quotes += trimmed[i];
    }

    // Create version with @ symbols removed (FM Towns uses @keyword markers)
    std::string no_at_signs = trimmed;
    size_t at_pos;
    while ((at_pos = no_at_signs.find('@')) != std::string::npos)
        no_at_signs.erase(at_pos, 1);

    // Also create no_quotes version without @ signs
    std::string no_quotes_no_at = no_quotes;
    while ((at_pos = no_quotes_no_at.find('@')) != std::string::npos)
        no_quotes_no_at.erase(at_pos, 1);

    std::map<uint16, std::map<std::string, std::string> >::iterator npc_it = dialogue_translations.find(npc_num);
    if (npc_it != dialogue_translations.end())
    {
        // Convert to lowercase for case-insensitive matching
        std::string lower_trimmed = toLowercase(trimmed);
        std::string lower_no_quotes = toLowercase(no_quotes);
        std::string lower_no_all_quotes = toLowercase(no_all_quotes);

        std::map<std::string, std::string>::iterator it = npc_it->second.find(lower_trimmed);
        if (it != npc_it->second.end())
            return it->second;

        it = npc_it->second.find(lower_no_quotes);
        if (it != npc_it->second.end())
            return it->second;

        std::string with_quotes = "\"" + lower_trimmed + "\"";
        it = npc_it->second.find(with_quotes);
        if (it != npc_it->second.end())
            return it->second;

        with_quotes = "\"" + lower_no_quotes + "\"";
        it = npc_it->second.find(with_quotes);
        if (it != npc_it->second.end())
            return it->second;

        // Try with trailing quote removed (for cases like text ending with "")
        if (!lower_trimmed.empty() && lower_trimmed.back() == '"')
        {
            std::string without_trailing_quote = lower_trimmed.substr(0, lower_trimmed.length() - 1);
            it = npc_it->second.find(without_trailing_quote);
            if (it != npc_it->second.end())
                return it->second;
        }

        // Try with ALL quotes removed (fuzzy match for 'Path' vs Path etc)
        it = npc_it->second.find(lower_no_all_quotes);
        if (it != npc_it->second.end())
            return it->second;

        // Try with @ symbols removed (FM Towns uses @keyword markers that DOS doesn't have)
        std::string lower_no_at = toLowercase(no_at_signs);
        it = npc_it->second.find(lower_no_at);
        if (it != npc_it->second.end())
            return it->second;

        std::string lower_no_quotes_no_at = toLowercase(no_quotes_no_at);
        it = npc_it->second.find(lower_no_quotes_no_at);
        if (it != npc_it->second.end())
            return it->second;

        // Try with quotes around the @ removed version
        with_quotes = "\"" + lower_no_quotes_no_at + "\"";
        it = npc_it->second.find(with_quotes);
        if (it != npc_it->second.end())
            return it->second;

        // Try with all spaces removed (handles DOS vs FM Towns spacing differences)
        std::string no_spaces;
        for (size_t i = 0; i < lower_no_quotes_no_at.size(); i++)
        {
            if (lower_no_quotes_no_at[i] != ' ')
                no_spaces += lower_no_quotes_no_at[i];
        }
        if (!no_spaces.empty())
        {
            it = npc_it->second.find(no_spaces);
            if (it != npc_it->second.end())
                return it->second;
        }

        // Try normalized version for multiline texts
        std::string normalized = toLowercase(normalizeDialogueText(trimmed));
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
    // Remove leading whitespace, newlines, and * (but NOT single quotes - they may be part of text like 'Path')
    while (!trimmed.empty() && (trimmed[0] == ' ' || trimmed[0] == '\n' || trimmed[0] == '\r' || trimmed[0] == '*'))
        trimmed.erase(0, 1);
    // Special case: if text starts with '' (double single-quote), remove one (orphan from previous line)
    if (trimmed.length() >= 2 && trimmed[0] == '\'' && trimmed[1] == '\'')
        trimmed.erase(0, 1);
    // Remove trailing whitespace, newlines, *, and stray quotes (orphan quotes from dialogue)
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == '*' || trimmed.back() == '\'' || trimmed.back() == '"'))
        trimmed.pop_back();

    // Try direct lookup first (for complete text)
    std::string result = lookupSingleDialogue(npc_num, trimmed);
    if (!result.empty())
        return result;

    // Check if this is a compound dialogue with multiple parts separated by *
    // Try multiple patterns: "text1"*"text2", "text1"\n*\n"text2", or narrative text.*\n\ntext2.*

    // Pattern 1: Quoted dialogue "text1"*"text2"
    size_t star_pos = trimmed.find("\"*\"");
    if (star_pos == std::string::npos)
        star_pos = trimmed.find("\"\n*\n\"");
    if (star_pos == std::string::npos)
        star_pos = trimmed.find("\" *\n\"");
    if (star_pos == std::string::npos)
        star_pos = trimmed.find("\"\n* \"");

    if (star_pos != std::string::npos)
    {
        // Found compound quoted dialogue - split and translate each part
        std::string part1 = trimmed.substr(0, star_pos + 1); // Include closing quote
        size_t part2_start = trimmed.find('"', star_pos + 1);
        if (part2_start != std::string::npos)
        {
            std::string part2 = trimmed.substr(part2_start);

            std::string trans1 = lookupSingleDialogue(npc_num, part1);
            std::string trans2 = lookupSingleDialogue(npc_num, part2);

            if (!trans1.empty() && !trans2.empty())
                return trans1 + "\n*\n" + trans2;
            if (!trans1.empty())
                return trans1 + "\n*\n" + part2;
            if (!trans2.empty())
                return part1 + "\n*\n" + trans2;
        }
    }

    // Pattern 2: Narrative text with separators like .\n\n* or *\n\n or .\n*\n
    // Examples: "She dances.*\n\nIt's beautiful." or "Text.\n\n*More text." or "'Rogue'.\n*\nThe"
    if (trimmed.find("*\n") != std::string::npos || trimmed.find(".*") != std::string::npos ||
        trimmed.find("\n\n*") != std::string::npos || trimmed.find("\n*") != std::string::npos ||
        trimmed.find(".\n*\n") != std::string::npos)
    {
        std::vector<std::string> parts;
        std::string remaining = trimmed;

        // Split by various patterns: .\n\n*, *\n\n, .\n*, *\n
        while (!remaining.empty())
        {
            // Try multiple separator patterns in order of specificity
            size_t sep_pos = std::string::npos;
            size_t sep_len = 0;
            size_t part_end_offset = 0; // How much to include in part before separator

            // Pattern: .\n\n* (sentence end, then continuation)
            size_t pos = remaining.find(".\n\n*");
            if (pos != std::string::npos && (sep_pos == std::string::npos || pos < sep_pos))
            {
                sep_pos = pos;
                sep_len = 4; // .\n\n*
                part_end_offset = 1; // Include the .
            }

            // Pattern: *\n\n (continuation marker at end)
            pos = remaining.find("*\n\n");
            if (pos != std::string::npos && (sep_pos == std::string::npos || pos < sep_pos))
            {
                sep_pos = pos;
                sep_len = 3;
                part_end_offset = 0;
            }

            // Pattern: .\n*\n (sentence end, newline, star, newline)
            pos = remaining.find(".\n*\n");
            if (pos != std::string::npos && (sep_pos == std::string::npos || pos < sep_pos))
            {
                sep_pos = pos;
                sep_len = 4; // .\n*\n
                part_end_offset = 1; // Include the .
            }

            // Pattern: .\n*
            pos = remaining.find(".\n*");
            if (pos != std::string::npos && (sep_pos == std::string::npos || pos < sep_pos))
            {
                sep_pos = pos;
                sep_len = 3;
                part_end_offset = 1;
            }

            // Pattern: *\n
            pos = remaining.find("*\n");
            if (pos != std::string::npos && (sep_pos == std::string::npos || pos < sep_pos))
            {
                sep_pos = pos;
                sep_len = 2;
                part_end_offset = 0;
            }

            if (sep_pos != std::string::npos)
            {
                std::string part = remaining.substr(0, sep_pos + part_end_offset);
                // Remove trailing * if present
                while (!part.empty() && part.back() == '*')
                    part.pop_back();
                if (!part.empty())
                    parts.push_back(part);
                remaining = remaining.substr(sep_pos + sep_len);
                // Remove leading whitespace and * from next part
                while (!remaining.empty() && (remaining[0] == ' ' || remaining[0] == '\n' || remaining[0] == '*'))
                    remaining.erase(0, 1);
            }
            else
            {
                // No more separators - add remaining as last part
                // Remove trailing * if present
                while (!remaining.empty() && remaining.back() == '*')
                    remaining.pop_back();
                if (!remaining.empty())
                    parts.push_back(remaining);
                break;
            }
        }

        if (parts.size() > 1)
        {
            std::string combined_result;
            bool any_translated = false;

            for (size_t i = 0; i < parts.size(); i++)
            {
                std::string trans = lookupSingleDialogue(npc_num, parts[i]);
                if (!trans.empty())
                {
                    any_translated = true;
                    if (!combined_result.empty())
                        combined_result += "*\n\n";
                    combined_result += trans;
                }
                else
                {
                    if (!combined_result.empty())
                        combined_result += "*\n\n";
                    combined_result += parts[i];
                }
            }

            if (any_translated)
                return combined_result;
        }
    }

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
