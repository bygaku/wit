/**
 * Created by intwi on 2026/06/04.
 * Copyright (c) 2026 All rights reserved.
 */
#include "mixer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "biquad.h"
#include "data/category_store.h"
#include "data/waveform_provider.h"
#include "voice.h"
#include "voice_pool.h"

namespace wit {

namespace {

inline void ClampInPlace(float* buffer, uint32_t count, float threshold) noexcept {
    for (uint32_t itr = 0; itr < count; ++itr) {
        if (buffer[itr] >  threshold) buffer[itr] =  threshold;
        if (buffer[itr] < -threshold) buffer[itr] = -threshold;
    }
}

inline float TapePositionAt(float base, float step, uint32_t offset) noexcept {
    float pos = base + step * static_cast<float>(offset);
    if (pos < 0.0f) pos = 0.0f;
    if (pos > 1.0f) pos = 1.0f;
    return pos;
}

/**
 * @brief Map a ramp position in [0, 1] onto a playback rate multiplier.
 * @note Position 1 is normal speed and each step down the ramp drops a fixed
 *       number of octaves, so the glide is even to the ear. A linear rate
 *       ramp would instead spend half its length in the first octave alone.
 */
inline float TapeRateFromPosition(float position) noexcept {
    if (position >= 1.0f) return 1.0f;
    if (position <= 0.0f) return 0.0f;
	return std::exp2((position - 1.0f) * Voice::TAPE_OCTAVE_RANGE) * position;
}

/**
 * @brief Amplitude taper applied at the very bottom of the ramp.
 * @note The rate can only fall so far before it is clamped, so without this
 *       the voice would still be audible when the ramp ends and the cut to
 *       silence would click.
 */
inline float TapeGainFromPosition(float position) noexcept {
    if (position >= Voice::TAPE_GAIN_TAPER) return 1.0f;
    if (position <= 0.0f)                   return 0.0f;
    return position / Voice::TAPE_GAIN_TAPER;
}

inline float FadeAt(float base, float step, uint32_t offset) noexcept {
    float g = base + step * static_cast<float>(offset);
    if (g < 0.0f) g = 0.0f;
    if (g > 1.0f) g = 1.0f;
    return g;
}

} // namespace

void Mixer::Process(VoicePool& voices, const CategoryStore& categories, uint32_t frame_count, float* output) noexcept {
    if (output == nullptr) return;	///< no output signal

	// Padding output signal.
    if (frame_count == 0 || frame_count > MAX_FRAME_COUNT) {
        std::memset(output, 0, static_cast<size_t>(frame_count) * 2 * sizeof(float));
        return;
    }

    // Render in short blocks.
    for (uint32_t offset = 0; offset < frame_count; offset += CONTROL_BLOCK_FRAMES) {
        uint32_t block = frame_count - offset;
        if (block > CONTROL_BLOCK_FRAMES) block = CONTROL_BLOCK_FRAMES;

        ProcessBlock(voices, categories, block,mix_left_.data() + offset,mix_right_.data() + offset);
    }

    // Apply the limiter.
    ClampInPlace(mix_left_.data(),  frame_count, LIMITER_THRESHOLD);
    ClampInPlace(mix_right_.data(), frame_count, LIMITER_THRESHOLD);

    // Planar to interleave.
    for (uint32_t itr = 0; itr < frame_count; ++itr) {
        output[itr * 2 + 0] = mix_left_[itr];
        output[itr * 2 + 1] = mix_right_[itr];
    }
}

void Mixer::ProcessBlock(VoicePool&	voices,	const CategoryStore& categories,
                         uint32_t frame_count, float* out_left,	float* out_right) noexcept {
    // Zero padding.
    std::fill_n(out_left,  frame_count, 0.0f);
    std::fill_n(out_right, frame_count, 0.0f);

    for (size_t si = 0; si < VoicePool::MAX_VOICE_COUNT; ++si) {
        Voice& v = voices.At(si);
        const bool audible = v.state == VoiceState::PLAYING
                          || v.state == VoiceState::PAUSING
                          || v.state == VoiceState::STOPPING;
        if (!audible) continue;	///< Ignore

        const float  category_gain = categories.GetCurrentLinearGain(v.category_id);
        const float  voice_gain    = v.volume * category_gain;

        float tape_rate       = 1.0f;
        float tape_gain_begin = 1.0f;
        float tape_gain_end   = 1.0f;
        if (v.tape.active) {
            const float pos_mid = TapePositionAt(v.tape.position, v.tape.step, frame_count / 2);
            const float pos_end = TapePositionAt(v.tape.position, v.tape.step, frame_count);
            tape_rate       = TapeRateFromPosition(pos_mid);
            tape_gain_begin = TapeGainFromPosition(v.tape.position);
            tape_gain_end   = TapeGainFromPosition(pos_end);
        	if (tape_rate < Voice::TAPE_MIN_RATE) tape_rate = Voice::TAPE_MIN_RATE;
        }

        const auto pitch = static_cast<double>(v.pitch) * static_cast<double>(tape_rate);
        if (pitch <= 0.0) continue;	///< can't playback

        std::fill_n(voice_left_.data(),  frame_count, 0.0f);
        std::fill_n(voice_right_.data(), frame_count, 0.0f);

        // Sample-accurate loop wrap point on the cue timeline (0 = no looping).
        const auto loop_end   = static_cast<double>(v.loop_end);
        const auto loop_start = static_cast<double>(v.loop_start);
        const bool looping    = v.loop_end > 0;

        for (uint8_t wi = 0; wi < v.active_slot_count; ++wi) {
            Voice::Slot& slot = v.slots[wi];
            if (slot.provider == nullptr) continue;

            const auto total_samples = slot.provider->TotalSampleCount();
            const auto total_d       = static_cast<double>(total_samples);
            if (!looping && slot.cursor >= total_d) continue;

            uint32_t out_index = 0;
            while (out_index < frame_count) {
                if (looping && slot.cursor >= loop_end) {
                    slot.cursor -= (loop_end - loop_start);	///< Wrap on the cue timeline
                    continue;
                }

                // Frames until the next timeline boundary at the current pitch.
                const double boundary  = looping ? std::min(total_d, loop_end) : total_d;
                const double remaining = boundary - slot.cursor;
                if (remaining <= 0.0) {
                    if (!looping) break;
                    // Silent tail of a short waveform: advance time only.
                    const auto silent = static_cast<uint32_t>(std::min<double>(
                        frame_count - out_index,
                        std::ceil((loop_end - slot.cursor) / pitch)));
                    if (silent == 0) break;
                    slot.cursor += pitch * static_cast<double>(silent);
                    out_index   += silent;
                    continue;
                }

                auto segment_frames = static_cast<uint32_t>(remaining / pitch) + 1;
                if (segment_frames > frame_count - out_index) segment_frames = frame_count - out_index;

                // Calc the source block range needed to cover segment_frames output frames advancing at `pitch`.
                const auto last_cursor = slot.cursor + pitch * static_cast<double>(segment_frames - 1);
                const auto src_begin   = static_cast<uint64_t>(slot.cursor);
                uint64_t   src_end     = static_cast<uint64_t>(last_cursor) + 2;
                if (src_end > total_samples) src_end = total_samples;
                if (src_end <= src_begin)    break;

                const uint64_t needed = src_end - src_begin;
                if (needed + 1 > TMP_SOURCE_SIZE) break;  ///< pitch too high

                const auto read = slot.provider->Read(
                    src_begin, needed, tmp_left_.data(), tmp_right_.data());
                if (read == 0) break;	///< Streaming underrun: silence, retry next callback

                double max_local = static_cast<double>(read) - 1.0;
                if (src_begin + read >= total_samples) {
                    tmp_left_[read]  = 0.0f;
                    tmp_right_[read] = 0.0f;
                    max_local = static_cast<double>(read);
                }

                auto local_cursor = slot.cursor - static_cast<double>(src_begin);

                uint32_t produced = 0;
                for (; produced < segment_frames; ++produced) {
                    if (local_cursor >= max_local) break;

                    const auto	   idx  = static_cast<uint64_t>(local_cursor);
                    const auto     frac = static_cast<float>(local_cursor - static_cast<double>(idx));
                    const float    sl   = tmp_left_[idx]  * (1.0f - frac) + tmp_left_[idx  + 1] * frac;
                    const float    sr   = tmp_right_[idx] * (1.0f - frac) + tmp_right_[idx + 1] * frac;

                    voice_left_[out_index + produced]  += sl;
                    voice_right_[out_index + produced] += sr;

                    local_cursor += pitch;
                }

                slot.cursor = static_cast<double>(src_begin) + local_cursor;
                out_index  += produced;
                if (produced == 0) break;	///< No forward progress (underrun boundary), bail out
            }
        }

        // Tone shaping first: the filter sees the raw voice signal only.
        if (v.filter.active) {
            biquad::ProcessInPlace(voice_left_.data(),  frame_count,
                                   v.filter.coeffs, v.filter.channel[0], v.filter.mix);
            biquad::ProcessInPlace(voice_right_.data(), frame_count,
                                   v.filter.coeffs, v.filter.channel[1], v.filter.mix);
        }

        const float tape_gain_delta = (tape_gain_end - tape_gain_begin) / static_cast<float>(frame_count);

        for (uint32_t itr = 0; itr < frame_count; ++itr) {
            const float fade = FadeAt(v.fade_gain, v.fade_step, itr);
            const float tape = tape_gain_begin + tape_gain_delta * static_cast<float>(itr);
            const float gain = voice_gain * fade * tape;
            out_left[itr]  += voice_left_[itr]  * gain;
            out_right[itr] += voice_right_[itr] * gain;
        }

        // Advance the fade once for the whole voice
        if (v.fade_step != 0.0f) {
            v.fade_gain = FadeAt(v.fade_gain, v.fade_step, frame_count);
        }

        // Advance the tape ramp once for the whole voice
        if (v.tape.active && v.tape.step != 0.0f) {
            v.tape.position = TapePositionAt(v.tape.position, v.tape.step, frame_count);
        }
    }

}

}