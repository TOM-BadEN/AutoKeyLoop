#pragma once
#include <string>

/**
 * 语言管理类 - 提供语言自动检测和加载功能
 */
class LanguageManager {
public:
    /**
     * 初始化语言系统
     * 
     * 工作流程：
     * 1. 获取 Switch 系统语言
     * 2. 检查对应语言文件是否存在
     * 3. 加载语言文件或降级到默认语言
     * 4. 如果是中文系统或无语言文件，使用代码中的中文
     */
    static void initialize();
    
    /**
     * 获取系统语言文件名
     * @return 语言文件名（如 "zh-cn.json", "en.json", "ja.json"）
     */
    static std::string getSystemLanguageCode();
};

