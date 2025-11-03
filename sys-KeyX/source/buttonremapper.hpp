#pragma once
#include <switch.h>
#include <vector>

// 按键映射结构体
struct ButtonMapping {
    const char* source;
    char target[8];
};

class ButtonRemapper {
public:
    // 设置手柄按键映射
    static Result SetMapping(const std::vector<ButtonMapping>& mappings);
    
    // 恢复默认按键配置
    static Result RestoreMapping();

private:
    // 查找按键枚举值（返回-1表示无效）
    static int FindButton(const char* name);
    
    // 应用映射（通用）
    template<typename ConfigType>
    static void ApplyMapping(ConfigType& config, const char* from, const char* to);
    
    // 应用映射到 Embedded 手柄
    static void ApplyMappingsToEmbedded(HidsysUniquePadId pad_id, const std::vector<ButtonMapping>& mappings);
    
    // 应用映射到 Full 手柄
    static void ApplyMappingsToFull(HidsysUniquePadId pad_id, const std::vector<ButtonMapping>& mappings);
};

