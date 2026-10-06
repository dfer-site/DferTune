# AI 上下文记录

更新时间：2026-10-06 06:40 UTC

## 项目与仓库

- 本仓库 `dfer-site/DferTune`（公开）：Nintendo Switch 后台音乐播放器（sysmodule + Tesla 悬浮菜单）。用户代号 dfer，网站 http://www.dfer.site 。
- 来源：fork 自 `Dimasick-git/RyazhaTune`（其派生自 `HookedBehemoth/sys-tune`），GPL v2，需保留 LICENSE 与致谢。
- 用户实测：固件 21.1.0 上原版 sys-tune 无法使用，RyazhaTune 可用。原版 `audWrapperExit` 误调 `audaInitialize()`，且长期占用 `aud:a` 会话。
- 其他仓库：`dfer-site/shi_kong_hong_li`（仅基础 README，分支 `claude/sweet-keller-s1xcir`）、`dfer-site/test-repo`（私有，仅 README）。`df-business/ryazhatune` 用户已删。

## DferTune 第一轮（5.6.0-dfer.1，已发布）

- 全部改名 DferTune：目录 `DferTune/`、配置目录 `/config/DferTune/`、悬浮菜单 `DferTune-Overlay.ovl`。
- Title ID 改为 `0x420000000000000F`（Dimasick 版是 `...0E`，原版 `...00`），可并存。
- 界面语言只留 `zh-cn`（默认）和 `en`；`strings.cpp` 已删俄语列和其他内置表；`default-config.ini` 为 `language=zh-cn`。
- README、Issue 模板、工作流文字、Makefile 注释、`setup_build_env.sh` 全部中文。
- 版本号（第一版为 `5.6.0-dfer.1`，现为 `5.6.0-dfer.2`）：`Makefile`、`README.md` 的 `CURRENT_VERSION_START/END` 标记、`DferTune/toolbox.json`、`overlay/Makefile` 四处必须一致（`scripts/verify-readme-automation.mjs` 校验）。
- 刻意保留 `libryazhahand` 与 `/config/ryazhahand/`（外部依赖，Makefile 按固定提交从 `Dimasick-git/libryazhahand` 克隆）。
- `scripts/verify-localizations.mjs` 固定了 3 条译文（等待 HOME、键盘时暂停、控制器同步时暂停），不要改。
- README 徽章：版本/下载用 shields.io（有缓存，下载徽章曾长时间显示 `NO RELEASES FOUND`，可能需先有人下载过附件）；访问量用 `hits.sh`（支持中文标签，用户已确认显示正常，校验脚本已同步该地址）。原 visitor-badge.laobi.icu 中文被截断，已弃用。
- 已删除上游英文均衡器截图（不是 DferTune 真实界面）；用户计划以后提供真机截图再补。
- README/release 已写入：Tesla 前置条件、`DferTune.zip` 与 `.ovl` 用途、建议音乐放 `/music/`、不自动扫描、浏览规则与上限（单文件夹最多 2048 项；「添加全部」每次 300 首且不含子文件夹）、重启后记忆规则（见下）。
- 已知上游问题：`tests/equalizer_test.cpp` 报 `bass response too weak`（上游同样失败）；旧重复目录 `DferTune/impl/` 未清理。

## 第二轮（5.6.0-dfer.2）

