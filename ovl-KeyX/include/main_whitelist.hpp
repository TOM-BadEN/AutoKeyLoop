#pragma once
#include <tesla.hpp>
#include <vector>

class SettingWhitelist : public tsl::Gui 
{
public:
    SettingWhitelist();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
private:
    struct AppEntry {
        u64 tid;
        tsl::elm::ToggleListItem* item;
    };
    std::vector<AppEntry> m_apps;
    size_t m_nextIndex = 0;
};
