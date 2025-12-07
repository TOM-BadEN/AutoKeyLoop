[🇨🇳 中文](#中文) | [EN English](#english)

https://github.com/user-attachments/assets/d7f530b9-baed-455c-9887-5b7a96a9dadf

---

<a name="中文"></a>
# 中文

**如果你对通知模块感兴趣，可以访问 [通知模块](https://github.com/TOM-BadEN/NX-Notification) NX-Notification**

## 注意

- **注意：因为SWITCH的底层特殊机制，部分功能表现可能有瑕疵，具体见下表。**
- 我并非程序员，已经尽力了，虽有瑕疵，但至少各功能属于能用的状态。
- 想要完美实现所有功能，需要通过大气层的MITM劫持来达到，但是我不会，而且需要完全重构。

## 当前功能瑕疵表现

| 功能 | Joycon | 三方分体（双子星2代） |JC(蓝牙) | LITE | 三方常规 (八爪鱼4) | PRO
|:------------:|:--------:|:------:|:------:|:------:|:------:|:------:|
| 按键连发 | 完美 ① | 完美 ① |不可用 | 完美 | 可用 ② | 未测试 ③ |
| 按键映射 | 完美 | 完美 | 完美 | 完美 | 完美 | 完美 |
| 按键宏 | 可用 ④ | 可用 ④ | 不可用 | 完美 | 可用 ④ | 未测试 ③ |

<table>
  <tr>
    <th align="center">组合功能</th>
    <th align="center">Joycon</th>
    <th align="center">三方分体（双子星2代）</th>
    <th align="center">JC(蓝牙)</th>
    <th align="center">LITE</th>
    <th align="center">三方常规 (八爪鱼4)</th>
    <th align="center">PRO</th>
  </tr>
  <tr>
    <td align="center">连发 + 映射</td>
    <td align="center">完美 ①</td>
    <td align="center">完美 ①</td>
    <td align="center">不可用</td>
    <td align="center">完美</td>
    <td align="center">可用 ②</td>
    <td align="center">未测试 ③</td>
  </tr>
  <tr>
    <td align="center">连发 + 宏</td>
    <td align="center" colspan="6">宏播放期间，自动屏蔽连发功能</td>
  </tr>
  <tr>
    <td align="center">映射 + 宏</td>
    <td align="center" colspan="6">如果播放的宏的按键，恰好是修改了映射的按键，可能出现输入不对的情况</td>
  </tr>
</table> 

**说明：**
- ① 仅右手柄支持连发。因为未知原因，若两侧都支持连发的话，会出现完全停止按键后，摇杆无法自动归0，需要手动碰一下才能恢复正常。
- ② 小概率完全停止按键后，摇杆无法自动归0，需要手动碰一下才能恢复正常。
- ③ 理论上与八爪鱼效果一致，但是我没有PRO手柄，所以只能说理论
- ④ 宏结束，有概率摇杆无法自动归0，需要手动碰一下才能恢复正常。


# KeyX 按键助手

[![platform](https://img.shields.io/badge/bilibili-玩家-FB7299?logo=bilibili&logoColor=FFFFFF)](https://www.bilibili.com/video/BV1u12cBvEmD/?spm_id_from=333.1387.homepage.video_card.click&vd_source=ee56f275632e70ae7ca2577aa1a56b81)
[![Latest Version](https://img.shields.io/github/v/release/TOM-BadEN/KeyX?label=Latest&color=blue&logo=github&logoColor=FFFFFF)](https://github.com/TOM-BadEN/KeyX/releases/latest)
[![GitHub Downloads](https://img.shields.io/github/downloads/TOM-BadEN/KeyX/total?label=Downloads&color=6f42c1&logo=github&logoColor=FFFFFF)](https://somsubhra.github.io/github-release-stats/?username=TOM-BadEN&repository=KeyX&page=1&per_page=300)
[![HB App Store](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/TOM-BadEN/KeyX/main/.github/hbappstore.json&label=HB%20store&color=6f42c1&logo=homeassistantcommunitystore&logoColor=FFFFFF)](https://hb-app.store/switch/KeyX)

Nintendo Switch 按键助手，支持连发、按键重新分配、按键宏三大功能。且拥有全局或游戏独立配置，根据记忆自动启动功能。
整个插件由特斯拉插件与系统模块两部分组成。

## 功能

![Tesla界面](image/tesla.jpg)       
![录制按键宏](image/recording.jpg)

- 美观现代的UI设计
- 可动态修改连发与映射按键
- 可使用特斯拉直接录制按键宏，且功能引导完善
- 可选择开启额外的通知弹窗
- 主页的蓝色图标代表该按键修改了映射
- 主页的黄色角标代表该按键启用了连发
- 主页的红色角标代表该按键绑定了宏

### 按键映射

- 支持 16 个按键互相映射 (A/B/X/Y/L/R/ZL/ZR/十字键/SELECT/START/L3/R3)
- 与连发功能可同时启用，不会有冲突
- **完美避开系统关于按键修改后的警告弹窗**
- 全局配置和游戏独立配置
- 自动记忆开关状态

### 按键连发 

- 支持 12 个按键连发（A/B/X/Y/L/R/ZL/ZR/十字键）
- 支持多个按键同时连发
- 连发时支持非连发键正常使用
- 可设置按下和松开时长
- 全局配置和游戏独立配置
- 自动记忆开关状态

### 按键宏

- 自动记忆宏功能开关状态
- 摇杆与按键状态均会被录制
- 最大录制时长为30s
- 录制帧率为60FPS
- 按一下对应快捷键为单次播放
- 按住对应快捷键为循环播放
- 播放期间再次按下快捷键取消播放

## 内存占用

- 系统模块仅占用 343 KB
- 弹窗额外占用 688 KB
- **弹窗只有触发的时候才有内存占用**

## 安装

将文件复制到 SD 卡根目录：
```
/atmosphere/contents/4100000002025924/
/atmosphere/contents/0100000000251020
/switch/.overlays/ovl-KeyX.ovl
```

## 多语言

- Chinese is hardcoded, no need to add
- I used AI to translate the English language file. 
- I don't understand other languages, and AI is even worse at it
- You can refer to en.json to add support for other languages

```
SUPPORTED LANGUAGES:
  - en.json       (English)
  - zh-cn.json    (No need to add, already hardcoded)
  - zh-tw.json    (Traditional Chinese)
  - ja.json       (Japanese)
  - ko.json       (Korean)
  - fr.json       (French)
  - de.json       (German)
  - it.json       (Italian)
  - es.json       (Spanish)
  - pt.json       (Portuguese)
  - ru.json       (Russian)
  - nl.json       (Dutch)
```

## 编译

```bash
cd sys-KeyX && make -j
cd ovl-KeyX && make -j
```
或者直接根目录

```bash
cd KeyX && make
```

## 感谢

- [libnx](https://github.com/switchbrew/libnx) - Switch 开发库
- [libultrahand](https://github.com/ppkantorski/libultrahand) - Tesla Overlay 框架
- [minIni-nx](https://github.com/ITotalJustice/minIni-nx) - INI 配置文件解析库


---

<a name="english"></a>
# English

**If you are interested in the notification module, visit [NX-Notification](https://github.com/TOM-BadEN/NX-Notification)**

## Notice

- **Note: Due to the special underlying mechanism of SWITCH, some functions may have flaws. See the table below for details.**
- I am not a professional programmer. I have tried my best. Although there are flaws, at least all functions are usable.
- To perfectly implement all functions, it requires MITM hijacking through Atmosphere, but I don't know how, and it would require a complete refactoring.

## Current Function Performance Issues

| Function | Joycon | 3rd-Party Split (MoPai Twin Star Gen2) | JC(Bluetooth) | LITE | 3rd-Party Regular (Octopus 4) | PRO
|:------------:|:--------:|:------:|:------:|:------:|:------:|:------:|
| Turbo | Perfect ① | Perfect ① | Not Available | Perfect | Available ② | Not Tested ③ |
| Key Mapping | Perfect | Perfect | Perfect | Perfect | Perfect | Perfect |
| Macro | Available ④ | Available ④ | Not Available | Perfect | Available ④ | Not Tested ③ |

<table>
  <tr>
    <th align="center">Combined Functions</th>
    <th align="center">Joycon</th>
    <th align="center">3rd-Party Split (MoPai Twin Star Gen2)</th>
    <th align="center">JC(Bluetooth)</th>
    <th align="center">LITE</th>
    <th align="center">3rd-Party Regular (Octopus 4)</th>
    <th align="center">PRO</th>
  </tr>
  <tr>
    <td align="center">Turbo + Mapping</td>
    <td align="center">Perfect ①</td>
    <td align="center">Perfect ①</td>
    <td align="center">Not Available</td>
    <td align="center">Perfect</td>
    <td align="center">Available ②</td>
    <td align="center">Not Tested ③</td>
  </tr>
  <tr>
    <td align="center">Turbo + Macro</td>
    <td align="center" colspan="6">Turbo function temporarily disabled during macro playback</td>
  </tr>
  <tr>
    <td align="center">Mapping + Macro</td>
    <td align="center" colspan="6">If the keys being played have been remapped, incorrect input may occur</td>
  </tr>
</table> 

**Notes:**
- ① Only the right Joy-Con supports turbo. For unknown reasons, if both sides support turbo, the joystick may fail to auto-reset after completely stopping button presses, requiring manual touch to restore.
- ② Low probability that after completely stopping button presses, the joystick may fail to auto-reset, requiring manual touch to restore.
- ③ Theoretically the same effect as Octopus, but I don't have a PRO controller, so it's just theoretical.
- ④ After macro ends, there's a probability the joystick may fail to auto-reset, requiring manual touch to restore.

# KeyX Button Assistant

[![platform](https://img.shields.io/badge/bilibili-玩家-FB7299?logo=bilibili&logoColor=FFFFFF)](https://www.bilibili.com/video/BV1u12cBvEmD/?spm_id_from=333.1387.homepage.video_card.click&vd_source=ee56f275632e70ae7ca2577aa1a56b81)
[![Latest Version](https://img.shields.io/github/v/release/TOM-BadEN/KeyX?label=Latest&color=blue&logo=github&logoColor=FFFFFF)](https://github.com/TOM-BadEN/KeyX/releases/latest)
[![GitHub Downloads](https://img.shields.io/github/downloads/TOM-BadEN/KeyX/total?label=Downloads&color=6f42c1&logo=github&logoColor=FFFFFF)](https://somsubhra.github.io/github-release-stats/?username=TOM-BadEN&repository=KeyX&page=1&per_page=300)
[![HB App Store](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/TOM-BadEN/KeyX/main/.github/hbappstore.json&label=HB%20store&color=6f42c1&logo=homeassistantcommunitystore&logoColor=FFFFFF)](https://hb-app.store/switch/KeyX)

Nintendo Switch button assistant supporting turbo, key remapping, and macro recording. Features global or per-game configuration with auto-start memory.
The complete plugin consists of Tesla overlay and system module.

## Features

![Tesla UI](image/tesla.jpg)       
![Macro Recording](image/recording.jpg)

- Beautiful and modern UI design
- Dynamically modify turbo and mapping settings
- Record macros directly using Tesla overlay with comprehensive guidance
- Optional notification popups
- Blue icons on home page indicate remapped buttons
- Yellow badges indicate turbo-enabled buttons
- Red badges indicate macro-bound buttons

### Key Mapping

- Remap 16 buttons (A/B/X/Y/L/R/ZL/ZR/D-pad/SELECT/START/L3/R3)
- Works together with turbo without conflicts
- **Perfectly avoids system warning popups about button changes**
- Global and per-game configuration
- Auto-remembers on/off state

### Turbo

- Turbo for 12 buttons (A/B/X/Y/L/R/ZL/ZR/D-pad)
- Multiple buttons can turbo simultaneously
- Non-turbo buttons work normally during turbo
- Customizable press and release duration
- Global and per-game configuration
- Auto-remembers on/off state

### Macro

- Auto-remembers macro function on/off state
- Both stick and button states are recorded
- Maximum recording duration: 30 seconds
- Recording frame rate: 60 FPS
- Press shortcut key once for single playback
- Hold shortcut key for loop playback
- Press shortcut key again during playback to cancel

## Memory Usage

- System module: only 343 KB
- Notification popup: extra 688 KB
- **Popup only uses memory when triggered**

## Installation

Copy files to SD card root:

```
/atmosphere/contents/4100000002025924/
/atmosphere/contents/0100000000251020
/switch/.overlays/ovl-KeyX.ovl
```

## Multi-Language

- Chinese is hardcoded, no need to add
- I used AI to translate the English language file
- I don't understand other languages, and AI is even worse at it
- You can refer to en.json to add support for other languages

```
SUPPORTED LANGUAGES:
  - en.json       (English)
  - zh-cn.json    (No need to add, already hardcoded)
  - zh-tw.json    (Traditional Chinese)
  - ja.json       (Japanese)
  - ko.json       (Korean)
  - fr.json       (French)
  - de.json       (German)
  - it.json       (Italian)
  - es.json       (Spanish)
  - pt.json       (Portuguese)
  - ru.json       (Russian)
  - nl.json       (Dutch)
```

## Build

```bash
cd sys-KeyX && make -j
cd ovl-KeyX && make -j
```

Or from root directory:

```bash
cd KeyX && make
```

## Credits

- [libnx](https://github.com/switchbrew/libnx) - Switch development library
- [libultrahand](https://github.com/ppkantorski/libultrahand) - Tesla Overlay framework
- [minIni-nx](https://github.com/ITotalJustice/minIni-nx) - INI config parser library
