/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_WAVEFORM_PROVIDER_H
#define WIT_WAVEFORM_PROVIDER_H

#include <cstdint>

namespace wit {

/**
 * @class IWaveformProvider
 * @brief Abstract sample source. This is the interface class.
 *
 * @note All reads produce non-interleaved(planar) float samples.
 */
class IWaveformProvider {
public:
    virtual ~IWaveformProvider() = default;

    /**
	 * @brief Read up to sample_count per-channel samples starting at sample_offset into left/right buffers.
	 *
	 * @return The number of samples actually written (<= sample_count).
     */
    virtual uint64_t Read(uint64_t sample_offset,	uint64_t sample_count,
						  float*   out_left,		float*	 out_right)	noexcept = 0;

	/* =====================================================================
	 * Accessors - virtual methods
	 * ===================================================================== */
    [[nodiscard]] virtual uint64_t TotalSampleCount()				const noexcept = 0;
    [[nodiscard]] virtual uint8_t  Channels()						const noexcept = 0;
};

}

#endif // WIT_WAVEFORM_PROVIDER_H
