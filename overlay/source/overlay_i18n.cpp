#include "overlay_i18n.hpp"

#include "config/config.hpp"
#include "strings.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <array>

#include <tesla.hpp>

#ifndef UI_OVERRIDE_PATH
#define UI_OVERRIDE_PATH "/config/ryazhahand/"
#endif

#ifndef VERSION
#define VERSION "dev"
#endif

void reloadDferTuneTranslations() {
    char lang[16]{};
    config::get_language(lang, sizeof(lang));
    if (lang[0] == '\0')
        std::strncpy(lang, "zh-cn", sizeof(lang) - 1);

    ult::clearTranslationCache();

    const std::array<std::string, 2> langRoots = {
        std::string("/config/DferTune/"),
        std::string(UI_OVERRIDE_PATH),
    };

    for (std::string base : langRoots) {
        ult::preprocessPath(base);
        if (!base.empty() && base.back() != '/')
            base.push_back('/');

        const std::string pkgLang = base + "lang/" + lang + ".json";
        if (ult::isFile(pkgLang)) {
            ult::loadTranslationsFromJSON(pkgLang);
            break;
        }
    }

    const std::string ultraLang = ult::LANG_PATH + lang + ".json";
    if (ult::isFile(ultraLang))
        ult::parseLanguage(ultraLang);
    else
        ult::reinitializeLangVars();

    ult::languageWasChanged.store(true, std::memory_order_release);
}

void maybeShowOverlayWhatsNew() {
    constexpr const char *kPath = "/config/DferTune/overlay_seen_version.txt";

    char buf[128]{};
    if (FILE *f = fopen(kPath, "r")) {
        if (fgets(buf, static_cast<int>(sizeof(buf)), f)) {
            for (char *p = buf; *p; ++p) {
                if (*p == '\n' || *p == '\r') {
                    *p = '\0';
                    break;
                }
            }
        }
        fclose(f);
    }

    if (std::strcmp(buf, VERSION) == 0)
        return;

    i18n::syncFromConfig();
    if (tsl::notification)
        tsl::notification->showNow(i18n::t(i18n::Str::WhatsNewBody), 22, VERSION, 4200, false);
    triggerNavigationFeedback();

    if (FILE *out = fopen(kPath, "w")) {
        std::fprintf(out, "%s\n", VERSION);
        fclose(out);
    }
}
