#pragma once

#include <string>
#include <switch.h>

class Network {
public:
    static bool download(const std::string& url, const std::string& toPath);
    static bool download2(const std::string& url, const std::string& toPath);
    static bool upload(const std::string& filePath, u64 titleId, const std::string& gameName, std::string& outCode);
};
