/**
* Created by intwi on 2026/07/11.
 * Copyright (c) 2026 All rights reserved.
 */
#include "streaming_reader.h"

#include <chrono>

#include "streaming_waveform_provider.h"

/** HACK: This code were written by Claude,
 *  I need to understand the implementation details and rewrite the code accordingly.
 */
namespace wit {

void StreamingReader::Start() {
	if (running_.load(std::memory_order_acquire)) return;

	running_.store(true, std::memory_order_release);
	thread_ = std::thread(&StreamingReader::ThreadMain, this);
}

void StreamingReader::Stop() {
	if (!running_.load(std::memory_order_acquire)) return;

	running_.store(false, std::memory_order_release);
	if (thread_.joinable()) thread_.join();

	StreamRequest request; ///< Drain anything left so no request outlives its provider
	while (requests_.Dequeue(request)) {}
}

bool StreamingReader::RequestFill(const StreamRequest& request) noexcept {
	if (request.provider == nullptr) return false;
	return requests_.Enqueue(request);
}

void StreamingReader::ThreadMain() {
	// Polling loop. A condition variable would wake faster, but notifying
	// one from the audio thread risks a syscall inside the realtime
	// callback. With a double buffer holding ~170ms of audio per fill,
	// a 1ms poll interval is far inside the deadline.
	while (running_.load(std::memory_order_acquire)) {
		StreamRequest request;
		bool worked = false;

		while (requests_.Dequeue(request)) {
			if (request.provider != nullptr) {
				request.provider->FillBuffer(request.buffer_index,
											 request.start_sample,
											 request.frame_count);
			}
			worked = true;
		}

		if (!worked) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

}
