#include "updater_data.hpp"
#include "language.hpp"
#include <download_funcs.hpp>
#include <json_funcs.hpp>
#include <cJSON.h>

namespace {
    constexpr const char* CN_UPDATE_URL = "https://gitee.com/TOM-BadEN/KeyX/raw/main/update/update.json";
    constexpr const char* EN_UPDATE_URL = "https://raw.githubusercontent.com/TOM-BadEN/KeyX/main/update/update.json";
    constexpr const char* CN_DOWNLOAD_URL = "https://gitee.com/TOM-BadEN/KeyX/raw/main/update/KeyX-CN.zip";
    constexpr const char* EN_DOWNLOAD_URL = "https://github.com/TOM-BadEN/KeyX/releases/latest/download/KeyX-EN.zip";
    constexpr const char* UPDATE_JSON_PATH = "sdmc:/config/KeyX/update/update.json";
    constexpr const char* UPDATE_KEYX_PATH = "sdmc:/config/KeyX/update/KeyX.zip";
    constexpr const char* UPDATE_UNZIP_PATH = "sdmc:/";
}




UpdaterData::UpdaterData() {
    m_isSimplifiedChinese = LanguageManager::isSimplifiedChinese();
}

UpdaterData::~UpdaterData() {
}

UpdateInfo UpdaterData::getUpdateInfo() {

    UpdateInfo info{};
    if (!ult::downloadFile(m_isSimplifiedChinese ? CN_UPDATE_URL : EN_UPDATE_URL, UPDATE_JSON_PATH, false)) {
        info.error = "请检查网络连接";
        return info;
    }

    auto json = ult::readJsonFromFile(UPDATE_JSON_PATH);
    if (!json) {
        info.error = "JSON 解析失败";
        ult::deleteFileOrDirectory(UPDATE_JSON_PATH);
        return info;
    }

    info.version = ult::getStringFromJson(json, "version");


    cJSON* root = reinterpret_cast<cJSON*>(json);
    cJSON* changelog = cJSON_GetObjectItem(root, "changelog");
    int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
    cJSON* list = cJSON_GetObjectItem(changelog, (langIndex < 2) ? "CN" : "EN");
    if (list && cJSON_IsArray(list)) {
        cJSON* item;
        cJSON_ArrayForEach(item, list) {
            if (cJSON_IsString(item)) info.changelog.push_back(item->valuestring);
        }
    }

    cJSON_Delete(root);
    info.success = true;
    ult::deleteFileOrDirectory(UPDATE_JSON_PATH);
    return info;
}

bool UpdaterData::hasNewVersion(const std::string& remote, const std::string& local) {
    // 去掉 v 前缀
    std::string r = (remote[0] == 'v' || remote[0] == 'V') ? remote.substr(1) : remote;
    std::string l = (local[0] == 'v' || local[0] == 'V') ? local.substr(1) : local;
    
    int r1 = 0, r2 = 0, r3 = 0;
    int l1 = 0, l2 = 0, l3 = 0;
    sscanf(r.c_str(), "%d.%d.%d", &r1, &r2, &r3);
    sscanf(l.c_str(), "%d.%d.%d", &l1, &l2, &l3);
    
    if (r1 != l1) return r1 > l1;
    if (r2 != l2) return r2 > l2;
    return r3 > l3;
}

bool UpdaterData::downloadZip() {
    bool success = ult::downloadFile(m_isSimplifiedChinese ? CN_DOWNLOAD_URL : EN_DOWNLOAD_URL, UPDATE_KEYX_PATH, false);
    if (!success) ult::deleteFileOrDirectory(UPDATE_KEYX_PATH);
    return success;
}

bool UpdaterData::unzipZip() {
    bool success = ult::unzipFile(UPDATE_KEYX_PATH, UPDATE_UNZIP_PATH);
    ult::deleteFileOrDirectory(UPDATE_KEYX_PATH);
    return success;
}
