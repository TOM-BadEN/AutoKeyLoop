#pragma once
#include <string>
#include <cstdlib>
#include <ultra.hpp>

/**
 * INI 配置文件辅助类
 * 封装 libultrahand 的 INI 功能，提供类似 minIni 的简洁接口
 */
namespace IniHelper {
    
    /**
     * 读取字符串值
     * @param section   Section 名称
     * @param key       键名
     * @param defValue  默认值（键不存在时返回）
     * @param filePath  INI 文件路径
     * @return 读取到的字符串值，失败则返回默认值
     */
    inline std::string getString(const std::string& section, 
                                  const std::string& key, 
                                  const std::string& defValue,
                                  const std::string& filePath) {
        std::string value = ult::parseValueFromIniSection(filePath, section, key);
        return value.empty() ? defValue : value;
    }
    
    /**
     * 读取整数值
     * @param section   Section 名称
     * @param key       键名
     * @param defValue  默认值（键不存在时返回）
     * @param filePath  INI 文件路径
     * @return 读取到的整数值，失败则返回默认值
     */
    inline int getInt(const std::string& section, 
                      const std::string& key, 
                      int defValue,
                      const std::string& filePath) {
        std::string value = ult::parseValueFromIniSection(filePath, section, key);
        if (value.empty()) {
            return defValue;
        }
        return std::atoi(value.c_str());
    }
    
    /**
     * 读取布尔值
     * @param section   Section 名称
     * @param key       键名
     * @param defValue  默认值（键不存在时返回）
     * @param filePath  INI 文件路径
     * @return 读取到的布尔值，失败则返回默认值
     */
    inline bool getBool(const std::string& section, 
                        const std::string& key, 
                        bool defValue,
                        const std::string& filePath) {
        std::string value = ult::parseValueFromIniSection(filePath, section, key);
        if (value.empty()) {
            return defValue;
        }
        // 支持多种布尔值表示：true/false, 1/0, yes/no, on/off
        if (value == "1" || value == "true" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "0" || value == "false" || value == "no" || value == "off") {
            return false;
        }
        return defValue;
    }
    
    /**
     * 写入字符串值
     * @param section   Section 名称
     * @param key       键名
     * @param value     要写入的值
     * @param filePath  INI 文件路径
     */
    inline void setString(const std::string& section, 
                          const std::string& key, 
                          const std::string& value,
                          const std::string& filePath) {
        ult::setIniFileValue(filePath, section, key, value);
    }
    
    /**
     * 写入整数值
     * @param section   Section 名称
     * @param key       键名
     * @param value     要写入的值
     * @param filePath  INI 文件路径
     */
    inline void setInt(const std::string& section, 
                       const std::string& key, 
                       int value,
                       const std::string& filePath) {
        ult::setIniFileValue(filePath, section, key, std::to_string(value));
    }
    
    /**
     * 写入布尔值
     * @param section   Section 名称
     * @param key       键名
     * @param value     要写入的值
     * @param filePath  INI 文件路径
     */
    inline void setBool(const std::string& section, 
                        const std::string& key, 
                        bool value,
                        const std::string& filePath) {
        ult::setIniFileValue(filePath, section, key, value ? "true" : "false");
    }
    
    /**
     * 删除键
     * @param section   Section 名称
     * @param key       键名
     * @param filePath  INI 文件路径
     */
    inline void removeKey(const std::string& section, 
                          const std::string& key,
                          const std::string& filePath) {
        ult::removeIniKey(filePath, section, key);
    }
    
    /**
     * 删除整个 Section
     * @param section   Section 名称
     * @param filePath  INI 文件路径
     */
    inline void removeSection(const std::string& section,
                              const std::string& filePath) {
        ult::removeIniSection(filePath, section);
    }
}

