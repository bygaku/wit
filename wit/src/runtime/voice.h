/**
 * Created by intwi on 2026/06/26.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_VOICE_H
#define WIT_VOICE_H

#include <array>
#include <cstdint>

#include "data/cue_data.h"

namespace wit {

class CuePlayer;
class IWaveformProvider;
class StreamingWaveformProvider;
struct CueData;

/**
 * @enum VoiceState
 * @brief Lifecycle state of a Voice slot.
 */
enum class VoiceState : uint8_t {
    INACTIVE = 0,   ///< Slot is free and may be acquired.
    CLAIMED,        ///< Acquired but caller has not yet transitioned to PLAYING.
    PREPARING,      ///< Waiting for streaming buffers to fill before PLAYING.
    PLAYING,        ///< Seek samples every audio callback.
    PAUSED,         ///< Retains cursor state.
    STOPPING,       ///< Will transition to FINISHED.
    FINISHED,       ///< Playback ended.
};

/**
 * @struct Voice
 * @brief A Voice represents one active playback instance of a Cue.
 */
struct Voice {
    /**
     * @struct Slot
     * @brief One waveform bound to this Voice.
     * @note Cleared to defaults when the Voice is INACTIVE.
     */
    struct Slot {
        IWaveformProvider*         provider  = nullptr;	///< One waveform bound to this Voice
        StreamingWaveformProvider* streaming = nullptr;	///< Same provider as a concrete pointer, nullptr if memory resident
        double                     cursor    = 0.0;		///< Per-channel sample position on the cue timeline
    };

    std::array<Slot, MAX_WAVEFORMS_PER_CUE> slots{};	///< Waveform slots

    VoiceState  state             = VoiceState::INACTIVE;
    uint8_t     active_slot_count = 0;
    uint16_t    category_id       = 0;
    bool        loop_enabled      = false;
    float       volume            = 1.0f;
    float       pitch             = 1.0f;

    /* Loop region resolved at Play time (POLYPHONIC only). loop_end == 0 means no looping. */
    uint64_t    loop_start        = 0;
    uint64_t    loop_end          = 0;

    const CueData* cue            = nullptr;
    CuePlayer*     owner		  = nullptr;
};

}

#endif // WIT_VOICE_H
