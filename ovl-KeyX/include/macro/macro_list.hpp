#pragma once

#include <tesla.hpp>
#include <vector>
#include <string>


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

    std::vector<MacroDirEntry> m_macroDirs;
    size_t m_nextIndex = 0;
};


