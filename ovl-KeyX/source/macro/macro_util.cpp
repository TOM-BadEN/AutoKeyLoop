#include "macro_util.hpp"
#include "ini_helper.hpp"
#include <ultra.hpp>
#include <algorithm>
#include <strings.h>
#include <unordered_set>

namespace {
    constexpr const char* MACROS_DIR = "sdmc:/config/KeyX/macros";
    constexpr const char* GAME_CFG_DIR = "sdmc:/config/KeyX/GameConfig";
}

void MacroUtil::getGameCfgPath(u64 titleId, char* outPath, size_t size) {
    snprintf(outPath, size, "%s/%016lX.ini", GAME_CFG_DIR, titleId);
}

void MacroUtil::getMacroDirPath(u64 titleId, char* outPath, size_t size) {
    snprintf(outPath, size, "%s/%016lX", MACROS_DIR, titleId);
}

std::vector<std::string> MacroUtil::getGameDirs() {
    return ult::getSubdirectories(MACROS_DIR);
}

std::vector<MacroUtil::MacroEntry> MacroUtil::getMacroList(u64 titleId) {
    std::vector<MacroEntry> result;
    
    // 获取所有宏文件
    char pattern[96];
    snprintf(pattern, sizeof(pattern), "%s/%016lX/*.macro", MACROS_DIR, titleId);
    std::vector<std::string> allFiles = ult::getFilesListByWildcards(pattern);
    
    // 按名称排序
    std::sort(allFiles.begin(), allFiles.end(), [](const std::string& a, const std::string& b) {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    });
    
    // 读取快捷键配置
    char cfgPath[96];
    getGameCfgPath(titleId, cfgPath, sizeof(cfgPath));
    
    std::vector<MacroEntry> boundMacros;
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, cfgPath);
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string pathKey = "macro_path_" + std::to_string(idx);
        std::string comboKey = "macro_combo_" + std::to_string(idx);
        std::string macroPath = IniHelper::getString("MACRO", pathKey, "", cfgPath);
        if (macroPath.empty()) continue;
        u64 hotkey = static_cast<u64>(IniHelper::getInt("MACRO", comboKey, 0, cfgPath));
        if (hotkey == 0) continue;
        boundMacros.push_back({macroPath, hotkey});
    }
    
    // 按名称排序
    std::sort(boundMacros.begin(), boundMacros.end(), [](const MacroEntry& a, const MacroEntry& b) {
        std::string nameA = ult::getFileName(a.path);
        std::string nameB = ult::getFileName(b.path);
        return strcasecmp(nameA.c_str(), nameB.c_str()) < 0;
    });
    
    // 合并：有快捷键的在前，无快捷键的在后
    std::unordered_set<std::string> boundPaths;
    for (const auto& entry : boundMacros) {
        boundPaths.insert(entry.path);
        result.push_back(entry);
    }
    for (const auto& path : allFiles) {
        if (boundPaths.count(path)) continue;
        result.push_back({path, 0});
    }
    
    return result;
}

u64 MacroUtil::getHotkey(u64 titleId, const char* macroPath) {
    char cfgPath[96];
    getGameCfgPath(titleId, cfgPath, sizeof(cfgPath));
    
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, cfgPath);
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string pathKey = "macro_path_" + std::to_string(idx);
        std::string stored = IniHelper::getString("MACRO", pathKey, "", cfgPath);
        if (stored == macroPath) {
            std::string comboKey = "macro_combo_" + std::to_string(idx);
            return static_cast<u64>(IniHelper::getInt("MACRO", comboKey, 0, cfgPath));
        }
    }
    return 0;
}

void MacroUtil::setHotkey(u64 titleId, const char* macroPath, u64 hotkey) {
    char cfgPath[96];
    getGameCfgPath(titleId, cfgPath, sizeof(cfgPath));
    
    // 先删除已有的（如果存在）
    removeHotkey(titleId, macroPath);
    
    // 添加新条目
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, cfgPath) + 1;
    IniHelper::setInt("MACRO", "macroCount", macroCount, cfgPath);
    
    std::string pathKey = "macro_path_" + std::to_string(macroCount);
    std::string comboKey = "macro_combo_" + std::to_string(macroCount);
    IniHelper::setString("MACRO", pathKey, macroPath, cfgPath);
    IniHelper::setInt("MACRO", comboKey, static_cast<int>(hotkey), cfgPath);
}

bool MacroUtil::removeHotkey(u64 titleId, const char* macroPath) {
    char cfgPath[96];
    getGameCfgPath(titleId, cfgPath, sizeof(cfgPath));
    
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, cfgPath);
    int foundIdx = -1;
    
    // 查找要删除的条目
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string pathKey = "macro_path_" + std::to_string(idx);
        std::string stored = IniHelper::getString("MACRO", pathKey, "", cfgPath);
        if (stored == macroPath) {
            foundIdx = idx;
            break;
        }
    }
    
    if (foundIdx < 0) return false;
    
    // 将后面的条目前移
    for (int idx = foundIdx; idx < macroCount; ++idx) {
        std::string srcPath = "macro_path_" + std::to_string(idx + 1);
        std::string srcCombo = "macro_combo_" + std::to_string(idx + 1);
        std::string dstPath = "macro_path_" + std::to_string(idx);
        std::string dstCombo = "macro_combo_" + std::to_string(idx);
        
        std::string path = IniHelper::getString("MACRO", srcPath, "", cfgPath);
        int combo = IniHelper::getInt("MACRO", srcCombo, 0, cfgPath);
        IniHelper::setString("MACRO", dstPath, path, cfgPath);
        IniHelper::setInt("MACRO", dstCombo, combo, cfgPath);
    }
    
    // 删除最后一个条目并更新计数
    std::string lastPath = "macro_path_" + std::to_string(macroCount);
    std::string lastCombo = "macro_combo_" + std::to_string(macroCount);
    IniHelper::removeKey("MACRO", lastPath, cfgPath);
    IniHelper::removeKey("MACRO", lastCombo, cfgPath);
    IniHelper::setInt("MACRO", "macroCount", macroCount - 1, cfgPath);
    
    return true;
}

