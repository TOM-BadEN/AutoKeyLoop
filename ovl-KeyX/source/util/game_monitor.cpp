#include "game_monitor.hpp"

// 获取当前运行程序的Title ID
u64 GameMonitor::getCurrentTitleId() {
    u64 pid = 0, tid = 0;
    
    // 1. 获取当前应用的进程ID
    if (R_FAILED(pmdmntGetApplicationProcessId(&pid)))
        return 0;  
    
    // 2. 根据进程ID获取程序ID (Title ID)
    if (R_FAILED(pmdmntGetProgramId(&tid, pid)))
        return 0;  
    
    // 3. 过滤非游戏ID - 检查Title ID是否为游戏格式（高32位以0x01开头）
    u32 high_part = (u32)(tid >> 32);
    if ((high_part & 0xFF000000) != 0x01000000) {
        return 0;  // 不是游戏ID，返回0
    }
    
    return tid; 
}

// 根据Title ID获取游戏名称
bool GameMonitor::getTitleIdGameName(u64 titleId, char* result) {
    
    strcpy(result, "未知游戏");

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
    
    // 语言优先级：简体中文(14) -> 繁体中文(13) -> 其他语言(0-15)
    int NameLanguageIndex[] = {14, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15};
    
    // 按优先级遍历所有语言
    for (int i = 0; i < 16; i++) {
        entry = &control_data->nacp.lang[NameLanguageIndex[i]];
        if (entry->name[0] == '\0') continue;
        strcpy(result, entry->name);
        nsExit();            
        return true;
    }

    nsExit();
    return false;
}