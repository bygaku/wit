/**
 * Created by intwi on 2026/07/05.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_WAVEFORM_INFO_H
#define WIT_WAVEFORM_INFO_H

#include <cstdint>
#include <array>

namespace wit {

constexpr std::size_t MAX_WAVEFORM_NAME_LENGTH = 32;

/**
 * @struct WaveformInfo
 * @brief Location information for a single waveform (loaded from .wccb).
 */
struct WaveformInfo {
    uint64_t    		wwb_offset		= 0;	///< Byte offset from the start of the .wwb file
    uint64_t    		wwb_size		= 0;	///< Byte size of the sample data
    uint64_t    		sample_count	= 0;	///< Number of samples per channel
	std::array<char, MAX_WAVEFORM_NAME_LENGTH> waveform_name = {0};	///< NUL-terminated (31 chars max)
};

}

#endif // WIT_WAVEFORM_INFO_H