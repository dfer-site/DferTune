# DferTune

DferTune 是 Nintendo Switch 的后台音乐播放器(sysmodule + Tesla 悬浮菜单),支持 MP3 / FLAC / WAV、5 段均衡器、播放列表保存与按游戏过滤。可以在游戏和系统界面中播放你放在 SD 卡上的音频文件。

本项目是 [Dimasick-git/RyazhaTune](https://github.com/Dimasick-git/RyazhaTune) 的简体中文分支,而 RyazhaTune 又派生自 [HookedBehemoth/sys-tune](https://github.com/HookedBehemoth/sys-tune)。界面仅保留简体中文和英文,默认简体中文;并使用独立的名称、配置目录和 Title ID,可以与上游版本并存。

[![最新版本](https://img.shields.io/github/v/release/dfer-site/DferTune?display_name=tag&label=%E7%89%88%E6%9C%AC&style=for-the-badge)](https://github.com/dfer-site/DferTune/releases/latest)
[![下载次数](https://img.shields.io/github/downloads/dfer-site/DferTune/total?label=%E4%B8%8B%E8%BD%BD&style=for-the-badge)](https://github.com/dfer-site/DferTune/releases)
![访问量](https://hits.sh/github.com/dfer-site/DferTune.svg?label=%E8%AE%BF%E9%97%AE%E9%87%8F&color=1f6feb)

> 下载次数来自 GitHub Releases 的统计数据;访问量为页面访问次数,由第三方服务 hits.sh 提供,计数从 0 开始。

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

**播放界面按键**(底部会显示当前可用的按键):

| 按键 | 作用 |
|---|---|
| `X` | 打开设置(或回到你刚才所在的播放列表 / 浏览页) |
| `L` | 上一首(播放超过 3 秒时,先回到本曲开头) |
| `R` | 下一首 |
| `ZR` | 播放 / 暂停 |
| `A` | 触发当前选中的按钮 |
| `←` / `→` | 在随机、上一首、播放、下一首、循环之间移动;在进度条上快退 / 快进 5 秒,长按加速 |
| `B` | 关闭悬浮菜单 |

`L`、`R`、`ZR` 只在单独按下时生效,不会和组合键冲突。

**帮助与关于**:「设置」页底部有两个入口。「帮助」用文字介绍整个插件和具体用法(快速开始、按键、浏览与去重、播放列表、游戏焦点、均衡器等),可上下滚动阅读;「关于」显示版本、许可证和作者资料(作者 dfer,邮箱 df_business@qq.com,QQ 3504725309,网站 http://www.dfer.site)。

**悬停提示**:光标移到任何一行(设置项、播放列表、浏览列表、播放控制按钮)上时,会弹出一个小气泡说明这一项的作用;光标移开后自动消失。

### 浏览与添加音乐的规则

- 「浏览」从 `/music/` 开始;如果 SD 卡上没有这个目录,则从根目录 `/` 开始。
- 只会读取你进入的文件夹,不会扫描整张 SD 卡,也不会读取主机内部存储。以 `.` 开头的隐藏文件和文件夹会被跳过。
- 浏览界面按键:`A` 播放,`Y` 加入播放列表,`X` 添加当前文件夹全部歌曲,`−` 设为开机自动播放,`B` 返回。在文件夹上按 `Y` 可把该文件夹里的歌加入播放列表。
- **自动去重**:添加歌曲(包括「添加全部」)时,会按**文件名**(不区分大小写,含扩展名)与当前播放列表比较:
  - 同一个文件已经在列表里 → 跳过,不会重复添加;
  - 文件名相同但在不同文件夹 → 用新添加的**覆盖**旧的(旧条目被移除,新条目排到列表末尾);
  - 其余歌曲正常添加。「添加全部」结束后会提示"新增 / 覆盖 / 跳过 / 失败"各多少首。

**上限**:

| 项目 | 上限 |
|---|---|
| 单个文件夹列出的条目(文件夹加文件) | 2048 项,超过会提示并停止扫描 |
| 「添加全部」一次添加的歌曲数 | 300 首 |
| 「添加全部」的范围 | 只含当前文件夹,**不包含子文件夹** |

歌曲很多时,建议按文件夹分开存放,每个文件夹不超过 300 首;子文件夹里的歌需要进入对应文件夹再添加。

### 重启后会记住什么

| 内容 | 重启后 | 说明 |
|---|---|---|
| 「浏览」当前所在目录 | **不记住** | 只保存在悬浮菜单运行时的内存里,每次打开都从 `/music/`(没有则 `/`)开始 |
| 用 `−` 设为开机播放的文件/文件夹 | **记住** | 路径保存在 `config.ini`。后台模块每次启动时读取:文件则加入这一首;文件夹则把其中的歌按文件名排序后加入队列(只含该文件夹本身,不含子文件夹) |
| 播放列表 | **记住** | 保存在 `/config/DferTune/saved_playlist*.txt`。后台模块重启后队列是空的,**第一次打开悬浮菜单时**才会把保存的播放列表装回队列,所以不打开悬浮菜单就不会自动恢复 |
| 上次播放来源(播放列表或某个文件夹) | 部分记住 | 记录在 `play_source.txt`。重启后文件夹队列无法还原,会回退到播放列表模式 |

要点:

- 想让某个文件夹下次开机就播放:在「浏览」里选中该文件夹按 `−` 设为开机播放,并打开「设置」中的**开机自动播放**。如果没开这个开关,歌曲只会载入队列并保持暂停,需要手动按播放。
- 开机播放的路径如果之后被删除或改名,会被静默跳过,不会报错;请重新设置。
- 想长期固定使用的歌单,建议加入播放列表,不要依赖「浏览」的位置。

### 单独更新悬浮菜单

只想换新版悬浮菜单时:下载 `DferTune-Overlay.ovl`,覆盖 SD 卡上的 `/switch/.overlays/DferTune-Overlay.ovl`,再重新打开 Tesla 菜单即可。注意后台模块没有更新,如果新版悬浮菜单要求新版后台模块,请改用完整的 `DferTune.zip`。

### 常见问题

- **Tesla 菜单里看不到 DferTune**:确认文件在 `/switch/.overlays/`,并且 Tesla 环境本身能正常打开。
- **打开后提示错误或无法控制**:通常是后台模块没有运行。确认 `atmosphere/contents/420000000000000F/` 完整、含 `flags/boot2.flag`,并完整重启过 Atmosphère。
- **想保留原有设置**:更新时不要覆盖 `/config/DferTune/config.ini`。
- DferTune 使用独立的 Title ID(`0x420000000000000F`)和配置目录(`/config/DferTune/`),不会覆盖原版 sys-tune 或 RyazhaTune。同时运行多个后台音乐模块会互相争抢音频,建议只启用其中一个。

## 卸载

DferTune 只会在 SD 卡上占用下面三处位置,没有写入系统存储,删掉即可完全卸载。

| 位置 | 内容 | 是否必须删除 |
|---|---|---|
| `/atmosphere/contents/420000000000000F/` | 后台模块(含 `boot2.flag`) | **必须**,否则开机仍会自启 |
| `/switch/.overlays/DferTune-Overlay.ovl` | 悬浮菜单插件 | **必须**,否则 Tesla 里仍显示 DferTune |
| `/config/DferTune/` | 设置、语言文件、保存的播放列表 | 可选,想彻底清理时再删 |

你的音乐文件(如 `/music/`)不属于 DferTune,不会被动到,也不需要删除。

### 卸载步骤

1. **关机**,把 SD 卡取出插到电脑(或在 Hekate 里用 USB 大容量存储模式连接电脑)。不要在后台模块运行时从系统内删除,以免文件被占用。
2. 打开电脑上的"显示隐藏文件",因为 `switch/.overlays/` 里的 `.overlays` 是隐藏文件夹。
3. 删除整个文件夹 `atmosphere/contents/420000000000000F/`。
4. 删除文件 `switch/.overlays/DferTune-Overlay.ovl`。
5. (可选)删除整个文件夹 `config/DferTune/`。**这会同时删掉你的设置和保存的播放列表**,之后无法恢复;打算以后重装并想保留设置的话,请先备份其中的 `config.ini` 和 `saved_playlist*.txt`。
6. 把 SD 卡放回 Switch,**完整重启** Atmosphère(关机后重新进入系统)。重启后 Tesla 菜单里不应再出现 DferTune。

### 只想暂时停用

不想删除文件、只想让它不再开机自启:删除 `atmosphere/contents/420000000000000F/flags/boot2.flag` 并重启即可。想恢复时,在同一位置新建一个同名空文件(`boot2.flag`),再重启。此时悬浮菜单仍会出现在 Tesla 里,但因为后台模块没有运行,打开后无法控制播放。

也可以不动文件,用系统里的后台模块管理工具(例如读取 `toolbox.json` 的 Ovl-Sysmodules 一类 Tesla 插件)在菜单里启动、停止 DferTune,或开关它的开机自启。较新版本的这类工具还自带"卸载"功能,会直接删除 SD 卡上的后台模块(`atmosphere/contents/420000000000000F/`)和悬浮菜单(`/switch/.overlays/DferTune-Overlay.ovl`),但会**保留** `/config/DferTune/`(设置和保存的播放列表),方便以后重装时沿用。想彻底清理,请再手动删除 `config/DferTune/`;不用这类工具时,按上面的「卸载步骤」手动删除即可。

### 卸载注意事项

- 只删上表中的三处。不要删 `/atmosphere/contents/` 下其他以数字和字母命名的文件夹,它们属于其他模块。
- DferTune 使用独立的 Title ID 和配置目录,卸载不会影响并存的 sys-tune 或 RyazhaTune。
- 确认 `420000000000000F` 这串编号完全一致后再删,不要整个删除 `atmosphere/contents/`。

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

- **当前版本:** <!-- CURRENT_VERSION_START -->5.6.0-dfer.2<!-- CURRENT_VERSION_END -->
- **状态:** 稳定

## 许可证

本项目遵循 GNU General Public License Version 2(GPLv2),详见 `LICENSE` 文件。内置的 `libtesla` 组件同样使用 GPLv2。根据 GPLv2,分发编译产物时必须同时提供对应源码。
