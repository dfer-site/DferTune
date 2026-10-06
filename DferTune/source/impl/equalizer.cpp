#include "equalizer.hpp"

#include <algorithm>
#include <cmath>

namespace tune::impl {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr std::uint32_t kTransitionFrames = FiveBandEqualizer::SampleRate / 50; // 20 ms

constexpr std::array<float, EqualizerSettings::BandCount> kBandFrequencies = {
    100.0f, 300.0f, 1'000.0f, 3'000.0f, 10'000.0f,
};

FiveBandEqualizer::Coefficients Normalize(float b0, float b1, float b2,
                                          float a0, float a1, float a2) {
    const float inverse_a0 = 1.0f / a0;
    return {
        b0 * inverse_a0,
        b1 * inverse_a0,
        b2 * inverse_a0,
        a1 * inverse_a0,
        a2 * inverse_a0,
    };
}

void AddScaled(FiveBandEqualizer::Coefficients& value,
               const FiveBandEqualizer::Coefficients& step) {
    value.b0 += step.b0;
    value.b1 += step.b1;
    value.b2 += step.b2;
    value.a1 += step.a1;
    value.a2 += step.a2;
}

FiveBandEqualizer::Coefficients Difference(
    const FiveBandEqualizer::Coefficients& target,
    const FiveBandEqualizer::Coefficients& current,
    float divisor) {
    return {
        (target.b0 - current.b0) / divisor,
        (target.b1 - current.b1) / divisor,
        (target.b2 - current.b2) / divisor,
        (target.a1 - current.a1) / divisor,
        (target.a2 - current.a2) / divisor,
    };
}

} // namespace

FiveBandEqualizer::FiveBandEqualizer() {
    m_requested.store(Pack({}), std::memory_order_relaxed);
    m_applied = m_requested.load(std::memory_order_relaxed);
}

std::uint64_t FiveBandEqualizer::Pack(const EqualizerSettings& raw) {
    std::uint64_t packed = raw.enabled ? 1ULL : 0ULL;
    for (std::size_t i = 0; i < EqualizerSettings::BandCount; ++i) {
        const int gain = std::clamp(static_cast<int>(raw.gains_db[i]),
                                    static_cast<int>(EqualizerSettings::MinGainDb),
                                    static_cast<int>(EqualizerSettings::MaxGainDb));
        packed |= static_cast<std::uint64_t>(gain - EqualizerSettings::MinGainDb)
                  << (1 + i * 5);
    }
    return packed;
}

EqualizerSettings FiveBandEqualizer::Unpack(std::uint64_t packed) {
    EqualizerSettings settings{};
    settings.enabled = (packed & 1ULL) != 0;
    for (std::size_t i = 0; i < EqualizerSettings::BandCount; ++i) {
        const int encoded = static_cast<int>((packed >> (1 + i * 5)) & 0x1FULL);
        settings.gains_db[i] = static_cast<std::int8_t>(std::clamp(
            encoded + static_cast<int>(EqualizerSettings::MinGainDb),
            static_cast<int>(EqualizerSettings::MinGainDb),
            static_cast<int>(EqualizerSettings::MaxGainDb)));
    }
    return settings;
}

void FiveBandEqualizer::SetSettings(const EqualizerSettings& settings) {
    m_requested.store(Pack(settings), std::memory_order_release);
}

EqualizerSettings FiveBandEqualizer::GetSettings() const {
    return Unpack(m_requested.load(std::memory_order_acquire));
}

FiveBandEqualizer::Coefficients FiveBandEqualizer::MakePeak(
    float frequency_hz, float q, float gain_db) {
    const float amplitude = std::pow(10.0f, gain_db / 40.0f);
    const float omega = 2.0f * kPi * frequency_hz / static_cast<float>(SampleRate);
    const float cosine = std::cos(omega);
    const float alpha = std::sin(omega) / (2.0f * q);

    return Normalize(1.0f + alpha * amplitude,
                     -2.0f * cosine,
                     1.0f - alpha * amplitude,
                     1.0f + alpha / amplitude,
                     -2.0f * cosine,
                     1.0f - alpha / amplitude);
}

FiveBandEqualizer::Coefficients FiveBandEqualizer::MakeLowShelf(
    float frequency_hz, float gain_db) {
    const float amplitude = std::pow(10.0f, gain_db / 40.0f);
    const float omega = 2.0f * kPi * frequency_hz / static_cast<float>(SampleRate);
    const float cosine = std::cos(omega);
    const float alpha = std::sin(omega) * 0.70710678118f;
    const float beta = 2.0f * std::sqrt(amplitude) * alpha;
    const float ap1 = amplitude + 1.0f;
    const float am1 = amplitude - 1.0f;

    return Normalize(amplitude * (ap1 - am1 * cosine + beta),
                     2.0f * amplitude * (am1 - ap1 * cosine),
                     amplitude * (ap1 - am1 * cosine - beta),
                     ap1 + am1 * cosine + beta,
                     -2.0f * (am1 + ap1 * cosine),
                     ap1 + am1 * cosine - beta);
}

