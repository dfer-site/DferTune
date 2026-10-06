#include "gui_playlist.hpp"
#include "gui_main.hpp"

#include "elm_overlayframe.hpp"
#include "config/config.hpp"
#include "play_context.hpp"
#include "tag_reader.hpp"
#include "symbol.hpp"
#include "tune.h"
#include "strings.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

// =============================================================================
// UI helpers
// =============================================================================

    constexpr float kIndicatorScale = 0.65f;
    constexpr s32   kSymMargin      = 19;

    /* Deferred focus item — set by click handlers, resolved in update(). */
    class ButtonListItem;
    ButtonListItem *g_focus_item = nullptr;

    /* Fire the count callback with the saved-playlist size. */
    void notifyCountChanged(const std::function<void(u32)> &cb) {
        if (!cb) return;
        cb(play_ctx::savedPlaylistSize());
    }

    struct PlaylistUndoState {
        bool        active        = false;
        u16         ttl           = 0;
        std::string path;
        u32         idx           = 0;
        bool        playlist_ctx = false;
    };

    PlaylistUndoState g_playlist_undo;

    static void drawPlaylistEmpty(tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h) {
        i18n::syncFromConfig();
        const s32 cx = x + w / 2;
        const s32 mid = y + h / 2;

        const char *icon = "\uE150";
        const u16 iw = r->getTextDimensions(icon, false, 90).first;
        r->drawString(icon, false, cx - static_cast<s32>(iw) / 2, mid - 100, 90, tsl::defaultTextColor);

        const char *line1 = i18n::t(i18n::Str::PlaylistEmpty);
        const u16   w1   = r->getTextDimensions(line1, false, 25).first;
        r->drawString(line1, false, cx - static_cast<s32>(w1) / 2, mid - 18, 25, tsl::defaultTextColor);

        const char *line2 = i18n::t(i18n::Str::EmptyPlaylistBrowseHint);
        const u16   w2   = r->getTextDimensions(line2, false, 20).first;
        r->drawString(line2, false, cx - static_cast<s32>(w2) / 2, mid + 28, 20, tsl::defaultTextColor);

        const char *line3 = i18n::t(i18n::Str::EmptyPlaylistShortcutsHint);
        const u16   w3   = r->getTextDimensions(line3, false, 18).first;
        r->drawString(line3, false, cx - static_cast<s32>(w3) / 2, mid + 72, 18, tsl::defaultTextColor);
    }

