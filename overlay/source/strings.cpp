#include "strings.hpp"

#include "config/config.hpp"

#include <tsl_utils.hpp>

#include <array>
#include <cstdio>
#include <cstring>

namespace i18n {

namespace {

char g_lang[8] = {};

struct Pair {
    const char *en;
};

// Indexed by underlying Str value — keep order identical to enum class Str.
// Count_ is a sentinel; it is NOT a row in this table.
constexpr std::array<Pair, static_cast<std::size_t>(Str::Count_)> kPairs = {{
    {"Player"},
    {"Settings"},
    {"Playlist"},
    {"Browse"},
    {"Music Library"},
    {"Volume"},
    {"Toggle Mute"},
    {"Music"},
    {"Game"},
    {"Title ID"},
    {"Preset Volume"},
    {"Default Focus"},
    {"Custom Focus"},
    {"Title Focus"},
    {"Home Focus"},
    {"Miscellaneous"},
    {"Playback Mode"},
    {"Normal"},
    {"Whitelist"},
    {"Blacklist"},
    {"Whitelist"},
    {"Blacklist"},
    {"Language"},
    {"Startup Settings"},
    {"Auto-play Startup"},
    {"Wait For Home"},
    {"Pause On Keyboard"},
    {"Pause On Controller Sync"},
    {"Pause On Lockscreen"},
    {"Remove Startup"},
    {"Stop DferTune"},
    {"On"},
    {"Off"},
    {"Pass"},
    {"Play"},
    {"Pause"},
    {"Language"},
    {"Empty..."},
    {"Tracks"},
    {"Playlist"},
    {"Playlist is empty!"},
    {"Couldn't open: "},
    {"Add To Playlist"},
    {"Add All"},
    {"Set As Startup"},
    {"Stopped Scanning Folder"},
    {"Too many entries in folder."},
    {"Failed to switch to folder."},
    {"Added 1 track to Playlist."},
    {"Failed to add track."},
    {"Startup File Set"},
    {"Startup Folder Set"},
    {"Startup Path Removed"},
    {"No startup path set in config."},
    {"Something went wrong."},
    {"Unknown Artist"},
    {"Language changed"},
    {"Open Browse and add tracks here."},
    {"Y remove · X clear · − startup"},
    {"Press + within a few seconds to undo."},
    {"Track restored to playlist."},
    {"Tip: Settings has playback modes, Home Focus, and language."},
    {" by "},
    {"Added %lld tracks to Playlist."},
    {"1 track"},
    {"%u tracks"},
    {"Shuffle"},
    {"Previous"},
    {"Next"},
    {"Repeat"},
    {"Select"},
    {"Back"},
    {"Selected"},
    {"Remove"},
    {"Remove All"},
    {"Replaced same-name track in Playlist."},
    {"Already in Playlist."},
    {"Added %lld, replaced %lld, skipped %lld, failed %lld."},
    {"OK"},
    {"Help"},
    {"About"},
    {"Version"},
    {"Author"},
    {"Email"},
    {"Website"},
    {"License"},
    {"Based on"},
}};


struct LocaleTable {
    const char *code;
    const std::array<const char *, static_cast<std::size_t>(Str::Count_)> &strings;
};

constexpr std::array<const char *, static_cast<std::size_t>(Str::Count_)> kZhCn = {{
    "播放器",
    "设置",
    "播放列表",
    "浏览",
    "音乐库",
    "音量",
    "静音",
    "音乐",
    "游戏",
    "ID",
    "预设音量",
    "默认焦点",
    "自定义焦点",
    "游戏焦点",
    "主页焦点",
    "杂项",
    "播放模式",
    "普通",
    "白名单",
    "黑名单",
    "白名单",
    "黑名单",
    "语言",
    "启动设置",
    "开机自动播放",
    "等待 HOME",
    "键盘时暂停",
    "控制器同步时暂停",
    "锁屏时暂停",
    "取消开机播放",
    "停止 DferTune",
    "开",
    "关",
    "跳过",
    "播放",
    "暂停",
    "语言",
    "空",
    "曲目",
    "播放列表",
    "列表为空",
    "打不开: ",
    "添加到列表",
    "添加全部",
    "设为开机播放",
    "扫描停止",
    "文件夹中条目过多。",
    "无法切换文件夹。",
    "已将1首歌曲添加到播放列表。",
    "添加曲目失败。",
    "已设为开机播放文件",
    "已设为开机播放文件夹",
    "开机播放路径已移除",
    "未设置开机播放路径",
    "出了点问题。",
    "未知艺术家",
    "语言已更改",
    "打开浏览并在此处添加曲目。",
    "Y 删除 · X 清空 · − 开机播放",
    "几秒内按 + 撤销。",
    "曲目已恢复到播放列表。",
    "提示：设置中有播放模式、主页焦点和语言。",
    " - ",
    "已将%lld首歌曲添加到播放列表。",
    "1 首",
    "%u首",
    "随机",
    "上一首",
    "下一首",
    "重复",
    "选择",
    "返回",
    "已选择",
    "移除",
    "全部移除",
    "已覆盖播放列表中的同名歌曲。",
    "播放列表中已有这首歌。",
    "新增%lld首，覆盖%lld首，跳过%lld首，失败%lld首。",
    "确定",
    "帮助",
    "关于",
    "版本",
    "作者",
    "邮箱",
    "网站",
    "许可证",
    "基于",
}};

constexpr std::array<LocaleTable, 1> kLocaleTables = {{
    {"zh-cn", kZhCn}
}};

constexpr std::array<const char *, static_cast<std::size_t>(Hint::Count_)> kHintEn = {{
    "A opens the current playlist to play or remove tracks.",
    "A browses folders on the SD card. Y adds one track, X adds the whole folder.",
    "Adjust the DferTune music volume. Y mutes/restores.",
    "Adjust the volume of the current game. Y mutes/restores.",
    "Games without their own volume use this preset. Y mutes/restores.",
    "A opens the 5-band equalizer.",
    "On: this game follows the global Title Focus. Off: set this game separately.",
    "A cycles what happens to the music when this game comes to the front: Pass / Play / Pause.",
    "A cycles Normal (all games) / Whitelist (listed games only) / Blacklist (listed games silent).",
    "Add the current game to the whitelist. Only used in Whitelist mode.",
    "Add the current game to the blacklist. Music stays silent for it in Blacklist mode.",
    "A opens language selection (Simplified Chinese / English).",
    "What happens to the music when you enter a game: Pass / Play / Pause (global default).",
    "What happens to the music when you press HOME: Pass / Play / Pause.",
    "A opens auto-play at boot and the pause rules for lock screen, keyboard and controller pairing.",
    "A stops the background music service. It comes back after a reboot.",
    "Start playing automatically after boot.",
    "After boot, wait for the HOME menu before playing.",
    "Pause the music while the on-screen keyboard is open.",
    "Pause the music while controller pairing is open.",
    "Pause the music on the lock screen.",
    "A clears the configured startup path.",
    "A switches to this language immediately.",
    "A starts editing. L/R picks a band, Up/Down changes gain, A or B finishes.",
    "Turn the equalizer on or off. Moving a band turns it on.",
    "A switches the target: DferTune music or game/system output.",
    "A cycles the presets: Flat, Bass, Vocal, Rock, Bright.",
    "A resets every band to 0 dB.",
    "A plays a track or opens a folder. Y adds to playlist. X adds all. Minus sets as startup.",
    "A plays. Y removes. X clears. Minus sets as startup. Plus undoes a removal.",
    "Shuffle on/off.",
    "Previous track (shortcut L). Restarts the track first if it is past 3 seconds.",
    "Play / pause (shortcut ZR).",
    "Next track (shortcut R).",
    "Repeat: off / one / all.",
    "Left/Right seeks 5 seconds. Hold to speed up.",
    "A opens an overview of the plugin and how to use it.",
    "A shows the author's details and version.",
}};

constexpr std::array<const char *, static_cast<std::size_t>(Hint::Count_)> kHintZh = {{
    "A 打开当前播放列表，可播放或删除已添加的歌曲。",
    "A 浏览 SD 卡里的文件夹；Y 添加单曲，X 添加整个文件夹。",
    "调节 DferTune 的音乐音量；Y 静音/恢复。",
    "调节当前游戏的音量；Y 静音/恢复。",
    "没有单独设置音量的游戏使用这个预设音量；Y 静音/恢复。",
    "A 打开五段均衡器。",
    "开启：本游戏沿用全局「游戏焦点」；关闭：可单独设置本游戏。",
    "A 切换本游戏进入前台时对音乐的处理：跳过 / 播放 / 暂停。",
    "A 切换：普通（所有游戏）/ 白名单（仅名单内游戏）/ 黑名单（名单内游戏不播放）。",
    "把当前游戏加入白名单，仅在白名单模式下生效。",
    "把当前游戏加入黑名单，黑名单模式下该游戏不会播放音乐。",
    "A 进入语言选择（简体中文 / English）。",
    "进入游戏界面时对音乐的处理：跳过 / 播放 / 暂停（全局默认）。",
    "按 HOME 回到主页时对音乐的处理：跳过 / 播放 / 暂停。",
    "A 进入：开机自动播放，以及锁屏、键盘、手柄配对时是否暂停。",
    "A 停止后台音乐服务，重启主机后自动恢复。",
    "开机后自动开始播放。",
    "开机后等进入主页再开始播放。",
    "屏幕键盘弹出时暂停音乐。",
    "手柄配对界面打开时暂停音乐。",
    "锁屏时暂停音乐。",
    "A 清除已设置的开机播放路径。",
    "A 立即切换到这种界面语言。",
    "A 开始编辑；L/R 选频段，↑/↓ 调增益，A 或 B 结束。",
    "开关均衡器；拖动任一频段时会自动开启。",
    "A 切换作用对象：DferTune 音乐 / 游戏及系统输出。",
    "A 循环切换预设：Flat、Bass、Vocal、Rock、Bright。",
    "A 把所有频段恢复为 0 dB。",
    "A 播放歌曲或打开文件夹；Y 添加到播放列表；X 添加全部；− 设为开机播放。",
    "A 播放；Y 移除；X 清空；− 设为开机播放；+ 撤销移除。",
    "随机播放：开 / 关。",
    "上一首（快捷键 L）；播放超过 3 秒时先回到开头。",
    "播放 / 暂停（快捷键 ZR）。",
    "下一首（快捷键 R）。",
    "循环模式：关 / 单曲 / 列表。",
    "←/→ 快退快进 5 秒，长按加速。",
    "A 查看插件介绍和详细用法。",
    "A 查看作者资料和版本信息。",
}};

constexpr InfoSection kHelpEn[] = {
    {"Overview", "DferTune is a background music player for the Switch. A background module plays the music, so it keeps going in games, and you control it from the Tesla overlay. Supports MP3, FLAC and WAV."},
    {"Quick start", "1. Put your songs in the /music/ folder on the SD card.\n2. Press X to open Settings, enter Browse and go to the folder with your songs.\n3. Press Y to add one track or X to add the whole folder, then press A to play."},
    {"Player keys", "X opens Settings; L previous track; R next track; ZR play / pause; A activates the selected button; Left/Right moves between buttons, or seeks 5 seconds on the progress bar (hold to speed up); B closes the overlay."},
    {"Browse and add", "Browse starts at /music/ and only reads the folders you enter. A plays or opens a folder, Y adds to the playlist, X adds every track in the current folder (up to 300, no subfolders), Minus sets it as the startup track, B goes back."},
    {"Automatic dedupe", "Tracks are compared by file name when you add them: the same file is never added twice, and a file with the same name from another folder overwrites the older entry."},
    {"Playlists", "There are 5 independent playlists. On the playlist page, Left/Right switches list, A plays, Y removes, X clears, Minus sets startup, Plus undoes a removal within a few seconds."},
    {"Title Focus / Home Focus", "Title Focus: what happens to the music when you enter a game - Pass / Play / Pause. Home Focus: the same when you press HOME. To set one game separately, turn off its Default Focus and use Custom Focus."},
    {"Playback mode", "Normal: music plays in every game. Whitelist: only listed games play music. Blacklist: listed games stay silent. Add the current game to a list from Settings."},
    {"Equalizer", "Five bands (100 Hz to 10 kHz, +/-12 dB) for DferTune's music or for game and system output. A starts editing, L/R picks a band, Up/Down changes gain, A or B finishes."},
    {"Startup settings", "Auto-play at boot, wait for the HOME menu before playing, and pause automatically on the lock screen, the on-screen keyboard and controller pairing."},
    {"What is remembered", "Playlists and the startup path are saved; the browse location is not. After a reboot the saved playlist is loaded back into the queue the first time you open the overlay."},
    {"Tips", "Move the cursor to any row for a bubble that explains it. B goes back one level. About, at the bottom of Settings, shows the author's details."},
};

constexpr InfoSection kHelpZh[] = {
    {"简介", "DferTune 是 Switch 的后台音乐播放器：音乐由后台模块播放，进游戏也不会中断，用 Tesla 悬浮菜单来控制。支持 MP3、FLAC、WAV。"},
    {"快速开始", "1. 把歌曲放进 SD 卡的 /music/ 文件夹。\n2. 按 X 打开设置，进入「浏览」，找到歌曲所在的文件夹。\n3. 按 Y 添加单曲，或按 X 添加整个文件夹，再按 A 开始播放。"},
    {"播放界面按键", "X 打开设置；L 上一首；R 下一首；ZR 播放 / 暂停；A 触发当前按钮；←/→ 在按钮间移动，在进度条上快退 / 快进 5 秒（长按加速）；B 关闭悬浮菜单。"},
    {"浏览与添加", "浏览从 /music/ 开始，只读取你进入的文件夹。A 播放或打开文件夹，Y 加入播放列表，X 添加当前文件夹的全部歌曲（一次最多 300 首，不含子文件夹），− 设为开机播放，B 返回。"},
    {"自动去重", "添加歌曲时按文件名比较：同一个文件不会重复添加；文件名相同但在不同文件夹时，新添加的会覆盖旧的。"},
    {"播放列表", "共有 5 个播放列表，互相独立。在播放列表页，←/→ 切换列表，A 播放，Y 移除，X 清空，− 设为开机播放，+ 可在几秒内撤销移除。"},
    {"游戏焦点 / 主页焦点", "游戏焦点：进入游戏界面时，音乐「跳过 / 播放 / 暂停」。主页焦点：按 HOME 回到主页时同样处理。想让某个游戏单独设置，先关闭它的「默认焦点」，再使用「自定义焦点」。"},
    {"播放模式", "普通：所有游戏都播放音乐。白名单：只有名单里的游戏播放。黑名单：名单里的游戏不播放。当前游戏可以在设置里加入白名单或黑名单。"},
    {"均衡器", "五段均衡器（100 Hz 到 10 kHz，±12 dB），可作用于 DferTune 的音乐，也可作用于游戏和系统输出。A 开始编辑，L/R 选频段，↑/↓ 调增益，A 或 B 结束。"},
    {"启动设置", "开机自动播放、等进入主页后再播放，以及锁屏、屏幕键盘、手柄配对时自动暂停。"},
    {"重启后会记住什么", "播放列表和开机播放路径会保存；浏览位置不保存。重启后第一次打开悬浮菜单，才会把保存的播放列表装回队列。"},
    {"小提示", "光标移到任意一行，会弹出气泡说明这一项；B 逐级返回；设置页底部有「关于」，可查看作者资料。"},
};

constexpr InfoSection kAboutEn[] = {
    {"DferTune", "A background music player for Nintendo Switch (sysmodule + Tesla overlay) with MP3 / FLAC / WAV, a five-band equalizer, playlists and per-game filtering. A Simplified Chinese fork of RyazhaTune and sys-tune."},
};

constexpr InfoSection kAboutZh[] = {
    {"DferTune", "Nintendo Switch 的后台音乐播放器（后台模块 + Tesla 悬浮菜单），支持 MP3 / FLAC / WAV、五段均衡器、播放列表和按游戏过滤。基于 RyazhaTune 与 sys-tune 的简体中文分支。"},
};

static_assert(kPairs.size() == static_cast<std::size_t>(Str::Count_),
              "kPairs size must match Str::Count_");

constexpr std::size_t idx(Str id) {
    return static_cast<std::size_t>(id);
}

const char *builtinTranslation(std::size_t i) {
    if (std::strcmp(g_lang, "en") == 0)
        return kPairs[i].en;

    for (const auto &locale : kLocaleTables) {
        if (std::strcmp(g_lang, locale.code) == 0)
            return locale.strings[i];
    }
    return nullptr;
}

} // namespace

void syncFromConfig() {
    config::get_language(g_lang, sizeof(g_lang));
    if (g_lang[0] == '\0') {
        std::strcpy(g_lang, "zh-cn");
    }
}

const char *t(Str id) {
    const auto i = idx(id);
    if (i >= kPairs.size())
        return "";

    // Every bundled language is loaded into this cache at startup and whenever
    // the language changes. It must be consulted first: otherwise a built-in
    // table silently masks new or corrected entries in overlay/lang/*.json.
    const char *en_key = kPairs[i].en;
    const auto it = ult::translationCache.find(en_key);
    if (it != ult::translationCache.end())
        return it->second.c_str();

    // The compiled tables remain a safe fallback for an incomplete custom
    // package or a missing translation file.
    if (const char *built_in = builtinTranslation(i))
        return built_in;
    return en_key;
}

const char *hint(Hint id) {
    const auto i = static_cast<std::size_t>(id);
    if (i >= kHintEn.size())
        return "";
    return std::strcmp(g_lang, "en") == 0 ? kHintEn[i] : kHintZh[i];
}

namespace {

template <std::size_t N>
constexpr InfoSections span(const InfoSection (&items)[N]) {
    return InfoSections{items, N};
}

} // namespace

InfoSections helpSections() {
    return std::strcmp(g_lang, "en") == 0 ? span(kHelpEn) : span(kHelpZh);
}

InfoSections aboutSections() {
    return std::strcmp(g_lang, "en") == 0 ? span(kAboutEn) : span(kAboutZh);
}

const char *text(const char *englishKey) {
    if (!englishKey)
        return "";
    const auto it = ult::translationCache.find(englishKey);
    return it != ult::translationCache.end() ? it->second.c_str() : englishKey;
}

const char *trackCountLabel(std::uint32_t count) {
    static char buf[48];
    if (count == 1u) {
        if (auto it = ult::translationCache.find("ONE_TRACK"); it != ult::translationCache.end())
            return it->second.c_str();
        return t(Str::TrackCountOne);
    }
    if (auto it = ult::translationCache.find("N_TRACKS"); it != ult::translationCache.end()) {
        std::snprintf(buf, sizeof(buf), "%u%s", static_cast<unsigned>(count), it->second.c_str());
    } else {
        std::snprintf(buf, sizeof(buf), t(Str::TrackCountManyFmt), static_cast<unsigned>(count));
    }
    return buf;
}

} // namespace i18n
