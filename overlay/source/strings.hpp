#pragma once

#include <cstddef>
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
    /** snprintf format, four args: long long added, replaced, skipped, failed */
    AddedManyTracksDedupFmt,
    Ok,
    Help,
    About,
    Version,
    Author,
    Email,
    Website,
    License,
    BasedOn,
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
    HelpPage,
    AboutPage,
    Count_
};

/** One paragraph of the Help / About pages. */
struct InfoSection {
    const char *heading;
    const char *body;
};

struct InfoSections {
    const InfoSection *items;
    std::size_t        count;
};

/** Built-in Help paragraphs for the active language (falls back to English). */
InfoSections helpSections();

/** Built-in introduction shown at the top of the About page. */
InfoSections aboutSections();

const char *t(Str id);

/** Tooltip text for the active language (falls back to English). */
const char *hint(Hint id);

/** Translate a runtime English key from overlay/lang, falling back to English. */
const char *text(const char *englishKey);

/** Localized "1 track" / "N tracks" (or Russian plural forms). */
const char *trackCountLabel(std::uint32_t count);

} // namespace i18n
