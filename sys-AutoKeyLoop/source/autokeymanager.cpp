#include "autokeymanager.hpp"
#include <cstring>
#include "../log/log.h"

// 静态线程栈定义
alignas(0x1000) char AutoKeyManager::autokey_thread_stack[4 * 1024];
alignas(0x1000) char AutoKeyManager::input_reader_thread_stack[4 * 1024];

// 静态HDLS工作缓冲区定义
alignas(0x1000) u8 AutoKeyManager::hdls_work_buffer[0x1000];

// 构造函数
AutoKeyManager::AutoKeyManager(u64 buttons, int presstime, int fireinterval) {

    // 存储配置参数
    m_AutoKeyWhitelistMask = buttons;
    m_PressDurationNs = (u64)presstime * 1000000ULL;    // 毫秒转纳秒
    m_ReleaseDurationNs = (u64)fireinterval * 1000000ULL; // 毫秒转纳秒
    
    log_info("连发配置: 白名单=0x%llx, 按下=%dms, 松开=%dms", 
             buttons, presstime, fireinterval);

    // 初始化HDLS工作缓冲区（核心，需要这个来虚拟输入）
    Result rc = hiddbgAttachHdlsWorkBuffer(&m_HdlsSessionId, hdls_work_buffer, sizeof(hdls_work_buffer));
    if (R_FAILED(rc)) {
        log_error("HDLS工作缓冲区初始化失败: 0x%x", rc);
        return;
    }

    log_info("HDLS工作缓冲区初始化成功，会话_id: 0x%x", m_HdlsSessionId);
    
    // 初始化状态列表
    memset(&m_StateList, 0, sizeof(m_StateList));

    // 用于析构函数的资源管理
    m_HdlsInitialized = true;
    
    // 初始化连发参数
    m_ShouldExit = false;
    m_AutoKeyIsPressed = false;
    m_AutoKeyLastSwitchTime = 0;
    m_AutoKeyReleaseStartTime = 0;
    memset(&m_SharedPhysicalState, 0, sizeof(m_SharedPhysicalState));
    
    // 初始化线程状态
    m_ThreadCreated = false;
    m_ThreadRunning = false;
    m_InputThreadCreated = false;
    m_InputThreadRunning = false;
    memset(&m_AutoKeyThread, 0, sizeof(Thread));
    memset(&m_InputReaderThread, 0, sizeof(Thread));
    
    // 创建物理输入读取线程
    rc = threadCreate(&m_InputReaderThread, input_reader_thread_func, this,
                     input_reader_thread_stack, sizeof(input_reader_thread_stack), 44, 3);
    
    if (R_FAILED(rc)) {
        log_error("物理输入读取线程创建失败: 0x%x", rc);
        return;
    }

    // 设置线程创建标志
    m_InputThreadCreated = true;

    // 启动物理输入读取线程
    rc = threadStart(&m_InputReaderThread);

    // 启动物理输入读取线程失败
    if (R_FAILED(rc)) {
        log_error("物理输入读取线程启动失败: 0x%x", rc);
        return;
    }

    // 设置线程运行标志
    m_InputThreadRunning = true;

    // 创建连发线程
    rc = threadCreate(&m_AutoKeyThread, autokey_thread_func, this, 
                        autokey_thread_stack, sizeof(autokey_thread_stack), 44, 3);
    
    // 创建连发线程失败
    if (R_FAILED(rc)) {
        log_error("连发线程创建失败: 0x%x", rc);
        return;
    }

    // 设置线程创建标志
    m_ThreadCreated = true;

    // 启动连发线程
    rc = threadStart(&m_AutoKeyThread);

    // 启动连发线程失败
    if (R_FAILED(rc)) {
        log_error("连发线程启动失败: 0x%x", rc);
        return;
    }
    
    // 设置线程运行标志
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
    m_AutoKeyReleaseStartTime = 0;
    log_info("配置已动态更新: 白名单=0x%llx, 按下=%dms, 松开=%dms", 
             buttons, presstime, fireinterval);
}

