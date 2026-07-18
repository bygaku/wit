/**
 * Created by intwi on 2026/06/02.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_COMMAND_H
#define WIT_COMMAND_H

#include <cstdint>
#include <type_traits>

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
};

/**
 * @struct Command
 * @brief A command targets a specific Voice slot.
 */
struct Command {
    CommandType type       = CommandType::NONE;
    uint8_t     slot_index = 0;
    uint32_t    generation = 0;
};

static_assert(std::is_trivially_copyable_v<Command>,
              "Command must be trivially copyable to fit in RingBuffer");
static_assert(sizeof(Command) == 8,
              "Command should stay compact (target 8 bytes)");

}

#endif // WIT_COMMAND_H
