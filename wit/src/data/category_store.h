/**
 * Created by intwi on 2026/07/03.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CATEGORY_STORE_H
#define WIT_CATEGORY_STORE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "category_info.h"

namespace wit {

/**
 * @class CategoryStore
 * @brief Maintain project category definitions.
 * @note It's not used in v1.0
 */
class CategoryStore {
public:
    CategoryStore() = default;

    /**
     * @brief Add a category.
     * @retval True SUCCESS!
     * @retval False IDs are duplicated
     */
    bool Add(const CategoryInfo& info);

	/**
	 * @brief Search by ID.
	 */
    [[nodiscard]] const CategoryInfo* Find(uint16_t id) const noexcept;

	/**
	 * @brief Verification of ID existence.
	 */
    [[nodiscard]] bool  Contains(uint16_t id) const noexcept;

	/**
	 * @brief Returns the current linear gain factor for the category ID.
	 */
    [[nodiscard]] float GetCurrentLinearGain(uint16_t category_id) const noexcept;

	/**
	 * @brief Number of registered categories.
	 */
    [[nodiscard]] size_t Count() const noexcept { return categories_.size(); }

	/**
	 * @brief Read-only access to internal arrays.
	 */
    [[nodiscard]] const std::vector<CategoryInfo>& All() const noexcept { return categories_; }

private:
    std::vector<CategoryInfo> categories_;
};

}

#endif // WIT_CATEGORY_STORE_H
