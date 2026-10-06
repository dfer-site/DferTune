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

    /* The library's own footer/clock strings come from the Ultrahand language
     * files, which are normally absent on an SD card. Without this the footer
     * shows "Back"/"OK" and the clock shows "Tue" regardless of our language. */
    i18n::syncFromConfig();
    ult::BACK = i18n::t(i18n::Str::Back);
    ult::OK   = i18n::t(i18n::Str::Ok);
    if (std::strcmp(lang, "en") != 0) {
        ult::SUN = "周日";
        ult::MON = "周一";
        ult::TUE = "周二";
        ult::WED = "周三";
        ult::THU = "周四";
        ult::FRI = "周五";
        ult::SAT = "周六";
    }

    /* Language names are endonyms. An old lang file on the SD card may still
     * map "简体中文" to "Simplified Chinese"; pin them so the language row
     * always shows its own name. */
    ult::translationCache["简体中文"] = "简体中文";
    ult::translationCache["English"] = "English";

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
