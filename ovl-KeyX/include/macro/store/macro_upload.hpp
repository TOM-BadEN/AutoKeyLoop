#pragma once
#include <tesla.hpp>
#include "macro_util.hpp"
#include "store_data.hpp"
#include <vector>

enum class UploadState {
    Uploading,      // 上传中
    UploadSuccess,  // 上传成功
    UploadFailed,   // 上传失败
};

// 脚本详情界面
class MacroUploadGui : public tsl::Gui {
public:
    MacroUploadGui(const std::string& macroFilePath, const std::string& gameName);
    ~MacroUploadGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    void drawUploading(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);
    void drawUploadSuccess(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);
    void prepareSuccessData();

    std::string m_gameName;
    MacroUtil::MacroMetadata m_meta;
    
    UploadState m_state = UploadState::Uploading;
    int m_frameIndex = 0;
    int m_frameCounter = 0;
    UploadResult m_uploadResult;
    
    // 上传成功后缓存（避免每帧重新生成）
    std::vector<std::vector<bool>> m_qrModules;
    int m_qrSize = 0;
};
