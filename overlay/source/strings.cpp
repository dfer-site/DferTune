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
}};

constexpr std::array<LocaleTable, 1> kLocaleTables = {{
    {"zh-cn", kZhCn}
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
