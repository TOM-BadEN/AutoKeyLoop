/*

放弃更新维护

未解决的问题

1. 非lite机型无法正常使用，因我没有非Lite机型，无法测试。


*/


#include "autokeymanager.hpp"
#include <cstring>

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
}

// 静态线程栈定义
alignas(0x1000) char AutoKeyManager::autokey_thread_stack[4 * 1024];

// 静态HDLS工作缓冲区定义
alignas(0x1000) u8 AutoKeyManager::hdls_work_buffer[0x1000];

// 构造函数
AutoKeyManager::AutoKeyManager(u64 buttons, int presstime, int fireinterval) {
    // 存储配置参数
    m_AutoKeyWhitelistMask = buttons;
    m_PressDurationNs = (u64)presstime * 1000000ULL;    // 毫秒转纳秒
    m_ReleaseDurationNs = (u64)fireinterval * 1000000ULL; // 毫秒转纳秒
    // 初始化HDLS工作缓冲区（核心，需要这个来虚拟输入）
    Result rc = hiddbgAttachHdlsWorkBuffer(&m_HdlsSessionId, hdls_work_buffer, sizeof(hdls_work_buffer));
    if (R_FAILED(rc)) return;
    memset(&m_StateList, 0, sizeof(m_StateList));
    // 用于析构函数的资源管理
    m_HdlsInitialized = true;
    // 初始化连发参数
    m_ShouldExit = false;
    m_AutoKeyIsPressed = false;
    m_AutoKeyLastSwitchTime = 0;
    memset(&m_PhysicalState, 0, sizeof(m_PhysicalState));
    // 初始化线程状态
    m_ThreadCreated = false;
    m_ThreadRunning = false;
    memset(&m_AutoKeyThread, 0, sizeof(Thread));
    // 创建连发线程（-2表示自适应核心分配）
    rc = threadCreate(&m_AutoKeyThread, autokey_thread_func, this, autokey_thread_stack, sizeof(autokey_thread_stack), 44, -2);
    if (R_FAILED(rc)) return;
    m_ThreadCreated = true;
    rc = threadStart(&m_AutoKeyThread);
    if (R_FAILED(rc)) return;
    m_ThreadRunning = true;
}

// 动态更新配置参数（线程安全）
void AutoKeyManager::UpdateConfig(u64 buttons, int presstime, int fireinterval) {
    std::lock_guard<std::mutex> lock(m_ConfigMutex);
    m_AutoKeyWhitelistMask = buttons;
    m_PressDurationNs = (u64)presstime * 1000000ULL;    // 毫秒转纳秒
    m_ReleaseDurationNs = (u64)fireinterval * 1000000ULL; // 毫秒转纳秒
    // 重置连发状态，让新配置立即生效（下次按键时重新计时）
    m_AutoKeyLastSwitchTime = 0;
    m_AutoKeyIsPressed = false;
}

// 暂停连发
void AutoKeyManager::Pause() {
    m_IsPaused = true;
    // 重置连发状态机，避免恢复时延续旧节奏
    m_AutoKeyLastSwitchTime = 0;
    m_AutoKeyIsPressed = false;
}

// 恢复连发
void AutoKeyManager::Resume() {
    m_IsPaused = false;
}

// 析构函数
AutoKeyManager::~AutoKeyManager() {
    // 下令停止线程
    m_ShouldExit = true;
    // 等待连发线程退出
    if (m_ThreadRunning) threadWaitForExit(&m_AutoKeyThread);
    // 关闭连发线程
    if (m_ThreadCreated) threadClose(&m_AutoKeyThread);
    // 释放HDLS工作缓冲区
    if (m_HdlsInitialized) hiddbgReleaseHdlsWorkBuffer(m_HdlsSessionId);
}


// 连发线程函数
void AutoKeyManager::autokey_thread_func(void* arg) {
    AutoKeyManager* manager = static_cast<AutoKeyManager*>(arg);
    manager->ProcessAutoKey();
}


// 连发处理函数（合并了输入读取）
void AutoKeyManager::ProcessAutoKey() {
    while (!m_ShouldExit) {
        if (m_IsPaused) {   // 暂停时，跳过所有工作
            svcSleepThread(1000000000ULL);  // 暂停时休眠1秒
            continue;
        }
        // 读取物理输入（完整状态，包括摇杆）
        ReadPhysicalInput(&m_PhysicalState);
        // 劫持并修改控制器状态
        HijackAndModifyState();
        svcSleepThread(UPDATE_INTERVAL_NS);
    }
}

