#include "game.hpp"
#include "language.hpp"
#include "ultra.hpp"

namespace {
    constexpr const char* WHITE_INI_PATH = "sdmc:/config/KeyX/white.ini";
}

std::unordered_set<u64> GameMonitor::s_whitelist;

// 获取当前运行程序的Title ID
u64 GameMonitor::getCurrentTitleId() {
    u64 pid = 0, tid = 0;
    
    // 1. 获取当前应用的进程ID
    if (R_FAILED(pmdmntGetApplicationProcessId(&pid)))
        return 0;  
    
    // 2. 根据进程ID获取程序ID (Title ID)
    if (R_FAILED(pmdmntGetProgramId(&tid, pid)))
        return 0;  
    
    // 3. 过滤非游戏ID
    u8 type = (u8)(tid >> 56);
    if (type == 0x01) return tid;  // 游戏直接通过
    if (s_whitelist.count(tid)) return tid;  // 白名单通过
    return 0;
}

// 根据Title ID获取游戏名称
bool GameMonitor::getTitleIdGameName(u64 titleId, char* result) {
    
    strcpy(result, "UNKNOWN");

    Result rc = nsInitialize();
    if (R_FAILED(rc)) return false;
    
    // 创建控制数据的智能指针
    auto control_data = std::make_unique<NsApplicationControlData>();
    u64 jpeg_size{};
    
    // 获取应用程序控制数据
    rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, titleId, control_data.get(), sizeof(NsApplicationControlData), &jpeg_size);
    if (R_FAILED(rc)) {
        nsExit();            
        return false;
    }
    
    // 声明entry变量
    NacpLanguageEntry* entry = nullptr;
    int NameLanguageIndex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    int systemLanguageIndex = 0;
    switch (g_systemLanguage) {
        case SetLanguage_ENUS:
            systemLanguageIndex = 0;
            break;
        case SetLanguage_ENGB:
            systemLanguageIndex = 1;
            break;
        case SetLanguage_JA:
            systemLanguageIndex = 2;
            break;
        case SetLanguage_FR:
            systemLanguageIndex = 3;
            break;
        case SetLanguage_DE:
            systemLanguageIndex = 4;
            break;
        case SetLanguage_ES419:
            systemLanguageIndex = 5;
            break;
        case SetLanguage_ES:
            systemLanguageIndex = 6;
            break;
        case SetLanguage_IT:
            systemLanguageIndex = 7;
            break;
        case SetLanguage_NL:
            systemLanguageIndex = 8;
            break;
        case SetLanguage_FRCA:
            systemLanguageIndex = 9;
            break;
        case SetLanguage_PT:
            systemLanguageIndex = 10;
            break;
        case SetLanguage_RU:
            systemLanguageIndex = 11;
            break;
        case SetLanguage_KO:
            systemLanguageIndex = 12;
            break;
        case SetLanguage_ZHTW:
        case SetLanguage_ZHHANT:
            systemLanguageIndex = 13;
            break;
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            systemLanguageIndex = 14;
            break;
        case SetLanguage_PTBR:
            systemLanguageIndex = 15;
            break;
        default:
            systemLanguageIndex = 0;
    }

    // 优先使用系统语言
    entry = &control_data->nacp.lang[systemLanguageIndex];
    if (entry->name[0] != '\0'){
        strcpy(result, entry->name);
        nsExit();            
        return true;
    }
    
    // 遍历所有语言
    for (int i = 0; i < 16; i++) {
        if (NameLanguageIndex[i] == systemLanguageIndex) continue; 
        entry = &control_data->nacp.lang[NameLanguageIndex[i]];
        if (entry->name[0] == '\0') continue;
        strcpy(result, entry->name);
        nsExit();            
        return true;
    }

    nsExit();
    return false;
}

// 获取所有已安装应用的Title ID列表
std::vector<u64> GameMonitor::getInstalledAppIds() {
    std::vector<u64> result;
    
    NsApplicationRecord records[250];
    s32 count = 0;
    
    Result rc = nsListApplicationRecord(records, 250, 0, &count);
    if (R_SUCCEEDED(rc)) {
        for (s32 i = 0; i < count; i++) {
            u64 tid = records[i].application_id;
            u8 type = (u8)(tid >> 56);
            // 过滤掉游戏(0x01)，保留其他应用
            if (type == 0x01) continue;
            result.push_back(tid);
        }
    }
    
    return result;
}

void GameMonitor::loadWhitelist() {
    s_whitelist.clear();
    auto kvPairs = ult::getKeyValuePairsFromSection(WHITE_INI_PATH, "white");
    for (const auto& kv : kvPairs) {
        u64 tid = strtoull(kv.first.c_str(), nullptr, 16);
        if (tid != 0) s_whitelist.insert(tid);
    }
}