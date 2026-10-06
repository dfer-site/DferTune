#include "gui_main.hpp"

#include "elm_overlayframe.hpp"
#include "elm_equalizer.hpp"
#include "elm_volume.hpp"
#include "elm_textblock.hpp"
#include "elm_wrappedheader.hpp"
#include "gui_browser.hpp"
#include "gui_playlist.hpp"
#include "play_context.hpp"
#include "pm/pm.hpp"
#include "config/config.hpp"
#include "strings.hpp"
#include "overlay_i18n.hpp"

#include <tsl_utils.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <string>

// =============================================================================
// PlayerRightDest + browser return path globals
// =============================================================================

static PlayerRightDest g_player_right_dest   = PlayerRightDest::Settings;
static std::string     g_browser_return_cwd;
static std::string     g_browser_return_root;

// ---------------------------------------------------------------------------
// Volume mute-backup persistence
// Three integers (0-100), one per line: Music, Game, Game (default).
// ---------------------------------------------------------------------------
static constexpr const char* kVolBackupFile = "/config/DferTune/volume_backup.txt";

static void readVolBackups(u8 &music, u8 &game, u8 &game_def) {
    music = game = game_def = 100;
    FILE* f = fopen(kVolBackupFile, "r");
    if (!f) return;
    int m = 100, g = 100, gd = 100;
    fscanf(f, "%d\n%d\n%d", &m, &g, &gd);
    fclose(f);
    music    = static_cast<u8>(std::clamp(m,  0, 100));
    game     = static_cast<u8>(std::clamp(g,  0, 100));
    game_def = static_cast<u8>(std::clamp(gd, 0, 100));
}

static void writeVolBackups(u8 music, u8 game, u8 game_def) {
    FILE* f = fopen(kVolBackupFile, "w");
    if (!f) return;
    fprintf(f, "%d\n%d\n%d\n", static_cast<int>(music),
                                static_cast<int>(game),
                                static_cast<int>(game_def));
    fclose(f);
}

void setPlayerRightDest(PlayerRightDest dest) { g_player_right_dest = dest; }

void setBrowserReturnPath(const std::string& cwd, const std::string& root) {
    g_browser_return_cwd   = cwd;
    g_browser_return_root  = root;
}

// ---------------------------------------------------------------------------
// Tri-state focus action used by the "Title Focus" and "Custom Focus"
// cycling list items in SettingsGui.
//
//   Pass  — policy engine takes no action at title focus transitions
//           (music simply keeps doing whatever it was doing).
//   Play  — force music to play.
//   Pause — force music to pause.
//
// A KEY_A press cycles: Pass -> Play -> Pause -> Pass -> ...
//
// Storage is unchanged from the previous pair-of-toggles UI — each list
// item maps to two mutually-exclusive bool keys in config.ini — so
// existing user configs are fully backward compatible.
// ---------------------------------------------------------------------------
namespace {
    enum class FocusMode { Pass, Play, Pause };

    const char* focusLabel(FocusMode m) {
        switch (m) {
            case FocusMode::Play:  return i18n::t(i18n::Str::Play);
            case FocusMode::Pause: return i18n::t(i18n::Str::Pause);
            case FocusMode::Pass:
            default:               return i18n::t(i18n::Str::Pass);
        }
    }

    constexpr FocusMode focusNext(FocusMode m) {
        switch (m) {
            case FocusMode::Pass:  return FocusMode::Play;
            case FocusMode::Play:  return FocusMode::Pause;
            case FocusMode::Pause:
            default:               return FocusMode::Pass;
        }
    }

    struct LanguageOption {
        const char *code;
        const char *label;
    };

    constexpr LanguageOption kLanguages[] = {
        {"zh-cn", "简体中文"},
        {"en", "English"},
    };

    size_t currentLanguageIndex() {
        char language[8]{};
        config::get_language(language, sizeof(language));
        for (size_t i = 0; i < std::size(kLanguages); ++i) {
            if (std::strcmp(language, kLanguages[i].code) == 0)
                return i;
        }

        config::set_language(kLanguages[0].code);
        return 0;
    }

    TuneStartupPolicy toIpcStartupPolicy(const config::StartupPolicy& policy) {
        return {
            .auto_play_startup = static_cast<u8>(policy.auto_play_startup),
            .wait_for_home = static_cast<u8>(policy.wait_for_home),
            .pause_on_keyboard = static_cast<u8>(policy.pause_on_keyboard),
            .pause_on_controller_sync = static_cast<u8>(policy.pause_on_controller_sync),
            .pause_on_lockscreen = static_cast<u8>(policy.pause_on_lockscreen),
            .reserved = {0, 0, 0},
        };
    }

    void applyStartupPolicy(const config::StartupPolicy& policy) {
        // Save in the overlay process first, then give the identical snapshot
        // to the already-running sysmodule. This avoids the old stale-cache
        // behaviour where a visible toggle did not change active playback.
        config::set_startup_policy(policy);
        const TuneStartupPolicy wire = toIpcStartupPolicy(policy);
        if (R_FAILED(tuneSetStartupPolicy(&wire)) && tsl::notification) {
            tsl::notification->showNow(i18n::t(i18n::Str::GenericError));
        }
    }

    std::string sectionTitle(const char* title, const char* description) {
        return std::string(i18n::text(title)) + "  " + ult::DIVIDER_SYMBOL + "  "
             + i18n::text(description);
    }

    struct EqualizerPreset {
        const char* key;
        std::array<s8, TUNE_EQUALIZER_BAND_COUNT> gains;
    };

    constexpr std::array<EqualizerPreset, 5> kEqualizerPresets = {{
        {"Flat",   { 0,  0,  0,  0,  0}},
        {"Bass",   { 6,  3,  0, -1,  1}},
        {"Vocal",  {-2,  0,  3,  4,  2}},
        {"Rock",   { 4,  2, -1,  3,  4}},
        {"Bright", {-1,  0,  1,  3,  6}},
    }};

    std::array<std::string, TUNE_EQUALIZER_BAND_COUNT> equalizerBandLabels() {
        return {i18n::text("100 Hz"), i18n::text("300 Hz"), i18n::text("1 kHz"),
                i18n::text("3 kHz"), i18n::text("10 kHz")};
    }

    const char* equalizerTargetLabel(u8 target) {
        return target == TUNE_EQUALIZER_TARGET_SYSTEM
            ? i18n::text("Game/System")
            : i18n::t(i18n::Str::Music);
    }

    std::size_t detectEqualizerPreset(const TuneEqualizerSettings& settings) {
        for (std::size_t preset = 0; preset < kEqualizerPresets.size(); ++preset) {
            bool matches = true;
            for (std::size_t band = 0; band < TUNE_EQUALIZER_BAND_COUNT; ++band) {
                if (settings.gains_db[band] != kEqualizerPresets[preset].gains[band]) {
                    matches = false;
                    break;
                }
            }
            if (matches)
                return preset;
        }
        return kEqualizerPresets.size();
    }

}

// ---------------------------------------------------------------------------
// Deferred SettingsGui rebuild — set by the "Default Focus" toggle listener
// and consumed at the top of SettingsGui::update() on the next frame.
//
// We CANNOT call tsl::swapTo from inside a setStateChangedListener because
// swapTo immediately destroys the current GUI (and therefore the element
// whose onClick is still on the call stack), causing a use-after-free on the
// triggerClickAnimation() and Element::onClick() calls that follow the
// listener invocation.  Deferring to update() lets the full input chain
// unwind safely before the rebuild fires.
// ---------------------------------------------------------------------------
static bool        s_settings_rebuild_pending = false;
static std::string s_settings_rebuild_jump;
static bool        s_equalizer_state_changed = false;

