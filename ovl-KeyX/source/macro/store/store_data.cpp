#include "store_data.hpp"
#include "language.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include <download_funcs.hpp>
#include <json_funcs.hpp>
#include <cJSON.h>
// #include <curl/curl.h>
#include <unordered_set>

namespace {
    constexpr const char* CN_BASE_URL = "https://macro.dokiss.cn/data/";
    constexpr const char* GAMELIST_JSON = "gamelist.json";
    constexpr const char* MACROLIST_JSON = "macrolist.json";
    constexpr const char* TEMP_GAMELIST_PATH = "sdmc:/config/KeyX/store/gamelist.json";
    constexpr const char* TEMP_MACROLIST_PATH = "sdmc:/config/KeyX/store/macrolist.json";
    constexpr const char* MACROS_DIR = "sdmc:/config/KeyX/macros/";
}

StoreData::StoreData() {
    m_isSimplifiedChinese = LanguageManager::isSimplifiedChinese();
}

StoreData::~StoreData() {
}

GameListResult StoreData::getGameList() {
    GameListResult result{};
    
    std::string url = std::string(CN_BASE_URL) + GAMELIST_JSON;
    
    if (!ult::downloadFile(url, TEMP_GAMELIST_PATH, false)) {
        result.error = "请检查网络连接";
        return result;
    }
    
    auto json = ult::readJsonFromFile(TEMP_GAMELIST_PATH);
    if (!json) {
        result.error = "JSON 解析失败";
        ult::deleteFileOrDirectory(TEMP_GAMELIST_PATH);
        return result;
    }
    
    cJSON* root = reinterpret_cast<cJSON*>(json);
    cJSON* games = cJSON_GetObjectItem(root, "games");
    
    std::unordered_set<u64> installedIds = GameMonitor::getInstalledAppAndGameIds();
    
    if (games && cJSON_IsArray(games)) {
        cJSON* item;
        cJSON_ArrayForEach(item, games) {
            cJSON* id = cJSON_GetObjectItem(item, "id");
            if (!id || !cJSON_IsString(id)) continue;
            u64 tid = strtoull(id->valuestring, nullptr, 16);
            if (installedIds.count(tid) == 0) continue;
            StoreGameEntry entry;
            entry.id = tid;
            cJSON* count = cJSON_GetObjectItem(item, "count");
            if (count && cJSON_IsNumber(count)) entry.count = count->valueint;
            result.games.push_back(entry);
        }
    }
    
    cJSON_Delete(root);
    ult::deleteFileOrDirectory(TEMP_GAMELIST_PATH);
    result.success = true;
    return result;
}

MacroListResult StoreData::getMacroList(const std::string& gameId) {
    MacroListResult result{};
    
    std::string url = std::string(CN_BASE_URL) + "games/" + gameId + "/" + MACROLIST_JSON;
    
    if (!ult::downloadFile(url, TEMP_MACROLIST_PATH, false)) {
        result.error = "请检查网络连接";
        return result;
    }
    
    ult::json_t* json = ult::readJsonFromFile(TEMP_MACROLIST_PATH);
    if (!json) {
        result.error = "JSON 解析失败";
        ult::deleteFileOrDirectory(TEMP_MACROLIST_PATH);
        return result;
    }
    
    cJSON* root = reinterpret_cast<cJSON*>(json);
    cJSON* macros = cJSON_GetObjectItem(root, "macros");
    
    int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
    
    if (macros && cJSON_IsArray(macros)) {
        cJSON* item;
        cJSON_ArrayForEach(item, macros) {
            StoreMacroEntry entry;
            cJSON* file = cJSON_GetObjectItem(item, "file");
            cJSON* name = cJSON_GetObjectItem(item, "name");
            cJSON* desc = cJSON_GetObjectItem(item, "desc");
            cJSON* author = cJSON_GetObjectItem(item, "author");
            
            if (file && cJSON_IsString(file)) entry.file = file->valuestring;
            if (author && cJSON_IsString(author)) entry.author = author->valuestring;
            
            if (name && cJSON_IsArray(name)) {
                cJSON* nameItem = cJSON_GetArrayItem(name, langIndex);
                if (nameItem && cJSON_IsString(nameItem)) entry.name = nameItem->valuestring;
            }
            if (desc && cJSON_IsArray(desc)) {
                cJSON* descItem = cJSON_GetArrayItem(desc, langIndex);
                if (descItem && cJSON_IsString(descItem)) entry.desc = descItem->valuestring;
            }
            
            result.macros.push_back(entry);
        }
    }
    
    cJSON_Delete(root);
    ult::deleteFileOrDirectory(TEMP_MACROLIST_PATH);
    result.success = true;
    return result;
}

bool StoreData::downloadMacro(const std::string& gameId, const std::string& fileName, const std::string& localPath) {
    std::string url = std::string(CN_BASE_URL) + "games/" + gameId + "/" + fileName;
    bool success = ult::downloadFile(url, localPath, false);
    if (!success) ult::deleteFileOrDirectory(localPath);
    return success;
}

UploadResult StoreData::uploadMacro(const char* filePath, u64 titleId, const char* gameName) {
    
    // 进度回调（用于支持中断）
    auto progressCallback = [](void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int {
        return ult::abortDownload.load() ? 1 : 0;
    };

    // 响应写入回调
    auto writeCallback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto* resp = static_cast<std::string*>(userdata);
        resp->append(ptr, size * nmemb);
        return size * nmemb;
    };

    UploadResult result{false, ""};
    ult::abortDownload = false;
    
    // 检查文件是否存在
    if (!ult::isFile(filePath)) return result;
    
    CURL* curl = curl_easy_init();
    if (!curl) return result;
    
    // 构建 multipart form
    curl_mime* mime = curl_mime_init(curl);
    
    // 添加文件字段
    curl_mimepart* filePart = curl_mime_addpart(mime);
    curl_mime_name(filePart, "macro");
    curl_mime_filedata(filePart, filePath);
    
    // 添加 titleId 字段
    char tidStr[32];
    snprintf(tidStr, sizeof(tidStr), "%016lX", titleId);
    curl_mimepart* tidPart = curl_mime_addpart(mime);
    curl_mime_name(tidPart, "titleid");
    curl_mime_data(tidPart, tidStr, CURL_ZERO_TERMINATED);
    
    // 添加 gamename 字段
    curl_mimepart* namePart = curl_mime_addpart(mime);
    curl_mime_name(namePart, "gamename");
    curl_mime_data(namePart, gameName, CURL_ZERO_TERMINATED);
    
    // 设置请求
    curl_easy_setopt(curl, CURLOPT_URL, "https://macro.dokiss.cn/upload.php");
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, 16 * 1024L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.response);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, +progressCallback);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_mime_free(mime);
    curl_easy_cleanup(curl);
    
    result.success = (res == CURLE_OK);
    return result;
}

void StoreData::saveMacroMetadataInfo(const std::string& gameId, const StoreMacroEntry& macro) {
    std::string iniPath = std::string(MACROS_DIR) + gameId + "/macroMetadata.ini";
    IniHelper::setString(macro.file, "name", macro.name, iniPath);
    IniHelper::setString(macro.file, "author", macro.author, iniPath);
    // 转义换行符，防止 INI 格式被破坏
    std::string escapedDesc = macro.desc;
    size_t pos = 0;
    while ((pos = escapedDesc.find('\n', pos)) != std::string::npos) {
        escapedDesc.replace(pos, 1, "\\n");
        pos += 2;
    }
    IniHelper::setString(macro.file, "desc", escapedDesc, iniPath);
}
