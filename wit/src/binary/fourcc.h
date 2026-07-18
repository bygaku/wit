/**
 * Created by intwi on 2026/05/28.
 * Copyright (c) 2026 All rights reserved.
 */

#ifndef WIT_FOUR_CC_H
#define WIT_FOUR_CC_H

#include <array>
#include <cstdint>

namespace wit::binary {
typedef uint32_t CHUNK_ID;		///< FourCC for chunks
typedef uint32_t CHUNK_SIZE;	///< The size of the chunk that follows

/**
 * @brief Creates a new 4-character code from four characters.
 * @code
 *	constexpr CHUNK_ID EXMP_ID = MakeFourCc('E', 'X', 'M', 'P');
 * @endcode
 */
constexpr uint32_t MakeFourCc(char a, char b, char c, char d) noexcept {
    return  static_cast<uint32_t>(static_cast<unsigned char>(a))
         | (static_cast<uint32_t>(static_cast<unsigned char>(b)) << 8)
         | (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 16)
         | (static_cast<uint32_t>(static_cast<unsigned char>(d)) << 24);
}

/**
 * @brief Convert a FourCC to a human-readable 5-character string (NULL-terminated).
 * For logging purposes.
 */
constexpr std::array<char, 5> FourCcToString(uint32_t fourcc) noexcept {
    return {
        static_cast<char>( fourcc        & 0xFF),
        static_cast<char>((fourcc >>  8) & 0xFF),
        static_cast<char>((fourcc >> 16) & 0xFF),
        static_cast<char>((fourcc >> 24) & 0xFF),
        '\0'
    };
}

/* =====================================================================
 * FourCC
 * ===================================================================== */

// Container identifier
constexpr CHUNK_ID FOURCC_RIFF      = MakeFourCc('R', 'I', 'F', 'F');
constexpr CHUNK_ID FOURCC_FORM_WPB  = MakeFourCc('W', 'P', 'B', ' ');
constexpr CHUNK_ID FOURCC_FORM_WCCB = MakeFourCc('W', 'C', 'C', 'B');
constexpr CHUNK_ID FOURCC_FORM_WWB  = MakeFourCc('W', 'W', 'B', ' ');

// Chunk for general
constexpr CHUNK_ID FOURCC_HEAD      = MakeFourCc('H', 'E', 'A', 'D');

// Chunks for .wpb
constexpr CHUNK_ID FOURCC_FMT       = MakeFourCc('F', 'M', 'T', ' ');
constexpr CHUNK_ID FOURCC_DUCK      = MakeFourCc('D', 'U', 'C', 'K');
constexpr CHUNK_ID FOURCC_CATE      = MakeFourCc('C', 'A', 'T', 'E');

// Chunks for .wccb
constexpr CHUNK_ID FOURCC_USEC      = MakeFourCc('U', 'S', 'E', 'C');
constexpr CHUNK_ID FOURCC_CUES      = MakeFourCc('C', 'U', 'E', 'S');

// Chunk for .wwb
constexpr CHUNK_ID FOURCC_DATA      = MakeFourCc('D', 'A', 'T', 'A');

// Versions outside the scope
constexpr CHUNK_ID FOURCC_PRIO      = MakeFourCc('P', 'R', 'I', 'O');
constexpr CHUNK_ID FOURCC_EFX       = MakeFourCc('E', 'F', 'X', ' ');

}  // namespace wit::binary
#endif //WIT_FOUR_CC_H