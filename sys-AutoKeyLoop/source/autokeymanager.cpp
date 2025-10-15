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
    log_info("输入读取线程开始运行");
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
        
        // 每5ms读取一次（比连发线程慢，避免过度轮询）
        svcSleepThread(5000000ULL);
    }
    log_info("输入读取线程函数结束运行");
}

// 连发处理函数
void AutoKeyManager::ProcessAutoKey() {
    log_info("连发线程开始运行");
    
    // 当析构函数运行时退出
    while (!m_ShouldExit) {
        // 劫持并修改控制器状态
        HijackAndModifyState();

        svcSleepThread(UPDATE_INTERVAL_NS);

    }
    
    log_info("连发线程函数结束运行");
}

// 读取物理输入（直接从HID读取真实手柄状态，绕过HDLS虚拟层）
void AutoKeyManager::ReadPhysicalInput(HidNpadHandheldState* out_state) {
    // 清空输出状态
    memset(out_state, 0, sizeof(HidNpadHandheldState));
    
    // 尝试读取掌机模式的状态历史（最多8个，最新的在末尾）
    HidNpadHandheldState handheld_states[8];
    size_t count = hidGetNpadStatesHandheld(HidNpadIdType_Handheld, handheld_states, 8);
    
    // 如果掌机模式有数据，取最新状态返回
    if (count > 0) {
        *out_state = handheld_states[count - 1];  // 最新状态在数组末尾
        return;
    }
    
    // 掌机模式无数据，尝试Pro手柄/分离模式（Fallback）
    HidNpadFullKeyState fullkey_states[8];
    count = hidGetNpadStatesFullKey(HidNpadIdType_No1, fullkey_states, 8);
    
    // 如果Pro手柄有数据，转换为掌机格式
    if (count > 0) {
        // 只复制兼容的字段（按键+摇杆）
        out_state->buttons = fullkey_states[count - 1].buttons;
        out_state->analog_stick_l = fullkey_states[count - 1].analog_stick_l;
        out_state->analog_stick_r = fullkey_states[count - 1].analog_stick_r;
    }
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
    
    // 过滤出白名单中的按键（允许连发的）
    u64 autokey_buttons = physical_buttons & m_AutoKeyWhitelistMask;
    
    // 其他按键（不连发，直接透传）
    u64 normal_buttons = physical_buttons & ~m_AutoKeyWhitelistMask;

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
