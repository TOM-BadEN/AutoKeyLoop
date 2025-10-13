#pragma once
#include <switch.h>
#include <functional>

// IPC命令定义
#define CMD_EXIT 999

// IPC服务器类
class IPCServer {
private:
    // 句柄和服务名
    Handle handles[2];
    SmServiceName server_name;
    Handle* const server_handle = &handles[0];
    Handle* const client_handle = &handles[1];
    
    // 状态变量
    bool is_client_connected = false;
    volatile bool should_exit = false;
    
    // 线程相关
    Thread ipc_thread;
    alignas(0x1000) static char ipc_thread_stack[8 * 1024];  // 8KB线程栈
    bool thread_created = false;
    bool thread_running = false;
    
    // 退出回调函数
    std::function<void()> exit_callback;
    
    // 内部方法
    void StartServer();
    void StopServer();
    void WaitAndProcessRequest();
    bool HandleCommand(u64 cmd_id);
    
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
    bool ShouldExit() const { return should_exit; }
};