#include "config.hpp"
#include "sdmc/sdmc.hpp"
#include "minIni/minIni.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>

namespace config {

namespace {

const char CONFIG_DIR[]{"/config/DferTune"};
const char CONFIG_PATH[]{"/config/DferTune/config.ini"};
constexpr const char* EQ_GAIN_KEYS[TUNE_EQUALIZER_BAND_COUNT] = {
    "gain_80_hz", "gain_250_hz", "gain_1_khz", "gain_4_khz", "gain_12_khz",
};

// In-memory cache for hot scalar config values read on the IPC critical path.
// Loaded lazily on first access; updated synchronously on every set call.
struct ScalarCache {
    std::atomic<bool>  loaded{false};
    std::mutex         mu;

    bool  shuffle{false};
    int   repeat{1};
    float volume{1.f};
    bool  auto_play_startup{false};
    bool  wait_for_home{false};
    bool  pause_on_keyboard{false};
    bool  pause_on_controller_sync{false};
    bool  pause_on_lockscreen{false};
    bool  play_on_title{false};
    bool  pause_on_title{false};
    float global_volume{1.f};
    int   tune_mode{0};
    bool  equalizer_enabled{false};
    u8    equalizer_target{TUNE_EQUALIZER_TARGET_MUSIC};
    s8    equalizer_gains[TUNE_EQUALIZER_BAND_COUNT]{};