FiveBandEqualizer::Coefficients FiveBandEqualizer::MakeHighShelf(
    float frequency_hz, float gain_db) {
    const float amplitude = std::pow(10.0f, gain_db / 40.0f);
    const float omega = 2.0f * kPi * frequency_hz / static_cast<float>(SampleRate);
    const float cosine = std::cos(omega);
    const float alpha = std::sin(omega) * 0.70710678118f;
    const float beta = 2.0f * std::sqrt(amplitude) * alpha;
    const float ap1 = amplitude + 1.0f;
    const float am1 = amplitude - 1.0f;

    return Normalize(amplitude * (ap1 + am1 * cosine + beta),
                     -2.0f * amplitude * (am1 + ap1 * cosine),
                     amplitude * (ap1 + am1 * cosine - beta),
                     ap1 - am1 * cosine + beta,
                     2.0f * (am1 - ap1 * cosine),
                     ap1 - am1 * cosine - beta);
}

float FiveBandEqualizer::RunBiquad(float input, const Coefficients& coefficients,
                                    FilterState& state) {
    const float output = coefficients.b0 * input + state.z1;
    state.z1 = coefficients.b1 * input - coefficients.a1 * output + state.z2;
    state.z2 = coefficients.b2 * input - coefficients.a2 * output;
    return output;
}

void FiveBandEqualizer::BeginTransition(std::uint64_t packed) {
    const EqualizerSettings settings = Unpack(packed);
    const float transition = static_cast<float>(kTransitionFrames);
    int maximum_boost_db = 0;

    for (std::size_t i = 0; i < EqualizerSettings::BandCount; ++i) {
        const float gain = settings.enabled ? static_cast<float>(settings.gains_db[i]) : 0.0f;
        maximum_boost_db = std::max(maximum_boost_db, static_cast<int>(gain));

        if (i == 0)
            m_targets[i] = MakeLowShelf(kBandFrequencies[i], gain);
        else if (i + 1 == EqualizerSettings::BandCount)
            m_targets[i] = MakeHighShelf(kBandFrequencies[i], gain);
        else
            m_targets[i] = MakePeak(kBandFrequencies[i], 1.0f, gain);

        m_steps[i] = Difference(m_targets[i], m_coefficients[i], transition);
    }

    // Conventional auto-preamp: a positive boost reserves the same amount of
    // headroom before the filters, substantially reducing PCM clipping.
    m_target_preamp = std::pow(10.0f, -static_cast<float>(maximum_boost_db) / 20.0f);
    m_preamp_step = (m_target_preamp - m_preamp) / transition;
    m_transition_frames = kTransitionFrames;
    m_target_bypass = !settings.enabled;
    m_bypass = false;
    m_applied = packed;
}

void FiveBandEqualizer::AdvanceTransition() {
    if (m_transition_frames == 0)
        return;

    for (std::size_t i = 0; i < EqualizerSettings::BandCount; ++i)
        AddScaled(m_coefficients[i], m_steps[i]);
    m_preamp += m_preamp_step;

    if (--m_transition_frames == 0) {
        m_coefficients = m_targets;
        m_preamp = m_target_preamp;
        if (m_target_bypass) {
            Reset();
            m_bypass = true;
        }
    }
}

void FiveBandEqualizer::Reset() {
    for (auto& channel : m_states)
        for (auto& state : channel)
            state = {};
}

void FiveBandEqualizer::Process(std::int16_t* samples, std::size_t frame_count) {
    if (!samples || frame_count == 0)
        return;

    const std::uint64_t requested = m_requested.load(std::memory_order_acquire);
    if (requested != m_applied)
        BeginTransition(requested);

    if (m_bypass && m_transition_frames == 0)
        return;

    constexpr float kInputScale = 1.0f / 32768.0f;
    constexpr float kOutputScale = 32768.0f;
    constexpr float kMaximumSample = 32767.0f / 32768.0f;

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        AdvanceTransition();

        for (std::size_t channel = 0; channel < 2; ++channel) {
            float value = static_cast<float>(samples[frame * 2 + channel]) * kInputScale;
            value *= m_preamp;

            for (std::size_t band = 0; band < EqualizerSettings::BandCount; ++band)
                value = RunBiquad(value, m_coefficients[band], m_states[channel][band]);

            value = std::clamp(value, -1.0f, kMaximumSample);
            samples[frame * 2 + channel] = static_cast<std::int16_t>(
                std::lround(value * kOutputScale));
        }
    }
}

} // namespace tune::impl