void requestDeferredSettingsRebuild(std::string jumpTo) {
    s_settings_rebuild_pending = true;
    s_settings_rebuild_jump    = std::move(jumpTo);
}

/** Set when the user picks a new language; SettingsGui::update rebuilds the list. */
static bool s_settings_locale_rebuild = false;

// ---------------------------------------------------------------------------
// Return to the exact browser directory the user was in.
// The stack is always [SettingsGui, BrowserGui] — one changeTo is enough.
// Pressing B inside BrowserGui swaps to the parent directory, so the full
// tree is navigable without ever growing the stack beyond depth 2.
// ---------------------------------------------------------------------------
static void pushBrowserStack() {
    tsl::changeTo<BrowserGui>(
        g_browser_return_cwd, /*focus_name=*/"",
        g_browser_return_root, /*on_count_changed=*/nullptr);
}

// ---------------------------------------------------------------------------
// Open whatever the player's footer button points at: Settings, or the
// Playlist / Browse page the user came from. Triggered by X or a footer tap.
// ---------------------------------------------------------------------------
static void openPlayerRightPage() {
    if (g_player_right_dest == PlayerRightDest::Playlist) {
        /* swapTo replaces MainGui with SettingsGui, then changeTo stacks
           PlaylistGui on top: [SettingsGui, PlaylistGui].
           B on Playlist → [SettingsGui] → B closes. */
        play_ctx::poll();
        tsl::swapTo<SettingsGui>();
        tsl::changeTo<PlaylistGui>(nullptr);
    } else if (g_player_right_dest == PlayerRightDest::Browse) {
        /* Return to the exact directory the user was browsing, not the
           currently-playing folder. */
        play_ctx::poll();
        tsl::swapTo<SettingsGui>();
        pushBrowserStack();
    } else {
        tsl::swapTo<SettingsGui>();
    }
    triggerNavigationFeedback();
}

// =============================================================================
// MainGui  (Page 0 — Player)
// =============================================================================

MainGui::MainGui() {
    i18n::syncFromConfig();
    // Initialise play context once on overlay open.
    // Loads persisted state from /config/DferTune/ and snapshots IPC on first run.
    play_ctx::init();

    m_status_bar = new StatusBar();
}

MainGui::~MainGui() {
    blockShoulderJump.store(false, std::memory_order_release);
    // m_list cascade-deletes all its children, including m_status_bar.
    // m_frame is owned by Tesla — do NOT delete here.
    delete m_list;
}

// ---------------------------------------------------------------------------
tsl::elm::Element* MainGui::createUI() {
    i18n::syncFromConfig();
    m_right_label = (g_player_right_dest == PlayerRightDest::Playlist) ? i18n::t(i18n::Str::Playlist)
                  : (g_player_right_dest == PlayerRightDest::Browse)   ? i18n::t(i18n::Str::Browse)
                  : i18n::t(i18n::Str::Settings);
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", m_right_label);

    m_list = new tsl::elm::List();
    m_list->addItem(m_status_bar, StatusBar::PreferredHeight(tsl::cfg::FramebufferWidth - 85));

    // Pre-warm before the first draw so m_playing and m_percentage are already
    // correct on frame 0 — prevents the 0:00 flicker when swapping back from Settings.
    m_status_bar->update();

    m_frame->setContent(m_list);
    return m_frame;
}

// ---------------------------------------------------------------------------
void MainGui::update() {
    i18n::syncFromConfig();
    // L / R / ZR are player shortcuts here, so stop Tesla from also treating
    // them as "jump to top / bottom" list navigation.
    blockShoulderJump.store(true, std::memory_order_release);
    static bool s_whats_new_shown = false;
    if (!s_whats_new_shown) {
        s_whats_new_shown = true;
        maybeShowOverlayWhatsNew();
    }
    //static u8 tick = 0;
    //if ((tick % 15) == 0)
    m_status_bar->update();
    //if ((tick % 15) == 8)   // stagger from status_bar: fires midway between its updates
    play_ctx::poll();
    //++tick;

    // Detect shuffle mode changes and immediately resync g_saved to the
    // service's new queue order (the IPC call is synchronous so by the time
    // CycleShuffle() returns, the service has already reshuffled the queue).
    // Without this, g_saved stays in the pre-shuffle order until the user
    // opens PlaylistGui, causing index-based operations to use wrong positions.
    if (play_ctx::source() == play_ctx::Source::Playlist) {
        static TuneShuffleMode s_last_shuffle = TuneShuffleMode_Off;
        TuneShuffleMode cur = TuneShuffleMode_Off;
        //if ((tick % 15) == 1) {  // stagger slightly from status_bar update
        tuneGetShuffleMode(&cur);
        if (cur != s_last_shuffle) {
            s_last_shuffle = cur;
            play_ctx::resyncFromIPC();
        }
        //}
    }

    const std::string newLabel =
        (g_player_right_dest == PlayerRightDest::Playlist) ? i18n::t(i18n::Str::Playlist) :
        (g_player_right_dest == PlayerRightDest::Browse)   ? i18n::t(i18n::Str::Browse)   : i18n::t(i18n::Str::Settings);
    if (newLabel != m_right_label) {
        m_right_label = newLabel;
        if (m_frame) m_frame->setPageNames("", m_right_label);
    }
}

// ---------------------------------------------------------------------------
bool MainGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                          HidAnalogStickState joyStickPosLeft,
                          HidAnalogStickState joyStickPosRight) {

    if (m_status_bar->hasFocus())
        m_status_bar->onHeld(keysHeld);

    // Footer tap on the right-hand button.
    if (ult::simulatedNextPage.exchange(false, std::memory_order_acq_rel)) {
        openPlayerRightPage();
        return true;
    }

    if (SysTuneGui::handleInput(keysDown, keysHeld, touchPos, joyStickPosLeft, joyStickPosRight))
        return true;

    // Player shortcuts. Each only fires when pressed on its own, so they never
    // clash with button combos:
    //   X  — open Settings (or the Playlist / Browse page you came from)
    //   L  — previous track        R  — next track        ZR — play / pause
    const auto pressedAlone = [&](u64 key) {
        return (keysDown & key) && !(keysHeld & ~key & ALL_KEYS_MASK);
    };

    if (pressedAlone(KEY_X)) {
        openPlayerRightPage();
        return true;
    }
    if (pressedAlone(KEY_L)) {
        m_status_bar->hotkeyPrev();
        return true;
    }
    if (pressedAlone(KEY_R)) {
        m_status_bar->hotkeyNext();
        return true;
    }
    if (pressedAlone(KEY_ZR)) {
        m_status_bar->hotkeyPlayPause();
        return true;
    }

    return false;
}

// =============================================================================
// SettingsGui  (Page 1 — Settings)
// =============================================================================

LanguageGui::~LanguageGui() {
    delete m_list;
}

