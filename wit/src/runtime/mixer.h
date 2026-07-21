/**
* Created by intwi on 2026/06/04.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_MIXER_H
#define WIT_MIXER_H

#include <array>
#include <cstdint>

namespace wit {

class VoicePool;
class CategoryStore;

/**
 * @class Mixer
 * @brief Combines all playing Voices.
 *
 * @note Called once per audio callback.
 */
class Mixer {
public:
	/// HACK: I'll look it up myself later. My AI said "miniaudio typically requests 256-2048 frames".
    static constexpr uint32_t MAX_FRAME_COUNT = 2048;	///< Largest frame count the internal buffers can handle in one Process() call
	static constexpr float LIMITER_THRESHOLD  = 0.99f;	///< It's so simple!
	static constexpr float FADE_DURATION_MS	  = 100.0f;	///< Very short click-suppression fade for pause / resume / stop.

    Mixer() = default;

    /**
     * @brief Produce one callback's worth of interleaved stereo float samples.
     */
    void Process(VoicePool& voices,			const CategoryStore& categories,
                 uint32_t	frame_count,	float* output) noexcept;

private:
    static constexpr uint32_t TMP_SOURCE_SIZE = MAX_FRAME_COUNT * 2 + 4;

	/// HACK: Will refactor
    std::array<float, MAX_FRAME_COUNT> mix_left_{};
    std::array<float, MAX_FRAME_COUNT> mix_right_{};
    std::array<float, TMP_SOURCE_SIZE> tmp_left_{};
    std::array<float, TMP_SOURCE_SIZE> tmp_right_{};
};

}

#endif // WIT_MIXER_H