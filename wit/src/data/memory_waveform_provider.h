/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_MEMORY_WAVEFORM_PROVIDER_H
#define WIT_MEMORY_WAVEFORM_PROVIDER_H

#include <cstdint>
#include <vector>

#include "audio_format.h"
#include "waveform_provider.h"

namespace wit {

/**
 * @class MemoryWaveformProvider
 * @brief Provider that serves samples from an in-memory byte buffer.
 */
class MemoryWaveformProvider : public IWaveformProvider {
public:
    /**
     * @param pcm_data from .wwb DATA.
     * @param sample_count Per-channel sample count. (frame size)
     * @param sample_format from .wpb FMT.
     * @param bit_depth from .wpb FMT.
     * @param channels from .wpb FMT.
     */
    MemoryWaveformProvider(std::vector<uint8_t>&& pcm_data,		 uint64_t sample_count,
                           SampleFormat			  sample_format, uint16_t bit_depth,	uint8_t channels) noexcept;

	/**
 	 * @brief Read up to sample_count per channel samples starting at sample_offset into left/right buffers.
 	 * @return The number of samples actually written (<= sample_count).
 	 */
    uint64_t Read(uint64_t sample_offset, uint64_t sample_count,
    			  float*   out_left,	  float*   out_right) noexcept override;

	/* =====================================================================
	 * Accessors
	 * ===================================================================== */
    [[nodiscard]] uint64_t TotalSampleCount() const noexcept override { return sample_count_; }
    [[nodiscard]] uint8_t  Channels()         const noexcept override { return channels_; }

private:
	std::vector<uint8_t> pcm_data_;
    uint64_t			 sample_count_;
    SampleFormat   		 sample_format_;
    uint16_t       		 bit_depth_;
    uint8_t        		 channels_;
};

}

#endif // WIT_MEMORY_WAVEFORM_PROVIDER_H