tsl::elm::Element* LanguageGui::createUI() {
    i18n::syncFromConfig();
    // No footer page button on sub-pages: B goes back.
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");
    m_list = new tsl::elm::List();

    addWrappedHeader(m_list,
        sectionTitle("Language", "Changes apply instantly"));

    const size_t selected = currentLanguageIndex();
    for (size_t i = 0; i < std::size(kLanguages); ++i) {
        // Always show the native endonym (e.g. "简体中文", "Deutsch") — never
        // translate it through the cache so the list stays language-neutral.
        const bool is_selected = (i == selected);
        const std::string lang_label = std::string(is_selected ? "● " : "  ") + kLanguages[i].label;
        // Keep the status in the right column so the list reads as two columns:
        // language name on the left, selected state on the right.
        auto *item = new tsl::elm::CompactListItem(
            lang_label, is_selected ? i18n::t(i18n::Str::Selected) : "");

        item->setClickListener([i](u64 keys) -> bool {
            if (keys & HidNpadButton_A) {
                config::set_language(kLanguages[i].code);
                reloadDferTuneTranslations();
                i18n::syncFromConfig();
                const std::string body =
                    std::string(i18n::t(i18n::Str::LanguageAppliedBody)) + "\n\n" +
                    kLanguages[i].label;
                if (tsl::notification)
                    tsl::notification->showNow(body.c_str(), 24, i18n::t(i18n::Str::Language), 3200, false);
                triggerNavigationFeedback();
                s_settings_locale_rebuild = true;
                tsl::goBack();
                return true;
            }
            return false;
        });
        m_list->addItem(item);
        m_frame->addHint(item, i18n::Hint::LanguageOption);
    }

    m_frame->setContent(m_list);
    // Jump to the currently selected language row
    m_list->jumpToItem(std::string("● ") + kLanguages[selected].label);
    return m_frame;
}

bool LanguageGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                              HidAnalogStickState joyStickPosLeft,
                              HidAnalogStickState joyStickPosRight) {
    return SysTuneGui::handleInput(keysDown, keysHeld, touchPos,
                                   joyStickPosLeft, joyStickPosRight);
}

// =============================================================================
// HelpGui / AboutGui
// =============================================================================

HelpGui::~HelpGui() {
    delete m_list;
}

tsl::elm::Element* HelpGui::createUI() {
    i18n::syncFromConfig();
    // No footer page button: B returns to Settings.
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");
    m_list = new tsl::elm::List();

    addWrappedHeader(m_list, i18n::t(i18n::Str::Help));

    const s32 rowWidth = static_cast<s32>(tsl::cfg::FramebufferWidth) - 85;
    const i18n::InfoSections help = i18n::helpSections();
    for (std::size_t i = 0; i < help.count; ++i) {
        auto *block = new TextBlock(help.items[i].heading, help.items[i].body, rowWidth);
        m_list->addItem(block, block->preferredHeight());
    }

    m_frame->setContent(m_list);
    return m_frame;
}

bool HelpGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                          HidAnalogStickState joyStickPosLeft,
                          HidAnalogStickState joyStickPosRight) {
    return SysTuneGui::handleInput(keysDown, keysHeld, touchPos,
                                   joyStickPosLeft, joyStickPosRight);
}

AboutGui::~AboutGui() {
    delete m_list;
}

tsl::elm::Element* AboutGui::createUI() {
    i18n::syncFromConfig();
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");
    m_list = new tsl::elm::List();

    addWrappedHeader(m_list, i18n::t(i18n::Str::About));

    const s32 rowWidth = static_cast<s32>(tsl::cfg::FramebufferWidth) - 85;
    const i18n::InfoSections intro = i18n::aboutSections();
    for (std::size_t i = 0; i < intro.count; ++i) {
        auto *block = new TextBlock(intro.items[i].heading, intro.items[i].body, rowWidth);
        m_list->addItem(block, block->preferredHeight());
    }

    // Author details are not translated: they are names and contact data.
    const auto addRow = [this](const char *label, const char *value) {
        m_list->addItem(new tsl::elm::CompactListItem(label, value));
    };
    addRow(i18n::t(i18n::Str::Author),  "dfer");
    addRow(i18n::t(i18n::Str::Email),   "df_business@qq.com");
    addRow("QQ",                        "3504725309");
    addRow(i18n::t(i18n::Str::Website), "http://www.dfer.site");
    addRow(i18n::t(i18n::Str::Version), VERSION);
    addRow(i18n::t(i18n::Str::License), "GPL v2");
    addRow(i18n::t(i18n::Str::BasedOn), "RyazhaTune / sys-tune");

    m_frame->setContent(m_list);
    return m_frame;
}

bool AboutGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                           HidAnalogStickState joyStickPosLeft,
                           HidAnalogStickState joyStickPosRight) {
    return SysTuneGui::handleInput(keysDown, keysHeld, touchPos,
                                   joyStickPosLeft, joyStickPosRight);
}

// =============================================================================
// StartupSettingsGui
// =============================================================================

StartupSettingsGui::~StartupSettingsGui() {
    delete m_list;
}

