#include "setting_macro.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"
#include "sysmodule.hpp"
#include "record_macro.hpp"
#include "focus.hpp"

// 录制消息全局变量
static char g_recordMessage[32] = "";

// 录制消息设置函数实现
void setRecordingMessage(const char* msg) {
    strncpy(g_recordMessage, msg, sizeof(g_recordMessage) - 1);
    g_recordMessage[sizeof(g_recordMessage) - 1] = '\0';
}

// 中间层用的 Frame
interlayerFrame::interlayerFrame()
{
}

void interlayerFrame::draw(tsl::gfx::Renderer* renderer) {
    renderer->fillScreen(tsl::Color(0x0, 0x0, 0x0, 0x0));
}

void interlayerFrame::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    this->setBoundaries(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);
}


// 中间层界面类
interlayerGui::interlayerGui()
{
}

tsl::elm::Element* interlayerGui::createUI() {
    return new interlayerFrame();
}

void interlayerGui::update() {
    ult::internalTouchReleased.store(false, std::memory_order_release);
    tsl::changeTo<CountdownGui>();
}

// 倒计时界面重写的Frame类
CountdownFrame::CountdownFrame()
 : m_countdown2("")
{
}

void CountdownFrame::setCountdown(const char* text) {
    strcpy(m_countdown2, text);
}

void CountdownFrame::draw(tsl::gfx::Renderer* renderer) {
    // 全透明背景
    renderer->fillScreen(tsl::Color(0x0, 0x0, 0x0, 0x0));
    // 计算尺寸
    auto textDim = renderer->getTextDimensions(m_countdown2, false, 300);
    s32 squareSize = (textDim.first > textDim.second ? textDim.first : textDim.second) + 60;
    // 正方形位置（屏幕中央）
    s32 squareX = (tsl::cfg::FramebufferWidth - squareSize) / 2;
    s32 squareY = (tsl::cfg::FramebufferHeight - squareSize) / 2;
    renderer->drawRoundedRect(squareX, squareY, squareSize, squareSize, 20, tsl::Color(0x0, 0x0, 0x0, 0x5));
    s32 textX = squareX + (squareSize - textDim.first) / 2;
    s32 textY = squareY + (squareSize + textDim.second) / 2 - 30;
    renderer->drawString(m_countdown2, false, textX, textY, 300, tsl::Color(0xF, 0xF, 0xF, 0xF));
}

void CountdownFrame::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    this->setBoundaries(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);
}

// 倒计时界面类
CountdownGui::CountdownGui()
 : m_startTime(armGetSystemTick())
 , m_countdown("")
 , m_lastFocusCheckMs(100)  // 初始化为100，跳过第一个100ms
{
    // 移动到屏幕中间
    u16 centerX = (tsl::cfg::ScreenWidth - tsl::cfg::LayerWidth) / 2;
    tsl::gfx::Renderer::get().setLayerPos(centerX, 0);
}

tsl::elm::Element* CountdownGui::createUI() {
    m_frame = new CountdownFrame();
    return m_frame;
}

void CountdownGui::update() {
    u64 elapsed_ms = armTicksToNs(armGetSystemTick() - m_startTime) / 1000000ULL;
    if (elapsed_ms >= m_lastFocusCheckMs + 100) {
        m_lastFocusCheckMs = elapsed_ms;
        FocusState focusState = FocusMonitor::GetState(GameMonitor::getCurrentTitleId());
        if (focusState == FocusState::OutOfFocus) {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
            setRecordingMessage("请在游戏中录制");
            tsl::goBack();
            tsl::goBack();
            return;
        }
    }
    if (elapsed_ms < 1000)  strcpy(m_countdown, "3");
    else if (elapsed_ms < 2000) strcpy(m_countdown, "2");
    else if (elapsed_ms < 3000) strcpy(m_countdown, "1");
    else if (elapsed_ms < 3100) strcpy(m_countdown, "0");
    else tsl::changeTo<RecordingGui>();
    m_frame->setCountdown(m_countdown);
}

bool CountdownGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // 检测特斯拉快捷键或B键
    const u64 combo = tsl::cfg::launchCombo;
    if ((((keysHeld & combo) == combo) && (keysDown & combo)) || (keysDown & HidNpadButton_B)) {
        setRecordingMessage("已取消录制");
        tsl::gfx::Renderer::get().setLayerPos(0, 0);
        tsl::goBack();
        tsl::goBack();
        return true;
    }
    return true;
}


// 设置界面类
SettingMacro::SettingMacro()
{
    tsl::disableComboHide.store(false, std::memory_order_release);  // 恢复特斯拉区域触摸和快捷键hide的功能
}

tsl::elm::Element* SettingMacro::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本设置", "配置脚本功能");
    auto list = new tsl::elm::List();
    
    auto ItemBasicSetting = new tsl::elm::CategoryHeader(" 脚本管理");
    list->addItem(ItemBasicSetting);

    auto listItemRecord = new tsl::elm::ListItem("录制脚本", "");
    listItemRecord->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            return HandleRecordClick();
        }
        return false;
    });
    list->addItem(listItemRecord);

    auto listItemView = new tsl::elm::ListItem("查看脚本", ">");
    listItemView->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            // TODO: 实现查看脚本功能
            return true;
        }
        return false;
    });
    list->addItem(listItemView);

    auto ItemRecordTutorial = new tsl::elm::CategoryHeader(" 录制教程");
    list->addItem(ItemRecordTutorial);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("1. 仅在游戏中可以点击脚本录制", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("2. 倒数3秒开始录制，按B可取消", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("3. 最大录制30s，使用特斯拉快捷键完成录制", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);

    auto countdown = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        s32 fontSize = 54;
        auto textDim = r->getTextDimensions(g_recordMessage, false, fontSize);
        while (textDim.first > w - 30) {
            fontSize = fontSize * 9 / 10;  // 缩小 10%
            textDim = r->getTextDimensions(g_recordMessage, false, fontSize);
        }
        s32 centerX = x + (w - textDim.first) / 2;
        s32 centerY = y + (h + textDim.second) / 2;
        r->drawString(g_recordMessage, false, centerX, centerY, fontSize, tsl::Color(0xF, 0x5, 0x5, 0xF));
    });

    list->addItem(countdown, 194);
    
    frame->setContent(list);
    return frame;
}

bool SettingMacro::HandleRecordClick() {
    if (!SysModuleManager::isRunning()) {
        setRecordingMessage("未启动系统模块");
        return true;
    }
    
    // 获取当前游戏TID
    u64 gameTid = GameMonitor::getCurrentTitleId();
    if (gameTid == 0) {
        setRecordingMessage("未启动游戏");
        return true;
    }
    
    // 检测游戏焦点状态
    FocusState focusState = FocusMonitor::GetState(gameTid);
    if (focusState == FocusState::OutOfFocus) {
        setRecordingMessage("请在游戏中录制");
        return true;
    }
    
    // 跳转到倒计时界面
    tsl::disableComboHide.store(true, std::memory_order_release);  // 禁用特斯拉区域触摸和快捷键hide的功能
    tsl::changeTo<interlayerGui>();
    return true;
}





