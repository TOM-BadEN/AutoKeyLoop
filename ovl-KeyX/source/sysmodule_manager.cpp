#include "sysmodule_manager.hpp"
#include "ipc_manager.hpp"
#include <ultra.hpp>

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
    g_ipcManager.sendExitCommand();

    // 为了确保正确停止，增加超时机制，最大200ms
    int timeout = 20;
    while (isRunning() && timeout > 0) {
        svcSleepThread(10000000ULL); // 10ms
        timeout--;
    }

    if (isRunning()) {
        return 999;
    }
    
    return 0;
}

// 切换系统模块开关（运行中则停止，未运行则启动）
Result SysModuleManager::toggleModule() {
    if (isRunning()) return stopModule();
    else return startModule();
}

// 检查是否有 boot2.flag（自启动标志）
bool SysModuleManager::hasBootFlag() {
    return ult::isFile(BOOT2_FLAG_PATH);
}

// 切换 boot2.flag（有则删除，无则创建）
Result SysModuleManager::toggleBootFlag() {
    // 如果文件存在，删除它
    if (ult::isFile(BOOT2_FLAG_PATH)) {
        if (remove(BOOT2_FLAG_PATH) != 0) return -1;
        return 0;
    }
    // 文件不存在，创建空文件
    ult::createTextFile(BOOT2_FLAG_PATH, "");
    return 0;
}

