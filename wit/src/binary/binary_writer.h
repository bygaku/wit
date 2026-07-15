/**
 * Created by intwi on 2026/06/28.
 * Copyright (c) 2026 All rights reserved.
 */

#ifndef WIT_BINARY_WRITER_H
#define WIT_BINARY_WRITER_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace wit::binary {

class Uuid;
/**
 * @class BinaryWriter
 * @brief Writing to a variable-length byte buffer.
 */
class BinaryWriter {
public:
    BinaryWriter() = default;

	/* =====================================================================
	 * Integer
	 * ===================================================================== */
    void WriteU8(uint8_t   v);
    void WriteU16(uint16_t v);
    void WriteU32(uint32_t v);
    void WriteU64(uint64_t v);
    void WriteS8(int8_t    v);
    void WriteS16(int16_t  v);
    void WriteS32(int32_t  v);
    void WriteS64(int64_t  v);

	/* =====================================================================
	 * Floating point
	 * ===================================================================== */
    void WriteF32(float  v);
    void WriteF64(double v);

	/* =====================================================================
	 * Raw
	 * ===================================================================== */
    void WriteBytes(const void* src, std::size_t size);

	/* =====================================================================
	 * for FourCC
	 * ===================================================================== */
    void WriteFourCc(uint32_t v);

	/* =====================================================================
	 * UUID
	 * ===================================================================== */
    void WriteUuid(const Uuid& v);

	/* =====================================================================
	 * String
	 * ===================================================================== */
    bool WriteString(std::string_view s);

    /**
	 * @brief Reserves a 4-byte placeholder at the current position and returns its starting offset.
	 */
    std::size_t ReserveU32();

    /**
  	 * @brief Overwrites the specified offset with a 4-byte uint32.
	 * @retval True Success.
	 * @retval False If the offset is out of range.
	 */
    bool PatchU32(std::size_t offset, uint32_t v);

    [[nodiscard]] const std::vector<uint8_t>& Bytes() const noexcept { return buffer_; }
    [[nodiscard]] std::size_t                 Size()  const noexcept { return buffer_.size(); }

private:
    std::vector<uint8_t> buffer_;
};

}

#endif // WIT_BINARY_WRITER_H