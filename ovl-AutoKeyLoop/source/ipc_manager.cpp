#include "ipc_manager.hpp"

// 全局实例定义 - 程序启动时创建，退出时自动析构
IPCManager g_ipcManager;

IPCManager::IPCManager() : m_service{0}, m_connected(false) {
}

IPCManager::~IPCManager() {
    disconnect();
}

Result IPCManager::connect() {
    // 如果已经连接，直接返回成功
    if (m_connected) return 0;
    // 尝试连接到系统模块的服务
    Result rc = smGetService(&m_service, SERVICE_NAME);
    if (R_SUCCEEDED(rc)) m_connected = true;
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

Result IPCManager::SendCommand(u64 cmd_id) {
    if (!m_connected) {
        Result rc = connect();
        if (R_FAILED(rc)) return rc;  
    }
    Result rc = serviceDispatch(&m_service, cmd_id);
    disconnect();
    return rc;
}

Result IPCManager::sendEnableAutoFireCommand() {
    return SendCommand(CMD_ENABLE_AUTOFIRE);
}

Result IPCManager::sendDisableAutoFireCommand() {
    return SendCommand(CMD_DISABLE_AUTOFIRE);
}

Result IPCManager::sendEnableMappingCommand() {
    return SendCommand(CMD_ENABLE_MAPPING);
}

Result IPCManager::sendDisableMappingCommand() {
    return SendCommand(CMD_DISABLE_MAPPING);
}

Result IPCManager::sendReloadBasicCommand() {
    return SendCommand(CMD_RELOAD_BASIC);
}

Result IPCManager::sendReloadAutoFireCommand() {
    return SendCommand(CMD_RELOAD_AUTOFIRE);
}

Result IPCManager::sendReloadMappingCommand() {
    return SendCommand(CMD_RELOAD_MAPPING);
}

Result IPCManager::sendExitCommand() {
    return SendCommand(CMD_EXIT);
}
