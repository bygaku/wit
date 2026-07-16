/**
 * Created by intwi on 2026/07/03.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_DUCKING_SETTINGS_H
#define WIT_DUCKING_SETTINGS_H

#include <cstdint>

namespace wit {

/**
 * @struct DuckingSettings
 * @brief Ducking fade settings common to the entire project.
 * @note It's not used in v1.0
 */
struct DuckingSettings {
    uint32_t fade_in_ms  = 200;
    uint32_t fade_out_ms = 500;
};

}

#endif // WIT_DUCKING_SETTINGS_H