// =============================================================================
// ButtonListItem
//
// Label format: "<entry_num>  ⊘  <song> by <artist>"
//
// The play/pause indicator is shown ONLY when:
//   • play_ctx::source() == Playlist  (IPC queue == saved[])
//   • this item's full_path matches play_ctx::currentPath()
//
// When source == Folder the user is playing from a folder, so no indicator
// appears in the playlist view.
// =============================================================================

    class ButtonListItem final : public tsl::elm::ListItem {
        std::string m_full_path;
        std::string m_song_name;
        std::string m_artist_name;
        u32         m_entry_num;
        bool        m_was_current = false;

        static constexpr const char *kPlaceholder = "\uE098";

    public:
        static std::string buildLabel(u32 num,
                                      const std::string &song,
                                      const std::string &artist) {
            i18n::syncFromConfig();
            const char *sep = i18n::t(i18n::Str::ByArtist);
            std::string s;
            s.reserve(8 + ult::DIVIDER_SYMBOL.size() + song.size() + std::strlen(sep) + artist.size());
            s += std::to_string(num);
            s += ult::DIVIDER_SYMBOL;
            s += song;
            s += sep;
            s += artist;
            return s;
        }

        ButtonListItem(u32 entry_num,
                       std::string song_name,
                       std::string artist_name,
                       std::string full_path)
            : ListItem(buildLabel(entry_num, song_name, artist_name),
                       /*value=*/"", /*isMini=*/true)
            , m_full_path  (std::move(full_path))
            , m_song_name  (std::move(song_name))
            , m_artist_name(std::move(artist_name))
            , m_entry_num  (entry_num) {}

        const std::string &getFullPath() const { return m_full_path; }

        void drawSeparators(tsl::gfx::Renderer*) override {}

        std::string getLabel() const {
            return buildLabel(m_entry_num, m_song_name, m_artist_name);
        }

        void setEntryNumber(u32 num) {
            if (m_entry_num == num) return;
            m_entry_num  = num;
            const std::string newLabel = buildLabel(num, m_song_name, m_artist_name);
            m_text       = newLabel;
            m_text_clean = newLabel;
            m_maxWidth          = 0;
            m_textWidth         = 0;
            m_scrollText.clear();
            m_ellipsisText.clear();
            m_flags.m_truncated = false;
        }

        /* True only when Playlist context is active AND this path is current. */
        bool isCurrent() const {
            if (play_ctx::source() != play_ctx::Source::Playlist) return false;
            const char *cp = play_ctx::currentPath();
            return !m_full_path.empty() && cp[0] != '\0' &&
                   strcasecmp(m_full_path.c_str(), cp) == 0;
        }

        void draw(tsl::gfx::Renderer *renderer) override {
            const bool cur = isCurrent();

            if (cur != m_was_current) {
                m_was_current = cur;
                setValue(cur ? kPlaceholder : "");
                m_maxWidth = 0;   // force layout recalc on state change
            }

            if (!cur) {
                ListItem::draw(renderer);
                return;
            }

            // ---- Indicator path --------------------------------------------
            const s32 scaledW = static_cast<s32>(26.0f * kIndicatorScale);

            if (!m_maxWidth) {
                const u16 textMaxW = static_cast<u16>(
                    static_cast<s32>(getWidth()) - kSymMargin - scaledW - kSymMargin - 19);
                m_maxWidth = static_cast<u16>(
                    static_cast<s32>(getWidth()) - kSymMargin - scaledW - kSymMargin - 55);
                const u16 textW = static_cast<u16>(
                    renderer->getTextDimensions(m_text_clean, false, 19).first);
                m_flags.m_truncated = (textW > textMaxW);
                if (m_flags.m_truncated) {
                    m_scrollText.clear();
                    m_scrollText.reserve(m_text_clean.size() * 2 + 8);
                    m_scrollText.append(m_text_clean).append("        ");
                    m_textWidth = static_cast<u16>(
                        renderer->getTextDimensions(m_scrollText, false, 19).first);
                    m_scrollText.append(m_text_clean);
                    m_ellipsisText = renderer->limitStringLength(
                        m_text_clean, false, 19, textMaxW);
                } else {
                    m_textWidth = static_cast<u16>(textW);
                }
            }

            {
                // Use swap to avoid a heap allocation on every frame.
                std::string saved_value;
                saved_value.swap(m_value);
                ListItem::draw(renderer);
                m_value.swap(saved_value);
            }

            const s32 cx = getX() + static_cast<s32>(getWidth()) - kSymMargin - scaledW / 2;
            const s32 cy = getY() + static_cast<s32>(m_listItemHeight) / 2;

            const bool useClickTextColor =
                m_touched &&
                Element::getInputMode() == tsl::InputMode::Touch &&
                ult::touchInBounds;

            const tsl::Color symColor = (m_focused && ult::useSelectionValue)
                ? (useClickTextColor ? tsl::clickTextColor : tsl::selectedValueTextColor)
                : tsl::onTextColor;

            const auto &sym = play_ctx::isPlaying()
                ? symbol::pause::symbol
                : symbol::play::symbol;

            sym.draw(play_ctx::isPlaying() ? cx - 1 : cx, cy, renderer, symColor, kIndicatorScale);
        }

        bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY,
                     s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
            if (event == tsl::elm::TouchEvent::Touch)
                this->m_touched = this->inBounds(currX, currY);

            if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
                this->m_touched = false;
                if (Element::getInputMode() == tsl::InputMode::Touch) {
                    m_clickAnimationProgress = 0;
                    const bool handled = onClick(HidNpadButton_A);
                    //if (handled) tsl::shiftItemFocus(this);
                    return handled;
                }
            }
            return false;
        }
    };

} // namespace

// =============================================================================
// PlaylistGui constructor
//
// Builds the UI from play_ctx::savedPlaylist() — the user's persisted list —
// regardless of whether we are currently in Playlist or Folder context.
// =============================================================================

PlaylistGui::~PlaylistGui() {
    g_playlist_undo = {};
    delete m_list;
}

