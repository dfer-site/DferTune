#pragma once

#include <tesla.hpp>

#include "elm_textwrap.hpp"

#include <string>

/**
 * @brief A paragraph for the Help / About pages: optional heading plus a
 *        word-wrapped body.
 *
 * The block is focusable only so the list can scroll through long text with
 * UP/DOWN (Tesla scrolls by moving focus). Its height is measured once from
 * the text, so pass preferredHeight() to List::addItem().
 */
class TextBlock final : public tsl::elm::Element {
public:
    /** @param width     Full width of the list row (FramebufferWidth - 85).
     *  @param focusable Pass false for a short note that must not take the cursor
     *                   (it then cannot be scrolled to, so keep it brief). */
    TextBlock(std::string heading, std::string body, s32 width, bool focusable = true)
        : m_textWidth(width - 2 * kPadX)
    {
        m_isItem = focusable;

        // drawString() never wraps by itself, so break the text into lines here.
        auto &renderer = tsl::gfx::Renderer::get();
        m_heading = textwrap::wrapForRenderer(renderer, heading, kHeadFont, m_textWidth);
        m_body    = textwrap::wrapForRenderer(renderer, body, kBodyFont, m_textWidth);

        m_bodyTop = 10;
        if (!m_heading.empty()) {
            const s32 lines = renderer.getTextDimensions(m_heading, false, kHeadFont).second;
            const s32 one   = renderer.getTextDimensions("A", false, kHeadFont).second;
            m_bodyTop = 44 + (lines > one ? lines - one : 0);   // room for extra heading lines
        }
        const s32 bodyHeight = renderer.getTextDimensions(m_body, false, kBodyFont).second;
        m_height = m_bodyTop + bodyHeight + kPadBottom;
    }

    s32 preferredHeight() const { return m_height; }

    void layout(u16, u16, u16, u16) override {
        setBoundaries(getX(), getY(), getWidth(), m_height);
    }

    tsl::elm::Element* requestFocus(tsl::elm::Element*, tsl::FocusDirection) override {
        return this;
    }

    void drawSeparators(tsl::gfx::Renderer *renderer) override {
        renderer->drawRect(getX() + 7, getBottomBound() - 1, getWidth() - 14, 1,
                           aWithOpacity(tsl::separatorColor));
    }

    void drawHighlight(tsl::gfx::Renderer *renderer) override {
        // A soft background instead of Tesla's bordered cursor, which would
        // frame a whole paragraph.
        renderer->drawRoundedRect(getX() + 2, getY() + 2, getWidth() - 4, m_height - 4,
                                  9.0f, aWithOpacity(tsl::selectionBGColor));
    }

    void draw(tsl::gfx::Renderer *renderer) override {
        const s32 x = getX() + kPadX;
        const s32 y = getY();
        if (!m_heading.empty())
            renderer->drawString(m_heading, false, x, y + 30, kHeadFont, a(tsl::onTextColor));
        renderer->drawString(m_body, false, x, y + m_bodyTop + kBodyFont - 2, kBodyFont,
                             a(tsl::defaultTextColor));
    }

private:
    static constexpr s32 kBodyFont   = 20;
    static constexpr s32 kHeadFont   = 22;
    static constexpr s32 kPadX       = 14;
    static constexpr s32 kPadBottom  = 14;

    std::string m_heading;
    std::string m_body;
    s32         m_textWidth;
    s32         m_bodyTop = 10;
    s32         m_height  = 0;
};
