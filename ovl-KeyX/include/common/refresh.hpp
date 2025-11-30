#pragma once
#include <cstdint>

// 界面刷新控制
namespace Refresh {
    
    // 刷新标志位（使用位掩码，可同时标记多个界面）
    enum RefrFlag : uint32_t {
        None            = 0,
        OnShow          = 1 << 0,   // 全局：overlay show 后刷新
        MainMenu        = 1 << 1,   // 主菜单
        TurboButton     = 1 << 2,   // 连发按键设置界面
        RemapConfig     = 1 << 3,   // 映射按键设置界面
        MacroGameList   = 1 << 4,   // 单个游戏的脚本列表
        MacroView       = 1 << 5,   // 单个脚本的详情界面
        MacroHotKey     = 1 << 6,   // 单个脚本的快捷键设置界面
        // 可继续添加更多界面...
    };
    
    extern uint32_t g_RefrFlags;
    
    // 请求刷新
    inline void RefrRequest(RefrFlag flag) {
        g_RefrFlags |= flag;
    }
    
    // 消耗刷新(自动清除)
    inline bool RefrConsume(RefrFlag flag) {
        if (g_RefrFlags & flag || g_RefrFlags & OnShow) {
            g_RefrFlags &= ~flag;
            g_RefrFlags &= ~OnShow;
            return true;
        }
        return false;
    }
    
    // 清除刷新
    inline void RefrClear(RefrFlag flag) {
        g_RefrFlags &= ~flag;
    }
    
    // 清除所有刷新
    inline void RefrClearAll() {
        g_RefrFlags = None;
    }
    
    // 设置多个刷新
    inline void RefrSetMultiple(uint32_t flags) {
        g_RefrFlags |= flags;
    }
}