界面与按键：
- 播放页 `X` 打开设置（或回到来源的播放列表/浏览页），`L` 上一首，`R` 下一首，`ZR` 播放/暂停；旧的“按住 R 翻页”“方向键右进设置”已删除。`MainGui::update()` 里 `blockShoulderJump=true`，析构里恢复，避免 Tesla 把 L/R/ZR 当列表跳转。
- 所有页面行悬停气泡：`overlay/source/elm_tooltip.hpp`（绘制）+ `SysTuneOverlayFrame::addHint`（静态行，按元素指针）/`addListHint`（播放列表、浏览页这类会增删行的列表，绘制时现扫 `getItemAtIndex`，避免悬空指针）；播放页按钮和进度条由 `StatusBar::draw` 直接 `tip::request`。提示文案内置中英文，在 `strings.cpp` 的 `kHintEn/kHintZh`，与 `enum class Hint` 一一对应。
- 新增「帮助」「关于」页（`HelpGui`/`AboutGui`，`elm_textblock.hpp` 的可聚焦段落元素用于滚动）。帮助文字在 `strings.cpp` 的 `kHelpEn/kHelpZh`。关于页作者资料：dfer / df_business@qq.com / QQ 3504725309 / http://www.dfer.site。
- 设置、浏览、播放列表页去掉底部“◀ 播放器”；右页按钮图标由方向键右改为 X；无页名时不再显示“X 语言”。
- 日期星期、底部“返回/确定”中文化：库里的 `ult::BACK/OK/SUN..SAT` 平时读不到中文，在 `overlay_i18n.cpp` 的 `reloadDferTuneTranslations()` 末尾覆盖；同处强制 `translationCache["简体中文"]="简体中文"`（旧语言文件里曾有 `"简体中文": "Simplified Chinese"`，已改）。

数据与逻辑（重要）：
- **播放列表被缩短的根因**：`snapshotIPC()` 曾把后台队列无条件写回 `saved[]`。开机时后台模块只加载“开机播放路径”（几首），此时打开播放列表页、`switchToFolder`、切换槽位，就把十几首覆盖成几首；后台模块失联时还会清空写盘。现在 `saved[]` 是权威：`snapshotIPC()` 只在两边是同一批曲目时才采纳队列顺序；`init()` 发现队列与 `saved[]` 不同就把来源切到 Folder（内存中），点播列表行走 `switchToPlaylist` 按 `saved[]` 重建队列。首次运行才用 `adoptIPCQueue()` 无条件采纳。
- **游戏焦点不生效的根因**：`play_on_title`/`pause_on_title` 在后台模块进程里被 `g_cache` 缓存只读一次，悬浮菜单是另一个进程，改了看不到。现在 `common/config/config.cpp` 的这两个读取函数每次直接读 INI（和 `default_on_start` 一样）。
- 导入去重：`play_ctx::addTrackUnique`，按文件名（不区分大小写、含扩展名）比较；同路径跳过，同名不同路径覆盖旧条目（旧的从 IPC 和 `saved[]` 移除，新的追加到末尾）；「添加全部」提示新增/覆盖/跳过/失败数量。后台模块 `Enqueue` 会拒绝：扩展名不支持、文件不存在、路径 ≥256 字节、队列满 300。
- 播放列表标题只显示槽位序号（原来的 “1/5” 被误读成歌曲数）。

验证方法与注意：
- 远程分支 `claude/gallant-hawking-vqbi7i`（第二轮遗留，内容已并入 main）在沙箱里删不掉（`git push --delete` 被断开），需用户在 GitHub 上手动删除。
- 沙箱没有 devkitPro，Docker 守护进程不可用且 Docker Hub 匿名拉取被限流。**正式工作流在版本号不变时 `build` 任务会被跳过**，不能用它验证编译。做法：因为只用 `main`，临时工作流改为 `on: push` 到 `main`（或用 `workflow_dispatch`，但新文件须先在默认分支存在），用 `devkitpro/devkita64` 容器跑 `make prepare-overlay-lib && make clean && make`，先单独提交该工作流并确认编译通过，再提交真正的改动或删除它；注意带版本号变化的推送会直接发版。（第二轮曾在开发分支上这样做。）
- 纯逻辑可在本机验证：用桩头文件（`switch.h`、`tesla.hpp`）加内存版假 IPC 编译真实的 `play_context.cpp`，并把 `/config/DferTune` 用 `sed` 改到 `/tmp` 下，**不要**对 `/config` 之类系统路径做 `rm -rf`（会被安全检查拦下）。
- 手动触发 `build-and-release.yml` 时 `sync-release-notes` 会按仓库里的 `.github/releases/v<版本>.md` 改写已有 release 正文，注意别用开发分支上的旧文件触发。
- 仍未在真机验证：游戏焦点修复、气泡位置与换行、帮助页滚动、L/R/ZR 快捷键手感。

