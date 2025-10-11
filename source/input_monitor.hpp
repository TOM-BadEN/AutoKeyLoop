#pragma once

#include <switch.h>
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

// 按键事件类型枚举
enum class ButtonEventType {
    PRESS,    // 按下
    HOLD,     // 长按
    RELEASE   // 松开
};

// 按键事件结构体
struct ButtonEvent {
    ButtonEventType type;     // 事件类型
    const char* button_name;  // 按键名称
};

// 按键状态结构体
typedef struct {
    bool current_pressed;     // 当前帧是否按下
    bool previous_pressed;    // 上一帧是否按下
    u64 press_start_tick;     // 按下开始时间
    bool is_holding;          // 是否正在长按（用于跟踪连发状态）
} ButtonState;

// 按键信息结构体
typedef struct {
    u64 mask;                 // 按键位掩码
    const char* name;         // 按键名称
} ButtonInfo;

class InputMonitor {
private:
    static const int BUTTON_COUNT = 14;  // 监控的按键数量（不包括摇杆）
    static const u64 CLICK_THRESHOLD = 5760000ULL; // 300ms的时间阈值（基于19.2MHz系统时钟）
    
    ButtonState button_states[BUTTON_COUNT];  // 按键状态数组
    ButtonInfo button_infos[BUTTON_COUNT];    // 按键信息数组
    
    // 事件回调函数
    std::function<void(const ButtonEvent&)> event_callback;
    
    // 检查单个按键状态变化
    void CheckButtonState(int button_index, u64 current_buttons);
    
    // 获取系统时钟
    u64 GetSystemTick();

public:
    // 构造函数
    InputMonitor();
    
    // 初始化输入监控
    void Initialize();
    
    // 设置事件回调函数
    void SetEventCallback(std::function<void(const ButtonEvent&)> callback);
    
    // 更新按键状态（每帧调用）
    void Update();
    
    // 析构函数
    ~InputMonitor();
};

#ifdef __cplusplus
}
#endif