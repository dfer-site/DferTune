#pragma once

#include <switch.h>
#include "tune.h"

namespace config {

// tune shuffle
auto get_shuffle() -> bool;
void set_shuffle(bool value);

// tune repeat
auto get_repeat() -> int;
void set_repeat(int value);

// tune volume
auto get_volume() -> float;
void set_volume(float value);

// per title tune enable (Play On Start — music plays when this title launches)
auto has_title_enabled(u64 tid) -> bool;
auto get_title_enabled(u64 tid) -> bool;
void set_title_enabled(u64 tid, bool value);

// per title Pause On Start — music pauses when this title launches.
// Stored separately from title_enabled so the UI can offer a tri-state
// (Play On Start, Pause On Start, or neither). The two are mutually
// exclusive by convention enforced in the UI.
auto has_title_pause_on_start(u64 tid) -> bool;
auto get_title_pause_on_start(u64 tid) -> bool;
void set_title_pause_on_start(u64 tid, bool value);
void clear_title_pause_on_start(u64 tid);
void clear_title_enabled(u64 tid);

// default for tune for every title (LEGACY — kept for config-file
// backward compatibility; no longer consulted by the policy engine).
auto get_title_enabled_default() -> bool;
void set_title_enabled_default(bool value);

// Complete startup/system-context policy. It is intentionally grouped so
// overlay and sysmodule can update every related switch as one snapshot.
struct StartupPolicy {
    bool auto_play_startup{false};
    bool wait_for_home{false};
    bool pause_on_keyboard{false};
    bool pause_on_controller_sync{false};
    bool pause_on_lockscreen{false};
};

StartupPolicy get_startup_policy();
void set_startup_policy(const StartupPolicy& policy);

// Auto-play Startup — when the sysmodule first launches, start music
// playing automatically if a startup playlist is set. Applies regardless
// of which title is in foreground at boot; per-title Play On Start and
// Pause On Start take over on subsequent title transitions.
auto get_auto_play_startup() -> bool;
void set_auto_play_startup(bool value);

// Wait For Home — gate Auto-play Startup on the home menu being up.
//
// boot2 launches this sysmodule long before the home menu is drawn, so
// with Auto-play Startup ON the music begins during the Nintendo Switch
// boot logo.  When this is ON, startup playback is held until qlaunch is
// observed running — i.e. until the console has finished booting into
// the home menu.
//
// Gates only the BOOT-time auto-play.  Every other route to playback
// (per-title policy, the overlay's Play button, IPC) is unaffected, and
// the gate is a no-op when Auto-play Startup is OFF.
auto get_wait_for_home() -> bool;
void set_wait_for_home(bool value);

// Pause On Keyboard — pause while the system software keyboard is open.
//
// The sysmodule has no applet session, so this is detected from the pdm
// play-event log: swkbd opening is logged as an applet Launch/InFocus
// event for AppletId_LibraryAppletSwkbd, and its close as Exit/OutOfFocus.
// The pre-keyboard pause state is snapshotted and restored on close, so a
// keyboard that opens over already-paused music leaves it paused.
auto get_pause_on_keyboard() -> bool;
void set_pause_on_keyboard(bool value);

// Pause On Controller Sync — pause while the system ControllerSupport
// library applet (the controller pairing/sync popup) is open. It uses the
// same play-event mechanism as the software keyboard, while keeping a
// separate opt-in because controller pairing is less frequent.
auto get_pause_on_controller_sync() -> bool;
void set_pause_on_controller_sync(bool value);

// Pause On Lockscreen — pause while the console sits on the lock screen.
//
// The lock screen is drawn by qlaunch and reports the same title id as
// the home menu, so it cannot be told apart by title alone.  It is
// instead inferred from the power-state cycle: the console pauses on
// entering sleep and stays paused after waking (the lock screen is what
// greets the user), resuming only once the unlock is observed as a fresh
// applet focus event.
auto get_pause_on_lockscreen() -> bool;
void set_pause_on_lockscreen(bool value);

// Global title-transition defaults — applied to any title whose
// per-title "Default On Start" flag is ON (the factory default).
// Mutually exclusive by convention (UI enforces it).
//   Play On Title  -> music plays  when any title launches
//   Pause On Title -> music pauses when any title launches
auto get_play_on_title() -> bool;
void set_play_on_title(bool value);
auto get_pause_on_title() -> bool;
void set_pause_on_title(bool value);

// Per-title opt-in to the global defaults above.
// Defaults to true — a fresh title uses Play/Pause On Title until the
// user explicitly turns this OFF and configures per-title overrides.
auto get_default_on_start(u64 tid) -> bool;
void set_default_on_start(u64 tid, bool value);

// per title volume
auto has_title_volume(u64 tid) -> bool;
auto get_title_volume(u64 tid) -> float;
void set_title_volume(u64 tid, float value);

// default volume for every title
auto get_default_title_volume() -> float;
void set_default_title_volume(float value);

// returns the length of the string
auto get_load_path(char* out, int max_len) -> int;
void set_load_path(const char* path);

enum class TuneMode : int {
    Normal = 0,
    Whitelist = 1,
    Blacklist = 2,
};

auto get_tune_mode() -> TuneMode;
void set_tune_mode(TuneMode mode);
auto is_tid_whitelisted(u64 tid) -> bool;
void set_tid_whitelisted(u64 tid, bool value);
auto is_tid_blacklisted(u64 tid) -> bool;
void set_tid_blacklisted(u64 tid, bool value);
auto is_title_allowed(u64 tid) -> bool;

// overlay language; defaults to Russian ("ru") for fresh configs.
void ensure_language_config();
auto get_language(char* out, int max_len) -> int;
void set_language(const char* language);

// Five-band equalizer for DferTune music or system/game output.
TuneEqualizerSettings get_equalizer_settings();
void set_equalizer_settings(const TuneEqualizerSettings& settings);

}
