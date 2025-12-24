#include "store_data.hpp"
#include "language.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "network.hpp"
#include <download_funcs.hpp>
#include <json_funcs.hpp>
#include <cJSON.h>
#include <unordered_set>

namespace {
    constexpr const char* BASE_JSON_URL = "https://macro.dokiss.cn/data/";
    constexpr const char* DOWNLOAD_URL = "https://macro.dokiss.cn/download.php?titleid=%s&file=%s";
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
    
    std::string url = std::string(BASE_JSON_URL) + GAMELIST_JSON;
    
    if (!Network::download2(url, TEMP_GAMELIST_PATH)) {
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
            if (entry.count == 0) continue;
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
    
    std::string url = std::string(BASE_JSON_URL) + "games/" + gameId + "/" + MACROLIST_JSON;
    
    if (!Network::download2(url, TEMP_MACROLIST_PATH)) {
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
            cJSON* downloads = cJSON_GetObjectItem(item, "downloads");
            
            if (file && cJSON_IsString(file)) entry.file = file->valuestring;
            if (author && cJSON_IsString(author)) entry.author = author->valuestring;
            
            if (name && cJSON_IsArray(name)) {
                cJSON* nameItem = cJSON_GetArrayItem(name, langIndex);
                if (langIndex < 2 && (!nameItem || !cJSON_IsString(nameItem) || !nameItem->valuestring[0]))
                    nameItem = cJSON_GetArrayItem(name, 1 - langIndex);  // 简繁互相回滚
                if (nameItem && cJSON_IsString(nameItem)) entry.name = nameItem->valuestring;
            }
            if (desc && cJSON_IsArray(desc)) {
                cJSON* descItem = cJSON_GetArrayItem(desc, langIndex);
                if (langIndex < 2 && (!descItem || !cJSON_IsString(descItem) || !descItem->valuestring[0]))
                    descItem = cJSON_GetArrayItem(desc, 1 - langIndex);  // 简繁互相回滚
                if (descItem && cJSON_IsString(descItem)) entry.desc = descItem->valuestring;
            }

            if (downloads && cJSON_IsNumber(downloads)) entry.downloads = downloads->valueint;
            
            result.macros.push_back(entry);
        }
    }
    
    cJSON_Delete(root);
    ult::deleteFileOrDirectory(TEMP_MACROLIST_PATH);
    result.success = true;
    return result;
}

bool StoreData::downloadMacro(const std::string& gameId, const std::string& fileName, const std::string& localPath) {
    char urlBuf[128];
    snprintf(urlBuf, sizeof(urlBuf), DOWNLOAD_URL, gameId.c_str(), fileName.c_str());
    std::string url = urlBuf;
    bool success = Network::download2(url, localPath);
    if (!success) ult::deleteFileOrDirectory(localPath);
    return success;
}

UploadResult StoreData::uploadMacro(const std::string& filePath, u64 titleId, const std::string& gameName) {
    UploadResult result{false, ""};
    result.success = Network::upload(filePath, titleId, gameName, result.code);
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
