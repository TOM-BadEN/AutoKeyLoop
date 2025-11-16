#include "autokeyloop.hpp"
#include <cstring>
#include "minIni.h"

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
    
    // 判断是否为左 JoyCon
    constexpr bool IsLeftController(HidDeviceType type) {
        return type == HidDeviceType_JoyLeft2 || 
               type == HidDeviceType_JoyLeft4 ||
               type == HidDeviceType_LarkHvcLeft ||
               type == HidDeviceType_LarkNesLeft;
    }
    
    // 判断是否为右 JoyCon
    constexpr bool IsRightController(HidDeviceType type) {
        return type == HidDeviceType_JoyRight1 || 
               type == HidDeviceType_JoyRight5 ||
               type == HidDeviceType_LarkHvcRight ||
               type == HidDeviceType_LarkNesRight;
    }
    
    // 更新间隔
    constexpr u64 UPDATE_INTERVAL_NS = 1000000ULL;  // 1ms
    constexpr const char* button_names[] = {
        "A", "B", "X", "Y",
        "Up", "Down", "Left", "Right",
        "L", "R", "ZL", "ZR",
        "StickL", "StickR", "Start", "Select"
    };
}

// 静态成员定义
alignas(0x1000) char AutoKeyLoop::thread_stack[32 * 1024];
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
            case FeatureEvent::EXECUTING:
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
    if (m_IsPaused) {
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
// 必须按当前顺序读取，不然第三方手柄可能会提供假类型，导致读取错误
void AutoKeyLoop::ReadPhysicalInput(ProcessResult& result) {
    HidNpadIdType npad_id = HidNpadIdType_No1;
    u32 style_set = hidGetNpadStyleSet(npad_id);
    HidNpadCommonState common_state;
    size_t count = 0;
    
    if (style_set != 0 && (style_set & HidNpadStyleTag_NpadSystemExt)) {  // 三方手柄
        count = hidGetNpadStatesSystemExt(npad_id, (HidNpadSystemExtState*)&common_state, 1);
        if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
            result.buttons = common_state.buttons & ~STICK_PSEUDO_BUTTON_MASK;
            result.analog_stick_l = common_state.analog_stick_l;
            result.analog_stick_r = common_state.analog_stick_r;
            return;
        }
    }
    
    // 再检测Handheld（Lite和掌机状态的其他机型）
    u32 handheld_style = hidGetNpadStyleSet(HidNpadIdType_Handheld);
    if (handheld_style & HidNpadStyleTag_NpadHandheld) {
        HidNpadHandheldState handheld_state;
        count = hidGetNpadStatesHandheld(HidNpadIdType_Handheld, &handheld_state, 1);
        if (count > 0 && (handheld_state.attributes & HidNpadAttribute_IsConnected)) {
            result.buttons = handheld_state.buttons & ~STICK_PSEUDO_BUTTON_MASK;
            result.analog_stick_l = handheld_state.analog_stick_l;
            result.analog_stick_r = handheld_state.analog_stick_r;
            return;
        }
    }
    
    // 没有手柄连接，返回空状态
    if (style_set == 0) return;
    
    if (style_set & HidNpadStyleTag_NpadFullKey) {  // 完整手柄，如pro
        count = hidGetNpadStatesFullKey(npad_id, (HidNpadFullKeyState*)&common_state, 1);
        if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
            result.buttons = common_state.buttons & ~STICK_PSEUDO_BUTTON_MASK;
            result.analog_stick_l = common_state.analog_stick_l;
            result.analog_stick_r = common_state.analog_stick_r;
            return;
        }
    } else if (style_set & HidNpadStyleTag_NpadJoyDual) {  // 狗头joycon
        count = hidGetNpadStatesJoyDual(npad_id, (HidNpadJoyDualState*)&common_state, 1);
        if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
            result.buttons = common_state.buttons & ~STICK_PSEUDO_BUTTON_MASK;
            result.analog_stick_l = common_state.analog_stick_l;
            result.analog_stick_r = common_state.analog_stick_r;
            return;
        }
    } else if (style_set & (HidNpadStyleTag_NpadJoyLeft | HidNpadStyleTag_NpadJoyRight)) {  //分体JC
        HidNpadJoyLeftState left_state;
        HidNpadJoyRightState right_state;
        size_t left_count = hidGetNpadStatesJoyLeft(npad_id, &left_state, 1);
        size_t right_count = hidGetNpadStatesJoyRight(npad_id, &right_state, 1);
        if (left_count > 0 && (left_state.attributes & HidNpadAttribute_IsConnected)) {
            result.buttons |= left_state.buttons;
            result.analog_stick_l = left_state.analog_stick_l;
        }
        if (right_count > 0 && (right_state.attributes & HidNpadAttribute_IsConnected)) {
            result.buttons |= right_state.buttons;
            result.analog_stick_r = right_state.analog_stick_r;
        }
        result.buttons &= ~STICK_PSEUDO_BUTTON_MASK;
    }
}

// 注入输出
// jc有未知机制，反正只注入我们修改的按键，其他的强制归0，不然即使注入结束，还会一直触发
// 其他手柄，发送所有按键+摇杆
void AutoKeyLoop::ApplyHdlsState(ProcessResult& result) {
    // 应用逆映射（直接修改result中的按键）
    ApplyReverseMapping(result.OtherButtons);
    ApplyReverseMapping(result.JoyconButtons);
    
    for (int i = 0; i < m_StateList.total_entries; i++) {
        memset(&m_StateList.entries[i].state, 0, sizeof(HiddbgHdlsState));
        HidDeviceType device_type = (HidDeviceType)m_StateList.entries[i].device.deviceType;
        if (IsLeftController(device_type)) {    
            m_StateList.entries[i].state.buttons = result.JoyconButtons & LEFT_JOYCON_BUTTONS;
        } else if (IsRightController(device_type)) { 
            m_StateList.entries[i].state.buttons = result.JoyconButtons & RIGHT_JOYCON_BUTTONS;
        } else {  
            m_StateList.entries[i].state.buttons = result.OtherButtons;
            m_StateList.entries[i].state.analog_stick_l = result.analog_stick_l;
            m_StateList.entries[i].state.analog_stick_r = result.analog_stick_r;
        }
    }
    hiddbgApplyHdlsStateList(m_HdlsSessionId, &m_StateList);
}


