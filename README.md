# DferTune

DferTune 是 Nintendo Switch 的后台音乐播放器(sysmodule + Tesla 悬浮菜单),支持 MP3 / FLAC / WAV、5 段均衡器、播放列表保存与按游戏过滤。可以在游戏和系统界面中播放你放在 SD 卡上的音频文件。

本项目是 [Dimasick-git/RyazhaTune](https://github.com/Dimasick-git/RyazhaTune) 的简体中文分支,而 RyazhaTune 又派生自 [HookedBehemoth/sys-tune](https://github.com/HookedBehemoth/sys-tune)。界面仅保留简体中文和英文,默认简体中文;并使用独立的名称、配置目录和 Title ID,可以与上游版本并存。

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
- **界面语言**:仅保留简体中文和英文,默认简体中文,可在悬浮菜单中切换。
- **音频格式**:内置 MP3、FLAC、WAV 解码。
- **5 段均衡器**:100 Hz、300 Hz、1 kHz、3 kHz、10 kHz 五个频段,实时生效并带有预设。可作用于 DferTune 播放的音乐(软件 DSP),也可作用于游戏/系统输出(模拟输出的硬件 EQ,底座 HDMI 输出不支持)。
- **Tesla 悬浮菜单**:所有操作都在悬浮菜单中完成。

## 安装

### 前置条件

- 已安装 [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere) 自制系统。
- 已安装 Tesla 悬浮菜单环境(`nx-ovlloader` 加 Tesla Menu 或 Ultrahand 等)。DferTune 的控制界面是 Tesla 悬浮菜单插件,没有它就无法打开。

### 发布包里有什么

[Releases](https://github.com/dfer-site/DferTune/releases) 页面有两个文件:

| 文件 | 用途 |
|---|---|
| `DferTune.zip` | **完整安装包**,首次安装请下载这个。包含后台模块、悬浮菜单、语言文件和默认配置。 |
| `DferTune-Overlay.ovl` | 只有悬浮菜单插件。仅在你已经装好完整包、只想单独更新悬浮菜单时使用。 |

### 首次安装

1. 下载 `DferTune.zip`,解压到 SD 卡**根目录**。解压后会得到:
   ```
   atmosphere/contents/420000000000000F/   后台模块(含 boot2.flag,开机自启)
   config/DferTune/                        默认配置和语言文件
   switch/.overlays/DferTune-Overlay.ovl   悬浮菜单插件
   ```
2. 把音频文件放到 SD 卡的 `/music/` 目录(建议),支持 `.mp3`、`.flac`、`.wav`、`.wave`。DferTune **不会自动扫描**整张 SD 卡,需要在悬浮菜单的「浏览」里手动添加。
3. **完整重启** Atmosphère(关机后重新进入系统),让后台模块随系统启动。

### 使用悬浮菜单(DferTune-Overlay.ovl)

`.ovl` 不是独立程序,不能直接运行,需要由 Tesla 加载:

1. 在系统或游戏中,按 Tesla 的呼出组合键打开 Tesla 菜单。默认组合键因 Tesla 版本和设置而不同(常见为 `L + ↓ + 右摇杆按下`),可以在 Tesla 菜单的设置里查看或修改。
2. 在列表中选择 **DferTune** 进入控制界面。
3. 首次使用:进入「浏览」,找到音乐所在文件夹,选择文件后「添加到列表」,再到「播放列表」页开始播放。添加后的播放列表会保存,重启后仍在。
4. 在「设置」里可以调整音量、随机/重复、均衡器、开机自动播放、按游戏过滤和界面语言(简体中文 / English)。

### 浏览与添加音乐的规则

- 「浏览」从 `/music/` 开始;如果 SD 卡上没有这个目录,则从根目录 `/` 开始。
- 只会读取你进入的文件夹,不会扫描整张 SD 卡,也不会读取主机内部存储。以 `.` 开头的隐藏文件和文件夹会被跳过。
- 浏览界面按键:`A` 播放,`Y` 加入播放列表,`X` 添加当前文件夹全部歌曲,`−` 设为开机自动播放,`B` 返回。在文件夹上按 `Y` 可把该文件夹里的歌加入播放列表。

**上限**:

| 项目 | 上限 |
|---|---|
| 单个文件夹列出的条目(文件夹加文件) | 2048 项,超过会提示并停止扫描 |
| 「添加全部」一次添加的歌曲数 | 300 首 |
| 「添加全部」的范围 | 只含当前文件夹,**不包含子文件夹** |

歌曲很多时,建议按文件夹分开存放,每个文件夹不超过 300 首;子文件夹里的歌需要进入对应文件夹再添加。

### 单独更新悬浮菜单

只想换新版悬浮菜单时:下载 `DferTune-Overlay.ovl`,覆盖 SD 卡上的 `/switch/.overlays/DferTune-Overlay.ovl`,再重新打开 Tesla 菜单即可。注意后台模块没有更新,如果新版悬浮菜单要求新版后台模块,请改用完整的 `DferTune.zip`。

### 常见问题

- **Tesla 菜单里看不到 DferTune**:确认文件在 `/switch/.overlays/`,并且 Tesla 环境本身能正常打开。
- **打开后提示错误或无法控制**:通常是后台模块没有运行。确认 `atmosphere/contents/420000000000000F/` 完整、含 `flags/boot2.flag`,并完整重启过 Atmosphère。
- **想保留原有设置**:更新时不要覆盖 `/config/DferTune/config.ini`。
- DferTune 使用独立的 Title ID(`0x420000000000000F`)和配置目录(`/config/DferTune/`),不会覆盖原版 sys-tune 或 RyazhaTune。同时运行多个后台音乐模块会互相争抢音频,建议只启用其中一个。

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
