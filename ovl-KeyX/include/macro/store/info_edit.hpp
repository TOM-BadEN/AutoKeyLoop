#pragma once
#include <tesla.hpp>
#include <vector>
#include <string>

class MacroInfoEditGui : public tsl::Gui {
public:
    MacroInfoEditGui(const char* editUrl, const std::string& macroName, const std::string& author);
    virtual tsl::elm::Element* createUI() override;

private:
    void drawNotSupported(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);
    void drawQrCode(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);
    const char* m_editUrl;
    std::string m_macroName;
    std::string m_author;
    bool m_isStoreMacro = false;
    std::vector<std::vector<bool>> m_qrModules;
    int m_qrSize = 0;
};
