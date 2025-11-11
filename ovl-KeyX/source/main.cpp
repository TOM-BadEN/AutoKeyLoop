#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include "main_menu.hpp"
#include "language_manager.hpp"

// KeyX 特斯拉覆盖层主类
class KeyXOverlay : public tsl::Overlay {
public:
    // 初始化系统服务
    virtual void initServices() override 
    {
        fsdevMountSdmc();     // 挂载SD卡
        pmdmntInitialize();   // 进程管理服务
        pmshellInitialize();  // 进程Shell服务（用于启动/停止系统模块）
        setInitialize();      // 初始化set服务（获取系统语言）
        LanguageManager::initialize();  // 初始化语言系统
        setExit();            // 退出set服务
        pdmqryInitialize();
    }
    
    // 退出系统服务
    virtual void exitServices() override 
    {
        pdmqryExit();
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
    return tsl::loop<KeyXOverlay>(argc, argv);
}