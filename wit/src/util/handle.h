/**
 * Created by intwi on 2026/07/02.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_HANDLE_H
#define WIT_HANDLE_H

#include <cstdint>

namespace wit {

/**
 * @class HandleAllocator
 * @tparam HandleType Wit***Hn
 * @brief Monotonically increasing opaque handle allocator.
 *
 * @note Return a distinct value cast to HandleType (a pointer typedef in the public C ABI).
 * Values start at 1; the value 0 is reserved for the invalid sentinel (WIT_INVALID_*_HN).
 */
template <typename HandleType>
class HandleAllocator {
public:
    HandleAllocator() = default;

#pragma region
    HandleAllocator(const HandleAllocator&)            = delete;
    HandleAllocator& operator=(const HandleAllocator&) = delete;
    HandleAllocator(HandleAllocator&&)                 = delete;
    HandleAllocator& operator=(HandleAllocator&&)      = delete;
#pragma endregion

    /**
     * @brief Returns the next unique handle value, cast into HandleType.
     * @note Values start at 1 (0 is reserved as the invalid sentinel).
     */
    HandleType Next() noexcept {
        ++counter_;
        return reinterpret_cast<HandleType>(counter_);
    }

private:
    uintptr_t counter_ = 0;
};

}

#endif // WIT_HANDLE_H
