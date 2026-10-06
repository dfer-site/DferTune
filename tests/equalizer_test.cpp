#include "../DferTune/source/impl/equalizer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

std::vector<std::int16_t> MakeSine(float frequency_hz, float amplitude,
                                   std::size_t frames) {
    std::vector<std::int16_t> samples(frames * 2);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float phase = 2.0f * kPi * frequency_hz
                          * static_cast<float>(frame)
                          / static_cast<float>(tune::impl::FiveBandEqualizer::SampleRate);
        const auto value = static_cast<std::int16_t>(
            std::lround(std::sin(phase) * amplitude * 32767.0f));
        samples[frame * 2] = value;
        samples[frame * 2 + 1] = value;
    }
    return samples;
}

double Rms(const std::vector<std::int16_t>& samples, std::size_t first_frame) {
    double sum = 0.0;
    std::size_t count = 0;
    for (std::size_t frame = first_frame; frame < samples.size() / 2; ++frame) {
        const double sample = static_cast<double>(samples[frame * 2]) / 32768.0;
        sum += sample * sample;
        ++count;
    }
    return std::sqrt(sum / static_cast<double>(count));
}

double FilteredRms(float frequency_hz) {
    constexpr std::size_t frames = tune::impl::FiveBandEqualizer::SampleRate;
    auto samples = MakeSine(frequency_hz, 0.2f, frames);

    tune::impl::FiveBandEqualizer equalizer;
    tune::impl::EqualizerSettings settings{};
    settings.enabled = true;
    settings.gains_db[0] = 6;
    equalizer.SetSettings(settings);
    equalizer.Process(samples.data(), frames);
    return Rms(samples, frames / 2);
}

} // namespace

int main() {
    constexpr std::size_t frames = 4'096;

    auto bypass_input = MakeSine(1'000.0f, 0.7f, frames);
    auto bypass_output = bypass_input;
    tune::impl::FiveBandEqualizer bypass;
    bypass.Process(bypass_output.data(), frames);
    if (bypass_output != bypass_input) {
        std::fputs("disabled bypass changed PCM\n", stderr);
        return 1;
    }

    tune::impl::FiveBandEqualizer clamped;
    tune::impl::EqualizerSettings invalid{};
    invalid.enabled = true;
    invalid.gains_db = {127, -128, 30, -30, 100};
    clamped.SetSettings(invalid);
    const auto sanitized = clamped.GetSettings();
    for (const auto gain : sanitized.gains_db) {
        if (gain < tune::impl::EqualizerSettings::MinGainDb
            || gain > tune::impl::EqualizerSettings::MaxGainDb) {
            std::fputs("gain clamp failed\n", stderr);
            return 2;
        }
    }

    const double bass_rms = FilteredRms(100.0f);
    const double mid_rms = FilteredRms(1'000.0f);
    if (!(bass_rms > mid_rms * 1.7)) {
        std::fprintf(stderr, "bass response too weak: bass=%f mid=%f\n",
                     bass_rms, mid_rms);
        return 3;
    }

    auto loud = MakeSine(100.0f, 0.99f, frames);
    tune::impl::FiveBandEqualizer limiter_check;
    tune::impl::EqualizerSettings boosted{};
    boosted.enabled = true;
    boosted.gains_db = {12, 12, 12, 12, 12};
    limiter_check.SetSettings(boosted);
    limiter_check.Process(loud.data(), frames);
    const auto peak = *std::max_element(loud.begin(), loud.end());
    const auto trough = *std::min_element(loud.begin(), loud.end());
    if (peak > 32767 || trough < -32768) {
        std::fputs("s16 range check failed\n", stderr);
        return 4;
    }

    std::printf("equalizer tests passed (100 Hz RMS %.6f, 1 kHz RMS %.6f)\n",
                bass_rms, mid_rms);
    return 0;
}
