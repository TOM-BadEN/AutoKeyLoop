#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include "main_menu.hpp"
#include "language_manager.hpp"

// AutoKeyLoop 特斯拉覆盖层主类
class AutoKeyLoopOverlay : public tsl::Overlay {
public:
    // 初始化系统服务
    virtual void initServices() override 
    {
        fsdevMountSdmc();     // 挂载SD卡
        pmdmntInitialize();   // 进程管理服务
        pmshellInitialize();  // 进程Shell服务（用于启动/停止系统模块）
        LanguageManager::initialize();  // 初始化语言系统
    }
    
    // 退出系统服务
    virtual void exitServices() override 
    {
        pmshellExit();        // 退出进程Shell服务
        pmdmntExit();         // 退出进程管理服务
        fsdevUnmountAll();    // 卸载所有文件系统
    }

    // 覆盖层显示时调用，用于更新游戏信息
    virtual void onShow() override {
        MainMenu::UpdateMainMenu();  // 更新文本区域信息
    }

    // 加载初始GUI界面
    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainMenu>();  // 返回主菜单界面
    }
};

// 程序入口点
int main(int argc, char **argv) {
    return tsl::loop<AutoKeyLoopOverlay>(argc, argv);
}