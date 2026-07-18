/**
 * Created by intwi on 2026/06/26.
 * Copyright (c) 2026 All rights reserved.
 */
#include "voice_pool.h"

#include "voice_handle.h"

namespace wit {

VoicePool::VoicePool()
	: voices_{}
	, generations_{} {
    for (uint32_t& g : generations_) g = 1; ///< 0 is the invalid handle.
}

WitVoiceHn VoicePool::Acquire() noexcept {
    for (size_t i = 0; i < MAX_VOICE_COUNT; ++i) {
        if (voices_[i].state == VoiceState::INACTIVE) {
            // Mark the slot as reserved so a subsequent Acquire cannot pick it up.
            voices_[i].state = VoiceState::CLAIMED;
            return voice_handle::Encode(static_cast<uint8_t>(i),
                                         generations_[i]);
        }
    }
    return WIT_INVALID_VOICE_HN;
}

void VoicePool::Release(WitVoiceHn handle) noexcept {
    if (handle == WIT_INVALID_VOICE_HN) return;

    const uint8_t  slot = voice_handle::DecodeSlot(handle);
    const uint32_t gen  = voice_handle::DecodeGeneration(handle);

    if (slot >= MAX_VOICE_COUNT)       return;
    if (generations_[slot] != gen)     return;

    ++generations_[slot];
    if (generations_[slot] == 0) generations_[slot] = 1;

    voices_[slot] = Voice{};
}

Voice* VoicePool::Find(WitVoiceHn handle) noexcept {
    if (handle == WIT_INVALID_VOICE_HN) return nullptr;

    const uint8_t  slot = voice_handle::DecodeSlot(handle);
    const uint32_t gen  = voice_handle::DecodeGeneration(handle);

    if (slot >= MAX_VOICE_COUNT)       return nullptr;
    if (generations_[slot] != gen)     return nullptr;

    return &voices_[slot];
}

const Voice* VoicePool::Find(WitVoiceHn handle) const noexcept {
    if (handle == WIT_INVALID_VOICE_HN) return nullptr;

    const uint8_t  slot = voice_handle::DecodeSlot(handle);
    const uint32_t gen  = voice_handle::DecodeGeneration(handle);

    if (slot >= MAX_VOICE_COUNT)       return nullptr;
    if (generations_[slot] != gen)     return nullptr;

    return &voices_[slot];
}

size_t VoicePool::ActiveCount() const noexcept {
    size_t n = 0;
    for (const Voice& v : voices_) {
        if (v.state != VoiceState::INACTIVE) ++n;
    }
    return n;
}

}
