#pragma once
#include <switch.h>
#include <functional>

// IPC命令定义
// 连发控制
#define CMD_ENABLE_AUTOFIRE   1   // 开启连发
#define CMD_DISABLE_AUTOFIRE  2   // 关闭连发

// 映射控制
#define CMD_ENABLE_MAPPING    3   // 开启映射
#define CMD_DISABLE_MAPPING   4   // 关闭映射

// 配置重载
#define CMD_RELOAD_BASIC      5   // 重载基础配置
#define CMD_RELOAD_AUTOFIRE   6   // 重载连发配置
#define CMD_RELOAD_MAPPING    7   // 重载映射配置
#define CMD_RELOAD_WHITELIST  11  // 重载白名单

// 宏控制
#define CMD_ENABLE_MACRO      8   // 开启宏
#define CMD_DISABLE_MACRO     9   // 关闭宏
#define CMD_RELOAD_MACRO      10  // 重载宏配置

// 系统控制
#define CMD_EXIT              999 // 退出系统模块

// IPC命令处理结果
struct CommandResult {
    bool should_close_connection;   // 是否需要关闭客户端连接
    bool should_exit_server;        // 是否需要退出服务器（在响应发送后）
    bool should_enable_autofire;    // 是否需要开启连发（在响应发送后）
    bool should_disable_autofire;   // 是否需要关闭连发（在响应发送后）
    bool should_enable_mapping;     // 是否需要开启映射（在响应发送后）
    bool should_disable_mapping;    // 是否需要关闭映射（在响应发送后）
    bool should_reload_basic;       // 是否需要重载基础配置（在响应发送后）
    bool should_reload_autofire;    // 是否需要重载连发配置（在响应发送后）
    bool should_reload_mapping;     // 是否需要重载映射配置（在响应发送后）
    bool should_enable_macro;       // 是否需要开启宏（在响应发送后）
    bool should_disable_macro;      // 是否需要关闭宏（在响应发送后）
    bool should_reload_macro;       // 是否需要重载宏配置（在响应发送后）
    bool should_reload_whitelist;   // 是否需要重载白名单（在响应发送后）
};

// IPC服务器类
class IPCServer {
private:
    // 句柄和服务名
    Handle m_Handles[2];
    SmServiceName m_ServerName;
    Handle* const m_ServerHandle = &m_Handles[0];
    Handle* const m_ClientHandle = &m_Handles[1];
    
    // 状态变量
    bool m_IsClientConnected = false;
    volatile bool m_ShouldExit = false;
    
    // 线程相关
    Thread m_IpcThread;
    alignas(0x1000) static char ipc_thread_stack[8 * 1024];  // 4KB线程栈
    bool m_ThreadCreated = false;
    bool m_ThreadRunning = false;
    
    // 回调函数
    std::function<void()> m_ExitCallback;             // 退出回调
    std::function<void()> m_EnableAutoFireCallback;   // 开启连发回调
    std::function<void()> m_DisableAutoFireCallback;  // 关闭连发回调
    std::function<void()> m_EnableMappingCallback;    // 开启映射回调
    std::function<void()> m_DisableMappingCallback;   // 关闭映射回调
    std::function<void()> m_ReloadBasicCallback;      // 重载基础配置回调
    std::function<void()> m_ReloadAutoFireCallback;   // 重载连发配置回调
    std::function<void()> m_ReloadMappingCallback;    // 重载映射配置回调
    std::function<void()> m_EnableMacroCallback;      // 开启宏回调
    std::function<void()> m_DisableMacroCallback;     // 关闭宏回调
    std::function<void()> m_ReloadMacroCallback;      // 重载宏配置回调
    std::function<void()> m_ReloadWhitelistCallback;  // 重载白名单回调
    
    // 内部方法
    void StartServer();
    void StopServer();
    void WaitAndProcessRequest();
    CommandResult HandleCommand(u64 cmd_id);
    
    // 请求解析和响应
    struct Request {
        u32 type;
        u64 cmd_id;
        void* data;
        u32 data_size;
    };
    
    Request ParseRequestFromTLS();
    void WriteResponseToTLS(Result rc);
    
    // 静态线程入口函数
    static void ThreadEntry(void* arg);
    
    // 线程主循环
    void IpcThreadMain();
    
public:
    // 构造函数和析构函数
    IPCServer();
    ~IPCServer();
    
    // 主要接口
    bool Start(const char* service_name);
    void Stop();
    void SetExitCallback(std::function<void()> callback);
    void SetEnableAutoFireCallback(std::function<void()> callback);
    void SetDisableAutoFireCallback(std::function<void()> callback);
    void SetEnableMappingCallback(std::function<void()> callback);
    void SetDisableMappingCallback(std::function<void()> callback);
    void SetReloadBasicCallback(std::function<void()> callback);
    void SetReloadAutoFireCallback(std::function<void()> callback);
    void SetReloadMappingCallback(std::function<void()> callback);
    void SetEnableMacroCallback(std::function<void()> callback);
    void SetDisableMacroCallback(std::function<void()> callback);
    void SetReloadMacroCallback(std::function<void()> callback);
    void SetReloadWhitelistCallback(std::function<void()> callback);
    bool ShouldExit() const { return m_ShouldExit; }
};