tsl::elm::Element* StartupSettingsGui::createUI() {
    i18n::syncFromConfig();
    // No footer page button on sub-pages: B goes back.
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");
    m_list = new tsl::elm::List();
    addWrappedHeader(m_list,
        sectionTitle("Startup", "System events"));

    const config::StartupPolicy initial = config::get_startup_policy();

    auto *auto_play = new tsl::elm::CompactToggleListItem(
        i18n::t(i18n::Str::AutoPlayStartup), initial.auto_play_startup,
        i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
    auto_play->setStateChangedListener([](bool value) {
        auto policy = config::get_startup_policy();
        policy.auto_play_startup = value;
        applyStartupPolicy(policy);
    });
    m_list->addItem(auto_play);
    m_frame->addHint(auto_play, i18n::Hint::StartupAutoPlay);

    auto *wait_home = new tsl::elm::CompactToggleListItem(
        i18n::t(i18n::Str::WaitForHome), initial.wait_for_home,
        i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
    wait_home->setStateChangedListener([](bool value) {
        auto policy = config::get_startup_policy();
        policy.wait_for_home = value;
        applyStartupPolicy(policy);
    });
    m_list->addItem(wait_home);
    m_frame->addHint(wait_home, i18n::Hint::StartupWaitHome);

    auto *keyboard = new tsl::elm::CompactToggleListItem(
        i18n::t(i18n::Str::PauseOnKeyboard), initial.pause_on_keyboard,
        i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
    keyboard->setStateChangedListener([](bool value) {
        auto policy = config::get_startup_policy();
        policy.pause_on_keyboard = value;
        applyStartupPolicy(policy);
    });
    m_list->addItem(keyboard);
    m_frame->addHint(keyboard, i18n::Hint::StartupPauseKeyboard);

    auto *controller_sync = new tsl::elm::CompactToggleListItem(
        i18n::t(i18n::Str::PauseOnControllerSync), initial.pause_on_controller_sync,
        i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
    controller_sync->setStateChangedListener([](bool value) {
        auto policy = config::get_startup_policy();
        policy.pause_on_controller_sync = value;
        applyStartupPolicy(policy);
    });
    m_list->addItem(controller_sync);
    m_frame->addHint(controller_sync, i18n::Hint::StartupPauseController);

    auto *lockscreen = new tsl::elm::CompactToggleListItem(
        i18n::t(i18n::Str::PauseOnLockscreen), initial.pause_on_lockscreen,
        i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
    lockscreen->setStateChangedListener([](bool value) {
        auto policy = config::get_startup_policy();
        policy.pause_on_lockscreen = value;
        applyStartupPolicy(policy);
    });
    m_list->addItem(lockscreen);
    m_frame->addHint(lockscreen, i18n::Hint::StartupPauseLockscreen);

    auto *remove_startup = new tsl::elm::CompactListItem(i18n::t(i18n::Str::RemoveStartup));
    remove_startup->setClickListener([](u64 keys) -> bool {
        if (!(keys & HidNpadButton_A))
            return false;
        char path[512]{};
        if (config::get_load_path(path, sizeof(path))) {
            config::set_load_path("");
            const char *name = path;
            if (const char *separator = std::strrchr(path, '/'))
                name = separator + 1;
            if (tsl::notification)
                tsl::notification->showNow(name, 26, i18n::t(i18n::Str::StartupPathRemoved), 2500, false);
        } else if (tsl::notification) {
            tsl::notification->showNow(i18n::t(i18n::Str::NoStartupPath));
        }
        return true;
    });
    m_list->addItem(remove_startup);
    m_frame->addHint(remove_startup, i18n::Hint::StartupRemove);

    m_frame->setContent(m_list);
    return m_frame;
}

bool StartupSettingsGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                                     HidAnalogStickState joyStickPosLeft,
                                     HidAnalogStickState joyStickPosRight) {
    return SysTuneGui::handleInput(keysDown, keysHeld, touchPos,
                                   joyStickPosLeft, joyStickPosRight);
}

// =============================================================================
// EqualizerGui
// =============================================================================

EqualizerGui::~EqualizerGui() {
    blockShoulderJump.store(false, std::memory_order_release);
    delete m_list;
}

bool EqualizerGui::applySettings() {
    if (R_SUCCEEDED(tuneSetEqualizerSettings(&m_settings))) {
        s_equalizer_state_changed = true;
        return true;
    } else if (tsl::notification) {
        tsl::notification->showNow(i18n::t(i18n::Str::GenericError));
    }
    return false;
}

void EqualizerGui::refreshPresetLabel() {
    if (!m_preset_item)
        return;
    const std::size_t preset = detectEqualizerPreset(m_settings);
    if (preset < kEqualizerPresets.size()) {
        m_preset_item->setValue(i18n::text(kEqualizerPresets[preset].key));
    } else {
        m_preset_item->setValue(i18n::text("Custom"));
    }
}

void EqualizerGui::refreshTargetLabel() {
    if (m_target_item)
        m_target_item->setValue(equalizerTargetLabel(m_settings.target));
}

void EqualizerGui::applyPreset(std::size_t index, bool enable) {
    if (index >= kEqualizerPresets.size())
        index = 0;

    const TuneEqualizerSettings previous = m_settings;
    if (enable)
        m_settings.enabled = 1;
    m_settings.reserved = 0;
    for (std::size_t band = 0; band < TUNE_EQUALIZER_BAND_COUNT; ++band)
        m_settings.gains_db[band] = kEqualizerPresets[index].gains[band];
    if (m_tuner)
        m_tuner->setGains(kEqualizerPresets[index].gains);
    if (enable && m_enable_toggle)
        m_enable_toggle->setState(true);
    if (!applySettings()) {
        m_settings = previous;
        if (m_tuner) {
            std::array<s8, TUNE_EQUALIZER_BAND_COUNT> restored{};
            std::copy(std::begin(m_settings.gains_db), std::end(m_settings.gains_db), restored.begin());
            m_tuner->setGains(restored);
        }
        if (m_enable_toggle)
            m_enable_toggle->setState(m_settings.enabled != 0);
    }
    refreshPresetLabel();
}

void EqualizerGui::setBandGain(std::size_t band, int gain) {
    if (band >= TUNE_EQUALIZER_BAND_COUNT)
        return;

    const s8 clamped = static_cast<s8>(std::clamp(
        gain, TUNE_EQUALIZER_MIN_GAIN_DB, TUNE_EQUALIZER_MAX_GAIN_DB));
    if (m_settings.gains_db[band] == clamped)
        return;

    const TuneEqualizerSettings previous = m_settings;
    // A fader is an active control: moving it enables the currently selected
    // backend so the user hears the change immediately instead of wondering
    // why the graph moved silently.
    m_settings.enabled = 1;
    m_settings.gains_db[band] = clamped;
    if (m_enable_toggle)
        m_enable_toggle->setState(true);
    if (!applySettings()) {
        m_settings = previous;
        if (m_enable_toggle)
            m_enable_toggle->setState(m_settings.enabled != 0);
    }
    if (m_tuner)
        m_tuner->setGain(band, m_settings.gains_db[band]);
    refreshPresetLabel();
}

tsl::elm::Element* EqualizerGui::createUI() {
    blockShoulderJump.store(false, std::memory_order_release);
    i18n::syncFromConfig();
    // No footer page button on sub-pages: B goes back.
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");
    m_list = new tsl::elm::List();

    if (R_FAILED(tuneGetEqualizerSettings(&m_settings)))
        m_settings = config::get_equalizer_settings();

    addWrappedHeader(m_list,
        sectionTitle("Live tuner", "Changes apply instantly"));

    std::array<s8, TUNE_EQUALIZER_BAND_COUNT> tunerGains{};
    std::copy(std::begin(m_settings.gains_db), std::end(m_settings.gains_db), tunerGains.begin());
    m_tuner = new EqualizerTuner(
        tunerGains, equalizerBandLabels(),
        [this](std::size_t band, s8 gain) { setBandGain(band, gain); });
    m_list->addItem(m_tuner, 226);
    m_frame->addHint(m_tuner, i18n::Hint::EqTuner);

    addWrappedHeader(m_list,
        sectionTitle("A edit/B done · L/R band · ↑/↓ gain", "Music DSP · Game/System output"));

    m_enable_toggle = new tsl::elm::CompactToggleListItem(
        i18n::text("Equalizer"), m_settings.enabled != 0,
        i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
    m_enable_toggle->setStateChangedListener([this](bool enabled) {
        const u8 previous = m_settings.enabled;
        m_settings.enabled = enabled ? 1 : 0;
        if (!applySettings()) {
            m_settings.enabled = previous;
            m_enable_toggle->setState(previous != 0);
        }
    });
    m_list->addItem(m_enable_toggle);
    m_frame->addHint(m_enable_toggle, i18n::Hint::EqEnable);

    m_target_item = new tsl::elm::CompactListItem(
        i18n::text("Target"), equalizerTargetLabel(m_settings.target));
    m_target_item->setClickListener([this](u64 keys) -> bool {
        if (!(keys & HidNpadButton_A))
            return false;
        const u8 previous = m_settings.target;
        m_settings.target = previous == TUNE_EQUALIZER_TARGET_SYSTEM
            ? TUNE_EQUALIZER_TARGET_MUSIC : TUNE_EQUALIZER_TARGET_SYSTEM;
        refreshTargetLabel();
        if (!applySettings()) {
            m_settings.target = previous;
            refreshTargetLabel();
        }
        return true;
    });
    m_list->addItem(m_target_item);
    m_frame->addHint(m_target_item, i18n::Hint::EqTarget);

    m_preset_item = new tsl::elm::CompactListItem(i18n::text("Preset"));
    refreshPresetLabel();
    m_preset_item->setClickListener([this](u64 keys) -> bool {
        if (!(keys & HidNpadButton_A))
            return false;
        const std::size_t current = detectEqualizerPreset(m_settings);
        const std::size_t next = current < kEqualizerPresets.size()
                               ? (current + 1) % kEqualizerPresets.size() : 0;
        applyPreset(next, true);
        return true;
    });
    m_list->addItem(m_preset_item);
    m_frame->addHint(m_preset_item, i18n::Hint::EqPreset);

    auto *reset = new tsl::elm::CompactListItem(
        i18n::text("Reset all"), "0 dB");
    reset->setClickListener([this](u64 keys) -> bool {
        if (!(keys & HidNpadButton_A))
            return false;
        applyPreset(0, false);
        return true;
    });
    m_list->addItem(reset);
    m_frame->addHint(reset, i18n::Hint::EqReset);

    m_frame->setContent(m_list);
    return m_frame;
}

bool EqualizerGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                               HidAnalogStickState joyStickPosLeft,
                               HidAnalogStickState joyStickPosRight) {
    const bool tunerFocused = m_tuner && m_tuner->hasFocus();
    blockShoulderJump.store(tunerFocused, std::memory_order_release);
    if (tunerFocused && m_tuner->handleTuningInput(keysDown, keysHeld))
        return true;

    // LEFT/RIGHT do not tune a vertical fader. Keep them local so LEFT cannot
    // become an accidental page-back action. B remains the predictable way
    // to leave this page.
    if (keysDown & (KEY_LEFT | KEY_RIGHT))
        return true;

    if (SysTuneGui::handleInput(keysDown, keysHeld, touchPos, joyStickPosLeft, joyStickPosRight))
        return true;
    return false;
}

// =============================================================================

SettingsGui::~SettingsGui() {
    delete m_list;
}

// ---------------------------------------------------------------------------
namespace {
    // The Playlist row shows a "playing" marker while music plays from the
    // playlist, and the usual arrow otherwise. It deliberately does NOT show a
    // song count: the count was not reliable enough to display.
    std::string playlistRowValue() {
        const bool playing = (play_ctx::source() == play_ctx::Source::Playlist)
                          && (play_ctx::currentPath()[0] != '\0');
        return playing ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL;
    }
}

void SettingsGui::refreshPlaylistCount(u32 /*count*/) {
    if (!m_queue_button) return;
    m_queue_button->setValue(playlistRowValue());
}

// ---------------------------------------------------------------------------
tsl::elm::Element* SettingsGui::createUI() {
    i18n::syncFromConfig();
    // No footer page button here: B already returns to the player.
    m_frame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");

    u64 pid{}, tid{};
    pm::getCurrentPidTid(&pid, &tid);
    m_tid      = tid;
    m_last_tid = tid; // in sync from the start so update() only fires on changes

    // Format a title ID as a hex string for use as a label.
    auto tidLabel = [](u64 t) -> std::string {
        char buf[19];
        std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(t));
        return buf;
    };

    m_list = new tsl::elm::List();

    // ---- Music Selection ----
    addWrappedHeader(m_list,
        sectionTitle("Music Library", "Choose source"));

    /* Snapshot play_ctx state once so both buttons get the right initial value
       on frame 0, before any update() tick fires. */
    const bool init_inPlaylist = (play_ctx::source() == play_ctx::Source::Playlist);
    const bool init_inFolder   = (play_ctx::source() == play_ctx::Source::Folder);
    const bool init_hasTrack   = (play_ctx::currentPath()[0] != '\0');

    m_queue_button = new tsl::elm::CompactListItem(play_ctx::activePlaylistLabel(), playlistRowValue());

    m_queue_button->setClickListener([this](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            //tsl::shiftItemFocus(m_queue_button);
            tsl::changeTo<PlaylistGui>([this](u32 count) {
                refreshPlaylistCount(count);
            });
            return true;
        }
        return false;
    });
    m_list->addItem(m_queue_button);
    m_frame->addHint(m_queue_button, i18n::Hint::PlaylistSlot);

    const std::string browseVal = (init_inFolder && init_hasTrack)
        ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL;
    auto browser_button = new tsl::elm::CompactListItem(i18n::t(i18n::Str::Browse), browseVal);
    m_browser_button = browser_button;
    browser_button->setClickListener([this, browser_button](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            //tsl::shiftItemFocus(browser_button);
            tsl::changeTo<BrowserGui>("", "", "", [this](u32 count) {
                refreshPlaylistCount(count);
            });
            return true;
        }
        return false;
    });
    m_list->addItem(browser_button);
    m_frame->addHint(browser_button, i18n::Hint::Browse);

    // ---- Volume ----
    addWrappedHeader(m_list,
        std::string(i18n::t(i18n::Str::Volume)) + " " + ult::DIVIDER_SYMBOL + " \uE13C " + i18n::t(i18n::Str::ToggleMute));

    float tune_volume = 1.f, title_volume = 1.f, default_title_volume = 1.f;
    tuneGetVolume(&tune_volume);
    tuneGetTitleVolume(&title_volume);
    tuneGetDefaultTitleVolume(&default_title_volume);

    // Load persisted pre-mute backups so Y-toggle survives overlay reopen.
    readVolBackups(m_music_vol_backup, m_game_vol_backup, m_game_default_vol_backup);

    // Initialise current-vol tracking from actual IPC values.
    m_music_vol        = static_cast<u8>(std::clamp(static_cast<int>(tune_volume          * 100.f + 0.5f), 0, 100));
    m_game_vol         = static_cast<u8>(std::clamp(static_cast<int>(title_volume         * 100.f + 0.5f), 0, 100));
    m_game_default_vol = static_cast<u8>(std::clamp(static_cast<int>(default_title_volume * 100.f + 0.5f), 0, 100));

    // Helper: same mute-toggle logic as the Y-key handler, usable from icon taps.
    // Captures SettingsGui members by pointer so the lambda stays copyable.
    auto makeMuteTap = [this](VolumeTrackBar** sliderPtr, u8* vol, u8* backup,
                               std::function<void(u8)> applyFn) -> std::function<void()> {
        return [this, sliderPtr, vol, backup, applyFn]() {
            auto* slider = *sliderPtr;
            if (!slider) return;
            if (*vol > 0) {
                *backup = *vol;
                *vol    = 0;
            } else {
                *vol = (*backup > 0) ? *backup : static_cast<u8>(100);
            }
            slider->setProgress(*vol);
            applyFn(*vol);
            writeVolBackups(m_music_vol_backup, m_game_vol_backup, m_game_default_vol_backup);
        };
    };

    auto tune_volume_slider = new VolumeTrackBar("\uE13C", false, false, true, i18n::t(i18n::Str::Music), "%", false);
    tune_volume_slider->setProgress(tune_volume * 100);
    tune_volume_slider->setValueChangedListener([this](u8 value) {
        m_music_vol = value;
        tuneSetVolume(float(value) / 100.f);
    });
    m_music_slider = tune_volume_slider;
    tune_volume_slider->setIconTapCallback(makeMuteTap(
        &m_music_slider, &m_music_vol, &m_music_vol_backup,
        [](u8 v) { tuneSetVolume(float(v) / 100.f); }));
    m_list->addItem(tune_volume_slider);
    m_frame->addHint(tune_volume_slider, i18n::Hint::MusicVolume);
    
    if (tid && pid) {
        auto title_volume_slider = new VolumeTrackBar("\uE13C", false, false, true, i18n::t(i18n::Str::Game), "%", false);
        title_volume_slider->setProgress(title_volume * 100);
        title_volume_slider->setValueChangedListener([this](u8 value) {
            m_game_vol = value;
            const float v = float(value) / 100.f;
            tuneSetTitleVolume(v);
            config::set_title_volume(m_tid, v);
        });
        m_game_slider = title_volume_slider;
        title_volume_slider->setIconTapCallback(makeMuteTap(
            &m_game_slider, &m_game_vol, &m_game_vol_backup,
            [this](u8 v) {
                const float fv = float(v) / 100.f;
                tuneSetTitleVolume(fv);
                config::set_title_volume(m_tid, fv);
            }));
        m_list->addItem(title_volume_slider);
        m_frame->addHint(title_volume_slider, i18n::Hint::GameVolume);
    }

    // ---- Sound shaping ----
    addWrappedHeader(m_list,
        sectionTitle("Sound", "Live processing"));
    TuneEqualizerSettings equalizer_state{};
    const bool equalizer_enabled =
        R_SUCCEEDED(tuneGetEqualizerSettings(&equalizer_state)) && equalizer_state.enabled;
    auto *equalizer_item = new tsl::elm::CompactListItem(
        i18n::text("5-band EQ"),
        equalizer_enabled ? equalizerTargetLabel(equalizer_state.target)
                          : i18n::t(i18n::Str::Off));
    equalizer_item->setValue(
        equalizer_enabled ? equalizerTargetLabel(equalizer_state.target)
                          : i18n::t(i18n::Str::Off),
        !equalizer_enabled);
    m_equalizer_button = equalizer_item;
    equalizer_item->setClickListener([](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<EqualizerGui>();
            return true;
        }
        return false;
    });
    m_list->addItem(equalizer_item);
    m_frame->addHint(equalizer_item, i18n::Hint::Equalizer);
    

    // ---- Auto Play ----
    // The HOME menu is a peculiar "title":
    //   - It has no per-game volume to tune, so "Preset Volume" is a no-op.
    //   - Its tid is a well-known constant rather than meaningful per-game info,
    //     so the "Title ID" header is noise.
    //   - Default Focus / Custom Focus are irrelevant at HOME because HOME
    //     always has an explicit per-title entry (set via the dedicated
    //     "Pause On Home" toggle added below).
    // Hide those widgets when the current tid is HOME.
    constexpr u64 kHomeScreenTid = 0x0100000000001000ULL;
    const bool at_home = (tid == kHomeScreenTid);

    if (!at_home) {
        auto* defaultTitleCategoryHeader = new tsl::elm::CompactCategoryHeader(
            sectionTitle("Current game", "Per-title controls"));
        defaultTitleCategoryHeader->setValue(tidLabel(tid), tsl::onTextColor);
        m_list->addItem(defaultTitleCategoryHeader);

        auto default_title_volume_slider = new VolumeTrackBar("\uE13C", false, false, true, i18n::t(i18n::Str::PresetVolume), "%", false);
        {
            float per_title_vol = 1.f;
            tuneGetDefaultTitleVolume(&per_title_vol);
            if (tid) per_title_vol = config::get_title_volume(tid);
            default_title_volume_slider->setProgress(static_cast<u8>(std::clamp(per_title_vol * 100.f + 0.5f, 0.f, 100.f)));
            m_game_default_vol = default_title_volume_slider->getProgress();
        }
        default_title_volume_slider->setValueChangedListener([this](u8 value) {
            m_game_default_vol = value;
            const float fv = float(value) / 100.f;
            // Preset Volume is the global fallback for titles without an override.
            // Do not write the current title's per-title exception here: that would
            // make the preset slider silently stop being a preset for this title.
            tuneSetDefaultTitleVolume(fv);
        });
        m_game_default_slider = default_title_volume_slider;
        default_title_volume_slider->setIconTapCallback(makeMuteTap(
            &m_game_default_slider, &m_game_default_vol, &m_game_default_vol_backup,
            [this](u8 v) {
                const float fv = float(v) / 100.f;
                // Keep this action global as well; it changes the fallback only.
                tuneSetDefaultTitleVolume(fv);
            }));
        m_list->addItem(default_title_volume_slider);
        m_frame->addHint(default_title_volume_slider, i18n::Hint::PresetVolume);

        // ---- Per-title start-up policy ----
        //
        // Two widgets work together:
        //
        //   Default Focus  (toggle) — when ON (the factory default), this
        //                             title defers to the global "Title
        //                             Focus" setting in Miscellaneous.
        //                             Turn it OFF to expose the per-title
        //                             "Custom Focus" override below.
        //
        //   Custom Focus  (tri-state cycle: Pass / Play / Pause) — only
        //                             shown when Default Focus is OFF.
        //                             Each KEY_A press cycles the state.
        //                             Storage:
        //
        //                               Pass  -> both keys absent
        //                               Play  -> title_enabled(tid) = true
        //                               Pause -> title_pause_on_start(tid) = true
        //
        // The underlying config keys (title_enabled /
        // title_pause_on_start / default_on_start) are unchanged from the
        // previous pair-of-toggles UI, so existing configs are fully
        // backward compatible.
        //
        // Toggling Default Focus does a deferred swapTo<SettingsGui> so
        // the list is rebuilt immediately: the Custom Focus row appears
        // or disappears without any stale state.
        const bool init_default_focus = config::get_default_on_start(tid);
        auto default_focus = new tsl::elm::CompactToggleListItem(
            i18n::t(i18n::Str::DefaultFocus), init_default_focus, i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
        default_focus->setStateChangedListener([tid](bool v) {
            config::set_default_on_start(tid, v);
            // Defer the rebuild to SettingsGui::update() so the full
            // onClick/handleInput call stack unwinds before swapTo fires.
            // Calling swapTo here directly would destroy the element while
            // its own onClick is still on the stack (use-after-free).
            s_settings_rebuild_pending = true;
            s_settings_rebuild_jump    = i18n::t(i18n::Str::DefaultFocus);
        });
        m_list->addItem(default_focus);
        m_frame->addHint(default_focus, i18n::Hint::DefaultFocus);

        // Only expose the per-title override when the user has opted out
        // of the global default — keeps the list clean by default.
        if (!init_default_focus) {
            // Pause takes priority over Play if both keys are somehow set
            // (matches resolvePerTitlePolicy's per-title order in
            // music_player.cpp).
            FocusMode initial = FocusMode::Pass;
            if (config::has_title_pause_on_start(tid) && config::get_title_pause_on_start(tid))
                initial = FocusMode::Pause;
            else if (config::has_title_enabled(tid) && config::get_title_enabled(tid))
                initial = FocusMode::Play;

            auto *custom_focus = new tsl::elm::CompactListItem(
                i18n::t(i18n::Str::CustomFocus), focusLabel(initial));
            // Render "Pass" faintly so it reads as "inactive", mirroring
            // how a ToggleListItem renders its OFF value.
            custom_focus->setValue(focusLabel(initial), initial == FocusMode::Pass);
            custom_focus->setClickListener(
                [custom_focus, state = initial, tid](u64 keys) mutable -> bool {
                    if (keys & HidNpadButton_A) {
                        state = focusNext(state);
                        switch (state) {
                            case FocusMode::Play:
                                config::set_title_enabled(tid, true);
                                config::clear_title_pause_on_start(tid);
                                break;
                            case FocusMode::Pause:
                                config::clear_title_enabled(tid);
                                config::set_title_pause_on_start(tid, true);
                                break;
                            case FocusMode::Pass:
                            default:
                                config::clear_title_enabled(tid);
                                config::clear_title_pause_on_start(tid);
                                break;
                        }
                        custom_focus->setValue(focusLabel(state),
                                               state == FocusMode::Pass);
                        return true;
                    }
                    return false;
                });
            m_list->addItem(custom_focus);
            m_frame->addHint(custom_focus, i18n::Hint::CustomFocus);
        }
    }

    // ---- Misc ----
    addWrappedHeader(m_list,
        sectionTitle("Playback", "Rules and language"));

    {
        auto modeLabel = []() -> const char* {
            switch (config::get_tune_mode()) {
                case config::TuneMode::Whitelist: return i18n::t(i18n::Str::ModeWhitelist);
                case config::TuneMode::Blacklist: return i18n::t(i18n::Str::ModeBlacklist);
                case config::TuneMode::Normal:
                default: return i18n::t(i18n::Str::ModeNormal);
            }
        };
        auto *mode_item = new tsl::elm::CompactListItem(
            i18n::t(i18n::Str::PlaybackMode), modeLabel());
        mode_item->setClickListener([mode_item, modeLabel](u64 keys) -> bool {
            if (!(keys & HidNpadButton_A))
                return false;
            config::TuneMode next = config::TuneMode::Normal;
            switch (config::get_tune_mode()) {
                case config::TuneMode::Normal: next = config::TuneMode::Whitelist; break;
                case config::TuneMode::Whitelist: next = config::TuneMode::Blacklist; break;
                case config::TuneMode::Blacklist: next = config::TuneMode::Normal; break;
            }
            config::set_tune_mode(next);
            mode_item->setValue(modeLabel());
            tuneApplyTitleFilter();
            return true;
        });
        m_list->addItem(mode_item);
        m_frame->addHint(mode_item, i18n::Hint::PlaybackMode);
    }

    if (tid && !at_home) {
        auto *whitelist_item = new tsl::elm::CompactToggleListItem(
            i18n::t(i18n::Str::WhitelistToggle), config::is_tid_whitelisted(tid), i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
        whitelist_item->setStateChangedListener([tid](bool v) {
            config::set_tid_whitelisted(tid, v);
            if (v)
                config::set_tid_blacklisted(tid, false);
            tuneApplyTitleFilter();
            requestDeferredSettingsRebuild(i18n::t(i18n::Str::WhitelistToggle));
        });
        m_list->addItem(whitelist_item);
        m_frame->addHint(whitelist_item, i18n::Hint::WhitelistToggle);

        auto *blacklist_item = new tsl::elm::CompactToggleListItem(
            i18n::t(i18n::Str::BlacklistToggle), config::is_tid_blacklisted(tid), i18n::t(i18n::Str::On), i18n::t(i18n::Str::Off));
        blacklist_item->setStateChangedListener([tid](bool v) {
            config::set_tid_blacklisted(tid, v);
            if (v)
                config::set_tid_whitelisted(tid, false);
            tuneApplyTitleFilter();
            requestDeferredSettingsRebuild(i18n::t(i18n::Str::BlacklistToggle));
        });
        m_list->addItem(blacklist_item);
        m_frame->addHint(blacklist_item, i18n::Hint::BlacklistToggle);
    }

    {
        const size_t language_index = currentLanguageIndex();
        auto *language_item = new tsl::elm::CompactListItem(
            i18n::t(i18n::Str::Language), kLanguages[language_index].label);
        language_item->setClickListener(
            [](u64 keys) -> bool {
                if (keys & HidNpadButton_A) {
                    tsl::changeTo<LanguageGui>();
                    return true;
                }
                return false;
            });
        m_language_button = language_item;
        m_list->addItem(language_item);
        m_frame->addHint(language_item, i18n::Hint::Language);
    }

    // Title Focus — global default applied to any title whose per-title
    // "Default Focus" is ON (the factory default). Cycling tri-state:
    //
    //   Pass  — no policy action on title focus transitions
    //           (music simply keeps doing whatever it was doing).
    //   Play  — force music to play on every title focus transition.
    //   Pause — force music to pause on every title focus transition.
    //
    // Storage is unchanged (two mutually-exclusive bool keys,
    // play_on_title / pause_on_title). The cycling widget just replaces
    // the previous pair of ON/OFF toggles with a single list item.
    //
    // Sits at the top of Miscellaneous so the user can set the
    // system-wide behaviour before drilling into per-title overrides.
    {
        // Pause takes priority over Play if both keys are somehow set
        // (matches resolvePerTitlePolicy's global order in music_player.cpp).
        FocusMode initial = FocusMode::Pass;
        if (config::get_pause_on_title())
            initial = FocusMode::Pause;
        else if (config::get_play_on_title())
            initial = FocusMode::Play;

        auto *title_focus = new tsl::elm::CompactListItem(
            i18n::t(i18n::Str::TitleFocus), focusLabel(initial));
        title_focus->setValue(focusLabel(initial), initial == FocusMode::Pass);
        title_focus->setClickListener(
            [title_focus, state = initial](u64 keys) mutable -> bool {
                if (keys & HidNpadButton_A) {
                    state = focusNext(state);
                    switch (state) {
                        case FocusMode::Play:
                            config::set_play_on_title(true);
                            config::set_pause_on_title(false);
                            break;
                        case FocusMode::Pause:
                            config::set_play_on_title(false);
                            config::set_pause_on_title(true);
                            break;
                        case FocusMode::Pass:
                        default:
                            config::set_play_on_title(false);
                            config::set_pause_on_title(false);
                            break;
                    }
                    title_focus->setValue(focusLabel(state),
                                          state == FocusMode::Pass);
                    return true;
                }
                return false;
            });
        m_list->addItem(title_focus);
        m_frame->addHint(title_focus, i18n::Hint::TitleFocus);
    }

    // Home Focus — cycling tri-state for the HOME menu press.
    // Always visible regardless of what title is running.
    //
    //   Pass  → no action when pressing HOME (music keeps its current state).
    //   Play  → music plays  when pressing HOME.
    //   Pause → music pauses when pressing HOME.
    //
    // Storage uses the same two mutually-exclusive keys as Custom Focus:
    //   Play  → title_enabled(kHomeScreenTid)        = true
    //   Pause → title_pause_on_start(kHomeScreenTid) = true
    //   Pass  → both keys absent
    {
        FocusMode home_initial = FocusMode::Pass;
        if (config::has_title_pause_on_start(kHomeScreenTid) && config::get_title_pause_on_start(kHomeScreenTid))
            home_initial = FocusMode::Pause;
        else if (config::has_title_enabled(kHomeScreenTid) && config::get_title_enabled(kHomeScreenTid))
            home_initial = FocusMode::Play;

        auto *home_focus = new tsl::elm::CompactListItem(
            i18n::t(i18n::Str::HomeFocus), focusLabel(home_initial));
        home_focus->setValue(focusLabel(home_initial), home_initial == FocusMode::Pass);
        home_focus->setClickListener(
            [home_focus, state = home_initial](u64 keys) mutable -> bool {
                if (keys & HidNpadButton_A) {
                    state = focusNext(state);
                    switch (state) {
                        case FocusMode::Play:
                            config::set_title_enabled(kHomeScreenTid, true);
                            config::clear_title_pause_on_start(kHomeScreenTid);
                            break;
                        case FocusMode::Pause:
                            config::clear_title_enabled(kHomeScreenTid);
                            config::set_title_pause_on_start(kHomeScreenTid, true);
                            break;
                        case FocusMode::Pass:
                        default:
                            config::clear_title_enabled(kHomeScreenTid);
                            config::clear_title_pause_on_start(kHomeScreenTid);
                            break;
                    }
                    home_focus->setValue(focusLabel(state), state == FocusMode::Pass);
                    return true;
                }
                return false;
            });
        m_list->addItem(home_focus);
        m_frame->addHint(home_focus, i18n::Hint::HomeFocus);
    }

    // Keep all boot and system-UI playback policies behind one deliberate
    // entry point. The child GUI applies each toggle to the running sysmodule
    // through IPC instead of only changing this overlay process's cache.
    auto *startup_settings = new tsl::elm::CompactListItem(
        i18n::t(i18n::Str::StartupSettings), ult::DROPDOWN_SYMBOL);
    startup_settings->setClickListener([](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<StartupSettingsGui>();
            return true;
        }
        return false;
    });
    m_list->addItem(startup_settings);
    m_frame->addHint(startup_settings, i18n::Hint::StartupSettings);

    // Help and About sit just above "Stop DferTune", which stays the last row.
    auto *help_item = new tsl::elm::CompactListItem(
        i18n::t(i18n::Str::Help), ult::DROPDOWN_SYMBOL);
    help_item->setClickListener([](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<HelpGui>();
            return true;
        }
        return false;
    });
    m_list->addItem(help_item);
    m_frame->addHint(help_item, i18n::Hint::HelpPage);

    auto *about_item = new tsl::elm::CompactListItem(
        i18n::t(i18n::Str::About), ult::DROPDOWN_SYMBOL);
    about_item->setClickListener([](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<AboutGui>();
            return true;
        }
        return false;
    });
    m_list->addItem(about_item);
    m_frame->addHint(about_item, i18n::Hint::AboutPage);

    auto exit_button = new tsl::elm::CompactSilentListItem(i18n::t(i18n::Str::StopDferTune));
    exit_button->setValue("\uE071", true);
    exit_button->setClickListener([exit_button](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            //tsl::shiftItemFocus(exit_button);
            tuneQuit();
            tsl::goBack();
            return true;
        }
        return false;
    });
    m_list->addItem(exit_button);
    m_frame->addHint(exit_button, i18n::Hint::StopDferTune);

    m_frame->setContent(m_list);

    // Auto-jump to whichever music source is actively playing so the user
    // lands directly on the relevant button rather than the first item.
    if (!m_jump_to.empty()) {
        m_list->jumpToItem(m_jump_to);
    } else if (init_inPlaylist && init_hasTrack) {
        m_list->jumpToItem(play_ctx::activePlaylistLabel());
    } else if (init_inFolder && init_hasTrack) {
        m_list->jumpToItem(i18n::t(i18n::Str::Browse));
    }

    return m_frame;
}

