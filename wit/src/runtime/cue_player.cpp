/**
 * Created by intwi on 2026/07/05.
 * Copyright (c) 2026 All rights reserved.
 */
#include "cue_player.h"

#include <algorithm>

namespace wit {

WitResult CuePlayer::AttachCue(WitWccbHn collection_handle, const CueData* cue) {
    if (collection_handle == WIT_INVALID_WCCB_HN) return WIT_RESULT_NO_CUE_ATTACHED;
    if (cue == nullptr)                           return WIT_RESULT_NO_CUE_ATTACHED;

    collection_handle_ = collection_handle;
    cue_               = cue;
	sequential_index_  = 0;
	shuffle_order_.clear();		///< Regenerated lazily on the next Play
	shuffle_cursor_    = 0;

	return WIT_RESULT_SUCCESS;
}

void CuePlayer::RegisterSpawnedVoice(WitVoiceHn handle) {
    if (handle == WIT_INVALID_VOICE_HN) return;
    spawned_voices_.push_back(handle);
}

void CuePlayer::ForgetVoice(WitVoiceHn handle) noexcept {
    if (handle == WIT_INVALID_VOICE_HN) return;
    auto itr = std::find(spawned_voices_.begin(), spawned_voices_.end(), handle);
    if (itr != spawned_voices_.end()) {
        spawned_voices_.erase(itr);
    }
}

}
