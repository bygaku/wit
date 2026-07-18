/**
 * Created by intwi on 2026/07/11.
 * Copyright (c) 2026 All rights reserved.
 */
#include "streaming_waveform_provider.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "sample_decode.h"

/** HACK: This code were written by Claude,
 *  I need to understand the implementation details and rewrite the code accordingly.
 */
namespace wit {

namespace {
constexpr uint64_t LOOKAHEAD_WINDOW = static_cast<uint64_t>(StreamingWaveformProvider::BUFFER_FRAMES) * 2;
}  // namespace

StreamingWaveformProvider::StreamingWaveformProvider(std::string  wwb_path,     uint64_t	 wwb_offset,	uint64_t byte_size,
                                                     uint64_t     sample_count, SampleFormat sample_format,
                                                     uint16_t     bit_depth,    uint8_t		 channels,
                                                     StreamingReader* reader) noexcept
    : wwb_path_(std::move(wwb_path)),
      wwb_offset_(wwb_offset),
      byte_size_(byte_size),
      sample_count_(sample_count),
      sample_format_(sample_format),
      bit_depth_(bit_depth),
      channels_(channels),
      bytes_per_frame_((static_cast<uint32_t>(bit_depth) / 8u) * channels),
      reader_(reader) {
    // Opened on the main thread during load; only the read thread seeks/reads afterward.
    file_.open(wwb_path_, std::ios::binary);
}

uint64_t StreamingWaveformProvider::Read(uint64_t sample_offset, uint64_t sample_count,
                                         float*   out_left,      float*   out_right) noexcept {
    if (sample_offset >= sample_count_) return 0;
    if (out_left == nullptr)            return 0;

    const uint64_t available = sample_count_ - sample_offset;
    const uint64_t to_read   = std::min(sample_count, available);	///< Amount Left to Read

    uint64_t copied = 0;
    uint64_t pos    = sample_offset;

    while (copied < to_read) {
        const Buffer* hit = nullptr;
        for (const Buffer& buf : buffers_) {
            if (!buf.ready.load(std::memory_order_acquire))	continue;
            if (pos < buf.start_sample)                     	continue;
            if (pos >= buf.start_sample + buf.valid_frames) 	continue;
            hit = &buf;
            break;
        }
        if (hit == nullptr) break;	///< Buffer Underrun

        const uint64_t local = pos - hit->start_sample;
        const uint64_t n     = std::min(to_read - copied,
                                        static_cast<uint64_t>(hit->valid_frames) - local);

        std::memcpy(out_left + copied, hit->left.data() + local, n * sizeof(float));
        if (out_right != nullptr) {
            std::memcpy(out_right + copied, hit->right.data() + local, n * sizeof(float));
        }

        copied += n;
        pos    += n;
    }

    return copied;
}

bool StreamingWaveformProvider::IsReadyAt(uint64_t sample_offset) const noexcept {
    if (sample_offset >= sample_count_) return true;	///< Nothing left to serve

    for (const Buffer& buf : buffers_) {
        if (!buf.ready.load(std::memory_order_acquire))         continue;
        if (sample_offset < buf.start_sample)                      continue;
        if (sample_offset >= buf.start_sample + buf.valid_frames)  continue;
        return true;
    }
    return false;
}

bool StreamingWaveformProvider::CoversOrPlanned(uint32_t buffer_index, uint64_t sample) const noexcept {
    if (fill_in_flight_[buffer_index]) {
        return sample >= planned_start_[buffer_index] &&
               sample <  planned_start_[buffer_index] + planned_frames_[buffer_index];
    }
    const Buffer& buf = buffers_[buffer_index];
    if (!buf.ready.load(std::memory_order_acquire)) return false;
    return sample >= buf.start_sample && sample < buf.start_sample + buf.valid_frames;
}

void StreamingWaveformProvider::SetLoopRegion(uint64_t loop_start, uint64_t wrap_at) noexcept {
    loop_start_ = loop_start < sample_count_ ? loop_start : 0;
    wrap_at_    = wrap_at > 0 && wrap_at <= sample_count_ ? wrap_at : 0;
    if (wrap_at_ != 0 && loop_start_ >= wrap_at_) {
        loop_start_ = 0;
        wrap_at_    = 0;
    }
}

uint64_t StreamingWaveformProvider::ComputeNextFillStart(uint64_t cursor) const noexcept {
    const uint64_t wrap_at    = wrap_at_ != 0 ? wrap_at_ : sample_count_;
    const uint64_t wrap_span  = wrap_at - loop_start_;

    uint64_t candidate = cursor;
    uint64_t walked    = 0;

    bool merged = true;
    while (merged) {
        merged = false;
        // Past `wrap_at` the　timeline continues from `loop_start_`.
        uint64_t logical = candidate;
        if (logical >= wrap_at) {
            logical = loop_start_ + (logical - wrap_at) % wrap_span;
        }

        for (uint32_t itr = 0; itr < BUFFER_COUNT; ++itr) {
            uint64_t range_start = 0;
            uint64_t range_end   = 0;

            if (fill_in_flight_[itr]) {
                range_start = planned_start_[itr];
                range_end   = planned_start_[itr] + planned_frames_[itr];
            } else if (buffers_[itr].ready.load(std::memory_order_acquire)) {
                range_start = buffers_[itr].start_sample;
                range_end   = buffers_[itr].start_sample + buffers_[itr].valid_frames;
            } else {
                continue;
            }

            if (logical >= range_start && logical < range_end) {
                uint64_t advance = range_end - logical;

            	if (logical < wrap_at && logical + advance > wrap_at) advance = wrap_at - logical;

            	candidate += advance;
                walked    += advance;
                merged     = true;
                break;
            }
        }

        if (walked >= LOOKAHEAD_WINDOW) return UINT64_MAX;	///< Runway already long enough
    }

    uint64_t logical = candidate;
    if (logical >= wrap_at) {
        logical = loop_start_ + (logical - wrap_at) % wrap_span;
    }
    return logical;
}

void StreamingWaveformProvider::UpdateStreaming(uint64_t sample_cursor) noexcept {
    if (reader_ == nullptr)  return;
    if (sample_count_ == 0)  return;

    const uint64_t cursor = std::min(sample_cursor, sample_count_ - 1);

    // Reconcile completed fills.
    for (uint32_t itr = 0; itr < BUFFER_COUNT; ++itr) {
        if (fill_in_flight_[itr] && buffers_[itr].ready.load(std::memory_order_acquire)) {
            fill_in_flight_[itr] = false;
        }
    }

    const uint64_t wrap_at   = wrap_at_ != 0 ? wrap_at_ : sample_count_;
    const uint64_t wrap_span = wrap_at - loop_start_;

    std::array<bool, BUFFER_COUNT> on_runway{};
    {
        uint64_t position = cursor;
        uint64_t walked   = 0;
        while (walked < LOOKAHEAD_WINDOW) {
            uint64_t logical = position;
            if (logical >= wrap_at) {
                logical = loop_start_ + (logical - wrap_at) % wrap_span;
            }

            bool found = false;
            for (uint32_t itr = 0; itr < BUFFER_COUNT; ++itr) {
                uint64_t range_start = 0;
                uint64_t range_end   = 0;
                if (fill_in_flight_[itr]) {
                    range_start = planned_start_[itr];
                    range_end   = planned_start_[itr] + planned_frames_[itr];
                } else if (buffers_[itr].ready.load(std::memory_order_acquire)) {
                    range_start = buffers_[itr].start_sample;
                    range_end   = buffers_[itr].start_sample + buffers_[itr].valid_frames;
                } else {
                    continue;
                }

                if (logical >= range_start && logical < range_end) {
                    uint64_t advance = range_end - logical;
                    if (logical < wrap_at && logical + advance > wrap_at) {
                        advance = wrap_at - logical;	///< The timeline wraps inside this range
                    }
                    on_runway[itr] = true;
                    position += advance;
                    walked   += advance;
                    found     = true;
                    break;
                }
            }
            if (!found) break;	///< First gap: everything past here is refilled below
        }
    }

    // Recycle ready buffers that are not part of the runway.
    for (uint32_t itr = 0; itr < BUFFER_COUNT; ++itr) {
        if (fill_in_flight_[itr]) continue;
        if (on_runway[itr])       continue;
        if (buffers_[itr].ready.load(std::memory_order_acquire)) {
            buffers_[itr].ready.store(false, std::memory_order_relaxed);	///< Audio thread owns the buffer again
        }
    }

    // Hand every empty buffer a fill order extending the covered runway.
    for (uint32_t itr = 0; itr < BUFFER_COUNT; ++itr) {
        if (fill_in_flight_[itr])                                  continue;
        if (buffers_[itr].ready.load(std::memory_order_acquire))   continue;

        const uint64_t start = ComputeNextFillStart(cursor);
        if (start == UINT64_MAX) break;	///< Lookahead window fully covered

        const uint64_t fill_limit = (wrap_at_ != 0 && start < wrap_at_) ? wrap_at_ : sample_count_;
        const uint32_t frames     = static_cast<uint32_t>(
            std::min<uint64_t>(BUFFER_FRAMES, fill_limit - start));
        if (frames == 0) break;

        StreamRequest request;
        request.provider     = this;
        request.buffer_index = itr;
        request.start_sample = start;
        request.frame_count  = frames;

        if (!reader_->RequestFill(request)) break;	///< Queue full, retry next callback

        fill_in_flight_[itr] = true;
        planned_start_[itr]  = start;
        planned_frames_[itr] = frames;
    }
}

void StreamingWaveformProvider::FillBuffer(uint32_t buffer_index, uint64_t start_sample, uint32_t frame_count) noexcept {
    if (buffer_index >= BUFFER_COUNT) return;

    Buffer& buf = buffers_[buffer_index];
    const uint32_t frames = std::min(frame_count, BUFFER_FRAMES);

    bool read_ok = false;
    std::vector<uint8_t> raw(static_cast<size_t>(frames) * bytes_per_frame_);

    if (file_.is_open() && bytes_per_frame_ > 0) {
        file_.clear();	///< Clear a possible EOF flag from a previous tail read
        const std::streamoff pos =
            static_cast<std::streamoff>(wwb_offset_) +
            static_cast<std::streamoff>(start_sample) * static_cast<std::streamoff>(bytes_per_frame_);
        file_.seekg(pos, std::ios::beg);
        file_.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()));
        read_ok = file_.gcount() == static_cast<std::streamsize>(raw.size());
    }

    if (read_ok) {
        for (uint32_t itr = 0; itr < frames; ++itr) {
            const uint8_t* fp = raw.data() + static_cast<size_t>(itr) * bytes_per_frame_;

            if (sample_format_ == SampleFormat::INT && bit_depth_ == 16) {
                if (channels_ == 1) {
                    const float s   = decode::ReadInt16LE(fp);
                    buf.left[itr]   = s;
                    buf.right[itr]  = s;
                } else {
                    buf.left[itr]   = decode::ReadInt16LE(fp);
                    buf.right[itr]  = decode::ReadInt16LE(fp + 2);
                }
            } else if (sample_format_ == SampleFormat::FLOAT && bit_depth_ == 32) {
                if (channels_ == 1) {
                    const float s   = decode::ReadFloat32LE(fp);
                    buf.left[itr]   = s;
                    buf.right[itr]  = s;
                } else {
                    buf.left[itr]   = decode::ReadFloat32LE(fp);
                    buf.right[itr]  = decode::ReadFloat32LE(fp + 4);
                }
            } else {
                buf.left[itr]  = 0.0f;
                buf.right[itr] = 0.0f;
            }
        }
    } else {
        // Fail soft: audible silence instead of a stalled voice.
        std::fill_n(buf.left.data(),  frames, 0.0f);
        std::fill_n(buf.right.data(), frames, 0.0f);
    }

    buf.start_sample = start_sample;
    buf.valid_frames = frames;
    buf.ready.store(true, std::memory_order_release);	///< Publish to the audio thread
}

}
