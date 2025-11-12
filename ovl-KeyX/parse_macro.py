#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
宏文件解析脚本
解析 KeyX 录制的宏文件并输出可读信息
"""

import struct
import sys

def parse_macro_file(filename):
    """解析宏文件并返回信息"""
    
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
            frame_rate = struct.unpack('<H', header_data[6:8])[0]
            title_id = struct.unpack('<Q', header_data[8:16])[0]
            frame_count = struct.unpack('<I', header_data[16:20])[0]
            
            # 计算时长
            duration = frame_count / frame_rate if frame_rate > 0 else 0
            
            # 构建输出信息
            output = []
            output.append("=" * 60)
            output.append("KeyX 宏文件解析结果")
            output.append("=" * 60)
            output.append("")
            output.append(f"文件名：{filename}")
            output.append(f"魔数：{magic}")
            output.append(f"版本：{version}")
            output.append(f"帧率：{frame_rate} fps")
            output.append(f"游戏TID：0x{title_id:016X}")
            output.append(f"总帧数：{frame_count}")
            output.append(f"录制时长：{duration:.2f} 秒（{duration/60:.1f} 分钟）")
            output.append(f"文件大小：{20 + frame_count * 24} 字节")
            output.append("")
            output.append("=" * 60)
            output.append("帧数据详情")
            output.append("=" * 60)
            output.append("")
            
            # 读取所有帧
            frames_read = 0
            
            for i in range(frame_count):
                frame_data = f.read(24)
                if len(frame_data) < 24:
                    output.append(f"\n警告: 第{i}帧数据不完整，读取终止")
                    break
                
                # 解析帧数据
                # keysHeld(u64), leftX(s32), leftY(s32), rightX(s32), rightY(s32)
                keys_held = struct.unpack('<Q', frame_data[0:8])[0]
                left_x = struct.unpack('<i', frame_data[8:12])[0]
                left_y = struct.unpack('<i', frame_data[12:16])[0]
                right_x = struct.unpack('<i', frame_data[16:20])[0]
                right_y = struct.unpack('<i', frame_data[20:24])[0]
                
                frames_read += 1
                
                # 检测是否为空帧（无任何操作）
                is_empty = (keys_held == 0 and left_x == 0 and left_y == 0 
                           and right_x == 0 and right_y == 0)
                
                if is_empty:
                    # 空帧简化输出
                    output.append(f"帧 {i:4d}（时间 {i/60:.3f}s）： 无操作")
                else:
                    # 解码按键
                    key_desc = decode_keys(keys_held) if keys_held > 0 else "无按键"
                    
                    # 第一行：帧号+时间+按键摘要
                    output.append(f"帧 {i:4d}（时间 {i/60:.3f}s）： {key_desc}")
                    # 详细信息
                    output.append(f"  按键状态：0x{keys_held:016X}")
                    output.append(f"  左摇杆：（{left_x:6d}，{left_y:6d}）")
                    output.append(f"  右摇杆：（{right_x:6d}，{right_y:6d}）")
                output.append("")
            
            # 统计信息
            output.append("=" * 60)
            output.append("统计信息")
            output.append("=" * 60)
            output.append(f"成功读取帧数：{frames_read}/{frame_count}")
            
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
        0x000400: "START",
        0x000800: "SELECT",
        # 十字键
        0x001000: "左",
        0x002000: "上",
        0x004000: "右",
        0x008000: "下",
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
    
    return " + ".join(pressed) if pressed else "无"

if __name__ == "__main__":
    # 目标文件
    input_file = r"D:\msys64\home\18084\KeyX\out\CN\macro_1762909346.macro"
    output_file = r"D:\msys64\home\18084\KeyX\out\CN\macro_1762909346.log"
    
    print(f"解析文件: {input_file}")
    
    # 解析文件
    result = parse_macro_file(input_file)
    
    # 写入日志文件
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(result)
        print(f"结果已保存到: {output_file}")
        print("\n" + "=" * 60)
        print(result)
    except Exception as e:
        print(f"写入日志文件失败: {e}")
        print("\n解析结果:")
        print(result)
