/**
 * Created by intwi on 2026/06/28.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_BINARY_READER_H
#define WIT_BINARY_READER_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace wit::binary {

class Uuid;

class BinaryReader {
public:
    /**
	 * @brief Specifies the buffer to be read from.
	 * @param data The start of the buffer. It must remain in existence for the duration of the BinaryReader's lifetime.
	 * @param size The buffer size (in bytes).
     */
    BinaryReader(const uint8_t* data, std::size_t size) noexcept;

	/* =====================================================================
	 * Integer
	 * ===================================================================== */
    bool ReadU8(uint8_t&   out) noexcept;
    bool ReadU16(uint16_t& out) noexcept;
    bool ReadU32(uint32_t& out) noexcept;
    bool ReadU64(uint64_t& out) noexcept;
    bool ReadS8(int8_t&    out) noexcept;
    bool ReadS16(int16_t&  out) noexcept;
    bool ReadS32(int32_t&  out) noexcept;
    bool ReadS64(int64_t&  out) noexcept;

	/* =====================================================================
	 * Floating point
	 * ===================================================================== */
    bool ReadF32(float&  out) noexcept;
    bool ReadF64(double& out) noexcept;

	/* =====================================================================
	 * Raw
	 * ===================================================================== */
    bool ReadBytes(void* dst, std::size_t size) noexcept;

	/* =====================================================================
	 * for FourCC
	 * ===================================================================== */
    bool ReadFourCc(uint32_t& out) noexcept;

	/* =====================================================================
	 * UUID
	 * ===================================================================== */
    bool ReadUuid(Uuid& out) noexcept;

	/* =====================================================================
	 * String
	 * ===================================================================== */
    bool ReadString(std::string& out);

    /**
     * @brief Moves the cursor n bytes forward. Returns false if the position is out of bounds.
     */
    bool Skip(std::size_t n) noexcept;

    /**
     * @brief Move the cursor to an absolute position.
     */
    bool SeekAbsolute(std::size_t position) noexcept;

	/* =====================================================================
	 * Accessors
	 * ===================================================================== */
    [[nodiscard]] std::size_t Position()  const noexcept { return position_; }
    [[nodiscard]] std::size_t Size()      const noexcept { return size_; }
    [[nodiscard]] std::size_t Remaining() const noexcept { return size_ - position_; }
    [[nodiscard]] bool        AtEnd()     const noexcept { return position_ >= size_; }

    /**
     * @brief Pointer to the start of the byte at the current position.
     */
    [[nodiscard]] const uint8_t* Cursor() const noexcept { return data_ + position_; }

private:
    const uint8_t* data_;
    std::size_t    size_;
    std::size_t    position_;
};

}
#endif // WIT_BINARY_READER_H