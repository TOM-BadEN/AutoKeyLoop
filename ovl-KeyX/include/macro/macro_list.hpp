#pragma once

#include <tesla.hpp>
#include <vector>
#include <string>

// 脚本清单所有游戏列表类
class MacroListGui : public tsl::Gui {
public:
    MacroListGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
   
private:

    struct MacroDirEntry {
        std::string dirName;          
        tsl::elm::ListItem* item{};   
    };

    std::vector<MacroDirEntry> m_macroDirs{};
    size_t m_nextIndex = 0;
};

// 脚本清单具体游戏的脚本列表类
class MacroListGuiGame : public tsl::Gui {
public:
    MacroListGuiGame(u64 titleId);
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;

private:

    struct Macro {
        std::string macroPath;
        u64 Hotkey;
    };

    std::vector<Macro> m_macro{};
    u64 m_titleId;
    char m_gameName[64];
};


