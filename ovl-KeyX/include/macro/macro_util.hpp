#pragma once
#include <vector>
#include <string>
#include <switch.h>

// 宏工具类
class MacroUtil {
public:
    // 宏条目（路径+快捷键）
    struct MacroEntry {
        std::string path;
        u64 hotkey;
    };

    // 获取有脚本的游戏目录列表（titleId字符串）
    static std::vector<std::string> getGameDirs();
    
    // 获取指定游戏的所有宏列表（已排序：有快捷键的在前）
    static std::vector<MacroEntry> getMacroList(u64 titleId);
    
    // 获取指定宏文件的快捷键
    static u64 getHotkey(u64 titleId, const char* macroPath);
    
    // 设置指定宏文件的快捷键
    static void setHotkey(u64 titleId, const char* macroPath, u64 hotkey);
    
    // 删除指定宏文件的快捷键
    static bool removeHotkey(u64 titleId, const char* macroPath);
    
    // 获取指定游戏已使用的快捷键列表
    static std::vector<u64> getUsedHotkeys(u64 titleId);
    
    // 从宏文件路径提取 titleId
    static u64 getTitleIdFromPath(const char* macroPath);
    
    // 更新配置中的宏路径（重命名时使用）
    static bool updateMacroPath(u64 titleId, const char* oldPath, const char* newPath);
    
    // 获取宏的显示名称（优先从元数据读取，否则用文件名）
    static std::string getDisplayName(const std::string& macroPath);
    
    // 删除宏文件及相关数据（快捷键配置、元数据），返回是否成功删除了快捷键
    static bool deleteMacro(u64 titleId, const char* macroPath);
    
    // 重命名宏文件后更新配置和元数据，返回是否有快捷键需要重载
    static bool renameMacro(const char* oldPath, const char* newPath);

private:
    // 获取游戏配置文件路径
    static void getGameCfgPath(u64 titleId, char* outPath, size_t size);
    
    // 获取宏目录路径
    static void getMacroDirPath(u64 titleId, char* outPath, size_t size);
};