// ---------------------------------------------------------------------------
void SettingsGui::update() {
    i18n::syncFromConfig();

    if (s_settings_locale_rebuild) {
        s_settings_locale_rebuild = false;
        tsl::swapTo<SettingsGui>();
        return;
    }

    // Consume any pending rebuild request.  This is set by the "Default On
    // Start" listener and intentionally deferred here so the swapTo fires
    // after the full onClick/handleInput chain has returned — not while the
    // toggle element is still mid-callback.
    if (s_settings_rebuild_pending) {
        s_settings_rebuild_pending = false;
        tsl::swapTo<SettingsGui>(std::move(s_settings_rebuild_jump));
        return;
    }

    /* Poll IPC on a throttled schedule — it's a syscall.
       Button label updates run every tick so values snap back instantly
       the moment SettingsGui resumes after a child GUI is popped. */
    //static u8 tick = 0;
    //if ((++tick % 15) == 0)
    play_ctx::poll();

    // Re-check the running title every tick (cheap — just reads globals set by
    // pm::getCurrentPidTid which was already called recently by play_ctx::poll).
    {
        u64 pid{}, tid{};
        pm::getCurrentPidTid(&pid, &tid);
        if (tid != m_last_tid) {
            m_tid      = tid;
            m_last_tid = tid;

            // Format the new title ID.
            char buf[19];
            std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(tid));
            const std::string label(buf);

            // Refresh the "Game (default)" slider — relabel and reload preset.
            if (m_game_default_slider) {
                m_game_default_slider->setLabel(label);
                float vol = 1.f;
                tuneGetDefaultTitleVolume(&vol);
                if (tid) vol = config::get_title_volume(tid);
                const u8 pct = static_cast<u8>(std::clamp(vol * 100.f + 0.5f, 0.f, 100.f));
                m_game_default_slider->setProgress(pct);
                m_game_default_vol = pct;
            }
        }
    }

    const bool inFolder   = (play_ctx::source() == play_ctx::Source::Folder);
    const bool hasTrack   = (play_ctx::currentPath()[0] != '\0');

    if (m_queue_button)
        m_queue_button->setValue(playlistRowValue());

    if (m_browser_button) {
        m_browser_button->setValue(
            (inFolder && hasTrack) ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
    }

    if (m_language_button) {
        m_language_button->setValue(kLanguages[currentLanguageIndex()].label);
    }

    if (m_equalizer_button && s_equalizer_state_changed) {
        s_equalizer_state_changed = false;
        TuneEqualizerSettings settings{};
        if (R_SUCCEEDED(tuneGetEqualizerSettings(&settings))) {
            m_equalizer_button->setValue(
                settings.enabled ? equalizerTargetLabel(settings.target)
                                 : i18n::t(i18n::Str::Off),
                settings.enabled == 0);
        }
    }
}