// 析构函数
AutoKeyManager::~AutoKeyManager() {

    // 下令停止所有线程
    m_ShouldExit = true;
    
    // 等待物理输入读取线程退出
    if (m_InputThreadRunning) {
        threadWaitForExit(&m_InputReaderThread);
        log_info("输入读取线程已停止");
    }
    
    // 关闭物理输入读取线程
    if (m_InputThreadCreated) {
        threadClose(&m_InputReaderThread);
        log_info("输入读取线程关闭成功");
    }
    
    // 等待连发线程退出
    if (m_ThreadRunning) {
        threadWaitForExit(&m_AutoKeyThread);
        log_info("连发线程已停止");
    }
    
    // 关闭连发线程
    if (m_ThreadCreated) {
        threadClose(&m_AutoKeyThread);
        log_info("连发线程关闭成功");
    }
    
    // 释放HDLS工作缓冲区
    if (m_HdlsInitialized) {
        Result rc = hiddbgReleaseHdlsWorkBuffer(m_HdlsSessionId);
        if (R_FAILED(rc)) log_error("HDLS工作缓冲区释放失败: 0x%x", rc);
        else log_info("HDLS工作缓冲区释放成功");
    }
}


// 物理输入读取线程函数
void AutoKeyManager::input_reader_thread_func(void* arg) {
    AutoKeyManager* manager = static_cast<AutoKeyManager*>(arg);
    // 处理物理输入读取
    manager->ProcessInputReading();
}


// 连发线程函数
void AutoKeyManager::autokey_thread_func(void* arg) {
    AutoKeyManager* manager = static_cast<AutoKeyManager*>(arg);
    // 处理连发
    manager->ProcessAutoKey();
}


// 物理输入读取循环
void AutoKeyManager::ProcessInputReading() {
    // 临时状态
    HidNpadHandheldState temp_state;
    // 当析构函数运行时退出
    while (!m_ShouldExit) {

        // 读取物理输入（完整状态，包括摇杆）
        ReadPhysicalInput(&temp_state);
        
        // 更新共享变量（加锁保护）
        {
            std::lock_guard<std::mutex> lock(m_InputMutex);
            m_SharedPhysicalState = temp_state;
        }
        
        svcSleepThread(UPDATE_INTERVAL_NS);
    }
}

// 连发处理函数
void AutoKeyManager::ProcessAutoKey() {
    
    // 当析构函数运行时退出
    while (!m_ShouldExit) {
        // 劫持并修改控制器状态
        HijackAndModifyState();

        svcSleepThread(UPDATE_INTERVAL_NS);

    }
    
}

