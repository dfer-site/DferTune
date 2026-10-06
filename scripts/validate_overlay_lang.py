#!/usr/bin/env python3
"""Validate DferTune overlay language JSON files.

The overlay is translated by libryazhahand at runtime, so every language file
must contain both the symbolic keys used by older code and the literal strings
that are actually drawn by the current UI.
"""

from __future__ import annotations

import json
from pathlib import Path

LANG_DIR = Path("overlay/lang")
REQUIRED_LANGUAGES = {"en", "zh-cn"}
RUNTIME_ALIASES = {
    "Language",
    "Stop DferTune",
    "DferTune ♫",
    "Music Library",
    "Volume ⊘ \ue0e3 Toggle Mute",
    "No startup path set in config.",
    "Add To Playlist",
    "Set As Startup",
    "Playlist is empty!",
}


def load_languages() -> dict[str, dict[str, str]]:
    languages: dict[str, dict[str, str]] = {}
    for path in sorted(LANG_DIR.glob("*.json")):
        languages[path.stem] = json.loads(path.read_text(encoding="utf-8"))
    if not languages:
        raise SystemExit("overlay/lang/*.json not found")
    return languages


def main() -> None:
    languages = load_languages()

    missing_files = sorted(REQUIRED_LANGUAGES - set(languages))
    if missing_files:
        raise SystemExit(f"Missing language files: {missing_files}")

    base_keys = set(languages["zh-cn"])
    missing_aliases = sorted(RUNTIME_ALIASES - base_keys)
    if missing_aliases:
        raise SystemExit(f"zh-cn.json missing runtime aliases: {missing_aliases}")

    for name, data in languages.items():
        keys = set(data)
        missing = sorted(base_keys - keys)
        extra = sorted(keys - base_keys)
        if missing or extra:
            raise SystemExit(
                f"{name}.json key mismatch: missing={missing}, extra={extra}"
            )

        missing_runtime_aliases = sorted(RUNTIME_ALIASES - keys)
        if missing_runtime_aliases:
            raise SystemExit(
                f"{name}.json missing runtime aliases: {missing_runtime_aliases}"
            )

    print(f"Successfully validated {len(languages)} language files with {len(base_keys)} keys each.")


if __name__ == "__main__":
    main()
