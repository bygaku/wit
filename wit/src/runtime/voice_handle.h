/**
 * Created by ClaudeCode on 2026/07/02.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_VOICE_HANDLE_H
#define WIT_VOICE_HANDLE_H

#include <cstdint>

#include <wit_types.h>

// A WitVoiceHn is an opaque pointer-typed handle whose bits encode:
//   bit 0-7  : slot index (0-63; VoicePool has 64 slots)
//   bit 8-31 : generation counter (>= 1 for valid handles)
// Handle value 0 (all zero) is reserved as WIT_INVALID_VOICE_HN, so
// generations start at 1 to guarantee the encoding never collides
// with the invalid sentinel.
namespace wit::voice_handle {

inline WitVoiceHn Encode(uint8_t slot, uint32_t generation) noexcept {
    const uintptr_t v =
        (static_cast<uintptr_t>(generation) << 8) |
        static_cast<uintptr_t>(slot);
    return reinterpret_cast<WitVoiceHn>(v);
}

inline uint8_t DecodeSlot(WitVoiceHn h) noexcept {
    return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(h) & 0xFFu);
}

inline uint32_t DecodeGeneration(WitVoiceHn h) noexcept {
    return static_cast<uint32_t>(
        (reinterpret_cast<uintptr_t>(h) >> 8) & 0xFFFFFFu);
}

}

#endif // WIT_VOICE_HANDLE_H
