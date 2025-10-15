#include "ipc_manager.hpp"

// 全局实例定义 - 程序启动时创建，退出时自动析构
IPCManager g_ipcManager;

IPCManager::IPCManager() : m_service{0}, m_connected(false) {
    // 构造函数不自动连接，按需连接
    // 这样可以避免系统模块未启动时的错误
}

IPCManager::~IPCManager() {
    // 析构函数自动清理资源
    // 程序退出时会自动调用，无需手动管理
    disconnect();
}

Result IPCManager::connect() {
    // 如果已经连接，直接返回成功
    if (m_connected) {
        return 0;
    }
    
    // 尝试连接到系统模块的服务
    Result rc = smGetService(&m_service, SERVICE_NAME);
    if (R_SUCCEEDED(rc)) {
        m_connected = true;
    }
    
    return rc;
}

void IPCManager::disconnect() {
    // 如果已连接，则关闭服务句柄
    if (m_connected) {
        serviceClose(&m_service);
        m_service = {0};
        m_connected = false;
    }
}

bool IPCManager::isConnected() const {
    return m_connected;
}

Result IPCManager::sendEnableCommand() {
    // 如果未连接，先尝试连接
    if (!m_connected) {
        Result rc = connect();
        if (R_FAILED(rc)) {
            return rc;  // 连接失败，可能系统模块未运行
        }
    }
    
    // 发送开启连发命令给系统模块
    Result rc = serviceDispatch(&m_service, CMD_ENABLE_AUTOKEY);
    
    // 发送完成后断开连接
    disconnect();
    
    return rc;
}

Result IPCManager::sendDisableCommand() {
    // 如果未连接，先尝试连接
    if (!m_connected) {
        Result rc = connect();
        if (R_FAILED(rc)) {
            return rc;  // 连接失败，可能系统模块未运行
        }
    }
    
    // 发送关闭连发命令给系统模块
    Result rc = serviceDispatch(&m_service, CMD_DISABLE_AUTOKEY);
    
    // 发送完成后断开连接
    disconnect();
    
    return rc;
}

Result IPCManager::sendExitCommand() {
    // 如果未连接，先尝试连接
    if (!m_connected) {
        Result rc = connect();
        if (R_FAILED(rc)) {
            return rc;  // 连接失败，可能系统模块未运行
        }
    }
    
    // 发送退出命令给系统模块
    Result rc = serviceDispatch(&m_service, CMD_EXIT);
    
    // 发送成功后断开连接（系统模块即将退出）
    disconnect();
    
    return rc;
}

Result IPCManager::sendRestartCommand() {
    // 发送关闭命令
    Result rc = sendDisableCommand();
    if (R_FAILED(rc)) {
        return rc;  // 关闭失败，返回错误
    }
    
    // 等待50ms，让系统模块完成清理
    svcSleepThread(50000000ULL);  // 50ms
    
    // 发送开启命令
    rc = sendEnableCommand();
    
    return rc;
}

Result IPCManager::sendReloadConfigCommand() {
    // 如果未连接，先尝试连接
    if (!m_connected) {
        Result rc = connect();
        if (R_FAILED(rc)) {
            return rc;  // 连接失败，可能系统模块未运行
        }
    }
    
    // 发送重载配置命令给系统模块
    Result rc = serviceDispatch(&m_service, CMD_RELOAD_CONFIG);
    
    // 发送完成后断开连接
    disconnect();
    
    return rc;
}

