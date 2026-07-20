/**
 * Created by intwi on 2026/05/28.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_UUID_H
#define WIT_UUID_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace wit::binary {

/**
 * @brief A 16-byte UUID. The byte order follows network order (big-endian notation as defined in RFC 4122).
 *
 * Embedded in the HEAD chunk of .wpb, .wccb, and .wwb files and used to verify consistency between files.
 */
class Uuid {
public:
    /**
     * @brief Initialize with a nil UUID (all zeros).
     */
    constexpr Uuid() noexcept : bytes_{} {}

    /**
     * @brief Construct it from a 16-byte array.
     */
    constexpr explicit Uuid(const std::array<uint8_t, 16>& bytes) noexcept
        : bytes_(bytes) {}

    /**
     * @brief Construct it from a 16-byte raw buffer.
     */
    static Uuid FromBytes(const uint8_t* src) noexcept;

    /**
     * @brief Returns a reference to the internal 16-byte value.
     */
    const std::array<uint8_t, 16>& Bytes() const noexcept { return bytes_; }

    /**
	* @brief Returns true if the UUID consists entirely of zeros.
     */
    bool IsNil() const noexcept;

    /**
     * @brief Convert to a string using the standard 8-4-4-4-12 hex notation.
     */
    std::string ToHexString() const;

    bool operator==(const Uuid& rhs) const noexcept { return bytes_ == rhs.bytes_; }
    bool operator!=(const Uuid& rhs) const noexcept { return !(*this == rhs); }

private:
    std::array<uint8_t, 16> bytes_;
};

}  // namespace wit::binary
#endif // WIT_UUID_H