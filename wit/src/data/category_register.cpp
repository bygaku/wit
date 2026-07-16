/**
 * Created by intwi on 2026/07/03.
 * Copyright (c) 2026 All rights reserved.
 */
#include "category_register.h"

#include <algorithm>

namespace wit {

bool CategoryRegister::Add(const CategoryInfo& info) {
    if (Contains(info.id)) return false;
    categories_.push_back(info);

    return true;
}

const CategoryInfo* CategoryRegister::Find(uint16_t id) const noexcept {
    auto itr = std::find_if(categories_.begin(), categories_.end(),
                           [id](const CategoryInfo& c) { return c.id == id; });

	return itr != categories_.end() ? &(*itr) : nullptr;
}

bool CategoryRegister::Contains(uint16_t id) const noexcept {
    return Find(id) != nullptr;
}

float CategoryRegister::GetCurrentLinearGain(uint16_t category_id) const noexcept {
    // v1.0 - beta: Ducking not implemented.
    // v1.x
	//   1. Determine whether the category with category_id is being ducked by Ducker
	//   2. If so, return the value from the ducking_attenuation_db multiplied by a linear factor
	//   3. If fading is in progress, return the current interpolated value
	(void)category_id;
    return 1.0f;
}

}
