/**
 * Created by intwi on 2026/07/03.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CATEGORY_INFO_H
#define WIT_CATEGORY_INFO_H

#include <cstdint>
#include <string>

namespace wit {
/**
 * @struct CategoryInfo
 * @brief Definition of a Category.
 * @note It's not used in v1.0
 */
struct CategoryInfo {
	std::string name{};                          ///< Category name (UTF-8)
	uint16_t    id                     = 0;      ///< Presets: 0-4, User defined: 5+
	float       static_volume_db       = 0.0f;   ///< Values to be burned in during the build.
	float       ducking_attenuation_db = -6.0f;  ///< Attenuation during ducking (dB)
	bool        is_preset              = false;  ///< If it's a preset category: true
	bool        is_ducker              = false;  ///< If it's the one dodging others, true.
};

}

#endif // WIT_CATEGORY_INFO_H