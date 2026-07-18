/**
 * Created by intwi on 2026/07/11.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_STREAMING_READER_H
#define WIT_STREAMING_READER_H

#include <atomic>
#include <cstdint>
#include <thread>
#include <type_traits>

#include "util/ring_buffer.h"

namespace wit {

class StreamingWaveformProvider;

/**
 * @struct StreamRequest
 * @brief A single buffer-fill order sent from the audio thread to the streaming read thread.
 */
struct StreamRequest {
    StreamingWaveformProvider* provider     = nullptr;	///< Target provider that owns the buffer
    uint32_t                   buffer_index = 0;		///< Which internal buffer to fill
    uint64_t                   start_sample = 0;		///< Per-channel sample offset within the waveform
    uint32_t                   frame_count  = 0;		///< Number of frames to fill
};

static_assert(std::is_trivially_copyable_v<StreamRequest>,
              "StreamRequest must be trivially copyable to fit in RingBuffer");

/**
 * @class StreamingReader
 * @brief Owns the streaming read thread and the fill-request queue.
 */
class StreamingReader {
public:
    static constexpr size_t REQUEST_QUEUE_CAPACITY = 64;

    StreamingReader() = default;
    ~StreamingReader() { Stop(); }

#pragma region
    StreamingReader(const StreamingReader&)            = delete;
    StreamingReader& operator=(const StreamingReader&) = delete;
    StreamingReader(StreamingReader&&)                 = delete;
    StreamingReader& operator=(StreamingReader&&)      = delete;
#pragma endregion

    /**
     * @brief Launch the read thread. No-op if already running.
     */
    void Start();

    /**
     * @brief Drain remaining requests and join the thread. Safe to call twice.
     */
    void Stop();

    /**
     * @brief Enqueue one fill request.
     * @note Audio thread only.
     * @return true if the request was accepted, false if the queue was full.
     */
    bool RequestFill(const StreamRequest& request) noexcept;

private:
    void ThreadMain();

    RingBuffer<StreamRequest, REQUEST_QUEUE_CAPACITY> requests_;
    std::thread       thread_;
    std::atomic<bool> running_{false};
};

}

#endif // WIT_STREAMING_READER_H
