#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
V2 宏文件解析脚本
解析 KeyX V2 格式的宏文件并输出可读信息
"""

import struct
import sys

def parse_macro_v2_file(filename):
    """解析V2宏文件并返回信息"""
    
    try:
        with open(filename, 'rb') as f:
            # 读取文件头 (20字节)
            header_data = f.read(20)
            if len(header_data) < 20:
                return "错误: 文件头不完整"
            
            # 解析文件头
            # magic[4], version(u16), frameRate(u16), titleId(u64), frameCount(u32)
            magic = header_data[0:4].decode('ascii', errors='ignore')
            version = struct.unpack('<H', header_data[4:6])[0]
            frame_rate = struct.unpack('<H', header_data[6:8])[0]  # V2中未使用
            title_id = struct.unpack('<Q', header_data[8:16])[0]
            frame_count = struct.unpack('<I', header_data[16:20])[0]
            
            # 检查版本
            if version != 2:
                return f"错误: 不是V2格式文件（版本={version}）"
            
            # 构建输出信息
            output = []
            output.append("=" * 80)
            output.append("KeyX V2 宏文件解析结果")
            output.append("=" * 80)
            output.append("")
            output.append(f"文件名：{filename}")
            output.append(f"魔数：{magic}")
            output.append(f"版本：{version}")
            output.append(f"帧率：{frame_rate} fps（V2未使用）")
            output.append(f"游戏TID：0x{title_id:016X}")
            output.append(f"总帧数：{frame_count}")
            output.append(f"文件大小：{20 + frame_count * 28} 字节（预期）")
            output.append("")
            output.append("=" * 80)
            output.append("帧数据详情")
            output.append("=" * 80)
            output.append("")
            
            # 读取所有帧
            frames_read = 0
            total_duration_ms = 0
            
            for i in range(frame_count):
                frame_data = f.read(28)  # V2帧数据是28字节
                if len(frame_data) < 28:
                    output.append(f"\n警告: 第{i}帧数据不完整，读取终止")
                    break
                
                # 解析V2帧数据
                # durationMs(u32), keysHeld(u64), leftX(s32), leftY(s32), rightX(s32), rightY(s32)
                duration_ms = struct.unpack('<I', frame_data[0:4])[0]
                keys_held = struct.unpack('<Q', frame_data[4:12])[0]
                left_x = struct.unpack('<i', frame_data[12:16])[0]
                left_y = struct.unpack('<i', frame_data[16:20])[0]
                right_x = struct.unpack('<i', frame_data[20:24])[0]
                right_y = struct.unpack('<i', frame_data[24:28])[0]
                
                frames_read += 1
                total_duration_ms += duration_ms
                
                # 解码按键
                key_desc = decode_keys(keys_held) if keys_held > 0 else ""
                
                # 解析摇杆
                left_stick = f"左摇杆({left_x:6d},{left_y:6d})" if (left_x != 0 or left_y != 0) else ""
                right_stick = f"右摇杆({right_x:6d},{right_y:6d})" if (right_x != 0 or right_y != 0) else ""
                
                # 组合描述
                components = []
                if left_stick:
                    components.append(left_stick)
                if right_stick:
                    components.append(right_stick)
                if key_desc:
                    components.append(f"按键[{key_desc}]")
                
                if components:
                    desc = "，".join(components)
                    output.append(f"帧 {i:4d}：{desc}，持续时间{duration_ms:5d}ms")
                else:
                    # 空帧
                    output.append(f"帧 {i:4d}：无操作，持续时间{duration_ms:5d}ms")
            
            output.append("")
            # 统计信息
            output.append("=" * 80)
            output.append("统计信息")
            output.append("=" * 80)
            output.append(f"成功读取帧数：{frames_read}/{frame_count}")
            output.append(f"总持续时间：{total_duration_ms}ms（{total_duration_ms/1000:.2f}秒，{total_duration_ms/60000:.2f}分钟）")
            
            return "\n".join(output)
            
    except FileNotFoundError:
        return f"错误：文件不存在 - {filename}"
    except Exception as e:
        return f"错误：{str(e)}"

def decode_keys(keys):
    """解码按键状态为可读字符串"""
    key_names = {
        # 基础按键
        0x000001: "A",
        0x000002: "B",
        0x000004: "X",
        0x000008: "Y",
        0x000010: "L3",
        0x000020: "R3",
        0x000040: "L",
        0x000080: "R",
        0x000100: "ZL",
        0x000200: "ZR",
        0x000400: "+",
        0x000800: "-",
        # 十字键
        0x001000: "左",
        0x002000: "上",
        0x004000: "右",
        0x008000: "下",
        # 摇杆伪按键（V2中可能存在）
        0x010000: "L↑",
        0x020000: "L↓",
        0x040000: "L←",
        0x080000: "L→",
        0x100000: "R↑",
        0x200000: "R↓",
        0x400000: "R←",
        0x800000: "R→",
    }
    
    pressed = []
    remaining = keys
    
    # 解析已知按键
    for mask, name in key_names.items():
        if keys & mask:
            pressed.append(name)
            remaining &= ~mask  # 清除已识别的位
    
    # 检测未知按键
    if remaining != 0:
        pressed.append(f"未知(0x{remaining:X})")
    
    return "+".join(pressed) if pressed else "无"

if __name__ == "__main__":
    # 默认文件路径（可以通过命令行参数覆盖）
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    else:
        input_file = r"D:\msys64\home\18084\KeyX\out\CN\test.macro"
    
    # 输出文件名
    output_file = input_file.rsplit('.', 1)[0] + "_v2.log"
    
    print(f"解析V2文件: {input_file}")
    
    # 解析文件
    result = parse_macro_v2_file(input_file)
    
    # 写入日志文件
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(result)
        print(f"结果已保存到: {output_file}")
        print("\n" + "=" * 80)
        print(result[:2000] + "\n...(更多内容请查看日志文件)")
    except Exception as e:
        print(f"写入日志文件失败: {e}")
        print("\n解析结果:")
        print(result)