std::vector<u64> MacroUtil::getUsedHotkeys(u64 titleId) {
    std::vector<u64> result;
    char cfgPath[96];
    getGameCfgPath(titleId, cfgPath, sizeof(cfgPath));
    
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, cfgPath);
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string comboKey = "macro_combo_" + std::to_string(idx);
        u64 hotkey = static_cast<u64>(IniHelper::getInt("MACRO", comboKey, 0, cfgPath));
        if (hotkey != 0) result.push_back(hotkey);
    }
    return result;
}

u64 MacroUtil::getTitleIdFromPath(const char* macroPath) {
    // 路径格式: sdmc:/config/KeyX/macros/{titleId}/filename.macro
    const char* macrosDir = strstr(macroPath, "/macros/");
    if (!macrosDir) return 0;
    char tidStr[17];
    memcpy(tidStr, macrosDir + 8, 16);
    tidStr[16] = '\0';
    return strtoull(tidStr, nullptr, 16);
}

bool MacroUtil::updateMacroPath(u64 titleId, const char* oldPath, const char* newPath) {
    char cfgPath[96];
    getGameCfgPath(titleId, cfgPath, sizeof(cfgPath));
    
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, cfgPath);
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string pathKey = "macro_path_" + std::to_string(idx);
        std::string stored = IniHelper::getString("MACRO", pathKey, "", cfgPath);
        if (stored == oldPath) {
            IniHelper::setString("MACRO", pathKey, newPath, cfgPath);
            return true;
        }
    }
    return false;
}

std::string MacroUtil::getDisplayName(const std::string& macroPath) {
    u64 tid = getTitleIdFromPath(macroPath.c_str());
    std::string fileName = ult::getFileName(macroPath);
    
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", tid);
    std::string iniPath = std::string(MACROS_DIR) + "/" + tidStr + "/macroMetadata.ini";
    
    std::string name = IniHelper::getString(fileName, "name", "", iniPath);
    
    if (name.empty()) {
        auto dot = fileName.rfind('.');
        if (dot != std::string::npos) name = fileName.substr(0, dot);
        else name = fileName;
    }
    
    return name;
}

MacroUtil::MacroMetadata MacroUtil::getMetadata(const std::string& macroPath) {
    u64 tid = getTitleIdFromPath(macroPath.c_str());
    std::string fileName = ult::getFileName(macroPath);
    
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", tid);
    std::string iniPath = std::string(MACROS_DIR) + "/" + tidStr + "/macroMetadata.ini";
    
    MacroMetadata meta;
    meta.name = IniHelper::getString(fileName, "name", "", iniPath);
    meta.author = IniHelper::getString(fileName, "author", "", iniPath);
    meta.desc = IniHelper::getString(fileName, "desc", "", iniPath);
    
    return meta;
}

bool MacroUtil::deleteMacro(u64 titleId, const char* macroPath) {
    // 删除宏文件
    ult::deleteFileOrDirectory(macroPath);
    
    // 删除快捷键配置
    bool hadHotkey = removeHotkey(titleId, macroPath);
    
    // 删除元数据
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", titleId);
    std::string iniPath = std::string(MACROS_DIR) + "/" + tidStr + "/macroMetadata.ini";
    std::string fileName = ult::getFileName(macroPath);
    IniHelper::removeSection(fileName, iniPath);
    
    return hadHotkey;
}

bool MacroUtil::renameMacro(const char* oldPath, const char* newPath) {
    // 重命名文件
    rename(oldPath, newPath);
    
    u64 titleId = getTitleIdFromPath(oldPath);
    
    // 更新快捷键配置
    bool hadHotkey = updateMacroPath(titleId, oldPath, newPath);
    
    // 更新元数据 section
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", titleId);
    std::string iniPath = std::string(MACROS_DIR) + "/" + tidStr + "/macroMetadata.ini";
    std::string oldFileName = ult::getFileName(oldPath);
    std::string newFileName = ult::getFileName(newPath);
    
    std::string name = IniHelper::getString(oldFileName, "name", "", iniPath);
    std::string author = IniHelper::getString(oldFileName, "author", "", iniPath);
    std::string desc = IniHelper::getString(oldFileName, "desc", "", iniPath);
    
    if (!name.empty() || !author.empty() || !desc.empty()) {
        if (!name.empty()) IniHelper::setString(newFileName, "name", name, iniPath);
        if (!author.empty()) IniHelper::setString(newFileName, "author", author, iniPath);
        if (!desc.empty()) IniHelper::setString(newFileName, "desc", desc, iniPath);
        IniHelper::removeSection(oldFileName, iniPath);
    }
    
    return hadHotkey;
}
