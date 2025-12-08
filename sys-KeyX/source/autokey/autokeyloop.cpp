#include "autokeyloop.hpp"
#include <cstring>
#include "minIni.h"
#include "common.hpp"

namespace {
    // 摇杆伪按键位掩码 (BIT16-23)，必须过滤
    constexpr u64 STICK_PSEUDO_BUTTON_MASK = 0xFF0000ULL;
    
    // 左 JoyCon 按键掩码（十字键、左肩键、左摇杆、SELECT）
    constexpr u64 LEFT_JOYCON_BUTTONS = 
        HidNpadButton_Left | HidNpadButton_Right | HidNpadButton_Up | HidNpadButton_Down |
        HidNpadButton_L | HidNpadButton_ZL | HidNpadButton_StickL |
        HidNpadButton_Minus;
    
    // 右 JoyCon 按键掩码（面键、右肩键、右摇杆、START）
    constexpr u64 RIGHT_JOYCON_BUTTONS = 
        HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y |
        HidNpadButton_R | HidNpadButton_ZR | HidNpadButton_StickR |
        HidNpadButton_Plus;
    
    // 更新间隔
    constexpr u64 UPDATE_INTERVAL_NS = 1000000ULL;  // 1ms
    constexpr const char* button_names[] = {
        "A", "B", "X", "Y",
        "Up", "Down", "Left", "Right",
        "L", "R", "ZL", "ZR",
        "StickL", "StickR", "Start", "Select"
    };
    
    // 读取手柄状态并应用到结果
    #define READ_NPAD_STATE(StateType, GetFunc, NpadId) \
        do { \
            StateType state; \
            size_t count = GetFunc(NpadId, &state, 1); \
            if (count > 0 && (state.attributes & HidNpadAttribute_IsConnected)) { \
                result.buttons = state.buttons & ~STICK_PSEUDO_BUTTON_MASK; \
                result.analog_stick_l = state.analog_stick_l; \
                result.analog_stick_r = state.analog_stick_r; \
            } \
        } while(0)
}

// 静态成员定义
alignas(0x1000) char AutoKeyLoop::thread_stack[4 * 1024];
alignas(0x1000) u8 AutoKeyLoop::hdls_work_buffer[0x1000];

// 构造函数
AutoKeyLoop::AutoKeyLoop(const char* config_path, const char* macroCfgPath, bool enable_turbo, bool enable_macro) {
    // 初始化HDLS工作缓冲区
    Result rc = hiddbgAttachHdlsWorkBuffer(&m_HdlsSessionId, hdls_work_buffer, sizeof(hdls_work_buffer));
    if (R_FAILED(rc)) return;
    
    memset(&m_StateList, 0, sizeof(m_StateList));
    m_HdlsInitialized = true;
    
    // 初始化状态
    m_ShouldExit = false;
    m_IsPaused = false;
    
    // 初始化逆映射表
    m_MappingCount = 0;
    memset(m_TargetMasks, 0, sizeof(m_TargetMasks));
    memset(m_SourceMasks, 0, sizeof(m_SourceMasks));
    
    // 初始化功能开关
    m_EnableTurbo = enable_turbo;
    m_EnableMacro = enable_macro;
    // 根据开关创建功能模块
    if (m_EnableTurbo) m_Turbo = std::make_unique<Turbo>(config_path);
    if (m_EnableMacro) m_Macro = std::make_unique<Macro>(macroCfgPath);
    
    // 初始化手柄类型
    m_ControllerType = ControllerType::C_NONE;
    
    // 加载按键映射配置并生成逆映射表
    UpdateButtonMappings(config_path);
    
    // 初始化线程状态
    m_ThreadCreated = false;
    m_ThreadRunning = false;
    memset(&m_Thread, 0, sizeof(Thread));
    
    // 创建线程
    rc = threadCreate(&m_Thread, ThreadFunc, this, thread_stack, sizeof(thread_stack), 44, -2);
    if (R_FAILED(rc)) return;
    m_ThreadCreated = true;
    
    rc = threadStart(&m_Thread);
    if (R_FAILED(rc)) return;
    m_ThreadRunning = true;
}