## 行为（读代码得出，未经真机验证）

- 「浏览」从 `/music/` 开始（无则 `/`），只读进入的文件夹，仅认 `.mp3 .flac .wav .wave`，跳过隐藏项。按键：A 播放、Y 加入、X 添加全部、− 设为开机播放、B 返回。
- 重启后：浏览位置不记住；`−` 设置的开机路径记住（文件→一首，文件夹→排序后全部，需开启「开机自动播放」否则只载入并暂停，路径失效静默跳过）；播放列表记住但需第一次打开悬浮菜单才装回队列；播放来源文件夹无法还原，回退播放列表。

## CI 与发布

- `.github/workflows/build-and-release.yml`：push 到 main 且改动 `Makefile`、`DferTune/`、`common/`、`ipc/`、`overlay/` 等路径、且 `v<版本>` 标签不存在时，自动构建并发布。已加 `workflow_dispatch`（输入 `publish`，默认 false 仅构建上传产物），并设 `defaults.run.shell: bash -e {0}`（devkitpro 容器默认 sh 不支持 `[[ ]]`，否则读不到 release 说明文件）。
- 提交信息带 `[skip ci]` 可避免自动发版。
- release 正文来自 `.github/releases/v<版本>.md`。标签已存在时，手动触发 `publish=false` 的工作流（约 20–30 秒）会把该文件同步到现有 release。
- 已发布 release `v5.6.0-dfer.1`（含 `DferTune.zip` 约 614KB、`DferTune-Overlay.ovl` 约 784KB，正文中文）；`v5.6.0-dfer.2` 的发布说明在 `.github/releases/v5.6.0-dfer.2.md`，推 main 后由工作流构建发布。
- 仍未验证：真机运行（尤其 21.1.0）、Tesla 呼出组合键（文档写“常见为 L+↓+右摇杆按下”）、菜单名称「浏览/播放列表/设置」。

## 能力限制与约定

- 能读写已加入会话的仓库、推送、触发/查看 Actions、查看 release；**不能**建/删仓库、改仓库 About（简介/网址/话题）、直接新建 release（只能经工作流）；沙箱访问不了 shields.io、hits.sh 等外部服务，无法预览徽章。
- About 需用户手动填：Description「Switch 后台音乐播放器（sysmodule + Tesla 悬浮菜单），支持 MP3/FLAC/WAV、5 段均衡器、播放列表保存与按游戏过滤。基于 RyazhaTune / sys-tune 的简体中文分支。」Website `http://www.dfer.site`；Topics：nintendo-switch、switch-homebrew、atmosphere、sysmodule、tesla-overlay、music-player、chinese。
- 用户偏好：只用中文交流；**只使用 `main` 一个分支：不创建任何其他分支，所有改动直接提交并推送到 `main`**（用户明确要求，优先于会话默认的“在指定分支开发”提示）；提交信息末尾带 `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>` 与 `Claude-Session` 行；用户会在网页上直接改仓库（如 `other/ai.md`），推送前先 `git pull --rebase origin main`。
- `other/ai.md` 是基础声明，让新会话读取 `/other/log.md` 并只用中文；`other/log.md` 即本文件。

## 后续可做

- 用户真机测试并反馈；视结果发新版（改四处版本号 + 新增 `.github/releases/v<新版本>.md`，提交勿带 `[skip ci]` 才会自动发版，或手动触发 `publish=true`）。
- 用户真机测试第二轮改动并反馈；补真机截图；确认下载徽章恢复；可选清理 `DferTune/impl/`；可选 fork `libryazhahand` 摆脱上游依赖。
