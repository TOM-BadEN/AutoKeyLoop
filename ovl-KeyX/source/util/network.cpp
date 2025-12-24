#include "network.hpp"
#include <ultra.hpp>
#include <cJSON.h>

namespace {
    constexpr SocketInitConfig socketConfig = {
        .tcp_tx_buf_size     = 8 * 1024,
        .tcp_rx_buf_size     = 16 * 1024 * 2,
        .tcp_tx_buf_max_size = 32 * 1024,
        .tcp_rx_buf_max_size = 64 * 1024 * 2,
        .udp_tx_buf_size     = 512,
        .udp_rx_buf_size     = 512,
        .sb_efficiency       = 1,
        .bsd_service_type    = BsdServiceType_Auto
    };
}

bool Network::download(const std::string& url, const std::string& toPath) {
    socketInitialize(&socketConfig);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    bool result = ult::downloadFile(url, toPath, false);
    curl_global_cleanup();
    socketExit();
    return result;
}

bool Network::upload(const std::string& filePath, u64 titleId, const std::string& gameName, std::string& outCode) {
    if (!ult::isFile(filePath)) return false;
    
    socketInitialize(&socketConfig);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    ult::abortDownload = false;
    std::string response;
    bool success = false;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        socketExit();
        return false;
    }
    
    // 响应写入回调
    auto writeCallback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto* resp = static_cast<std::string*>(userdata);
        resp->append(ptr, size * nmemb);
        return size * nmemb;
    };
    
    // 进度回调（支持中断）
    auto progressCallback = [](void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int {
        return ult::abortDownload.load() ? 1 : 0;
    };
    
    // 构建 multipart form
    curl_mime* mime = curl_mime_init(curl);
    if (!mime) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        socketExit();
        return false;
    }
    
    // 添加文件字段
    curl_mimepart* filePart = curl_mime_addpart(mime);
    curl_mime_name(filePart, "macro");
    curl_mime_filedata(filePart, filePath.c_str());
    
    // 添加 titleId 字段
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", titleId);
    curl_mimepart* tidPart = curl_mime_addpart(mime);
    curl_mime_name(tidPart, "titleid");
    curl_mime_data(tidPart, tidStr, CURL_ZERO_TERMINATED);
    
    // 添加 gameName 字段
    curl_mimepart* namePart = curl_mime_addpart(mime);
    curl_mime_name(namePart, "gamename");
    curl_mime_data(namePart, gameName.c_str(), CURL_ZERO_TERMINATED);
    
    // 设置请求
    curl_easy_setopt(curl, CURLOPT_URL, "https://macro.dokiss.cn/upload.php");
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, 16 * 1024L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, +progressCallback);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_mime_free(mime);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    socketExit();
    
    if (res != CURLE_OK) return false;
    
    // 解析 JSON 获取 code
    cJSON* root = cJSON_Parse(response.c_str());
    if (root) {
        cJSON* codeItem = cJSON_GetObjectItem(root, "code");
        if (codeItem && cJSON_IsString(codeItem)) {
            outCode = codeItem->valuestring;
            success = true;
        }
        cJSON_Delete(root);
    }
    
    return success;
}
