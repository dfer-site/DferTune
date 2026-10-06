#pragma once

/** Load Tesla / ryazhahand JSON translations for the active config language. */
void reloadDferTuneTranslations();

/** One-shot toast after an overlay build/version bump (persists last seen in /config). */
void maybeShowOverlayWhatsNew();