// 析构函数
AutoKeyLoop::~AutoKeyLoop() {
    // 停止线程
    m_ShouldExit = true;
    if (m_ThreadRunning) threadWaitForExit(&m_Thread);
    if (m_ThreadCreated) threadClose(&m_Thread);
    // 释放HDLS
    if (m_HdlsInitialized) hiddbgReleaseHdlsWorkBuffer(m_HdlsSessionId);
}

// 线程函数
void AutoKeyLoop::ThreadFunc(void* arg) {
    AutoKeyLoop* loop = static_cast<AutoKeyLoop*>(arg);
    loop->MainLoop();
}

// 主循环
void AutoKeyLoop::MainLoop() {
    while (!m_ShouldExit) {
        ProcessResult result{};
        ReadPhysicalInput(result);
        DetermineEvent(result);
        switch (result.event) {
            case FeatureEvent::PAUSED:
                for (int i = 0; i < 10 && !m_ShouldExit; ++i) svcSleepThread(100000000ULL);  // 100ms
                continue;
            case FeatureEvent::IDLE:
                break;
            case FeatureEvent::STARTING:
                hiddbgDumpHdlsStates(m_HdlsSessionId, &m_StateList);
                break;
            case FeatureEvent::Turbo_EXECUTING:
                ApplyHdlsState(result);
                break;
            case FeatureEvent::Macro_EXECUTING:
                ApplyHdlsState(result);
                break;
            case FeatureEvent::FINISHING:
                ApplyHdlsState(result);
                break;
        }
        svcSleepThread(UPDATE_INTERVAL_NS);
    }
}

// 判定事件
void AutoKeyLoop::DetermineEvent(ProcessResult& result) {
    /*
        事件判定规则：
        1. 连发或者宏模块内部，会返回应该处于的事件
        2. 检测事件时优先检测宏的事件
        3. 如果宏事件只要不是IDLE，就直接返回宏事件
        4. 在返回之前，会检查宏事件是否为STARTING，且若连发模块启用了，就重置一次连发的状态机参数
        5. 如果宏事件是返回的IDLE，则检查连发模块是否启用，如果启用则返回连发模块的事件
        6. 如果连发模块没有启用，则返回IDLE
    */ 
    if (m_IsPaused || m_ControllerType == ControllerType::C_NONE) {
        result.event = FeatureEvent::PAUSED;
        return;
    }
    if (m_Macro) {
        m_Macro->Process(result);
        if (result.event == FeatureEvent::STARTING && m_Turbo) m_Turbo->TurboFinishing();
        if (result.event != FeatureEvent::IDLE) return;
    }
    if (m_Turbo) {
        m_Turbo->Process(result);
        return;
    }
    result.event = FeatureEvent::IDLE;
}


// 暂停
void AutoKeyLoop::Pause() {
    if (m_Turbo) m_Turbo->TurboFinishing();
    if (m_Macro) m_Macro->MacroFinishing();
    m_IsPaused = true;
}

// 恢复
void AutoKeyLoop::Resume() {
    m_IsPaused = false;
}

// 更新连发功能
void AutoKeyLoop::UpdateTurboFeature(bool enable, const char* config_path) {
    if (m_EnableTurbo && enable && m_Turbo) m_Turbo->LoadConfig(config_path);
    else if (m_EnableTurbo && !enable && m_Turbo) m_Turbo.reset();
    else if (!m_EnableTurbo && enable) m_Turbo = std::make_unique<Turbo>(config_path);
    m_EnableTurbo = enable;
}

// 更新宏功能
void AutoKeyLoop::UpdateMacroFeature(bool enable, const char* macroCfgPath) {
    if (m_EnableMacro && enable && m_Macro) m_Macro->LoadConfig(macroCfgPath);
    else if (m_EnableMacro && !enable && m_Macro) m_Macro.reset();
    else if (!m_EnableMacro && enable) m_Macro = std::make_unique<Macro>(macroCfgPath);
    m_EnableMacro = enable;
}

