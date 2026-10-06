#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Word wrapping that only needs a "how wide is this character" callback.
 *
 * Tesla's drawString() does not wrap: its maxWidth argument just stops drawing
 * once the text is too wide, and only a '\n' in the string starts a new line.
 * So long text has to be broken into lines first. wrap() inserts those '\n'.
 *
 * Rules:
 *  - an explicit '\n' always starts a new line;
 *  - English words are kept whole and break at spaces; a single word longer
 *    than a line is split;
 *  - CJK text may break between any two characters, except that closing
 *    punctuation is never pushed to the start of a line (it may overhang the
 *    right edge by one character instead).
 *
 * WidthOf is any callable: s32/int width = widthOf(const std::string &utf8Char).
 * Width is the sum of per-character advances, the same way drawString advances.
 */
namespace textwrap {

namespace detail {

inline std::size_t charLength(unsigned char lead, std::size_t remaining) {
    std::size_t len = lead < 0x80 ? 1 : lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
    return len > remaining ? remaining : len;
}

// Closing punctuation that must not start a line.
inline bool isNoLineStart(const std::string &ch) {
    static const char *const kSet[] = {
        "，", "。", "、", "；", "：", "！", "？", "）", "」", "』", "》", "】", "”", "’", "…", "·", "—",
    };
    for (const char *s : kSet)
        if (ch == s) return true;
    return false;
}

} // namespace detail

template <typename WidthOf>
std::string wrap(const std::string &text, int maxWidth, WidthOf widthOf) {
    if (maxWidth <= 0 || text.empty())
        return text;

    std::string out;
    out.reserve(text.size() + 16);
    int lineW = 0;

    std::string word;          // pending ASCII word, including the spaces that follow it
    int wordW = 0;
    int trailingSpaces = 0;    // width of the spaces at the end of 'word'
    const int spaceW = widthOf(std::string(" "));

    auto newLine = [&]() {
        while (!out.empty() && out.back() == ' ')
            out.pop_back();
        out += '\n';
        lineW = 0;
    };

    auto placeWord = [&]() {
        if (word.empty())
            return;
        const int core = wordW - trailingSpaces;
        std::size_t coreLen = word.size();
        while (coreLen > 0 && word[coreLen - 1] == ' ')
            --coreLen;

        if (lineW > 0 && lineW + core > maxWidth)
            newLine();

        if (lineW == 0 && core > maxWidth) {
            // One word wider than a whole line: split it character by character.
            for (std::size_t i = 0; i < coreLen; ++i) {
                const std::string ch(1, word[i]);
                const int w = widthOf(ch);
                if (lineW > 0 && lineW + w > maxWidth)
                    newLine();
                out += ch;
                lineW += w;
            }
            out.append(word, coreLen, std::string::npos);
            lineW += trailingSpaces;
        } else {
            out += word;
            lineW += wordW;
        }
        word.clear();
        wordW = 0;
        trailingSpaces = 0;
    };

    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t len = detail::charLength(static_cast<unsigned char>(text[i]), text.size() - i);
        const std::string ch = text.substr(i, len);
        i += len;

        if (ch == "\n") {
            placeWord();
            while (!out.empty() && out.back() == ' ')
                out.pop_back();
            out += '\n';
            lineW = 0;
        } else if (ch == " ") {
            word += ' ';
            wordW += spaceW;
            trailingSpaces += spaceW;
            placeWord();
        } else if (len == 1) {
            // A character of an ASCII word.
            if (trailingSpaces > 0)
                placeWord();
            word += ch;
            wordW += widthOf(ch);
        } else {
            // Any other character (CJK and so on) can break on its own.
            placeWord();
            const int w = widthOf(ch);
            if (lineW > 0 && lineW + w > maxWidth && !detail::isNoLineStart(ch))
                newLine();
            out += ch;
            lineW += w;
        }
    }
    placeWord();
    return out;
}

} // namespace textwrap
