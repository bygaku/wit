/**
 * Created by intwi on 2026/06/06.
 * Copyright (c) 2026 All rights reserved.
 */

#ifndef WIT_RING_BUFFER_H
#define WIT_RING_BUFFER_H

#include <atomic>
#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>

namespace wit {
constexpr std::size_t SAFE_CACHE_LINE_SIZE =
#if defined(__cpp_lib_hardware_interference_size) && (!defined(_MSC_VER) || _HAS_DESTRUCTIVE_INTERFERENCE_SIZE)
	64/*std::hardware_destructive_interference_size*/;
#else
		64; ///< The most widely used
#endif

/**
 * @class RingBuffer
 * @tparam ItemType Type to add to the buffer. Must be trivially copyable.
 * @tparam Cap Capacity
 */
template<typename ItemType, std::size_t Cap>
class RingBuffer {
public:
#pragma region /** To improve performance */
	static_assert((Cap & (Cap - 1)) == 0, "Capacity must be a power of two");
	static_assert(Cap >= 2, "RingBuffer Cap must be at least 2");
	static_assert(std::is_trivially_copyable_v<ItemType>, "ItemType must be trivially copyable");
#pragma endregion

#pragma region /** Disable copy and move this class */
	RingBuffer(const RingBuffer&)            = delete;
	RingBuffer& operator=(const RingBuffer&) = delete;
	RingBuffer(RingBuffer&&)                 = delete;
	RingBuffer& operator=(RingBuffer&&)      = delete;
#pragma endregion

	RingBuffer() = default;

	 /**
	  * @brief Adds a single item to the buffer.
      * @param[in] item The item to add (pass-by-value, copied internally).
	  * @return true: successful / false: The buffer was full.
	  */
	bool Enqueue(const ItemType& item) noexcept {
		const std::uint32_t write = write_idx_.load(std::memory_order_relaxed);
		const std::uint32_t read  = read_idx_.load(std::memory_order_acquire);

		// Writer's cumulative counter is Cap units ahead of the reader's.
		if (write - read >= Cap) {
			return false;
		}

		const std::uint32_t index = write & (Cap - 1);
		buffer_[index] = item;

		write_idx_.store(write + 1, std::memory_order_release);
		return true;
	}

	/**
	 * @brief Retrieves a single item from the buffer.
	 * @param[out] dest The destination for the retrieved item.
	 * @return true: successful / false: The buffer was empty (out remains unchanged)
	 */
	bool Dequeue(ItemType& dest) noexcept {
		const std::uint32_t read  = read_idx_.load(std::memory_order_relaxed);
		const std::uint32_t write = write_idx_.load(std::memory_order_acquire);

		// Reader's cumulative counter catches up to the writer's position.
		if (read == write) {
			return false;
		}

		const std::uint32_t index = read & (Cap - 1);
		dest = buffer_[index];

		read_idx_.store(read + 1, std::memory_order_release);
		return true;
	}

	/**
	 * @brief Checks whether the buffer is empty.
	 * @return true if the buffer has no elements, otherwise false.
	 */
	[[nodiscard]] bool Empty() const noexcept {
		return read_idx_.load(std::memory_order_relaxed) ==
			   write_idx_.load(std::memory_order_relaxed);
	}

	/**
	 * @brief Retrieves the approximate number of elements currently held in the buffer.
	 * @return The approximate size of the buffer,
	 * calculated as the difference between the write index and the read index.
	 */
	[[nodiscard]] size_t ApproximateSize() const noexcept {
		const size_t t = write_idx_.load(std::memory_order_relaxed);
		const size_t h = read_idx_.load(std::memory_order_relaxed);
		return t - h;
	}

	static constexpr size_t Capacity() noexcept { return Cap; }

private:
	/**
	 * @ref https://kumagi.hatenablog.com/entry/ring-buffer
	 * @note Specifying `alignas(...)` pads the memory cache.
	 * Resolve hardware dependencies by specifying `std::hardware_destructive_interference_size`.
	 * July 14th: Do not use `std::hardware_destructive_interference_size`. Use a constant instead.
	 */
	alignas(SAFE_CACHE_LINE_SIZE)std::atomic<std::uint32_t> write_idx_{0};
	alignas(SAFE_CACHE_LINE_SIZE)std::atomic<std::uint32_t> read_idx_{0};

	ItemType buffer_[Cap]{};
};

}

#endif // WIT_RING_BUFFER_H
