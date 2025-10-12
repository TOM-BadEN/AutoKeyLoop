#include "autokeymanager.hpp"
#include <cstring>
#include "../log/log.h"

// 静态线程栈定义
alignas(0x1000) char AutoKeyManager::autokey_thread_stack[4 * 1024];
alignas(0x1000) char AutoKeyManager::input_reader_thread_stack[4 * 1024];

// 静态HDLS工作缓冲区定义
alignas(0x1000) u8 AutoKeyManager::hdls_work_buffer[0x1000];

// 构造函数
AutoKeyManager::AutoKeyManager() {

    // 初始化HDLS工作缓冲区（核心，需要这个来虚拟输入）
    Result rc = hiddbgAttachHdlsWorkBuffer(&hdls_session_id, hdls_work_buffer, sizeof(hdls_work_buffer));
    if (R_FAILED(rc)) {
        log_error("HDLS工作缓冲区初始化失败: 0x%x", rc);
        return;
    }
    
    // 初始化状态列表
    memset(&state_list, 0, sizeof(state_list));
    
    hdls_initialized = true;
    
    // 初始化连发参数
    should_exit = false;
    autokey_is_pressed = false;
    autokey_last_switch_time = 0;
    memset(&shared_physical_state, 0, sizeof(shared_physical_state));
    
    // 初始化连发按键池（白名单）- 默认允许 ABXY + LR + ZLZR
    autokey_whitelist_mask = 
        HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y |
        HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR;
    
    log_info("连发按键白名单: A B X Y L R ZL ZR (掩码=0x%llx)", autokey_whitelist_mask);
    
    // 初始化线程状态
    thread_created = false;
    thread_running = false;
    input_thread_created = false;
    input_thread_running = false;
    memset(&autokey_thread, 0, sizeof(Thread));
    memset(&input_reader_thread, 0, sizeof(Thread));
    
    // 创建物理输入读取线程
    rc = threadCreate(&input_reader_thread, input_reader_thread_func, this,
                     input_reader_thread_stack, sizeof(input_reader_thread_stack), 44, 3);
    
    if (R_SUCCEEDED(rc)) {
        input_thread_created = true;
        if (R_SUCCEEDED(threadStart(&input_reader_thread))) {
            input_thread_running = true;
            log_info("输入读取线程启动成功");
        } else {
            log_error("输入读取线程启动失败");
        }
    } else {
        log_error("输入读取线程创建失败: 0x%x", rc);
    }
    
    // 创建连发线程
    rc = threadCreate(&autokey_thread, autokey_thread_func, this, 
                     autokey_thread_stack, sizeof(autokey_thread_stack), 44, 3);
    
    if (R_SUCCEEDED(rc)) {
        thread_created = true;
        if (R_SUCCEEDED(threadStart(&autokey_thread))) {
            thread_running = true;
            log_info("连发线程启动成功");
        } else {
            log_error("连发线程启动失败");
        }
    } else {
        log_error("连发线程创建失败: 0x%x", rc);
    }
}

// 析构函数
AutoKeyManager::~AutoKeyManager() {
    // 停止所有线程
    should_exit = true;
    
    if (input_thread_running) {
        threadWaitForExit(&input_reader_thread);
        log_info("输入读取线程已停止");
    }
    
    if (input_thread_created) {
        threadClose(&input_reader_thread);
        log_info("输入读取线程关闭成功");
    }
    
    if (thread_running) {
        threadWaitForExit(&autokey_thread);
        log_info("连发线程已停止");
    }
    
    if (thread_created) {
        threadClose(&autokey_thread);
        log_info("连发线程关闭成功");
    }
    
    // 释放HDLS工作缓冲区
    if (hdls_initialized) {
        Result rc = hiddbgReleaseHdlsWorkBuffer(hdls_session_id);
        if (R_FAILED(rc)) {
            log_error("HDLS工作缓冲区释放失败: 0x%x", rc);
        } else {
            log_info("HDLS工作缓冲区释放成功");
        }
    }
}

// 物理输入读取线程函数
void AutoKeyManager::input_reader_thread_func(void* arg) {
    AutoKeyManager* manager = static_cast<AutoKeyManager*>(arg);
    manager->ProcessInputReading();
}

// 连发线程函数
void AutoKeyManager::autokey_thread_func(void* arg) {
    AutoKeyManager* manager = static_cast<AutoKeyManager*>(arg);
    manager->ProcessAutoKey();
}

// 物理输入读取循环
void AutoKeyManager::ProcessInputReading() {
    log_info("输入读取线程开始运行");
    
    HidNpadHandheldState temp_state;
    
    while (!should_exit) {
        // 读取物理输入（完整状态，包括摇杆）
        ReadPhysicalInput(&temp_state);
        
        // 更新共享变量（加锁保护）
        {
            std::lock_guard<std::mutex> lock(input_mutex);
            shared_physical_state = temp_state;
        }
        
        // 每5ms读取一次（比连发线程慢，避免过度轮询）
        svcSleepThread(5000000ULL);
    }
    
    log_info("输入读取线程结束运行");
}

