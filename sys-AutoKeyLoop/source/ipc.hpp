#pragma once
#include <switch.h>
#include <functional>

// IPC命令定义
#define CMD_ENABLE_AUTOKEY  1   // 开启连发模块
#define CMD_DISABLE_AUTOKEY 2   // 关闭连发模块
#define CMD_RELOAD_CONFIG   3   // 重载配置
#define CMD_EXIT            999 // 退出系统模块

// IPC命令处理结果
struct CommandResult {
    bool should_close_connection;   // 是否需要关闭客户端连接
    bool should_exit_server;        // 是否需要退出服务器（在响应发送后）
    bool should_enable_autokey;     // 是否需要开启连发（在响应发送后）
    bool should_disable_autokey;    // 是否需要关闭连发（在响应发送后）
    bool should_reload_config;      // 是否需要重载配置（在响应发送后）
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
    alignas(0x1000) static char ipc_thread_stack[8 * 1024];  // 8KB线程栈
    bool m_ThreadCreated = false;
    bool m_ThreadRunning = false;
    
    // 回调函数
    std::function<void()> m_ExitCallback;        // 退出回调
    std::function<void()> m_EnableCallback;      // 开启连发回调
    std::function<void()> m_DisableCallback;     // 关闭连发回调
    std::function<void()> m_ReloadConfigCallback; // 重载配置回调
    
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
    void SetEnableCallback(std::function<void()> callback);
    void SetDisableCallback(std::function<void()> callback);
    void SetReloadConfigCallback(std::function<void()> callback);
    bool ShouldExit() const { return m_ShouldExit; }
};