// 读取物理输入（直接从HID读取真实手柄状态，绕过HDLS虚拟层）
void AutoKeyManager::ReadPhysicalInput(HidNpadHandheldState* out_state) {
    // 清空输出状态
    memset(out_state, 0, sizeof(HidNpadHandheldState));

    // 先检测No1的SystemExt（第三方最常见在No1） 只检查NO,1
    HidNpadIdType npad_id = HidNpadIdType_No1;
    u32 style_set = hidGetNpadStyleSet(npad_id);
    // 准备通用状态结构（所有State类型都基于HidNpadCommonState）
    HidNpadCommonState common_state;
    size_t count = 0;
    
    if (style_set != 0) {
        if (style_set & HidNpadStyleTag_NpadSystemExt) {
            count = hidGetNpadStatesSystemExt(npad_id, (HidNpadSystemExtState*)&common_state, 1);
            if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
                out_state->buttons = common_state.buttons;
                out_state->analog_stick_l = common_state.analog_stick_l;
                out_state->analog_stick_r = common_state.analog_stick_r;
                out_state->attributes = common_state.attributes;
                return;
            }
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
    
    // 检查剩下的类型
    if (style_set != 0) {
        if (style_set & HidNpadStyleTag_NpadFullKey) {
            count = hidGetNpadStatesFullKey(npad_id, (HidNpadFullKeyState*)&common_state, 1);
            if (count > 0 && (common_state.attributes & HidNpadAttribute_IsConnected)) {
                out_state->buttons = common_state.buttons;
                out_state->analog_stick_l = common_state.analog_stick_l;
                out_state->analog_stick_r = common_state.analog_stick_r;
                out_state->attributes = common_state.attributes;
                return;
            }
        } else if (style_set & HidNpadStyleTag_NpadJoyDual) {
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
            // 只要有一个手柄连接就返回
            if (left_count > 0 || right_count > 0) {
                return;
            }
        }
    }

    // 所有位置都没找到，返回空状态
}

// 获取共享的物理输入（为了线程安全）
void AutoKeyManager::GetSharedPhysicalState(HidNpadHandheldState* out_state) {
    std::lock_guard<std::mutex> lock(m_InputMutex);
    *out_state = m_SharedPhysicalState;
}

// 劫持并修改控制器状态（只对白名单中的按键连发）
void AutoKeyManager::HijackAndModifyState() {
    // 如果HDLS未初始化，则返回
    if (!m_HdlsInitialized) return;
    
    //从共享变量读取物理输入（由独立线程更新，包括摇杆）
    HidNpadHandheldState physical_state;
    GetSharedPhysicalState(&physical_state);
    // 获取物理按键
    u64 physical_buttons = physical_state.buttons;
    
    // 定义掩码：排除摇杆伪按键位 (BIT16-23)
    // 这些位在物理输入中代表摇杆方向，但在HDLS中BIT(18)=Home, BIT(19)=Capture
    // 必须过滤以避免误触发系统按键
    const u64 STICK_PSEUDO_BUTTON_MASK = 0xFF0000ULL;  // BIT(16) ~ BIT(23)
    
    // 过滤出白名单中的按键（允许连发的），同时排除摇杆伪按键位
    u64 autokey_buttons = physical_buttons & m_AutoKeyWhitelistMask & ~STICK_PSEUDO_BUTTON_MASK;
    
    // 其他按键（不连发，直接透传），同时排除摇杆伪按键位
    u64 normal_buttons = physical_buttons & ~m_AutoKeyWhitelistMask & ~STICK_PSEUDO_BUTTON_MASK;

    if (autokey_buttons != 0) {
        // 有白名单按键被按住
        u64 current_time = armGetSystemTick();
        
        // 重置松开计时器（因为有按键，不是松开状态）
        m_AutoKeyReleaseStartTime = 0;
        
        // 初始化：第一次获取设备列表
        if (m_AutoKeyLastSwitchTime == 0) {
            Result rc = hiddbgDumpHdlsStates(m_HdlsSessionId, &m_StateList);
            if (R_FAILED(rc) || m_StateList.total_entries == 0) {
                log_error("获取HDLS设备列表失败: 0x%x", rc);
                return;
            }
            m_AutoKeyLastSwitchTime = current_time;
            m_AutoKeyIsPressed = true;
            log_info("连发开始: 连发键=0x%llx, 普通键=0x%llx", autokey_buttons, normal_buttons);
        }

        // 计算经过的时间
        u64 elapsed_ns = armTicksToNs(current_time - m_AutoKeyLastSwitchTime);
        
        // 检查是否到达切换时间（按下和松开时间不同）
        u64 threshold = m_AutoKeyIsPressed ? m_PressDurationNs : m_ReleaseDurationNs;
        if (elapsed_ns >= threshold) {
            // 切换连发状态
            m_AutoKeyIsPressed = !m_AutoKeyIsPressed;
            m_AutoKeyLastSwitchTime = current_time;
        }
        
        // 构建最终状态：其他按键直接透传 + 白名单按键连发 + 摇杆数据
        for (int i = 0; i < m_StateList.total_entries; i++) {
            memset(&m_StateList.entries[i].state, 0, sizeof(HiddbgHdlsState));
            
            // ★ 复制摇杆数据（保留摇杆输入）
            m_StateList.entries[i].state.analog_stick_l = physical_state.analog_stick_l;
            m_StateList.entries[i].state.analog_stick_r = physical_state.analog_stick_r;
            
            // 合并：普通按键（直接透传）+ 连发按键（按状态）
            if (m_AutoKeyIsPressed) {
                m_StateList.entries[i].state.buttons = normal_buttons | autokey_buttons;
            } else {
                m_StateList.entries[i].state.buttons = normal_buttons;
            }
        }
        
        // 应用状态列表
        Result rc = hiddbgApplyHdlsStateList(m_HdlsSessionId, &m_StateList);
        if (R_FAILED(rc)) {
            log_error("[连发] ApplyHdlsStateList失败: 0x%x", rc);
        }
    } else {
        // 没有白名单按键被按住，进入松开防抖动流程
        if (m_AutoKeyLastSwitchTime != 0) {
            // 之前在连发中
            u64 current_time = armGetSystemTick();
            
            if (m_AutoKeyReleaseStartTime == 0) {
                // 第一次检测到没按键，开始计时
                m_AutoKeyReleaseStartTime = current_time;
                // 不输出日志，不结束连发，继续观察
            } else {
                // 持续没按键，检查持续了多久
                u64 release_duration_ns = armTicksToNs(current_time - m_AutoKeyReleaseStartTime);
                
                if (release_duration_ns >= RELEASE_DEBOUNCE_NS) {
                    // 持续松开超过15ms，确认是真松开
                    log_info("连发结束 (松开持续%llums)", release_duration_ns / 1000000ULL);
                    m_AutoKeyLastSwitchTime = 0;
                    m_AutoKeyIsPressed = false;
                    m_AutoKeyReleaseStartTime = 0;
                }
                // else: 还在15ms内，可能是抖动，保持连发状态，继续观察
            }
        }
    }

}