// 按键名转换为掩码
u64 AutoKeyLoop::ButtonNameToMask(const char* name) const {
    if (strcmp(name, "A") == 0) return HidNpadButton_A;
    if (strcmp(name, "B") == 0) return HidNpadButton_B;
    if (strcmp(name, "X") == 0) return HidNpadButton_X;
    if (strcmp(name, "Y") == 0) return HidNpadButton_Y;
    if (strcmp(name, "StickL") == 0) return HidNpadButton_StickL;
    if (strcmp(name, "StickR") == 0) return HidNpadButton_StickR;
    if (strcmp(name, "L") == 0) return HidNpadButton_L;
    if (strcmp(name, "R") == 0) return HidNpadButton_R;
    if (strcmp(name, "ZL") == 0) return HidNpadButton_ZL;
    if (strcmp(name, "ZR") == 0) return HidNpadButton_ZR;
    if (strcmp(name, "Select") == 0 || strcmp(name, "Minus") == 0) return HidNpadButton_Minus;
    if (strcmp(name, "Start") == 0 || strcmp(name, "Plus") == 0) return HidNpadButton_Plus;
    if (strcmp(name, "Left") == 0) return HidNpadButton_Left;
    if (strcmp(name, "Up") == 0) return HidNpadButton_Up;
    if (strcmp(name, "Right") == 0) return HidNpadButton_Right;
    if (strcmp(name, "Down") == 0) return HidNpadButton_Down;
    return 0;
}

// 更新按键映射配置（生成逆映射表）
void AutoKeyLoop::UpdateButtonMappings(const char* config_path) {
    m_MappingCount = 0;
    for (int i = 0; i < MAX_MAPPINGS; i++) {
        char target_buf[64];
        ini_gets("MAPPING", button_names[i], button_names[i], target_buf, sizeof(target_buf), config_path);
        if (strcmp(button_names[i], target_buf) == 0) continue;
        u64 source_mask = ButtonNameToMask(button_names[i]);
        u64 target_mask = ButtonNameToMask(target_buf);
        if (source_mask == 0 || target_mask == 0) continue;
        bool found = false;
        for (int j = 0; j < m_MappingCount; j++) {
            if (m_TargetMasks[j] == target_mask) {
                m_SourceMasks[j] = source_mask;  // 覆盖
                found = true;
                break;
            }
        }
        if (!found && m_MappingCount < MAX_MAPPINGS) {
            m_TargetMasks[m_MappingCount] = target_mask;
            m_SourceMasks[m_MappingCount] = source_mask;
            m_MappingCount++;
        }
    }
}

// 应用逆映射到按键（两阶段处理：先收集，后应用）
void AutoKeyLoop::ApplyReverseMapping(u64& buttons) const {
    if (m_MappingCount == 0 || buttons == 0) return;
    u64 to_clear = 0;  // 要清除的按键掩码
    u64 to_set = 0;    // 要设置的按键掩码
    // 第一阶段：遍历逆映射表，收集要修改的按键
    for (int i = 0; i < m_MappingCount; i++) {
        if (buttons & m_TargetMasks[i]) {
            to_clear |= m_TargetMasks[i];  // 记录要清除的目标按键
            to_set |= m_SourceMasks[i];    // 记录要设置的源按键
        }
    }
    // 第二阶段：一次性应用所有修改
    // 1. 清除所有要映射的按键：buttons & ~to_clear
    // 2. 设置所有映射后的按键：... | to_set
    buttons = (buttons & ~to_clear) | to_set;
}

// 读取物理输入
void AutoKeyLoop::ReadPhysicalInput(ProcessResult& result) {
    // 先确认手柄类型
    HidNpadIdType npad_id = HidNpadIdType_No1;
    u32 style_set = hidGetNpadStyleSet(npad_id);
    u32 handheld_style = hidGetNpadStyleSet(HidNpadIdType_Handheld);
    // 默认为未知
    m_ControllerType = ControllerType::C_NONE;
    if (style_set & HidNpadStyleTag_NpadFullKey) m_ControllerType = ControllerType::C_PRO;
    else if (style_set & HidNpadStyleTag_NpadJoyDual) m_ControllerType = ControllerType::C_JOYDUAL;
    else if (style_set & HidNpadStyleTag_NpadSystemExt) m_ControllerType = ControllerType::C_SYSTEMEXT;
    else if (handheld_style & HidNpadStyleTag_NpadHandheld) m_ControllerType = ControllerType::C_HANDHELD;
    // 根据类型读取按键数据
    switch (m_ControllerType) {
        case ControllerType::C_PRO:
            READ_NPAD_STATE(HidNpadFullKeyState, hidGetNpadStatesFullKey, npad_id);
            break;
        case ControllerType::C_JOYDUAL:
            READ_NPAD_STATE(HidNpadJoyDualState, hidGetNpadStatesJoyDual, npad_id);
            break;
        case ControllerType::C_SYSTEMEXT:
            READ_NPAD_STATE(HidNpadSystemExtState, hidGetNpadStatesSystemExt, npad_id);
            break;
        case ControllerType::C_HANDHELD:
            READ_NPAD_STATE(HidNpadHandheldState, hidGetNpadStatesHandheld, HidNpadIdType_Handheld);
            break;
        default:
            break;
    }
}


