#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动将 ovl-AutoKeyLoop 标题替换为中文
"""
import sys

def patch_ovl(ovl_file):
    # 读取文件
    with open(ovl_file, 'rb') as f:
        data = f.read()
    
    # 原始标题: "ovl-AutoKeyLoop" (15字节)
    old_bytes = bytes.fromhex('6F766C2D4175746F4B65794C6F6F70')
    
    # 新标题: "按键连发" + 补零 (15字节)
    new_bytes = bytes.fromhex('E68C89E994AEE8BF9EE58F91000000')
    
    # 检查是否找到
    if old_bytes not in data:
        print("========================================")
        print("\033[91m警告: 未找到 'ovl-AutoKeyLoop' 字节序列\033[0m")
        return False
    
    # 替换
    new_data = data.replace(old_bytes, new_bytes)
    
    # 写回文件
    with open(ovl_file, 'wb') as f:
        f.write(new_data)
    
    print("========================================")
    print("\033[92m已将ovl标题修改为: 按键连发\033[0m")
    return True

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("用法: python ChineseTitle.py <ovl文件>")
        sys.exit(1)
    
    success = patch_ovl(sys.argv[1])
    sys.exit(0 if success else 1)

