# DferTune

DferTune 是 Nintendo Switch 上的后台音乐播放模块(sysmodule),可以在游戏和系统界面中播放你自己放在 SD 卡上的音频文件,并通过 Tesla 悬浮菜单控制播放。

本项目是 [Dimasick-git/RyazhaTune](https://github.com/Dimasick-git/RyazhaTune) 的中文分支,而 RyazhaTune 又派生自 [HookedBehemoth/sys-tune](https://github.com/HookedBehemoth/sys-tune)。本分支默认使用简体中文界面,并改用独立的名称、配置目录和 Title ID,可以与上游版本并存。

[![最新版本](https://img.shields.io/github/v/release/dfer-site/DferTune?display_name=tag&label=%E7%89%88%E6%9C%AC&style=for-the-badge)](https://github.com/dfer-site/DferTune/releases/latest)
[![下载次数](https://img.shields.io/github/downloads/dfer-site/DferTune/total?label=%E4%B8%8B%E8%BD%BD&style=for-the-badge)](https://github.com/dfer-site/DferTune/releases)
![访问次数](https://visitor-badge.laobi.icu/badge?page_id=dfer-site.DferTune&left_text=%E8%AE%BF%E9%97%AE)

> 下载次数来自 GitHub Releases 的统计数据;访问次数由第三方徽章服务提供。

网站:http://www.dfer.site

## 功能

- **播放列表持久化**:重启主机后播放列表依然保留。
- **开机自动播放**:可设置系统启动后自动开始播放,并可选择等到进入主页后再播放。
- **按游戏过滤**:`Normal`(普通)/ `Whitelist`(白名单)/ `Blacklist`(黑名单)三种模式,可在悬浮菜单中针对每个游戏单独设置。
- **灵活的焦点策略**:全局"游戏焦点"加上每个游戏的"自定义焦点"(播放 / 暂停 / 跳过),并支持单独设置 HOME 键行为。
- **自动暂停**:软键盘、手柄配对弹窗、锁屏时可自动暂停。
- **多语言界面**:内置多种语言,默认简体中文,可在悬浮菜单中切换。
- **音频格式**:内置 MP3、FLAC、WAV 解码。
- **5 段均衡器**:100 Hz、300 Hz、1 kHz、3 kHz、10 kHz 五个频段,实时生效并带有预设。可作用于 DferTune 播放的音乐(软件 DSP),也可作用于游戏/系统输出(模拟输出的硬件 EQ,底座 HDMI 输出不支持)。
- **Tesla 悬浮菜单**:所有操作都在悬浮菜单中完成。

## 安装

1. 从本仓库的 [Releases](https://github.com/dfer-site/DferTune/releases) 页面下载最新的 `.zip` 压缩包。
2. 将压缩包内容解压到 SD 卡根目录。
3. 把 MP3、FLAC、WAV 等音频文件放到 SD 卡上。
4. 呼出 Tesla 悬浮菜单,打开 DferTune 控制播放和设置。

提示:DferTune 使用独立的 Title ID(`0x420000000000000F`)和配置目录(`/config/DferTune/`),不会覆盖原版 sys-tune 或 RyazhaTune。同时运行多个后台音乐模块会互相争抢音频,建议只启用其中一个。

## 项目结构

- **DferTune/**:后台系统模块,负责解码并通过 `audren` 服务播放音频。
- **overlay/**:Tesla 悬浮菜单插件(`.ovl`),提供图形化控制界面。
- **ipc/**:悬浮菜单与系统模块之间的 IPC 接口定义。
- **common/**:配置读写(`minIni`)、SD 卡访问、进程管理等公共代码。
- **tests/**、**scripts/**:均衡器单元测试与校验脚本。

## 配置

配置文件在首次使用时自动生成,位于 `/config/DferTune/`:

- `config.ini`:主配置(自动播放、音量、随机、重复、语言、按游戏过滤、`equalizer` 段等)。
- `config.ini` 内的 `whitelist` / `blacklist` 段:按游戏的白名单、黑名单标记。
- `saved_playlist.txt`:持久化的播放列表。
- `play_source.txt`:上次的播放来源(`Playlist` 或 `Folder:<路径>`)。

### 过滤模式(TuneMode)

- `Normal`:不按 Title ID 过滤。
- `Whitelist`:只在白名单内的 Title ID 播放音乐。
- `Blacklist`:黑名单内的 Title ID 不播放音乐。

## 从源码构建

### 环境要求

- 安装 [devkitPro](https://devkitpro.org/) 及 `switch-dev` 软件包组。
- 安装 GNU Make。
- 系统中可用 `python3`。
- 正确设置 `DEVKITPRO` 环境变量。

### 标准构建

编译系统模块和悬浮菜单:

```bash
make all
```

生成可直接安装的目录结构和压缩包:

```bash
make dist
```

### 扩展构建(Horizon-OC)

需要集成 Horizon-OC 并对 Atmosphere 打补丁时:

1. 运行自动配置脚本:
   ```bash
   chmod +x setup_build_env.sh
   ./setup_build_env.sh
   ```
2. 脚本会自动克隆所需仓库(Atmosphere、Horizon-OC),应用 `ldr_process_creation.cpp` 补丁并开始构建。

## 致谢

- [HookedBehemoth/sys-tune](https://github.com/HookedBehemoth/sys-tune):原版 sys-tune 的作者,本项目的基础。
- [Dimasick-git/RyazhaTune](https://github.com/Dimasick-git/RyazhaTune):本项目直接派生自该分支,带来了播放列表持久化、均衡器、过滤模式等大量功能。
- [WerWolv/libtesla](https://github.com/WerWolv/libtesla):Tesla 悬浮菜单使用的界面库。
- [mackron/dr_libs](https://github.com/mackron/dr_libs):本项目使用的音频解码库。

## 版本信息

- **当前版本:** <!-- CURRENT_VERSION_START -->5.6.0-dfer.1<!-- CURRENT_VERSION_END -->
- **状态:** 稳定

## 许可证

本项目遵循 GNU General Public License Version 2(GPLv2),详见 `LICENSE` 文件。内置的 `libtesla` 组件同样使用 GPLv2。根据 GPLv2,分发编译产物时必须同时提供对应源码。
