/**
 * Created by intwi on 2026/07/16.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_SAMPLE_DECODE_H
#define WIT_SAMPLE_DECODE_H

#include <cstdint>
#include <cstring>

namespace wit::decode {

/**
 * @brief Decode a little-endian int16 sample to normalized float.
 */
inline float ReadInt16LE(const uint8_t* p, std::size_t size) noexcept {
	if (size != 2) return 0.0f;

	const auto v = static_cast<int16_t>(static_cast<uint16_t>(p[0]) |
										static_cast<uint16_t>(p[1]) << 8);

	return static_cast<float>(v) * (1.0f / 32768.0f);
}

/**
 * @brief Decode a little-endian float32 sample.
 */
inline float ReadFloat32LE(const uint8_t* p, std::size_t size) noexcept {
	if (size != 4) return 0.0f;

	const uint32_t bits =
			static_cast<uint32_t> (p[0])      |
			static_cast<uint32_t>(p[1]) << 8  |
			static_cast<uint32_t>(p[2]) << 16 |
			static_cast<uint32_t>(p[3]) << 24;

	float f;
	std::memcpy(&f, &bits, sizeof(f));
	return f;
}

}

#endif // WIT_SAMPLE_DECODE_H
