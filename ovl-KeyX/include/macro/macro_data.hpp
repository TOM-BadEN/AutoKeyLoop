#pragma once
#include <vector>
#include <string>
#include <switch.h>

// 宏文件头
struct MacroHeader {
    char magic[4];      // "KEYX"
    u16 version;        // 版本号
    u16 frameRate;      // 帧率
    u64 titleId;        // 游戏TID
    u32 frameCount;     // 总帧数
} __attribute__((packed));

// 宏单帧数据 (V1)
struct MacroFrame {
    u64 keysHeld;       // 按键状态
    s32 leftX;          // 左摇杆X
    s32 leftY;          // 左摇杆Y
    s32 rightX;         // 右摇杆X
    s32 rightY;         // 右摇杆Y
} __attribute__((packed));

// 宏单帧数据 (V2: 带时间戳方案，更精确更稳定的录制与播放)
// 新增与1.4.2版本
struct MacroFrameV2 {
    u32 durationMs;     // 持续时间（毫秒）
    u64 keysHeld;       // 按键状态
    s32 leftX;          // 左摇杆X
    s32 leftY;          // 左摇杆Y
    s32 rightX;         // 右摇杆X
    s32 rightY;         // 右摇杆Y
} __attribute__((packed));

// 摇杆方向枚举
enum class StickDir { 
    None, Up, UpRight, Right, DownRight, 
    Down, DownLeft, Left, UpLeft 
};

// 单个动作
struct Action {
    u64 buttons;        // 按键位掩码
    StickDir stickL;    // 左摇杆方向
    StickDir stickR;    // 右摇杆方向
    u32 duration;       // 持续时间（毫秒）
    u32 frameCount;     // 帧数量
    bool modified = false;  // 是否被修改过
};

// 宏基础信息
struct MacroBasicInfo {
    u64 titleId;           // 游戏TID
    char fileName[64];     // 文件名（不含路径和扩展名）
    u32 fileSize;          // 文件大小（字节）
    u32 durationMs;        // 总时长（毫秒）
    u32 frameRate;         // 帧率
    u32 frameCount;        // 总帧数
    u16 version;           // 版本号
};

class MacroData {
public:
    static bool load(const char* filePath);
    static bool loadFrameAndBasicInfo(const char* filePath);
    static bool loadBakMacroData();
    static bool saveForEdit();
    static const char* getFilePath();
    static const MacroHeader& getHeader();
    static const MacroBasicInfo& getBasicInfo();
    static void parseActions();
    static std::vector<Action>& getActions();
    
    // 编辑操作
    static void insertAction(s32 actionIndex, bool insertBefore);
    static void resetActions(s32 startIndex, s32 endIndex);
    static void deleteActions(s32 startIndex, s32 endIndex);
    static void setActionDuration(s32 actionIndex, u32 durationMs);
    static void setActionButtons(s32 actionIndex, u64 buttons);
    
    // 撤销功能
    static bool undo();
    static bool canUndo();
    
    // 内存管理
    static void allCleanup();
    static void undoCleanup();

private:
    static char s_filePath[128];
    static MacroHeader s_header;
    static MacroBasicInfo s_basicInfo;
    static std::vector<MacroFrame> s_frames;
    static std::vector<MacroFrameV2> s_framesV2;
    static std::vector<Action> s_actions;
    
    // 撤销快照栈（最多3层）
    struct UndoSnapshot {
        std::vector<MacroFrame> frames;
        std::vector<MacroFrameV2> framesV2;
        std::vector<Action> actions;
    };
    static std::vector<UndoSnapshot> s_undoStack;
    
    // 辅助函数：判断摇杆方向
    static StickDir getStickDir(s32 x, s32 y);
    
    // 辅助函数：判断两帧状态是否相同
    static bool isSameState(const MacroFrame& a, const MacroFrame& b);
    static bool isSameState(const MacroFrameV2& a, const MacroFrameV2& b);
    
    // 辅助函数：判断两个动作是否相同（可合并）
    static bool isSameAction(const Action& a, const Action& b);
    
    // 辅助函数：尝试合并指定位置与前后相邻的相同动作
    static void tryMergeAt(s32 actionIndex);
    
    // 辅助函数：删除后尝试合并（检查deletedIndex-1与deletedIndex位置）
    static void tryMergeAfterDelete(s32 deletedIndex);

    // 辅助函数：保存快照,用于撤销
    static void saveSnapshot();
    
    // 辅助函数：解析动作（版本分离）
    static void parseActionsV1();
    static void parseActionsV2();
    
    // 辅助函数：保存帧数据（版本分离）
    static void saveForEditV1(FILE* fp);
    static void saveForEditV2(FILE* fp);
};
