# AI 上下文记录

更新时间：2026-10-06 01:55 UTC

## 项目与仓库

- 本仓库 `dfer-site/DferTune`（公开）：Nintendo Switch 后台音乐播放器（sysmodule + Tesla 悬浮菜单）。用户代号 dfer，网站 http://www.dfer.site 。
- 来源：fork 自 `Dimasick-git/RyazhaTune`（其派生自 `HookedBehemoth/sys-tune`），GPL v2，需保留 LICENSE 与致谢。
- 用户实测：固件 21.1.0 上原版 sys-tune 无法使用，RyazhaTune 可用。原版 `audWrapperExit` 误调 `audaInitialize()`，且长期占用 `aud:a` 会话。
- 其他仓库：`dfer-site/shi_kong_hong_li`（仅基础 README，分支 `claude/sweet-keller-s1xcir`）、`dfer-site/test-repo`（私有，仅 README）。`df-business/ryazhatune` 用户已删。

## DferTune 已完成（均已推送 main，最新提交约 `54902e7`）

- 全部改名 DferTune：目录 `DferTune/`、配置目录 `/config/DferTune/`、悬浮菜单 `DferTune-Overlay.ovl`。
- Title ID 改为 `0x420000000000000F`（Dimasick 版是 `...0E`，原版 `...00`），可并存。
- 界面语言只留 `zh-cn`（默认）和 `en`；`strings.cpp` 已删俄语列和其他内置表；`default-config.ini` 为 `language=zh-cn`。
- README、Issue 模板、工作流文字、Makefile 注释、`setup_build_env.sh` 全部中文。
- 版本 `5.6.0-dfer.1`：`Makefile`、`README.md` 的 `CURRENT_VERSION_START/END` 标记、`DferTune/toolbox.json`、`overlay/Makefile` 四处必须一致（`scripts/verify-readme-automation.mjs` 校验）。
- 刻意保留 `libryazhahand` 与 `/config/ryazhahand/`（外部依赖，Makefile 按固定提交从 `Dimasick-git/libryazhahand` 克隆）。
- `scripts/verify-localizations.mjs` 固定了 3 条译文（等待 HOME、键盘时暂停、控制器同步时暂停），不要改。
- README 徽章：版本/下载用 shields.io（有缓存，下载徽章曾长时间显示 `NO RELEASES FOUND`，可能需先有人下载过附件）；访问量用 `hits.sh`（支持中文标签，用户已确认显示正常，校验脚本已同步该地址）。原 visitor-badge.laobi.icu 中文被截断，已弃用。
- 已删除上游英文均衡器截图（不是 DferTune 真实界面）；用户计划以后提供真机截图再补。
- README/release 已写入：Tesla 前置条件、`DferTune.zip` 与 `.ovl` 用途、建议音乐放 `/music/`、不自动扫描、浏览规则与上限（单文件夹最多 2048 项；「添加全部」每次 300 首且不含子文件夹）、重启后记忆规则（见下）。
- 已知上游问题：`tests/equalizer_test.cpp` 报 `bass response too weak`（上游同样失败）；旧重复目录 `DferTune/impl/` 未清理。

## 行为（读代码得出，未经真机验证）

- 「浏览」从 `/music/` 开始（无则 `/`），只读进入的文件夹，仅认 `.mp3 .flac .wav .wave`，跳过隐藏项。按键：A 播放、Y 加入、X 添加全部、− 设为开机播放、B 返回。
- 重启后：浏览位置不记住；`−` 设置的开机路径记住（文件→一首，文件夹→排序后全部，需开启「开机自动播放」否则只载入并暂停，路径失效静默跳过）；播放列表记住但需第一次打开悬浮菜单才装回队列；播放来源文件夹无法还原，回退播放列表。

## CI 与发布

- `.github/workflows/build-and-release.yml`：push 到 main 且改动 `Makefile`、`DferTune/`、`common/`、`ipc/`、`overlay/` 等路径、且 `v<版本>` 标签不存在时，自动构建并发布。已加 `workflow_dispatch`（输入 `publish`，默认 false 仅构建上传产物），并设 `defaults.run.shell: bash -e {0}`（devkitpro 容器默认 sh 不支持 `[[ ]]`，否则读不到 release 说明文件）。
- 提交信息带 `[skip ci]` 可避免自动发版。
- release 正文来自 `.github/releases/v<版本>.md`。标签已存在时，手动触发 `publish=false` 的工作流（约 20–30 秒）会把该文件同步到现有 release。
- 已发布 release `v5.6.0-dfer.1`（01:32 UTC，含 `DferTune.zip` 约 614KB、`DferTune-Overlay.ovl` 约 784KB，正文中文）。
- 仍未验证：真机运行（尤其 21.1.0）、Tesla 呼出组合键（文档写“常见为 L+↓+右摇杆按下”）、菜单名称「浏览/播放列表/设置」。

## 能力限制与约定

- 能读写已加入会话的仓库、推送、触发/查看 Actions、查看 release；**不能**建/删仓库、改仓库 About（简介/网址/话题）、直接新建 release（只能经工作流）；沙箱访问不了 shields.io、hits.sh 等外部服务，无法预览徽章。
- About 需用户手动填：Description「Switch 后台音乐播放器（sysmodule + Tesla 悬浮菜单），支持 MP3/FLAC/WAV、5 段均衡器、播放列表保存与按游戏过滤。基于 RyazhaTune / sys-tune 的简体中文分支。」Website `http://www.dfer.site`；Topics：nintendo-switch、switch-homebrew、atmosphere、sysmodule、tesla-overlay、music-player、chinese。
- 用户偏好：只用中文交流；直接推 `main`；提交信息末尾带 `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>` 与 `Claude-Session` 行；用户会在网页上直接改仓库（如 `other/ai.md`），推送前先 `git pull --rebase origin main`。
- `other/ai.md` 是基础声明，让新会话读取 `/other/log.md` 并只用中文；`other/log.md` 即本文件。

## 后续可做

- 用户真机测试并反馈；视结果发新版（改四处版本号 + 新增 `.github/releases/v<新版本>.md`，提交勿带 `[skip ci]` 才会自动发版，或手动触发 `publish=true`）。
- 补真机截图；确认下载徽章恢复；可选清理 `DferTune/impl/`；可选 fork `libryazhahand` 摆脱上游依赖。
