/**
 * Created by intwi on 2026/06/02.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_COMMAND_H
#define WIT_COMMAND_H

#include <cstdint>
#include <type_traits>

#include "biquad.h"

namespace wit {

/**
 * @enum CommandType
 * @brief Kinds of instructions the main thread sends to the audio thread.
 */
enum class CommandType : uint8_t {
    NONE = 0,			///< Padding / uninitialized.
    START_PLAYING,		///< Flip state to PLAYING.
    PAUSE_VOICE,		///< Freeze slot's cursor advance.
    RESUME_VOICE,   	///< Resume cursor advance from a paused slot.
    STOP_VOICE,     	///< Transition slot toward FINISHED.
	SET_FILTER,			///< Install/replace the voice filter (payload.filter).
	CLEAR_FILTER,		///< Bypass and reset the voice filter.
};

/**
 * @struct FilterCommandPayload
 * @brief Precomputed filter settings carried by SET_FILTER.
 * @note Coefficients are computed on the main thread so the audio thread
 *       never evaluates trigonometry.
 */
struct FilterCommandPayload {
	BiquadCoeffs coeffs{};
	float        mix = 1.0f;
};

/**
 * @struct Command
 * @brief A command targets a specific Voice slot.
 */
struct Command {
    CommandType type       = CommandType::NONE;
    uint8_t     slot_index = 0;
    uint32_t    generation = 0;

	union {
		FilterCommandPayload filter;
	};

	Command() noexcept
		: filter{} {
	}
};

static_assert(std::is_trivially_copyable_v<Command>,
              "Command must be trivially copyable to fit in RingBuffer");

}

#endif // WIT_COMMAND_H
