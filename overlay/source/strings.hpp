#pragma once

#include <cstdint>

/** Overlay UI strings: call syncFromConfig() once per GUI frame before t(). */
namespace i18n {

void syncFromConfig();

enum class Str : std::uint8_t {
    Player,
    Settings,
    Playlist,
    Browse,
    MusicLibrary,
    Volume,
    ToggleMute,
    Music,
    Game,
    TitleId,
    PresetVolume,
    DefaultFocus,
    CustomFocus,
    TitleFocus,
    HomeFocus,
    Miscellaneous,
    PlaybackMode,
    ModeNormal,
    ModeWhitelist,
    ModeBlacklist,
    WhitelistToggle,
    BlacklistToggle,
    Language,
    StartupSettings,
    AutoPlayStartup,
    WaitForHome,
    PauseOnKeyboard,
    PauseOnControllerSync,
    PauseOnLockscreen,
    RemoveStartup,
    StopDferTune,
    On,
    Off,
    Pass,
    Play,
    Pause,
    CategoryLanguage,
    EmptyFolder,
    Tracks,
    PlaylistHeader,
    PlaylistEmpty,
    CouldNotOpenPrefix,
    AddToPlaylistShort,
    AddAll,
    SetAsStartupShort,
    ScanStoppedTitle,
    ScanStoppedBody,
    FailedSwitchFolder,
    AddedOneTrack,
    FailedAddTrack,
    StartupFileSet,
    StartupFolderSet,
    StartupPathRemoved,
    NoStartupPath,
    GenericError,
    UnknownArtist,
    LanguageAppliedBody,
    EmptyPlaylistBrowseHint,
    EmptyPlaylistShortcutsHint,
    TrackRemovedUndoHint,
    TrackRestoredToast,
    WhatsNewBody,
    ByArtist,
    /** snprintf format, one arg: long long count */
    AddedManyTracksFmt,
    TrackCountOne,
    /** snprintf format, one arg: unsigned count — used when not Russian */
    TrackCountManyFmt,
    Shuffle,
    Previous,
    Next,
    Repeat,
    Select,
    Back,
    Selected,
    Remove,
    RemoveAll,
    ReplacedOneTrack,
    TrackAlreadyAdded,
    /** snprintf format, three args: long long added, replaced, skipped */
    AddedManyTracksDedupFmt,
    Ok,
    Count_
};

/** Floating tooltip texts shown while a row has focus. Built in for every
 *  bundled language so they do not depend on the lang files on the SD card. */
enum class Hint : std::uint8_t {
    PlaylistSlot,
    Browse,
    MusicVolume,
    GameVolume,
    PresetVolume,
    Equalizer,
    DefaultFocus,
    CustomFocus,
    PlaybackMode,
    WhitelistToggle,
    BlacklistToggle,
    Language,
    TitleFocus,
    HomeFocus,
    StartupSettings,
    StopDferTune,
    StartupAutoPlay,
    StartupWaitHome,
    StartupPauseKeyboard,
    StartupPauseController,
    StartupPauseLockscreen,
    StartupRemove,
    LanguageOption,
    EqTuner,
    EqEnable,
    EqTarget,
    EqPreset,
    EqReset,
    BrowserRow,
    PlaylistTrack,
    BtnShuffle,
    BtnPrev,
    BtnPlay,
    BtnNext,
    BtnRepeat,
    SeekBar,
    Count_
};

const char *t(Str id);

/** Tooltip text for the active language (falls back to English). */
const char *hint(Hint id);

/** Translate a runtime English key from overlay/lang, falling back to English. */
const char *text(const char *englishKey);

/** Localized "1 track" / "N tracks" (or Russian plural forms). */
const char *trackCountLabel(std::uint32_t count);

} // namespace i18n
