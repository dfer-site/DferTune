#pragma once

#include <tesla.hpp>

#include "elm_textwrap.hpp"

#include <string>

/**
 * @brief Category header that wraps onto extra lines instead of scrolling.
 *
 * Tesla's own headers marquee-scroll text that is too wide, which leaves part
 * of a long hint hidden most of the time. This one shows the whole text: a
 * single line looks and measures exactly like CompactCategoryHeader (33 px);
 * longer text wraps and the row grows to fit.
 */
class WrappedHeader final : public tsl::elm::Element {
public:
    explicit WrappedHeader(std::string text)
        : m_text(std::move(text))
    {
        m_isItem = false;

        // Same inset as CategoryHeader: bar at x+7, text at x+21.
        m_textWidth = static_cast<s32>(tsl::cfg::FramebufferWidth) - 85 - 21 - 10;

        auto &renderer = tsl::gfx::Renderer::get();
        const auto oneLine = renderer.getTextDimensions(m_text, false, kFont);
        m_wrapped = oneLine.first > m_textWidth;
        m_extra = 0;
        if (m_wrapped) {
            // drawString() never wraps by itself, so break the text into lines here.
            m_text = textwrap::wrapForRenderer(renderer, m_text, kFont, m_textWidth);
            const auto block = renderer.getTextDimensions(m_text, false, kFont);
            m_extra = block.second > oneLine.second ? block.second - oneLine.second : 0;
        }
        m_height = 33 + m_extra;
    }

    s32 preferredHeight() const { return m_height; }

    void layout(u16, u16, u16, u16) override {
        setBoundaries(getX(), getY(), getWidth(), m_height);
    }

    void draw(tsl::gfx::Renderer *renderer) override {
        const s32 top   = getY();
        const s32 textX = getX() + 2 + 5 + 14;
        const s32 textY = top + 17;

        renderer->drawRect(getX() + 2 + 5, top, 4, 22 + m_extra,
                           aWithOpacity(tsl::headerSeparatorColor));

        if (m_wrapped) {
            renderer->drawString(m_text, false, textX, textY, kFont, tsl::headerTextColor);
        } else {
            renderer->drawStringWithColoredSections(
                m_text, false, tsl::s_dividerSpecialChars, textX, textY, kFont,
                tsl::headerTextColor, tsl::textSeparatorColor);
        }
    }

private:
    static constexpr u32 kFont = 16;

    std::string m_text;
    s32  m_textWidth = 0;
    s32  m_extra     = 0;
    s32  m_height    = 33;
    bool m_wrapped   = false;
};

/** Add a WrappedHeader to 'list' with the height it needs. */
inline void addWrappedHeader(tsl::elm::List *list, const std::string &text) {
    auto *header = new WrappedHeader(text);
    list->addItem(header, header->preferredHeight());
}
