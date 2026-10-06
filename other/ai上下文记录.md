# AI 上下文记录

更新时间：2026-10-06 01:34 UTC

## 项目与仓库

- 本仓库 `dfer-site/DferTune`：Nintendo Switch 后台音乐播放器（sysmodule + Tesla 悬浮菜单），用户代号 dfer，网站 http://www.dfer.site 。
- 来源：fork 自 `Dimasick-git/RyazhaTune`（其派生自 `HookedBehemoth/sys-tune`），许可证 GPL v2，需保留 LICENSE 与致谢。
- 用户实测：Switch 固件 21.1.0 上原版 sys-tune 无法使用，RyazhaTune 可正常运行。原版代码里 `audWrapperExit` 误调用 `audaInitialize()`，且长期占用 `aud:a` 会话。
- 其他相关仓库：`dfer-site/shi_kong_hong_li`（只有一个基础 README，分支 `claude/sweet-keller-s1xcir`）、`dfer-site/test-repo`（私有，只有 README）。`df-business/ryazhatune` 已被用户删除。

## DferTune 已完成的工作（均已推送 main）

- 全部改名为 DferTune：目录 `DferTune/`、目标名、配置目录 `/config/DferTune/`、悬浮菜单 `DferTune-Overlay.ovl`。
- Title ID 改为 `0x420000000000000F`（上游 Dimasick 版是 `...0E`，原版是 `...00`），可并存。
- 界面语言只保留简体中文（`zh-cn`，默认）和英文（`en`）；`strings.cpp` 删除俄语列和其他内置语言表；`default-config.ini` 默认 `language=zh-cn`。
- README、Issue 模板、工作流文字、Makefile 注释、`setup_build_env.sh` 都已改为中文；README 保留版本标记 `CURRENT_VERSION_START/END` 与徽章（CI 脚本依赖）。
- 版本号 `5.6.0-dfer.1`（`Makefile`、`README.md`、`DferTune/toolbox.json`、`overlay/Makefile` 四处必须一致）。
- 中文译文润色：开机自动播放、取消开机播放等。`scripts/verify-localizations.mjs` 固定了 3 条译文（等待 HOME、键盘时暂停、控制器同步时暂停），不要改动。
- 刻意保留 `libryazhahand` 与 `/config/ryazhahand/`：这是外部依赖库，Makefile 会从 `Dimasick-git/libryazhahand` 按固定提交克隆。
- 已知上游问题：`tests/equalizer_test.cpp` 报 `bass response too weak`（上游原仓库同样失败，与本项目改动无关）；仓库里还有一份旧的重复目录 `DferTune/impl/`，未处理。

## CI 与发布

- 工作流 `.github/workflows/build-and-release.yml`：push 到 main 且改动 `Makefile`、`DferTune/`、`common/`、`ipc/`、`overlay/` 等路径时，若 `v<版本>` 标签不存在就自动构建并发布。已加 `workflow_dispatch`（输入 `publish`，默认 false 只构建上传产物），并改用 `bash -e {0}`（容器默认 sh 不支持 `[[ ]]`）。
- 提交信息带 `[skip ci]` 可避免触发自动发版。
- 发布说明文件：`.github/releases/v<版本>.md`，CI 发布时用作正文。
- 已发布 release：`v5.6.0-dfer.1`（01:32 UTC，含 `DferTune.zip`、`DferTune-Overlay.ovl`，正文为中文）。
- 仍未验证：真机运行（尤其固件 21.1.0）。构建警告：`upload-artifact@v4` 的 Node.js 20 弃用提示，暂不影响。

## 能力限制与约定

- GitHub 授权只能读写已加入会话的仓库；无法创建/删除仓库，无法改仓库 About（简介、网址、话题），无法直接新建 release（只能通过工作流发布）。
- About 建议文案：Switch 后台音乐播放器（sysmodule + Tesla 悬浮菜单），支持 MP3/FLAC/WAV、5 段均衡器、播放列表保存与按游戏过滤；基于 RyazhaTune / sys-tune 的简体中文分支。Website `http://www.dfer.site`；Topics：nintendo-switch、switch-homebrew、atmosphere、sysmodule、tesla-overlay、music-player、chinese。需用户手动填写。
- 用户偏好：只用中文交流；直接推 `main`；提交信息末尾附 `Co-Authored-By` 与 `Claude-Session` 署名行。
- 远端可能有用户在网页上做的提交，推送前先 `git pull --rebase origin main`。

## 后续可做

- 用户在真机测试并反馈；视结果再发新版（先改版本号四处，并新增 `.github/releases/v<新版本>.md`）。
- 可选：清理重复目录 `DferTune/impl/`；把 `libryazhahand` 也 fork 以摆脱对上游仓库的依赖。
