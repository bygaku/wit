/**
 * Created by intwi on 2026/07/05.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CUE_PLAYER_H
#define WIT_CUE_PLAYER_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <wit_types.h>

namespace wit {

struct CueData;

/**
 * @class CuePlayer
 * @brief A single CuePlayer binds to one cue (via AttachCue) and can play it any number of times.
 */
class CuePlayer {
public:
    CuePlayer()  = default;
    ~CuePlayer() = default;

#pragma region
    CuePlayer(const CuePlayer&)            = delete;
    CuePlayer& operator=(const CuePlayer&) = delete;
    CuePlayer(CuePlayer&&)                 = delete;
    CuePlayer& operator=(CuePlayer&&)      = delete;
#pragma endregion

    /**
     * @brief Bind this player to one cue of a loaded collection.
     * The engine resolves `cue` through CueCollectionStore before calling.
     * @retval RESULT_SUCCESS yey
     * @retval NO_CUE_ATTACHED If cue is nullptr or the handle is invalid.
     */
    WitResult AttachCue(WitWccbHn collection_handle, const CueData* cue);

	/* =====================================================================
	 * Read-only accessors
	 * ===================================================================== */
    [[nodiscard]] const CueData*                 AttachedCue()    const noexcept { return cue_; }
    [[nodiscard]] WitWccbHn                      AttachedHandle() const noexcept { return collection_handle_; }
    [[nodiscard]] const std::vector<WitVoiceHn>& SpawnedVoices() const noexcept { return spawned_voices_; }

	/* =====================================================================
	 * Sequential playback state
	 * ===================================================================== */
	[[nodiscard]] uint32_t GetSequentialIndex() const noexcept { return sequential_index_; }
    void     SetSequentialIndex(uint32_t index)		  noexcept { sequential_index_ = index; }

	/* =====================================================================
	 * Shuffle playback state
	 * A permutation is generated once (lazily at first Play) and consumed
	 * one entry per Play. Loop ON restarts the same permutation from the
	 * top; loop OFF saturates on its last entry.
	 * ===================================================================== */
	[[nodiscard]] bool                        HasShuffleOrder() const noexcept { return !shuffle_order_.empty(); }
	[[nodiscard]] const std::vector<uint8_t>& ShuffleOrder()    const noexcept { return shuffle_order_; }
	[[nodiscard]] uint32_t                    ShuffleCursor()   const noexcept { return shuffle_cursor_; }
	void SetShuffleOrder(std::vector<uint8_t> order) noexcept {
		shuffle_order_  = std::move(order);
		shuffle_cursor_ = 0;
	}
	void SetShuffleCursor(uint32_t cursor) noexcept { shuffle_cursor_ = cursor; }

    /**
     * @brief Track a voice that this player just spawned so subsequent StopAll / PauseAll / ResumeAll can find it.
     */
    void RegisterSpawnedVoice(WitVoiceHn handle);

    /**
     * @brief Remove a voice handle from the tracking list.
     */
    void ForgetVoice(WitVoiceHn handle) noexcept;

private:
    WitWccbHn               collection_handle_ = WIT_INVALID_WCCB_HN;
    const CueData*          cue_  = nullptr;
    uint32_t                sequential_index_ = 0;
    std::vector<uint8_t>    shuffle_order_;
    uint32_t                shuffle_cursor_   = 0;
    std::vector<WitVoiceHn> spawned_voices_;
};

}

#endif // WIT_CUE_PLAYER_H
