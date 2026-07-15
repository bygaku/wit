/**
 * Created by intwi on 2026/07/03.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_AUDIO_FORMAT_H
#define WIT_AUDIO_FORMAT_H

#include <cstdint>

namespace wit {

/**
 * @enum SampleFormat
 */
enum class SampleFormat : uint8_t {
	INT   = 0,
	FLOAT = 1,
};

/**
 * @struct AudioFormat
 * @brief The audio format used consistently throughout the entire project.
 *        All .wwb waveform data is saved in this format.
 */
struct AudioFormat {
	uint32_t     sample_rate   = 0;                   ///< 44100 / 48000
	uint16_t     bit_depth     = 0;                   ///< 16 / 32
	SampleFormat sample_format = SampleFormat::INT;   ///< The sample format used throughout the entire project
	uint8_t      channels      = 0;                   ///< 1 / 2
};

}

#endif // WIT_AUDIO_FORMAT_H