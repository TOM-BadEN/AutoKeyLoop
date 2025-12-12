#pragma once
#include <switch.h>

// IPC命令定义 - 与 sys-KeyX 保持一致
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

// 宏控制
#define CMD_ENABLE_MACRO      8   // 开启宏
#define CMD_DISABLE_MACRO     9   // 关闭宏
#define CMD_RELOAD_MACRO      10  // 重载宏配置

// 白名单控制
#define CMD_RELOAD_WHITELIST  11  // 重载白名单

// 系统控制
#define CMD_EXIT              999 // 退出系统模块

/**
 * IPC管理类 - 负责与 sys-KeyX 系统模块的通信
 * 
 * 使用全局对象管理 IPC 服务生命周期，程序退出时自动断开连接
 * 每次发送命令后自动断开连接
 */
class IPCManager {
private:
    static constexpr const char* SERVICE_NAME = "keyLoop";  // 系统模块注册的服务名
    
    Service m_service;      // IPC 服务句柄
    bool m_connected;       // 连接状态
    
    /**
     * 通用命令发送方法 - 连接、发送、断开
     * @param cmd_id 命令ID
     * @param auto_start 如果系统模块未运行，是否尝试自动启动
     * @return Result 0=成功，其他=失败
     */
    Result SendCommand(u64 cmd_id, bool auto_start = false);

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
     * @return true=已连接, false=未连
     */
    bool isConnected() const;
    
    /**
     * 发送开启连发命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendEnableAutoFireCommand();
    
    /**
     * 发送关闭连发命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendDisableAutoFireCommand();
    
    /**
     * 发送开启映射命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendEnableMappingCommand();
    
    /**
     * 发送关闭映射命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendDisableMappingCommand();
    
    /**
     * 发送重载基础配置命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 重载游戏特定的基础配置
     */
    Result sendReloadBasicCommand();
    
    /**
     * 发送重载连发配置命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 重载连发相关配置
     */
    Result sendReloadAutoFireCommand();
    
    /**
     * 发送重载映射配置命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 重载按键映射配置
     */
    Result sendReloadMappingCommand();
    
    /**
     * 发送退出命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 成功发送后会自动断开连接
     */
    Result sendExitCommand();

    /**
     * 发送开启宏命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendEnableMacroCommand();
    
    /**
     * 发送关闭宏命令给系统模块
     * @return Result 0=成功，其他=失败
     */
    Result sendDisableMacroCommand();
    
    /**
     * 发送重载宏配置命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 重载按键宏配置
     */
    Result sendReloadMacroCommand();
    
    /**
     * 发送重载白名单命令给系统模块
     * @return Result 0=成功，其他=失败
     * @note 重载白名单配置
     */
    Result sendReloadWhitelistCommand();
};

// 全局实例 - 程序退出时自动调用析构函数
extern IPCManager g_ipcManager;