// 读取物理输入（直接从HID读取真实手柄状态，绕过HDLS虚拟层）
void AutoKeyManager::ReadPhysicalInput(HidNpadHandheldState* out_state) {
    // 清空输出状态
    memset(out_state, 0, sizeof(HidNpadHandheldState));
    HidNpadIdType npad_id = HidNpadIdType_No1;
    u32 style_set = hidGetNpadStyleSet(npad_id);
    HidNpadCommonState common_state;
    size_t count = 0;
    if (style_set != 0 && (style_set & HidNpadStyleTag_NpadSystemExt)) {  // 三方手柄
        count = hidGetNpadStatesSystemExt(npad_id, (HidNpadSystemExtState*)&common_state, 1);
        if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
            out_state->buttons = common_state.buttons;
            out_state->analog_stick_l = common_state.analog_stick_l;
            out_state->analog_stick_r = common_state.analog_stick_r;
            out_state->attributes = common_state.attributes;
            return;
        }
    }
    // 再检测Handheld（Lite和掌机状态的其他机型）
    u32 handheld_style = hidGetNpadStyleSet(HidNpadIdType_Handheld);
    if (handheld_style & HidNpadStyleTag_NpadHandheld) {
        HidNpadHandheldState handheld_state;
        count = hidGetNpadStatesHandheld(HidNpadIdType_Handheld, &handheld_state, 1);
        if (count > 0 && (handheld_state.attributes & HidNpadAttribute_IsConnected)) {
            *out_state = handheld_state;
            return;
        }
    }
    // 没有手柄连接，返回空状态
    if (style_set == 0) return;
    if (style_set & HidNpadStyleTag_NpadFullKey) {  // 完整手柄，如pro
        count = hidGetNpadStatesFullKey(npad_id, (HidNpadFullKeyState*)&common_state, 1);
        if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
            out_state->buttons = common_state.buttons;
            out_state->analog_stick_l = common_state.analog_stick_l;
            out_state->analog_stick_r = common_state.analog_stick_r;
            out_state->attributes = common_state.attributes;
            return;
        }
    } else if (style_set & HidNpadStyleTag_NpadJoyDual) {  // 狗头joycon
        count = hidGetNpadStatesJoyDual(npad_id, (HidNpadJoyDualState*)&common_state, 1);
        if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
            out_state->buttons = common_state.buttons;
            out_state->analog_stick_l = common_state.analog_stick_l;
            out_state->analog_stick_r = common_state.analog_stick_r;
            out_state->attributes = common_state.attributes;
            return;
        }
    } else if (style_set & (HidNpadStyleTag_NpadJoyLeft | HidNpadStyleTag_NpadJoyRight)) {
        // 检测到单独Joy-Con，读取左右手柄并合并
        HidNpadJoyLeftState left_state;
        HidNpadJoyRightState right_state;
        size_t left_count = hidGetNpadStatesJoyLeft(npad_id, &left_state, 1);
        size_t right_count = hidGetNpadStatesJoyRight(npad_id, &right_state, 1);
        // 合并左手柄
        if (left_count > 0 && (left_state.attributes & HidNpadAttribute_IsConnected)) {
            out_state->buttons |= left_state.buttons;
            out_state->analog_stick_l = left_state.analog_stick_l;
            out_state->attributes |= left_state.attributes;
        }
        // 合并右手柄
        if (right_count > 0 && (right_state.attributes & HidNpadAttribute_IsConnected)) {
            out_state->buttons |= right_state.buttons;
            out_state->analog_stick_r = right_state.analog_stick_r;
            out_state->attributes |= right_state.attributes;
        }
    }
}

// 仅在"按下周期"检测真松开（避免污染）
bool AutoKeyManager::CheckReleaseInWindow(u64 autokey_buttons) {
    // 当前是松开周期（注入0），读到0可能是污染 → 跳过检测
    if (!m_AutoKeyIsPressed) return false; 
    // 安全期检查：距离上次状态切换 < 10ms 时，跳过检测（避免HDLS异步写入延迟导致误判）
    u64 current_time = armGetSystemTick();
    u64 time_since_last_switch_ns = armTicksToNs(current_time - m_AutoKeyLastSwitchTime);
    if (time_since_last_switch_ns < 10000000ULL) return false; 
    if (autokey_buttons == 0) return true;
    return false;
}

// 事件判定：根据当前状态判断应该执行什么操作
AutoKeyManager::TurboEvent AutoKeyManager::DetermineTurboEvent(u64 autokey_buttons) {
    bool has_autokey = (autokey_buttons != 0);           // 有连发按键被按下
    bool turbo_active = (m_AutoKeyLastSwitchTime != 0);  // 状态机运行中
    if (turbo_active && CheckReleaseInWindow(autokey_buttons)) return TurboEvent::TURBO_STOP;
    else if (turbo_active) return TurboEvent::TURBO_RUNNING;
    else if (has_autokey) return TurboEvent::TURBO_START;
    return TurboEvent::NO_ACTION;
}

