#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace tune::impl {

struct EqualizerSettings {
    static constexpr std::size_t BandCount = 5;
    static constexpr std::int8_t MinGainDb = -12;
    static constexpr std::int8_t MaxGainDb = 12;

    bool enabled{false};
    std::array<std::int8_t, BandCount> gains_db{};
};

/**
 * Five-band stereo equalizer for DferTune's 48 kHz interleaved PCM stream.
 *
 * The implementation is deliberately independent from hardware-codec EQ
 * projects. It uses RBJ-style biquads in transposed direct-form II, smooths
 * coefficient changes over 20 ms, and publishes UI changes to the audio
 * thread through one lock-free atomic word.
 */
class FiveBandEqualizer final {
public:
    static constexpr std::uint32_t SampleRate = 48'000;

    FiveBandEqualizer();

    void SetSettings(const EqualizerSettings& settings);
    EqualizerSettings GetSettings() const;

    void Reset();
    void Process(std::int16_t* interleaved_stereo, std::size_t frame_count);

public:
    struct Coefficients {
        float b0{1.0f};
        float b1{0.0f};
        float b2{0.0f};
        float a1{0.0f};
        float a2{0.0f};
    };

    struct FilterState {
        float z1{0.0f};
        float z2{0.0f};
    };

private:
    static std::uint64_t Pack(const EqualizerSettings& settings);
    static EqualizerSettings Unpack(std::uint64_t packed);
    static Coefficients MakeLowShelf(float frequency_hz, float gain_db);
    static Coefficients MakePeak(float frequency_hz, float q, float gain_db);
    static Coefficients MakeHighShelf(float frequency_hz, float gain_db);
    static float RunBiquad(float input, const Coefficients& coefficients,
                           FilterState& state);

    void BeginTransition(std::uint64_t packed);
    void AdvanceTransition();

    std::atomic<std::uint64_t> m_requested{0};
    std::uint64_t m_applied{0};
    std::array<Coefficients, EqualizerSettings::BandCount> m_coefficients{};
    std::array<Coefficients, EqualizerSettings::BandCount> m_targets{};
    std::array<Coefficients, EqualizerSettings::BandCount> m_steps{};
    std::array<std::array<FilterState, EqualizerSettings::BandCount>, 2> m_states{};
    float m_preamp{1.0f};
    float m_target_preamp{1.0f};
    float m_preamp_step{0.0f};
    std::uint32_t m_transition_frames{0};
    bool m_target_bypass{true};
    bool m_bypass{true};
};

} // namespace tune::impl
