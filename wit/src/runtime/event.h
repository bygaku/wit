/**
 * Created by intwi on 2026/06/02.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_EVENT_H
#define WIT_EVENT_H

#include <cstdint>
#include <type_traits>

namespace wit {

/**
 * @enum EventType
 * @brief Kinds of notifications the audio thread sends back to the main thread.
 */
enum class EventType : uint8_t {
    NONE = 0,          ///< Padding / uninitialized.
    VOICE_FINISHED,    ///< Voice reached the end of playback.
};

/**
 * @struct Event
 * @brief An event refers to a specific Voice slot.
 */
struct Event {
    EventType type       = EventType::NONE;
    uint8_t   slot_index = 0;
    uint32_t  generation = 0;
};

static_assert(std::is_trivially_copyable_v<Event>,
              "Event must be trivially copyable to fit in RingBuffer");
static_assert(sizeof(Event) == 8,
              "Event should stay compact (target 8 bytes)");

}

#endif // WIT_EVENT_H
