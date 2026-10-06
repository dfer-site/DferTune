#include "codec_equalizer.hpp"

#include "../tune_result.hpp"

#include <algorithm>

namespace tune::impl::codec_equalizer {
namespace {

// ALC5639 register map (16-bit big-endian register values).
constexpr u8 RegPrivateIndex = 0x6A;
constexpr u8 RegPrivateData  = 0x6C;
constexpr u8 RegEqControl1   = 0xB0;
constexpr u8 RegEqControl2   = 0xB1;
constexpr u8 RegVendorId     = 0xFE;

constexpr u16 EqLatch       = 1u << 14;
constexpr u16 EqAdcSource   = 1u << 15;
constexpr u16 EqLowPassType = 1u << 7;
constexpr u16 EqStageMask   = 0x001F;

constexpr std::array<u8, BandCount> CoefficientSlots = {
    0xA1, 0xA4, 0xA7, 0xAA, 0xAD,
};

// ALC5639 does not use one common neutral H0 for all five stages. The low-pass
// stage and BPF2..4 reset to 0x01F4, while BPF1 resets to 0x1D2C. Treating
// 0x1D2C as neutral everywhere made four bands about 15x too strong, which
// sounded like a broadband volume/bass boost instead of an equalizer.
constexpr std::array<u16, BandCount> NeutralGainCoefficient = {
    0x01F4, 0x1D2C, 0x01F4, 0x01F4, 0x01F4,
};

// -12..+12 dB multipliers expressed relative to the exact 0 dB BPF1 value.
// Scaling the per-stage neutral value preserves the codec's factory crossover
// curve and changes only the requested frequency band's contribution.
constexpr std::array<u16, 25> ReferenceGainCoefficient = {
     1876,  2105,  2362,  2650,  2973,  3336,  3743,  4200,  4712,
     5287,  5932,  6656,  7468,  8379,  9402, 10549, 11836, 13280,
    14901, 16719, 18759, 21048, 23616, 26497, 29731,
};
constexpr u32 ReferenceNeutral = 0x1D2C;

bool g_initialized = false;
bool g_active = false;
std::array<s8, BandCount> g_applied_gains{};

class CodecBus final {
public:
    Result Open() {
        const Result rc = i2cOpenSession(&m_session, I2cDevice_Alc5639);
        m_open = R_SUCCEEDED(rc);
        return rc;
    }

    ~CodecBus() {
        if (m_open)
            i2csessionClose(&m_session);
    }

    Result Read(u8 reg, u16& value) {
        R_UNLESS(m_open, tune::Generic);

        const u8 request = reg;
        R_TRY(i2csessionSendAuto(
            &m_session, &request, sizeof(request), I2cTransactionOption_All));

        u8 response[2]{};
        R_TRY(i2csessionReceiveAuto(
            &m_session, response, sizeof(response), I2cTransactionOption_All));
        value = static_cast<u16>((static_cast<u16>(response[0]) << 8) | response[1]);
        return 0;
    }

    Result Write(u8 reg, u16 value) {
        R_UNLESS(m_open, tune::Generic);

        const u8 request[3] = {
            reg,
            static_cast<u8>(value >> 8),
            static_cast<u8>(value),
        };
        return i2csessionSendAuto(
            &m_session, request, sizeof(request), I2cTransactionOption_All);
    }

    Result WritePrivate(u8 index, u16 value) {
        R_TRY(Write(RegPrivateIndex, index));
        return Write(RegPrivateData, value);
    }

private:
    I2cSession m_session{};
    bool m_open{false};
};

u16 CoefficientFor(std::size_t band, s8 gain_db) {
    const int gain = std::clamp(static_cast<int>(gain_db), -12, 12);
    const u32 neutral = NeutralGainCoefficient[band];
    const u32 reference = ReferenceGainCoefficient[static_cast<std::size_t>(gain + 12)];
    return static_cast<u16>((neutral * reference + ReferenceNeutral / 2) / ReferenceNeutral);
}

} // namespace

Result Initialize() {
    if (g_initialized)
        return 0;
    const Result rc = i2cInitialize();
    if (R_SUCCEEDED(rc))
        g_initialized = true;
    return rc;
}

void Exit() {
    if (!g_initialized)
        return;
    i2cExit();
    g_initialized = false;
    g_active = false;
}

Result Apply(bool enabled, const std::array<s8, BandCount>& gains_db) {
    R_TRY(Initialize());

    CodecBus bus;
    R_TRY(bus.Open());

    // A successful vendor-ID read also distinguishes handheld analogue output
    // from configurations where the codec is unavailable (notably HDMI dock).
    u16 vendor_id = 0;
    R_TRY(bus.Read(RegVendorId, vendor_id));
    (void)vendor_id;

    u16 control2 = 0;
    if (!enabled) {
        R_TRY(bus.Read(RegEqControl2, control2));
        const Result rc = bus.Write(RegEqControl2, static_cast<u16>(control2 & ~EqStageMask));
        if (R_SUCCEEDED(rc))
            g_active = false;
        return rc;
    }

    bool coefficients_changed = !g_active;
    for (std::size_t band = 0; band < BandCount; ++band) {
        if (g_active && gains_db[band] == g_applied_gains[band])
            continue;
        R_TRY(bus.WritePrivate(CoefficientSlots[band], CoefficientFor(band, gains_db[band])));
        coefficients_changed = true;
    }

    if (!g_active) {
        R_TRY(bus.Read(RegEqControl2, control2));
        control2 = static_cast<u16>((control2 | EqStageMask) & ~EqLowPassType);
        R_TRY(bus.Write(RegEqControl2, control2));
    }

    if (!coefficients_changed)
        return 0;

    u16 control1 = 0;
    R_TRY(bus.Read(RegEqControl1, control1));
    control1 = static_cast<u16>(control1 & ~EqAdcSource); // DAC/output path
    R_TRY(bus.Write(RegEqControl1, static_cast<u16>(control1 | EqLatch)));
    R_TRY(bus.Write(RegEqControl1, control1));
    g_applied_gains = gains_db;
    g_active = true;
    return 0;
}

} // namespace tune::impl::codec_equalizer
