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
    {"Added %lld, replaced %lld, skipped %lld."},
    {"OK"},
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
    "新增%lld首，覆盖%lld首，跳过%lld首。",
    "确定",
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
}};

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
