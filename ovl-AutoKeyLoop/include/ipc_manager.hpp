#pragma once
#include <switch.h>

// IPC命令定义
#define CMD_ENABLE_AUTOKEY  1   // 开启连发模块
#define CMD_DISABLE_AUTOKEY 2   // 关闭连发模块
#define CMD_RELOAD_CONFIG   3   // 重载配置
#define CMD_EXIT            999 // 退出系统模块

/**
 * IPC管理类 - 负责与 sys-AutoKeyLoop 系统模块的通信
 * 
 * 使用全局对象管理 IPC 服务生命周期，程序退出时自动断开连接
 */
class IPCManager {
private:
    static constexpr const char* SERVICE_NAME = "keyLoop";  // 系统模块注册的服务名
    
    Service m_service;      // IPC 服务句柄
    bool m_connected;       // 连接状态

public:
    /**
     * 构造函数 - 初始化但不自动连接
     */
    IPCManager();
    
    /**
     * 析构函数 - 自动断开连接并清理资源
     */
    ~IPCManager();
    
    // 禁止拷贝
    IPCManager(const IPCManager&) = delete;
    IPCManager& operator=(const IPCManager&) = delete;
    
    /**
     * 连接到系统模块的 IPC 服务
     * @return Result 0=成功，其他=失败
     */
    Result connect();
    
    /**
     * 断开 IPC 连接
     */
    void disconnect();
    
    /**
     * 检查是否已连接
     * @return true=已连接, false=未连接
     */
    bool isConnected() const;
    
    /**
     * 发送开启连发命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendEnableCommand();
    
    /**
     * 发送关闭连发命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendDisableCommand();
    
    /**
     * 发送退出命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 成功发送后会自动断开连接
     */
    Result sendExitCommand();
    
    /**
     * 重启连发功能（先关闭，等待50ms，再开启）
     * @return Result 0=成功，其他=失败
     * @note 用于切换配置后重新加载配置
     */
    Result sendRestartCommand();
    
    /**
     * 发送重载配置命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 系统模块会重新读取配置并动态更新连发参数（不重启线程）
     */
    Result sendReloadConfigCommand();
};

// 全局实例 - 程序退出时自动调用析构函数
extern IPCManager g_ipcManager;

