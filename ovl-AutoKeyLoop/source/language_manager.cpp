#include "language_manager.hpp"
#include <tesla.hpp>

const std::string LANG_PATH = "/switch/.overlays/lang/AutoKeyLoop/";

// 初始化语言系统
void LanguageManager::initialize() {
    std::string langFileName = getSystemLanguageCode();
    
    // 如果系统语言是简体中文，不需要翻译（中文已经硬编码了）
    if (langFileName == "zh-cn.json") return;
    
    // 尝试加载系统语言对应的翻译文件
    std::string langFile = LANG_PATH + langFileName;
    if (ult::isFile(langFile)) {
        ult::parseLanguage(langFile);
        return;
    }
    
    // 如果没有对应语言，尝试加载英文
    std::string enFile = LANG_PATH + "en.json";
    if (ult::isFile(enFile)) {
        ult::parseLanguage(enFile);
        return;
    }
    
    // 否则不翻译，使用硬编码的中文
}

// 获取系统语言代码（返回文件名）
std::string LanguageManager::getSystemLanguageCode() {
    u64 languageCode;
    if (R_FAILED(setGetSystemLanguage(&languageCode))) {
        return "en.json";
    }
    
    SetLanguage setLanguage;
    if (R_FAILED(setMakeLanguage(languageCode, &setLanguage))) {
        return "en.json";
    }
    
    switch (setLanguage) {
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            return "zh-cn.json";
        case SetLanguage_ZHTW:
        case SetLanguage_ZHHANT:
            return "zh-tw.json";
        case SetLanguage_JA:
            return "ja.json";
        case SetLanguage_KO:
            return "ko.json";
        case SetLanguage_FR:
        case SetLanguage_FRCA:
            return "fr.json";
        case SetLanguage_DE:
            return "de.json";
        case SetLanguage_IT:
            return "it.json";
        case SetLanguage_ES:
        case SetLanguage_ES419:
            return "es.json";
        case SetLanguage_PT:
        case SetLanguage_PTBR:
            return "pt.json";
        case SetLanguage_RU:
            return "ru.json";
        case SetLanguage_NL:
            return "nl.json";
        default:
            return "en.json";
    }
}