    void load() {
        std::lock_guard<std::mutex> lk(mu);
        if (loaded.load(std::memory_order_relaxed)) return;
        shuffle          = ini_getbool("config", "shuffle",          false, CONFIG_PATH);
        repeat           = (int)ini_getl("config", "repeat",         1,     CONFIG_PATH);
        volume           = ini_getf("config", "volume",              1.f,   CONFIG_PATH);
        auto_play_startup= ini_getbool("config", "auto_play_startup",false, CONFIG_PATH);
        wait_for_home    = ini_getbool("config", "wait_for_home",    false, CONFIG_PATH);
        pause_on_keyboard= ini_getbool("config", "pause_on_keyboard",false, CONFIG_PATH);
        pause_on_controller_sync
                         = ini_getbool("config", "pause_on_controller_sync", false, CONFIG_PATH);
        pause_on_lockscreen
                         = ini_getbool("config", "pause_on_lockscreen", false, CONFIG_PATH);
        play_on_title    = ini_getbool("config", "play_on_title",    false, CONFIG_PATH);
        pause_on_title   = ini_getbool("config", "pause_on_title",   false, CONFIG_PATH);
        global_volume    = ini_getf("config", "global_volume",       1.f,   CONFIG_PATH);
        tune_mode        = (int)ini_getl("config", "tune_mode",      0,     CONFIG_PATH);
        equalizer_enabled = ini_getbool("equalizer", "enabled", false, CONFIG_PATH);
        equalizer_target = static_cast<u8>(std::clamp(
            ini_getl("equalizer", "target", TUNE_EQUALIZER_TARGET_MUSIC, CONFIG_PATH),
            static_cast<long>(TUNE_EQUALIZER_TARGET_MUSIC),
            static_cast<long>(TUNE_EQUALIZER_TARGET_SYSTEM)));
        for (size_t i = 0; i < TUNE_EQUALIZER_BAND_COUNT; ++i) {
            const long gain = ini_getl("equalizer", EQ_GAIN_KEYS[i], 0, CONFIG_PATH);
            equalizer_gains[i] = static_cast<s8>(std::clamp(
                gain, static_cast<long>(TUNE_EQUALIZER_MIN_GAIN_DB),
                static_cast<long>(TUNE_EQUALIZER_MAX_GAIN_DB)));
        }
        loaded.store(true, std::memory_order_release);
    }
} g_cache;

void create_config_dir() {
    /* Creating directory on every set call looks sus, but the user may delete the dir */
    /* whilst the sys-mod is running and then any changes made via the overlay */
    /* is lost, which sucks. */
    sdmc::CreateFolder("/config");
    sdmc::CreateFolder(CONFIG_DIR);
}

auto get_tid_str(u64 tid) -> const char* {
    static thread_local char buf[21]{};
    std::sprintf(buf, "%016lX", tid);
    return buf;
}

}

auto get_shuffle() -> bool {
    g_cache.load();
    return g_cache.shuffle;
}

void set_shuffle(bool value) {
    create_config_dir();
    ini_putl("config", "shuffle", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.shuffle = value;
}

auto get_repeat() -> int {
    g_cache.load();
    return g_cache.repeat;
}

void set_repeat(int value) {
    create_config_dir();
    ini_putl("config", "repeat", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.repeat = value;
}

auto get_volume() -> float {
    g_cache.load();
    return g_cache.volume;
}

void set_volume(float value) {
    create_config_dir();
    ini_putf("config", "volume", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.volume = value;
}

auto has_title_enabled(u64 tid) -> bool {
    return ini_haskey("title", get_tid_str(tid), CONFIG_PATH);
}

auto get_title_enabled(u64 tid) -> bool {
    return ini_getbool("title", get_tid_str(tid), true, CONFIG_PATH);
}

void set_title_enabled(u64 tid, bool value) {
    create_config_dir();
    ini_putl("title", get_tid_str(tid), value, CONFIG_PATH);
}

void clear_title_enabled(u64 tid) {
    create_config_dir();
    /* minIni: passing NULL as the value removes the key. */
    ini_puts("title", get_tid_str(tid), nullptr, CONFIG_PATH);
}

auto has_title_pause_on_start(u64 tid) -> bool {
    return ini_haskey("pause_on_start", get_tid_str(tid), CONFIG_PATH);
}

auto get_title_pause_on_start(u64 tid) -> bool {
    return ini_getbool("pause_on_start", get_tid_str(tid), false, CONFIG_PATH);
}

void set_title_pause_on_start(u64 tid, bool value) {
    create_config_dir();
    ini_putl("pause_on_start", get_tid_str(tid), value, CONFIG_PATH);
}

void clear_title_pause_on_start(u64 tid) {
    create_config_dir();
    ini_puts("pause_on_start", get_tid_str(tid), nullptr, CONFIG_PATH);
}

auto get_title_enabled_default() -> bool {
    return ini_getbool("title", "default", true, CONFIG_PATH);
}

void set_title_enabled_default(bool value) {
    create_config_dir();
    ini_putl("title", "default", value, CONFIG_PATH);
}

StartupPolicy get_startup_policy() {
    g_cache.load();
    std::lock_guard<std::mutex> lk(g_cache.mu);
    return {
        .auto_play_startup = g_cache.auto_play_startup,
        .wait_for_home = g_cache.wait_for_home,
        .pause_on_keyboard = g_cache.pause_on_keyboard,
        .pause_on_controller_sync = g_cache.pause_on_controller_sync,
        .pause_on_lockscreen = g_cache.pause_on_lockscreen,
    };
}

void set_startup_policy(const StartupPolicy& policy) {
    // Publish the complete new state before filesystem writes. The sysmodule
    // policy loop therefore observes one coherent snapshot on its next tick;
    // the INI writes are persistence only, not its transport mechanism.
    g_cache.load();
    {
        std::lock_guard<std::mutex> lk(g_cache.mu);
        g_cache.auto_play_startup = policy.auto_play_startup;
        g_cache.wait_for_home = policy.wait_for_home;
        g_cache.pause_on_keyboard = policy.pause_on_keyboard;
        g_cache.pause_on_controller_sync = policy.pause_on_controller_sync;
        g_cache.pause_on_lockscreen = policy.pause_on_lockscreen;
    }

    create_config_dir();
    ini_putl("config", "auto_play_startup", policy.auto_play_startup, CONFIG_PATH);
    ini_putl("config", "wait_for_home", policy.wait_for_home, CONFIG_PATH);
    ini_putl("config", "pause_on_keyboard", policy.pause_on_keyboard, CONFIG_PATH);
    ini_putl("config", "pause_on_controller_sync", policy.pause_on_controller_sync, CONFIG_PATH);
    ini_putl("config", "pause_on_lockscreen", policy.pause_on_lockscreen, CONFIG_PATH);
}

auto get_auto_play_startup() -> bool {
    g_cache.load();
    return g_cache.auto_play_startup;
}

void set_auto_play_startup(bool value) {
    create_config_dir();
    ini_putl("config", "auto_play_startup", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.auto_play_startup = value;
}

auto get_wait_for_home() -> bool {
    g_cache.load();
    return g_cache.wait_for_home;
}

void set_wait_for_home(bool value) {
    create_config_dir();
    ini_putl("config", "wait_for_home", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.wait_for_home = value;
}

auto get_pause_on_keyboard() -> bool {
    g_cache.load();
    return g_cache.pause_on_keyboard;
}

void set_pause_on_keyboard(bool value) {
    create_config_dir();
    ini_putl("config", "pause_on_keyboard", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.pause_on_keyboard = value;
}

auto get_pause_on_controller_sync() -> bool {
    g_cache.load();
    return g_cache.pause_on_controller_sync;
}

void set_pause_on_controller_sync(bool value) {
    create_config_dir();
    ini_putl("config", "pause_on_controller_sync", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.pause_on_controller_sync = value;
}

auto get_pause_on_lockscreen() -> bool {
    g_cache.load();
    return g_cache.pause_on_lockscreen;
}

void set_pause_on_lockscreen(bool value) {
    create_config_dir();
    ini_putl("config", "pause_on_lockscreen", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.pause_on_lockscreen = value;
}

/* Title Focus is written by the overlay process but consumed by the sysmodule
 * process, which cannot see the overlay's in-memory cache. Read the INI every
 * time instead of using g_cache, otherwise changing it in the overlay has no
 * effect until the sysmodule restarts. These are only queried on title/focus
 * transitions, so the file read is cheap. */
auto get_play_on_title() -> bool {
    return ini_getbool("config", "play_on_title", false, CONFIG_PATH);
}

void set_play_on_title(bool value) {
    create_config_dir();
    ini_putl("config", "play_on_title", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.play_on_title = value;
}

auto get_pause_on_title() -> bool {
    return ini_getbool("config", "pause_on_title", false, CONFIG_PATH);
}

void set_pause_on_title(bool value) {
    create_config_dir();
    ini_putl("config", "pause_on_title", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.pause_on_title = value;
}

auto get_default_on_start(u64 tid) -> bool {
    return ini_getbool("default_on_start", get_tid_str(tid), true, CONFIG_PATH);
}

void set_default_on_start(u64 tid, bool value) {
    create_config_dir();
    ini_putl("default_on_start", get_tid_str(tid), value, CONFIG_PATH);
}

auto has_title_volume(u64 tid) -> bool {
    return ini_haskey("volume", get_tid_str(tid), CONFIG_PATH);
}

auto get_title_volume(u64 tid) -> float {
    return ini_getf("volume", get_tid_str(tid), 1.f, CONFIG_PATH);
}

void set_title_volume(u64 tid, float value) {
    create_config_dir();
    ini_putf("volume", get_tid_str(tid), value, CONFIG_PATH);
}

auto get_default_title_volume() -> float {
    g_cache.load();
    return g_cache.global_volume;
}

void set_default_title_volume(float value) {
    create_config_dir();
    ini_putf("config", "global_volume", value, CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.global_volume = value;
}

auto get_load_path(char* out, int max_len) -> int {
    return ini_gets("config", "load_path", "", out, max_len, CONFIG_PATH);
}

void set_load_path(const char* path) {
    create_config_dir();
    ini_puts("config", "load_path", path, CONFIG_PATH);
}

auto get_tune_mode() -> TuneMode {
    g_cache.load();
    const int mode = g_cache.tune_mode;
    if (mode == static_cast<int>(TuneMode::Whitelist))
        return TuneMode::Whitelist;
    if (mode == static_cast<int>(TuneMode::Blacklist))
        return TuneMode::Blacklist;
    return TuneMode::Normal;
}

void set_tune_mode(TuneMode mode) {
    create_config_dir();
    ini_putl("config", "tune_mode", static_cast<long>(mode), CONFIG_PATH);
    std::lock_guard<std::mutex> lk(g_cache.mu);
    g_cache.tune_mode = static_cast<int>(mode);
}

auto is_tid_whitelisted(u64 tid) -> bool {
    return ini_getbool("whitelist", get_tid_str(tid), false, CONFIG_PATH);
}

void set_tid_whitelisted(u64 tid, bool value) {
    create_config_dir();
    if (value)
        ini_putl("whitelist", get_tid_str(tid), true, CONFIG_PATH);
    else
        ini_puts("whitelist", get_tid_str(tid), nullptr, CONFIG_PATH);
}

auto is_tid_blacklisted(u64 tid) -> bool {
    return ini_getbool("blacklist", get_tid_str(tid), false, CONFIG_PATH);
}

void set_tid_blacklisted(u64 tid, bool value) {
    create_config_dir();
    if (value)
        ini_putl("blacklist", get_tid_str(tid), true, CONFIG_PATH);
    else
        ini_puts("blacklist", get_tid_str(tid), nullptr, CONFIG_PATH);
}

auto is_title_allowed(u64 tid) -> bool {
    /* Filter modes are for applications/games. Keep HOME/system fallback TIDs
     * outside the lists so HOME focus policy and startup behavior do not get
     * accidentally blocked by an empty whitelist. */
    constexpr u64 kHomeScreenTid = 0x0100000000001000ULL;
    if (tid == 0 || tid == kHomeScreenTid)
        return true;

    switch (get_tune_mode()) {
        case TuneMode::Whitelist:
            return is_tid_whitelisted(tid);
        case TuneMode::Blacklist:
            return !is_tid_blacklisted(tid);
        case TuneMode::Normal:
        default:
            return true;
    }
}

void ensure_language_config() {
    create_config_dir();
    char language[8]{};
    ini_gets("config", "language", "", language, sizeof(language), CONFIG_PATH);

    if (language[0] == '\0')
        std::snprintf(language, sizeof(language), "%s", "zh-cn");

    ini_puts("config", "language", language, CONFIG_PATH);
}

auto get_language(char* out, int max_len) -> int {
    const int len = ini_gets("config", "language", "", out, max_len, CONFIG_PATH);
    if (len > 0)
        return len;

    ensure_language_config();
    return ini_gets("config", "language", "zh-cn", out, max_len, CONFIG_PATH);
}

void set_language(const char* language) {
    create_config_dir();
    ini_puts("config", "language", language, CONFIG_PATH);
}

TuneEqualizerSettings get_equalizer_settings() {
    g_cache.load();
    std::lock_guard<std::mutex> lk(g_cache.mu);

    TuneEqualizerSettings settings{};
    settings.enabled = g_cache.equalizer_enabled ? 1 : 0;
    settings.target = g_cache.equalizer_target;
    for (size_t i = 0; i < TUNE_EQUALIZER_BAND_COUNT; ++i)
        settings.gains_db[i] = g_cache.equalizer_gains[i];
    return settings;
}

void set_equalizer_settings(const TuneEqualizerSettings& raw) {
    g_cache.load();
    create_config_dir();

    TuneEqualizerSettings settings{};
    settings.enabled = raw.enabled ? 1 : 0;
    settings.target = raw.target == TUNE_EQUALIZER_TARGET_SYSTEM
        ? TUNE_EQUALIZER_TARGET_SYSTEM : TUNE_EQUALIZER_TARGET_MUSIC;
    for (size_t i = 0; i < TUNE_EQUALIZER_BAND_COUNT; ++i) {
        settings.gains_db[i] = static_cast<s8>(std::clamp(
            static_cast<int>(raw.gains_db[i]), TUNE_EQUALIZER_MIN_GAIN_DB,
            TUNE_EQUALIZER_MAX_GAIN_DB));
    }

    std::lock_guard<std::mutex> lk(g_cache.mu);
    if (g_cache.equalizer_enabled != (settings.enabled != 0)) {
        ini_putl("equalizer", "enabled", settings.enabled, CONFIG_PATH);
        g_cache.equalizer_enabled = settings.enabled != 0;
    }
    if (g_cache.equalizer_target != settings.target) {
        ini_putl("equalizer", "target", settings.target, CONFIG_PATH);
        g_cache.equalizer_target = settings.target;
    }
    for (size_t i = 0; i < TUNE_EQUALIZER_BAND_COUNT; ++i) {
        if (g_cache.equalizer_gains[i] == settings.gains_db[i])
            continue;
        ini_putl("equalizer", EQ_GAIN_KEYS[i], settings.gains_db[i], CONFIG_PATH);
        g_cache.equalizer_gains[i] = settings.gains_db[i];
    }
}

}