// 连发处理函数
void AutoKeyManager::ProcessAutoKey() {
    log_info("连发线程开始运行");
    
    while (!should_exit) {
        HijackAndModifyState();
        svcSleepThread(UPDATE_INTERVAL_NS);
    }
    
    log_info("连发线程结束运行");
}

// 读取物理输入（从HID API读取，不通过HDLS）
void AutoKeyManager::ReadPhysicalInput(HidNpadHandheldState* out_state) {
    memset(out_state, 0, sizeof(HidNpadHandheldState));
    
    // 尝试读取掌机模式
    HidNpadHandheldState handheld_states[8];
    size_t count = hidGetNpadStatesHandheld(HidNpadIdType_Handheld, handheld_states, 8);
    
    if (count > 0) {
        *out_state = handheld_states[count - 1];
        return;
    }
    
    // 尝试读取Pro手柄（需要转换为HidNpadHandheldState格式）
    HidNpadFullKeyState fullkey_states[8];
    count = hidGetNpadStatesFullKey(HidNpadIdType_No1, fullkey_states, 8);
    
    if (count > 0) {
        // 复制按键和摇杆数据
        out_state->buttons = fullkey_states[count - 1].buttons;
        out_state->analog_stick_l = fullkey_states[count - 1].analog_stick_l;
        out_state->analog_stick_r = fullkey_states[count - 1].analog_stick_r;
    }
}

// 获取共享的物理输入（线程安全）
void AutoKeyManager::GetSharedPhysicalState(HidNpadHandheldState* out_state) {
    std::lock_guard<std::mutex> lock(input_mutex);
    *out_state = shared_physical_state;
}

// 劫持并修改控制器状态（只对白名单中的按键连发）
void AutoKeyManager::HijackAndModifyState() {
    if (!hdls_initialized) return;
    
    // ★ 从共享变量读取物理输入（由独立线程更新，包括摇杆）
    HidNpadHandheldState physical_state;
    GetSharedPhysicalState(&physical_state);
    
    u64 physical_buttons = physical_state.buttons;
    
    // ★ 过滤出白名单中的按键（允许连发的）
    u64 autokey_buttons = physical_buttons & autokey_whitelist_mask;
    
    // ★ 其他按键（不连发，直接透传）
    u64 normal_buttons = physical_buttons & ~autokey_whitelist_mask;
    
    if (autokey_buttons != 0) {
        // 有白名单按键被按住
        u64 current_time = armGetSystemTick();
        
        // 初始化：第一次获取设备列表
        if (autokey_last_switch_time == 0) {
            Result rc = hiddbgDumpHdlsStates(hdls_session_id, &state_list);
            if (R_FAILED(rc) || state_list.total_entries == 0) {
                log_error("获取HDLS设备列表失败: 0x%x", rc);
                return;
            }
            autokey_last_switch_time = current_time;
            autokey_is_pressed = true;
            log_info("连发开始: 连发键=0x%llx, 普通键=0x%llx", autokey_buttons, normal_buttons);
        }
        
        // 计算经过的时间
        u64 elapsed_ns = armTicksToNs(current_time - autokey_last_switch_time);
        
        // 检查是否到达切换时间（100ms）
        if (elapsed_ns >= FIRE_INTERVAL_NS) {
            // 切换连发状态
            autokey_is_pressed = !autokey_is_pressed;
            autokey_last_switch_time = current_time;
        }
        
        // 构建最终状态：其他按键直接透传 + 白名单按键连发 + 摇杆数据
        for (int i = 0; i < state_list.total_entries; i++) {
            memset(&state_list.entries[i].state, 0, sizeof(HiddbgHdlsState));
            
            // ★ 复制摇杆数据（保留摇杆输入）
            state_list.entries[i].state.analog_stick_l = physical_state.analog_stick_l;
            state_list.entries[i].state.analog_stick_r = physical_state.analog_stick_r;
            
            // 合并：普通按键（直接透传）+ 连发按键（按状态）
            if (autokey_is_pressed) {
                state_list.entries[i].state.buttons = normal_buttons | autokey_buttons;
            } else {
                state_list.entries[i].state.buttons = normal_buttons;
            }
        }
        
        // 应用状态列表
        Result rc = hiddbgApplyHdlsStateList(hdls_session_id, &state_list);
        if (R_FAILED(rc)) {
            log_error("[连发] ApplyHdlsStateList失败: 0x%x", rc);
        }
    } else {
        // 没有白名单按键被按住，重置连发状态
        if (autokey_last_switch_time != 0) {
            log_info("连发结束");
            autokey_last_switch_time = 0;
            autokey_is_pressed = false;
        }
    }
}
