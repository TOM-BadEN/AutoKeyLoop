#pragma once
#include <tsl_utils.hpp>

inline std::string i18n(const std::string& text) {
    auto it = ult::translationCache.find(text);
    return (it != ult::translationCache.end()) ? it->second : text;
}