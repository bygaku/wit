/**
 * Created by intwi on 2026/07/11.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_STREAMING_WAVEFORM_PROVIDER_H
#define WIT_STREAMING_WAVEFORM_PROVIDER_H

#include <array>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <string>

#include "audio_format.h"
#include "streaming_reader.h"
#include "waveform_provider.h"

namespace wit {

/**
 * @class StreamingWaveformProvider
 * @brief Supplies samples from a .wwb file on demand through a double buffer filled by the streaming read thread.
 */
class StreamingWaveformProvider final : public IWaveformProvider {
public:
    static constexpr uint32_t BUFFER_FRAMES = 8192;	///< Frames per internal buffer
    static constexpr uint32_t BUFFER_COUNT  = 2;	///< Double buffering

    StreamingWaveformProvider(std::string	wwb_path,	   uint64_t		wwb_offset,		uint64_t byte_size,
                              uint64_t		sample_count,  SampleFormat sample_format,
                              uint16_t		bit_depth,	   uint8_t		channels,
                              StreamingReader* reader) noexcept;

    ~StreamingWaveformProvider() override = default;

#pragma region
	StreamingWaveformProvider(const StreamingWaveformProvider&)            = delete;
    StreamingWaveformProvider& operator=(const StreamingWaveformProvider&) = delete;
    StreamingWaveformProvider(StreamingWaveformProvider&&)                 = delete;
    StreamingWaveformProvider& operator=(StreamingWaveformProvider&&)      = delete;
#pragma endregion

	/* =====================================================================
	 * Accessors
	 * ===================================================================== */
    uint64_t Read(uint64_t sample_offset,	uint64_t sample_count,
				  float*   out_left,		float*	 out_right)	noexcept override;

    [[nodiscard]] uint64_t TotalSampleCount() const noexcept override { return sample_count_; }
    [[nodiscard]] uint8_t  Channels()		  const noexcept override { return channels_; }

	/* =====================================================================
	 * Streaming control
	 * ===================================================================== */
    /**
     * @brief Whether samples at the given offset can be served right now.
     */
    [[nodiscard]] bool IsReadyAt(uint64_t sample_offset) const noexcept;

    /**
     * @brief Per-callback maintenance.
     *
     * Recycle consumed buffers and enqueue fill requests.
     */
    void UpdateStreaming(uint64_t sample_cursor) noexcept;

    /**
     * @brief Set where prefetch wraps.
     *
     * Past `wrap_at` the read-ahead continues from `loop_start`.
     * Defaults to a full-waveform wrap.
     */
    void SetLoopRegion(uint64_t loop_start, uint64_t wrap_at) noexcept;

	/* =====================================================================
	 * Streaming read thread entry
	 * ===================================================================== */
    /**
     * @brief Fill one buffer with decoded planar samples read from the .wwb file.
     * @note Called from the streaming read thread only.
     */
    void FillBuffer(uint32_t buffer_index, uint64_t start_sample, uint32_t frame_count) noexcept;

private:
    /**
     * @struct Buffer
     * @brief One half of the double buffer.
     *
     * @note `left` / `right` / `start_sample` / `valid_frames` are written by the read thread
     * and become visible to the audio thread through `ready`.
     */
    struct Buffer {
        std::array<float, BUFFER_FRAMES> left{};
        std::array<float, BUFFER_FRAMES> right{};

        uint64_t          start_sample = 0;
        uint32_t          valid_frames = 0;

    	std::atomic<bool> ready{false};
    };

    /**
     * @brief Wraps past the waveform end to stay ready for future loop playback.
     * @return The next uncovered sample position, or UINT64_MAX if the lookahead window is already fully covered.
     */
    [[nodiscard]] uint64_t ComputeNextFillStart(uint64_t cursor) const noexcept;

    /**
     * @brief Whether a buffer currently holds (or will hold) the given range start.
     */
    [[nodiscard]] bool CoversOrPlanned(uint32_t buffer_index, uint64_t sample) const noexcept;

	/* =====================================================================
	 * Immutable source description
	 * ===================================================================== */
    std::string   wwb_path_;
    uint64_t      wwb_offset_    = 0;
    uint64_t      byte_size_     = 0;
    uint64_t      sample_count_  = 0;
    SampleFormat  sample_format_ = SampleFormat::INT;
    uint16_t      bit_depth_     = 0;
    uint8_t       channels_      = 0;
    uint32_t      bytes_per_frame_ = 0;

	/* =====================================================================
	 * Shared buffer storage (handoff via `ready`)
	 * ===================================================================== */
    std::array<Buffer, BUFFER_COUNT> buffers_;

	/* =====================================================================
	 * Audio-thread-only bookkeeping
	 * ===================================================================== */
    std::array<bool,     BUFFER_COUNT> fill_in_flight_{};	///< Request enqueued, completion not yet observed
    std::array<uint64_t, BUFFER_COUNT> planned_start_{};	///< Range promised to an in-flight fill
    std::array<uint32_t, BUFFER_COUNT> planned_frames_{};
    uint64_t loop_start_ = 0;								///< Prefetch wrap target
    uint64_t wrap_at_    = 0;								///< Prefetch wrap point. 0 = waveform end

	/* =====================================================================
	 * Read-thread-only state
	 * ===================================================================== */
    std::ifstream file_;	///< Opened once at construction, used only by the read thread afterwards

    StreamingReader* reader_ = nullptr;
};

}

#endif // WIT_STREAMING_WAVEFORM_PROVIDER_H