PlaylistGui::PlaylistGui(std::function<void(u32)> on_count_changed)
    : m_on_count_changed(std::move(on_count_changed)) {

    i18n::syncFromConfig();
    m_list = new tsl::elm::List();

    // Re-snapshot the IPC queue order into g_saved so the list always reflects
    // the actual playback order (including any shuffle the service has applied).
    // This also calls poll() internally to refresh currentPath().
    play_ctx::resyncFromIPC();

    const auto &saved = play_ctx::savedPlaylist();
    const u32   count = static_cast<u32>(saved.size());

    if (count == 0) {
        m_list->addItem(new tsl::elm::CustomDrawer(
            [](tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h) { drawPlaylistEmpty(r, x, y, w, h); }), 420);
        return;
    }

    const std::string playlist_hint =
        play_ctx::activePlaylistLabel() + "  " +
        "A " + i18n::t(i18n::Str::Play) + "  " +
        "Y " + i18n::t(i18n::Str::Remove) + "  " +
        "X " + i18n::t(i18n::Str::RemoveAll) + "  " +
        "− " + i18n::t(i18n::Str::SetAsStartupShort) + "  " +
        "\uE0EB\uE0EC " + i18n::t(i18n::Str::Playlist);
    m_list->addItem(new tsl::elm::CompactCategoryHeader(playlist_hint, true));

    m_items.reserve(count);

    // Jump target: only meaningful when source == Playlist (indicator would
    // be shown) — otherwise we leave the list at the top.
    const bool   inPlaylistCtx = (play_ctx::source() == play_ctx::Source::Playlist);
    const char  *cp            = play_ctx::currentPath();
    std::string  current_label;

    for (u32 i = 0; i < count; ++i) {
        const std::string &full_path = saved[i];

        TitleArtist ta = readTitleArtist(full_path.c_str());

        auto *item = new ButtonListItem(i + 1,
                                        std::move(ta.title),
                                        std::move(ta.artist),
                                        full_path);

        // Only record a jump target when the playlist is the active context.
        if (inPlaylistCtx && cp[0] != '\0' &&
            !strcasecmp(full_path.c_str(), cp) &&
            current_label.empty())
        {
            current_label = item->getLabel();
        }

        item->setClickListener([this, item](u64 keys) -> bool {
            const auto index      = this->m_list->getIndexInList(item);
            const auto tune_index = index - 1;  // subtract 1 for the header

            // ---- A: play / toggle / context-switch -------------------------
            if (keys & HidNpadButton_A) {

                if (play_ctx::source() == play_ctx::Source::Playlist) {
                    // Already in playlist context — toggle or select.
                    const bool isCurrentTrack =
                        play_ctx::currentPath()[0] != '\0' &&
                        strcasecmp(item->getFullPath().c_str(),
                                   play_ctx::currentPath()) == 0;

                    if (isCurrentTrack) {
                        if (play_ctx::isPlaying()) { tunePause(); }
                        else                       { tunePlay();  }
                    } else {
                        tuneSelect(tune_index);
                    }
                    play_ctx::poll();

                } else {
                    // Folder context — g_saved is in original sorted order and
                    // tune_index is the sorted position.  If shuffle is on,
                    // tuneSelect(tune_index) would play m_shuffle_playlist[tune_index]
                    // which is a random song, not the one clicked.  Disable shuffle
                    // first so the sorted index maps correctly, then switch context.
                    {
                        TuneShuffleMode shuffleMode = TuneShuffleMode_Off;
                        tuneGetShuffleMode(&shuffleMode);
                        if (shuffleMode != TuneShuffleMode_Off) {
                            tuneSetShuffleMode(TuneShuffleMode_Off);
                            config::set_shuffle(TuneShuffleMode_Off);
                        }
                    }
                    play_ctx::switchToPlaylist(static_cast<u32>(tune_index));
                    play_ctx::poll();
                }
                return true;

            // ---- Y: remove this item from saved playlist -------------------
            } else if (keys & KEY_Y) {
                const std::string undo_path = item->getFullPath();
                const u32         undo_idx  = static_cast<u32>(tune_index);
                const bool        undo_pl   = (play_ctx::source() == play_ctx::Source::Playlist);

                bool ok = false;
                if (play_ctx::source() == play_ctx::Source::Playlist) {
                    // IPC and saved[] are in sync — remove from both.
                    ok = play_ctx::playlistTuneRemove(static_cast<u32>(tune_index));
                } else {
                    // Folder context — IPC has folder songs; only remove from saved[].
                    play_ctx::savedRemove(static_cast<u32>(tune_index));
                    ok = true;
                }

                if (ok) {
                    g_playlist_undo.active        = true;
                    g_playlist_undo.ttl           = 240;
                    g_playlist_undo.path          = undo_path;
                    g_playlist_undo.idx           = undo_idx;
                    g_playlist_undo.playlist_ctx  = undo_pl;
                    i18n::syncFromConfig();
                    if (tsl::notification)
                        tsl::notification->showNow(i18n::t(i18n::Str::TrackRemovedUndoHint), 22,
                                                   i18n::t(i18n::Str::Playlist), 3200, false);

                    const s32 nextIndex = (index + 1 < this->m_list->getLastIndex())
                        ? index + 1 : index - 1;
                
                    this->removeFocus();
                    this->m_list->removeIndex(index);
                
                    if ((size_t)tune_index < this->m_items.size())
                        this->m_items.erase(this->m_items.begin() + tune_index);
                
                    if (this->m_items.empty()) {
                        this->m_list->clear();

                        m_list->addItem(new tsl::elm::CustomDrawer(
                            [](tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h) {
                                drawPlaylistEmpty(r, x, y, w, h);
                            }), 420);

                        notifyCountChanged(m_on_count_changed);
                        triggerMoveFeedback();
                        return true;
                    }
                
                    // Re-number remaining items
                    for (size_t k = (size_t)tune_index; k < this->m_items.size(); ++k)
                        static_cast<ButtonListItem*>(this->m_items[k])->setEntryNumber(
                            static_cast<u32>(k + 1));
                
                    this->m_list->setFocusedIndex(nextIndex);
                
                    notifyCountChanged(m_on_count_changed);
                    triggerMoveFeedback();
                }
                return true;

            // ---- X: clear entire playlist ----------------------------------
            } else if (keys & KEY_X) {
                bool ok = false;
                if (play_ctx::source() == play_ctx::Source::Playlist) {
                    ok = play_ctx::playlistTuneClearQueue();
                } else {
                    play_ctx::savedClear();
                    ok = true;
                }

                if (ok) {
                    g_playlist_undo = {};
                    this->removeFocus();
                    this->m_list->clear();
                    this->m_items.clear();
                    m_list->addItem(new tsl::elm::CustomDrawer(
                        [](tsl::gfx::Renderer *r, s32 x, s32 y, s32 w, s32 h) { drawPlaylistEmpty(r, x, y, w, h); }),
                        420);
                    if (m_on_count_changed) {
                        triggerMoveFeedback();
                        m_on_count_changed(0);
                    }
                }
                return true;

            // ---- Minus: set as startup path --------------------------------
            } else if (keys & KEY_MINUS) {
                // Use saved[] path directly — it's the authoritative source.
                const auto &sp = play_ctx::savedPlaylist();
                if ((size_t)tune_index < sp.size()) {
                    config::set_load_path(sp[tune_index].c_str());
                    if (tsl::notification)
                        tsl::notification->showNow(item->getText(), 26, i18n::t(i18n::Str::StartupFileSet), 2500, false);
                }
                return true;
            }
            return false;
        });

        m_list->addItem(item);
        m_items.push_back(item);
    }

    // Jump to currently playing item (only when in Playlist context and
    // the track is actually in this list).
    if (!current_label.empty())
        m_list->jumpToItem(current_label);
}