// 事件处理：启动连发
void AutoKeyManager::HandleTurboStart(u64 autokey_buttons) {
    Result rc = hiddbgDumpHdlsStates(m_HdlsSessionId, &m_StateList);
    if (R_FAILED(rc) || m_StateList.total_entries == 0) return;
    u64 current_time = armGetSystemTick();
    m_AutoKeyLastSwitchTime = current_time;
    m_AutoKeyIsPressed = true;
}

// 事件处理：连发运行
void AutoKeyManager::HandleTurboRunning(u64 normal_buttons, u64 autokey_buttons) {
    u64 current_time = armGetSystemTick();
    u64 elapsed_ns = armTicksToNs(current_time - m_AutoKeyLastSwitchTime);
    // 检查是否到达切换时间
    u64 threshold = m_AutoKeyIsPressed ? m_PressDurationNs : m_ReleaseDurationNs;
    if (elapsed_ns >= threshold) {
        m_AutoKeyIsPressed = !m_AutoKeyIsPressed;
        m_AutoKeyLastSwitchTime = current_time;
    }
    // autokey_buttons 可能为0（松开周期污染），但状态机继续运行保持节奏
    u64 final_buttons = m_AutoKeyIsPressed ? (normal_buttons | autokey_buttons) : normal_buttons;
    ApplyHdlsState(final_buttons);
}

// 事件处理：停止连发
void AutoKeyManager::HandleTurboStop(u64 physical_buttons) {
    // 重置状态机
    m_AutoKeyLastSwitchTime = 0;
    m_AutoKeyIsPressed = false;
    // 透传完整物理状态
    ApplyHdlsState(physical_buttons & ~STICK_PSEUDO_BUTTON_MASK);
}

// 状态写入：统一的HDLS状态应用函数
void AutoKeyManager::ApplyHdlsState(u64 buttons) {
    for (int i = 0; i < m_StateList.total_entries; i++) {
        memset(&m_StateList.entries[i].state, 0, sizeof(HiddbgHdlsState));
        
        // 获取设备类型（显式转换 u8 到 HidDeviceType）
        HidDeviceType device_type = (HidDeviceType)m_StateList.entries[i].device.deviceType;
        
        // 根据设备类型过滤按键和摇杆
        if (IsLeftController(device_type)) {
            // 左 JoyCon：只发送左边按键+左摇杆
            m_StateList.entries[i].state.buttons = buttons & LEFT_JOYCON_BUTTONS;
            m_StateList.entries[i].state.analog_stick_l = m_PhysicalState.analog_stick_l;
            // analog_stick_r 保持为 0
        }
        else if (IsRightController(device_type)) {
            // 右 JoyCon：只发送右边按键+右摇杆
            m_StateList.entries[i].state.buttons = buttons & RIGHT_JOYCON_BUTTONS;
            m_StateList.entries[i].state.analog_stick_r = m_PhysicalState.analog_stick_r;
            // analog_stick_l 保持为 0
        }
        else {
            // 完整手柄（Pro、Lite、SNES等）：发送所有按键+双摇杆
            m_StateList.entries[i].state.buttons = buttons;
            m_StateList.entries[i].state.analog_stick_l = m_PhysicalState.analog_stick_l;
            m_StateList.entries[i].state.analog_stick_r = m_PhysicalState.analog_stick_r;
        }
    }
    hiddbgApplyHdlsStateList(m_HdlsSessionId, &m_StateList);
}

// 劫持并修改控制器状态（事件驱动）
void AutoKeyManager::HijackAndModifyState() {
    if (!m_HdlsInitialized) return;
    // 计算按键分类
    u64 physical_buttons = m_PhysicalState.buttons;
    u64 autokey_buttons = physical_buttons & m_AutoKeyWhitelistMask & ~STICK_PSEUDO_BUTTON_MASK;
    u64 normal_buttons = physical_buttons & ~m_AutoKeyWhitelistMask & ~STICK_PSEUDO_BUTTON_MASK;
    // 判定当前事件
    TurboEvent event = DetermineTurboEvent(autokey_buttons);
    switch (event) {
        case TurboEvent::NO_ACTION:
            return;
        case TurboEvent::TURBO_START:
            HandleTurboStart(autokey_buttons);
            return;
        case TurboEvent::TURBO_RUNNING:
            HandleTurboRunning(normal_buttons, autokey_buttons);
            return;
        case TurboEvent::TURBO_STOP:
            HandleTurboStop(physical_buttons);
            return;
    }
}

