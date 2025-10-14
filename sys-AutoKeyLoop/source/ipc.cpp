#include "ipc.hpp"
#include "../log/log.h"
#include <cstring>

// 静态线程栈定义
alignas(0x1000) char IPCServer::ipc_thread_stack[8 * 1024];

// 构造函数
IPCServer::IPCServer() : 
    is_client_connected(false),
    should_exit(false),
    thread_created(false),
    thread_running(false) {
    
    // 初始化句柄数组
    handles[0] = INVALID_HANDLE;
    handles[1] = INVALID_HANDLE;
    
    // 初始化服务名
    memset(&server_name, 0, sizeof(SmServiceName));
    
    // 初始化线程
    memset(&ipc_thread, 0, sizeof(Thread));
}

// 析构函数
IPCServer::~IPCServer() {
    Stop();
}

// 启动IPC服务
bool IPCServer::Start(const char* service_name) {
    
    // 设置服务名称（最多8字符）
    memset(&server_name, 0, sizeof(SmServiceName));
    memcpy(server_name.name, service_name, 
           service_name[7] == '\0' ? 8 : 7);  // 确保不超过8字符
    
    // 创建IPC线程
    Result rc = threadCreate(&ipc_thread, ThreadEntry, this, 
                           ipc_thread_stack, sizeof(ipc_thread_stack), 44, 3);
    if (R_FAILED(rc)) {
        log_error("IPC线程创建失败: 0x%x", rc);
        return false;
    }
    
    thread_created = true;
    
    // 启动线程
    rc = threadStart(&ipc_thread);
    if (R_FAILED(rc)) {
        log_error("IPC线程启动失败: 0x%x", rc);
        return false;
    }
    
    thread_running = true;
    log_info("IPC服务启动成功: %s", service_name);
    return true;
}

// 停止IPC服务
void IPCServer::Stop() {
    if (!thread_created) return;
    
    log_info("准备停止IPC服务");
    should_exit = true;
    
    // 等待线程结束
    if (thread_running) {
        threadWaitForExit(&ipc_thread);
        log_info("IPC线程已停止");
    }
    
    // 关闭线程
    if (thread_created) {
        threadClose(&ipc_thread);
        log_info("IPC线程关闭成功");
    }
}

// 设置退出回调函数
void IPCServer::SetExitCallback(std::function<void()> callback) {
    exit_callback = callback;
}

// 设置开启连发回调函数
void IPCServer::SetEnableCallback(std::function<void()> callback) {
    enable_callback = callback;
}

// 设置关闭连发回调函数
void IPCServer::SetDisableCallback(std::function<void()> callback) {
    disable_callback = callback;
}

// 静态线程入口函数
void IPCServer::ThreadEntry(void* arg) {
    IPCServer* server = static_cast<IPCServer*>(arg);
    server->IpcThreadMain();
}

// 线程主循环
void IPCServer::IpcThreadMain() {
    StartServer();
    
    while (!should_exit) {
        WaitAndProcessRequest();
    }
    
    StopServer();
}

// 启动服务器
void IPCServer::StartServer() {
    
    // ★ 修正：第二个参数传 SmServiceName 值，不是指针
    Result rc = smRegisterService(server_handle, server_name, false, 1);
    if (R_FAILED(rc)) {
        log_error("注册IPC服务失败: 0x%x", rc);
        should_exit = true;
        return;
    }
    log_info("IPC服务注册成功: %s", server_name.name);
}

// 停止服务器
void IPCServer::StopServer() {
    // 关闭客户端连接
    if (is_client_connected && *client_handle != INVALID_HANDLE) {
        svcCloseHandle(*client_handle);
        *client_handle = INVALID_HANDLE;
        is_client_connected = false;
    }
    
    // 关闭服务器句柄并注销服务
    if (*server_handle != INVALID_HANDLE) {
        svcCloseHandle(*server_handle);
        
        Result rc = smUnregisterService(server_name);
        if (R_FAILED(rc)) {
            log_error("注销IPC服务失败: 0x%x", rc);
        }
        
        *server_handle = INVALID_HANDLE;
    }
}

