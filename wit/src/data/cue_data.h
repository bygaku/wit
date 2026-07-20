/**
 * Created by intwi on 2026/07/05.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CUE_DATA_H
#define WIT_CUE_DATA_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "waveform_info.h"

namespace wit {

constexpr std::size_t MAX_WAVEFORMS_PER_CUE = 16;	///< Maximum number of waveforms in a cue
constexpr std::size_t MAX_CUE_NAME_LENGTH	= 32;	///< Maximum length of cue name
typedef   std::array<uint8_t, 36> FIELD_3D;			///< HACK: Extended field for v2.0, To fix the memory layout.

/**
 * @enum CueType
 * @brief Policy for handling multiple waveforms within a cue.
 */
enum class CueType : uint8_t {
	POLYPHONIC		= 0,	///< Play all waveforms simultaneously (default)
	SHUFFLE			= 1,	///< On each Play, pick one waveform at random
	SEQUENTIAL		= 2,	///< On each Play, pick the next waveform in sequence (cursor-based)
};

/**
 * @enum StreamingMode
 * @brief Whether the waveform is streamed or loaded into memory.
 */
enum class StreamingMode : uint8_t {
	MEMORY_RESIDENT = 0,	///< Full expand on memory (default)
	STREAMING		= 1,	///< Stream from the ref file on demand
};

/**
 * @struct RandomizeRange
 * @brief Randomization range for volume or pitch.
 */
struct RandomizeRange {
	bool  enabled	= false;
	float min		= 1.0f;
	float max		= 1.0f;
};

// HACK: Consider the memory layout
/**
 * @struct CueData
 * @brief Complete definition of Cue.
 *
 * @note Held in a std::vector inside Wccb;
 * Voices reference CueData by const pointer.
 * Immutable for the lifetime of the owning Wccb.
 */
struct CueData {
	/* =====================================================================
	 * Registered
	 * ===================================================================== */
    std::array<WaveformInfo, MAX_WAVEFORMS_PER_CUE> waveforms;

	/* =====================================================================
	 * Identify
	 * ===================================================================== */
	std::array<char, MAX_CUE_NAME_LENGTH> cue_name = {0};	///< Cue name used as the lookup key

	/* =====================================================================
	 * Cue Parameters
	 * ===================================================================== */
	RandomizeRange	volume_random;				///< Parameter for the random range of volume (default: enabled = false)
	RandomizeRange	pitch_random;				///< Parameter for the random range of pitch (default: enabled = false)
	float			base_volume		= 1.0f;		///< Base volume in [0.0, 1.0]
	float			base_pitch		= 1.0f;		///< Base pitch (recommended range 0.5 - 2.0)
	uint64_t		loop_start_point = 0;		///<
	uint64_t		loop_end_point	 = 0;		///<

	/* =====================================================================
	 * Identify
	 * ===================================================================== */
	uint32_t		cue_id			= 0;	///< Auto-assigned by WitStudio

	/* =====================================================================
	 * Cue Parameters
	 * ===================================================================== */
	uint32_t		waveform_count	= 0;
    uint16_t		category_id		= 0;	///< Category this cue belongs to
    CueType			cue_type		= CueType::POLYPHONIC;
    StreamingMode	streaming_mode	= StreamingMode::MEMORY_RESIDENT;
    bool			loop_enabled	= true;

	/* =====================================================================
	 * Extended Parameters
	 * ===================================================================== */
    bool			is_3d			= false;	///< Reserved for v2.0 3D audio
    FIELD_3D		reserved_3d		= {};		///< 36-byte reserved area for v2.0 3D fields
};

}

#endif  // WIT_CUE_DATA_H
