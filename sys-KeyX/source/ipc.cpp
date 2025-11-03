#include "ipc.hpp"
#include <cstring>

// 静态线程栈定义
alignas(0x1000) char IPCServer::ipc_thread_stack[4 * 1024];

// 构造函数
IPCServer::IPCServer() : 
    m_IsClientConnected(false),
    m_ShouldExit(false),
    m_ThreadCreated(false),
    m_ThreadRunning(false) {
    
    // 初始化句柄数组
    m_Handles[0] = INVALID_HANDLE;
    m_Handles[1] = INVALID_HANDLE;
    
    // 初始化服务名
    memset(&m_ServerName, 0, sizeof(SmServiceName));
    
    // 初始化线程
    memset(&m_IpcThread, 0, sizeof(Thread));
}

// 析构函数
IPCServer::~IPCServer() {
    Stop();
}

// 启动IPC服务
bool IPCServer::Start(const char* service_name) {
    
    // 设置服务名称（最多8字符）
    memset(&m_ServerName, 0, sizeof(SmServiceName));
    memcpy(m_ServerName.name, service_name, 
           service_name[7] == '\0' ? 8 : 7);  // 确保不超过8字符
    
    // 创建IPC线程（-2表示自适应核心分配）
    Result rc = threadCreate(&m_IpcThread, ThreadEntry, this, 
                           ipc_thread_stack, sizeof(ipc_thread_stack), 44, -2);
    if (R_FAILED(rc)) {
        return false;
    }
    
    m_ThreadCreated = true;
    
    // 启动线程
    rc = threadStart(&m_IpcThread);
    if (R_FAILED(rc)) {
        return false;
    }
    
    m_ThreadRunning = true;
    return true;
}

// 停止IPC服务
void IPCServer::Stop() {
    if (!m_ThreadCreated) return;
    
    m_ShouldExit = true;
    
    // 等待线程结束
    if (m_ThreadRunning) {
        threadWaitForExit(&m_IpcThread);
    }
    
    // 关闭线程
    if (m_ThreadCreated) {
        threadClose(&m_IpcThread);
    }
}

// 设置退出回调函数
void IPCServer::SetExitCallback(std::function<void()> callback) {
    m_ExitCallback = callback;
}

// 设置开启连发回调函数
void IPCServer::SetEnableAutoFireCallback(std::function<void()> callback) {
    m_EnableAutoFireCallback = callback;
}

// 设置关闭连发回调函数
void IPCServer::SetDisableAutoFireCallback(std::function<void()> callback) {
    m_DisableAutoFireCallback = callback;
}

// 设置开启映射回调函数
void IPCServer::SetEnableMappingCallback(std::function<void()> callback) {
    m_EnableMappingCallback = callback;
}

// 设置关闭映射回调函数
void IPCServer::SetDisableMappingCallback(std::function<void()> callback) {
    m_DisableMappingCallback = callback;
}

// 设置重载基础配置回调函数
void IPCServer::SetReloadBasicCallback(std::function<void()> callback) {
    m_ReloadBasicCallback = callback;
}

// 设置重载连发配置回调函数
void IPCServer::SetReloadAutoFireCallback(std::function<void()> callback) {
    m_ReloadAutoFireCallback = callback;
}

// 设置重载映射配置回调函数
void IPCServer::SetReloadMappingCallback(std::function<void()> callback) {
    m_ReloadMappingCallback = callback;
}

// 静态线程入口函数
void IPCServer::ThreadEntry(void* arg) {
    IPCServer* server = static_cast<IPCServer*>(arg);
    server->IpcThreadMain();
}

// 线程主循环
void IPCServer::IpcThreadMain() {
    StartServer();
    
    while (!m_ShouldExit) {
        WaitAndProcessRequest();
    }
    
    StopServer();
}

// 启动服务器
void IPCServer::StartServer() {
    
    // ★ 修正：第二个参数传 SmServiceName 值，不是指针
    Result rc = smRegisterService(m_ServerHandle, m_ServerName, false, 1);
    if (R_FAILED(rc)) {
        m_ShouldExit = true;
        return;
    }
}

// 停止服务器
void IPCServer::StopServer() {
    // 关闭客户端连接
    if (m_IsClientConnected && *m_ClientHandle != INVALID_HANDLE) {
        svcCloseHandle(*m_ClientHandle);
        *m_ClientHandle = INVALID_HANDLE;
        m_IsClientConnected = false;
    }
    
    // 关闭服务器句柄并注销服务
    if (*m_ServerHandle != INVALID_HANDLE) {
        svcCloseHandle(*m_ServerHandle);
        smUnregisterService(m_ServerName);
        *m_ServerHandle = INVALID_HANDLE;
    }
}

