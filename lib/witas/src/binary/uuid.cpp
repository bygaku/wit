/**
 * Created by intwi on 2026/05/28.
 * Copyright (c) 2026 All rights reserved.
 */
#include "uuid.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace wit::binary {

Uuid Uuid::FromBytes(const uint8_t* src) noexcept {
    Uuid uuid;
    std::memcpy(uuid.bytes_.data(), src, 16);
    return uuid;
}

bool Uuid::IsNil() const noexcept {
    return std::all_of(bytes_.begin(), bytes_.end(),
                       [](uint8_t b) { return b == 0; });
}

std::string Uuid::ToHexString() const {
    char buffer[37];
    std::snprintf(buffer, sizeof(buffer),
                  "%02x%02x%02x%02x-"
                  "%02x%02x-"
                  "%02x%02x-"
                  "%02x%02x-"
                  "%02x%02x%02x%02x%02x%02x",
                  bytes_[0],  bytes_[1],  bytes_[2],  bytes_[3],
                  bytes_[4],  bytes_[5],
                  bytes_[6],  bytes_[7],
                  bytes_[8],  bytes_[9],
                  bytes_[10], bytes_[11], bytes_[12], bytes_[13], bytes_[14], bytes_[15]);
    return std::string(buffer);
}

}  // namespace wit::binary
