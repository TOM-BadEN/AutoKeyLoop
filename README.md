# AutoKeyLoop

Nintendo Switch 按键连发插件，支持全局和游戏独立配置。
由特斯拉插件与系统模块两部分组成。

## 功能

- 支持 12 个按键连发（A/B/X/Y/L/R/ZL/ZR/十字键）
- 支持多个按键同时连发
- 连发时支持非连发键正常使用
- 可设置按下和松开时长
- 全局配置和游戏独立配置
- 美观现代的特斯拉插件

## 内存占用

- 待机占用 xxx KB
- 开启连发功能占用 xxx KB

## 注意

- 如需彻底关闭系统模块，请使用本项目的特斯拉插件进行关闭。
- sys-modules插件关闭是直接KILL的，会导致资源无法释放。

## 安装

将文件复制到 SD 卡根目录：
```
/atmosphere/contents/4100000002025924/
/switch/.overlays/ovl-AutoKeyLoop.ovl
```

## 编译

```bash
cd sys-AutoKeyLoop && make
cd ovl-AutoKeyLoop && make
```
或者直接根目录

```bash
cd AutoKeyLoop && make
```

## 许可证

MIT License

本项目仅供学习和研究使用。

## 感谢

- [libnx](https://github.com/switchbrew/libnx) - Switch 开发库
- [libultrahand](https://github.com/ppkantorski/libultrahand) - Tesla Overlay 框架
- [minIni-nx](https://github.com/ITotalJustice/minIni-nx) - INI 配置文件解析库
