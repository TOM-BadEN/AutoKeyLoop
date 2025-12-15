#pragma once
#include <string>
#include <vector>

struct UpdateInfo {
    bool success = false;                   // 获取是否成功
    std::string error;                      // 错误信息（失败时）
    std::string version;                    // 最新版本号
    std::vector<std::string> changelog;     // 更新日志（自动选择 CN/EN）
};

class UpdaterData {
public:
    UpdaterData();
    ~UpdaterData();
    
    UpdateInfo getUpdateInfo();                                                 // 获取远程更新信息
    bool hasNewVersion(const std::string& remote, const std::string& local);    // 判断是否有新版本
    bool downloadZip();                                                         // 下载更新压缩包
    bool unzipZip();                                                            // 解压更新压缩包

private:

    bool m_isSimplifiedChinese = false;
};
