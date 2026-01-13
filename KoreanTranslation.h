/*
 *  KoreanTranslation.h
 *  Nuvie - Korean Localization
 *
 *  Manages loading and lookup of Korean translations for game text.
 *  Allows playing the game with Korean text without modifying original game files.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __KoreanTranslation_h__
#define __KoreanTranslation_h__

#include <string>
#include <map>

class Configuration;

class KoreanTranslation
{
private:
    Configuration *config;
    bool enabled;

    // Translation maps for different text types
    std::map<uint16, std::string> look_translations;    // Item descriptions by index
    std::map<std::string, std::string> item_names;      // Item names (English -> Korean)
    std::map<std::string, std::string> npc_names;       // NPC names
    std::map<std::string, std::string> keywords;        // Conversation keywords

    // NPC dialogue translations: npc_num -> (keyword -> response)
    std::map<uint16, std::map<std::string, std::string>> npc_dialogues;

    // UI text translations
    std::map<std::string, std::string> ui_texts;

    // Book/sign/scroll text translations by book_num
    std::map<uint16, std::string> book_translations;

    // Spell name translations by spell_num
    std::map<uint16, std::string> spell_translations;

    // Dialogue translations: npc_num -> (english_text -> korean_text)
    std::map<uint16, std::map<std::string, std::string>> dialogue_translations;

    std::string data_path;  // Path to translation data files

    // Helper for looking up single dialogue line
    std::string lookupSingleDialogue(uint16 npc_num, const std::string &text);

public:
    KoreanTranslation(Configuration *cfg);
    ~KoreanTranslation();

    bool init();

    // Enable/disable Korean translation
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() { return enabled; }

    // Load translation files
    bool loadLookTranslations(const std::string &filename);
    bool loadNPCTranslations(const std::string &filename);
    bool loadUITranslations(const std::string &filename);
    bool loadNPCDialogues(const std::string &filename);
    bool loadBookTranslations(const std::string &filename);
    bool loadDialogueTranslations(const std::string &filename);
    bool loadSpellTranslations(const std::string &filename);
    bool loadKeywordTranslations(const std::string &filename);

    // Get translated text
    std::string getLookText(uint16 item_index);
    std::string getNPCName(const std::string &english_name);
    std::string getNPCDialogue(uint16 npc_num, const std::string &keyword);
    std::string getUIText(const std::string &key);
    std::string getBookText(uint16 book_num);
    std::string getDialogueTranslation(uint16 npc_num, const std::string &english_text);
    std::string getSpellName(uint16 spell_num);

    // Translate a string if translation exists, otherwise return original
    std::string translate(const std::string &english_text);

    // Get English keyword from Korean input (for conversation system)
    std::string getEnglishKeyword(const std::string &korean_keyword);

    // Check if translation exists
    bool hasLookTranslation(uint16 item_index);
    bool hasNPCName(const std::string &english_name);

    // Korean particle/postposition helpers (을/를, 이/가, 은/는, 와/과, 으로/로)
    // Returns appropriate particle based on whether the last character has a final consonant (받침)
    static std::string getParticle_eulreul(const std::string &korean_word);  // 을/를 (object marker)
    static std::string getParticle_iga(const std::string &korean_word);      // 이/가 (subject marker)
    static std::string getParticle_eunneun(const std::string &korean_word);  // 은/는 (topic marker)
    static std::string getParticle_wagwa(const std::string &korean_word);    // 와/과 (and/with)
    static std::string getParticle_euroro(const std::string &korean_word);   // 으로/로 (instrumental)
    static std::string getParticle_iya(const std::string &korean_word);      // 이야/야 (informal copula)
    static std::string getParticle_io(const std::string &korean_word);       // 이오/오 (formal archaic copula)
    static bool hasJongseong(const std::string &korean_word);  // Check if last char has final consonant
    static bool hasJongseongRieul(const std::string &korean_word);  // Check if last char ends with ㄹ
};

#endif /* __KoreanTranslation_h__ */
