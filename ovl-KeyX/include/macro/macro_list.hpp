#pragma once

#include <tesla.hpp>
#include <vector>
#include <string>


class MacroListGui : public tsl::Gui {
public:
    MacroListGui();
    virtual tsl::elm::Element* createUI() override;
   
private:
    std::vector<std::string> m_macroDirs;
};


