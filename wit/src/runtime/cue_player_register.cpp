/**
 * Created by intwi on 2026/07/05.
 * Copyright (c) 2026 All rights reserved.
 */
#include "cue_player_register.h"

#include "cue_player.h"

namespace wit {

WitCuePlayerHn CuePlayerRegister::Create() {
    auto player = std::make_unique<CuePlayer>();
    const WitCuePlayerHn handle = allocator_.Next();
    players_.emplace(handle, std::move(player));
    return handle;
}

void CuePlayerRegister::Destroy(WitCuePlayerHn handle) noexcept {
    if (handle == WIT_INVALID_CUE_PLAYER_HN) return;
    players_.erase(handle);
}

CuePlayer* CuePlayerRegister::Find(WitCuePlayerHn handle) noexcept {
    if (handle == WIT_INVALID_CUE_PLAYER_HN) return nullptr;
    const auto itr = players_.find(handle);
    return itr != players_.end() ? itr->second.get() : nullptr;
}

const CuePlayer* CuePlayerRegister::Find(WitCuePlayerHn handle) const noexcept {
    if (handle == WIT_INVALID_CUE_PLAYER_HN) return nullptr;
    const auto itr = players_.find(handle);
    return itr != players_.end() ? itr->second.get() : nullptr;
}

}