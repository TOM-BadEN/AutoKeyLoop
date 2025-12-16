#pragma once
#include <string>
#include <vector>
#include <switch.h>

// 游戏条目
struct StoreGameEntry {
    u64 id;              // 游戏TID
    int count;           // 宏数量
};

// 游戏列表结果
struct GameListResult {
    bool success = false;
    std::string error;
    std::vector<StoreGameEntry> games;
};

// 宏条目
struct StoreMacroEntry {
    std::string file;    // 文件名
    std::string name;    // 显示名（已根据语言选择）
    std::string desc;    // 说明（已根据语言选择）
    std::string author;  // 作者
};

// 宏列表结果
struct MacroListResult {
    bool success = false;
    std::string error;
    std::vector<StoreMacroEntry> macros;
};

class StoreData {
public:
    StoreData();
    ~StoreData();

    // 获取游戏列表（从远程 gamelist.json）
    GameListResult getGameList();

    // 获取指定游戏的宏列表（从远程 games/{gameId}/macrolist.json）
    MacroListResult getMacroList(const std::string& gameId);

    // 下载宏文件到本地
    bool downloadMacro(const std::string& gameId, const std::string& fileName, const std::string& localPath);

private:
    bool m_isSimplifiedChinese = false;
};
