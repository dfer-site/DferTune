#pragma once

#include <tesla.hpp>

#include "textwrap_core.hpp"

#include <string>
#include <unordered_map>

namespace textwrap {

/**
 * Break 'text' into lines no wider than maxWidth pixels at the given font size,
 * using the renderer's own glyph advances so the result matches what
 * drawString() draws. Draw the returned string without a maxWidth argument:
 * drawString() starts a new line at every '\n' but never wraps by itself.
 */
inline std::string wrapForRenderer(tsl::gfx::Renderer &renderer, const std::string &text,
                                   u32 fontSize, s32 maxWidth) {
    static std::unordered_map<std::string, int> s_widths;   // "font:char" -> advance in pixels
    const auto widthOf = [&](const std::string &ch) -> int {
        std::string key = std::to_string(fontSize);
        key += ':';
        key += ch;
        const auto it = s_widths.find(key);
        if (it != s_widths.end())
            return it->second;
        const int w = renderer.getTextDimensions(ch, false, fontSize).first;
        s_widths.emplace(std::move(key), w);
        return w;
    };
    return wrap(text, maxWidth, widthOf);
}

} // namespace textwrap
