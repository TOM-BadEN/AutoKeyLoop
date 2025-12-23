#pragma once
#include <tesla.hpp>
#include <vector>
#include <string>
#include "store_data.hpp"


// 加载状态枚举
enum class LoadState {
    Loading,        // 加载中
    NetworkError,   // 网络错误
    GameListEmpty,  // 游戏列表为空
    MacroListEmpty, // 宏列表为空
    Success         // 数据获取成功
};

// 商店数据获取过渡界面
class StoreGetDataGui : public tsl::Gui {
public:
    StoreGetDataGui(u64 tid = 0);
    ~StoreGetDataGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    u64 m_tid;
    LoadState m_state = LoadState::Loading;
    int m_frameIndex = 0;
    int m_frameCounter = 0;
    char gameName[64];
    
    static void getMacroListForTid(u64 tid);
};


// 商店游戏列表界面
class StoreGameListGui : public tsl::Gui {
public:
    StoreGameListGui();
    ~StoreGameListGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    struct GameEntry {
        u64 tid;
        tsl::elm::ListItem* item;
    };
    std::vector<GameEntry> m_entries;
    size_t m_nextIndex = 0;
    int m_loadingIndex = -1;
    int m_frameIndex = 0;
    int m_frameCounter = 0;
};

// 商店宏列表界面
class StoreMacroListGui : public tsl::Gui {
public:
    StoreMacroListGui(u64 tid, const std::string& gameName);
    ~StoreMacroListGui();
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    u64 m_tid;
    std::string m_gameName;
};

// 宏详情状态
enum class MacroViewState {
    Ready,          // 准备下载
    Downloading,    // 下载中
    Success,        // 下载成功
    Cancelled,      // 已取消
    Error           // 下载失败
};

// 商店宏详情界面
class StoreMacroViewGui : public tsl::Gui {
public:
    StoreMacroViewGui(u64 tid, const std::string& gameName);
    ~StoreMacroViewGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    u64 m_tid;
    std::string m_gameName;
    std::string m_localPath;  // 下载路径
    MacroViewState m_state = MacroViewState::Ready;
    
    s32 m_scrollOffset = 0;
    s32 m_maxScrollOffset = 0;
    
    void drawContent(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);
};

// 网页商店界面
class WebStoreGui : public tsl::Gui {
public:
    WebStoreGui(u64 tid, const std::string& gameName);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    void drawContent(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);
    
    u64 m_tid;
    std::string m_gameName;
    std::vector<std::vector<bool>> m_qrModules;
    int m_qrSize = 0;
};
