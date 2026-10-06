#pragma once

#include <tesla.hpp>
#include <algorithm>

/**
 * @brief Floating tooltip bubble for the focused row.
 *
 * Whoever knows what is focused (SysTuneOverlayFrame for list rows, StatusBar
 * for its buttons) calls tip::request() while drawing. The frame then calls
 * tip::draw() once, after the content, so the bubble floats above everything
 * and is gone the very next frame once nothing requests it any more.
 *
 * All of this runs on Tesla's single UI thread, so no locking is needed.
 */
namespace tip {

struct Request {
    const void *owner  = nullptr;  ///< identity of the focused thing (fade restarts when it changes)
    const char *text   = nullptr;  ///< must stay valid until tip::draw() returns
    s32         top    = 0;        ///< top of the focused row, in screen pixels
    s32         bottom = 0;        ///< bottom of the focused row, in screen pixels
};

inline Request g_request;

inline void request(const void *owner, const char *text, s32 top, s32 bottom) {
    g_request = Request{owner, text, top, bottom};
}

inline bool pending() {
    return g_request.text != nullptr;
}

/** Paint the pending bubble (if any) and clear the request. */
inline void draw(tsl::gfx::Renderer *renderer) {
    static const void *s_owner = nullptr;
    static u64         s_since = 0;

    const Request r = g_request;
    g_request = Request{};

    if (r.text == nullptr || r.text[0] == '\0') {
        s_owner = nullptr;
        return;
    }

    const u64 now = ult::nowNs();
    if (r.owner != s_owner) {
        s_owner = r.owner;
        s_since = now;
    }

    // Short fade-in so the bubble does not flicker while the cursor is
    // simply passing over rows.
    constexpr float kFadeNs = 150e6f;
    const float progress = std::min(1.0f, static_cast<float>(now - s_since) / kFadeNs);
    const u8 alpha = static_cast<u8>(progress * 15.0f);
    if (alpha == 0)
        return;

    constexpr u32 kFont     = 18;
    constexpr s32 kPadX     = 12;
    constexpr s32 kPadY     = 8;
    constexpr s32 kGap      = 8;
    constexpr s32 kMaxTextW = 340;

    const s32 screenW = static_cast<s32>(tsl::cfg::FramebufferWidth);
    const s32 screenH = static_cast<s32>(tsl::cfg::FramebufferHeight);
    const s32 minY    = 100;            // below the title / clock header
    const s32 maxY    = screenH - 80;   // above the footer buttons

    const auto dim = renderer->getTextDimensions(r.text, false, kFont, kMaxTextW);
    const s32 w = dim.first  + 2 * kPadX;
    const s32 h = dim.second + 2 * kPadY;

    s32 x = (screenW - w) / 2;
    x = std::max<s32>(8, std::min<s32>(x, screenW - w - 8));

    // Prefer below the row; flip above when it would run into the footer.
    s32 y = r.bottom + kGap;
    if (y + h > maxY) {
        const s32 above = r.top - h - kGap;
        y = (above >= minY) ? above : std::max<s32>(minY, maxY - h);
    }

    const tsl::Color border(0xA, 0xA, 0xA, alpha);
    const tsl::Color fill  (0x1, 0x1, 0x2, alpha);
    const tsl::Color text  (0xF, 0xF, 0xF, alpha);

    renderer->drawRoundedRect(x - 1, y - 1, w + 2, h + 2, 9, border);
    renderer->drawRoundedRect(x, y, w, h, 8, fill);
    renderer->drawString(r.text, false, x + kPadX, y + kPadY + static_cast<s32>(kFont) - 3,
                         kFont, text, kMaxTextW);
}

} // namespace tip
