/**
 * Created by intwi on 2026/06/26.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_VOICE_H
#define WIT_VOICE_H

#include <array>
#include <cstdint>

#include "biquad.h"
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
    PAUSING,        ///< Fading out toward PAUSED; still audible this callback.
    PAUSED,         ///< Retains cursor state.
    STOPPING,       ///< Fading out toward FINISHED; still audible this callback.
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

	/**
 	 * @struct Filter
 	 * @brief Runtime low-pass filter applied to this Voice's mixed output.
 	 * @note Coefficients are supplied by the main thread via a command.
 	 *       State is owned by the audio thread and reset when (re)enabled.
 	 */
	struct Filter {
		BiquadCoeffs coeffs{};					///< Set from the main thread.
		float        mix    = 1.0f;				///< Wet amount in [0, 1].
		bool         active = false;			///< When false the filter is bypassed.
		std::array<BiquadState, 2> channel{};	///< Per-channel state (audio thread).
	};

	/**
	 * @struct Tape
	 * @brief Runtime tape effect: a speed ramp driving this Voice's playback rate.
	 */
	struct Tape {
		float rise_step = 0.0f;		///< Per-sample increment used on play / resume.
		float fall_step = 0.0f;		///< Per-sample decrement used on stop / pause.
		float position  = 1.0f;		///< Ramp position in [0, 1]: 1 is normal speed, 0 is stopped.
		float step      = 0.0f;		///< Signed increment applied this callback, 0 when settled.
		bool  active    = false;	///< When false the Voice behaves normally.
	};

	static constexpr float TAPE_MIN_POSITION	= 0.0001f;
	static constexpr float TAPE_MIN_RATE		= 0.00001f;
	static constexpr float TAPE_OCTAVE_RANGE	= 3.0f;
	static constexpr float TAPE_GAIN_TAPER		= 0.5f;

    std::array<Slot, MAX_WAVEFORMS_PER_CUE> slots{};	///< Waveform slots
	Filter      filter{};								///< Runtime filter, off by default
	Tape        tape{};									///< Runtime tape effect, off by default

    VoiceState  state             = VoiceState::INACTIVE;
    uint8_t     active_slot_count = 0;
    uint16_t    category_id       = 0;
    bool        loop_enabled      = false;
    float       volume            = 1.0f;
    float       pitch             = 1.0f;

    float       fade_gain         = 1.0f;
    float       fade_step         = 0.0f;

	uint64_t    loop_start        = 0;
    uint64_t    loop_end          = 0;	///< 0 means no looping

    const CueData* cue            = nullptr;
    CuePlayer*     owner		  = nullptr;
};

}

#endif // WIT_VOICE_H