// ---------------------------------------------------------------------------
bool SettingsGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                              HidAnalogStickState joyStickPosLeft,
                              HidAnalogStickState joyStickPosRight) {

    if (ult::simulatedNextPage.exchange(false, std::memory_order_acq_rel)) {
        setPlayerRightDest(PlayerRightDest::Settings);
        tsl::swapTo<MainGui>();
        triggerNavigationFeedback();
        return true;
    }

    // Settings replaces the player at the same stack depth, so Tesla's
    // default B action would close the overlay. Make B consistently return to
    // the player instead. Horizontal input remains available to sliders.
    if (keysDown & HidNpadButton_B) {
        setPlayerRightDest(PlayerRightDest::Settings);
        tsl::swapTo<MainGui>();
        triggerExitFeedback();
        return true;
    }

    // Y — mute/restore toggle on the focused volume slider.
    // If the slider is at any non-zero value, back it up and set to 0.
    // If it is already at 0, restore from the backup (or 100 if no backup).
    // The backup values are written to disk so they survive overlay reopen.
    if (keysDown & KEY_Y) {
        auto doMuteToggle = [&](VolumeTrackBar* slider, u8& vol, u8& backup,
                                std::function<void(u8)> applyFn) -> bool {
            if (!slider || !slider->hasFocus()) return false;
            if (vol > 0) {
                backup = vol;           // store current level
                vol    = 0;
            } else {
                vol = (backup > 0) ? backup : static_cast<u8>(100);  // restore
            }
            slider->setProgress(vol);
            applyFn(vol);
            writeVolBackups(m_music_vol_backup, m_game_vol_backup, m_game_default_vol_backup);
            triggerNavigationFeedback();
            return true;
        };

        if (doMuteToggle(m_music_slider, m_music_vol, m_music_vol_backup,
            [](u8 v) { tuneSetVolume(float(v) / 100.f); }))
            return true;

        if (doMuteToggle(m_game_slider, m_game_vol, m_game_vol_backup,
            [this](u8 v) {
                const float fv = float(v) / 100.f;
                tuneSetTitleVolume(fv);
                config::set_title_volume(m_tid, fv);
            }))
            return true;

        if (doMuteToggle(m_game_default_slider, m_game_default_vol, m_game_default_vol_backup,
            [](u8 v) { tuneSetDefaultTitleVolume(float(v) / 100.f); }))
            return true;
    }

    if (SysTuneGui::handleInput(keysDown, keysHeld, touchPos, joyStickPosLeft, joyStickPosRight))
        return true;

    return false;
}
