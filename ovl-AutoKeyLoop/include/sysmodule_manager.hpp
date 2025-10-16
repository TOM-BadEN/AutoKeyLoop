#pragma once
#include <switch.h>

/**
 * 系统模块管理类 - 用于管理 sys-AutoKeyLoop 系统模块
 */
class SysModuleManager {
private:
    // sys-AutoKeyLoop 系统模块的程序ID (Title ID)
    static constexpr u64 MODULE_ID = 0x4100000002025924;
    
public:
    /**
     * 检查系统模块是否正在运行
     * @return true=运行中, false=未运行
     */
    static bool isRunning();
    
    /**
     * 启动系统模块
     * @return Result 0=成功，其他=失败
     */
    static Result startModule();
    
    /**
     * 通过 IPC 通知系统模块退出
     * @return Result 0=成功，其他=失败
     * @note 如果系统模块未运行，会返回失败
     */
    static Result stopModule();
    
    /**
     * 重启系统模块（先停止，等待50ms，再启动）
     * @return Result 0=成功，其他=失败
     * @note 如果系统模块未运行，直接返回成功
     */
    static Result restartModule();
    
    /**
     * 切换系统模块开关（运行中则停止，未运行则启动）
     * @return Result 0=成功，其他=失败
     */
    static Result toggleModule();
};

