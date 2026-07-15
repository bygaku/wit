/**
 * Created by intwi on 2026/06/28.
 * Copyright (c) 2026 All rights reserved.
 */

#ifndef WIT_RIFF_CHUNK_ITERATOR_H
#define WIT_RIFF_CHUNK_ITERATOR_H

#include <cstddef>
#include <cstdint>

#include "fourcc.h"

namespace wit::binary {
/**
 * @class RiffChunkIterator
 * @brief Scan the chunks of a RIFF-format file sequentially.
 */
class RiffChunkIterator {
public:
    RiffChunkIterator() = default;

	/**
     * @brief Stores the Chunk's FourCC and size
     * and retains a pointer to the beginning of the Chunk's data section for as long as this class exists.
     */
    struct Chunk {
        CHUNK_ID        chunk_id   = 0;			///< FourCC
        CHUNK_SIZE      chunk_size = 0;			///< Data block size
        const uint8_t*  data       = nullptr;	///< Start of a data section
    };

    /**
     * @brief Validate the RIFF Header.
     * @retval True Success.
     * @retval False RIFF magic mismatch, a `form_type` mismatch, or an invalid size.
     */
    static bool Open(const uint8_t*			data, std::size_t		size,
                     uint32_t expected_form_type, RiffChunkIterator& out) noexcept;

    /**
     * @brief Get the next chunk.
     */
    bool Next(Chunk& out) noexcept;

    /**
     * @brief Whether an error occurred during the scan (e.g., truncation, excessive chunk_size, etc.).
     */
    [[nodiscard]] bool HasError() const noexcept { return has_error_; }

    /**
     * @brief RIFF form_type.
     */
    [[nodiscard]] uint32_t FormType() const noexcept { return form_type_; }

private:
    const uint8_t* data_       = nullptr;
    std::size_t    size_       = 0;
    std::size_t    cursor_     = 0;
    uint32_t       form_type_  = 0;
    bool           has_error_  = false;
};

}

#endif // WIT_RIFF_CHUNK_ITERATOR_H
