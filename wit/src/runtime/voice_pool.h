/**
 * Created by intwi on 2026/06/26.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_VOICE_POOL_H
#define WIT_VOICE_POOL_H

#include <array>
#include <cstddef>
#include <cstdint>

#include <wit_types.h>

#include "voice.h"

namespace wit {

/**
 * @class VoicePool
 * @brief Objects pool of Voice slots with generation-tracked handles.
 */
class VoicePool {
public:
    static constexpr size_t MAX_VOICE_COUNT = 64;	///< fixed-size

    VoicePool();
    ~VoicePool() = default;

#pragma region prohibit
    VoicePool(const VoicePool&)            = delete;
    VoicePool& operator=(const VoicePool&) = delete;
    VoicePool(VoicePool&&)                 = delete;
    VoicePool& operator=(VoicePool&&)      = delete;
#pragma endregion

    /**
     * @brief Find an INACTIVE slot and mark it as CLAIMED.
     * The caller populates fields and then flips state to PLAYING when configuration is complete.
     * @return VoiceHandle
     * @retval WIT_INVALID_VOICE_HN(0) when every slot is in use.
     */
    WitVoiceHn Acquire() noexcept;

    /**
     * @brief Release the slot referenced by the handle.
     */
    void Release(WitVoiceHn handle) noexcept;

    /**
     * @brief Resolve handle to a Voice pointer.
	 * @retval Voice*
	 * @retval nullptr Invalid or stale handles.
     */
    Voice*       Find(WitVoiceHn handle)	   noexcept;

	/**
	 * @brief Resolve handle to a CuePlayer pointer.
	 * @note Read-only
	 * @retval const Voice*
	 * @retval nullptr Invalid or stale handles.
	 */
    const Voice* Find(WitVoiceHn handle) const noexcept;

	/// HACK: Accessing `MAX_VOICE_COUNT` in a `for` loop. but beware of out-of-range access!!
    /**
     * @brief Access to the voice slot directly.
     */
    Voice& At					 (size_t slot_index)	   noexcept { return voices_[slot_index]; }

	/**
	 * @brief Access to the voice slot directly.
	 * @note Read-only
	 */
	[[nodiscard]] const Voice& At(size_t slot_index) const noexcept { return voices_[slot_index]; }

    /**
     * @brief Current generation counter for a slot.
     * Used by the audio thread to build a valid handle for the current occupant when posting a natural-completion event.
     */
    [[nodiscard]] uint32_t SlotGeneration(size_t slot_index) const noexcept { return generations_[slot_index]; }

    /**
     * @brief Maximum amount of simultaneous Voice.
     */
    static constexpr size_t Capacity() noexcept { return MAX_VOICE_COUNT; }

    /**
     * @brief Number of slots currently not in the INACTIVE state.
     */
    [[nodiscard]] size_t ActiveCount() const noexcept;

private:
    std::array<Voice,	 MAX_VOICE_COUNT> voices_;
    std::array<uint32_t, MAX_VOICE_COUNT> generations_;
};

}

#endif // WIT_VOICE_POOL_H
