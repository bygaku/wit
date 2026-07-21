/**
 * Created by intwi on 2026/07/21.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_BIQUAD_H
#define WIT_BIQUAD_H

#include <cmath>
#include <cstdint>

namespace wit {

/**
 * @struct BiquadCoeffs
 * @brief Normalized transposed-direct-form-II biquad coefficients.
 *
 * Computed on the main thread and shipped to the audio thread as a value.
 * The audio thread never evaluates trigonometry; it only multiplies and adds.
 */
struct BiquadCoeffs {
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

/**
 * @struct BiquadState
 * @brief Per-channel filter memory (transposed DF-II state variables).
 * @note Owned and touched by the audio thread only.
 */
struct BiquadState {
    float z1 = 0.0f;
    float z2 = 0.0f;

    void Reset() noexcept {
        z1 = 0.0f;
        z2 = 0.0f;
    }
};

namespace biquad {
inline constexpr float FIXED_Q = 0.70710678f; ///< Fixed Q. Butterworth response (maximally flat passband).

/**
 * @brief Compute low-pass coefficients from the RBJ Audio EQ Cookbook.
 * @param cutoff_hz   Corner frequency in Hz.
 * @param sample_rate Sampling rate in Hz.
 * @return Normalized BiquadCoeffs (a0 divided out).
 *
 * @note cutoff is clamped to a safe range below Nyquist to keep the
 *       filter stable regardless of what the caller passes.
 */
inline BiquadCoeffs MakeLowpass(float cutoff_hz, float sample_rate) noexcept {
    BiquadCoeffs c;
    if (sample_rate <= 0.0f) return c;

    const float nyquist_freq = sample_rate * 0.5f;
    if (cutoff_hz < 1.0f)					cutoff_hz = 1.0f;
    if (cutoff_hz > nyquist_freq - 1.0f)	cutoff_hz = nyquist_freq - 1.0f;

    const float omega = 2.0f * 3.14159265358979323846f * cutoff_hz / sample_rate;
    const float sn    = std::sin(omega);
    const float cs    = std::cos(omega);
    const float alpha = sn / (2.0f * FIXED_Q);

    const float b0 = (1.0f - cs) * 0.5f;
    const float b1 =  1.0f - cs;
    const float b2 = (1.0f - cs) * 0.5f;
    const float a0 =  1.0f + alpha;
    const float a1 = -2.0f * cs;
    const float a2 =  1.0f - alpha;

    const float inv_a0 = 1.0f / a0;
    c.b0 = b0 * inv_a0;
    c.b1 = b1 * inv_a0;
    c.b2 = b2 * inv_a0;
    c.a1 = a1 * inv_a0;
    c.a2 = a2 * inv_a0;
    return c;
}

/**
 * @brief Process one planar channel buffer in place with a dry/wet blend.
 * @param buffer Planar samples for a single channel.
 * @param count  Sample count.
 * @param coeffs Filter coefficients.
 * @param state  Per-channel state, carried across calls.
 * @param mix    Wet amount in [0, 1]. 0 = bypass, 1 = fully filtered.
 *
 * @note Transposed direct form II: one multiply-add pair per state variable.
 */
inline void ProcessInPlace(float*			   buffer, uint32_t		count,
						   const BiquadCoeffs& coeffs, BiquadState& state, float mix) noexcept {
    if (buffer == nullptr || count == 0) return;

    const float dry = 1.0f - mix;
    for (uint32_t itr = 0; itr < count; ++itr) {
        const float in  = buffer[itr];
        const float out = coeffs.b0 * in + state.z1;
        state.z1 = coeffs.b1 * in - coeffs.a1 * out + state.z2;
        state.z2 = coeffs.b2 * in - coeffs.a2 * out;
        buffer[itr] = dry * in + mix * out;
    }
}

} // namespace biquad

}

#endif // WIT_BIQUAD_H