#include "sysmodule_manager.hpp"
#include "ipc_manager.hpp"

// 检查系统模块是否正在运行
bool SysModuleManager::isRunning() {
    u64 pid = 0;
    
    // 使用 pmdmnt 服务获取进程ID（libtesla已初始化）
    if (R_FAILED(pmdmntGetProcessId(&pid, MODULE_ID))) {
        return false;
    }
    
    return pid > 0;
}

// 启动系统模块
Result SysModuleManager::startModule() {
    // 如果已经在运行，则不需要启动
    if (isRunning()) {
        return 0;
    }
    
    // 构建程序位置信息
    NcmProgramLocation programLocation = {
        .program_id = MODULE_ID,
        .storageID = NcmStorageId_None
    };
    
    // 使用 pmshell 启动程序
    u64 pid = 0;
    Result rc = pmshellLaunchProgram(0, &programLocation, &pid);
    
    return rc;
}

// 通过 IPC 通知系统模块退出
Result SysModuleManager::stopModule() {
    // 检查系统模块是否正在运行
    if (!isRunning()) {
        return 0xCAFE02; // 自定义错误码：系统模块未运行
    }
    
    // 使用 IPC 管理器发送退出命令
    Result rc = g_ipcManager.sendExitCommand();


    // 为了确保正确停止，增加超时机制，最大200ms
    int timeout = 20;
    while (isRunning() && timeout > 0) {
        svcSleepThread(10000000ULL); // 10ms
        timeout--;
    }

    if (isRunning()) {
        return 9999; // 自定义错误码：系统模块退出超时
    }
    
    return rc;
}

// 重启系统模块（先停止，等待50ms，再启动）
Result SysModuleManager::restartModule() {
    // 检查系统模块是否正在运行
    if (!isRunning()) {
        return 0;  // 未运行，直接返回成功
    }
    
    // 停止系统模块
    Result rc = stopModule();
    if (R_FAILED(rc)) {
        return rc;  // 停止失败，返回错误
    }
    
    // 重新启动系统模块
    rc = startModule();
    
    return rc;
}

// 切换系统模块开关（运行中则停止，未运行则启动）
Result SysModuleManager::toggleModule() {
    if (isRunning()) return stopModule();
    else return startModule();
}