void AutoKeyLoop::ApplyHdlsState(ProcessResult& result) {
    switch (m_ControllerType) {
        case ControllerType::C_PRO:
        case ControllerType::C_SYSTEMEXT:
            InjectPro(result);
            break;
        case ControllerType::C_JOYDUAL:
            InjectJoyDual(result);
            break;
        case ControllerType::C_HANDHELD:
            InjectHandheld(result);
            break;
        default:
            break;
    }
}

// Pro手柄注入
void AutoKeyLoop::InjectPro(ProcessResult& result) {
    for (int i = 0; i < m_StateList.total_entries; i++) {
        HidDeviceType device_type = (HidDeviceType)m_StateList.entries[i].device.deviceType;
        HiddbgHdlsState state;
        memset(&state, 0, sizeof(HiddbgHdlsState));
        if (device_type == HidDeviceType_FullKey3) {
            state.buttons = result.OtherButtons;
            state.analog_stick_l = result.analog_stick_l;
            state.analog_stick_r = result.analog_stick_r;
            hiddbgSetHdlsState(m_StateList.entries[i].handle, &state);
            break;
        }
    }
}

// 双JoyCon蓝牙模式
// 已知：蓝牙模式注入延迟（可能我猜的）非常大，反正jc蓝牙模式效果极差基本等于不可用
void AutoKeyLoop::InjectJoyDual(ProcessResult& result) {
    int found_count = 0;
    for (int i = 0; i < m_StateList.total_entries; i++) {
        HidDeviceType device_type = (HidDeviceType)m_StateList.entries[i].device.deviceType;
        HiddbgHdlsState state;
        memset(&state, 0, sizeof(HiddbgHdlsState));
        if (device_type == HidDeviceType_JoyLeft2) {
            found_count++;
            if (result.event != FeatureEvent::Macro_EXECUTING) continue;
            state.buttons = result.OtherButtons & LEFT_JOYCON_BUTTONS;
            state.analog_stick_l = result.analog_stick_l;
            hiddbgSetHdlsState(m_StateList.entries[i].handle, &state);
        }
        else if (device_type == HidDeviceType_JoyRight1) {
            found_count++;
            state.buttons = result.OtherButtons & RIGHT_JOYCON_BUTTONS;
            state.analog_stick_r = result.analog_stick_r;
            hiddbgSetHdlsState(m_StateList.entries[i].handle, &state);
        }
        if (found_count == 2) break;
    }
}


// 掌机模式注入
void AutoKeyLoop::InjectHandheld(ProcessResult& result) {
    int found_count = 0;
    for (int i = 0; i < m_StateList.total_entries; i++) {
        HidDeviceType device_type = (HidDeviceType)m_StateList.entries[i].device.deviceType;
        HiddbgHdlsState state;
        memset(&state, 0, sizeof(HiddbgHdlsState));
        if (device_type == HidDeviceType_JoyLeft2) {
            found_count++;
            if (result.event != FeatureEvent::Macro_EXECUTING) continue;
            state.buttons = result.OtherButtons & LEFT_JOYCON_BUTTONS;
            state.analog_stick_l = result.analog_stick_l;
            hiddbgSetHdlsState(m_StateList.entries[i].handle, &state);
        }
        else if (device_type == HidDeviceType_JoyRight1) {
            found_count++;
            state.buttons = result.OtherButtons & RIGHT_JOYCON_BUTTONS;
            state.analog_stick_r = result.analog_stick_r;
            hiddbgSetHdlsState(m_StateList.entries[i].handle, &state);
        } else if (device_type == HidDeviceType_DebugPad) {
            ApplyReverseMapping(result.OtherButtons);
            state.buttons = result.OtherButtons;
            state.analog_stick_l = result.analog_stick_l;
            state.analog_stick_r = result.analog_stick_r;
            hiddbgSetHdlsState(m_StateList.entries[i].handle, &state);
            break;
        }
        if (found_count == 2) break;
    }
}


