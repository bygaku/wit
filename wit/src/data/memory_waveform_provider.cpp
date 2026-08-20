/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#include "memory_waveform_provider.h"

#include <algorithm>

#include "sample_decode.h"

namespace wit {

MemoryWaveformProvider::MemoryWaveformProvider(std::vector<uint8_t>&& pcm_data, uint64_t sample_count, SampleFormat sample_format, uint16_t bit_depth, uint8_t channels) noexcept
    : pcm_data_(std::move(pcm_data)),
      sample_count_(sample_count),
      sample_format_(sample_format),
      bit_depth_(bit_depth),
      channels_(channels) {
}

uint64_t MemoryWaveformProvider::Read(uint64_t sample_offset, uint64_t sample_count, float*	out_left, float* out_right) noexcept {
    if (sample_offset >= sample_count_) return 0;
    if (out_left == nullptr)            return 0;

    const uint64_t available = sample_count_ - sample_offset;
    const uint64_t to_read   = std::min(sample_count, available);

    const uint32_t bytes_per_sample = static_cast<uint32_t>(bit_depth_) / 8u;
    const uint32_t bytes_per_frame  = bytes_per_sample * channels_;

	// 16bit int
	if (sample_format_ == SampleFormat::INT && bit_depth_ == 16) {
		for (uint32_t i = 0; i < to_read; ++i) {
			const uint64_t frame_index = sample_offset + i;
			const uint8_t* fp          = pcm_data_.data() + frame_index * bytes_per_frame;

			if (channels_ == 1) {
					const float s	= decode::ReadInt16LE(fp);
					out_left[i]		= s;
					out_right[i]	= s;
			} else if (channels_ == 2) {
					out_left[i]  = decode::ReadInt16LE(fp);
					out_right[i] = decode::ReadInt16LE(fp + 2);
			}
		}
	}

	// 32bit float
	if (sample_format_ == SampleFormat::FLOAT && bit_depth_ == 32) {
		for (uint32_t i = 0; i < to_read; ++i) {
			const uint64_t frame_index = sample_offset + i;
			const uint8_t* fp          = pcm_data_.data() + frame_index * bytes_per_frame;

			if (channels_ == 1) {
				const float s	= decode::ReadFloat32LE(fp);
				out_left[i]		= s;
				out_right[i]	= s;
			} else if (channels_ == 2) {
				out_left[i]  = decode::ReadFloat32LE(fp);
				out_right[i] = decode::ReadFloat32LE(fp + 4);
			}
		}
	}

    return to_read;
}

}
