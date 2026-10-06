#pragma once

#include <cstdint>

/** Overlay UI strings: call syncFromConfig() once per GUI frame before t(). */
namespace i18n {

void syncFromConfig();

/** True after syncFromConfig() if the active overlay language is Russian. */
bool isRu();

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
    Count_
};

const char *t(Str id);

/** Translate a runtime English key from overlay/lang, falling back to English. */
const char *text(const char *englishKey);

/** Localized "1 track" / "N tracks" (or Russian plural forms). */
const char *trackCountLabel(std::uint32_t count);

} // namespace i18n