// 等待并处理请求
void IPCServer::WaitAndProcessRequest() {
    s32 index = -1;
    
    // 等待同步（监听服务器句柄和客户端句柄）
    Result rc = svcWaitSynchronization(&index, m_Handles, m_IsClientConnected ? 2 : 1, UINT64_MAX);
    if (R_FAILED(rc)) {
        m_ShouldExit = true;
        return;
    }
    
    if (index == 0) {
        // 接受新的客户端会话
        Handle new_client;
        rc = svcAcceptSession(&new_client, *m_ServerHandle);
        if (R_FAILED(rc)) {
            return;
        }
        
        // 如果已经有客户端连接，关闭新连接（最大客户端数限制）
        if (m_IsClientConnected) {
            svcCloseHandle(new_client);
            return;
        }
        
        m_IsClientConnected = true;
        *m_ClientHandle = new_client;
        
    } else if (index == 1) {
        // 处理客户端消息
        if (!m_IsClientConnected) {
            m_ShouldExit = true;
            return;
        }
        
        s32 _idx;
        rc = svcReplyAndReceive(&_idx, m_ClientHandle, 1, 0, UINT64_MAX);
        if (R_FAILED(rc)) {
            return;
        }
        
        bool should_close = false;
        CommandResult cmd_result = {false, false, false, false, false, false, false, false, false};
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
        rc = svcReplyAndReceive(&_idx, m_ClientHandle, 0, *m_ClientHandle, 0);
        
        // 然后关闭连接
        if (should_close) {
            svcCloseHandle(*m_ClientHandle);
            *m_ClientHandle = INVALID_HANDLE;
            m_IsClientConnected = false;
        }
        
        // 最后才执行回调逻辑（确保响应已成功发送）
        
        // 开启连发回调
        if (cmd_result.should_enable_autofire) {
            if (m_EnableAutoFireCallback) m_EnableAutoFireCallback();
        }
        
        // 关闭连发回调
        if (cmd_result.should_disable_autofire) {
            if (m_DisableAutoFireCallback) m_DisableAutoFireCallback();
        }
        
        // 开启映射回调
        if (cmd_result.should_enable_mapping) {
            if (m_EnableMappingCallback) m_EnableMappingCallback();
        }
        
        // 关闭映射回调
        if (cmd_result.should_disable_mapping) {
            if (m_DisableMappingCallback) m_DisableMappingCallback();
        }
        
        // 重载基础配置回调
        if (cmd_result.should_reload_basic) {
            if (m_ReloadBasicCallback) m_ReloadBasicCallback();
        }
        
        // 重载连发配置回调
        if (cmd_result.should_reload_autofire) {
            if (m_ReloadAutoFireCallback) m_ReloadAutoFireCallback();
        }
        
        // 重载映射配置回调
        if (cmd_result.should_reload_mapping) {
            if (m_ReloadMappingCallback) m_ReloadMappingCallback();
        }
        
        // 退出服务器回调
        if (cmd_result.should_exit_server) {
            m_ShouldExit = true;
            if (m_ExitCallback) m_ExitCallback();
        }
    }
}

// 处理命令 - 完整处理命令逻辑，但不直接修改服务器状态
CommandResult IPCServer::HandleCommand(u64 cmd_id) {
    CommandResult result = {false, false, false, false, false, false, false, false, false};
    
    switch (cmd_id) {
        case CMD_ENABLE_AUTOFIRE:
            WriteResponseToTLS(0);
            result.should_enable_autofire = true;
            break;
            
        case CMD_DISABLE_AUTOFIRE:
            WriteResponseToTLS(0);
            result.should_disable_autofire = true;
            break;
            
        case CMD_ENABLE_MAPPING:
            WriteResponseToTLS(0);
            result.should_enable_mapping = true;
            break;
            
        case CMD_DISABLE_MAPPING:
            WriteResponseToTLS(0);
            result.should_disable_mapping = true;
            break;
            
        case CMD_RELOAD_BASIC:
            WriteResponseToTLS(0);
            result.should_reload_basic = true;
            break;
            
        case CMD_RELOAD_AUTOFIRE:
            WriteResponseToTLS(0);
            result.should_reload_autofire = true;
            break;
            
        case CMD_RELOAD_MAPPING:
            WriteResponseToTLS(0);
            result.should_reload_mapping = true;
            break;
            
        case CMD_EXIT:
            WriteResponseToTLS(0);
            result.should_close_connection = true;
            result.should_exit_server = true;
            break;
            
        default:
            WriteResponseToTLS(1);
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
            return req;
        }
        if (data_size < sizeof(CmifInHeader)) {
            return req;
        }
        if (header->magic != CMIF_IN_HEADER_MAGIC) {
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