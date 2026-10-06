#pragma once

#include <array>
#include <switch.h>

namespace tune::impl::codec_equalizer {

constexpr std::size_t BandCount = 5;

Result Initialize();
void Exit();

/**
 * Applies the five-band curve to the ALC5639 DAC path.
 *
 * This is the system/game target: the codec sits after application mixing, so
 * it affects the analogue output as a whole. HDMI audio in docked mode does
 * not pass through this codec and returns an error instead of pretending the
 * setting was applied.
 */
Result Apply(bool enabled, const std::array<s8, BandCount>& gains_db);

} // namespace tune::impl::codec_equalizer