// 等待并处理请求
void IPCServer::WaitAndProcessRequest() {
    s32 index = -1;
    
    // 等待同步（监听服务器句柄和客户端句柄）
    Result rc = svcWaitSynchronization(&index, handles, is_client_connected ? 2 : 1, UINT64_MAX);
    if (R_FAILED(rc)) {
        log_error("等待同步失败: 0x%x", rc);
        should_exit = true;
        return;
    }
    
    if (index == 0) {
        // 接受新的客户端会话
        Handle new_client;
        rc = svcAcceptSession(&new_client, *server_handle);
        if (R_FAILED(rc)) {
            log_error("接受会话失败: 0x%x", rc);
            return;
        }
        
        // 如果已经有客户端连接，关闭新连接（最大客户端数限制）
        if (is_client_connected) {
            svcCloseHandle(new_client);
            log_warning("已有客户端连接，拒绝新连接");
            return;
        }
        
        is_client_connected = true;
        *client_handle = new_client;
        log_info("客户端连接成功");
        
    } else if (index == 1) {
        // 处理客户端消息
        if (!is_client_connected) {
            log_error("客户端未连接但收到消息");
            should_exit = true;
            return;
        }
        
        s32 _idx;
        rc = svcReplyAndReceive(&_idx, client_handle, 1, 0, UINT64_MAX);
        if (R_FAILED(rc)) {
            log_error("接收消息失败: 0x%x", rc);
            return;
        }
        
        bool should_close = false;
        CommandResult cmd_result = {false, false, false, false};
        Request request = ParseRequestFromTLS();
        
        switch (request.type) {
            case CmifCommandType_Request:
                // 处理命令并获取结果
                cmd_result = HandleCommand(request.cmd_id);
                should_close = cmd_result.should_close_connection;
                break;
            case CmifCommandType_Close:
                WriteResponseToTLS(0);
                should_close = true;
                break;
            default:
                WriteResponseToTLS(1);
                break;
        }
        
        // ✅ 先发送响应（此时服务器处于正常运行状态）
        rc = svcReplyAndReceive(&_idx, client_handle, 0, *client_handle, 0);
        
        if (rc != KERNELRESULT(TimedOut)) {
            if (R_FAILED(rc)) {
                log_error("发送响应失败: 0x%x", rc);
            }
        }
        
        // 然后关闭连接
        if (should_close) {
            svcCloseHandle(*client_handle);
            *client_handle = INVALID_HANDLE;
            is_client_connected = false;
            log_info("客户端连接关闭");
        }
        
        // 最后才执行回调逻辑（确保响应已成功发送）
        
        // 开启连发回调
        if (cmd_result.should_enable_autokey) {
            if (enable_callback) {
                enable_callback();
            }
        }
        
        // 关闭连发回调
        if (cmd_result.should_disable_autokey) {
            if (disable_callback) {
                disable_callback();
            }
        }
        
        // 退出服务器回调
        if (cmd_result.should_exit_server) {
            should_exit = true;
            if (exit_callback) {
                exit_callback();
            }
        }
    }
}

// 处理命令 - 完整处理命令逻辑，但不直接修改服务器状态
CommandResult IPCServer::HandleCommand(u64 cmd_id) {
    CommandResult result = {false, false, false, false};
    
    switch (cmd_id) {
        case CMD_ENABLE_AUTOKEY:
            log_info("收到开启连发命令");
            WriteResponseToTLS(0);  // 写入成功响应
            // ✅ 通过返回值告诉调用者需要在响应发送后执行回调
            result.should_enable_autokey = true;
            break;
            
        case CMD_DISABLE_AUTOKEY:
            log_info("收到关闭连发命令");
            WriteResponseToTLS(0);  // 写入成功响应
            // ✅ 通过返回值告诉调用者需要在响应发送后执行回调
            result.should_disable_autokey = true;
            break;
            
        case CMD_EXIT:
            log_info("收到退出命令");
            // ✅ 写入成功响应
            WriteResponseToTLS(0);
            // ✅ 通过返回值告诉调用者需要做什么（而不是直接修改状态）
            result.should_close_connection = true;  // 需要关闭客户端连接
            result.should_exit_server = true;       // 需要退出服务器
            break;
            
        default:
            log_warning("未知命令: %llu", cmd_id);
            WriteResponseToTLS(1);
            // 默认值 {false, false, false, false}，不做任何额外操作
            break;
    }
    
    return result;
}

// 解析TLS中的请求
IPCServer::Request IPCServer::ParseRequestFromTLS() {
    Request req = {0};
    
    void* base = armGetTls();
    HipcParsedRequest hipc = hipcParseRequest(base);
    
    req.type = hipc.meta.type;
    
    if (hipc.meta.type == CmifCommandType_Request) {
        CmifInHeader* header = (CmifInHeader*)cmifGetAlignedDataStart(hipc.data.data_words, base);
        size_t data_size = hipc.meta.num_data_words * 4;
        
        if (!header) {
            log_error("无效的请求头");
            return req;
        }
        if (data_size < sizeof(CmifInHeader)) {
            log_error("数据大小不足");
            return req;
        }
        if (header->magic != CMIF_IN_HEADER_MAGIC) {
            log_error("请求头魔数错误");
            return req;
        }
        
        req.cmd_id = header->command_id;
        req.data_size = data_size - sizeof(CmifInHeader);
        req.data = req.data_size ? ((u8*)header) + sizeof(CmifInHeader) : NULL;
    }
    
    return req;
}

// 写响应到TLS
void IPCServer::WriteResponseToTLS(Result rc) {
    HipcMetadata meta = {0};
    meta.type = CmifCommandType_Request;
    meta.num_data_words = (sizeof(CmifOutHeader) + 0x10) / 4;
    
    void* base = armGetTls();
    HipcRequest hipc = hipcMakeRequest(base, meta);
    CmifOutHeader* raw_header = (CmifOutHeader*)cmifGetAlignedDataStart(hipc.data_words, base);
    
    raw_header->magic = CMIF_OUT_HEADER_MAGIC;
    raw_header->result = rc;
    raw_header->token = 0;
}