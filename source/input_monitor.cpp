#include "input_monitor.hpp"
#include "../log/log.h"
#include <cstring>

// 构造函数 - 初始化按键信息数组（不包括摇杆按键）
InputMonitor::InputMonitor() {
    // 初始化按键信息数组（不包括摇杆按键）
    button_infos[0] = {HidNpadButton_A, "A"};
    button_infos[1] = {HidNpadButton_B, "B"};
    button_infos[2] = {HidNpadButton_X, "X"};
    button_infos[3] = {HidNpadButton_Y, "Y"};
    button_infos[4] = {HidNpadButton_L, "L"};
    button_infos[5] = {HidNpadButton_R, "R"};
    button_infos[6] = {HidNpadButton_ZL, "ZL"};
    button_infos[7] = {HidNpadButton_ZR, "ZR"};
    button_infos[8] = {HidNpadButton_Plus, "Plus"};
    button_infos[9] = {HidNpadButton_Minus, "Minus"};
    button_infos[10] = {HidNpadButton_Up, "Up"};
    button_infos[11] = {HidNpadButton_Down, "Down"};
    button_infos[12] = {HidNpadButton_Left, "Left"};
    button_infos[13] = {HidNpadButton_Right, "Right"};
    
    // 初始化按键状态
    memset(button_states, 0, sizeof(button_states));
}

// 析构函数
InputMonitor::~InputMonitor() {
    // 清理资源（如果需要）
}

// 初始化输入监控
void InputMonitor::Initialize() {
    // 初始化Npad系统
    hidInitializeNpad();
    
    // 设置支持的控制器类型
    u32 style_set = HidNpadStyleTag_NpadHandheld | HidNpadStyleTag_NpadFullKey | HidNpadStyleTag_NpadJoyDual;
    hidSetSupportedNpadStyleSet(style_set);
    
    // 设置支持的控制器ID
    HidNpadIdType npad_ids[] = {HidNpadIdType_Handheld, HidNpadIdType_No1};
    hidSetSupportedNpadIdType(npad_ids, sizeof(npad_ids) / sizeof(npad_ids[0]));
    
    log_info("InputMonitor initialized - 开始监控按键状态");
}

// 设置事件回调函数
void InputMonitor::SetEventCallback(std::function<void(const ButtonEvent&)> callback) {
    event_callback = callback;
}

// 获取系统时钟
u64 InputMonitor::GetSystemTick() {
    return svcGetSystemTick();
}

// 检查单个按键状态变化
void InputMonitor::CheckButtonState(int button_index, u64 current_buttons) {
    ButtonState* state = &button_states[button_index];
    const ButtonInfo* info = &button_infos[button_index];
    
    // 更新当前按键状态
    state->current_pressed = (current_buttons & info->mask) != 0;
    
    // 检测状态变化
    if (state->current_pressed && !state->previous_pressed) {
        // 按键刚按下 - 记录按下时间并触发按下事件
        state->press_start_tick = GetSystemTick();
        if (event_callback) {
            event_callback({ButtonEventType::PRESS, info->name});
        }
        
    } else if (state->current_pressed && state->previous_pressed) {
        // 按键持续按下 - 检查是否达到长按阈值
        u64 current_tick = GetSystemTick();
        u64 hold_duration = current_tick - state->press_start_tick;
        
        if (hold_duration > CLICK_THRESHOLD && !state->is_holding) {
            // 达到长按阈值，触发长按事件
            if (event_callback) {
                event_callback({ButtonEventType::HOLD, info->name});
            }
            state->is_holding = true;
        }
        
    } else if (!state->current_pressed && state->previous_pressed) {
        // 按键刚松开 - 触发松开事件
        if (event_callback) {
            event_callback({ButtonEventType::RELEASE, info->name});
        }
        
        // 重置状态
        state->is_holding = false;
    }
    
    // 更新上一帧状态
    state->previous_pressed = state->current_pressed;
}

// 更新按键状态（每帧调用）
void InputMonitor::Update() {
    // 尝试读取掌机模式状态
    HidNpadHandheldState handheld_states[8];
    size_t count = hidGetNpadStatesHandheld(HidNpadIdType_Handheld, handheld_states, 8);
    
    u64 current_buttons = 0;
    
    if (count > 0) {
        // 使用最新的状态
        current_buttons = handheld_states[count - 1].buttons;
    } else {
        // 如果掌机模式没有数据，尝试读取Pro手柄状态
        HidNpadFullKeyState fullkey_states[8];
        count = hidGetNpadStatesFullKey(HidNpadIdType_No1, fullkey_states, 8);
        
        if (count > 0) {
            current_buttons = fullkey_states[count - 1].buttons;
        }
    }
    
    // 检查所有按键状态
    for (int i = 0; i < BUTTON_COUNT; i++) {
        CheckButtonState(i, current_buttons);
    }
}