// =============================================================================

tsl::elm::Element *PlaylistGui::createUI() {
    i18n::syncFromConfig();
    // No footer page button: B returns to Settings, and from there to the player.
    auto *rootFrame = new SysTuneOverlayFrame(/*pageLeft=*/"", /*pageRight=*/"");
    rootFrame->addListHint(this->m_list, i18n::Hint::PlaylistTrack);
    rootFrame->setContent(this->m_list);
    return rootFrame;
}

// =============================================================================

void PlaylistGui::update() {
    i18n::syncFromConfig();
    if (g_playlist_undo.active) {
        if (g_playlist_undo.ttl > 0)
            --g_playlist_undo.ttl;
        else
            g_playlist_undo = {};
    }

    // Resolve any deferred focus change set during a click handler.
    if (g_focus_item) {
        const auto index = m_list->getIndexInList(g_focus_item);
        if (index >= 0) {
            this->removeFocus();
            this->requestFocus(g_focus_item, tsl::FocusDirection::Down);
            m_list->setFocusedIndex(index);
            g_focus_item = nullptr;
        }
    }

    static u8 tick = 0;
    if ((++tick % 15) == 0)
        play_ctx::poll();
}

// ---------------------------------------------------------------------------
bool PlaylistGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                              HidAnalogStickState joyStickPosLeft,
                              HidAnalogStickState joyStickPosRight) {
    // --- Analog stick left/right handling for playlist switching ---
    static u64 lastStickInputTime = 0;
    u64 currentTime = svcGetSystemTick();
    
    // Check for significant left/right stick movement
    if (joyStickPosLeft.x < -0.7f || joyStickPosLeft.x > 0.7f) {
        // Debounce: only trigger every 200ms
        if (currentTime - lastStickInputTime > 200 * 1000 * 1000) {
            lastStickInputTime = currentTime;
            if (joyStickPosLeft.x > 0.7f) {
                keysDown |= KEY_RIGHT;  // Emulate RIGHT for next playlist
            } else if (joyStickPosLeft.x < -0.7f) {
                keysDown |= KEY_LEFT;   // Emulate LEFT for previous playlist
            }
        }
    }

    // The footer returns to the player; D-pad LEFT/RIGHT exclusively switches
    // playlist slots and can no longer be mistaken for page navigation.
    const bool goLeft = ult::simulatedNextPage.exchange(false, std::memory_order_acq_rel);
    if (goLeft) {
        setPlayerRightDest(PlayerRightDest::Playlist);
        tsl::swapTo<MainGui>(SwapDepth{2});
        triggerNavigationFeedback();
        return true;
    }

    if (keysDown & HidNpadButton_B) {
        tsl::goBack();
        triggerExitFeedback();
        return true;
    }

    // Handle playlist switching with LEFT/RIGHT keys (including emulated from stick)
    if (((keysDown & KEY_PLUS) || (keysDown & KEY_LEFT) || (keysDown & KEY_RIGHT)) && 
        !(keysHeld & ~(KEY_PLUS | KEY_LEFT | KEY_RIGHT) & ALL_KEYS_MASK)) {
        i18n::syncFromConfig();
        if ((keysDown & KEY_PLUS) && g_playlist_undo.active && g_playlist_undo.ttl > 0) {
            play_ctx::savedInsert(g_playlist_undo.idx, g_playlist_undo.path);
            if (g_playlist_undo.playlist_ctx) {
                const u32 sz = play_ctx::savedPlaylistSize();
                if (sz > 0) {
                    const u32 pick = std::min(g_playlist_undo.idx, sz - 1u);
                    play_ctx::switchToPlaylist(pick);
                }
            }
            g_playlist_undo = {};
            if (tsl::notification)
                tsl::notification->showNow(i18n::t(i18n::Str::TrackRestoredToast), 24,
                                             i18n::t(i18n::Str::Playlist), 2200, false);
            triggerNavigationFeedback();
        } else {
            const u32 current = play_ctx::activePlaylistIndex();
            const u32 max     = play_ctx::maxPlaylistCount();
            u32 next = current;

            if (keysDown & KEY_RIGHT) {
                next = (current + 1) % max;  // Next playlist
            } else if (keysDown & KEY_LEFT) {
                next = (current > 0) ? (current - 1) : (max - 1);  // Previous playlist
            } else {
                next = (current + 1) % max;
            }

            play_ctx::switchPlaylistSlot(next, true);
            if (tsl::notification)
                tsl::notification->showNow(play_ctx::activePlaylistLabel().c_str(), 24,
                                           i18n::t(i18n::Str::Playlist), 2200, false);
            triggerNavigationFeedback();
        }
        tsl::swapTo<PlaylistGui>(m_on_count_changed);
        return true;
    }

    return SysTuneGui::handleInput(keysDown, keysHeld, touchPos,
                                   joyStickPosLeft, joyStickPosRight);
}
