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

我的OLED+JC将在这几天到货，到时候我会进行详细的测试，以及尝试修复，但是据我目前的了解以及之前的通过别人测试， 我来修复的失败经验来看，JC的相关问题修复的概率很低。

| 功能 | Joycon | LITE |
|------|--------|------|
| 按键连发 | 只按连发完美，但若在连发过程中进行别的操作，融合的不好 | 完美 |
| 按键映射 | 完美 | 完美 |
| 按键宏 | 宏结束后，可能出现摇杆无法归位的情况，碰一下摇杆恢复正常 | 完美 |

| 组合功能 | Joycon | LITE |
|----------|--------|------|
| 连发+映射 | 同按键连发 | 完美 |
| 连发+宏 | 宏播放期间插件将暂时屏蔽连发功能 | 宏播放期间插件将暂时屏蔽连发功能 |
| 映射+宏 | 如果播放的宏的按键，恰好是修改了映射的按键，可能出现输入不对的情况 | 如果播放的宏的按键，恰好是修改了映射的按键，可能出现输入不对的情况 | 

## 项目实现原理

通过在'HID内存'上，申请一块'HDLs的缓冲区'。
使用'hidGetNpadStatesHandheld'等API，读取手柄的按键与摇杆数据（实际上就是从HID内存读取），
进过算法判断，修改按键与摇杆数据，
再使用'hiddbgApplyHdlsStateList'注入回HID内存（此时按键生效）。

但是该方法存在以下问题。

1. 注入的按键和摇杆数据，会被hidGetNpadStatesHandheld再次读取到，从而导致无法读取到真正的用户输入。不过好在通过算法控制，勉强的让程序能正确的运行起来。目前JC存在上面表格的问题，LITE则表现完美。

2. JOYCON不知道存在什么特殊机制，如果注入摇杆或者其他按键，就会导致即使代码停止注入，用户完全不碰手柄，但是摇杆仍旧一直触发，通过读取发现一直触发的最后注入的数据，但是对于SWITCH LITE就没有这个问题，这就非常奇怪，不知道到底是什么原因。为了解决这个问题，我暂时强制JC注入的时候强制将非连发按键以及摇杆修改为0，但是这会导致在连发期间无法正常的进行其他按键的操作。

3. 通过标准的API，修改按键映射后，如X->Y,Y->X。接着我们持续注入Y，但是游戏中实际表现为Y中掺入了X。针对连发功能，采用了逆映射的算法，修正了这个问题，但是导致问题的机制，仍然搞不清楚。而针对宏功能，则完全无论能力，因此正如上表所说，如果播放的宏的按键，恰好是修改了映射的按键，可能出现输入不对的情况。

**如果有大佬知道如何解决，或者导致问题的根本原因，还请劳烦传道一下。**

目前已知的可能的完美解决办法，就是改用AMS的MITM，通过拦截游戏对HID内存的访问，设置一个影子内存，让游戏只访问完全受我们控制的影子内存。但是我不是程序员，而且MITM太复杂了，没有模板和文档，我无能为力。


# KeyX 按键助手

[![bilibili](https://img.shields.io/badge/゚゚゙゚゚゚゚玩家?logo=C++.svg)](https://www.bilibili.com/video/BV1u12cBvEmD/?spm_id_from=333.1387.homepage.video_card.click&vd_source=ee56f275632e70ae7ca2577aa1a56b81)
[![Latest Version](https://img.shields.io/github/v/release/TOM-BadEN/KeyX?label=latest&color=blue)](https://github.com/TOM-BadEN/KeyX/releases/latest)
[![GitHub Downloads](https://img.shields.io/github/downloads/TOM-BadEN/KeyX/total?color=6f42c1)](https://somsubhra.github.io/github-release-stats/?username=TOM-BadEN&repository=KeyX&page=1&per_page=300)

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

My OLED+JC will arrive in the next few days. I will conduct detailed testing and attempt to fix the issues, but based on what I currently know and previous failed attempts, both mine and others’ tests, the probability of successfully fixing the JC-related problems is very low.

| Function | Joycon | LITE |
|----------|--------|------|
| Turbo | Pressing only the turbo button works fine, but inputting other buttons or using the joystick during turbo performs poorly (to be fixed) | Perfect |
| Key Mapping | Perfect | Perfect |
| Macro | After macro ends, stick may fail to reset, touch the stick to restore | Perfect |

| Combined Functions | Joycon | LITE |
|--------------------|--------|------|
| Turbo + Mapping | Same as Turbo | Perfect |
| Turbo + Macro | Turbo function temporarily disabled during macro playback | Turbo function temporarily disabled during macro playback |
| Mapping + Macro | If the keys being played have been remapped, incorrect input may occur | If the keys being played have been remapped, incorrect input may occur |

## Project Implementation Principle

By allocating a 'HDLs buffer' on 'HID memory'.
Using APIs like 'hidGetNpadStatesHandheld' to read button and stick data from the controller (which actually reads from HID memory),
processing the data through algorithms to modify button and stick data,
then using 'hiddbgApplyHdlsStateList' to inject it back into HID memory (where the buttons take effect).

However, this method has the following issues:

1. The injected button and joystick data can be read again by hidGetNpadStatesHandheld, which prevents the program from reading the actual user input. Fortunately, through algorithmic control, the program can still run correctly, albeit imperfectly. Currently, the JC has the issues listed in the table above, while the LITE performs perfectly.

2. JOY-CON seems to have some unknown special mechanism: if joystick or other buttons are injected, even when the code stops injecting and the user does not touch the controller at all, the joystick continues to trigger. Reading the input shows that the last injected data keeps being read. This issue does not occur on the SWITCH LITE, which is very strange and the cause is unclear. To work around this problem, I temporarily force the JC to set non-Tubro buttons and the joystick to 0 during injection, but this prevents other button operations from working normally while Tubro is active.

3. Using the standard API, after remapping buttons, for example X → Y and Y → X, if we continuously inject Y, the game actually registers Y mixed with X. For the Tubro function, a reverse-mapping algorithm is used to correct this issue, but the underlying cause remains unclear. As for the macro function, it is completely powerless against this problem. Therefore, as mentioned in the table above, if a macro plays a button that happens to be remapped, the input may be incorrect.

**If any experts know how to solve this or the root cause, please enlighten me.**

Currently, the only known perfect solution is to use AMS MITM, by intercepting the game's access to HID memory and setting up a shadow memory, so the game only accesses the shadow memory that we fully control. However, I'm not a programmer, and MITM is too complex with no templates or documentation, so I'm unable to do it.

# KeyX Button Assistant

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
