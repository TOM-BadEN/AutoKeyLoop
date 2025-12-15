#pragma once
#include <switch.h>

/**
 * 系统模块管理类 - 用于管理 sys-KeyX 系统模块
 */
class SysModuleManager {
private:
    // sys-KeyX 系统模块的程序ID (Title ID)
    static constexpr u64 MODULE_ID = 0x4100000002025924;
    
    // boot2.flag 路径
    static constexpr const char* BOOT2_FLAG_PATH = "/atmosphere/contents/4100000002025924/flags/boot2.flag";
    static constexpr const char* BOOT2_FLAG_DIR = "/atmosphere/contents/4100000002025924/flags";
    
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
     * 切换系统模块开关（运行中则停止，未运行则启动）
     * @return Result 0=成功，其他=失败
     */
    static Result toggleModule();
    
    /**
     * 重启系统模块（未运行则直接返回成功，运行中则先停止再启动）
     * @return Result 0=成功，其他=失败
     */
    static Result restartModule();
    
    /**
     * 检查是否有 boot2.flag（自启动标志）
     * @return true=有 flag，false=无 flag
     */
    static bool hasBootFlag();
    
    /**
     * 切换 boot2.flag（有则删除，无则创建）
     * @return Result 0=成功，其他=失败
     */
    static Result toggleBootFlag();
};

